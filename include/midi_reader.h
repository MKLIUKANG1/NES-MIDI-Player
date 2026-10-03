#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct MidiNote {
    int    pitch;
    int    velocity;
    int    channel;
    int    program;
    int    track;
    double start_sec;
    double dur_sec;
};

struct MidiFile {
    std::vector<MidiNote> notes;
    double total_sec = 0.0;
    double tempo_bpm = 120.0;
    int    tracks    = 0;
    int    ticks_per_qn = 480;
};

// wide-версия: работает с кириллицей в путях (Windows)
bool load_midi_w(const std::wstring& path, MidiFile& out);
