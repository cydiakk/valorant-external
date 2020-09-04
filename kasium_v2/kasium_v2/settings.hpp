#pragma once

namespace settings
{
	extern bool show_menu;
	extern bool is_ingame;

	namespace esp
	{
		extern bool master;
		extern bool hide_dormants;
		extern bool draw_team;
		extern bool boxes;
		extern bool distance;
		extern bool laser;
		extern bool health;
		extern bool skeleton;
	}

	namespace aimbot
	{
		extern float aimkey;
		extern bool master;
		extern float fov;
		extern float smooth;
		extern bool rcs;
		extern bool aim_team;
		//extern bool aim_lock;
	}

	namespace misc {
		extern bool draw_fps;
		extern bool draw_aim_fov;
		extern bool draw_crosshair;
		extern float crosshair_size;
	}
}