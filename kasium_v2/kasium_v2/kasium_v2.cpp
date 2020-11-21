#include "stdafx.hpp"

int kasium/*main*/()
{
	printf(_xor_("\n\n").c_str());
	printf(_xor_("[*] ... start VALORANT!\n").c_str());
	printf(_xor_("[*] ... exit with F12!\n").c_str());

	if (!core::core_init())
		return 1;

	utils::hide_from_taskbar(GetConsoleWindow());

	while (globals::t_hwnd == nullptr) {
		globals::t_hwnd = FindWindowA(0, _xor_("VALORANT  ").c_str());
		Sleep(1000);
	}

	printf(_xor_("[*] target found!\n").c_str());

	GetWindowThreadProcessId(globals::t_hwnd, (DWORD*)& globals::t_proc_id);

	if (globals::t_hwnd && globals::t_proc_id) {
		cheat::cheat_loop();
	}

	return 0;
}