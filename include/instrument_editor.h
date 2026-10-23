#pragma once
#include "apu.h"
#include "imgui.h"
#include <vector>
#include "dmc.h"

class ApuPlayer;

// Проиграть drum slot «вживую» через preview устройство
void play_drum_preview(const ChipSettings& s, int slot);

bool draw_instrument_editor(
    ChipSettings& s,
    ApuPlayer* player,
    int& selected_idx,
    const std::vector<DmcSample>* dmc_samples
);
