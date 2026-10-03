#include "wav_writer.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>

#pragma pack(push, 1)
struct WavHeader {
    char     riff[4];
    uint32_t size;
    char     wave[4];
    char     fmt[4];
    uint32_t fmt_size;
    uint16_t audio_fmt;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     data[4];
    uint32_t data_size;
};
#pragma pack(pop)

bool render_to_wav(const std::wstring& path,
                   const MidiFile& mf,
                   const ChipSettings& settings,
                   int sample_rate,
                   float master_volume,
                   std::atomic<int>*  progress,
                   std::atomic<bool>* cancel)
{
    FILE* f = _wfopen(path.c_str(), L"wb");
    if (!f) { if (progress) progress->store(-1); return false; }

    const int total_samples = (int)((mf.total_sec + 0.5) * sample_rate);
    const uint32_t data_bytes = (uint32_t)total_samples * 2 * sizeof(int16_t);

    WavHeader hdr{};
    memcpy(hdr.riff, "RIFF", 4); hdr.size = 36 + data_bytes;
    memcpy(hdr.wave, "WAVE", 4);
    memcpy(hdr.fmt,  "fmt ", 4); hdr.fmt_size = 16;
    hdr.audio_fmt       = 1;
    hdr.channels        = 2;
    hdr.sample_rate     = (uint32_t)sample_rate;
    hdr.byte_rate       = (uint32_t)sample_rate * 4;
    hdr.block_align     = 4;
    hdr.bits_per_sample = 16;
    memcpy(hdr.data, "data", 4); hdr.data_size = data_bytes;
    fwrite(&hdr, sizeof(hdr), 1, f);

    NesApu apu; apu.set_sample_rate(sample_rate);
    ApuPlayer tmp(apu, mf);
    tmp.settings().copy_from(settings);
    apu.set_settings(&tmp.settings());
    tmp.set_master_volume(master_volume);
    tmp.play();

    std::vector<int16_t> buf(1024 * 2);
    int remaining = total_samples;
    int last_pct = -1;
    while (remaining > 0) {
        if (cancel && cancel->load()) {
            fclose(f);
            if (progress) progress->store(-1);
            return false;
        }
        int n = std::min(remaining, 1024);
        tmp.generate(buf.data(), n);
        fwrite(buf.data(), sizeof(int16_t), (size_t)n * 2, f);
        remaining -= n;

        if (progress) {
            int pct = (int)(100.0 * (total_samples - remaining) / total_samples);
            if (pct != last_pct) { progress->store(pct); last_pct = pct; }
        }
    }
    fclose(f);
    if (progress) progress->store(100);
    return true;
}
