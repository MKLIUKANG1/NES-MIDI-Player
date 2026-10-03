#pragma once
#include <cstdint>
#include <vector>
#include <atomic>
#include <mutex>
#include "midi_reader.h"
#include "dmc.h"

enum ApuChannel { CH_PULSE = 0, CH_TRI = 1, CH_NOI = 2 };

enum DrumSlotId {
    DRUM_KICK=0, DRUM_SNARE, DRUM_SNARE2, DRUM_CLAP,
    DRUM_HIHAT, DRUM_HIHAT2, DRUM_TOM, DRUM_CYMBAL,
    DRUM_COUNT
};
inline const char* const DRUM_SLOT_NAMES[DRUM_COUNT] = {
    "Kick","Snare","Snare 2","Clap","HiHat","HiHat Open","Tom","Cymbal"
};
inline int midi_pitch_to_drum_slot(int pitch) {
    switch (pitch) {
        case 35: case 36: return DRUM_KICK;
        case 38:          return DRUM_SNARE;
        case 37: case 40: return DRUM_SNARE2;
        case 39:          return DRUM_CLAP;
        case 42: case 44: return DRUM_HIHAT;
        case 46:          return DRUM_HIHAT2;
        case 41: case 43: case 45: case 47: case 48: case 50: return DRUM_TOM;
        case 49: case 51: case 52: case 53: case 55: case 57: case 59: return DRUM_CYMBAL;
    }
    return -1;
}

struct EnvParams {
    float attack=0.005f, decay=0.050f, sustain=0.80f, release=0.100f;
};

// ===== Envelope Curve =====
struct EnvelopeCurve {
    static constexpr int MAX_STEPS = 64;
    std::atomic<bool> enabled{false};
    std::atomic<int>  length{16};
    std::atomic<int>  speed{1};      // ticks @60Hz per step
    std::atomic<int>  loop_start{-1};
    std::atomic<int>  loop_end{-1};
    std::atomic<int>  values[MAX_STEPS];

    EnvelopeCurve() { for (int i=0;i<MAX_STEPS;++i) values[i].store(0); }
    EnvelopeCurve(const EnvelopeCurve& o){ copy_from(o); }
    EnvelopeCurve& operator=(const EnvelopeCurve& o){ copy_from(o); return *this; }
    void copy_from(const EnvelopeCurve& o){
        enabled.store(o.enabled.load());
        length.store(o.length.load());
        speed.store(o.speed.load());
        loop_start.store(o.loop_start.load());
        loop_end.store(o.loop_end.load());
        for (int i=0;i<MAX_STEPS;++i) values[i].store(o.values[i].load());
    }
    void fill(int v){ for (int i=0;i<MAX_STEPS;++i) values[i].store(v); }
};

struct Instrument {
    std::atomic<int>   channel{CH_PULSE};
    std::atomic<int>   duty   {2};
    std::atomic<float> attack {0.005f};
    std::atomic<float> decay  {0.050f};
    std::atomic<float> sustain{0.80f};
    std::atomic<float> release{0.100f};
    std::atomic<int>   sweep_mode{0};
    std::atomic<float> sweep_rate{0.0f};
    std::atomic<int>   noise_mode    {0};
    std::atomic<float> noise_rate_mult{8.0f};
    std::atomic<float> noise_fixed_hz{0.0f};

    EnvelopeCurve vol_env, pitch_env, duty_env, arp_env;

    Instrument() = default;
    Instrument(const Instrument& o){ copy_from(o); }
    Instrument& operator=(const Instrument& o){ copy_from(o); return *this; }

    void copy_from(const Instrument& o){
        channel.store(o.channel.load());
        duty.store(o.duty.load());
        attack.store(o.attack.load());
        decay.store(o.decay.load());
        sustain.store(o.sustain.load());
        release.store(o.release.load());
        sweep_mode.store(o.sweep_mode.load());
        sweep_rate.store(o.sweep_rate.load());
        noise_mode.store(o.noise_mode.load());
        noise_rate_mult.store(o.noise_rate_mult.load());
        noise_fixed_hz.store(o.noise_fixed_hz.load());
        vol_env.copy_from(o.vol_env);
        pitch_env.copy_from(o.pitch_env);
        duty_env.copy_from(o.duty_env);
        arp_env.copy_from(o.arp_env);
    }
};

struct ChipSettings {
    std::atomic<int> pulse1_vol{15}, pulse2_vol{15};
    std::atomic<int> triangle_vol{15}, noise_vol{15}, dmc_vol{15};
    std::atomic<int> pulse1_duty{2}, pulse2_duty{2};
    std::atomic<bool> use_gm_mapping{true};
    std::atomic<bool> use_envelopes{true};
    Instrument inst[128];
    int drum_channel = 9;
    std::atomic<bool> use_dpcm_drums{false};
    std::atomic<int>  drum_sample[DRUM_COUNT];
    std::atomic<int>  drum_rate[DRUM_COUNT];
    std::atomic<int>  drum_volume[DRUM_COUNT];
    std::atomic<bool> drum_loop[DRUM_COUNT];
    std::atomic<int>   drum_noise_mode{1};
    std::atomic<float> drum_noise_rate_mult{8.0f};
    std::atomic<float> drum_noise_fixed_hz{0.0f};
    std::atomic<float> drum_attack{0.001f};
    std::atomic<float> drum_decay{0.150f};
    std::atomic<float> drum_sustain{0.00f};
    std::atomic<float> drum_release{0.100f};

    ChipSettings();
    void reset_all();
    void reset_one(int i);
    void reset_drum();
    void copy_from(const ChipSettings& o);
    ChipSettings(const ChipSettings& o){ copy_from(o); }
    ChipSettings& operator=(const ChipSettings& o){ copy_from(o); return *this; }
};

class NesApu {
public:
    void set_sample_rate(double sr){ sample_rate_=sr; }
    double sample_rate() const { return sample_rate_; }
    void set_settings(ChipSettings* s){ settings_=s; }
    void set_pulse1(double f,int d,int v);
    void set_pulse2(double f,int d,int v);
    void set_triangle(double f,int v);
    void set_noise(double r,int v,int m);
    void trigger_dmc(const std::vector<uint8_t>* data,int rate_index,int volume,bool loop);
    void stop_dmc();
    bool dmc_active() const { return dmc_.active; }
    void generate(int16_t* out,int samples);
private:
    float tick_one();
    double sample_rate_=44100.0;
    ChipSettings* settings_=nullptr;
    struct Pulse { double phase=0,freq=0; int duty=2,vol=0; };
    struct Tri { double phase=0,freq=0; int vol=0; };
    struct Noise { double phase=0,rate=0; int vol=0,mode=0; uint16_t lfsr=0x7FFF; };
    Pulse p1_,p2_; Tri tri_; Noise noi_;
    struct Dmc {
        const std::vector<uint8_t>* data=nullptr;
        size_t byte_pos=0; int bit_pos=0,level=64;
        double phase=0.0,rate_hz=54.0;
        int volume=15; bool loop=false,active=false;
    };
    Dmc dmc_;
};

class ApuPlayer {
public:
    ApuPlayer(NesApu& apu, const MidiFile& mf);
    void generate(int16_t* out,int samples);
    void play(); void pause(); void stop(); void seek(double sec); void remap();
    bool is_playing() const { return playing_.load(); }
    double current_time() const { return cur_time_.load(); }
    double total_time() const { return total_; }
    bool finished() const { return cur_time_.load()>=total_; }
    ChipSettings& settings() { return settings_; }
    void set_master_volume(float v){ master_vol_.store(v); }
    float master_volume() const { return master_vol_.load(); }
    const std::vector<MidiNote>& raw_notes() const { return notes_; }
    void set_dmc_samples(const std::vector<DmcSample>* s){ dmc_samples_=s; }
    const std::vector<DmcSample>* dmc_samples() const { return dmc_samples_; }
    struct NowPlaying { int p1_pitch=-1,p2_pitch=-1,tri_pitch=-1; bool noise_on=false; int dmc_slot=-1; };
    NowPlaying now_playing() const;
private:
    struct ApuNote {
        double start,end,dur_sec,freq;
        int pitch,volume,duty,sweep_mode;
        float sweep_rate;
        int noise_mode;
        float noise_rate;
        int instrument_idx;     // -1 = не GM
        EnvParams env;
    };
    struct DmcNote { double start; int sample_idx,rate_index,volume; bool loop; };
    void map_notes(); void advance_to(double t); void reset_indices();
    NesApu& apu_;
    std::vector<MidiNote> notes_;
    double total_=0.0;
    std::vector<ApuNote> p1_,p2_,tri_,noi_;
    std::vector<DmcNote> dmc_;
    size_t i1_=0,i2_=0,it_=0,in_=0,id_=0;
    std::atomic<bool> playing_{false};
    std::atomic<double> cur_time_{0.0},seek_req_{-1.0};
    std::atomic<float> master_vol_{1.0f};
    ChipSettings settings_;
    const std::vector<DmcSample>* dmc_samples_=nullptr;
};

inline int derive_channel(const MidiNote& n, const ChipSettings& s){
    if (n.channel==s.drum_channel) return CH_NOI;
    if (s.use_gm_mapping.load() && n.program>=0){
        int idx = n.program<0?0:(n.program>127?127:n.program);
        return s.inst[idx].channel.load();
    }
    if (n.pitch<=47) return CH_TRI;
    return CH_PULSE;
}
