#include <enet/enet.h>
#include <iostream>
#include <fstream>
#include <windows.h> 
#include <stdio.h> 
#include <string> 

using namespace std;


ENetHost * server;
ENetHost * client;
ENetPeer * client_peer;

int login_user = 0;
int login_token = 0;
string login_doorid;
int login_lmode = 0;
string login_packet_str;
unsigned char login_packet[1024];
unsigned int login_packet_length = 0;

bool doesPacketRedirect = false;
bool isRedirectInProcess = false;
bool isPacketLoginRequest = false;

char* GetTextPointerFromPacket(ENetPacket* packet)
{
	char zero = 0;
	memcpy(packet->data + packet->dataLength - 1, &zero, 1);
	return (char*)(packet->data + 4);
}

void connectClient(string address, int port) {
	client = enet_host_create(NULL /* create a client host */,
		1 /* only allow 1 outgoing connection */,
		2 /* allow up 2 channels to be used, 0 and 1 */,
		0 /* 56K modem with 56 Kbps downstream bandwidth */,
		0 /* 56K modem with 14 Kbps upstream bandwidth */);
	if (client == NULL)
	{
		cout << "An error occurred while trying to create an ENet client host.\n";
		exit(EXIT_FAILURE);
	}

	ENetAddress client_address;

	client->checksum = enet_crc32;
	enet_host_compress_with_range_coder(client);
	enet_address_set_host(&client_address, address.c_str());
	client_address.port = port;
	/* Initiate the connection, allocating the two channels 0 and 1. */
	client_peer = enet_host_connect(client, &client_address, 2, 0);
	if (client_peer == NULL)
	{
		cout << "No available peers for initiating an ENet connection.\n";
		exit(EXIT_FAILURE);
	}

	enet_host_flush(client);
}

void OnSendToServer(string address, int port, int userId, int token, int lmode, string doorid)
{
	cout << "Redirecting to " << address << ":" << to_string(port) << " with user id " << to_string(userId) << " and login token " << to_string(token) << endl;

	login_user = userId;
	login_lmode = lmode;
	login_doorid = doorid;
	if (token != -1) {
		login_token = token;
	}
	connectClient(address, port);
	doesPacketRedirect = true;
	isRedirectInProcess = true;
}
struct OnSendToServerStruct
{
	string address;
	int port;
	int userId;
	int token;
	string doorid;
	int lmode;
};

BYTE* GetExtendedDataPointerFromTankPacket(BYTE* a1)
{
	return (BYTE*)((int)(a1 + 56));
}

BYTE* GetStructPointerFromTankPacket(ENetPacket* packet)
{
	unsigned int packetLenght = packet->dataLength;
	BYTE* result = NULL;
	if (packetLenght >= 0x3C)
	{
		BYTE* packetData = packet->data;
		result = packetData + 4;
		if (*(BYTE*)(packetData + 16) & 8)
		{
			if (packetLenght < *(int*)(packetData + 56) + 60)
			{
				result = 0;
			}
		}
		else
		{
			int zero = 0;
			memcpy(packetData + 56, &zero, 4);
		}
	}
	return result;
}

int GetMessageTypeFromPacket(ENetPacket* packet)
{
	int result;

	if (packet->dataLength > 3u)
	{
		result = *(packet->data);
	}
	else
	{
		result = 0;
	}
	return result;
}

void SerializeFromMem(byte *pSrc, int bufferSize, int *pBytesReadOut, int netId)
{
	string action = "";
	BYTE* dataStruct = NULL;
	byte* startPtr = pSrc;
	byte *pStartPos = pSrc;
	byte count = pSrc[0]; pSrc++;
	bool isNetIdHandled = false;
	for (int i = 0; i < count; i++)
	{
		byte index = pSrc[0]; pSrc++;
		byte type = pSrc[0]; pSrc++;
		switch (type)
		{
		case 2:
		{
			int strLen;
			memcpy(&strLen, pSrc, 4); pSrc += 4;

			string v;
			v.resize(strLen);
			memcpy(&v[0], pSrc, strLen); pSrc += strLen;
			if (index == 0)
			{
				action = v;
				if (action == "OnSendToServer")
				{
					dataStruct = (BYTE*)new OnSendToServerStruct;
				}
			}
			if (action == "OnSendToServer" && index == 4)
			{
				((OnSendToServerStruct*)dataStruct)->address = v.substr(0,v.find("|"));
				((OnSendToServerStruct*)dataStruct)->doorid = v.substr(v.find("|") + 1);
			}
			break;
		}

		case 5:
		{
			pSrc += sizeof(int);
			break;
		}
		case 9:
		{
			int v;
			memcpy(&v, pSrc, sizeof(int));
			pSrc += sizeof(int);
			if (action == "OnSendToServer" && index == 1)
			{
				((OnSendToServerStruct*)dataStruct)->port = v;
			}
			else if (action == "OnSendToServer" && index == 2)
			{
				((OnSendToServerStruct*)dataStruct)->token = v;
			}
			else if (action == "OnSendToServer" && index == 3)
			{
				((OnSendToServerStruct*)dataStruct)->userId = v;
			}
			else if (action == "OnSendToServer" && index == 5)
			{
				((OnSendToServerStruct*)dataStruct)->lmode = v;
			}
			break;
		}

		case 1:
		{
			pSrc += sizeof(float);
			break;
		}

		case 3:
		{
			pSrc += sizeof(float);
			pSrc += sizeof(float);
			break;
		}

		case 4:
		{
			pSrc += sizeof(float);
			pSrc += sizeof(float);
			pSrc += sizeof(float);
			break;
		}

		case 8:
		{
			return;
		}

		default:
			return;
		}
	}
	if (action == "OnSendToServer")
	{
		OnSendToServer(((OnSendToServerStruct*)dataStruct)->address, ((OnSendToServerStruct*)dataStruct)->port, ((OnSendToServerStruct*)dataStruct)->userId, ((OnSendToServerStruct*)dataStruct)->token, ((OnSendToServerStruct*)dataStruct)->lmode, ((OnSendToServerStruct*)dataStruct)->doorid);
		cout << "Redirecting to " << ((OnSendToServerStruct*)dataStruct)->address << ":" << to_string(((OnSendToServerStruct*)dataStruct)->port) << " with user id " << to_string(((OnSendToServerStruct*)dataStruct)->userId) << " and login token " << to_string(((OnSendToServerStruct*)dataStruct)->token) << endl;
	}
	if (dataStruct != NULL)
	{
		free(dataStruct);
	}
}
void ProcessTankUpdatePacket(BYTE* structPointer) {
	if ((*(char*)structPointer) == 1) {
		try {
			SerializeFromMem(GetExtendedDataPointerFromTankPacket(structPointer), *(int*)(structPointer + 52), 0, *(int*)(structPointer + 4));
		}
		catch (int e) {

		}
	}
}

void SendPacket(int a1, string a2, ENetPeer* enetPeer)
{
	if (enetPeer)
	{
		ENetPacket* v3 = enet_packet_create(0, a2.length() + 5, 1);
		memcpy(v3->data, &a1, 4);
		//*(v3->data) = (DWORD)a1;
		memcpy((v3->data) + 4, a2.c_str(), a2.length());

		//cout << std::hex << (int)(char)v3->data[3] << endl;
		enet_peer_send(enetPeer, 0, v3);
	}
}

ofstream stm_auto;
void onLoginRequested() {
	string token;
	if (!login_user && !login_token) {
		token = "";
	}
        else if (login_doorid != "") {
		token = "\nuser|" + std::to_string(login_user) + "\ntoken|" + std::to_string(login_token) + "\ndoorID|" + login_doorid + '\n';
	}
	else {
		token = "\nuser|" + std::to_string(login_user) + "\ntoken|" + std::to_string(login_token) + '\n';
	}
	int lmodefix = login_packet_str.find("lmode|");
	string stringlmode = to_string(login_lmode);
	login_packet_str[lmodefix + 6] = stringlmode[0]; // Change lmode|0 in login packet to our lmode from server.
	string packStr = login_packet_str + token;
	int packetLen = login_packet_length + token.length();
	SendPacket(2, packStr, client_peer);
	//enet_packet_destroy(loginPacket);
	stm_auto.write((char*)login_packet, packetLen);
	stm_auto.flush();
	isRedirectInProcess = false;
	isPacketLoginRequest = true;
	cout << "Redirected." << endl;
}

void captureLoginPacket(ENetEvent* event) {
	login_packet_str = GetTextPointerFromPacket(event->packet);
	stm_auto.write("orig:", 5);
	stm_auto.write((char*)login_packet, login_packet_length);
	stm_auto.flush();
}

void ProcessPacket(ENetEvent* event, ENetPeer* peer)
{
	doesPacketRedirect = false;
	isPacketLoginRequest = false;
	BYTE sender = ((BYTE*)peer->data)[0];
	if (sender == 2) { // client
		if (login_packet_length == 0 && (int)(event->packet->data[0]) == 2) {
			captureLoginPacket(event);
		}
	}
	else if (sender == 1) { // server
		int messageType = GetMessageTypeFromPacket(event->packet);
		if (messageType == 1 && isRedirectInProcess) {
			onLoginRequested();
			return;
		}
		if (messageType == 4)
		{
			BYTE* tankUpdatePacket = GetStructPointerFromTankPacket(event->packet);
			if (tankUpdatePacket)
			{
				ProcessTankUpdatePacket(tankUpdatePacket);
			}
		}
	}
}
BOOL WINAPI consoleHandler(DWORD signal) {

	if (signal == CTRL_C_EVENT) {
		printf("Ctrl-C handled\n"); // do cleanup
		exit(0);
	}

	return TRUE;
}
int main(int argc, char ** argv)
{

	if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
		printf("\nERROR: Could not set control handler");
	}
	if (enet_initialize() != 0)
	{
		std::cout << "An error occurred while initializing ENet.\n";
		return EXIT_FAILURE;
	}
	atexit(enet_deinitialize);
	ENetAddress address;
	/* Bind the server to the default localhost.     */
	/* A specific host address can be specified by   */
	/* enet_address_set_host (& address, "x.x.x.x"); */
	enet_address_set_host(&address, "127.0.0.1");
	/* Bind the server to port 1234. */
	address.port = 17125;
	server = enet_host_create(&address /* the address to bind the server host to */,
		1      /* allow up to 32 clients and/or outgoing connections */,
		2      /* allow up to 2 channels to be used, 0 and 1 */,
		0      /* assume any amount of incoming bandwidth */,
		0      /* assume any amount of outgoing bandwidth */);
	if (server == NULL)
	{
		fprintf(stderr,
			"An error occurred while trying to create an ENet server host.\n");
		exit(EXIT_FAILURE);
	}
	client = enet_host_create(NULL /* create a client host */,
		1 /* only allow 1 outgoing connection */,
		2 /* allow up 2 channels to be used, 0 and 1 */,
		0 /* assume any amount of incoming bandwidth */,
		0 /* assume any amount of outgoing bandwidth */);
	if (client == NULL)
	{
		fprintf(stderr,
			"An error occurred while trying to create an ENet client host.\n");
		exit(EXIT_FAILURE);
	}
	server->checksum = enet_crc32;
	enet_host_compress_with_range_coder(server);
	
	client->checksum = enet_crc32;
	enet_host_compress_with_range_coder(client);

	//client set host
	ENetAddress client_address;
	enet_address_set_host(&client_address, "209.59.191.86");
	client_address.port = 17093;
	
	/* Wait up to 1000 milliseconds for an event. */
	stm_auto.open("stm_auto", std::ofstream::out | std::ofstream::binary);
	std::ofstream stm_client;
	stm_client.open("stm_client", std::ofstream::out | std::ofstream::binary);
	std::ofstream stm_server;
	stm_server.open("stm_server", std::ofstream::out | std::ofstream::binary);
	ENetPeer *server_peer;
	bool connected = false;
	bool server_connected = false;
	std::cout << "Initialized!" << std::endl;
	while (true) {
		if (connected) {
			if (!server_connected) {
				client_peer = enet_host_connect(client, &client_address, 2, 0);

				if (client_peer == NULL)
				{
					std::cout << "No available peers for initiating an ENet connection.\n";
					exit(EXIT_FAILURE);
				}

				enet_host_flush(client);
				server_connected = true;
			}
			else {
				ENetEvent event;
				while (enet_host_service(client, &event, 0) > 0)
				{
					switch (event.type)
					{
					case ENET_EVENT_TYPE_CONNECT:
						printf("Successfully connected to Growtopia Server (%x:%u).\n",
							event.peer->address.host,
							event.peer->address.port);
						client_peer->data = malloc(16);
						((BYTE*)client_peer->data)[0] = 1;
						continue;
					case ENET_EVENT_TYPE_RECEIVE:
						stm_server.write("!PACKHDR!", 9);
						stm_server.write((char *)event.packet->data, event.packet->dataLength);
						ProcessPacket(&event, event.peer);
						if (isPacketLoginRequest)
							continue;
						if (!doesPacketRedirect)
							enet_peer_send(server_peer, 0, event.packet);
						//enet_packet_destroy(event.packet);
						continue;

					case ENET_EVENT_TYPE_DISCONNECT:
						cout << "Server disconnected." << endl;
						/* Reset the peer's client information. */
						event.peer->data = NULL;
					}
				}
			}
		}
		ENetEvent event;
		while (enet_host_service(server, &event, 0) > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				printf("A new client connected from %x:%u.\n",
					event.peer->address.host,
					event.peer->address.port);
				server_peer = event.peer;
				server_peer->data = malloc(16);
				((BYTE*)server_peer->data)[0] = 2;
				connected = true;
				continue;
			case ENET_EVENT_TYPE_RECEIVE:
				/* Clean up the packet now that we're done using it. */
				stm_client.write("!PACKHDR!", 9);
				stm_client.write((char *)event.packet->data, event.packet->dataLength);
				ProcessPacket(&event, event.peer);
				if (isPacketLoginRequest)
					continue;
				if (!doesPacketRedirect)
					enet_peer_send(client_peer, 0, event.packet);
				//enet_packet_destroy(event.packet);
				continue;

			case ENET_EVENT_TYPE_DISCONNECT:
				cout << "Client disconnected." << endl;
				/* Reset the peer's client information. */
				event.peer->data = NULL;
			}
		}
	}
	stm_client.close();
	stm_server.close();



}
