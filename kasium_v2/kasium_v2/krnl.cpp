#include "stdafx.hpp"
#include "krnl_defs.hpp"

#define MAGICINICATOR 0x6942069420694269

//////////////////INITIALIZATION
uint64_t shared[4] = { 0 };
typedef INT64(__stdcall* nt_compare_signing_levels)(UINT64);

__forceinline void setupbuffer() {
	shared[0] = MAGICINICATOR;
}

void start_srv() {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	nt_compare_signing_levels NtCompareSigningLevels = (nt_compare_signing_levels)GetProcAddress(LoadLibraryA("ntdll.dll"), _xor_("NtCompareSigningLevels").c_str());
	if (!NtCompareSigningLevels) {
		return;
	}

	NtCompareSigningLevels(0xDEADBEEF);

	MessageBoxA(nullptr, _xor_("Please load the SERVICE before you start again!").c_str(), _xor_("ERROR").c_str(), NULL);
	exit(0);
}

////////////////FUNCTIONALITY
bool driver::init() {
	shared[1] = (uint64_t)DRIVER_CONTINUE;
	shared[2] = (uint64_t)DRIVER_CONTINUE;
	shared[3] = (uint64_t)DRIVER_CONTINUE;

	//setup buffer so kernelmode can find it
	setupbuffer();

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	std::cout << _xor_("[+] shared buffer ").c_str() << std::hex << "0x" << (uint64_t)& shared[1] << std::endl;
	std::cout << _xor_("[+] local base: ").c_str() << std::hex << (uint64_t)GetModuleHandleA(NULL) << std::endl;

	HANDLE hThread = CreateThread(NULL,
		0,
		(LPTHREAD_START_ROUTINE)start_srv,
		NULL,
		NULL,
		NULL);

	//give krnl some time to find the pattern
	Sleep(2000);

	if (!hThread)
		return false;

	return true;
}

bool driver::stop() {
	while (shared[1] != (uint64_t)DRIVER_STOP)
		shared[1] = (uint64_t)DRIVER_STOP;
	Sleep(100);
	return true;
}

///////////////////////FUNCTIONALITY

uint64_t driver::get_process_base(const char* process_name) {
	uint64_t base = NULL;
	_k_get_base_module out = {};

	memcpy(&out.name, process_name, sizeof(char[256]));
	out.dst = (uint64_t)& base;

	shared[2] = (uint64_t)& out;
	shared[1] = (uint64_t)DRIVER_GET_BASE;

	std::clock_t start;
	double duration;

	start = std::clock();

	while (shared[3] == (uint64_t)0) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(0));
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		if (duration > (double)0.01) {
			shared[1] = (uint64_t)DRIVER_CONTINUE;
			shared[2] = (uint64_t)DRIVER_CONTINUE;
			shared[3] = (uint64_t)DRIVER_CONTINUE;
			goto except;
		}
	}

	shared[1] = (uint64_t)DRIVER_CONTINUE;
	shared[2] = (uint64_t)DRIVER_CONTINUE;
	shared[3] = (uint64_t)DRIVER_CONTINUE;

	return base;

except:
	return get_process_base(process_name);
}

uint64_t driver::get_process_base_by_id(uint32_t pid) {
	uint64_t base = NULL;
	_k_get_base_by_id out = { pid, (uint64_t)& base };

	shared[2] = (uint64_t)& out;
	shared[1] = (uint64_t)DRIVER_GET_BASE_BY_ID;


	std::clock_t start;
	double duration;

	start = std::clock();

	while (shared[3] == (uint64_t)0) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(0));
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		if (duration > (double)0.1) {
			shared[1] = (uint64_t)DRIVER_CONTINUE;
			shared[2] = (uint64_t)DRIVER_CONTINUE;
			shared[3] = (uint64_t)DRIVER_CONTINUE;
			goto except;
		}
	}

	shared[1] = (uint64_t)DRIVER_CONTINUE;
	shared[2] = (uint64_t)DRIVER_CONTINUE;
	shared[3] = (uint64_t)DRIVER_CONTINUE;

	return base;

except:
	return get_process_base_by_id(pid);
}

#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

wchar_t* GetWC(const char* c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, c, cSize);

	return wc;
}

uint64_t driver::get_um_module(uint32_t process_id, const char* module_name, uint32_t & size) {
	uint64_t mod_base = NULL;
	uint32_t mod_size = NULL;
	_k_get_um_module out = {};

	wchar_t* wc = GetWC(module_name);

	memset(out.moduleName, 0, sizeof(WCHAR) * 256);
	wcscpy(out.moduleName, wc);

	out.dst_base = (uint64_t)& mod_base;
	out.dst_size = (uint64_t)& mod_size;
	out.pid = process_id;

	shared[2] = (uint64_t)& out;
	shared[1] = (uint64_t)DRIVER_GET_UM_MODULE;

	std::clock_t start;
	double duration;

	start = std::clock();

	while (shared[3] == (uint64_t)0) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(0));
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		if (duration > (double)0.01) {
			shared[1] = (uint64_t)DRIVER_CONTINUE;
			shared[2] = (uint64_t)DRIVER_CONTINUE;
			shared[3] = (uint64_t)DRIVER_CONTINUE;
			goto except;
		}
	}

	shared[1] = (uint64_t)DRIVER_CONTINUE;
	shared[2] = (uint64_t)DRIVER_CONTINUE;
	shared[3] = (uint64_t)DRIVER_CONTINUE;

	size = mod_size;

	return mod_base;

except:
	return get_um_module(process_id, module_name, size);
}

bool driver::copy_memory(
	uint32_t src_pid,
	uint64_t src_addr,
	uint32_t dst_pid,
	uint64_t dst_addr,
	size_t size) {
	_k_rw_request out = { src_pid, src_addr, dst_pid, dst_addr, size };

	while (shared[2] != (uint64_t)& out)
		shared[2] = (uint64_t)& out;

	shared[1] = (uint64_t)DRIVER_COPYMEMROY;

	std::clock_t start;
	double duration;

	start = std::clock();

	while (shared[3] == (uint64_t)0) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(0));
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		if (duration > (double)0.01) {
			shared[1] = (uint64_t)DRIVER_CONTINUE;
			shared[2] = (uint64_t)DRIVER_CONTINUE;
			shared[3] = (uint64_t)DRIVER_CONTINUE;
			goto except;
		}
	}

	if (shared[3] == (uint64_t)2) {
		shared[1] = (uint64_t)DRIVER_CONTINUE;
		shared[2] = (uint64_t)DRIVER_CONTINUE;
		shared[3] = (uint64_t)DRIVER_CONTINUE;
		return false;
	}
	else {
		shared[1] = (uint64_t)DRIVER_CONTINUE;
		shared[2] = (uint64_t)DRIVER_CONTINUE;
		shared[3] = (uint64_t)DRIVER_CONTINUE;
		return true;
	}

except:
	//printf("ERROR");
	copy_memory(src_pid, src_addr, dst_pid, dst_addr, size);
}

bool driver::copy_to_readonly(
	uint32_t src_pid,
	uint64_t src_addr,
	uint32_t dst_pid,
	uint64_t dst_addr,
	size_t size) {
	_k_rw_request out = { src_pid, src_addr, dst_pid, dst_addr, size };

	shared[2] = (uint64_t)& out;
	shared[1] = (uint64_t)DRIVER_WRITE_TO_READONLY;

	std::clock_t start;
	double duration;

	start = std::clock();

	while (shared[3] == (uint64_t)0) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(0));
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		if (duration > (double)0.01) {
			shared[1] = (uint64_t)DRIVER_CONTINUE;
			shared[2] = (uint64_t)DRIVER_CONTINUE;
			shared[3] = (uint64_t)DRIVER_CONTINUE;
			goto except;
		}
	}

	if (shared[3] == (uint64_t)2) {
		shared[1] = (uint64_t)DRIVER_CONTINUE;
		shared[2] = (uint64_t)DRIVER_CONTINUE;
		shared[3] = (uint64_t)DRIVER_CONTINUE;
		return false;
	}
	else {
		shared[1] = (uint64_t)DRIVER_CONTINUE;
		shared[2] = (uint64_t)DRIVER_CONTINUE;
		shared[3] = (uint64_t)DRIVER_CONTINUE;
		return true;
	}

except:
	copy_to_readonly(src_pid, src_addr, dst_pid, dst_addr, size);
}

bool driver::virtual_protect(
	uint32_t process_id,
	uintptr_t address,
	uint32_t protect,
	size_t size) {
	_k_virtual_protect out = { process_id, protect, address, size };

	shared[2] = (uint64_t)& out;
	shared[1] = (uint64_t)DRIVER_PROTECT;

	std::clock_t start;
	double duration;

	start = std::clock();

	while (shared[3] == (uint64_t)0) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(0));
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		if (duration > (double)0.01) {
			shared[1] = (uint64_t)DRIVER_CONTINUE;
			shared[2] = (uint64_t)DRIVER_CONTINUE;
			shared[3] = (uint64_t)DRIVER_CONTINUE;
			goto except;
		}
	}

	if (shared[3] == (uint64_t)2) {
		shared[1] = (uint64_t)DRIVER_CONTINUE;
		shared[2] = (uint64_t)DRIVER_CONTINUE;
		shared[3] = (uint64_t)DRIVER_CONTINUE;
		return false;
	}
	else {
		shared[1] = (uint64_t)DRIVER_CONTINUE;
		shared[2] = (uint64_t)DRIVER_CONTINUE;
		shared[3] = (uint64_t)DRIVER_CONTINUE;
		return true;
	}

except:
	virtual_protect(process_id, address, protect, size);
}

uint64_t driver::virtual_alloc(
	uint32_t process_id,
	uint32_t allocation_type,
	uint32_t protect,
	size_t size) {
	uint64_t base = NULL;
	_k_virtual_alloc out = { process_id, allocation_type, protect, (uint64_t)& base, size };

	shared[3] = (uint64_t)0;
	shared[2] = (uint64_t)& out;
	shared[1] = (uint64_t)DRIVER_ALLOC;

	std::clock_t start;
	double duration;

	start = std::clock();

	while (shared[3] == (uint64_t)0) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(0));
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		if (duration > (double)0.01) {
			shared[1] = (uint64_t)DRIVER_CONTINUE;
			shared[2] = (uint64_t)DRIVER_CONTINUE;
			shared[3] = (uint64_t)DRIVER_CONTINUE;
			goto except;
		}
	}

	shared[1] = (uint64_t)DRIVER_CONTINUE;
	shared[2] = (uint64_t)DRIVER_CONTINUE;
	shared[3] = (uint64_t)DRIVER_CONTINUE;

	return base;

except:
	return virtual_alloc(process_id, allocation_type, protect, size);
}

bool driver::virtual_free(uint32_t process_id, uintptr_t address, size_t size, uint64_t freetype) {
	_k_virtual_free out = { process_id, address, size, freetype };

	shared[2] = (uint64_t)& out;
	shared[1] = (uint64_t)DRIVER_FREE;

	while (shared[3] == (uint64_t)0) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(0));
	}

	if (shared[3] == (uint64_t)2) {
		shared[1] = (uint64_t)DRIVER_CONTINUE;
		shared[2] = (uint64_t)DRIVER_CONTINUE;
		shared[3] = (uint64_t)DRIVER_CONTINUE;
		return false;
	}
	else {
		shared[1] = (uint64_t)DRIVER_CONTINUE;
		shared[2] = (uint64_t)DRIVER_CONTINUE;
		shared[3] = (uint64_t)DRIVER_CONTINUE;
		return true;
	}
}

bool driver::secure_memory(
	uint32_t process_id,
	uint64_t addr,
	size_t size,
	uint64_t probemode) {
	_k_secure_mem out = { process_id, addr, size, probemode };

	shared[3] = (uint64_t)0;
	shared[2] = (uint64_t)& out;
	shared[1] = (uint64_t)DRIVER_SECURE;

	while (shared[3] == (uint64_t)0) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(0));
	}

	if (shared[3] == (uint64_t)2) {
		shared[1] = (uint64_t)DRIVER_CONTINUE;
		shared[2] = (uint64_t)DRIVER_CONTINUE;
		shared[3] = (uint64_t)DRIVER_CONTINUE;
		return false;
	}
	else {
		shared[1] = (uint64_t)DRIVER_CONTINUE;
		shared[2] = (uint64_t)DRIVER_CONTINUE;
		shared[3] = (uint64_t)DRIVER_CONTINUE;
		return true;
	}
}

void driver::get_thread(HWND window_handle, uint64_t * thread_context) {
	_k_gen_thread_ctx out{};
	out.window_handle = reinterpret_cast<uint64_t>(window_handle);
	out.thread_pointer = 10;
	out.thread_alternative = 0;

	shared[3] = (uint64_t)0;
	shared[2] = (uint64_t)& out;
	shared[1] = (uint64_t)DRIVER_GET_THREAD;

	std::clock_t start;
	double duration;

	start = std::clock();

	while (shared[3] == (uint64_t)0) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(0));
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		if (duration > (double)1) {
			shared[1] = (uint64_t)DRIVER_CONTINUE;
			shared[2] = (uint64_t)DRIVER_CONTINUE;
			shared[3] = (uint64_t)DRIVER_CONTINUE;
			goto except;
		}
	}

	shared[1] = (uint64_t)DRIVER_CONTINUE;
	shared[2] = (uint64_t)DRIVER_CONTINUE;
	shared[3] = (uint64_t)DRIVER_CONTINUE;

	*thread_context = out.thread_pointer;
	return;

except:
	return get_thread(window_handle, thread_context);
}

void driver::set_thread(HWND window_handle, uint64_t thread_context) {
	_k_gen_thread_ctx out{};

	out.window_handle = reinterpret_cast<uint64_t>(window_handle);
	out.thread_pointer = thread_context;
	out.thread_alternative = thread_context;

	shared[3] = (uint64_t)0;
	shared[2] = (uint64_t)& out;
	shared[1] = (uint64_t)DRIVER_SET_THREAD;

	std::clock_t start;
	double duration;

	start = std::clock();

	while (shared[3] == (uint64_t)0) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(0));
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		if (duration > (double)1) {
			shared[1] = (uint64_t)DRIVER_CONTINUE;
			shared[2] = (uint64_t)DRIVER_CONTINUE;
			shared[3] = (uint64_t)DRIVER_CONTINUE;
			goto except;
		}
	}

	shared[1] = (uint64_t)DRIVER_CONTINUE;
	shared[2] = (uint64_t)DRIVER_CONTINUE;
	shared[3] = (uint64_t)DRIVER_CONTINUE;

	return;

except:
	return set_thread(window_handle, thread_context);
}