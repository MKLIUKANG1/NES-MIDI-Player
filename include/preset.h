#pragma once
#include <string>
#include "apu.h"

bool save_preset(const std::wstring& path, const ChipSettings& s);
bool load_preset(const std::wstring& path, ChipSettings& s);
