#pragma once
typedef bool(__cdecl *PacketGenerator)(int a1, int size, void *data);
typedef bool(__cdecl *PacketHandler)(int a1, int a2, int a3);
typedef int(__cdecl *PacketUnknownFunction)(int a1, int a2, int a3);
struct NetworkPacket
{
	unsigned int is_initialized;
	char *name;
	unsigned int unk1;
	unsigned int min;
	unsigned int max;
	PacketGenerator generate_packet;
	PacketHandler handle_packet;
	PacketUnknownFunction unk2;
};

static_assert(sizeof(NetworkPacket) == 0x20, "Bad NetworkPacket size.");

class CustomNetwork {
public:
	enum packet_type
	{
		ping,
		pong,
		broadcast_search,
		broadcast_reply,
		connect_request,
		connect_refuse,
		connect_establish,
		connect_closed,
		join_request,
		join_abort,
		join_refuse,
		leave_session,
		leave_acknowledge,
		session_disband,
		session_boot,
		host_handoff,
		peer_handoff,
		host_transition,
		host_reestablish,
		host_decline,
		peer_reestablish,
		peer_establish,
		election,
		election_refuse,
		time_synchronize,
		membership_update,
		peer_properties,
		delegate_leadership,
		boot_machine,
		player_add,
		player_refuse,
		player_remove,
		player_properties,
		parameters_update,
		parameters_request,
		countdown_timer,
		mode_acknowledge,
		virtual_couch_update,
		virtual_couch_request,
		vote_update,
		view_establishment,
		player_acknowledge,
		synchronous_update,
		synchronous_actions,
		synchronous_join,
		synchronous_gamestate,
		game_results,
		text_chat,
		test,
		leave_session_new_test,
		packet_count
	};
	static_assert(packet_count <= 0xFF, "Too many packets registered");
	static NetworkPacket *Register_Packet(NetworkPacket *packet_table,
		int id,
		char *name,
		int min,
		int max,
		PacketGenerator generator,
		PacketHandler handler,
		PacketUnknownFunction unk2 = nullptr,
		unsigned int unk1 = 0
		);
	static NetworkPacket *GetPacketTable();
	static void ClearPacketTable();
};