#include "stdafx.hpp"

#include "comp_render.hpp"

//#define DEBUG_PRINT

namespace cheat {
	std::vector<TslEntity> entityList;
	magic::HijackState state = { 0 };

	LocalPlayer localPlayer = { 0 };
	entityCache entities[0x800] = { 0 };

	bool bShellcodeWritten = false;
	uintptr_t plocalproxy = 0;
	uintptr_t pentitycache = 0;
	uintptr_t decrypted_world = 0;

	bool cache() {
		std::vector<TslEntity> tmpList;

		//do magic
		if (!bShellcodeWritten) { 
			//really critical... will close the cheat on failure
			if (magic::magic_scan(decryptor::pkey, decryptor::pstate)) {
				Beep(300, 100); Beep(500, 100);
				decrypted_world = decryptor::read_uworld(globals::t_process_base);
				if (!utils::is_valid_addr(decrypted_world)) { return false; }
				magic::write_shell(decrypted_world, globals::t_process_base, pentitycache, plocalproxy);
			}
			else {
				MessageBoxA(nullptr, _xor_("AutoUpdate ERROR!").c_str(), "ERROR", NULL);
				exit(0);
			}
		}
		bShellcodeWritten = true;

		state = magic::read_results();

		cache::actors = state.actors.Ptr;
		cache::actor_count = state.actors.Size;
		//end magic

		if (cache::actor_count > 0x800) { return false; }

		if (!utils::is_valid_addr(cache::actors)) { return false; }
		localPlayer.get_localplayer(state);

		//copy all entities at once
		core::mem_cpy(globals::t_proc_id, pentitycache, GetCurrentProcessId(), (uintptr_t)entities, sizeof(entityCache) * cache::actor_count);

		TslEntity tslEntity{};
		for (uint32_t i = 0; i < cache::actor_count; i++) {
			entityCache cached = entities[i];

			if (!tslEntity.get_info(cached)) {
				continue;
			}
			else {
				tmpList.push_back(tslEntity);
			}
		}
		entityList = tmpList;
		return true;
	}

	void draw_skeleton(d2d_renderer_t& renderer, TslEntity& entity, D2D1::ColorF color) {
		if (entity.num_bones < 90) { return; }

		Vector3 neckpos = engine::GetBoneWithRotation(entity.mesh, engine::e_male_bones::Spine4);
		Vector3 pelvispos = engine::GetBoneWithRotation(entity.mesh, 72);

		// draw head bone
		Vector3 headpos = engine::GetBoneWithRotation(entity.mesh, 8);
		Vector3 headpos2d = engine::WorldToScreen(headpos, localPlayer.camera_position, localPlayer.camera_rotation, localPlayer.fov);
		renderer.draw_circle(headpos2d.x, headpos2d.y, (4500 / localPlayer.camera_position.Distance(headpos)) * 2.5, 2, D2D1::ColorF::WhiteSmoke, false);

		Vector3 previous(0, 0, 0);
		Vector3 current, p1, c1;

		for (auto a : (entity.num_bones == 99 ? engine::f_skeleton : engine::m_skeleton)) {
			previous = Vector3(0, 0, 0);

			for (int bone : a)
			{
				current = bone == 6 ? neckpos : (bone == 72 ? pelvispos : engine::GetBoneWithRotation(entity.mesh, bone));

				if (previous.x == 0.f)
				{
					previous = current;
					continue;
				}

				p1 = engine::WorldToScreen(previous, localPlayer.camera_position, localPlayer.camera_rotation, localPlayer.fov);
				c1 = engine::WorldToScreen(current, localPlayer.camera_position, localPlayer.camera_rotation, localPlayer.fov);

				renderer.draw_line(p1.x, p1.y, c1.x, c1.y, 2, color);

				previous = current;
			}
		}
	}

	void docheat(d2d_renderer_t& renderer) {
		if (!cache() || entityList.empty()) {
			settings::is_ingame = false;
			return;
		}

		settings::is_ingame = true;

		//settings::is_ingame = true;
		auto entityListCpy = entityList;

		float distance = 0.f;
		int revise = 0;
		float min_angle = FLT_MAX;
		TslEntity closes_entity{};

		for (uint32_t i = 0; i < entityListCpy.size(); ++i) {
			TslEntity current = entityListCpy[i];

			if (localPlayer.local_pawn == current.p_obj_ptr || !utils::is_valid_addr(current.skeletal_mesh))
				continue;

			if (current.head_position.z <= current.root_position.z || current.head_position_2d.y > current.root_position_2d.y)
				continue;

			if (current.num_bones > 5 && current.num_bones < 40) {
				renderer.draw_corner_box(current.root_position_2d.x - 20.f, current.root_position_2d.y - 20.f, 100 / (distance / 3.5f), 100 / (distance / 3.5f), 2, D2D1::ColorF::White);
				continue;
			}

			if (current.num_bones < 95 || current.num_bones > 103) { continue; }
			{
				Vector3 neckpos = engine::GetBoneWithRotation(current.mesh, engine::e_male_bones::Spine4);
				Vector3 pelvispos = engine::GetBoneWithRotation(current.mesh, 72);

				if (neckpos.x < current.root_position.x - 200 || neckpos.x > current.root_position.x + 200 || neckpos.y < current.root_position.y - 200 || neckpos.y > current.root_position.y + 200 || neckpos.z < current.root_position.z - 200 || neckpos.z > current.root_position.z + 200)
					continue;

				if (pelvispos.x < current.root_position.x - 200 || pelvispos.x > current.root_position.x + 200 || pelvispos.y < current.root_position.y - 200 || pelvispos.y > current.root_position.y + 200 || pelvispos.z > current.root_position.z + 200 || pelvispos.z < current.root_position.z - 200)
					continue;
			}

			//update cameramanager every entity to keep things fluent
			localPlayer.get_camera();
			distance = localPlayer.camera_position.Distance(current.root_position);

			int height = (current.root_position_2d.y - current.head_position_2d.y) * 2.5;

			if (settings::esp::master) {
				if (!settings::esp::hide_dormants) {
					if (current.b_dormant && current.health > 0) {
						if (settings::esp::boxes)
							renderer.draw_corner_box(current.head_position_2d.x - height / 4, current.head_position_2d.y - height / 6, height / 2, height, 2, D2D1::ColorF::Gray);

						if (settings::esp::health)
							renderer.draw_health_bar(current.health, current.head_position_2d.x, current.head_position_2d.y - height / 6, height, D2D1::ColorF::Gray);

						if (settings::esp::skeleton)
							draw_skeleton(renderer, current, D2D1::ColorF::Gray);

						if (settings::esp::laser)
							renderer.draw_line(globals::screen_width / 2, globals::screen_height, (current.head_position_2d.x - height / 4) + (height / 4), current.head_position_2d.y - height / 6 + height, 1, D2D1::ColorF::Gray);
					}
				}

				if (!current.b_dormant/* && current.health > 0*/) {
					if (localPlayer.team == current.team) {
						if (settings::esp::draw_team) {
							if (settings::esp::health)
								renderer.draw_health_bar(current.health, current.head_position_2d.x, current.head_position_2d.y - height / 6, height, D2D1::ColorF::Green);

							if (settings::esp::boxes)
								renderer.draw_corner_box(current.head_position_2d.x - height / 4, current.head_position_2d.y - height / 6, height / 2, height, 2, D2D1::ColorF::White);

							if (settings::esp::skeleton)
								draw_skeleton(renderer, current, D2D1::ColorF::WhiteSmoke);

							if (settings::esp::laser)
								renderer.draw_line(globals::screen_width / 2, globals::screen_height, (current.head_position_2d.x - height / 4) + (height / 4), current.head_position_2d.y - height / 6 + height, 2, D2D1::ColorF::White);
						}
					}
					else {
						if (settings::esp::health)
							renderer.draw_health_bar(current.health, current.head_position_2d.x, current.head_position_2d.y - height / 6, height, D2D1::ColorF::Green);

						if (current.b_recently_rendered) {
							if (settings::esp::boxes)
								renderer.draw_corner_box(current.head_position_2d.x - height / 4, current.head_position_2d.y - height / 6, height / 2, height, 2, D2D1::ColorF::Green);

							if (settings::esp::skeleton)
								draw_skeleton(renderer, current, D2D1::ColorF::WhiteSmoke);

							if (settings::esp::laser)
								renderer.draw_line(globals::screen_width / 2, globals::screen_height, (current.head_position_2d.x - height / 4) + (height / 4), current.head_position_2d.y - height / 6 + height, 2, D2D1::ColorF::Green);
						}
						else {
							if (settings::esp::boxes)
								renderer.draw_corner_box(current.head_position_2d.x - height / 4, current.head_position_2d.y - height / 6, height / 2, height, 2, D2D1::ColorF::Red);

							if (settings::esp::skeleton)
								draw_skeleton(renderer, current, D2D1::ColorF::WhiteSmoke);

							if (settings::esp::laser)
								renderer.draw_line(globals::screen_width / 2, globals::screen_height, (current.head_position_2d.x - height / 4) + (height / 4), current.head_position_2d.y - height / 6 + height, 2, D2D1::ColorF::Red);
						}
					}
				}
			}

			//aimbot setup
			//team check
			if (current.team == localPlayer.team && !settings::aimbot::aim_team)
				continue;

			auto dx = current.head_position_2d.x - (globals::screen_width / 2.0f);
			auto dy = current.head_position_2d.y - (globals::screen_height / 2.0f);
			auto dist = sqrtf(dx * dx + dy * dy);

			if (dist <= settings::aimbot::fov && dist <= min_angle) {
				if (!current.b_dormant) {
					min_angle = dist;
					closes_entity = current;
				}
			}
		}

		if (settings::aimbot::master && GetAsyncKeyState(settings::aimbot::aimkey) & 0x8000 && closes_entity.mesh) {
			uint32_t on = 0x1111;
			//do aimbot
			if (utils::is_valid_addr(plocalproxy)) {
				aimbot::aimbot(closes_entity, plocalproxy);
				core::write<uint32_t>(globals::t_proc_id, (uintptr_t)&on, plocalproxy + 0x48, sizeof(uint32_t));
			}
		}
		else {
			uint32_t off = 0x0;
			core::write<uint32_t>(globals::t_proc_id, (uintptr_t)&off, plocalproxy + 0x48, sizeof(uint32_t));
		}
	}
	
	void cheat_loop() {
		while(!utils::is_valid_addr(globals::t_process_base))
			globals::t_process_base = core::get_process_base_by_id(globals::t_proc_id);

		printf(_xor_("[+] id: %d\n").c_str(), globals::t_proc_id);
		printf(_xor_("[+] base:  0x%p\n").c_str(), globals::t_process_base);

		d2d_window_t window { };
		d2d_renderer_t renderer{ window._handle, globals::t_hwnd };
		uint32_t counter = 0;

#ifndef DEBUG_PRINT
		ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
		while (true) {
			static const auto center = wnd_hjk::vec2_t{ wnd_hjk::screen_resolution.first * 0.5f, wnd_hjk::screen_resolution.second * 0.5f };
			globals::screen_width = wnd_hjk::screen_resolution.first;
			globals::screen_height = wnd_hjk::screen_resolution.second;

			renderer.begin_scene();
			renderer.draw_line(0, 0, 5, 5, 10, D2D1::ColorF::LightGray);

			//draw crosshair ghetto
			if (settings::is_ingame) {
				if (settings::misc::draw_crosshair) {
					renderer.draw_crosshair(globals::screen_width / 2, globals::screen_height / 2, D2D1::ColorF::WhiteSmoke, settings::misc::crosshair_size);
				}

				if (settings::misc::draw_aim_fov) {
					renderer.draw_circle(globals::screen_width / 2, globals::screen_height / 2, settings::aimbot::fov, 1, D2D1::ColorF::LightGray, false);
				}
			}

			docheat(renderer);
			renderer.end_scene();

			//exit if key is pressed
			if (GetAsyncKeyState(VK_F12) & 0x8000) {
				Beep(100, 100);
				break;
			}

			//exit if game is closed
			if (!FindWindowA(0, _xor_("VALORANT  ").c_str())) {
				Beep(100, 100);
				break;
			}

			counter++;
			if (counter >= 20) {
				utils::parse_config();
				counter = 0;
			}
		}
	}
}