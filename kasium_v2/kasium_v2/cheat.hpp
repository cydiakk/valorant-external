#pragma once

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

namespace cheat {
#ifndef CHEAT
#define CHEAT
	struct LocalPlayer {
		//uintptr_t	local_player_array{};
		//uintptr_t	local_player{};
		//uintptr_t	local_ctrl{};
		uintptr_t	local_pawn{};
		uintptr_t	player_state{};

		//uintptr_t	camera_manager{};

		int32_t	team{};

		float		fov{};
		Vector3		camera_position{};
		Vector3		camera_rotation{};

		void get_camera() {
			//if (!this->camera_manager) { return; }

			//this->camera_position = driver::read<Vector3>(globals::t_proc_id, this->camera_manager + offsets::ginstance::localplayer::camera_manager::camera_position);
			//this->camera_rotation = driver::read<Vector3>(globals::t_proc_id, this->camera_manager + offsets::ginstance::localplayer::camera_manager::camera_rotation);
			//this->fov = driver::read<float>(globals::t_proc_id, this->camera_manager + offsets::ginstance::localplayer::camera_manager::camera_fov);
			magic::HijackState state = magic::read_results();
			this->camera_position = state.Position;
			this->camera_rotation = state.Rotation;
			this->fov = state.fov;
		}
		
		void get_localplayer(magic::HijackState &state/*uintptr_t gameinstance*/) {
			//this->local_player_array = driver::read<uintptr_t>(globals::t_proc_id, gameinstance + offsets::ginstance::localplayer_array);
			//this->local_player = driver::read<uintptr_t>(globals::t_proc_id, this->local_player_array);
			//this->local_ctrl = driver::read<uintptr_t>(globals::t_proc_id, this->local_player + offsets::ginstance::localplayer::local_ctrl);
			//this->local_pawn = driver::read<uintptr_t>(globals::t_proc_id, this->local_ctrl + offsets::ginstance::localplayer::local_pawn);
			//
			//if (!this->local_ctrl) { return; }

			//this->camera_manager = driver::read<uintptr_t>(globals::t_proc_id, this->local_ctrl + offsets::ginstance::localplayer::l_camera_manager);

			//this->player_state = driver::read<uintptr_t>(globals::t_proc_id, this->local_pawn + offsets::actor::a_playerstate);
			//this->team = driver::read<int32_t>(globals::t_proc_id, driver::read<uint64_t>(globals::t_proc_id, this->player_state + offsets::actor::playerstate::team_id_dref) + offsets::actor::playerstate::team_id);

			this->local_pawn = state.localpawn;
			this->player_state = state.PlayerState;
			this->team = driver::read<int32_t>(globals::t_proc_id, driver::read<uint64_t>(globals::t_proc_id, this->player_state + offsets::actor::playerstate::team_id_dref) + offsets::actor::playerstate::team_id);
		}
	};

	extern LocalPlayer localPlayer;

#pragma pack(push, 1)
	struct entityCache {		// 0x30 bytes
		uint32_t unique_id;		//4 bytes	
		uint32_t bdormant;			//4 bytes
		uint64_t playerstate;	//8 bytes
		uint64_t mesh;			//8 bytes
		uint64_t root_comp;		//8 bytes
		uint64_t dmg_ctrl;		//8 bytes
		uint64_t pobjptr;			//8 bytes
	};
#pragma pack(pop)

	struct TslEntity {
		uintptr_t	p_obj_ptr{};
		uintptr_t	root_comp{};
		uintptr_t	damage_ctrl{};
		uintptr_t	player_state{};
		uintptr_t	mesh{};
		uintptr_t	skeletal_mesh{};

		uint32_t	unique_id{};
		float		health{};
		bool		b_dormant{};
		bool		b_recently_rendered{};
		int32_t		team{};
		uint32_t	num_bones{};

		Vector3		root_position{};
		Vector3		head_position{};

		Vector3		root_position_2d{};
		Vector3		head_position_2d{};

	private:
		void get_team(uintptr_t player_state) {
			this->player_state = player_state;
			this->team = driver::read<int32_t>(globals::t_proc_id, driver::read<uint64_t>(globals::t_proc_id, player_state + offsets::actor::playerstate::team_id_dref) + offsets::actor::playerstate::team_id);
		}

		void get_mesh(uintptr_t mesh) {
			this->mesh = mesh;

			this->skeletal_mesh = driver::read<uintptr_t>(globals::t_proc_id, mesh + offsets::actor::mesh::skeletal_mesh);
			this->num_bones = driver::read<uint32_t>(globals::t_proc_id, this->skeletal_mesh + offsets::actor::mesh::bones_num);
		}

		void get_visibility(uintptr_t mesh) {
			byte b_recently_rendered = driver::read<byte>(globals::t_proc_id, mesh + offsets::actor::mesh::b_recently_rendered);
			this->b_recently_rendered = utils::IsBitSet(b_recently_rendered, 5) ? true : false;
		}

		void get_root_position(uintptr_t root_comp) {
			this->root_comp = root_comp;
		}

		void get_health(uintptr_t damage_ctrl) {
			this->damage_ctrl = damage_ctrl;
			this->health = driver::read<float>(globals::t_proc_id, damage_ctrl + offsets::actor::dmg_ctrl::health);

			if (this->health > 100)
				this->health = 100;
		}

		bool get_2d_pos(uintptr_t mesh) {
			this->head_position = engine::GetBoneWithRotation(mesh, engine::e_male_bones::Head);
			this->root_position = driver::read<Vector3>(globals::t_proc_id, root_comp + offsets::actor::root_pos);

			//if (this->head_position.z > this->root_position.z) {
			//	return false;
			//}

			//no idea why this suddenly happens :-(
			if (this->head_position.z < this->root_position.z) {
				float diff = this->root_position.z - this->head_position.z;
				float zcache = head_position.z;

				head_position.z = root_position.z + (diff - diff / 3.3f);
				root_position.z = zcache + (diff - diff / 3.3f);
			}

			this->head_position_2d = engine::WorldToScreen(this->head_position, localPlayer.camera_position, localPlayer.camera_rotation, localPlayer.fov);
			this->root_position_2d = engine::WorldToScreen(this->root_position, localPlayer.camera_position, localPlayer.camera_rotation, localPlayer.fov);

			return true;
		}

		void get_dormant(uint32_t bdormant) {
			//get the first byte the 1337 ghetto way
			this->b_dormant = (bool)(bdormant & ((1 << 8) - 1));
		}

	public:
		bool get_info(entityCache& cached) {
			if (cached.unique_id != 0x100011e) { return false; }
			this->unique_id = cached.unique_id;

			if (!utils::is_valid_addr(cached.mesh) || !utils::is_valid_addr(cached.root_comp) || !utils::is_valid_addr(cached.dmg_ctrl) || !utils::is_valid_addr(cached.playerstate)) {
				return false;
			}

			this->p_obj_ptr = cached.pobjptr;

			//mesh related
			this->get_mesh(cached.mesh);

			this->get_visibility(cached.mesh);
			this->get_root_position(cached.root_comp);

			if (!this->get_2d_pos(cached.mesh)) {
				return false;
			}

			this->get_health(cached.dmg_ctrl);
			this->get_team(cached.playerstate);
			this->get_dormant(cached.bdormant);

			return true;
		}
	};
#endif

	extern bool cheat_loop();
}