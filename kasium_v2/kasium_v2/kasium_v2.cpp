#include "stdafx.hpp"

int kasium()
{
	printf(_xor_("\n\n").c_str());
	printf(_xor_("[*] ... start VALORANT!\n").c_str());
	printf(_xor_("[*] ... exit with F12!\n").c_str());

	utils::hide_from_taskbar(GetConsoleWindow());

	std::clock_t start = std::clock();
	double duration;

	while (globals::t_hwnd == nullptr) {
		globals::t_hwnd = FindWindowA(0, _xor_("VALORANT  ").c_str());
		Sleep(1000);

		//timeout
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		if (duration > (double)180) {
			MessageBoxA(nullptr, "Timed out", "TIMEOUT", NULL);
			return 1;
		}

		//stop if neccesary
		if (GetAsyncKeyState(VK_F12) & 1) {
			return 1;
		}
	}

	printf(_xor_("[*] target found!\n").c_str());

	GetWindowThreadProcessId(globals::t_hwnd, (DWORD*)& globals::t_proc_id);

	if (globals::t_hwnd && globals::t_proc_id) {
		cheat::cheat_loop();
	}

	return 0;
}