#include "stdafx.hpp"

#define O_PLAYERCTRL_CTRLROTATION 0x418

namespace aimbot {
	inline float NormalizeAngle(float flAng)
	{
		if (!std::isfinite(flAng))
		{
			return 0.0f;
		}

		return std::remainder(flAng, 360.0f);
	}


	inline void ClampViewAngles(Vector3& vecAng)
	{
		vecAng.x = NormalizeAngle(vecAng.x);
		vecAng.y = NormalizeAngle(vecAng.y);
		vecAng.z = 0.0f;

		if (vecAng.x < -89.0)
		{
			vecAng.x = -89.0;
		}
		else if (vecAng.x > 89.0)
		{
			vecAng.x = 89.0;
		}
	}

	Vector3 smooth_aim(Vector3 camera_rotation, Vector3 target, float smooth_factor)
	{
		Vector3 diff = target - camera_rotation;
		diff.Normalize();

		diff.x = diff.x / smooth_factor;
		diff.y = diff.y / smooth_factor;
		diff.z = diff.z / smooth_factor;

		return camera_rotation + diff;
	}

	void rcs(Vector3 target, float smooth_factor, uintptr_t ctrlrotation_proxy, uintptr_t controlrotation) {
		// Camera 2 Control space
		Vector3 ConvertRotation = cheat::localPlayer.camera_rotation;
		ConvertRotation.Normalize();

		// Calculate recoil/aimpunch
		auto ControlRotation = driver::read<Vector3>(globals::t_proc_id, controlrotation);
		Vector3 DeltaRotation = ConvertRotation - ControlRotation;
		DeltaRotation.Normalize();

		// Remove 2x aimpunch from CameraRotation
		ConvertRotation = target - (DeltaRotation * smooth_factor);
		ConvertRotation.Normalize();

		//Smooth the whole thing
		Vector3 Smoothed = smooth_aim(cheat::localPlayer.camera_rotation, ConvertRotation, smooth_factor);

		DeltaRotation.x = DeltaRotation.x / smooth_factor;
		DeltaRotation.y = DeltaRotation.y / smooth_factor;
		DeltaRotation.z = DeltaRotation.z / smooth_factor;

		Smoothed.x -= DeltaRotation.x;
		Smoothed.y -= DeltaRotation.y;
		Smoothed.z -= DeltaRotation.z;

		driver::write<Vector3>(globals::t_proc_id, (uintptr_t)& Smoothed, ctrlrotation_proxy, sizeof(Vector3));
	}

	void aimbot(cheat::TslEntity entity, uintptr_t plocalproxy) {
		uintptr_t controlrotation = plocalproxy + 0x30;
		uintptr_t ctrlrotation_proxy = plocalproxy + 0x3C;

		if (!settings::is_ingame)
			return;

		Vector3 out;
		int bone = 8;

		Vector3 target = entity.head_position;/*engine::GetBoneWithRotation(entity.mesh, bone);*/
		Vector3 ctr_rot = driver::read<Vector3>(globals::t_proc_id, controlrotation);
		Vector3 delta = Vector3((cheat::localPlayer.camera_position.x - target.x), (cheat::localPlayer.camera_position.y - target.y), (cheat::localPlayer.camera_position.z - target.z));

		float hyp = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);

		Vector3 Rotation{};
		Rotation.x = acosf(delta.z / hyp) * (float)(RadianToURotation);
		Rotation.y = atanf(delta.y / delta.x) * (float)(RadianToURotation);
		Rotation.z = 0;

		//clamp?
		Rotation.x += 270.f;
		if (Rotation.x > 360.f) {
			Rotation.x -= 360.f;
		}
		if (delta.x >= 0.0f) {
			Rotation.y += 180.0f;
		}
		if (Rotation.y < 0.f) {
			Rotation.y += 360.f;
		}

		out.x = Rotation.x;
		out.y = Rotation.y;

		if (cheat::localPlayer.player_state && settings::is_ingame) {
			if (settings::aimbot::rcs) {
				rcs(out, settings::aimbot::smooth, ctrlrotation_proxy, controlrotation);
			}
			else {
				Vector3 smoothed = smooth_aim(cheat::localPlayer.camera_rotation, out, settings::aimbot::smooth);
				driver::write<Vector3>(globals::t_proc_id, (uintptr_t)& smoothed, ctrlrotation_proxy, sizeof(Vector3));
			}
		}
	}
}