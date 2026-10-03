#pragma once
#include "apu.h"
#include "imgui.h"

// Графический редактор одной кривой. ЛКМ рисует, ПКМ стирает.
// Возвращает true, если что-то изменилось.
inline bool envelope_editor_ui(const char* label, EnvelopeCurve& env,
                               int min_val, int max_val, int default_val,
                               ImU32 color, int id_suffix)
{
    bool changed = false;
    ImGui::PushID(id_suffix);

    // ---- Верхняя панель ----
    bool en = env.enabled.load();
    if (ImGui::Checkbox("##en", &en)) { env.enabled.store(en); changed = true; }
    ImGui::SameLine();
    ImGui::Text("%-8s", label);
    ImGui::SameLine();

    int len = env.length.load();
    ImGui::SetNextItemWidth(120);
    if (ImGui::SliderInt("len", &len, 1, EnvelopeCurve::MAX_STEPS, "%d"))
    { env.length.store(len); changed = true; }

    ImGui::SameLine();
    int sp = env.speed.load();
    ImGui::SetNextItemWidth(120);
    if (ImGui::SliderInt("spd", &sp, 1, 30, "%d"))
    { env.speed.store(sp); changed = true; }

    ImGui::SameLine();
    int ls = env.loop_start.load();
    ImGui::SetNextItemWidth(70);
    if (ImGui::SliderInt("L", &ls, -1, EnvelopeCurve::MAX_STEPS - 1, "%d"))
    { env.loop_start.store(ls); changed = true; }

    ImGui::SameLine();
    int le = env.loop_end.load();
    ImGui::SetNextItemWidth(70);
    if (ImGui::SliderInt("R", &le, -1, EnvelopeCurve::MAX_STEPS - 1, "%d"))
    { env.loop_end.store(le); changed = true; }

    ImGui::SameLine();
    if (ImGui::SmallButton("clear")) { env.fill(default_val); changed = true; }

    // ---- Полотно ----
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float W = avail.x; if (W < 200) W = 200;
    ImVec2 size = ImVec2(W, 76);
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(p0, p1, IM_COL32(20,20,28,255), 3);
    dl->AddRect(p0, p1, IM_COL32(70,70,90,255), 3);

    int maxs = EnvelopeCurve::MAX_STEPS;
    float step_w = size.x / (float)maxs;
    int range = max_val - min_val + 1;
    float val_h = (size.y - 6.0f) / (float)range;

    // вертикальные линии делений (каждые 4 шага)
    int len_cur = env.length.load();
    if (len_cur < 1) len_cur = 1;
    if (len_cur > maxs) len_cur = maxs;
    for (int i = 0; i <= maxs; i += 4) {
        float x = p0.x + i * step_w;
        dl->AddLine(ImVec2(x, p0.y + 3), ImVec2(x, p1.y - 3), IM_COL32(40,40,55,150));
    }
    // Маркер длины
    {
        float x = p0.x + len_cur * step_w;
        dl->AddLine(ImVec2(x, p0.y + 3), ImVec2(x, p1.y - 3),
                    IM_COL32(255,180,80,180), 1.6f);
    }
    // Loop region
    int ls_v = env.loop_start.load(), le_v = env.loop_end.load();
    if (ls_v >= 0 && le_v >= ls_v) {
        float x0 = p0.x + ls_v * step_w;
        float x1 = p0.x + (le_v + 1) * step_w;
        if (x0 < p0.x) x0 = p0.x;
        if (x1 > p1.x) x1 = p1.x;
        dl->AddRectFilled(ImVec2(x0, p0.y + 3), ImVec2(x1, p1.y - 3),
                          IM_COL32(100,140,200,55));
    }

    // Осевая линия (нулевое значение)
    if (min_val <= 0 && max_val >= 0) {
        float y = p1.y - 3 - ((0 - min_val) + 0.5f) * val_h;
        dl->AddLine(ImVec2(p0.x, y), ImVec2(p1.x, y), IM_COL32(60,60,80,180));
    }

    // Значения — ступеньки
    for (int i = 0; i < len_cur; ++i) {
        int v = env.values[i].load();
        if (v < min_val) v = min_val;
        if (v > max_val) v = max_val;
        float x0 = p0.x + i * step_w;
        float x1 = p0.x + (i + 1) * step_w;
        float yc = p1.y - 3 - ((v - min_val) + 0.5f) * val_h;
        float bar_h = val_h * 0.8f;
        if (bar_h < 2.0f) bar_h = 2.0f;
        dl->AddRectFilled(ImVec2(x0 + 0.5f, yc - bar_h * 0.5f),
                          ImVec2(x1 - 0.5f, yc + bar_h * 0.5f), color);
    }

    // ---- Взаимодействие ----
    ImGui::InvisibleButton("##canvas", size);
    bool hovered = ImGui::IsItemHovered();
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mp = io.MousePos;

    if (hovered && (ImGui::IsMouseDown(0) || ImGui::IsMouseDown(1))) {
        int col = (int)((mp.x - p0.x) / step_w);
        if (col >= 0 && col < maxs) {
            if (ImGui::IsMouseDown(1)) {
                if (env.values[col].load() != default_val) {
                    env.values[col].store(default_val);
                    changed = true;
                }
            } else if (ImGui::IsMouseDown(0)) {
                int v = min_val + (int)((p1.y - 3 - mp.y) / val_h);
                if (v < min_val) v = min_val;
                if (v > max_val) v = max_val;
                if (env.values[col].load() != v) {
                    env.values[col].store(v);
                    changed = true;
                }
            }
        }
    }
    ImGui::TextDisabled("ЛКМ — рисовать, ПКМ — стереть");

    ImGui::PopID();
    return changed;
}
