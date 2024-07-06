#include <windows.h>
#include <winsock.h>
#include <fstream>
#include <thread>
#include <mutex>
#include "curl/curl.h"
#include "toml++/toml.hpp"
#include "nya_commonhooklib.h"
#include "nya_commontimer.h"

uint32_t nServerIpAddr = 0;
std::string sMasterServerAddr;
uint16_t nPort1 = 23756;
uint16_t nPort2 = 23757;
uint16_t nPort3 = 23755;
std::vector<uint32_t> aServers;
std::mutex mServerMutex;

bool bLogLobbySearch = false;
bool bLogMasterServer = false;

void WriteLog(const std::string& str) {
	static auto file = std::ofstream("FlatOutOnlinePlay_gcp.log");
	static std::mutex mutex;

	mutex.lock();
	file << str;
	file << "\n";
	file.flush();
	mutex.unlock();
}

int __stdcall sendtoHooked(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen) {
	auto addr = (sockaddr_in*)to;
	auto port = (uint16_t*)to->sa_data;
	// contact specific server ip addresses instead of broadcasting through LAN
	if (nServerIpAddr) {
		addr->sin_addr.S_un.S_addr = nServerIpAddr;
		if (bLogLobbySearch) {
			if (auto ip = inet_ntoa(addr->sin_addr))
				WriteLog("Checking for lobby at specified address " + (std::string) ip + ":" + std::to_string(ntohs(port[0])));
		}
		return sendto(s, buf, len, flags, to, tolen);
	}
	else if (!aServers.empty()) {
		if (bLogLobbySearch) {
			WriteLog("Checking for lobbies from a list of " + std::to_string(aServers.size()));
		}

		int value = 0;
		mServerMutex.lock();
		for (auto &server: aServers) {
			addr->sin_addr.S_un.S_addr = server;
			value = sendto(s, buf, len, flags, to, tolen);
		}
		mServerMutex.unlock();
		return value;
	}
	return sendto(s, buf, len, flags, to, tolen);
}
void* pSendtoHooked = (void*)&sendtoHooked;

void AddServerThread() {
	if(auto curl = curl_easy_init()) {
		curl_easy_setopt(curl, CURLOPT_URL, ("http://" + sMasterServerAddr + ":" + std::to_string(nPort3) + "/add-lobby/" + std::to_string(nPort2) + "/" + std::to_string(nPort1)).c_str());
		curl_easy_setopt(curl, CURLOPT_POST, 1);

		int err = curl_easy_perform(curl);
		if (err == CURLE_OK) {
			if (bLogMasterServer) WriteLog("Added server to masterserver list");
		}
		else {
			if (bLogMasterServer) WriteLog("Adding server to masterserver list failed, error " + std::to_string(err));
		}
		curl_easy_cleanup(curl);
	}
}

int __stdcall recvfromHooked(SOCKET s, char* buf, int len, int flags, sockaddr* from, int* fromlen) {
	auto ret = recvfrom(s, buf, len, flags, from, fromlen);
	if (ret == 106) {
		auto addr = (sockaddr_in*)from;

		auto tmp = &buf[0x18];
		*(uint32_t*)tmp = addr->sin_addr.S_un.S_addr; // set ip address in server info struct to public ip
	}
	return ret;
}
void* pRecvfromHooked = (void*)&recvfromHooked;

void PortSetterThread() {
	while (true) {
		*(int *) 0x6C02FC = nPort1;
		*(int *) 0x6C0300 = nPort2;
		Sleep(1);
	}
}

size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

void GetServerListThread() {
	while (true) {
		if (auto curl = curl_easy_init()) {
			std::string readBuffer;

			curl_easy_setopt(curl, CURLOPT_URL, ("http://" + sMasterServerAddr + ":" + std::to_string(nPort3) + "/get-lobbies/" + std::to_string(nPort2) + "/" +
												 std::to_string(nPort1)).c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

			int err = curl_easy_perform(curl);
			if (err == CURLE_OK) {
				std::stringstream ss(readBuffer);
				std::string to;

				mServerMutex.lock();
				aServers.clear();
				if (!readBuffer.empty()) {
					while(std::getline(ss,to,'\n')){
						auto addr = inet_addr(to.c_str());
						if (addr != 0 && addr != 0xFFFFFFFF) aServers.push_back(addr);
					}
				}

				if (bLogMasterServer) WriteLog("Read masterserver list, " + std::to_string(aServers.size()) + " entries found");
				mServerMutex.unlock();
			}
			else {
				if (bLogMasterServer) WriteLog("Reading masterserver list failed, error " + std::to_string(err));
			}
			curl_easy_cleanup(curl);
		}

		Sleep(3000);
	}
}

CNyaTimer BroadcastTimer;
void __fastcall AdvertiseServer(const char* data) {
	// checks done by the game before responding with lobby data
	if (!data[0xC]) return;
	if (!data[0xF]) return;
	if (!data[0x1B4] && !data[0x1B5] && !data[0x1B6] && !data[0x1B7]) return;
	if (!data[0x1B8] && !data[0x1B9] && !data[0x1BA] && !data[0x1BB]) return;

	static double broadcastInterval = 3;
	broadcastInterval += BroadcastTimer.Process();
	if (broadcastInterval > 3) {
		std::thread(AddServerThread).detach();
		broadcastInterval = 0;
	}
}

uintptr_t AdvertiseServerASM_jmp = 0x495495;
void __attribute__((naked)) AdvertiseServerASM() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, esi\n\t"
		"call %0\n\t"
		"popad\n\t"
		"mov al, [esp+0xF]\n"
		"test al, al\n\t"
		"jmp %1\n\t"
		:
		:  "i" (AdvertiseServer), "m" (AdvertiseServerASM_jmp)
	);
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x1E829F) {
				MessageBoxA(nullptr, "Unsupported game version! Make sure you're using v1.1 (.exe size of 2822144 bytes)", "nya?!~", MB_ICONERROR);
				return TRUE;
			}

			auto config = toml::parse_file("FlatOutOnlinePlay_gcp.toml");
			auto addrString = config["main"]["server_ip"].as_string();
			auto masterServerString = config["main"]["masterserver_ip"].as_string();
			nPort1 = config["main"]["port_game"].value_or(23756);
			nPort2 = config["main"]["port_lobby"].value_or(23757);
			nPort3 = config["main"]["port_masterserver"].value_or(23755);
			bLogLobbySearch = config["logging"]["log_lobby_search"].value_or(false);
			bLogMasterServer = config["logging"]["log_master_server"].value_or(false);
			if (addrString && !addrString->get().empty()) {
				nServerIpAddr = inet_addr(addrString->get().c_str());
				WriteLog("Server IP address: " + (std::string)inet_ntoa(*(in_addr*)&nServerIpAddr));
			}
			else if (masterServerString && !masterServerString->get().empty()) {
				curl_global_init(CURL_GLOBAL_ALL);
				sMasterServerAddr = masterServerString->get();
				WriteLog("Master Server IP address: " + sMasterServerAddr);
				std::thread(GetServerListThread).detach();
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x49548F, &AdvertiseServerASM);
			}

			std::thread(PortSetterThread).detach();
			NyaHookLib::Patch(0x49532E + 2, &pSendtoHooked);
			NyaHookLib::Patch(0x4954C1 + 2, &pRecvfromHooked);
			NyaHookLib::Patch<uint8_t>(0x4955BD, 0xEB); // reply back with server info to the sender's address instead of broadcast
			NyaHookLib::Patch<uint8_t>(0x494E6B, 0xEB); // disable broadcasting on the socket
		} break;
		default:
			break;
	}
	return TRUE;
}