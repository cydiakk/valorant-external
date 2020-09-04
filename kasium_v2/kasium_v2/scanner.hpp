#pragma once

namespace scan {
	uintptr_t scan_rw_memory(uint32_t proc_id, char* pattern, char* mask);
	uintptr_t scan_rx_memory(uint32_t proc_id, char* pattern, char* mask);
}
