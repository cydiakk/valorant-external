#define _CRT_SECURE_NO_WARNINGS 1

#include "menu.hpp"
#include "settings.hpp"
#include <functional>
#include "items/ID3DMenuItem.hpp"
#include "items/D3DMenuBoolItem.hpp"
#include "items/D3DMenuSubFolderItem.hpp"
#include "items/D3DMenuFlagItem.hpp"
#include "items/D3DMenuIntItem.hpp"
#include "items/D3DMenuFloatItem.hpp"
#include "D3DMenuInputItem.hpp"

#include <Windows.h>
//#include <d3dx9.h>

#include "stdafx.hpp"

char* menuDrawBuff = (char*)malloc(1024);

text_menu::text_menu()
{
	auto esp = std::make_shared<D3DMenuSubFolderItem>(_xor_("ESP").c_str());
	esp->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("MASTER ").c_str(), settings::esp::master, true));
	esp->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("Hide Dormants: ").c_str(), settings::esp::hide_dormants, true));
	esp->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("Draw Team: ").c_str(), settings::esp::draw_team, true));
	esp->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("Draw Skeletons: ").c_str(), settings::esp::skeleton, true));
	esp->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("Draw Boxes: ").c_str(), settings::esp::boxes, true));
	//esp->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("Draw Distance: ").c_str(), settings::esp::distance, true));
	esp->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("Draw Health: ").c_str(), settings::esp::health, true));
	esp->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("Draw Laser: ").c_str(), settings::esp::laser, false));

	auto aimbot = std::make_shared<D3DMenuSubFolderItem>(_xor_("Aimbot").c_str());
	aimbot->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("MASTER: ").c_str(), settings::aimbot::master, true));
	aimbot->add_sub_item(std::make_shared<D3DMenuInputItem>(_xor_("Aimkey: ").c_str(), settings::aimbot::aimkey, 2.f, 360.f, 1.f));
	aimbot->add_sub_item(std::make_shared<D3DMenuFloatItem>(_xor_("FOV: ").c_str(), settings::aimbot::fov, 2.f, 360.f, 2.f));
	aimbot->add_sub_item(std::make_shared<D3DMenuFloatItem>(_xor_("Smooth: ").c_str(), settings::aimbot::smooth, 20.f, 60.f, 2.f));
	aimbot->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("RCS: ").c_str(), settings::aimbot::rcs, true));
	aimbot->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("Aim on Team: ").c_str(), settings::aimbot::aim_team, true));
	//aimbot->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("Aim Lock: ").c_str(), settings::aimbot::aim_lock, true));

	auto misc = std::make_shared<D3DMenuSubFolderItem>(_xor_("Misc").c_str());
	misc->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("Draw Aim FOV: ").c_str(), settings::misc::draw_aim_fov, true));
	misc->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("Draw Crosshair: ").c_str(), settings::misc::draw_crosshair, true));
	//misc->add_sub_item(std::make_shared<D3DMenuBoolItem>(_xor_("Draw FPS: ").c_str(), settings::misc::draw_fps, true));
	misc->add_sub_item(std::make_shared<D3DMenuFloatItem>(_xor_("Crosshair Size: ").c_str(), settings::misc::crosshair_size, 5.f, 99.f, 1.f));

	menu_items.emplace_back(esp);
	menu_items.emplace_back(aimbot);
	menu_items.emplace_back(misc);

	menu_items.at(0)->is_selected() = true;

	font_height = 20;
}

void text_menu::render()
{
	menu_height = -static_cast<int>(font_height * 0.75f);

	// BattlEye doesn't like LL keyboard hooks.
	if (GetAsyncKeyState(VK_LEFT) & 1)
		handle_left();
	else if (GetAsyncKeyState(VK_RIGHT) & 1)
		handle_right();
	else if (GetAsyncKeyState(VK_UP) & 1)
		handle_up();
	else if (GetAsyncKeyState(VK_DOWN) & 1)
		handle_down();

	for (const auto& item : menu_items)
	{
		draw_menu_item(item);
	}
}

void text_menu::draw_menu_item(std::shared_ptr<ID3DMenuItem> item, const float padding)
{
	auto color = /*col.white*/2;
	if (item->is_selected())
		color = 3;

	auto name = item->get_name();

	if (item->is_subfolder())
	{
		const auto folder = std::static_pointer_cast<D3DMenuSubFolderItem>(item);

		if (folder->is_opened())
			name = "[ / ] " + item->get_name();
		else
			name = "[ + ] " + item->get_name();
	}

	sprintf(menuDrawBuff, u8"%s %s", name.c_str(), item->get_value_text().c_str());
	//render::draw_stroke_text(5 + padding, 50.f + static_cast<float>(menu_height += font_height), &color, menuDrawBuff);

	if (item->is_subfolder() && std::static_pointer_cast<D3DMenuSubFolderItem>(item)->is_opened())
	{
		static auto length = 50;

		for (const auto& sub : std::static_pointer_cast<D3DMenuSubFolderItem>(item)->get_sub_items())
		{
			draw_menu_item(sub, padding + length + 6.f);
		}
	}
}

std::vector<std::shared_ptr<ID3DMenuItem>> text_menu::get_all_visible_items()
{
	std::vector<std::shared_ptr<ID3DMenuItem>> ret;

	std::function<void(const std::vector<std::shared_ptr<ID3DMenuItem>> & list)> add_folder_items;
	add_folder_items = [&](const std::vector<std::shared_ptr<ID3DMenuItem>> & list)
	{
		for (const auto& item : list)
		{
			ret.emplace_back(item);

			if (item->is_subfolder())
			{
				auto folder = std::static_pointer_cast<D3DMenuSubFolderItem>(item);

				if (folder->is_opened())
					add_folder_items(folder->get_sub_items());
			}
		}
	};

	add_folder_items(menu_items);

	return ret;
}

std::shared_ptr<ID3DMenuItem> text_menu::get_current_selected()
{
	auto items = get_all_visible_items();
	for (const auto& item : items)
	{
		if (item->is_selected()) return item;
	}

	return nullptr;
}

void text_menu::handle_left()
{
	auto current = get_current_selected();
	if (!current) return;
	current->handle_left();
}

void text_menu::handle_right()
{
	auto current = get_current_selected();
	if (!current) return;
	current->handle_right();
}

void text_menu::handle_up()
{
	auto items = get_all_visible_items();

	auto current = get_current_selected();
	auto index = std::find(items.begin(), items.end(), current) - items.begin();

	if (index == 0)
		return;

	index--;

	auto indexed = items[index];
	current->is_selected() = false;
	indexed->is_selected() = true;
}

void text_menu::handle_down()
{
	auto items = get_all_visible_items();

	auto current = get_current_selected();
	auto index = std::find(items.begin(), items.end(), current) - items.begin();

	if (index >= static_cast<int>(items.size() - 1))
		return;

	index++;

	auto indexed = items[index];
	current->is_selected() = false;
	indexed->is_selected() = true;
}