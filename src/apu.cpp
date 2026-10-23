#include "apu.h"
#include <cmath>
#include <algorithm>

static inline double midi_to_freq(int p){ return 440.0*std::pow(2.0,(p-69)/12.0); }

static const double DMC_RATE_HZ[16] = {
    4181.71, 4709.17, 5264.23, 5598.97, 6257.91, 7042.71, 7919.35, 8363.42,
    9419.17, 11186.09, 12604.33, 13982.42, 16884.52, 21306.23, 24859.85, 33143.92
};

static inline double env_value(double t,double dur,const EnvParams& e){
    if (t<0) return 0.0;
    double A=e.attack>1e-6?e.attack:1e-6;
    double D=e.decay >1e-6?e.decay :1e-6;
    double R=e.release>1e-6?e.release:1e-6;
    double S=std::max(0.0,std::min(1.0,(double)e.sustain));
    if (t<A) return t/A;
    if (t<A+D) return 1.0-(1.0-S)*((t-A)/D);
    if (t<dur) return S;
    double rt=t-dur;
    if (rt<R) return S*(1.0-rt/R);
    return 0.0;
}

static inline int effective_duty(int base,int mode,float rate,double t){
    if (mode<=0 || rate<0.01f) return base&3;
    int step=(int)(t*rate); if (step<0) step=0;
    switch(mode){
        case 1:{ static const int p[6]={0,1,2,3,2,1}; return p[step%6]; }
        case 2: return step&3;
        case 3: return (step&1)?2:0;
    }
    return base&3;
}

// Шаг envelope-кривой по времени
static inline int env_step(const EnvelopeCurve& e, double t_sec) {
    int sp = e.speed.load(); if (sp<1) sp=1;
    int len = e.length.load();
    if (len<1) len=1;
    if (len>EnvelopeCurve::MAX_STEPS) len=EnvelopeCurve::MAX_STEPS;
    int step = (int)(t_sec*60.0/sp); if (step<0) step=0;
    int ls = e.loop_start.load(), le = e.loop_end.load();
    if (ls>=0 && le>ls && step>le) {
        int lp = le-ls+1;
        step = ls + (step-ls)%lp;
    } else if (step>=len) {
        step = len-1;
    }
    if (step<0) step=0;
    if (step>=EnvelopeCurve::MAX_STEPS) step=EnvelopeCurve::MAX_STEPS-1;
    return step;
}

namespace {
struct FamilyDef { int ch,duty; float a,d,s,r; };
static const FamilyDef FAM[16]={
    {CH_PULSE,2,0.005f,0.400f,0.30f,0.300f},
    {CH_PULSE,1,0.001f,0.300f,0.00f,0.200f},
    {CH_PULSE,3,0.020f,0.050f,1.00f,0.050f},
    {CH_PULSE,0,0.005f,0.500f,0.20f,0.200f},
    {CH_TRI  ,2,0.005f,0.300f,0.50f,0.100f},
    {CH_PULSE,2,0.150f,0.100f,0.80f,0.200f},
    {CH_PULSE,2,0.100f,0.100f,0.90f,0.200f},
    {CH_PULSE,3,0.050f,0.100f,0.70f,0.150f},
    {CH_PULSE,1,0.020f,0.100f,0.80f,0.100f},
    {CH_PULSE,0,0.050f,0.050f,1.00f,0.100f},
    {CH_PULSE,2,0.010f,0.100f,0.80f,0.100f},
    {CH_PULSE,1,0.300f,0.500f,0.70f,0.500f},
    {CH_PULSE,3,0.050f,0.300f,0.50f,0.400f},
    {CH_PULSE,2,0.010f,0.300f,0.50f,0.200f},
    {CH_NOI  ,2,0.001f,0.150f,0.00f,0.100f},
    {CH_NOI  ,2,0.001f,0.100f,0.00f,0.050f},
};
}

// ==== Палитра цветов для 128 GM инструментов ====
uint32_t instrument_default_color(int idx) {
    static const uint32_t CAT_COLORS[16] = {
        0xFF4A90E2, // 0 Piano       — синий
        0xFF50E3C2, // 1 ChromPerc   — бирюзовый
        0xFF7ED321, // 2 Organ       — зелёный
        0xFFF5A623, // 3 Guitar      — оранжевый
        0xFFD0021B, // 4 Bass        — красный
        0xFF9B9B9B, // 5 Strings     — серый
        0xFFF8E71C, // 6 Ensemble    — жёлтый
        0xFF9013FE, // 7 Brass       — фиолетовый
        0xFFB8E986, // 8 Reed        — светл. зелёный
        0xFF2ECC71, // 9 Pipe        — изумруд
        0xFFEC6C9A, // 10 Lead       — розовый
        0xFFB8A98C, // 11 Pad        — бежевый
        0xFF4A4A4A, // 12 FX         — тёмно-серый
        0xFFD4A017, // 13 Ethnic     — золото
        0xFFE02A2A, // 14 Perc       — ярко-красный
        0xFF6B6B6B, // 15 SFX        — серый
    };
    if (idx < 0) idx = 0;
    if (idx > 127) idx = 127;
    int cat = idx / 8;
    int sub = idx % 8;
    uint32_t base = CAT_COLORS[cat];
    // Немного варьируем яркость в пределах категории
    int r = (base >>  0) & 0xFF;
    int g = (base >>  8) & 0xFF;
    int b = (base >> 16) & 0xFF;
    int a = (base >> 24) & 0xFF;
    float t = (sub - 3.5f) / 3.5f;   // -1..+1
    auto clamp255 = [](int x){ return x<0?0:(x>255?255:x); };
    r = clamp255(r + (int)(t * 30));
    g = clamp255(g + (int)(t * 30));
    b = clamp255(b + (int)(t * 30));
    return ((uint32_t)a<<24) | ((uint32_t)b<<16) | ((uint32_t)g<<8) | (uint32_t)r;
}

ChipSettings::ChipSettings(){ reset_all(); }
void ChipSettings::reset_all(){
    for (int i=0;i<128;++i) reset_one(i);
    reset_drum();
    for (int i=0;i<DRUM_COUNT;++i){
        drum_sample[i].store(-1);
        drum_rate[i].store(15);
        drum_volume[i].store(15);
        drum_loop[i].store(false);
    }
    use_dpcm_drums.store(false);
    dmc_vol.store(15);
}
void ChipSettings::reset_one(int i){
    if (i<0||i>=128) return;
    const auto& f = FAM[i/8];
    auto& I = inst[i];
    I.color.store(0);   // auto
    I.channel.store(f.ch);
    I.duty.store(f.duty);
    I.attack.store(f.a);
    I.decay.store(f.d);
    I.sustain.store(f.s);
    I.release.store(f.r);
    I.sweep_mode.store(0);
    I.sweep_rate.store(0.0f);
    I.noise_mode.store(f.ch==CH_NOI?1:0);
    I.noise_rate_mult.store(8.0f);
    I.noise_fixed_hz.store(0.0f);
    // Начальный rate index для noise-инструментов — 6 (14 кГц)
    I.noise_rate_index.store(6);
    I.rate_env.enabled.store(false);
    I.rate_env.length.store(16);
    I.rate_env.speed.store(1);
    I.rate_env.loop_start.store(-1);
    I.rate_env.loop_end.store(-1);
    I.rate_env.fill(0);
    I.vol_env.enabled.store(false);   I.vol_env.length.store(16); I.vol_env.speed.store(1);
    I.vol_env.loop_start.store(-1);   I.vol_env.loop_end.store(-1);
    I.vol_env.fill(15);
    I.pitch_env.enabled.store(false); I.pitch_env.length.store(16); I.pitch_env.speed.store(1);
    I.pitch_env.loop_start.store(-1); I.pitch_env.loop_end.store(-1);
    I.pitch_env.fill(0);
    I.duty_env.enabled.store(false);  I.duty_env.length.store(16); I.duty_env.speed.store(1);
    I.duty_env.loop_start.store(-1);  I.duty_env.loop_end.store(-1);
    I.duty_env.fill(0);
    I.arp_env.enabled.store(false);   I.arp_env.length.store(16); I.arp_env.speed.store(1);
    I.arp_env.loop_start.store(-1);   I.arp_env.loop_end.store(-1);
    I.arp_env.fill(0);
}
void ChipSettings::reset_drum(){
    drum_noise_mode.store(1);
    drum_noise_rate_mult.store(8.0f);
    drum_noise_fixed_hz.store(0.0f);
    drum_attack.store(0.001f);
    drum_decay.store(0.150f);
    drum_sustain.store(0.00f);
    drum_release.store(0.100f);
}
void ChipSettings::copy_from(const ChipSettings& o){
    pulse1_vol.store(o.pulse1_vol.load());
    pulse2_vol.store(o.pulse2_vol.load());
    triangle_vol.store(o.triangle_vol.load());
    noise_vol.store(o.noise_vol.load());
    dmc_vol.store(o.dmc_vol.load());
    pulse1_duty.store(o.pulse1_duty.load());
    pulse2_duty.store(o.pulse2_duty.load());
    use_gm_mapping.store(o.use_gm_mapping.load());
    use_envelopes.store(o.use_envelopes.load());
    use_dpcm_drums.store(o.use_dpcm_drums.load());
    drum_channel=o.drum_channel;
    for (int i=0;i<128;++i) inst[i].copy_from(o.inst[i]);
    for (int i=0;i<DRUM_COUNT;++i){
        drum_sample[i].store(o.drum_sample[i].load());
        drum_rate[i].store(o.drum_rate[i].load());
        drum_volume[i].store(o.drum_volume[i].load());
        drum_loop[i].store(o.drum_loop[i].load());
    }
    drum_noise_mode.store(o.drum_noise_mode.load());
    drum_noise_rate_mult.store(o.drum_noise_rate_mult.load());
    drum_noise_fixed_hz.store(o.drum_noise_fixed_hz.load());
    drum_attack.store(o.drum_attack.load());
    drum_decay.store(o.drum_decay.load());
    drum_sustain.store(o.drum_sustain.load());
    drum_release.store(o.drum_release.load());
}

// ===== NesApu =====
void NesApu::set_pulse1(double f,int d,int v){ p1_.freq=f;p1_.duty=d;p1_.vol=v; }
void NesApu::set_pulse2(double f,int d,int v){ p2_.freq=f;p2_.duty=d;p2_.vol=v; }
void NesApu::set_triangle(double f,int v){ tri_.freq=f;tri_.vol=v; }
void NesApu::set_noise(double r,int v,int m){ noi_.rate=r;noi_.vol=v;noi_.mode=m; }
void NesApu::trigger_dmc(const std::vector<uint8_t>* d,int ri,int v,bool lp){
    if (!d||d->empty()) return;
    dmc_.data=d; dmc_.byte_pos=0; dmc_.bit_pos=0; dmc_.level=64;
    dmc_.phase=0.0;
    dmc_.rate_hz=DMC_RATE_HZ[std::max(0,std::min(15,ri))];
    dmc_.volume=std::max(0,std::min(15,v));
    dmc_.loop=lp; dmc_.active=true;
}
void NesApu::stop_dmc(){ dmc_.active=false; }

float NesApu::tick_one(){
    const double inv=1.0/sample_rate_;
    float mix=0.0f;
    static const double duty_frac[4]={0.125,0.25,0.5,0.75};

    int v1=p1_.vol,v2=p2_.vol,vt=tri_.vol,vn=noi_.vol;
    if (settings_){
        v1=std::min(v1,settings_->pulse1_vol.load());
        v2=std::min(v2,settings_->pulse2_vol.load());
        vt=std::min(vt,settings_->triangle_vol.load());
        vn=std::min(vn,settings_->noise_vol.load());
    }
    if (v1>0&&p1_.freq>0){
        p1_.phase+=p1_.freq*inv;
        if (p1_.phase>=1.0) p1_.phase-=std::floor(p1_.phase);
        float a=v1/15.0f;
        mix += (p1_.phase<duty_frac[p1_.duty&3])?a:-a;
    }
    if (v2>0&&p2_.freq>0){
        p2_.phase+=p2_.freq*inv;
        if (p2_.phase>=1.0) p2_.phase-=std::floor(p2_.phase);
        float a=v2/15.0f;
        mix += (p2_.phase<duty_frac[p2_.duty&3])?a:-a;
    }
    if (vt>0&&tri_.freq>0){
        tri_.phase+=tri_.freq*inv;
        if (tri_.phase>=1.0) tri_.phase-=std::floor(tri_.phase);
        double t=tri_.phase;
        double w=(t<0.5)?(4.0*t-1.0):(3.0-4.0*t);
        mix += (float)(w*(vt/15.0f));
    }
    if (vn>0&&noi_.rate>0){
        noi_.phase+=noi_.rate*inv;
        while (noi_.phase>=1.0){
            noi_.phase-=1.0;
            uint16_t bit;
            if (noi_.mode) bit=(noi_.lfsr^(noi_.lfsr>>6))&1;
            else bit=(noi_.lfsr^(noi_.lfsr>>1))&1;
            noi_.lfsr=(uint16_t)((noi_.lfsr>>1)|(bit<<14));
        }
        float a=vn/15.0f;
        mix += (noi_.lfsr&1)?a:-a;
    }

    float dmc_mix=0.0f;
    if (dmc_.active && dmc_.data){
        const auto& d=*dmc_.data;
        dmc_.phase += dmc_.rate_hz*inv;
        int guard=64;
        while (dmc_.phase>=1.0 && guard-->0){
            dmc_.phase-=1.0;
            if (dmc_.byte_pos>=d.size()){
                if (dmc_.loop){ dmc_.byte_pos=0; dmc_.bit_pos=0; }
                else { dmc_.active=false; break; }
            }
            uint8_t b=d[dmc_.byte_pos];
            int bit=(b>>(7-dmc_.bit_pos))&1;
            if (bit) dmc_.level=std::min(127,dmc_.level+2);
            else dmc_.level=std::max(0,dmc_.level-2);
            dmc_.bit_pos++;
            if (dmc_.bit_pos>=8){ dmc_.bit_pos=0; dmc_.byte_pos++; }
        }
        int dvol=dmc_.volume;
        if (settings_) dvol=std::min(dvol,settings_->dmc_vol.load());
        if (dvol>0){
            float s=(dmc_.level-64)/64.0f;
            dmc_mix = s*(dvol/15.0f);
        }
    }
    return (mix*0.25f) + (dmc_mix*1.0f);
}

void NesApu::generate(int16_t* out,int samples){
    for (int i=0;i<samples;++i){
        float s=tick_one();
        if (s>1.0f) s=1.0f;
        if (s<-1.0f) s=-1.0f;
        int16_t v=(int16_t)(s*32000.0f);
        out[2*i]=v; out[2*i+1]=v;
    }
}

// ===== ApuPlayer =====
ApuPlayer::ApuPlayer(NesApu& apu, const MidiFile& mf)
    : apu_(apu), notes_(mf.notes), total_(mf.total_sec+0.5) { map_notes(); }

void ApuPlayer::reset_indices(){ i1_=i2_=it_=in_=id_=0; apu_.stop_dmc(); }

void ApuPlayer::map_notes(){
    p1_.clear(); p2_.clear(); tri_.clear(); noi_.clear(); dmc_.clear();
    reset_indices();

    std::vector<MidiNote> v=notes_;
    std::sort(v.begin(),v.end(),
        [](const MidiNote&a,const MidiNote&b){return a.start_sec<b.start_sec;});

    const bool use_gm=settings_.use_gm_mapping.load();
    const bool use_env=settings_.use_envelopes.load();
    const bool use_dpcm=settings_.use_dpcm_drums.load();
    const int drum_ch=settings_.drum_channel;

    auto tail_end=[](const std::vector<ApuNote>& vv)->double{
        if (vv.empty()) return -1e9;
        return vv.back().end+vv.back().env.release;
    };
    auto put=[](std::vector<ApuNote>& dst,ApuNote an){
        if (!dst.empty()){
            double tail=dst.back().end+dst.back().env.release;
            if (tail>an.start){
                dst.back().end=an.start;
                dst.back().env.release=0.0f;
                dst.back().dur_sec=dst.back().end-dst.back().start;
            }
        }
        dst.push_back(an);
    };

    for (const auto& n : v){
        // ===== DRUMS =====
        if (n.channel == drum_ch) {
            int slot = midi_pitch_to_drum_slot(n.pitch);

            if (use_dpcm && slot >= 0) {
                int sidx = settings_.drum_sample[slot].load();
                if (sidx >= 0) {
                    DmcNote dn;
                    dn.start = n.start_sec;
                    dn.sample_idx = sidx;
                    dn.rate_index = settings_.drum_rate[slot].load();
                    dn.volume = settings_.drum_volume[slot].load();
                    dn.loop = settings_.drum_loop[slot].load();
                    dmc_.push_back(dn);
                    continue;
                }
            }

            ApuNote an;
            an.start   = n.start_sec;
            an.end     = n.start_sec + n.dur_sec;
            an.dur_sec = n.dur_sec;
            an.pitch   = n.pitch;
            int vol = n.velocity / 8;
            an.volume  = std::max(1, std::min(15, vol));
            an.duty    = 2;
            an.sweep_mode = 0;
            an.sweep_rate = 0.0f;
            an.instrument_idx = -1;
            an.drum_slot = slot;   // ВАЖНО: явно устанавливаем

            if (slot >= 0) {
                // Per-slot настройки
                an.freq       = midi_to_freq(n.pitch);
                an.noise_mode = settings_.drum_noise_mode_slot[slot].load();
                int ri        = settings_.drum_noise_rate_index_slot[slot].load();
                if (ri < 0) ri = 0;
                if (ri > 15) ri = 15;
                an.noise_rate_index = ri;
                an.noise_rate       = 0.0f;
                an.env.attack  = settings_.drum_noise_attack_slot[slot].load();
                an.env.decay   = settings_.drum_noise_decay_slot[slot].load();
                an.env.sustain = settings_.drum_noise_sustain_slot[slot].load();
                an.env.release = settings_.drum_noise_release_slot[slot].load();
            } else {
                // Legacy (питч не маппится)
                an.freq       = midi_to_freq(n.pitch);
                an.noise_mode = settings_.drum_noise_mode.load();
                float mult    = settings_.drum_noise_rate_mult.load();
                float fixed   = settings_.drum_noise_fixed_hz.load();
                an.noise_rate = (fixed > 0.0f) ? fixed : an.freq * mult;
                an.noise_rate_index = -1;
                an.env.attack  = settings_.drum_attack.load();
                an.env.decay   = settings_.drum_decay.load();
                an.env.sustain = settings_.drum_sustain.load();
                an.env.release = settings_.drum_release.load();
            }

            // Fallback: если D=0 и S=0 -> тишина, ставим короткий удар
            if (an.env.decay < 0.01f && an.env.sustain < 0.01f) {
                an.env.decay = 0.08f;
            }
            if (an.env.release < 0.01f) an.env.release = 0.05f;

            put(noi_, an);
            continue;
        }


        // ===== MELODIC =====
        int target_ch=CH_PULSE, duty=2, sweep_mode=0;
        float sweep_rate=0.0f, noise_mult=8.0f, noise_fixed=0.0f;
        int noise_mode=0;
        int noise_rate_idx_val = -1;   // -1 = legacy (mult*freq)
        int instrument_idx=-1;
        EnvParams env;

        if (use_gm && n.program>=0){
            int idx=std::max(0,std::min(127,n.program));
            auto& I=settings_.inst[idx];
            instrument_idx=idx;
            target_ch=std::max(0,std::min(2,I.channel.load()));
            duty=I.duty.load();
            sweep_mode=I.sweep_mode.load();
            sweep_rate=I.sweep_rate.load();
            noise_mode=I.noise_mode.load();
            noise_mult=I.noise_rate_mult.load();
            noise_fixed=I.noise_fixed_hz.load();
            noise_rate_idx_val = I.noise_rate_index.load();
            if (use_env){
                env.attack=I.attack.load();
                env.decay=I.decay.load();
                env.sustain=I.sustain.load();
                env.release=I.release.load();
            } else env={0.001f,0.001f,1.0f,0.010f};
        } else {
            if (n.pitch<=47) target_ch=CH_TRI;
            env={0.005f,0.05f,0.8f,0.10f};
        }

        int vol=n.velocity/8;
        vol=std::max(1,std::min(15,vol));

        ApuNote an;
        an.start=n.start_sec;
        an.end=n.start_sec+n.dur_sec;
        an.dur_sec=n.dur_sec;
        an.freq=midi_to_freq(n.pitch);
        an.pitch=n.pitch;
        an.volume=vol;
        an.duty=duty;
        an.sweep_mode=sweep_mode;
        an.sweep_rate=sweep_rate;
        an.noise_mode=noise_mode;
        an.noise_rate=(noise_fixed>0.0f)?noise_fixed:an.freq*noise_mult;
        an.noise_rate_index = noise_rate_idx_val;
        an.instrument_idx=instrument_idx;
        an.env=env;

        switch (target_ch){
            case CH_TRI: put(tri_,an); break;
            case CH_NOI: put(noi_,an); break;
            case CH_PULSE:
            default: {
                double e1=tail_end(p1_),e2=tail_end(p2_);
                if (e1<=e2) put(p1_,an); else put(p2_,an);
            }
        }
    }
}

void ApuPlayer::remap(){ map_notes(); }

// Применяем envelope к параметрам ноты в момент t
static inline void apply_envelopes(int inst_idx, int base_pitch,
                                   const ChipSettings& s,
                                   double tl, double& freq, int& vol, int& duty)
{
    if (inst_idx < 0) return;
    const auto& I = s.inst[inst_idx];

    if (I.vol_env.enabled.load()){
        int st=env_step(I.vol_env,tl);
        int v=I.vol_env.values[st].load();
        if (v<0) v=0; if (v>15) v=15;
        vol = vol * v / 15;
    }
    bool is_noise = (I.channel.load() == CH_NOI);
    if (!is_noise) {
        if (I.pitch_env.enabled.load()){
            int st=env_step(I.pitch_env,tl);
            int p=I.pitch_env.values[st].load();
            if (p<-36) p=-36; if (p>36) p=36;
            freq = midi_to_freq(base_pitch + p);
        }
        if (I.arp_env.enabled.load()){
            int st=env_step(I.arp_env,tl);
            int a=I.arp_env.values[st].load();
            if (a<-24) a=-24; if (a>24) a=24;
            freq = midi_to_freq(base_pitch + a);
        }
        if (I.duty_env.enabled.load()){
            int st=env_step(I.duty_env,tl);
            duty = I.duty_env.values[st].load() & 3;
        }
    }
}

void ApuPlayer::advance_to(double t){
    // Pulse 1
    while (i1_<p1_.size()  && p1_[i1_].end  + p1_[i1_].env.release  <= t) ++i1_;
    if (i1_<p1_.size() && p1_[i1_].start<=t){
        const auto& n=p1_[i1_];
        double tl=t-n.start;
        double a=env_value(tl,n.dur_sec,n.env);
        int duty=effective_duty(n.duty,n.sweep_mode,n.sweep_rate,tl);
        double freq=n.freq;
        int vol=(int)std::lround(n.volume*a);
        if (settings_.use_envelopes.load()) apply_envelopes(n.instrument_idx,n.pitch,settings_,tl,freq,vol,duty);
        apu_.set_pulse1(freq, duty, std::max(0,std::min(15,vol)));
    } else apu_.set_pulse1(0,2,0);

    // Pulse 2
    while (i2_<p2_.size()  && p2_[i2_].end  + p2_[i2_].env.release  <= t) ++i2_;
    if (i2_<p2_.size() && p2_[i2_].start<=t){
        const auto& n=p2_[i2_];
        double tl=t-n.start;
        double a=env_value(tl,n.dur_sec,n.env);
        int duty=effective_duty(n.duty,n.sweep_mode,n.sweep_rate,tl);
        double freq=n.freq;
        int vol=(int)std::lround(n.volume*a);
        if (settings_.use_envelopes.load()) apply_envelopes(n.instrument_idx,n.pitch,settings_,tl,freq,vol,duty);
        apu_.set_pulse2(freq, duty, std::max(0,std::min(15,vol)));
    } else apu_.set_pulse2(0,2,0);

    // Triangle
    while (it_<tri_.size() && tri_[it_].end + tri_[it_].env.release <= t) ++it_;
    if (it_<tri_.size() && tri_[it_].start<=t){
        const auto& n=tri_[it_];
        double tl=t-n.start;
        double a=env_value(tl,n.dur_sec,n.env);
        double freq=n.freq;
        int vol=(int)std::lround(n.volume*a);
        int dummy_duty=2;
        if (settings_.use_envelopes.load()) apply_envelopes(n.instrument_idx,n.pitch,settings_,tl,freq,vol,dummy_duty);
        apu_.set_triangle(freq, std::max(0,std::min(15,vol)));
    } else apu_.set_triangle(0,0);

    // Noise
    while (in_<noi_.size() && noi_[in_].end + noi_[in_].env.release <= t) ++in_;
    if (in_<noi_.size() && noi_[in_].start<=t){
        const auto& n=noi_[in_];
        // DEBUG: только для drum, только первый сэмпл
        if (n.drum_slot >= 0 && t - n.start < 0.001) {
        }
        double tl=t-n.start;
        double a=env_value(tl,n.dur_sec,n.env);
        int vol=(int)std::lround(n.volume*a);
        int dummy_duty=2;
        double dummy_freq = n.freq;
        if (settings_.use_envelopes.load())
            apply_envelopes(n.instrument_idx,n.pitch,settings_,tl,dummy_freq,vol,dummy_duty);

        double rate_hz;
        if (n.noise_rate_index >= 0) {
            // Новый режим: NES rate table + rate_env
            int idx = n.noise_rate_index;
            if (n.instrument_idx >= 0) {
                const auto& I = settings_.inst[n.instrument_idx];
                if (I.rate_env.enabled.load()) {
                    int step = env_step(I.rate_env, tl);
                    idx += I.rate_env.values[step].load();
                    if (idx < 0) idx = 0;
                    if (idx > 15) idx = 15;
                }
            }
            rate_hz = NES_NOISE_RATE[idx];
        } else {
            // Legacy (drums): n.noise_rate в Гц
            rate_hz = n.noise_rate;
        }
        apu_.set_noise(rate_hz, std::max(0,std::min(15,vol)), n.noise_mode);
    } else apu_.set_noise(0,0,0);

    // DMC
    while (id_<dmc_.size() && dmc_[id_].start<=t){
        const auto& dn=dmc_[id_];
        if (dmc_samples_ && dn.sample_idx>=0 && dn.sample_idx<(int)dmc_samples_->size()){
            const auto& smp=(*dmc_samples_)[dn.sample_idx];
            apu_.trigger_dmc(&smp.data,dn.rate_index,dn.volume,dn.loop);
        }
        ++id_;
    }
}

ApuPlayer::NowPlaying ApuPlayer::now_playing() const {
    NowPlaying np;
    double t=cur_time_.load();
    auto peek=[t](const std::vector<ApuNote>& v,size_t i){
        if (i<v.size() && v[i].start<=t && t<v[i].end+v[i].env.release) return v[i].pitch;
        return -1;
    };
    np.p1_pitch=peek(p1_,i1_);
    np.p2_pitch=peek(p2_,i2_);
    np.tri_pitch=peek(tri_,it_);
    np.noise_on=(in_<noi_.size() && noi_[in_].start<=t
                 && t<noi_[in_].end+noi_[in_].env.release);
    np.dmc_slot=-1;
    for (size_t i=id_; i<dmc_.size(); ++i){
        if (dmc_[i].start>t) break;
        np.dmc_slot=dmc_[i].sample_idx;
    }
    return np;
}

void ApuPlayer::generate(int16_t* out,int samples){
    if (!playing_.load()){ for (int i=0;i<2*samples;++i) out[i]=0; return; }
    double sk=seek_req_.exchange(-1.0);
    if (sk>=0){ cur_time_.store(sk); reset_indices(); }
    const double dt=1.0/apu_.sample_rate();
    const float g=master_vol_.load();
    int16_t tmp[2];
    for (int i=0;i<samples;++i){
        double t=cur_time_.load();
        if (t>=total_){ playing_.store(false); out[2*i]=out[2*i+1]=0; continue; }
        advance_to(t);
        apu_.generate(tmp,1);
        out[2*i]=(int16_t)(tmp[0]*g);
        out[2*i+1]=(int16_t)(tmp[1]*g);
        cur_time_.store(t+dt);
    }
}
void ApuPlayer::play(){ playing_.store(true); }
void ApuPlayer::pause(){ playing_.store(false); }
void ApuPlayer::stop(){ playing_.store(false); seek_req_.store(0.0); }
void ApuPlayer::seek(double s){
    if (s<0) s=0;
    if (s>total_) s=total_;
    seek_req_.store(s);
}