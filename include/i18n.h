#pragma once
#include <string>
#include <vector>

enum class Lang { EN = 0, RU, ZH, COUNT };

struct LangInfo {
    const char* code;    // "en", "ru", "zh"
    const char* native;  // "English", "Русский", "中文"
    Lang        id;
};

const std::vector<LangInfo>& langs();
Lang        current_lang();
void        set_lang(Lang l);
const char* tr(const char* en);
const char* tr_drum_slot(int slot);

// Сохранение/загрузка языковых настроек
bool save_lang_pref(const std::wstring& path);
bool load_lang_pref(const std::wstring& path);

#define TR(s) tr(s)
