#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <SDL2/SDL_syswm.h>

#include <cstdio>
#include <cstring>
#include <cctype>
#include <iostream>
#include <string>
#include <cmath>
#include <thread>
#include <atomic>
#include <vector>
#include <filesystem>
#include "midi_reader.h"
#include "apu.h"
#include "dmc.h"
#include "gm_names.h"
#include "preset.h"
#include "wav_writer.h"
#include "builtin_presets.h"
#include "util.h"
#include "version.h"
#include "i18n.h"
#include "env_editor.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

static ApuPlayer* g_player = nullptr;
static int g_env_edit_idx = -1;
static void audio_cb(void*, Uint8* stream, int len) {
    int n = len / (2 * (int)sizeof(int16_t));
    if (g_player) g_player->generate((int16_t*)stream, n);
    else          SDL_memset(stream, 0, len);
}

// NTSC DMC rate table (Hz)
static const double DMC_RATE_HZ[16] = {
    4181.71, 4709.17, 5264.23, 5598.97, 6257.91, 7042.71, 7919.35, 8363.42,
    9419.17, 11186.09, 12604.33, 13982.42, 16884.52, 21306.23, 24859.85, 33143.92
};

// Отдельное SDL-устройство для preview DMC (SDL_QueueAudio)
static SDL_AudioDeviceID g_preview_dev = 0;
static int               g_preview_sr  = 44100;

static void play_dmc_preview(const DmcSample& smp, int rate_index, int volume) {
    if (!g_preview_dev) return;
    if (smp.data.empty()) return;

    double rate_hz = DMC_RATE_HZ[std::max(0, std::min(15, rate_index))];
    double dur_sec = smp.data.size() * 8.0 / rate_hz + 0.05;
    int total_samples = (int)(dur_sec * g_preview_sr);
    if (total_samples <= 0) return;
    if (total_samples > (int)g_preview_sr * 30) total_samples = g_preview_sr * 30;  // max 30s

    std::vector<int16_t> buf(total_samples * 2);
    int level = 64;
    int bit_pos = 0;
    size_t byte_pos = 0;
    double phase = 0.0;

    for (int i = 0; i < total_samples; ++i) {
        phase += rate_hz / (double)g_preview_sr;
        int guard = 64;
        while (phase >= 1.0 && guard-- > 0) {
            phase -= 1.0;
            if (byte_pos >= smp.data.size()) break;
            uint8_t b = smp.data[byte_pos];
            int bit = (b >> (7 - bit_pos)) & 1;
            if (bit) level = std::min(127, level + 2);
            else     level = std::max(0,   level - 2);
            bit_pos++;
            if (bit_pos >= 8) { bit_pos = 0; byte_pos++; }
        }
        float s = (level - 64) / 64.0f * (volume / 15.0f);
        int16_t v = (int16_t)(s * 26000.0f);
        buf[2*i] = v; buf[2*i+1] = v;
    }
    SDL_ClearQueuedAudio(g_preview_dev);
    SDL_QueueAudio(g_preview_dev, buf.data(),
                   (Uint32)(total_samples * 2 * sizeof(int16_t)));
}

// ---------- Нативные диалоги ----------
static bool dlg_open(SDL_Window* win, std::wstring& out,
                     const wchar_t* filter, const wchar_t* title) {
    SDL_SysWMinfo wi; SDL_VERSION(&wi.version);
    HWND hwnd = nullptr;
    if (SDL_GetWindowWMInfo(win, &wi)) hwnd = wi.info.win.window;
    wchar_t buf[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = filter; ofn.lpstrFile = buf; ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn)) return false;
    out = buf; return true;
}
static bool dlg_save(SDL_Window* win, std::wstring& out,
                     const wchar_t* filter, const wchar_t* title,
                     const wchar_t* def_ext) {
    SDL_SysWMinfo wi; SDL_VERSION(&wi.version);
    HWND hwnd = nullptr;
    if (SDL_GetWindowWMInfo(win, &wi)) hwnd = wi.info.win.window;
    wchar_t buf[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = filter; ofn.lpstrFile = buf; ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title; ofn.lpstrDefExt = def_ext;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&ofn)) return false;
    out = buf; return true;
}

// ---------- ADSR-график ----------
static void draw_adsr_graph(float A, float D, float S, float R, ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);
    dl->AddRectFilled(p0, p1, IM_COL32(25, 25, 35, 255), 3);
    dl->AddRect      (p0, p1, IM_COL32(70, 70, 90, 255), 3);
    for (int k = 1; k <= 2; ++k) {
        float y = p1.y - 3 - (size.y - 6) * (k * 0.5f);
        dl->AddLine(ImVec2(p0.x, y), ImVec2(p1.x, y), IM_COL32(50,50,60,120));
    }
    const float sustain_show = 0.25f;
    float total = A + D + sustain_show + R;
    if (total < 0.01f) total = 0.01f;
    float xs = size.x / total;
    float ys = size.y - 6;
    auto pt = [&](float t, float v) { return ImVec2(p0.x + t*xs, p1.y - 3 - v*ys); };
    ImVec2 pts[6] = {
        pt(0,0), pt(A,1), pt(A+D,S), pt(A+D+sustain_show,S),
        pt(A+D+sustain_show+R,0), pt(total,0)
    };
    dl->AddConvexPolyFilled(pts, 6, IM_COL32(80,160,130,60));
    dl->AddPolyline(pts, 6, IM_COL32(120,220,180,255), 0, 1.8f);
    dl->AddLine(pt(A,0), pt(A,1), IM_COL32(200,200,120,90));
    dl->AddLine(pt(A+D,0), pt(A+D,S), IM_COL32(200,200,120,90));
    ImGui::Dummy(size);
}

// ---------- Piano Roll ----------
static float  pr_zoom = 1.0f;
static double pr_pan  = 0.0;

static void draw_piano_roll(ApuPlayer& player, ImVec2 size, int highlight_ch) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);
    dl->AddRectFilled(p0, p1, IM_COL32(18,18,24,255), 3);
    dl->AddRect      (p0, p1, IM_COL32(80,80,90,255), 3);

    const auto& notes = player.raw_notes();
    double total = player.total_time();
    if (total <= 0.0 || notes.empty()) {
        dl->AddText(ImVec2(p0.x+8, p0.y+8), IM_COL32(160,160,180,255), TR("no notes"));
        ImGui::Dummy(size);
        return;
    }

    if (pr_zoom < 1.0f) pr_zoom = 1.0f;
    if (pr_zoom > 64.0f) pr_zoom = 64.0f;
    double view_span = total / pr_zoom;
    double t0 = pr_pan, t1 = t0 + view_span;
    if (t1 > total) { t1 = total; t0 = t1 - view_span; if (t0 < 0) t0 = 0; }
    pr_pan = t0;
    view_span = t1 - t0;

    ImGui::InvisibleButton("##pr_canvas", size);
    bool hovered = ImGui::IsItemHovered();
    ImGuiIO& io = ImGui::GetIO();

    if (hovered && io.MouseWheel != 0.0f) {
        ImVec2 m = ImGui::GetMousePos();
        double rel = (m.x - p0.x) / size.x;
        double cur_t = t0 + rel * view_span;
        float factor = (io.MouseWheel > 0) ? 1.25f : 1.0f/1.25f;
        pr_zoom *= factor;
        if (pr_zoom < 1.0f) pr_zoom = 1.0f;
        if (pr_zoom > 64.0f) pr_zoom = 64.0f;
        double ns = total / pr_zoom;
        pr_pan = cur_t - rel * ns;
    }
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(2, 0.0f)) {
        double dx = io.MouseDelta.x / size.x * view_span;
        pr_pan -= dx;
        if (pr_pan < 0) pr_pan = 0;
        double max_pan = total - view_span;
        if (max_pan < 0) max_pan = 0;
        if (pr_pan > max_pan) pr_pan = max_pan;
    }
    if (hovered && ImGui::IsMouseClicked(0)) {
        ImVec2 m = ImGui::GetMousePos();
        double t = t0 + (m.x - p0.x) / size.x * view_span;
        if (t < 0) t = 0;
        if (t > total) t = total;
        player.seek(t);
    }

    int min_p = 127, max_p = 0;
    for (const auto& n : notes) {
        if (n.pitch < min_p) min_p = n.pitch;
        if (n.pitch > max_p) max_p = n.pitch;
    }
    if (max_p - min_p < 12) { int c = (max_p+min_p)/2; min_p = c-6; max_p = c+6; }
    int span = max_p - min_p + 1;

    double xs = size.x / view_span;
    double ys = (double)size.y / span;

    for (int p = (min_p/12)*12; p <= max_p; p += 12) {
        if (p < min_p) continue;
        float y = p1.y - (float)((p - min_p + 1) * ys);
        dl->AddLine(ImVec2(p0.x, y), ImVec2(p1.x, y), IM_COL32(40,40,55,150));
    }

    static const ImU32 CH_COL[3] = {
        IM_COL32(120,220,120,220),
        IM_COL32(255,180,120,220),
        IM_COL32(255,120,140,220),
    };
    static const ImU32 CH_COL_HL[3] = {
        IM_COL32(180,255,180,255),
        IM_COL32(255,220,180,255),
        IM_COL32(255,180,200,255),
    };

    const auto& s = player.settings();
    for (const auto& n : notes) {
        double n_end = n.start_sec + n.dur_sec;
        if (n_end < t0 || n.start_sec > t1) continue;
        int ch = derive_channel(n, s);
        if (ch < 0) ch = 0;
        if (ch > 2) ch = 2;
        bool dim  = (highlight_ch != -1 && highlight_ch != ch);
        ImU32 col = dim ? IM_COL32(80,80,90,120)
                        : (highlight_ch == ch ? CH_COL_HL[ch] : CH_COL[ch]);
        float x1 = p0.x + (float)((n.start_sec - t0) * xs);
        float x2 = p0.x + (float)((n_end       - t0) * xs);
        float y1 = p1.y - (float)((n.pitch - min_p + 1) * ys);
        float y2 = p1.y - (float)((n.pitch - min_p) * ys);
        if (x2 - x1 < 1.0f) x2 = x1 + 1.0f;
        if (y2 - y1 < 1.5f) y2 = y1 + 1.5f;
        if (x1 < p0.x) x1 = p0.x;
        if (x2 > p1.x) x2 = p1.x;
        dl->AddRectFilled(ImVec2(x1,y1), ImVec2(x2,y2), col);
    }

    double ct = player.current_time();
    if (ct >= t0 && ct <= t1) {
        float px = p0.x + (float)((ct - t0) * xs);
        dl->AddLine(ImVec2(px, p0.y), ImVec2(px, p1.y),
                    IM_COL32(255,240,120,230), 1.8f);
    }

    char info[128];
    snprintf(info, sizeof(info), TR("%.1f - %.1f s  (zoom x%.1f)"), t0, t1, pr_zoom);
    dl->AddText(ImVec2(p0.x + 6, p1.y - 18), IM_COL32(180,180,200,200), info);
}

// ---------- Export state ----------
static std::thread       g_export_thread;
static std::atomic<int>  g_export_progress{0};
static std::atomic<bool> g_export_running{false};
static std::atomic<bool> g_export_cancel{false};
static std::atomic<bool> g_export_ok{false};
static bool              g_export_popup_open = false;

static bool draw_instrument_row(Instrument& I) {
    float A = I.attack .load();
    float D = I.decay  .load();
    float S = I.sustain.load();
    float R = I.release.load();
    bool changed = false;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2,2));
    ImGui::SetNextItemWidth(78);
    if (ImGui::SliderFloat("##A",&A,0,1,"A%.2f")) I.attack.store(A);
    if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(78);
    if (ImGui::SliderFloat("##D",&D,0,1,"D%.2f")) I.decay.store(D);
    if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(78);
    if (ImGui::SliderFloat("##S",&S,0,1,"S%.2f")) I.sustain.store(S);
    if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(78);
    if (ImGui::SliderFloat("##R",&R,0,2,"R%.2f")) I.release.store(R);
    if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;
    ImGui::PopStyleVar();
    return changed;
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* win = SDL_CreateWindow(APP_NAME " v" APP_VERSION " — by " APP_AUTHOR,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 900, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl = SDL_GL_CreateContext(win);
    SDL_GL_MakeCurrent(win, gl);
    SDL_GL_SetSwapInterval(1);

    // Основное аудио-устройство (с callback)
    SDL_AudioSpec want{}, have{};
    want.freq = 44100; want.format = AUDIO_S16SYS;
    want.channels = 2;  want.samples = 1024;
    want.callback = audio_cb;
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);

    // Preview-устройство (без callback) — для Test DMC
    {
        SDL_AudioSpec pw{}, ph{};
        pw.freq = have.freq;
        pw.format = AUDIO_S16SYS;
        pw.channels = 2;
        pw.samples = 1024;
        pw.callback = nullptr;
        g_preview_dev = SDL_OpenAudioDevice(nullptr, 0, &pw, &ph, 0);
        g_preview_sr  = ph.freq ? ph.freq : 44100;
        if (g_preview_dev) SDL_PauseAudioDevice(g_preview_dev, 0);
        printf("Preview device: %s (%d Hz)\n",
               g_preview_dev ? "OK" : SDL_GetError(), g_preview_sr);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    ImFont* fnt = io.Fonts->AddFontFromFileTTF(
        "C:/Windows/Fonts/segoeui.ttf", 17.0f,
        nullptr, io.Fonts->GetGlyphRangesCyrillic());
    if (!fnt) fnt = io.Fonts->AddFontFromFileTTF(
        "C:/Windows/Fonts/consola.ttf", 17.0f,
        nullptr, io.Fonts->GetGlyphRangesCyrillic());
    if (!fnt) io.Fonts->AddFontDefault();

    ImGui_ImplSDL2_InitForOpenGL(win, gl);
    ImGui_ImplOpenGL3_Init("#version 150");

    // ===== Загрузка DMC-сэмплов =====
    std::wstring dmc_dir = L"DPCM Samples";
    {
        wchar_t exe_path[MAX_PATH];
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
        std::wstring exe_dir(exe_path);
        size_t slash = exe_dir.find_last_of(L"\\/");
        if (slash != std::wstring::npos) exe_dir.resize(slash);
        std::wstring try1 = exe_dir + L"\\DPCM Samples";
        std::wstring try2 = exe_dir + L"\\..\\DPCM Samples";
        std::error_code ec;
        if (std::filesystem::exists(try1, ec)) dmc_dir = try1;
        else if (std::filesystem::exists(try2, ec)) dmc_dir = try2;
    }
    // ===== Загрузка языковых настроек =====
    std::wstring exe_dir_lang;
    {
        wchar_t p[MAX_PATH];
        GetModuleFileNameW(nullptr, p, MAX_PATH);
        exe_dir_lang = p;
        size_t sl = exe_dir_lang.find_last_of(L"\\/");
        if (sl != std::wstring::npos) exe_dir_lang.resize(sl);
    }
    std::wstring lang_pref_path = exe_dir_lang + L"\\Presets\\language.txt";
    load_lang_pref(lang_pref_path);

    std::vector<DmcSample> dmc_samples = load_dmc_folder(dmc_dir);
    printf("DMC samples loaded: %zu (from '%s')\n",
           dmc_samples.size(), wide_to_utf8(dmc_dir).c_str());

    // ===== Загрузка пресета по умолчанию =====
    std::wstring exe_dir_for_preset;
    {
        wchar_t p[MAX_PATH];
        GetModuleFileNameW(nullptr, p, MAX_PATH);
        exe_dir_for_preset = p;
        size_t sl = exe_dir_for_preset.find_last_of(L"\\/");
        if (sl != std::wstring::npos) exe_dir_for_preset.resize(sl);
    }
    std::wstring presets_dir        = exe_dir_for_preset + L"\\Presets";
    std::wstring default_preset_path = presets_dir + L"\\default.nespreset";

    ChipSettings startup_settings;   // по умолчанию — встроенные значения
    if (std::filesystem::exists(default_preset_path)) {
        if (load_preset(default_preset_path, startup_settings)) {
            printf("Default preset loaded: %s\n",
                   wide_to_utf8(default_preset_path).c_str());
        } else {
            printf("Failed to parse default preset, using built-in\n");
        }
    } else {
        printf("Default preset not found at '%s', using built-in\n",
               wide_to_utf8(default_preset_path).c_str());
    }

    NesApu apu; apu.set_sample_rate(have.freq);
    ApuPlayer* player = nullptr;
    MidiFile   current_mf;
    std::wstring current_file_w;
    std::string  current_file_u8;
    int  global_vol_pct = 80;
    bool show_editor = false;
    bool show_pianoroll = true;
    int  highlight_ch = -1;
    std::string status_msg;
    double status_until = 0.0;

    auto set_status = [&](const std::string& msg){
        status_msg   = msg;
        status_until = (double)SDL_GetTicks() / 1000.0 + 3.0;
    };

    auto load_file = [&](const std::wstring& wpath) -> bool {
        MidiFile mf;
        if (!load_midi_w(wpath, mf)) return false;
        if (mf.notes.empty()) return false;
        SDL_LockAudioDevice(dev);
        g_player = nullptr;
        delete player;
        player = new ApuPlayer(apu, mf);
        player->settings().copy_from(startup_settings);   // применяем default-пресет
        apu.set_settings(&player->settings());
        player->set_dmc_samples(&dmc_samples);
        player->set_master_volume(global_vol_pct / 100.0f);
        g_player = player;
        current_mf = mf;
        current_file_w  = wpath;
        current_file_u8 = wide_to_utf8(wpath);
        SDL_UnlockAudioDevice(dev);
        player->play();
        pr_zoom = 1.0f; pr_pan = 0.0;
        set_status(std::string(TR("Loaded:")) + " " + std::to_string(mf.notes.size()) + " " + TR("notes"));
        return true;
    };

    auto start_export = [&](const std::wstring& p) {
        if (g_export_running.load()) { set_status(TR("Export already running")); return; }
        if (!player) { set_status(TR("No data")); return; }
        g_export_cancel.store(false);
        g_export_progress.store(0);
        g_export_ok.store(false);
        g_export_running.store(true);
        MidiFile    mf_copy  = current_mf;
        ChipSettings set_cp; set_cp.copy_from(player->settings());
        int   sr  = have.freq;
        float vol = global_vol_pct / 100.0f;
        if (g_export_thread.joinable()) g_export_thread.join();
        g_export_thread = std::thread([p, mf_copy, set_cp, sr, vol]() {
            bool ok = render_to_wav(p, mf_copy, set_cp, sr, vol,
                                    &g_export_progress, &g_export_cancel);
            g_export_ok.store(ok);
            g_export_running.store(false);
        });
        g_export_popup_open = true;
    };

    if (argc >= 2) load_file(utf8_to_wide(argv[1]));
    SDL_PauseAudioDevice(dev, 0);

    bool running = true;
    int  win_w = 1280, win_h = 900;

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT) running = false;
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (ev.key.keysym.sym == SDLK_SPACE && player) {
                    player->is_playing() ? player->pause() : player->play();
                }
                if ((ev.key.keysym.mod & KMOD_CTRL) && ev.key.keysym.sym == SDLK_o) {
                    std::wstring p;
                    if (dlg_open(win, p,
                        L"MIDI Files (*.mid;*.midi)\0*.mid;*.midi\0All Files (*.*)\0*.*\0",
                        L"Открыть MIDI файл"))
                        load_file(p);
                }
            }
            if (ev.type == SDL_WINDOWEVENT &&
                ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                win_w = ev.window.data1; win_h = ev.window.data2;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // ===== Меню =====
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu(TR("File"))) {
                if (ImGui::MenuItem(TR("Open MIDI..."), "Ctrl+O")) {
                    std::wstring p;
                    if (dlg_open(win, p,
                        L"MIDI Files (*.mid;*.midi)\0*.mid;*.midi\0All Files (*.*)\0*.*\0",
                        L"Открыть MIDI файл"))
                        load_file(p);
                }
                ImGui::Separator();
                if (player) {
                    if (ImGui::MenuItem(TR("Save Preset..."))) {
                        std::wstring p;
                        if (dlg_save(win, p,
                            L"NES Preset (*.nespreset)\0*.nespreset\0All Files (*.*)\0*.*\0",
                            L"Сохранить пресет", L"nespreset"))
                            set_status(save_preset(p, player->settings()) ?
                                       TR("Preset saved") : TR("Preset save error"));
                    }
                    if (ImGui::MenuItem(TR("Load Preset..."))) {
                        std::wstring p;
                        if (dlg_open(win, p,
                            L"NES Preset (*.nespreset)\0*.nespreset\0All Files (*.*)\0*.*\0",
                            L"Загрузить пресет")) {
                            if (load_preset(p, player->settings())) {
                                player->remap();
                                set_status(TR("Preset loaded"));
                            } else set_status(TR("Preset load error"));
                        }
                    }
                    if (ImGui::BeginMenu(TR("Built-in Presets"))) {
                        if (ImGui::MenuItem("Default"))              { preset_default   (player->settings()); player->remap(); set_status("Default"); }
                        if (ImGui::MenuItem("Castlevania-style"))    { preset_castlevania(player->settings()); player->remap(); set_status("Castlevania"); }
                        if (ImGui::MenuItem("Mega Man-style"))       { preset_megaman   (player->settings()); player->remap(); set_status("Mega Man"); }
                        if (ImGui::MenuItem("Contra-style"))         { preset_contra    (player->settings()); player->remap(); set_status("Contra"); }
                        if (ImGui::MenuItem("Chrono Trigger-style")) { preset_chrono    (player->settings()); player->remap(); set_status("Chrono Trigger"); }
                        ImGui::EndMenu();
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem(TR("Export to WAV..."))) {
                        std::wstring p;
                        if (dlg_save(win, p,
                            L"WAV Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0",
                            L"Экспорт в WAV", L"wav"))
                            start_export(p);
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem(TR("Save as Default Preset"))) {
                        std::error_code ec;
                        std::filesystem::create_directories(presets_dir, ec);
                        if (save_preset(default_preset_path, player->settings())) {
                            startup_settings.copy_from(player->settings());
                            set_status(TR("Saved: Presets/default.nespreset"));
                        } else {
                            set_status(TR("Error saving default preset"));
                        }
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem(TR("Refresh DPCM Samples"))) {
                        dmc_samples = load_dmc_folder(dmc_dir);
                        set_status(std::string(TR("DMC:")) + " " + std::to_string(dmc_samples.size()) + " " + TR("samples"));
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem(TR("Exit"), "Esc")) running = false;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(TR("View"))) {
                ImGui::MenuItem(TR("Instrument Editor"), nullptr, &show_editor);
                ImGui::MenuItem("Piano Roll",             nullptr, &show_pianoroll);
                ImGui::Separator();
                if (ImGui::BeginMenu(TR("Language"))) {
                    for (const auto& li : langs()) {
                        bool sel = (current_lang() == li.id);
                        if (ImGui::MenuItem(li.native, nullptr, sel)) {
                            set_lang(li.id);
                            save_lang_pref(lang_pref_path);
                            SDL_SetWindowTitle(win,
                                APP_NAME " v" APP_VERSION " — by " APP_AUTHOR);
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(TR("Playback"))) {
                if (player) {
                    if (ImGui::MenuItem(TR("Play"), "Space")) player->play();
                    if (ImGui::MenuItem(TR("Pause")))           player->pause();
                    if (ImGui::MenuItem(TR("Stop")))            player->stop();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(TR("Help"))) {
                if (ImGui::MenuItem(TR("About...")))
                    ImGui::OpenPopup("About");
                ImGui::Separator();
                ImGui::MenuItem(APP_NAME " v" APP_VERSION, nullptr, false, false);
                ImGui::MenuItem(("Автор: " APP_AUTHOR),   nullptr, false, false);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // ===== About popup =====
        if (ImGui::BeginPopupModal("About", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.9f, 1.0f, 1.0f));
            ImGui::Text("%s", APP_NAME " v" APP_VERSION);
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::TextWrapped("%s", APP_DESCRIPTION);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Автор:  " APP_AUTHOR);
            ImGui::Text("Год:    " APP_YEAR);
            ImGui::Text("Сборка: MSYS2 + MinGW-w64");
            ImGui::Text("Лibs:   SDL2, Dear ImGui, portsmf-fallback");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.7f,0.7f,0.7f,1),
                               "Спасибо всем, кто поддерживает NES-чиптюн сцену!");
            ImGui::Spacing();
            if (ImGui::Button(TR("Close"), ImVec2(120, 30)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // ===== Модальное окно экспорта =====
        if (g_export_popup_open) { ImGui::OpenPopup(TR("Export WAV")); g_export_popup_open = false; }
        if (ImGui::BeginPopupModal(TR("Export WAV"), nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        {
            int pct = g_export_progress.load();
            if (pct < 0) pct = 0;
            ImGui::ProgressBar(pct / 100.0f, ImVec2(420, 24));
            ImGui::Text("%d %%", pct);
            if (g_export_running.load()) {
                if (ImGui::Button(TR("Cancel"), ImVec2(120, 30)))
                    g_export_cancel.store(true);
            } else {
                if (g_export_thread.joinable()) g_export_thread.join();
                if (g_export_ok.load()) set_status(TR("WAV saved"));
                else set_status(g_export_cancel.load() ? TR("Export cancelled") : TR("WAV error"));
                if (ImGui::Button(TR("Close"), ImVec2(120, 30)))
                    ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // ===== Транспорт =====
        ImGui::SetNextWindowPos (ImVec2(0, 20), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2((float)win_w, 100), ImGuiCond_Always);
        ImGui::Begin("##transport", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        if (player) {
            auto& s = player->settings();

            if (ImGui::Button(player->is_playing() ? TR("Pause") : TR("Play"), ImVec2(90, 30)))
                player->is_playing() ? player->pause() : player->play();
            ImGui::SameLine();
            if (ImGui::Button(TR("Stop"), ImVec2(70, 30))) player->stop();
            ImGui::SameLine();

            // === Большой цветной переключатель DPCM / Noise для барабанов ===
            bool ud = s.use_dpcm_drums.load();
            if (ud) ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.20f,0.55f,0.25f,1));
            else    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f,0.25f,0.25f,1));
            if (ud) ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f,0.70f,0.35f,1));
            else    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f,0.35f,0.35f,1));
            if (ImGui::Button(ud ? TR("Drums: DPCM") : TR("Drums: Noise"), ImVec2(180, 30))) {
                s.use_dpcm_drums.store(!ud);
                player->remap();
            }
            ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Переключить источник барабанов (MIDI ch 9):\n"
                                  "  DPCM — играть через DMC-сэмплы (если назначены)\n"
                                  "  Noise — играть через шумовой канал");

            ImGui::SameLine();
            ImGui::Text(TR("Time: %.2f / %.2f s"), player->current_time(), player->total_time());
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            if (ImGui::SliderInt(TR("Vol %%"), &global_vol_pct, 0, 100))
                player->set_master_volume(global_vol_pct / 100.0f);
            ImGui::SameLine();
            if (ImGui::Button(TR("Instruments..."))) show_editor = true;
            ImGui::SameLine();
            ImGui::Text("| %s", current_file_u8.empty() ? TR("(none)") : current_file_u8.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.55f,0.75f,0.9f,1),
                               "  [" APP_NAME " v" APP_VERSION " by " APP_AUTHOR "]");

            float cur = (float)player->current_time();
            float tot = (float)player->total_time();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat("##seek", &cur, 0.0f, tot, "%.2f s"))
                player->seek(cur);

            double now = (double)SDL_GetTicks() / 1000.0;
            if (!status_msg.empty() && now < status_until)
                ImGui::TextColored(ImVec4(0.6f,1.0f,0.6f,1.0f), "%s", status_msg.c_str());
        } else {
            ImGui::TextColored(ImVec4(1,0.6f,0.2f,1),
                TR("No file loaded. File -> Open MIDI... (Ctrl+O)"));
        }
        ImGui::End();

        int new_hl = -1;

        if (player) {
            auto& s = player->settings();

            // ===== Каналы =====
            ImGui::SetNextWindowPos (ImVec2(0, 130), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(420, 380), ImGuiCond_FirstUseEver);
            if (ImGui::Begin(TR("Channels"))) {
                bool use_gm = s.use_gm_mapping.load();
                if (ImGui::Checkbox("GM mapping", &use_gm)) s.use_gm_mapping.store(use_gm);
                bool use_env = s.use_envelopes.load();
                if (ImGui::Checkbox("Envelopes", &use_env)) s.use_envelopes.store(use_env);
                ImGui::Separator();
                int v;
                v=s.pulse1_vol.load();   if (ImGui::SliderInt("Pulse 1 vol",&v,0,15)) s.pulse1_vol.store(v);
                v=s.pulse2_vol.load();   if (ImGui::SliderInt("Pulse 2 vol",&v,0,15)) s.pulse2_vol.store(v);
                v=s.triangle_vol.load(); if (ImGui::SliderInt("Triangle vol",&v,0,15)) s.triangle_vol.store(v);
                v=s.noise_vol.load();    if (ImGui::SliderInt("Noise vol",&v,0,15)) s.noise_vol.store(v);
                v=s.dmc_vol.load();      if (ImGui::SliderInt("DMC vol",&v,0,15)) s.dmc_vol.store(v);
                ImGui::Separator();
                static const char* duties[] = {"12.5%","25%","50%","75%"};
                int d;
                d=s.pulse1_duty.load(); if (ImGui::Combo("Pulse 1 duty",&d,duties,4)) s.pulse1_duty.store(d);
                d=s.pulse2_duty.load(); if (ImGui::Combo("Pulse 2 duty",&d,duties,4)) s.pulse2_duty.store(d);
                ImGui::Separator();

                ImGui::TextDisabled(TR("Drum source (MIDI ch 9):"));
                bool udrm = s.use_dpcm_drums.load();
                if (ImGui::RadioButton(TR("DPCM (DMC samples)"), udrm)) {
                    s.use_dpcm_drums.store(true);
                    player->remap();
                }
                if (ImGui::RadioButton("Noise", !udrm)) {
                    s.use_dpcm_drums.store(false);
                    player->remap();
                }

                if (dmc_samples.empty())
                    ImGui::TextColored(ImVec4(1,0.6f,0.2f,1),
                        TR("No samples. Put .dmc files in 'DPCM Samples'"));
                else
                    {
                    char _buf[128];
                    snprintf(_buf, sizeof(_buf), "%s %zu",
                             TR("Loaded DMC:"), dmc_samples.size());
                    ImGui::TextDisabled("%s", _buf);
                }
                ImGui::Separator();
                if (ImGui::Button(TR("Re-apply"))) player->remap();
            }
            ImGui::End();

            // ===== Сейчас играет =====
            ImGui::SetNextWindowPos (ImVec2(0, 520), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(420, 200), ImGuiCond_FirstUseEver);
            if (ImGui::Begin(TR("Now Playing"))) {
                auto np = player->now_playing();
                auto bar = [&new_hl](const char* name, int ch, int pitch, ImU32 col){
                    ImGui::Text("%-10s", name);
                    bool hovered = ImGui::IsItemHovered();
                    ImGui::SameLine();
                    if (pitch < 0) { ImGui::TextDisabled("-"); }
                    else {
                        char buf[64];
                        snprintf(buf, sizeof(buf), TR("note %d (%.1f Hz)"),
                                 pitch, 440.0*pow(2.0,(pitch-69)/12.0));
                        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(col), "%s", buf);
                    }
                    if (hovered) new_hl = ch;
                };
                bar("Pulse A",  CH_PULSE, np.p1_pitch,  IM_COL32(120,220,120,255));
                bar("Pulse B",  CH_PULSE, np.p2_pitch,  IM_COL32(120,220,120,255));
                bar("Triangle", CH_TRI,   np.tri_pitch, IM_COL32(255,180,120,255));
                ImGui::Text("%-10s", "Noise");
                bool h = ImGui::IsItemHovered(); ImGui::SameLine();
                if (h) new_hl = CH_NOI;
                ImGui::TextColored(np.noise_on ? ImVec4(1,0.6f,0.6f,1) : ImVec4(0.5f,0.5f,0.5f,1),
                                   np.noise_on ? TR("active") : "-");
                ImGui::Text("%-10s", "DMC");
                ImGui::SameLine();
                if (np.dmc_slot >= 0 && np.dmc_slot < (int)dmc_samples.size())
                    ImGui::TextColored(ImVec4(0.9f,0.7f,1.0f,1), "%s",
                                       dmc_samples[np.dmc_slot].name.c_str());
                else
                    ImGui::TextDisabled("-");
            }
            ImGui::End();

            // ===== Piano Roll =====
            if (show_pianoroll) {
                ImGui::SetNextWindowPos (ImVec2(430, 130), ImGuiCond_FirstUseEver);
                ImGui::SetNextWindowSize(ImVec2(840, 400), ImGuiCond_FirstUseEver);
                if (ImGui::Begin("Piano Roll", &show_pianoroll)) {
                    ImVec2 avail = ImGui::GetContentRegionAvail();
                    if (avail.x < 100) avail.x = 100;
                    if (avail.y < 100) avail.y = 100;
                    draw_piano_roll(*player, avail, highlight_ch);
                    ImGui::TextDisabled(TR("LMB = seek | Wheel = zoom | MMB-drag = pan"));
                }
                ImGui::End();
            }

            // ===== Редактор инструментов =====
            if (show_editor) {
                ImGui::SetNextWindowSize(ImVec2(1150, 720), ImGuiCond_FirstUseEver);
                if (ImGui::Begin(TR("Instrument Editor"), &show_editor)) {
                    if (ImGui::Button(TR("Reset All"))) player->settings().reset_all();
                    ImGui::SameLine();
                    if (ImGui::Button(TR("Save Preset..."))) {
                        std::wstring p;
                        if (dlg_save(win, p,
                            L"NES Preset (*.nespreset)\0*.nespreset\0All Files (*.*)\0*.*\0",
                            L"Сохранить пресет", L"nespreset"))
                            save_preset(p, player->settings());
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(TR("Load Preset..."))) {
                        std::wstring p;
                        if (dlg_open(win, p,
                            L"NES Preset (*.nespreset)\0*.nespreset\0All Files (*.*)\0*.*\0",
                            L"Загрузить пресет"))
                            if (load_preset(p, player->settings())) player->remap();
                    }
                    ImGui::SameLine();
                    static char filter[64] = "";
                    ImGui::SetNextItemWidth(200);
                    ImGui::InputTextWithHint("##filter", TR("filter..."), filter, sizeof(filter));

                    static const char* chans[]  = {TR("Pulse"), TR("Triangle"), TR("Noise")};
                    static const char* duties[] = {"12.5%","25%","50%","75%"};
                    static const char* sweeps[] = {TR("off"),TR("up-down"),TR("cycle"),TR("alt")};
                    static const char* noises[] = {TR("long LFSR"), TR("short LFSR")};

                    bool need_remap = false;

                    // ====== DMC — Drum Slots ======
                    if (ImGui::CollapsingHeader(TR("DMC - Drum Slots (DPCM)"),
                                                ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        if (dmc_samples.empty()) {
                            ImGui::TextColored(ImVec4(1,0.6f,0.2f,1),
                                TR("No .dmc samples in 'DPCM Samples'. Put files and File -> Refresh DPCM Samples."));
                        } else {
                            // Имена для combo
                            std::vector<const char*> cnames;
                            cnames.reserve(dmc_samples.size() + 1);
                            cnames.push_back("(none)");
                            for (auto& smp : dmc_samples) cnames.push_back(smp.name.c_str());

                            if (ImGui::BeginTable("drum_table", 6,
                                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                ImGuiTableFlags_SizingFixedFit))
                            {
                                ImGui::TableSetupColumn("Слот",  ImGuiTableColumnFlags_WidthFixed, 100);
                                ImGui::TableSetupColumn("Sample",ImGuiTableColumnFlags_WidthFixed, 220);
                                ImGui::TableSetupColumn("Rate",  ImGuiTableColumnFlags_WidthFixed, 180);
                                ImGui::TableSetupColumn("Vol",   ImGuiTableColumnFlags_WidthFixed, 160);
                                ImGui::TableSetupColumn("Loop",  ImGuiTableColumnFlags_WidthFixed, 60);
                                ImGui::TableSetupColumn("Test",  ImGuiTableColumnFlags_WidthFixed, 70);
                                ImGui::TableHeadersRow();

                                for (int i = 0; i < DRUM_COUNT; ++i) {
                                    ImGui::TableNextRow();
                                    ImGui::PushID(1000 + i);

                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::AlignTextToFramePadding();
                                    ImGui::TextUnformatted(tr_drum_slot(i));

                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::SetNextItemWidth(-1);
                                    int cur = s.drum_sample[i].load() + 1;
                                    if (cur < 0) cur = 0;
                                    if (cur >= (int)cnames.size()) cur = 0;
                                    if (ImGui::Combo("##smp", &cur, cnames.data(), (int)cnames.size())) {
                                        s.drum_sample[i].store(cur - 1);
                                        need_remap = true;
                                    }

                                    ImGui::TableSetColumnIndex(2);
                                    ImGui::SetNextItemWidth(-1);
                                    int rate = s.drum_rate[i].load();
                                    if (ImGui::SliderInt("##rt", &rate, 0, 15)) s.drum_rate[i].store(rate);
                                    if (ImGui::IsItemDeactivatedAfterEdit()) need_remap = true;

                                    ImGui::TableSetColumnIndex(3);
                                    ImGui::SetNextItemWidth(-1);
                                    int vol = s.drum_volume[i].load();
                                    if (ImGui::SliderInt("##vl", &vol, 0, 15)) s.drum_volume[i].store(vol);
                                    if (ImGui::IsItemDeactivatedAfterEdit()) need_remap = true;

                                    ImGui::TableSetColumnIndex(4);
                                    bool lp = s.drum_loop[i].load();
                                    if (ImGui::Checkbox("##lp", &lp)) s.drum_loop[i].store(lp);

                                    ImGui::TableSetColumnIndex(5);
                                    if (ImGui::SmallButton("Test")) {
                                        int smp = s.drum_sample[i].load();
                                        if (smp >= 0 && smp < (int)dmc_samples.size()) {
                                            play_dmc_preview(dmc_samples[smp],
                                                             s.drum_rate[i].load(),
                                                             s.drum_volume[i].load());
                                        }
                                    }
                                    ImGui::PopID();
                                }
                                ImGui::EndTable();
                            }
                            ImGui::TextDisabled(TR("Rate: 0 = 428 Hz (high), 15 = 54 Hz (low)"));
                        }
                        ImGui::Separator();
                    }

                    // ====== Барабаны — Noise fallback ======
                    if (ImGui::CollapsingHeader(TR("Drums - Noise fallback"),
                                                ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        int nm = s.drum_noise_mode.load();
                        ImGui::SetNextItemWidth(140);
                        if (ImGui::Combo("LFSR mode##drum", &nm, noises, 2))
                            s.drum_noise_mode.store(nm);
                        ImGui::SameLine();
                        float mult = s.drum_noise_rate_mult.load();
                        ImGui::SetNextItemWidth(150);
                        if (ImGui::SliderFloat("Rate x##drum", &mult, 0.5f, 32.0f, "%.1f"))
                            s.drum_noise_rate_mult.store(mult);
                        ImGui::SameLine();
                        float fh = s.drum_noise_fixed_hz.load();
                        ImGui::SetNextItemWidth(150);
                        if (ImGui::SliderFloat("Fixed Hz##drum", &fh, 0.0f, 8000.0f, "%.0f"))
                            s.drum_noise_fixed_hz.store(fh);

                        float A = s.drum_attack .load();
                        float D = s.drum_decay  .load();
                        float S = s.drum_sustain.load();
                        float R = s.drum_release.load();
                        ImGui::SetNextItemWidth(120);
                        if (ImGui::SliderFloat("A##drum", &A, 0, 1, "%.2f")) s.drum_attack.store(A);
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(120);
                        if (ImGui::SliderFloat("D##drum", &D, 0, 1, "%.2f")) s.drum_decay.store(D);
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(120);
                        if (ImGui::SliderFloat("S##drum", &S, 0, 1, "%.2f")) s.drum_sustain.store(S);
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(120);
                        if (ImGui::SliderFloat("R##drum", &R, 0, 2, "%.2f")) s.drum_release.store(R);
                        ImGui::Separator();
                    }

                    // ====== Таблица 128 GM инструментов ======
                    if (ImGui::BeginTable("inst_table", 8,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit,
                        ImVec2(0, 480)))
                    {
                        ImGui::TableSetupScrollFreeze(0, 1);
                        ImGui::TableSetupColumn("#",       ImGuiTableColumnFlags_WidthFixed, 30);
                        ImGui::TableSetupColumn("Имя",     ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("Канал",   ImGuiTableColumnFlags_WidthFixed, 90);
                        ImGui::TableSetupColumn("Duty",    ImGuiTableColumnFlags_WidthFixed, 90);
                        ImGui::TableSetupColumn("A D S R", ImGuiTableColumnFlags_WidthFixed, 340);
                        ImGui::TableSetupColumn("Sweep",   ImGuiTableColumnFlags_WidthFixed, 170);
                        ImGui::TableSetupColumn("Noise",   ImGuiTableColumnFlags_WidthFixed, 200);
                        ImGui::TableSetupColumn("",        ImGuiTableColumnFlags_WidthFixed, 50);
                        ImGui::TableHeadersRow();

                        ImGuiListClipper clip;
                        clip.Begin(128);
                        while (clip.Step()) {
                            for (int i = clip.DisplayStart; i < clip.DisplayEnd; ++i) {
                                const char* name = GM_NAMES[i];
                                if (filter[0]) {
                                    std::string n = name, f = filter;
                                    for (auto& c : n) c = (char)tolower((unsigned char)c);
                                    for (auto& c : f) c = (char)tolower((unsigned char)c);
                                    if (n.find(f) == std::string::npos) continue;
                                }
                                ImGui::TableNextRow();
                                ImGui::PushID(i);

                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("%3d", i);

                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextUnformatted(name);
                                bool hover_name = ImGui::IsItemHovered();

                                auto& I = player->settings().inst[i];
                                int cur_ch = I.channel.load();

                                ImGui::TableSetColumnIndex(2);
                                ImGui::SetNextItemWidth(-1);
                                int c = cur_ch;
                                if (ImGui::Combo("##c", &c, chans, 3)) { I.channel.store(c); need_remap = true; }

                                ImGui::TableSetColumnIndex(3);
                                if (cur_ch == CH_PULSE) {
                                    ImGui::SetNextItemWidth(-1);
                                    int d = I.duty.load();
                                    if (ImGui::Combo("##d", &d, duties, 4)) { I.duty.store(d); need_remap = true; }
                                } else {
                                    ImGui::TextDisabled("—");
                                }

                                ImGui::TableSetColumnIndex(4);
                                if (draw_instrument_row(I)) need_remap = true;

                                ImGui::TableSetColumnIndex(5);
                                if (cur_ch == CH_PULSE) {
                                    int sm = I.sweep_mode.load();
                                    float sr = I.sweep_rate.load();
                                    ImGui::SetNextItemWidth(80);
                                    if (ImGui::Combo("##sm", &sm, sweeps, 4)) { I.sweep_mode.store(sm); need_remap = true; }
                                    ImGui::SameLine();
                                    ImGui::SetNextItemWidth(80);
                                    if (ImGui::SliderFloat("##sr", &sr, 0.0f, 20.0f, "%.1fHz"))
                                        I.sweep_rate.store(sr);
                                    if (ImGui::IsItemDeactivatedAfterEdit()) need_remap = true;
                                } else {
                                    ImGui::TextDisabled("—");
                                }

                                ImGui::TableSetColumnIndex(6);
                                if (cur_ch == CH_NOI) {
                                    int nm = I.noise_mode.load();
                                    ImGui::SetNextItemWidth(90);
                                    if (ImGui::Combo("##nm", &nm, noises, 2)) { I.noise_mode.store(nm); need_remap = true; }
                                    ImGui::SameLine();
                                    float nrm = I.noise_rate_mult.load();
                                    ImGui::SetNextItemWidth(80);
                                    if (ImGui::SliderFloat("##nrm", &nrm, 0.5f, 32.0f, "x%.1f"))
                                        I.noise_rate_mult.store(nrm);
                                    if (ImGui::IsItemDeactivatedAfterEdit()) need_remap = true;
                                } else {
                                    ImGui::TextDisabled("—");
                                }

                                ImGui::TableSetColumnIndex(7);
                                if (ImGui::SmallButton("Env")) g_env_edit_idx = i;
                                ImGui::SameLine();
                                if (ImGui::SmallButton("Def")) {
                                    player->settings().reset_one(i);
                                    need_remap = true;
                                }
                                ImGui::PopID();

                                if (hover_name) {
                                    ImGui::BeginTooltip();
                                    ImGui::Text("%s", name);
                                    ImGui::Separator();
                                    float A = I.attack.load();
                                    float D = I.decay.load();
                                    float S = I.sustain.load();
                                    float R = I.release.load();
                                    draw_adsr_graph(A, D, S, R, ImVec2(260, 90));
                                    const char* chn = (cur_ch == CH_PULSE) ? "Pulse" :
                                                      (cur_ch == CH_TRI)   ? "Triangle" : "Noise";
                                    ImGui::Text("Канал: %s", chn);
                                    ImGui::Text("A=%.2f D=%.2f S=%.2f R=%.2f", A,D,S,R);
                                    ImGui::EndTooltip();
                                }
                            }
                        }
                        ImGui::EndTable();
                    }
                    if (need_remap) player->remap();

                    // ===== Envelope Editor (popup) =====
                    if (g_env_edit_idx >= 0 && !ImGui::IsPopupOpen("Envelope Editor"))
                        ImGui::OpenPopup("Envelope Editor");
                    if (g_env_edit_idx >= 0 &&
                        ImGui::BeginPopupModal("Envelope Editor", nullptr,
                        ImGuiWindowFlags_AlwaysAutoResize))
                    {
                        auto& I = player->settings().inst[g_env_edit_idx];
                        ImGui::Text("Инструмент %d: %s", g_env_edit_idx,
                                    GM_NAMES[g_env_edit_idx]);
                        ImGui::Separator();

                        envelope_editor_ui("Volume", I.vol_env,   0, 15, 15,
                                           IM_COL32(120,220,120,255), 1);
                        envelope_editor_ui("Pitch",  I.pitch_env,-24, 24,  0,
                                           IM_COL32(120,180,255,255), 2);
                        envelope_editor_ui("Duty",   I.duty_env,   0,  3,  0,
                                           IM_COL32(255,180,120,255), 3);
                        envelope_editor_ui("Arp",    I.arp_env,  -12, 12,  0,
                                           IM_COL32(255,140,200,255), 4);

                        ImGui::Separator();
                        if (ImGui::Button(TR("Close"), ImVec2(120, 30))) {
                            ImGui::CloseCurrentPopup();
                            g_env_edit_idx = -1;
                        }
                        ImGui::EndPopup();
                    }
                }
                ImGui::End();
            }
        }

        highlight_ch = new_hl;

        ImGui::Render();
        glViewport(0, 0, win_w, win_h);
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(win);
    }

    if (g_export_thread.joinable()) g_export_thread.join();
    SDL_LockAudioDevice(dev);
    g_player = nullptr;
    delete player;
    SDL_UnlockAudioDevice(dev);
    if (g_preview_dev) SDL_CloseAudioDevice(g_preview_dev);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_CloseAudioDevice(dev);
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
