#include "i18n.h"
#include "util.h"
#include "apu.h"
#include <cstring>
#include <fstream>
#include <string>

struct Tr {
    const char* en;
    const char* ru;
    const char* zh;
};

// ==== Таблица переводов ====
static const Tr TR_TABLE[] = {
    // ===== Меню =====
    { "File",                       "Файл",                       "文件" },
    { "View",                       "Вид",                        "视图" },
    { "Playback",                   "Воспроизведение",            "播放" },
    { "Help",                       "Помощь",                     "帮助" },
    { "Language",                   "Язык",                       "语言" },
    { "Open MIDI...",               "Открыть MIDI...",            "打开 MIDI..." },
    { "Save Preset...",             "Сохранить пресет...",        "保存预设..." },
    { "Load Preset...",             "Загрузить пресет...",        "加载预设..." },
    { "Built-in Presets",           "Встроенные пресеты",         "内置预设" },
    { "Save as Default Preset",     "Сохранить как пресет по умолчанию", "保存为默认预设" },
    { "Refresh DPCM Samples",       "Обновить DPCM Samples",      "刷新 DPCM 采样" },
    { "Export to WAV...",           "Экспорт в WAV...",           "导出为 WAV..." },
    { "Exit",                       "Выход",                      "退出" },
    { "Instrument Editor",          "Редактор инструментов",      "乐器编辑器" },
    { "Piano Roll",                 "Piano Roll",                 "钢琴卷帘" },
    { "Play",                       "Играть",                     "播放" },
    { "Pause",                      "Пауза",                      "暂停" },
    { "Stop",                       "Стоп",                       "停止" },
    { "About...",                   "О программе...",             "关于..." },
    { "Close",                      "Закрыть",                    "关闭" },
    { "Cancel",                     "Отмена",                     "取消" },

    // ===== Пресеты =====
    { "Default",                    "По умолчанию",               "默认" },
    { "Castlevania-style",          "Стиль Castlevania",          "恶魔城风格" },
    { "Mega Man-style",             "Стиль Mega Man",             "洛克人风格" },
    { "Contra-style",               "Стиль Contra",               "魂斗罗风格" },
    { "Chrono Trigger-style",       "Стиль Chrono Trigger",       "时空之轮风格" },

    // ===== Транспорт =====
    { "Time: %.2f / %.2f s",         "Время: %.2f / %.2f с",       "时间: %.2f / %.2f 秒" },
    { "Vol %%",                     "Гром %%",                    "音量 %%" },
    { "Instruments...",             "Инструменты...",             "乐器..." },
    { "Drums: DPCM",                "Барабаны: DPCM",             "鼓: DPCM" },
    { "Drums: Noise",               "Барабаны: Noise",            "鼓: 噪声" },
    { "Toggle drum source (MIDI ch 9):\n"
      "  DPCM - play via DMC samples (if assigned)\n"
      "  Noise - play via noise channel",
      "Переключить источник барабанов (MIDI ch 9):\n"
      "  DPCM — играть через DMC-сэмплы (если назначены)\n"
      "  Noise — играть через шумовой канал",
      "切换鼓源 (MIDI 通道 9):\n"
      "  DPCM - 使用 DMC 采样 (若已分配)\n"
      "  噪声 - 使用噪声通道" },
    { "No file loaded. File -> Open MIDI... (Ctrl+O)",
      "Файл не загружен. Файл -> Открыть MIDI... (Ctrl+O)",
      "未加载文件。文件 -> 打开 MIDI... (Ctrl+O)" },
    { "File:",                      "Файл:",                      "文件:" },
    { "(none)",                     "(нет)",                      "(无)" },

    // ===== Окно «Каналы» =====
    { "Channels",                   "Каналы",                     "声道" },
    { "GM mapping",                 "GM-маппинг",                 "GM 映射" },
    { "Envelopes",                  "Огибающие",                  "包络" },
    { "Pulse 1 vol",                "Pulse 1 гром.",              "Pulse 1 音量" },
    { "Pulse 2 vol",                "Pulse 2 гром.",              "Pulse 2 音量" },
    { "Triangle vol",               "Triangle гром.",             "Triangle 音量" },
    { "Noise vol",                  "Noise гром.",                "噪声音量" },
    { "DMC vol",                    "DMC гром.",                  "DMC 音量" },
    { "Pulse 1 duty",               "Pulse 1 duty",               "Pulse 1 占空比" },
    { "Pulse 2 duty",               "Pulse 2 duty",               "Pulse 2 占空比" },
    { "Drum source (MIDI ch 9):",   "Источник барабанов (MIDI ch 9):", "鼓源 (MIDI 通道 9):" },
    { "DPCM (DMC samples)",         "DPCM (DMC сэмплы)",          "DPCM (DMC 采样)" },
    { "No samples. Put .dmc files in 'DPCM Samples'",
      "Нет сэмплов. Положите .dmc в 'DPCM Samples'",
      "无采样。请将 .dmc 文件放入 'DPCM Samples'" },
    { "Loaded DMC:",                "Загружено DMC:",             "已加载 DMC:" },
    { "Re-apply",                   "Применить заново",           "重新应用" },

    // ===== Окно «Сейчас играет» =====
    { "Now Playing",                "Сейчас играет",              "正在播放" },
    { "note %d (%.1f Hz)",          "нота %d (%.1f Гц)",          "音符 %d (%.1f 赫兹)" },
    { "active",                     "активен",                    "活跃" },
    { "Hover -> highlight in Piano Roll",
      "Наведите -> подсветка в Piano Roll",
      "悬停 -> 在钢琴卷帘中高亮" },

    // ===== Piano Roll =====
    { "no notes",                   "нет нот",                    "无音符" },
    { "%.1f - %.1f s  (zoom x%.1f)", "%.1f - %.1f с  (зум x%.1f)",
      "%.1f - %.1f 秒  (缩放 x%.1f)" },
    { "LMB = seek | Wheel = zoom | MMB-drag = pan",
      "ЛКМ = перемотка | Колесо = зум | СКМ-драг = панорама",
      "左键 = 定位 | 滚轮 = 缩放 | 中键拖动 = 平移" },

    // ===== Редактор инструментов =====
    { "Reset All",                  "Сбросить всё",               "全部重置" },
    { "filter...",                  "фильтр...",                  "筛选..." },
    { "DMC - Drum Slots (DPCM)",    "DMC — Drum Slots (DPCM)",    "DMC - 鼓槽 (DPCM)" },
    { "Drums - Noise fallback",     "Барабаны — Noise fallback",  "鼓 - 噪声后备" },
    { "Slot",                       "Слот",                       "槽位" },
    { "Sample",                     "Сэмпл",                      "采样" },
    { "Rate",                       "Скорость",                   "速率" },
    { "Vol",                        "Гром.",                      "音量" },
    { "Loop",                       "Цикл",                       "循环" },
    { "Test",                       "Тест",                       "测试" },
    { "LFSR mode",                  "Режим LFSR",                 "LFSR 模式" },
    { "Rate x",                     "Множ. частоты",              "频率倍数" },
    { "Fixed Hz",                   "Фикс. Гц",                   "固定赫兹" },
    { "Rate: 0 = 428 Hz (high), 15 = 54 Hz (low)",
      "Rate: 0 = 428 Гц (высокий), 15 = 54 Гц (низкий)",
      "速率: 0 = 428 赫兹 (高), 15 = 54 赫兹 (低)" },
    { "No .dmc samples in 'DPCM Samples'. Put files and File -> Refresh DPCM Samples.",
      "Нет .dmc сэмплов в папке 'DPCM Samples'. Положите файлы и нажмите Файл -> Обновить DPCM Samples.",
      "在 'DPCM Samples' 中没有 .dmc 采样。请放入文件并点击 文件 -> 刷新 DPCM 采样。" },

    // ===== Таблица инструментов =====
    { "#",                          "№",                          "序号" },
    { "Name",                       "Имя",                        "名称" },
    { "Channel",                    "Канал",                      "声道" },
    { "Duty",                       "Duty",                       "占空比" },
    { "A D S R",                    "A D S R",                    "A D S R" },
    { "Sweep",                      "Sweep",                      "扫频" },
    { "Noise",                      "Noise",                      "噪声" },
    { "Pulse",                      "Pulse",                      "脉冲" },
    { "Triangle",                   "Triangle",                   "三角波" },
    { "long LFSR",                  "длинный LFSR",               "长 LFSR" },
    { "short LFSR",                 "короткий LFSR",              "短 LFSR" },
    { "off",                        "выкл",                       "关闭" },
    { "up-down",                    "вверх-вниз",                 "上下" },
    { "cycle",                      "цикл",                       "循环" },
    { "alt",                        "чередование",                "交替" },

    // ===== Envelope editor =====
    { "Envelope Editor",            "Редактор огибающих",         "包络编辑器" },
    { "Instrument",                 "Инструмент",                 "乐器" },
    { "Volume",                     "Громкость",                  "音量" },
    { "Pitch",                      "Высота",                     "音高" },
    { "Arp",                        "Арпеджио",                   "琶音" },
    { "LMB - draw, RMB - erase",    "ЛКМ — рисовать, ПКМ — стереть", "左键 - 绘制, 右键 - 擦除" },
    { "len",                        "длина",                      "长度" },
    { "spd",                        "скор.",                      "速度" },
    { "clear",                      "очистить",                   "清除" },

    // ===== Экспорт =====
    { "Export WAV",                 "Экспорт WAV",                "导出 WAV" },

    // ===== About =====
    { "About",                      "О программе",                "关于" },
    { "Author:",                    "Автор:",                     "作者:" },
    { "Year:",                      "Год:",                       "年份:" },
    { "Build:",                     "Сборка:",                    "构建:" },
    { "Libs:",                      "Библиотеки:",                "库:" },
    { "Thanks to everyone supporting the NES chiptune scene!",
      "Спасибо всем, кто поддерживает NES-чиптюн сцену!",
      "感谢所有支持 NES 芯片音乐的人！" },

    // ===== Статус-сообщения =====
    { "Loaded:",                    "Загружено:",                 "已加载:" },
    { "notes",                      "нот",                        "个音符" },
    { "Preset saved",               "Пресет сохранён",            "预设已保存" },
    { "Preset load error",          "Ошибка загрузки пресета",    "预设加载错误" },
    { "Preset loaded",              "Пресет загружен",            "预设已加载" },
    { "Preset save error",          "Ошибка сохранения пресета",  "预设保存错误" },
    { "Saved: Presets/default.nespreset", "Сохранено: Presets/default.nespreset",
      "已保存: Presets/default.nespreset" },
    { "Error saving default preset", "Ошибка сохранения default preset",
      "保存默认预设时出错" },
    { "WAV saved",                  "WAV сохранён",               "WAV 已保存" },
    { "Export cancelled",           "Экспорт отменён",            "导出已取消" },
    { "WAV error",                  "Ошибка WAV",                 "WAV 错误" },
    { "Export already running",     "Экспорт уже идёт",           "导出正在进行" },
    { "No data",                    "Нет данных",                 "无数据" },
    { "DMC:",                       "DMC:",                       "DMC:" },
    { "samples",                    "сэмплов",                    "个采样" },

    // ===== Диалоги файлов =====
    { "Open MIDI File",             "Открыть MIDI файл",          "打开 MIDI 文件" },
    { "Save Preset",                "Сохранить пресет",           "保存预设" },
    { "Load Preset",                "Загрузить пресет",           "加载预设" },
    { "Export to WAV",              "Экспорт в WAV",              "导出为 WAV" },
    { "MIDI Files (*.mid;*.midi)",  "MIDI-файлы (*.mid;*.midi)",  "MIDI 文件 (*.mid;*.midi)" },
    { "All Files (*.*)",            "Все файлы (*.*)",            "所有文件 (*.*)" },
    { "NES Preset (*.nespreset)",   "Пресет NES (*.nespreset)",   "NES 预设 (*.nespreset)" },
    { "WAV Files (*.wav)",          "WAV-файлы (*.wav)",          "WAV 文件 (*.wav)" },

    // ===== Drum slots =====
    { "Kick",       "Бочка",       "底鼓" },
    { "Snare",      "Малый",       "军鼓" },
    { "Snare 2",    "Малый 2",     "军鼓 2" },
    { "Clap",       "Хлопок",      "拍手" },
    { "HiHat",      "Хэт",         "踩镲" },
    { "HiHat Open", "Хэт откр.",   "开镲" },
    { "Tom",        "Том",         "嗵鼓" },
    { "Cymbal",     "Тарелка",     "镲片" },

    // ===== Misc =====
    { "Ready",      "Готов",       "就绪" },
};

static Lang g_lang = Lang::EN;

const std::vector<LangInfo>& langs() {
    static const std::vector<LangInfo> v = {
        { "en", "English",  Lang::EN },
        { "ru", "Русский",  Lang::RU },
        { "zh", "中文",     Lang::ZH },
    };
    return v;
}

Lang current_lang() { return g_lang; }
void set_lang(Lang l) {
    if ((int)l >= 0 && (int)l < (int)Lang::COUNT) g_lang = l;
}

const char* tr(const char* en) {
    if (!en) return "";
    if (g_lang == Lang::EN) return en;
    for (const auto& t : TR_TABLE) {
        if (std::strcmp(t.en, en) == 0) {
            if (g_lang == Lang::RU) return t.ru ? t.ru : t.en;
            if (g_lang == Lang::ZH) return t.zh ? t.zh : t.en;
            return t.en;
        }
    }
    return en;
}

const char* tr_drum_slot(int slot) {
    if (slot < 0 || slot >= DRUM_COUNT) return "";
    return TR(DRUM_SLOT_NAMES[slot]);
}

bool save_lang_pref(const std::wstring& path) {
    std::ofstream f(wide_to_utf8(path));
    if (!f) return false;
    const auto& ls = langs();
    int idx = (int)g_lang;
    if (idx < 0 || idx >= (int)ls.size()) idx = 0;
    f << ls[idx].code << "\n";
    return f.good();
}

bool load_lang_pref(const std::wstring& path) {
    std::ifstream f(wide_to_utf8(path));
    if (!f) return false;
    std::string code;
    std::getline(f, code);
    while (!code.empty() && (code.back()=='\r' || code.back()=='\n' || code.back()==' '))
        code.pop_back();
    for (const auto& li : langs()) {
        if (code == li.code) { g_lang = li.id; return true; }
    }
    return false;
}
