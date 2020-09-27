#include "stdafx.hpp"

namespace core {
	nt_compare_signing_levels ntcomparesigninglevles = nullptr;

	bool core_init() {
		ntcomparesigninglevles = (nt_compare_signing_levels)GetProcAddress(LoadLibraryA("ntdll.dll"), "NtCompareSigningLevels");
		if (!ntcomparesigninglevles) {
			//some more shit
			return false;
		}
		else
		{
			return true;
		}
	}

	uint64_t get_process_base_by_id(uint32_t pid) {
		uint64_t base = 0;
		_k_get_base_by_id out = { pid, (uint64_t)& base };

		uint64_t status = ntcomparesigninglevles(0xDEADBEEF + DRIVER_GET_BASE_BY_ID, &out);

		return base;
	}

	uint64_t virtual_alloc(uint32_t process_id, uint32_t allocation_type, uint32_t protect, size_t size) {
		uint64_t base = 0;
		_k_virtual_alloc out = { process_id, allocation_type, protect, (uint64_t)& base, size };

		uint64_t status = ntcomparesigninglevles(0xDEADBEEF + DRIVER_ALLOC, &out);
		return base;
	}

	bool get_um_module(uint32_t pid, const char* module_name, uint64_t& base, uint32_t& size) {
		uint64_t mod_base = NULL;
		uint32_t mod_size = NULL;
		_k_get_um_module out = {};

		wchar_t* wc = utils::getwc(module_name);

		memset(out.moduleName, 0, sizeof(WCHAR) * 256);
		wcscpy(out.moduleName, wc);

		out.dst_base = (uint64_t)& mod_base;
		out.dst_size = (uint64_t)& mod_size;
		out.pid = pid;

		uint64_t status = ntcomparesigninglevles(0xDEADBEEF + DRIVER_GET_UM_MODULE, &out);
		//if (status == 0xDEADBEEF) {
		base = mod_base;
		size = mod_size;
		return true;
		//}
		//else
		//	return false;
	}

	bool mem_cpy(uint32_t src_pid, uint64_t src_addr, uint32_t dst_pid, uint64_t dst_addr, size_t size) {
		_k_rw_request out = { src_pid, src_addr, dst_pid, dst_addr, size };

		uint64_t status = ntcomparesigninglevles(0xDEADBEEF + DRIVER_MEM_CPY, &out);
		//if (status == 0xDEADBEEF)
		//	return true;
		//else
		//	return false;
		return true;
	}

	bool mem_cpy_readonly(uint32_t src_pid, uint64_t src_addr, uint32_t dst_pid, uint64_t dst_addr, size_t size) {
		_k_rw_request out = { src_pid, src_addr, dst_pid, dst_addr, size };

		uint64_t status = ntcomparesigninglevles(0xDEADBEEF + DRIVER_CPY_TO_READONLY, &out);
		//if (status == 0xDEADBEEF)
		//	return true;
		//else
		//	return false;
		return true;
	}

	bool virtual_protect(uint32_t process_id, uintptr_t address, uint32_t protect, size_t size) {
		_k_virtual_protect out = { process_id, protect, address, size };

		uint64_t status = ntcomparesigninglevles(0xDEADBEEF + DRIVER_PROTECT, &out);
		//if (status == 0xDEADBEEF)
		//	return true;
		//else
		//	return false;
		return true;
	}

	bool get_thread(HWND window_handle, uint64_t* thread_context) {
		_k_gen_thread_ctx out{};
		out.window_handle = reinterpret_cast<uint64_t>(window_handle);
		out.thread_pointer = 10;
		out.thread_alternative = 0;

		uint64_t status = ntcomparesigninglevles(0xDEADBEEF + DRIVER_GET_THREAD, &out);
		//if (status == 0xDEADBEEF) {
		//	*thread_context = out.thread_pointer;
		//	return true;
		//}
		//else {
		//	return false;
		//}

		*thread_context = out.thread_pointer;
		return true;
	}

	bool set_thread(HWND window_handle, uint64_t thread_context) {
		_k_gen_thread_ctx out{};

		out.window_handle = reinterpret_cast<uint64_t>(window_handle);
		out.thread_pointer = thread_context;
		out.thread_alternative = thread_context;

		uint64_t status = ntcomparesigninglevles(0xDEADBEEF + DRIVER_SET_THREAD, &out);
		//if (status == 0xDEADBEEF) {
		//	return true;
		//}
		//else {
		//	return false;
		//}
		return true;
	}
}