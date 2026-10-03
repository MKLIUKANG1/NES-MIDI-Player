#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct DmcSample {
    std::string name;                   // имя без расширения
    std::vector<uint8_t> data;          // raw byte format, 1 bit per sample
    int default_rate_index = 15;        // 0..15
};

// Загружает все .dmc/.bin из папки (не рекурсивно)
std::vector<DmcSample> load_dmc_folder(const std::wstring& dir);
