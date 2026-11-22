#pragma once
#include "protocol.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include <algorithm>
#include <cstdio>

namespace transit
{
    // Reserve scrollbar space so incoming values never change the panel height.
    inline void begin_status_row()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5 * ui_scale, 0));
        ImGui::BeginChild("Telemetry status", ImVec2(0, ImGui::GetFrameHeight() + ImGui::GetStyle().ScrollbarSize),
            false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::AlignTextToFramePadding();
    }

    inline void end_status_row()
    {
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
    }

    inline void lamp(const char *label, bool on, const char *tip, bool warning = false)
    {
        const float radius = 4.5f * ui_scale;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImU32 color = on ? (warning ? IM_COL32(255, 175, 40, 255) : IM_COL32(65, 220, 100, 255)) : IM_COL32(70, 75, 80, 255);
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(pos.x + radius, pos.y + ImGui::GetFrameHeight() / 2), radius, color);
        ImGui::Dummy(ImVec2(radius * 2, ImGui::GetFrameHeight()));
        ImGui::SameLine(0, 4 * ui_scale);
        ImGui::TextUnformatted(label);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tip);
    }

    inline void format_lamps(const Frame &f, bool available, bool stacked = false)
    {
        lamp("H", available && f.state == 'H', "Both ADC words match the normal format; no CRC."); ImGui::SameLine();
        lamp("R", available && f.state == 'R', "ADC format fits after rotating payload words right once; no CRC."); ImGui::SameLine();
        lamp("X", available && f.state == 'X', "Suspect format: display raw flags, do not interpret sensor values.", true);
        if (!stacked) ImGui::SameLine();
        lamp("SF0", available && f.sf == 0, "Subframe 0: E240"); ImGui::SameLine();
        lamp("SF1", available && f.sf == 1, "Subframe 1: E25F");
    }

    inline void flag_lamps(const char *label, uint16_t value, int bits, bool available, bool raw, bool wrap = false)
    {
        ImGui::TextUnformatted(label);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s[%d:0]: %0*X%s%s\nNo verified PCM bit assignments.\nTG-890 Fig. 6 (p. 26): boom release, first motion and full extension\nare analog TM levels; no mapping to w12/w13 is given.\nSDO 1600 p. III-16 describes the encoder, not its bit allocation.",
                label, bits - 1, (bits + 3) / 4, value, raw ? " (raw, suspect frame)" : "", available ? "" : " (no current frame)");
        ImGui::PushID(label);
        for (int b = bits - 1; b >= 0; --b)
        {
            const float width = std::max(17 * ui_scale, ImGui::CalcTextSize("15").x + 4 * ui_scale);
            const float height = ImGui::GetFrameHeight();
            if (!wrap || (b != bits - 1 && ImGui::GetItemRectMax().x + 2 * ui_scale + width <=
                                          ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x))
                ImGui::SameLine(0, 2 * ui_scale);
            bool on = available && (value & (1u << b));
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImGui::PushID(b);
            ImGui::InvisibleButton("bit", ImVec2(width, height));
            auto *dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), on ?
                (raw ? IM_COL32(155, 105, 25, 255) : IM_COL32(25, 130, 65, 255)) : IM_COL32(45, 48, 52, 255), 3 * ui_scale);
            char text[4]; std::snprintf(text, sizeof(text), "%d", b);
            auto size = ImGui::CalcTextSize(text);
            dl->AddText(ImVec2(pos.x + (width - size.x) / 2, pos.y + (height - size.y) / 2),
                available ? IM_COL32(235, 240, 240, 255) : IM_COL32(110, 115, 120, 255), text);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s bit %d: %s\nMeaning unknown%s", label, b, available ? ((value & (1u << b)) ? "1" : "0") : "no current frame",
                    raw ? "; raw word from suspect frame" : "");
            ImGui::PopID();
        }
        ImGui::PopID();
    }

    inline void status_flags(const Frame &f, bool available)
    {
        bool raw = f.state == 'X';
        const auto &w = raw ? f.raw : f.words;
        flag_lamps("w12", w[11], 16, available, raw);
        ImGui::SameLine();
        flag_lamps("w13", w[12] & 0xfff, 12, available, raw);
    }

    inline void frame_values(const Frame &f, bool available)
    {
        if (available && f.state != 'X')
            ImGui::Text("C%u G%u A%u/%u", f.words[8], sun_gray(f.words[12]), adc_counts(f.words[6]), adc_counts(f.words[14]));
        else
            ImGui::TextDisabled("C-- G-- A--/--");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("C: frame counter (modulo 65536)\nG: probable sun index, reversed high nibble of w13 decoded as Gray\nA: PAM ADC counts in w7 / w15\nUnavailable for stale or suspect frames.");
    }

    // The decoder panels use vertical sections; the product inspector retains
    // its compact row helpers above.
    inline void frame_details(const Frame &f, bool available)
    {
        ImGui::SeparatorText("Encoder format");
        format_lamps(f, available, true);
        ImGui::SeparatorText("Latest values");
        if (ImGui::BeginTable("Frame values", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV))
        {
            const char *labels[] = {"Frame counter", "Sun Gray (probable)", "PAM ADC w7", "PAM ADC w15"};
            const unsigned values[] = {f.words[8], sun_gray(f.words[12]), adc_counts(f.words[6]), adc_counts(f.words[14])};
            for (int i = 0; i < 4; ++i)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextWrapped("%s", labels[i]);
                ImGui::TableNextColumn();
                if (available && f.state != 'X') ImGui::Text("%u", values[i]);
                else ImGui::TextDisabled("--");
            }
            ImGui::EndTable();
        }
        ImGui::SeparatorText("Status bits");
        const bool raw = f.state == 'X';
        const auto &words = raw ? f.raw : f.words;
        flag_lamps("w12", words[11], 16, available, raw, true);
        flag_lamps("w13", words[12] & 0xfff, 12, available, raw, true);
        ImGui::TextWrapped("Unverified bit assignments; no CRC. ADC values are raw counts.");
    }
}
