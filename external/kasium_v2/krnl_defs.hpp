#pragma once

struct _k_get_base_module {
	char		name[256];
	uint64_t	dst;
};

struct _k_rw_request {
	uint32_t	src_pid;
	uint64_t	src_addr;
	uint32_t	dst_pid;
	uint64_t	dst_addr;
	size_t		size;
};

struct _k_virtual_alloc {
	uint32_t pid;
	uint32_t allocation_type;
	uint32_t protect;
	uint64_t addr;
	size_t size;
};

struct _k_virtual_free {
	uint32_t pid;
	uint64_t addr;
	size_t size;
	uint64_t freetype;
};

struct _k_virtual_protect {
	uint32_t pid;
	uint32_t protect;
	uint64_t addr;
	size_t size;
};

struct _k_get_um_module {
	uint32_t pid;
	WCHAR	 moduleName[256];
	uint64_t	dst_base;
	uint64_t	dst_size;
};

struct _k_secure_mem {
	uint32_t pid;
	uint64_t addr;
	size_t   size;
	uint64_t probemode;
};

struct _k_get_base_by_id {
	uint32_t pid;
	uint64_t addr;
};

//dcomp shit
struct _k_gen_thread_ctx {
	uint64_t window_handle;
	uint64_t thread_pointer;
	uint64_t thread_alternative;
};