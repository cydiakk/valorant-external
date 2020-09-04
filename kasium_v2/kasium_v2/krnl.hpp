#pragma once

#define DRIVER_CONTINUE				0
#define DRIVER_GET_BASE				1
#define DRIVER_COPYMEMROY			2
#define DRIVER_PROTECT				3
#define DRIVER_ALLOC				4
#define DRIVER_STOP					5
#define DRIVER_GET_UM_MODULE		6
#define DRIVER_SECURE				7
#define DRIVER_GET_BASE_BY_ID		8
#define DRIVER_WRITE_TO_READONLY	9
#define DRIVER_FREE					11

#define DRIVER_SET_THREAD			12
#define DRIVER_GET_THREAD			13

namespace driver {
	bool init();
	bool stop();

	uint64_t get_process_base(const char* process_name);
	uint64_t get_process_base_by_id(uint32_t pid);
	uint64_t get_um_module(uint32_t process_id, const char* module_name, uint32_t& size);

	bool copy_memory(uint32_t src_pid, uint64_t src_addr, uint32_t dst_pid, uint64_t dst_addr, size_t size);
	bool copy_to_readonly(uint32_t src_pid, uint64_t src_addr, uint32_t dst_pid, uint64_t dst_addr, size_t size);
	bool secure_memory(uint32_t process_id, uint64_t addr, size_t size, uint64_t probemode);

	bool virtual_protect(uint32_t process_id, uintptr_t address, uint32_t protect, size_t size);
	bool virtual_free(uint32_t process_id, uintptr_t address, size_t size, uint64_t freetype);
	uint64_t virtual_alloc(uint32_t process_id, uint32_t allocation_type, uint32_t protect, size_t size);

	//dcomp shit
	void get_thread(HWND window_handle, uint64_t* thread_context);
	void set_thread(HWND window_handle, uint64_t thread_context);

	template <typename T>
	T read(const uint32_t process_id, const uintptr_t src, size_t size = sizeof(T))
	{
		T buffer;
		copy_memory(process_id, src, GetCurrentProcessId(), (uintptr_t)& buffer, size);
		return buffer;
	}

	template <typename T>
	void write(const uint32_t process_id, const uintptr_t src, const uintptr_t dst, size_t size)
	{
		copy_memory(GetCurrentProcessId(), src, process_id, dst, size);
	}

	template <typename T>	//critical
	bool write_to_readonly(const uint32_t process_id, const uintptr_t src, const uintptr_t dst, size_t size)
	{
		return copy_to_readonly(GetCurrentProcessId(), src, process_id, dst, size);
	}
}
