#include "Globals.h"
#include <h2mod.pb.h>
#include "MapChecksumSync.h"

using namespace std; 

time_t start2 = time(0);

std::string EMPTY_STR2("");

void startGameThread() {
	while (1) {
		if (!gameManager->isHost()) {
			if (!network->queuedNetworkCommands.empty()) {
				std::deque<std::string>::iterator it = network->queuedNetworkCommands.begin();
				std::string command = *it;
				INT32 len = *reinterpret_cast<const INT32*>(command.c_str());
				network->queuedNetworkCommands.pop_front();
				if (len > 0) {
					H2ModPacket recvpak;
					recvpak.ParseFromArray(command.c_str(), len);
					if (recvpak.has_type()) {
						switch (recvpak.type()) {
						case H2ModPacket_Type_set_player_team:
							TRACE_GAME("[h2mod-network] Received change player team packet");
							if (recvpak.has_h2_set_player_team()) {
								BYTE TeamIndex = recvpak.h2_set_player_team().team();
								TRACE_GAME("[h2mod-network] Got a set team request from server! TeamIndex: %i", TeamIndex);
								h2mod->set_local_team_index(TeamIndex);
							}
							break;
						case H2ModPacket_Type_set_unit_grenades:
							TRACE_GAME("[h2mod-network] Received change player grenades packet");
							if (recvpak.has_set_grenade())
							{
								//protobuf has some weird bug where passing 0 has type in the current set_grenade packet definition
								//completely breaks on the serialization side, https://groups.google.com/forum/#!topic/protobuf/JsougkaRcw4
								//no idea why, so we always subtract 1 here (since we added 1 before save)
								int type = recvpak.set_grenade().type() - 1;
								int count = recvpak.set_grenade().count();
								int pIndex = recvpak.set_grenade().pindex();
								TRACE_GAME("[h2mod-network] grenades packet info type=%d, count=%d, pIndex=%d", type, count, pIndex);
								h2mod->set_local_grenades(type, count, pIndex);
							}
							break;
						case H2ModPacket_Type_map_info_request:
							TRACE_GAME("[h2mod-network] Received map info packet");
							if (recvpak.has_map_info()) {
								std::string mapFilename = recvpak.map_info().mapfilename();
								if (!mapFilename.empty()) {
									TRACE_GAME_N("[h2mod-network] map file name from packet %s", mapFilename.c_str());
									if (!mapManager->hasCustomMap(mapFilename)) {
										//TODO: set map filesize
										//TODO: if downloading from repo files, try p2p
										mapManager->downloadFromRepo(mapFilename);
									}
									else {
										TRACE_GAME_N("[h2mod-network] already has map %s", mapFilename.c_str());
									}
								}
							}
							break;
						case H2ModPacket_Type_set_lobby_settings:
							TRACE_GAME("[h2mod-network] Received lobby settings packet");
							if (recvpak.has_lobby_settings()) {
								advLobbySettings->parseLobbySettings(recvpak.lobby_settings());
							}
							break;
						case H2ModPacket_Type_map_checksum_state_sync:
							if (recvpak.has_checksum()) {
								MapChecksumSync::HandlePacket(recvpak);
							}
							break;
						default:
							//TODO: log error
							break;
						}
					}
				}
			}

			std::string mapFilenameToDownload = mapManager->getMapFilenameToDownload();
			if (!mapFilenameToDownload.empty()) {
				TRACE_GAME_N("[h2mod-network] map file name from membership packet %s", mapFilenameToDownload.c_str());
				if (!mapManager->hasCustomMap(mapFilenameToDownload)) {
					//TODO: set map filesize
					//TODO: if downloading from repo files, try p2p
					mapManager->downloadFromRepo(mapFilenameToDownload);
				}
				else {
					TRACE_GAME_N("[h2mod-network] already has map %s", mapFilenameToDownload.c_str());
				}
				mapManager->setMapFileNameToDownload(EMPTY_STR2);
			}
		}
		std::this_thread::sleep_for(1s);
	}
}

bool GameManager::isHost() {
	if (h2mod->Server) {
		return true;
	}
	int packetDataObj = (*(int*)(h2mod->GetBase() + 0x420FE8));
	DWORD isHostByteValue = *(DWORD*)(packetDataObj + 29600);
	return isHostByteValue == 5 || isHostByteValue == 6 || isHostByteValue == 7 || isHostByteValue == 8;
}

bool GameManager::start() {
	if (!this->started) {
		std::thread t1(startGameThread);
		t1.detach();
		this->started = true;
	}
	return this->started;
}