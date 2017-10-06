#include "CustomNetworking.h"
#include "Windows.h"

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