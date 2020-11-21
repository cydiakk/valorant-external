#include "stdafx.hpp"

std::string utils::randomstring(int len)
{
	char alphabet[] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g',
						  'h', 'i', 'j', 'k', 'l', 'm', 'n',
						  'o', 'p', 'q', 'r', 's', 't', 'u',
						  'v', 'w', 'x', 'y', 'z' };

	std::string res = "";
	for (int i = 0; i < len; i++)
		res = res + alphabet[rand() % 27];

	return res;
}

std::string utils::string_to_utf8(const std::string & str)
{
	int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);

	wchar_t* pwBuf = new wchar_t[nwLen + 1];
	ZeroMemory(pwBuf, nwLen * 2 + 2);

	::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen);

	int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

	char* pBuf = new char[nLen + 1];
	ZeroMemory(pBuf, nLen + 1);

	::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

	std::string retStr(pBuf);

	delete[]pwBuf;
	delete[]pBuf;

	pwBuf = NULL;
	pBuf = NULL;

	return retStr;
}

void utils::hide_from_taskbar(HWND hwnd) {
	ITaskbarList* pTaskList = NULL;
	HRESULT initRet = CoInitialize(NULL);
	HRESULT createRet = CoCreateInstance(CLSID_TaskbarList,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskbarList,
		(LPVOID*)& pTaskList);

	if (createRet == S_OK)
	{

		pTaskList->DeleteTab(hwnd);

		pTaskList->Release();
	}

	CoUninitialize();
}

bool utils::is_valid_addr(uint64_t addr) {
	if (addr > 0x1000 && addr < 0x7FFFFFFFFFFF)
		return true;
	else
		return false;
}

std::uintptr_t utils::scanPattern(std::uint8_t * base, const std::size_t size, char* pattern, char* mask) {
	const auto patternSize = strlen(mask);

	for (std::size_t i = {}; i < size - patternSize; i++)
	{
		for (std::size_t j = {}; j < patternSize; j++)
		{
			if (mask[j] != '?' && *reinterpret_cast<std::uint8_t*>(base + i + j) != static_cast<std::uint8_t>(pattern[j]))
				break;

			if (j == patternSize - 1)
				return reinterpret_cast<std::uintptr_t>(base) + i;
		}
	}

	return {};
}

DWORD utils::get_proc_id_by_name(LPCTSTR lpczProc)
{
	HANDLE			hSnap;
	PROCESSENTRY32	peProc;
	DWORD			dwRet = -1;

	if ((hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) != INVALID_HANDLE_VALUE)
	{
		peProc.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnap, &peProc))
			while (Process32Next(hSnap, &peProc))
				if (!lstrcmp(lpczProc, peProc.szExeFile))
					dwRet = peProc.th32ProcessID;
	}
	CloseHandle(hSnap);

	return dwRet;
}

bool utils::get_process_threads(uint32_t dwOwnerPID, std::list<uint32_t>& thread_ids) {
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;

	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap == INVALID_HANDLE_VALUE)
		return(FALSE);

	// Fill in the size of the structure before using it. 
	te32.dwSize = sizeof(THREADENTRY32);

	if (!Thread32First(hThreadSnap, &te32))
	{
		CloseHandle(hThreadSnap);
		return false;
	}

	do
	{
		if (te32.th32OwnerProcessID == dwOwnerPID)
		{
			thread_ids.push_back(te32.th32ThreadID);
		}
	} while (Thread32Next(hThreadSnap, &te32));

	CloseHandle(hThreadSnap);
	return true;
}

uint64_t utils::get_threadstack_address(uint32_t process_id, std::list<uint32_t>& thread_ids) {
	utils::get_process_threads(process_id, thread_ids);

	std::list<uint32_t>::iterator it;
	NT_TIB tib = { 0 };
	THREAD_BASIC_INFORMATION tbi = { 0 };
	uint64_t stacktop = 0;
	size_t size = 0;
	NTSTATUS status;

typedef NTSTATUS(WINAPI * ntqueryinformationthread)(HANDLE, LONG, PVOID, ULONG, PULONG);
	ntqueryinformationthread NtQueryInformationThread;

	if (NtQueryInformationThread = (ntqueryinformationthread)GetProcAddress(LoadLibraryA("ntdll.dll"), "NtQueryInformationThread")) {
		for (it = thread_ids.begin(); it != thread_ids.end(); ++it) {
			HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT, false, *it);
			if (hThread) {
				status = NtQueryInformationThread(hThread, 0, &tbi, sizeof(tbi), nullptr);
				stacktop = core::read<uint64_t>(process_id, (uint64_t)tbi.TebBaseAddress + 8);
				if (stacktop != 0) {
					//find the stack entry pointing to the function that calls "ExitXXXXXThread"
					//Fun thing to note: It's the first entry that points to a address in kernel32
					byte ptrs[4096] = { 0 };
					ZeroMemory(ptrs, sizeof(ptrs));

					uint32_t size = 0;
					uint64_t bounds = 0;
					core::get_um_module(process_id, "kernel32.dll", bounds, size);

					if (bounds) {
						if (core::mem_cpy(process_id, stacktop - 4096, GetCurrentProcessId(), (uintptr_t)ptrs, sizeof(ptrs))) {
							//the proc is 64 bit
							for (uint64_t i = 8; i < 4096; i++) {
								uint64_t test = *(uintptr_t*)& ptrs[i];
								if (test > bounds && test < (bounds + size)) {
									return stacktop - 4096 + i;
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

wchar_t* utils::getwc(const char* c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, c, cSize);

	return wc;
}

bool utils::IsBitSet(byte b, int pos) { return (b & (1 << pos)) != 0; }

void utils::seprivilege()
{
	TOKEN_PRIVILEGES NewState;
	LUID luid;
	HANDLE hToken;

	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);

	NewState.PrivilegeCount = 1;
	NewState.Privileges[0].Luid = luid;
	NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(hToken, FALSE, &NewState, sizeof(NewState), NULL, NULL);

	CloseHandle(hToken);

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		printf("Execute o programa como Administrador.");
		Sleep(5000);
		ExitProcess(0);
	}
}

void utils::parse_config() {
	std::ifstream cFile(_xor_("config.cfg").c_str());
	if (cFile.is_open())
	{
		std::string line;
		while (getline(cFile, line)) {
			try {
				line.erase(std::remove_if(line.begin(), line.end(), isspace),
					line.end());
				if (line[0] == '#' || line.empty())
					continue;
				auto delimiterPos = line.find("=");
				auto name = line.substr(0, delimiterPos);
				auto value = line.substr(delimiterPos + 1);


				//ESP
				if (name == _xor_("bool::esp::master").c_str())
					settings::esp::master = std::stoi(value);
				if (name == _xor_("bool::esp::hide_dormants").c_str())
					settings::esp::hide_dormants = std::stoi(value);
				if (name == _xor_("bool::esp::team").c_str())
					settings::esp::draw_team = std::stoi(value);
				if (name == _xor_("bool::esp::box").c_str())
					settings::esp::boxes = std::stoi(value);
				if (name == _xor_("bool::esp::skeleton").c_str())
					settings::esp::skeleton = std::stoi(value);
				if (name == _xor_("bool::esp::laster").c_str())
					settings::esp::laser = std::stoi(value);
				if (name == _xor_("bool::esp::health").c_str())
					settings::esp::health = std::stoi(value);

				//AIMBOT
				if (name == _xor_("bool::aim::master").c_str())
					settings::aimbot::master = std::stoi(value);
				if (name == _xor_("hex::aim::key").c_str())
					settings::aimbot::aimkey = std::stoi(value);
				if (name == _xor_("dec::aim::fov").c_str())
					settings::aimbot::fov = std::stoi(value);
				if (name == _xor_("dec::aim::smooth").c_str())
					settings::aimbot::smooth = static_cast<float>(std::stoi(value));
				if (name == _xor_("bool::aim::rcs").c_str()) {
					if (std::stoi(value) < 8) {
						settings::aimbot::smooth = 8.f;
					}
					else {
						settings::aimbot::smooth = std::stoi(value);
					}
				}
				if (name == _xor_("bool::aim::rcs").c_str())
					settings::aimbot::rcs = std::stoi(value);
				if (name == _xor_("bool::aim::team").c_str())
					settings::aimbot::aim_team = std::stoi(value);

				//MISC
				if (name == _xor_("bool::show::aimfov").c_str())
					settings::misc::draw_aim_fov = std::stoi(value);
				if (name == _xor_("bool::show::crosshair").c_str())
					settings::misc::draw_crosshair = std::stoi(value);
				if (name == _xor_("dec::crosshairsize").c_str())
					settings::misc::crosshair_size = std::stoi(value);
			}
			catch (...) {
				continue;
			}
		}

		return;
	}
	else {
		std::cerr << _xor_("Couldn't open config file for reading.\n").c_str();
		return;
	}
}