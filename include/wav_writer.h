#pragma once
#include <string>
#include <atomic>
#include "apu.h"
#include "midi_reader.h"

// progress — 0..100 (обновляется из фонового потока)
// cancel — если станет true, рендер завершается досрочно
bool render_to_wav(const std::wstring& path,
                   const MidiFile& mf,
                   const ChipSettings& settings,
                   int sample_rate,
                   float master_volume,
                   std::atomic<int>*  progress = nullptr,
                   std::atomic<bool>* cancel   = nullptr);
