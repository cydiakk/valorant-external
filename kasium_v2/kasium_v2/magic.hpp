#pragma once

namespace magic {
	extern byte magic[];

#ifndef MAGIC_STRUCTS
#define MAGIC_STRUCTS
#pragma pack(push, 1)
	struct ArrayHeader
	{
		uint64_t Ptr;
		uint32_t Size;
	};

	struct HijackState
	{
		ArrayHeader Actors;
		uint64_t PlayerState;
		Vector3 Position;
		Vector3 Rotation;
		float fov;
		Vector3 ControlRotation;
		Vector3 WriteCtrlRotation;
		int32_t writeflag;
		uintptr_t localpawn;
	};
#pragma pack(pop)
#endif // !MAGIC_STRUCTS

	extern bool magic_scan(uintptr_t& worldcrypt_key, uintptr_t& worldcrypt_state);
	extern bool write_shell(uint64_t decrypted_world, uint64_t base, uintptr_t& pentitycache, uintptr_t& plocalproxy);
	extern HijackState read_results();
}