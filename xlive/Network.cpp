#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include "Network.h"
#include "H2MOD.h"
#include "xliveless.h"
#include "CustomNetworking.h"

namespace Network
{
	namespace bitstreamptr
	{

		//DWORD write_uint = 0xD17C6;
		DWORD read_int = 0xCE49F;
		DWORD read_uint = 0xCE49F;
		DWORD read_byte = 0xCE501;
		DWORD read_char = 0xCE561;
		DWORD read_ulonglong = 0xCE5B7;
		DWORD read_block = 0xCE54F;

		DWORD write_uint = 0xCDD80;
		DWORD write_address = 0xCDED2;


	};
	char* GetPacketType(unsigned int typeval);
}


char* Network::GetPacketType(unsigned int typeval)
{
	if (typeval >= CustomNetwork::packet_count)
		return "unknown";
	auto packet_table = CustomNetwork::GetPacketTable();
	return packet_table[typeval].name;
}

typedef int(__stdcall *tWrite_uint)(DWORD *, char*, unsigned int, signed int);
tWrite_uint pWrite_uint;

typedef int(__stdcall *twrite_address)(DWORD *, char*, unsigned int);
twrite_address pwrite_address;

//typedef int(__stdcall *tRead_block)(DWORD*, char*, char*, signed int);
//tRead_block pRead_block;



int __stdcall write_uint(DWORD *thisptr, char* valuestr, unsigned int value, signed int maxval)
{


	if (memcmp(valuestr, "type", strlen(valuestr)))
		TRACE_GAME_N("bitstream::write_uint - string: %s, value: %i", valuestr, value);
	else
		TRACE_GAME_NETWORK("bitstream::write_uint::packet - %s", Network::GetPacketType(value));


	return pWrite_uint(thisptr, valuestr, value, maxval);
}

int __stdcall write_address(DWORD *thisptr, char* valuestr, unsigned int value)
{
	TRACE_GAME_N("bitstream::write_uint - string: %s, value: %08X", valuestr, value);

	return pwrite_address(thisptr, valuestr, value);
}
/**
int __stdcall read_block(DWORD* thisptr, char* valuestr, char* blockptr, signed int bitsize)
{
TRACE_GAME_N("bitstream::read_block - string: %s, ptr: %08X, bitsize: %i", valuestr, blockptr, bitsize);

int ret = pRead_block(thisptr, valuestr, blockptr, bitsize);

char* blockhex = { 0 };

signed int bytesize = bitsize / 8;

blockhex = (char*)malloc(bytesize * 2 + 1);


for (int i = 0; i < bytesize; i++)
{
_snprintf(&blockhex[i * 2], 2, "%02X", blockptr[i]);
}

TRACE_GAME_N("bitstream::read_block - hex: %s", blockhex);

blockhex[sizeof(blockhex)] = { 0 };

free(blockhex);

return ret;
}*/

/*
Sloppy CodeCave for write_uint until I can find a better way to hook.
*/
DWORD read_uint_retAddr = 0;
char* read_uint_string = 0;

int read_uint_MaxValue = 0;
int read_uint_value = 0;
DWORD thisptr = 0;
DWORD subfunc = 0xCDC35;

void read_uint_display()
{
	if (memcmp(read_uint_string, "type", strlen(read_uint_string)))
		TRACE_GAME_N("bitstream::read_uint - string: %s value: %i", read_uint_string, read_uint_value);
	else
		TRACE_GAME_NETWORK("bitstream::read_uint::packet - %s", Network::GetPacketType(read_uint_value));

	if (!memcmp(read_uint_string, "bungienet-user-role", strlen(read_uint_string)))
	{
		read_uint_value = 1;
	}

}

/* The actual caved function */
__declspec(naked) void read_bitstream_uint(void)
{

	__asm
	{
		pop read_uint_retAddr;
		mov thisptr, ecx
			mov eax, dword ptr[esp + 0x04]
			mov read_uint_string, eax
			mov eax, dword ptr[esp + 0x08]
			mov read_uint_MaxValue, eax

			push eax
			mov eax, subfunc
			call eax

			mov read_uint_value, eax

	}

	read_uint_display();

	__asm
	{
		mov ecx, thisptr
		mov eax, read_uint_value
		mov[esp + 0x08], eax
		push read_uint_retAddr
		ret
	}

}


void Network::Initialize()
{

	if (h2mod->Server)
	{
		
		Network::bitstreamptr::read_uint += h2mod->GetBase();
		Network::bitstreamptr::write_uint += h2mod->GetBase();
		Network::bitstreamptr::read_block += h2mod->GetBase();
		Network::bitstreamptr::write_address += h2mod->GetBase();

		subfunc += h2mod->GetBase();
		DWORD dwBack;



		TRACE_GAME("H2MOD::Network::bitstream - Writing uint(%08X) hook", Network::bitstreamptr::read_uint);
		Codecave(Network::bitstreamptr::read_uint, read_bitstream_uint, 5);

		TRACE_GAME("H2MOD::Network::bistream - Writing write_uint(%08X) hook", Network::bitstreamptr::write_uint);
		pWrite_uint = (tWrite_uint)DetourClassFunc((BYTE*)Network::bitstreamptr::write_uint, (BYTE*)write_uint, 11);
		VirtualProtect(pWrite_uint, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		TRACE_GAME("H2MOD::Network::Bitstream - Writing unk_int(%08X) hook", Network::bitstreamptr::write_address);
		pwrite_address = (twrite_address)DetourClassFunc((BYTE*)Network::bitstreamptr::write_address, (BYTE*)write_address, 9);
		VirtualProtect(pwrite_address, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		/*
		TRACE_GAME("H2MOD::Network::bitstream - Writing read_block(%08X) hook", Network::bitstreamptr::read_block);
		pRead_block = (tRead_block)DetourClassFunc((BYTE*)Network::bitstreamptr::read_block, (BYTE*)read_block, 8);
		VirtualProtect(pRead_block, 4, PAGE_EXECUTE_READWRITE, &dwBack);*/

		TRACE_GAME("H2MOD::Network - Initialized\n");
	}

}