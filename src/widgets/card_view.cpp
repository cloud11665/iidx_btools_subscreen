#include <cstdint>
#include <fstream>
#include <string_view>
#include <filesystem>
#include <string>
#include <array>
#include <print>

#include "imgui.h"
#include "imgui_internal.h"
#include "json.hpp"

#include "widgets/widgets.hpp"
#include "globals.hpp"

using namespace nlohmann;

std::once_flag of;
json card_data;

inline uint8_t hex_to_byte(char high, char low) {
	auto hex = [](char c) -> uint8_t {
		if ('0' <= c && c <= '9') return c - '0';
		if ('a' <= c && c <= 'f') return 10 + (c - 'a');
		if ('A' <= c && c <= 'F') return 10 + (c - 'A');
		throw std::invalid_argument("Invalid hex digit");
		};
	return (hex(high) << 4) | hex(low);
}

inline std::array<uint8_t, 8> hex16_to_bytes(std::string_view id) {
	if (id.size() != 16)
		throw std::invalid_argument("hex string must be exactly 16 characters");

	std::array<uint8_t, 8> bytes{};
	for (size_t i = 0; i < 8; ++i)
		bytes[i] = hex_to_byte(id[2 * i], id[2 * i + 1]);
	return bytes;
}

std::string get_appdata_path(const std::string& filename) {
	const char* appdata = std::getenv("APPDATA");
	if (!appdata) throw std::runtime_error("APPDATA env var not set");
	return std::string(appdata) + "\\" + filename;
}

static int selected_idx = -1;
std::optional<std::string> selected_id;


auto card_view::draw() -> void
{
	std::call_once(of, []() {
		std::ifstream ifs(get_appdata_path("spicetools_card_manager.json"));
		card_data = json::parse(ifs);
		});

	ImVec2 frame_padding = ImGui::GetStyle().FramePadding;

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize({ 600, 700 }, ImGuiCond_Appearing);

	if (g_gui_card_view_visible)
		ImGui::OpenPopup("card_button_modal");
	else
	{
		selected_id = std::nullopt;
		selected_idx = -1;
	}

	if (ImGui::BeginPopupModal("card_button_modal", &g_gui_card_view_visible, ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoScrollbar))
	{
		float table_height = ImGui::GetContentRegionAvail().y - frame_padding.y * 2 - 40.f;

		auto cards = card_data["pages"][0]["cards"];


		ImGui::PushFont(nullptr, 30.f);

		ImGui::SetNextWindowSize({ ImGui::GetContentRegionAvail().x, table_height });
		ImGui::BeginChild("cards_child");

		if (ImGui::BeginTable("cards", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInner | ImGuiTableFlags_Borders))
		{
			//ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Card");



			int i = 0;
			for (auto card : cards)
			{
				ImGui::PushID(i++);

				ImGui::TableNextRow();

				ImColor color;
				if (card.contains("color"))
				{
					float r = card["color"][0];
					float g = card["color"][1];
					float b = card["color"][2];
					color = ImColor(r, g, b, 1.f);
				}
				else
				{
					color = ImColor(200, 100, 200, 255);
				}

				ImGui::TableSetColumnIndex(0);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
				ImGui::ColorButton("##col", color);
				ImGui::SameLine();
				ImGui::PopStyleVar();


				//ImGui::TableSetColumnIndex(1);
				std::string_view name = card["name"];
				std::string_view id = card["id"];

				if (ImGui::Selectable(name.data(), selected_idx == i, ImGuiSelectableFlags_SpanAllColumns))
				{
					selected_idx = i;
					selected_id = id;
				}

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%s", id.data());

				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		ImGui::EndChild();
		ImVec2 sz = { (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.x * 2) / 2.f, 40.f};
		ImGui::BeginDisabled(selected_idx == -1 || g_eamio_card_p1 || !selected_id);
		if (ImGui::Button("Insert P1", sz))
		{
			std::println("p1 inserted from cardmenu {}", selected_id.value_or("ERROR NO VALUE"));
			g_eamio_card_p1 = hex16_to_bytes(*selected_id);
		}
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::BeginDisabled(selected_idx == -1 || g_eamio_card_p2 || !selected_id);
			if (ImGui::Button("Insert P2", sz))
			{
				std::println("p2 inserted from cardmenu {}", selected_id.value_or("ERROR NO VALUE"));
				g_eamio_card_p2 = hex16_to_bytes(*selected_id);
			}
		ImGui::EndDisabled();

		ImGui::PopFont();

		ImVec2 win_pos = ImGui::GetWindowPos();
		ImVec2 win_size = ImGui::GetWindowSize();
		ImRect modal_rect(win_pos, win_pos + win_size);

		if (ImGui::IsMouseClicked(0) && !modal_rect.Contains(ImGui::GetMousePos())) {
			ImGui::CloseCurrentPopup();
			g_gui_card_view_visible = false;
		}

		ImGui::EndPopup();

	}

}