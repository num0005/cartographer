#include "CustomNetworking.h"
#include "Windows.h"
#include "H2MOD.h"

NetworkPacket packet_table[CustomNetwork::packet_type::packet_count];

NetworkPacket *CustomNetwork::Register_Packet(NetworkPacket *packet_table,
	int id,
	char *name,
	int min,
	int max,
	PacketGenerator generator,
	PacketHandler handler,
	PacketUnknownFunction unk2,
	unsigned int unk1)
{
	NetworkPacket *packet = &packet_table[id];
	packet->name = name;
	packet->unk1 = unk1;
	packet->min = min;
	packet->max = max;
	packet->generate_packet = generator;
	packet->handle_packet = handler;
	packet->unk2 = unk2;
	packet->is_initialized = 1;
	return packet;
}

NetworkPacket *CustomNetwork::GetPacketTable()
{
	return packet_table;
}

void CustomNetwork::ClearPacketTable()
{
	ZeroMemory(packet_table, sizeof(NetworkPacket) * 49);
}

void CustomNetwork::SendPacket(packet_type id, void *data, int size, int packet_id )
{
	int get_packet_offset = 0x1AC81D;
	int send_packet_offset = 0x1BB3E7;
	if (h2mod->Server)
	{
		get_packet_offset = 0x1AC81D;
		send_packet_offset = 0x1BB3E7;
	}

	typedef void *(__cdecl *get_packet)(int id);
	get_packet get_packet_impl = reinterpret_cast<get_packet>(h2mod->GetBase() + get_packet_offset);
	typedef int(__thiscall *send_packet)(void *packet, unsigned int type, unsigned int size, void *data_obj);
	send_packet send_packet_impl = reinterpret_cast<send_packet>(h2mod->GetBase() + send_packet_offset);

	void *packet = get_packet_impl(0);
	send_packet_impl(packet, id, size, data);
}