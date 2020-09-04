#include "stdafx.hpp"

namespace scan {
	uintptr_t scan_rw_memory(uint32_t proc_id, char* pattern, char* mask) {
		HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION, false, proc_id);

		if (h_process == INVALID_HANDLE_VALUE) {
			return 0;
		}

		//ready to mem scan
		MEMORY_BASIC_INFORMATION mbi = { 0 };
		SYSTEM_INFO sysInfo = { 0 };

		GetSystemInfo(&sysInfo);

		uint64_t mem_region_start = (uint64_t)sysInfo.lpMinimumApplicationAddress;
		uint64_t mem_region_end = (uint64_t)sysInfo.lpMaximumApplicationAddress;

		do {
			if (VirtualQueryEx(h_process, (LPCVOID)mem_region_start, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) != 0) {
				//only check commited pages that are accessable
				if ((mbi.State == MEM_COMMIT) && ((mbi.Protect & PAGE_GUARD) == 0) && ((mbi.Protect & PAGE_NOACCESS) == 0)) {
					//only check writeable pages
					if ((mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_WRITECOPY)) {
						auto dump = new unsigned char[mbi.RegionSize + 1];
						memset(dump, 0x00, mbi.RegionSize + 1);

						//dump region
						if (!driver::copy_memory(proc_id, (uint64_t)mbi.BaseAddress, GetCurrentProcessId(), (uint64_t)dump, mbi.RegionSize)) {
							mem_region_start += mbi.RegionSize;
							continue;
						}

						//scan
						auto potentialptr = utils::scanPattern(dump, mbi.RegionSize, pattern, mask);
						if (utils::is_valid_addr(potentialptr)) {
							return ((uintptr_t)mbi.BaseAddress + (potentialptr - (uintptr_t)dump));
						}

						//cuz we are clean boyz
						delete[] dump;
					}
				}
				mem_region_start += mbi.RegionSize;
			}
			else {
				mem_region_start += mbi.RegionSize;
			}
		} while (mem_region_start <= mem_region_end);
		return 0;
	}

	uintptr_t scan_rx_memory(uint32_t proc_id, char* pattern, char* mask) {
		HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION, false, proc_id);

		if (h_process == INVALID_HANDLE_VALUE) {
			return 0;
		}

		//ready to mem scan
		MEMORY_BASIC_INFORMATION mbi = { 0 };
		SYSTEM_INFO sysInfo = { 0 };

		GetSystemInfo(&sysInfo);

		uint64_t mem_region_start = (uint64_t)sysInfo.lpMinimumApplicationAddress;
		uint64_t mem_region_end = (uint64_t)sysInfo.lpMaximumApplicationAddress;

		do {
			if (VirtualQueryEx(h_process, (LPCVOID)mem_region_start, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) != 0) {
				//only check commited pages that are accessable
				if ((mbi.State == MEM_COMMIT) && ((mbi.Protect & PAGE_GUARD) == 0) && ((mbi.Protect & PAGE_NOACCESS) == 0)) {
					//only check writeable pages
					if ((mbi.Protect == PAGE_EXECUTE_READ)) {
						auto dump = new unsigned char[mbi.RegionSize + 1];
						memset(dump, 0x00, mbi.RegionSize + 1);

						//dump region
						if (!driver::copy_memory(proc_id, (uint64_t)mbi.BaseAddress, GetCurrentProcessId(), (uint64_t)dump, mbi.RegionSize)) {
							mem_region_start += mbi.RegionSize;
							continue;
						}

						//scan
						auto potentialptr = utils::scanPattern(dump, mbi.RegionSize, pattern, mask);
						if (utils::is_valid_addr(potentialptr)) {
							return ((uintptr_t)mbi.BaseAddress + (potentialptr - (uintptr_t)dump));
						}

						//cuz we are clean boyz
						delete[] dump;
					}
				}
				mem_region_start += mbi.RegionSize;
			}
			else {
				mem_region_start += mbi.RegionSize;
			}
		} while (mem_region_start <= mem_region_end);
		return 0;
	}
}