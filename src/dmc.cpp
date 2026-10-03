#include "dmc.h"
#include "util.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cwctype>

namespace fs = std::filesystem;

std::vector<DmcSample> load_dmc_folder(const std::wstring& dir) {
    std::vector<DmcSample> out;
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return out;

    for (auto& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file(ec)) continue;
        auto ext = entry.path().extension().wstring();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](wchar_t c){ return (wchar_t)std::towlower(c); });
        if (ext != L".dmc" && ext != L".bin") continue;

        std::ifstream f(entry.path(), std::ios::binary);
        if (!f) continue;
        DmcSample s;
        s.name = wide_to_utf8(entry.path().stem().wstring());
        s.data.assign(std::istreambuf_iterator<char>(f),
                      std::istreambuf_iterator<char>());
        if (s.data.empty()) continue;
        // Не позволяем огромных файлов (разумный предел)
        if (s.data.size() > 65535) s.data.resize(65535);
        out.push_back(std::move(s));
    }
    std::sort(out.begin(), out.end(),
              [](const DmcSample& a, const DmcSample& b){ return a.name < b.name; });
    return out;
}
