#include "preset.h"
#include "util.h"
#include <fstream>
#include <string>

static int remap_old_channel(int old_ch){
    switch (old_ch){ case 0: case 1: return 0; case 2: return 1; case 3: return 2; }
    return 0;
}

static void save_env(std::ofstream& f, const char* key, int idx,
                     const EnvelopeCurve& e)
{
    f << key << " " << idx
      << " " << (e.enabled.load()?1:0)
      << " " << e.length.load()
      << " " << e.speed.load()
      << " " << e.loop_start.load()
      << " " << e.loop_end.load();
    for (int i = 0; i < EnvelopeCurve::MAX_STEPS; ++i)
        f << " " << e.values[i].load();
    f << "\n";
}

static void load_env(std::ifstream& f, EnvelopeCurve& e, int ver){
    int en=0, len=16, sp=1, ls=-1, le=-1;
    f >> en >> len >> sp >> ls >> le;
    e.enabled.store(en!=0);
    e.length.store(len);
    e.speed.store(sp);
    e.loop_start.store(ls);
    e.loop_end.store(le);
    for (int i = 0; i < EnvelopeCurve::MAX_STEPS; ++i){
        int v = 0; f >> v;
        e.values[i].store(v);
    }
    (void)ver;
}

bool save_preset(const std::wstring& wpath, const ChipSettings& s){
    std::ofstream f(wide_to_utf8(wpath));
    if (!f) return false;
    f << "NESPRESET 5\n";
    f << "p1v " << s.pulse1_vol.load() << "\n";
    f << "p2v " << s.pulse2_vol.load() << "\n";
    f << "tv "  << s.triangle_vol.load() << "\n";
    f << "nv "  << s.noise_vol.load() << "\n";
    f << "dv "  << s.dmc_vol.load() << "\n";
    f << "p1d " << s.pulse1_duty.load() << "\n";
    f << "p2d " << s.pulse2_duty.load() << "\n";
    f << "ugm " << (s.use_gm_mapping.load()?1:0) << "\n";
    f << "uenv " << (s.use_envelopes.load()?1:0) << "\n";
    f << "udrm " << (s.use_dpcm_drums.load()?1:0) << "\n";
    f << "dch " << s.drum_channel << "\n";
    f << "dnm " << s.drum_noise_mode.load() << "\n";
    f << "dnrm " << s.drum_noise_rate_mult.load() << "\n";
    f << "dnfh " << s.drum_noise_fixed_hz.load() << "\n";
    f << "datt " << s.drum_attack.load() << "\n";
    f << "ddec " << s.drum_decay.load() << "\n";
    f << "dsus " << s.drum_sustain.load() << "\n";
    f << "drel " << s.drum_release.load() << "\n";
    for (int i=0;i<DRUM_COUNT;++i){
        f << "ds " << i
          << " " << s.drum_sample[i].load()
          << " " << s.drum_rate[i].load()
          << " " << s.drum_volume[i].load()
          << " " << (s.drum_loop[i].load()?1:0) << "\n";
    }
    for (int i=0;i<128;++i){
        const auto& I = s.inst[i];
        f << "i " << i
          << " " << I.channel.load()
          << " " << I.duty.load()
          << " " << I.attack.load()
          << " " << I.decay.load()
          << " " << I.sustain.load()
          << " " << I.release.load()
          << " " << I.sweep_mode.load()
          << " " << I.sweep_rate.load()
          << " " << I.noise_mode.load()
          << " " << I.noise_rate_mult.load()
          << " " << I.noise_fixed_hz.load() << "\n";
        save_env(f, "ev", i, I.vol_env);
        save_env(f, "ep", i, I.pitch_env);
        save_env(f, "ed", i, I.duty_env);
        save_env(f, "ea", i, I.arp_env);
    }
    return f.good();
}

bool load_preset(const std::wstring& wpath, ChipSettings& s){
    std::ifstream f(wide_to_utf8(wpath));
    if (!f) return false;
    std::string tag; int ver=0;
    f >> tag >> ver;
    if (tag!="NESPRESET" || ver<1 || ver>5) return false;
    std::string key;
    while (f >> key){
        if      (key=="p1v"){ int x; f>>x; s.pulse1_vol.store(x); }
        else if (key=="p2v"){ int x; f>>x; s.pulse2_vol.store(x); }
        else if (key=="tv") { int x; f>>x; s.triangle_vol.store(x); }
        else if (key=="nv") { int x; f>>x; s.noise_vol.store(x); }
        else if (key=="dv") { int x; f>>x; s.dmc_vol.store(x); }
        else if (key=="p1d"){ int x; f>>x; s.pulse1_duty.store(x); }
        else if (key=="p2d"){ int x; f>>x; s.pulse2_duty.store(x); }
        else if (key=="ugm"){ int x; f>>x; s.use_gm_mapping.store(x!=0); }
        else if (key=="uenv"){int x; f>>x; s.use_envelopes.store(x!=0); }
        else if (key=="udrm"){int x; f>>x; s.use_dpcm_drums.store(x!=0); }
        else if (key=="dch"){ int x; f>>x; s.drum_channel=x; }
        else if (key=="dnm"){ int x;   f>>x; s.drum_noise_mode.store(x); }
        else if (key=="dnrm"){float x; f>>x; s.drum_noise_rate_mult.store(x); }
        else if (key=="dnfh"){float x; f>>x; s.drum_noise_fixed_hz.store(x); }
        else if (key=="datt"){float x; f>>x; s.drum_attack.store(x); }
        else if (key=="ddec"){float x; f>>x; s.drum_decay.store(x); }
        else if (key=="dsus"){float x; f>>x; s.drum_sustain.store(x); }
        else if (key=="drel"){float x; f>>x; s.drum_release.store(x); }
        else if (key=="ds"){
            int idx,smp,rate,vol,lp;
            f>>idx>>smp>>rate>>vol>>lp;
            if (idx>=0&&idx<DRUM_COUNT){
                s.drum_sample[idx].store(smp);
                s.drum_rate[idx].store(rate);
                s.drum_volume[idx].store(vol);
                s.drum_loop[idx].store(lp!=0);
            }
        }
        else if (key=="i"){
            int idx,ch,duty,sw=0,nm=0;
            float a,d,su,r,swr=0.0f,nrm=8.0f,nfh=0.0f;
            f>>idx>>ch>>duty>>a>>d>>su>>r;
            if (ver>=2) f>>sw>>swr;
            if (ver>=3) f>>nm>>nrm>>nfh;
            if (ver<3) ch = remap_old_channel(ch);
            if (idx>=0&&idx<128){
                auto& I = s.inst[idx];
                I.channel.store(ch);
                I.duty.store(duty);
                I.attack.store(a);
                I.decay.store(d);
                I.sustain.store(su);
                I.release.store(r);
                I.sweep_mode.store(sw);
                I.sweep_rate.store(swr);
                I.noise_mode.store(nm);
                I.noise_rate_mult.store(nrm);
                I.noise_fixed_hz.store(nfh);
            }
        }
        else if (key=="ev" || key=="ep" || key=="ed" || key=="ea"){
            int idx; f >> idx;
            if (idx>=0 && idx<128){
                auto& I = s.inst[idx];
                if      (key=="ev") load_env(f, I.vol_env,   ver);
                else if (key=="ep") load_env(f, I.pitch_env, ver);
                else if (key=="ed") load_env(f, I.duty_env,  ver);
                else if (key=="ea") load_env(f, I.arp_env,   ver);
            } else {
                // Пропускаем строку
                std::string tmp; std::getline(f, tmp);
            }
        }
    }
    return true;
}
