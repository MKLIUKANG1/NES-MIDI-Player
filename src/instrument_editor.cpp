#include "instrument_editor.h"
#include "env_editor.h"
#include "gm_names.h"
#include <cstring>
#include <cctype>

// Утилиты
static uint32_t get_color(const Instrument& I, int idx) {
    uint32_t c = I.color.load();
    if (c == 0) return instrument_default_color(idx);
    return c;
}

static const char* ch_letter(int ch) {
    switch (ch) {
        case CH_PULSE: return "P";
        case CH_TRI:   return "T";
        case CH_NOI:   return "N";
    }
    return "?";
}

static ImU32 ch_color(int ch) {
    switch (ch) {
        case CH_PULSE: return IM_COL32(120,230,120,255);
        case CH_TRI:   return IM_COL32(255,180,120,255);
        case CH_NOI:   return IM_COL32(255,120,180,255);
    }
    return IM_COL32(200,200,200,255);
}

// ===== Главная функция =====
bool draw_instrument_editor(
    ChipSettings& s, ApuPlayer* /*player*/, int& selected_idx,
    const std::vector<DmcSample>* dmc_samples)
{
    bool need_remap = false;

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float left_w = 320.0f;
    if (avail.x < 700) left_w = avail.x * 0.42f;
    float right_w = avail.x - left_w - 8;

    // ============= ЛЕВАЯ ПАНЕЛЬ: Список =============
    ImGui::BeginChild("##InstList", ImVec2(left_w, 0), ImGuiChildFlags_Borders);

    static char filter_buf[64] = "";
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##filter", "Filter...", filter_buf, sizeof(filter_buf));

    if (ImGui::Button("Reset One", ImVec2(105, 26)) && selected_idx >= 0) {
        s.reset_one(selected_idx);
        need_remap = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset All", ImVec2(105, 26))) {
        s.reset_all();
        need_remap = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%d", selected_idx);

    ImGui::Separator();

    ImGui::BeginChild("##ListScroll", ImVec2(0, 0), ImGuiChildFlags_None);

    std::string f_lower = filter_buf;
    for (auto& c : f_lower) c = (char)tolower((unsigned char)c);

    for (int i = 0; i < 128; ++i) {
        const char* name = GM_NAMES[i];

        if (!f_lower.empty()) {
            std::string n_lower = name;
            for (auto& c : n_lower) c = (char)tolower((unsigned char)c);
            if (n_lower.find(f_lower) == std::string::npos) continue;
        }

        auto& I = s.inst[i];
        int ch = I.channel.load();
        uint32_t col = get_color(I, i);

        ImGui::PushID(i);
        bool is_sel = (selected_idx == i);

        // Full-row InvisibleButton -> используем Selectable с ##ID
        float avail_w = ImGui::GetContentRegionAvail().x;
        if (ImGui::Selectable("##row", is_sel, 0, ImVec2(avail_w, 22))) {
            selected_idx = i;
        }

        // Рисуем поверх
        ImVec2 rmin = ImGui::GetItemRectMin();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // цветной квадрат
        dl->AddRectFilled(ImVec2(rmin.x + 2,  rmin.y + 3), ImVec2(rmin.x + 18, rmin.y + 19), col, 2.0f);
        dl->AddRect(ImVec2(rmin.x + 2,  rmin.y + 3), ImVec2(rmin.x + 18, rmin.y + 19), IM_COL32(0,0,0,180), 2.0f);

        // буква канала
        dl->AddText(ImVec2(rmin.x + 24, rmin.y + 3), ch_color(ch), ch_letter(ch));

        // номер + название
        char label[128];
        snprintf(label, sizeof(label), "%d  %s", i, name);
        dl->AddText(ImVec2(rmin.x + 42, rmin.y + 3), IM_COL32(230,230,230,255), label);

        ImGui::PopID();
    }

    ImGui::EndChild();
    ImGui::EndChild();

    // ============= ПРАВАЯ ПАНЕЛЬ: Редактор =============
    ImGui::SameLine();
    ImGui::BeginChild("##InstEdit", ImVec2(right_w, 0), ImGuiChildFlags_Borders);

    if (selected_idx < 0 || selected_idx >= 128) {
        ImGui::TextColored(ImVec4(0.7f,0.7f,0.7f,1), "Выберите инструмент слева");
        ImGui::EndChild();
        return need_remap;
    }

    auto& I = s.inst[selected_idx];
    uint32_t cur_color = get_color(I, selected_idx);
    ImVec4 colv = ImGui::ColorConvertU32ToFloat4(cur_color);

    // Заголовок
    ImGui::PushStyleColor(ImGuiCol_Text, colv);
    ImGui::Text("%d — %s", selected_idx, GM_NAMES[selected_idx]);
    ImGui::PopStyleColor();
    ImGui::Separator();

    // Цвет
    float col_edit[4] = { colv.x, colv.y, colv.z, colv.w };
    ImGui::SetNextItemWidth(60);
    if (ImGui::ColorEdit4("##color", col_edit,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar |
        ImGuiColorEditFlags_NoLabel))
    {
        uint32_t packed = ImGui::ColorConvertFloat4ToU32(
            ImVec4(col_edit[0], col_edit[1], col_edit[2], col_edit[3]));
        I.color.store(packed);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Auto")) I.color.store(0);
    ImGui::SameLine();

    // Канал + duty
    int cur_ch = I.channel.load();
    const char* ch_items[] = { "Pulse", "Triangle", "Noise" };
    ImGui::SetNextItemWidth(120);
    if (ImGui::Combo("##ch", &cur_ch, ch_items, 3)) {
        I.channel.store(cur_ch);
        need_remap = true;
    }
    if (cur_ch == CH_PULSE) {
        ImGui::SameLine();
        static const char* du[] = { "12.5%", "25%", "50%", "75%" };
        int d = I.duty.load();
        ImGui::SetNextItemWidth(100);
        if (ImGui::Combo("##du", &d, du, 4)) { I.duty.store(d); need_remap = true; }
    }

    // Noise-специфика
    if (cur_ch == CH_NOI) {
        ImGui::Separator();
        static const char* lf[] = { "Long LFSR", "Short LFSR" };
        int nm = I.noise_mode.load();
        ImGui::SetNextItemWidth(160);
        if (ImGui::Combo("LFSR", &nm, lf, 2)) { I.noise_mode.store(nm); need_remap = true; }
        ImGui::SameLine();
        static const char* rr[16] = {
            "0:447k","1:224k","2:112k","3:55.9k","4:28k","5:18.6k","6:14k","7:11.2k",
            "8:8.86k","9:7.05k","10:4.71k","11:3.52k","12:2.35k","13:1.76k","14:880","15:440"
        };
        int ri = I.noise_rate_index.load();
        if (ri < 0) ri = 0;
        if (ri > 15) ri = 15;
        ImGui::SetNextItemWidth(140);
        if (ImGui::Combo("Rate", &ri, rr, 16)) { I.noise_rate_index.store(ri); need_remap = true; }
    }

    // ADSR (только для melodic)
    if (cur_ch != CH_NOI) {
        ImGui::Separator();
        float A = I.attack.load(), D = I.decay.load();
        float S = I.sustain.load(), R = I.release.load();
        ImGui::SetNextItemWidth(80);
        if (ImGui::SliderFloat("A", &A, 0, 1, "%.3f")) I.attack.store(A);
        ImGui::SameLine(); ImGui::SetNextItemWidth(80);
        if (ImGui::SliderFloat("D", &D, 0, 1, "%.3f")) I.decay.store(D);
        ImGui::SameLine(); ImGui::SetNextItemWidth(80);
        if (ImGui::SliderFloat("S", &S, 0, 1, "%.3f")) I.sustain.store(S);
        ImGui::SameLine(); ImGui::SetNextItemWidth(80);
        if (ImGui::SliderFloat("R", &R, 0, 2, "%.3f")) I.release.store(R);
        if (ImGui::IsItemDeactivatedAfterEdit()) need_remap = true;
    }

    // Envelope curves
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f,0.9f,0.9f,1), "Envelope curves:");

    if (envelope_editor_ui("Volume", I.vol_env, 0, 15, 15,
                           IM_COL32(120,220,120,255), 1))
        need_remap = true;

    if (cur_ch == CH_NOI) {
        if (envelope_editor_ui("Rate (-15..+15)", I.rate_env, -15, 15, 0,
                               IM_COL32(180,140,255,255), 5))
            need_remap = true;
        ImGui::TextDisabled("Duty/Pitch/Arp не используются для Noise");
    } else {
        if (cur_ch == CH_PULSE) {
            if (envelope_editor_ui("Duty", I.duty_env, 0, 3, 0,
                                   IM_COL32(255,180,120,255), 3))
                need_remap = true;
        }
        if (envelope_editor_ui("Pitch", I.pitch_env, -24, 24, 0,
                               IM_COL32(120,180,255,255), 2))
            need_remap = true;
        if (envelope_editor_ui("Arp", I.arp_env, -12, 12, 0,
                               IM_COL32(255,140,200,255), 4))
            need_remap = true;
    }

    // ============= DRUM SLOTS (DPCM + Noise) =============
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Drum Slots", ImGuiTreeNodeFlags_DefaultOpen)) {

        // ===== ГЛОБАЛЬНЫЙ ИНДИКАТОР: DPCM или Noise =====
        bool ud = s.use_dpcm_drums.load();
        ImVec4 bg  = ud ? ImVec4(0.55f,0.25f,0.20f,1) : ImVec4(0.20f,0.45f,0.25f,1);
        ImVec4 txt = ud ? ImVec4(1.0f,0.75f,0.60f,1) : ImVec4(0.70f,1.0f,0.70f,1);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg);
        ImGui::BeginChild("##drum_src_banner", ImVec2(0, 62), ImGuiChildFlags_Borders);
        if (ud) {
            ImGui::TextColored(txt, "⚠  Барабаны сейчас: DPCM");
            ImGui::TextWrapped("Noise-настройки НЕ ПРИМЕНЯЮТСЯ. Ноты уходят в DMC-сэмплы.");
        } else {
            ImGui::TextColored(txt, "✓  Барабаны сейчас: Noise");
            ImGui::TextWrapped("Noise-настройки АКТИВНЫ. DMC-сэмплы игнорируются.");
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Button(ud ? "→ Переключить на Noise" : "→ Переключить на DPCM",
                          ImVec2(230, 50)))
        {
            s.use_dpcm_drums.store(!ud);
            need_remap = true;
        }
        ImGui::EndChild();
        ImGui::Separator();

        if (ImGui::BeginTabBar("##drum_tabs")) {

            // ===== TAB 1: DPCM =====
            if (ImGui::BeginTabItem("DPCM Samples")) {
                if (!dmc_samples || dmc_samples->empty()) {
                    ImGui::TextColored(ImVec4(1,0.6f,0.2f,1),
                        "Нет .dmc файлов в папке 'DPCM Samples'");
                } else {
                    std::vector<const char*> cnames;
                    cnames.push_back("(none)");
                    for (auto& smp : *dmc_samples) cnames.push_back(smp.name.c_str());

                    if (ImGui::BeginTable("drum_dpcm", 5,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                        ImGuiTableFlags_SizingFixedFit))
                    {
                        ImGui::TableSetupColumn("Slot",   ImGuiTableColumnFlags_WidthFixed, 100);
                        ImGui::TableSetupColumn("Sample", ImGuiTableColumnFlags_WidthFixed, 200);
                        ImGui::TableSetupColumn("Rate",   ImGuiTableColumnFlags_WidthFixed, 160);
                        ImGui::TableSetupColumn("Vol",    ImGuiTableColumnFlags_WidthFixed, 150);
                        ImGui::TableSetupColumn("Loop",   ImGuiTableColumnFlags_WidthFixed, 60);
                        ImGui::TableHeadersRow();
                        for (int i = 0; i < DRUM_COUNT; ++i) {
                            ImGui::TableNextRow();
                            ImGui::PushID(1000 + i);
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted(DRUM_SLOT_NAMES[i]);
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
                            int rt = s.drum_rate[i].load();
                            if (ImGui::SliderInt("##rt", &rt, 0, 15)) s.drum_rate[i].store(rt);
                            if (ImGui::IsItemDeactivatedAfterEdit()) need_remap = true;
                            ImGui::TableSetColumnIndex(3);
                            ImGui::SetNextItemWidth(-1);
                            int vl = s.drum_volume[i].load();
                            if (ImGui::SliderInt("##vl", &vl, 0, 15)) s.drum_volume[i].store(vl);
                            if (ImGui::IsItemDeactivatedAfterEdit()) need_remap = true;
                            ImGui::TableSetColumnIndex(4);
                            bool lp = s.drum_loop[i].load();
                            if (ImGui::Checkbox("##lp", &lp)) s.drum_loop[i].store(lp);
                            ImGui::PopID();
                        }
                        ImGui::EndTable();
                    }
                }
                ImGui::EndTabItem();
            }

            // ===== TAB 2: Noise per-slot с графическим редактором =====
            if (ImGui::BeginTabItem("Noise (per slot)")) {
                static int sel_drum = 0;   // какой слот редактируем

                // ==== Кнопки выбора слота ====
                ImGui::Text("Выберите слот:");
                for (int i = 0; i < DRUM_COUNT; ++i) {
                    if (i > 0) ImGui::SameLine();
                    bool sel = (sel_drum == i);
                    if (sel) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f,0.5f,0.2f,1));
                    char btn[64];
                    snprintf(btn, sizeof(btn), "%s##sl%d", DRUM_SLOT_NAMES[i], i);
                    if (ImGui::Button(btn, ImVec2(90, 28))) sel_drum = i;
                    if (sel) ImGui::PopStyleColor();
                }

                ImGui::Separator();

                int slot = sel_drum;
                auto& I_mode = s.drum_noise_mode_slot[slot];
                auto& I_rate = s.drum_noise_rate_index_slot[slot];
                auto& I_pitch = s.drum_noise_pitch_offset_slot[slot];
                auto& I_atk = s.drum_noise_attack_slot[slot];
                auto& I_dec = s.drum_noise_decay_slot[slot];
                auto& I_sus = s.drum_noise_sustain_slot[slot];
                auto& I_rel = s.drum_noise_release_slot[slot];

                // ==== Верхняя строка: LFSR, Rate, Pitch, Test ====
                static const char* lf[] = { "Long", "Short" };
                int m = I_mode.load();
                ImGui::SetNextItemWidth(110);
                if (ImGui::Combo("LFSR", &m, lf, 2)) { I_mode.store(m); need_remap = true; }

                ImGui::SameLine();
                static const char* rn[16] = {
                    "0:447k","1:224k","2:112k","3:55.9k","4:28k","5:18.6k","6:14k","7:11.2k",
                    "8:8.86k","9:7.05k","10:4.71k","11:3.52k","12:2.35k","13:1.76k","14:880","15:440"
                };
                int ri = I_rate.load();
                if (ri < 0) ri = 0;
                if (ri > 15) ri = 15;
                ImGui::SetNextItemWidth(130);
                if (ImGui::Combo("Base Rate", &ri, rn, 16)) { I_rate.store(ri); need_remap = true; }

                ImGui::SameLine();
                ImGui::SetNextItemWidth(130);
                float po = I_pitch.load();
                if (ImGui::SliderFloat("Pitch", &po, -24.0f, 24.0f, "%.0f st")) I_pitch.store(po);
                if (ImGui::IsItemDeactivatedAfterEdit()) need_remap = true;

                ImGui::SameLine();
                if (ImGui::Button("🔊 Test", ImVec2(90, 26))) {
                    printf("[UI] Test button clicked, slot=%d\n", slot);
                    play_drum_preview(s, slot);
                }

                // ==== ADSR-слайдеры (компактно) ====
                ImGui::Separator();
                float A = I_atk.load(), D = I_dec.load(), S = I_sus.load(), R = I_rel.load();
                ImGui::SetNextItemWidth(100);
                if (ImGui::SliderFloat("A", &A, 0, 1, "%.3f")) I_atk.store(A);
                if (ImGui::IsItemDeactivatedAfterEdit()) need_remap = true;
                ImGui::SameLine(); ImGui::SetNextItemWidth(100);
                if (ImGui::SliderFloat("D", &D, 0, 1, "%.3f")) I_dec.store(D);
                if (ImGui::IsItemDeactivatedAfterEdit()) need_remap = true;
                ImGui::SameLine(); ImGui::SetNextItemWidth(100);
                if (ImGui::SliderFloat("S", &S, 0, 1, "%.3f")) I_sus.store(S);
                if (ImGui::IsItemDeactivatedAfterEdit()) need_remap = true;
                ImGui::SameLine(); ImGui::SetNextItemWidth(100);
                if (ImGui::SliderFloat("R", &R, 0, 2, "%.3f")) I_rel.store(R);
                if (ImGui::IsItemDeactivatedAfterEdit()) need_remap = true;
                ImGui::SameLine();
                if (ImGui::SmallButton("Apply")) need_remap = true;

                // ==== Графические огибающие ====
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.6f,0.9f,0.9f,1),
                    "Графические огибающие для %s:", DRUM_SLOT_NAMES[slot]);

                if (envelope_editor_ui("Volume env", s.drum_vol_env[slot], 0, 15, 15,
                                       IM_COL32(120,220,120,255), 800 + slot))
                    need_remap = true;

                if (envelope_editor_ui("Rate env (-15..+15)", s.drum_rate_env[slot], -15, 15, 0,
                                       IM_COL32(180,140,255,255), 900 + slot))
                    need_remap = true;

                ImGui::TextDisabled("Если огибающие выключены — применяются ADSR-слайдеры выше");

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    ImGui::EndChild();
    return need_remap;
}
