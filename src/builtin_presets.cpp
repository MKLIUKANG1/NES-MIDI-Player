#include "builtin_presets.h"

namespace {
void set(ChipSettings& s, int i, int ch, int duty,
         float A, float D, float S, float R,
         int sweep_mode = 0, float sweep_rate = 0.0f,
         int noise_mode = 0, float noise_mult = 8.0f)
{
    auto& I = s.inst[i];
    I.channel.store(ch);
    I.duty.store(duty);
    I.attack.store(A);
    I.decay.store(D);
    I.sustain.store(S);
    I.release.store(R);
    I.sweep_mode.store(sweep_mode);
    I.sweep_rate.store(sweep_rate);
    I.noise_mode.store(noise_mode);
    I.noise_rate_mult.store(noise_mult);
    I.noise_fixed_hz.store(0.0f);
}
} // namespace

void preset_default(ChipSettings& s) { s.reset_all(); }

void preset_castlevania(ChipSettings& s) {
    s.reset_all();
    for (int i = 0; i < 8; ++i)   set(s, i, CH_PULSE, 2, 0.005f, 0.600f, 0.20f, 0.500f);
    for (int i = 8; i < 16; ++i)  set(s, i, CH_PULSE, 1, 0.001f, 0.400f, 0.00f, 0.300f);
    for (int i = 16; i < 24; ++i) set(s, i, CH_PULSE, 3, 0.040f, 0.050f, 1.00f, 0.100f);
    for (int i = 24; i < 32; ++i) set(s, i, CH_PULSE, 0, 0.005f, 0.500f, 0.20f, 0.200f);
    for (int i = 32; i < 40; ++i) set(s, i, CH_TRI,   2, 0.005f, 0.400f, 0.40f, 0.150f);
    for (int i = 40; i < 48; ++i) set(s, i, CH_PULSE, 2, 0.300f, 0.200f, 0.85f, 0.400f, 1, 0.7f);
    for (int i = 48; i < 56; ++i) set(s, i, CH_PULSE, 2, 0.200f, 0.200f, 0.90f, 0.400f, 1, 0.5f);
    for (int i = 56; i < 64; ++i) set(s, i, CH_PULSE, 3, 0.080f, 0.150f, 0.70f, 0.200f);
    for (int i = 64; i < 72; ++i) set(s, i, CH_PULSE, 1, 0.040f, 0.100f, 0.80f, 0.150f);
    for (int i = 72; i < 80; ++i) set(s, i, CH_PULSE, 0, 0.080f, 0.100f, 1.00f, 0.200f);
    for (int i = 80; i < 88; ++i) set(s, i, CH_PULSE, 2, 0.010f, 0.100f, 0.85f, 0.150f, 3, 8.0f);
    for (int i = 88; i < 96; ++i) set(s, i, CH_PULSE, 1, 0.500f, 0.400f, 0.80f, 0.800f, 1, 0.4f);
    for (int i = 96; i < 104;++i) set(s, i, CH_PULSE, 3, 0.100f, 0.300f, 0.50f, 0.500f, 2, 4.0f);
    for (int i =104; i < 112;++i) set(s, i, CH_PULSE, 2, 0.010f, 0.300f, 0.50f, 0.200f);
    for (int i =112; i < 120;++i) set(s, i, CH_NOI,   2, 0.001f, 0.200f, 0.00f, 0.100f, 0, 0, 1, 8.0f);
    for (int i =120; i < 128;++i) set(s, i, CH_NOI,   2, 0.001f, 0.150f, 0.00f, 0.080f, 0, 0, 1, 8.0f);

    s.drum_noise_mode.store(1);
    s.drum_noise_rate_mult.store(8.0f);
    s.drum_noise_fixed_hz.store(0.0f);
    s.drum_attack.store(0.001f);
    s.drum_decay.store(0.200f);
    s.drum_sustain.store(0.0f);
    s.drum_release.store(0.100f);
}

void preset_megaman(ChipSettings& s) {
    s.reset_all();
    for (int i = 0; i < 8; ++i)   set(s, i, CH_PULSE, 2, 0.001f, 0.150f, 0.10f, 0.150f);
    for (int i = 8; i < 16; ++i)  set(s, i, CH_PULSE, 1, 0.001f, 0.100f, 0.00f, 0.100f);
    for (int i = 16; i < 24; ++i) set(s, i, CH_PULSE, 3, 0.005f, 0.050f, 0.90f, 0.050f);
    for (int i = 24; i < 32; ++i) set(s, i, CH_PULSE, 0, 0.001f, 0.200f, 0.10f, 0.100f);
    for (int i = 32; i < 40; ++i) set(s, i, CH_TRI,   2, 0.001f, 0.200f, 0.30f, 0.080f);
    for (int i = 40; i < 48; ++i) set(s, i, CH_PULSE, 2, 0.020f, 0.100f, 0.70f, 0.150f);
    for (int i = 48; i < 56; ++i) set(s, i, CH_PULSE, 2, 0.020f, 0.100f, 0.80f, 0.150f);
    for (int i = 56; i < 64; ++i) set(s, i, CH_PULSE, 3, 0.005f, 0.100f, 0.60f, 0.100f);
    for (int i = 64; i < 72; ++i) set(s, i, CH_PULSE, 1, 0.005f, 0.100f, 0.70f, 0.100f);
    for (int i = 72; i < 80; ++i) set(s, i, CH_PULSE, 0, 0.010f, 0.050f, 0.95f, 0.100f);
    for (int i = 80; i < 88; ++i) set(s, i, CH_PULSE, 1, 0.001f, 0.100f, 0.60f, 0.080f);
    for (int i = 88; i < 96; ++i) set(s, i, CH_PULSE, 1, 0.080f, 0.300f, 0.60f, 0.300f);
    for (int i = 96; i < 104;++i) set(s, i, CH_PULSE, 3, 0.005f, 0.200f, 0.30f, 0.300f, 2, 6.0f);
    for (int i =104; i < 112;++i) set(s, i, CH_PULSE, 2, 0.005f, 0.150f, 0.40f, 0.150f);
    for (int i =112; i < 120;++i) set(s, i, CH_NOI,   2, 0.001f, 0.080f, 0.00f, 0.050f, 0, 0, 1, 8.0f);
    for (int i =120; i < 128;++i) set(s, i, CH_NOI,   2, 0.001f, 0.100f, 0.00f, 0.060f, 0, 0, 1, 8.0f);

    s.drum_noise_mode.store(1);
    s.drum_noise_rate_mult.store(8.0f);
    s.drum_attack.store(0.001f);
    s.drum_decay.store(0.080f);
    s.drum_sustain.store(0.0f);
    s.drum_release.store(0.050f);
}

void preset_contra(ChipSettings& s) {
    s.reset_all();
    for (int i = 0; i < 8; ++i)   set(s, i, CH_PULSE, 2, 0.001f, 0.200f, 0.20f, 0.150f);
    for (int i = 8; i < 16; ++i)  set(s, i, CH_PULSE, 0, 0.001f, 0.150f, 0.00f, 0.120f);
    for (int i = 16; i < 24; ++i) set(s, i, CH_PULSE, 3, 0.010f, 0.050f, 0.95f, 0.060f);
    for (int i = 24; i < 32; ++i) set(s, i, CH_PULSE, 0, 0.001f, 0.400f, 0.10f, 0.150f);
    for (int i = 32; i < 40; ++i) set(s, i, CH_TRI,   2, 0.002f, 0.300f, 0.35f, 0.100f);
    for (int i = 40; i < 48; ++i) set(s, i, CH_PULSE, 2, 0.050f, 0.150f, 0.75f, 0.200f, 1, 1.2f);
    for (int i = 48; i < 56; ++i) set(s, i, CH_PULSE, 2, 0.080f, 0.150f, 0.85f, 0.200f, 1, 1.0f);
    for (int i = 56; i < 64; ++i) set(s, i, CH_PULSE, 3, 0.020f, 0.100f, 0.70f, 0.120f);
    for (int i = 64; i < 72; ++i) set(s, i, CH_PULSE, 1, 0.010f, 0.100f, 0.75f, 0.100f);
    for (int i = 72; i < 80; ++i) set(s, i, CH_PULSE, 0, 0.020f, 0.080f, 0.95f, 0.150f);
    for (int i = 80; i < 88; ++i) set(s, i, CH_PULSE, 1, 0.001f, 0.150f, 0.70f, 0.100f);
    for (int i = 88; i < 96; ++i) set(s, i, CH_PULSE, 2, 0.150f, 0.300f, 0.80f, 0.400f, 1, 0.6f);
    for (int i = 96; i < 104;++i) set(s, i, CH_PULSE, 3, 0.020f, 0.250f, 0.40f, 0.300f, 2, 5.0f);
    for (int i =104; i < 112;++i) set(s, i, CH_PULSE, 2, 0.005f, 0.200f, 0.45f, 0.150f);
    for (int i =112; i < 120;++i) set(s, i, CH_NOI,   2, 0.001f, 0.120f, 0.00f, 0.080f, 0, 0, 1, 10.0f);
    for (int i =120; i < 128;++i) set(s, i, CH_NOI,   2, 0.001f, 0.100f, 0.00f, 0.060f, 0, 0, 1, 10.0f);

    s.drum_noise_mode.store(1);
    s.drum_noise_rate_mult.store(10.0f);
    s.drum_attack.store(0.001f);
    s.drum_decay.store(0.120f);
    s.drum_sustain.store(0.0f);
    s.drum_release.store(0.080f);
}

void preset_chrono(ChipSettings& s) {
    s.reset_all();
    for (int i = 0; i < 8; ++i)   set(s, i, CH_PULSE, 1, 0.005f, 0.700f, 0.30f, 0.500f, 1, 3.0f);
    for (int i = 8; i < 16; ++i)  set(s, i, CH_PULSE, 0, 0.001f, 0.500f, 0.00f, 0.400f);
    for (int i = 16; i < 24; ++i) set(s, i, CH_PULSE, 3, 0.100f, 0.200f, 0.95f, 0.300f);
    for (int i = 24; i < 32; ++i) set(s, i, CH_PULSE, 0, 0.005f, 0.400f, 0.30f, 0.200f);
    for (int i = 32; i < 40; ++i) set(s, i, CH_TRI,   2, 0.010f, 0.300f, 0.60f, 0.200f);
    for (int i = 40; i < 48; ++i) set(s, i, CH_PULSE, 2, 0.250f, 0.200f, 0.85f, 0.500f, 1, 0.5f);
    for (int i = 48; i < 56; ++i) set(s, i, CH_PULSE, 2, 0.150f, 0.200f, 0.90f, 0.500f, 1, 0.6f);
    for (int i = 56; i < 64; ++i) set(s, i, CH_PULSE, 3, 0.060f, 0.150f, 0.70f, 0.200f);
    for (int i = 64; i < 72; ++i) set(s, i, CH_PULSE, 1, 0.030f, 0.100f, 0.85f, 0.150f);
    for (int i = 72; i < 80; ++i) set(s, i, CH_PULSE, 0, 0.100f, 0.100f, 1.00f, 0.250f);
    for (int i = 80; i < 88; ++i) set(s, i, CH_PULSE, 1, 0.010f, 0.150f, 0.80f, 0.150f, 3, 6.0f);
    for (int i = 88; i < 96; ++i) set(s, i, CH_PULSE, 2, 0.400f, 0.400f, 0.85f, 0.800f, 1, 0.35f);
    for (int i = 96; i < 104;++i) set(s, i, CH_PULSE, 3, 0.100f, 0.400f, 0.40f, 0.500f, 2, 3.0f);
    for (int i =104; i < 112;++i) set(s, i, CH_PULSE, 2, 0.010f, 0.300f, 0.50f, 0.200f);
    for (int i =112; i < 120;++i) set(s, i, CH_NOI,   2, 0.001f, 0.180f, 0.00f, 0.100f, 0, 0, 0, 8.0f);
    for (int i =120; i < 128;++i) set(s, i, CH_NOI,   2, 0.001f, 0.150f, 0.00f, 0.080f, 0, 0, 0, 8.0f);

    s.drum_noise_mode.store(1);
    s.drum_noise_rate_mult.store(8.0f);
    s.drum_attack.store(0.001f);
    s.drum_decay.store(0.180f);
    s.drum_sustain.store(0.0f);
    s.drum_release.store(0.100f);
}
