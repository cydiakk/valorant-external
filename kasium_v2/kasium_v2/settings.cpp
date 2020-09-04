#include "settings.hpp"

namespace settings
{
	bool show_menu = true;
	bool is_ingame = true;

	namespace esp
	{
		bool master = true;

		bool hide_dormants = false;
		bool draw_team = true;
		bool boxes = true;
		bool laser = true;
		bool health = true;
		bool skeleton = true;
	}
	
	namespace aimbot
	{
		bool master = true;
		float aimkey = 0x12;
		float fov = 120;
		float smooth = 20;
		bool rcs = true;
		bool aim_team = true;
	}

	namespace misc {
		bool draw_aim_fov = true;
		bool draw_crosshair = true;
		float crosshair_size = 25;
	}
}