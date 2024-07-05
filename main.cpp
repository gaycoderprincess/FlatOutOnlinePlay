#include <windows.h>
#include <winsock.h>
#include <wininet.h>
#include <fstream>
#include <thread>
#include "toml++/toml.hpp"
#include "nya_commonhooklib.h"

uint32_t nServerIpAddr = 0;
uint16_t nPort1 = 23756;
uint16_t nPort2 = 23757;

bool bLogLobbySearch = false;

void WriteLog(const std::string& str) {
	static auto file = std::ofstream("FlatOutOnlinePlay_gcp.log");
	file << str;
	file << "\n";
	file.flush();
}

int __stdcall sendtoHooked(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen) {
	auto addr = (sockaddr_in*)to;
	auto port = (uint16_t*)to->sa_data;
	if (nServerIpAddr) addr->sin_addr.S_un.S_addr = nServerIpAddr; // contact the specific server ip address instead of broadcasting through LAN
	if (bLogLobbySearch) {
		if (auto ip = inet_ntoa(addr->sin_addr))
			WriteLog("Checking for lobby at " + (std::string) ip + ":" + std::to_string(ntohs(port[0])));
	}
	return sendto(s, buf, len, flags, to, tolen);
}
void* pSendtoHooked = (void*)&sendtoHooked;

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

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x1E829F) {
				MessageBoxA(nullptr, "Unsupported game version! Make sure you're using v1.1 (.exe size of 2822144 bytes)", "nya?!~", MB_ICONERROR);
				return TRUE;
			}

			auto config = toml::parse_file("FlatOutOnlinePlay_gcp.toml");
			auto addrString = config["main"]["server_ip"].as_string();
			nPort1 = config["main"]["port_game"].value_or(23756);
			nPort2 = config["main"]["port_lobby"].value_or(23757);
			bLogLobbySearch = config["logging"]["log_lobby_search"].value_or(false);
			if (addrString && !addrString->get().empty()) {
				nServerIpAddr = inet_addr(addrString->get().c_str());
				WriteLog("Server IP address: " + (std::string)inet_ntoa(*(in_addr*)&nServerIpAddr));
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