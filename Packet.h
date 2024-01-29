#pragma once
#ifndef __PACKET_H__
#define __PACKET_H__

#include <cstring>
#include <Winsock2.h>

// ������ ������� ������� unsigned short �� �����߱� ������ ��� 2����Ʈ
// ������ ����� ���� ���� �ǹ��ϴ� 100 �� �ϳ��� �����ϸ� �� �ڷδ� 1�� ������ ���� ����
// ���� S2C_Spawn �� 101, Max �� 108 �� �˴ϴ�.
// Max �� �ش� �������� ������ ��Ŷ ������ ������ �� �̻� ���ٴ� ���� �ǹ��մϴ�.
enum class EPacketType : unsigned short
{
	C2S_Spawn = 100,
	S2C_Spawn,
	C2S_Login,
	S2C_Login,
	C2S_Logout,
	S2C_Logout,
	C2S_Move,
	S2C_Move,

	Max
};

class Player
{
public:
	int X;
	int Y;
	int ID;
};

class PacketManager
{
public:
	// ��Ŷ�� �������� ���� �޸𸮿� �÷��� ���������� ����ϱ� ���ؼ� static ���
	static unsigned short Size;
	static Player PlayerData;
	static EPacketType Type;

	void static MakePacket(char* Buffer)
	{
		// �ڷḦ ����� ���´� �Ʒ��� �����ϴ�. [] -> 1����Ʈ , �츮�� �迭�� �ɰ��� ���� ��(����) ó�� ��������ϴ�.
		// Size Type ID       X        Y
		// [][] [][] [][][][] [][][][] [][][][]
		// htons �� ����� ������ Size �� ������ �ڹٲ�� ������� �ٲ�� ����
		unsigned short Data = htons(Size);		// Player ���� Size
		memcpy(&Buffer[0], &Data, 2);			// 2����Ʈ ¥�� ���� Buffer �� �ֱ�

		Data = htons((unsigned short)(Type));	// Player ���� Type
		memcpy(&Buffer[2], &Data, 2);			// 2����Ʈ ¥�� ���� Buffer �� �ֱ�

		int Data2 = htonl(PlayerData.ID);		// Player ID
		memcpy(&Buffer[4], &Data2, 4);			// 4����Ʈ ¥�� ���� Buffer �� �ֱ�

		Data2 = htonl(PlayerData.X);			// Player X ��ǥ
		memcpy(&Buffer[8], &Data2, 4);			// 4����Ʈ ¥�� ���� Buffer �� �ֱ�

		Data2 = htonl(PlayerData.Y);			// Player Y ��ǥ
		memcpy(&Buffer[12], &Data2, 4);			// 4����Ʈ ¥�� ���� Buffer �� �ֱ�
	}
};

// static ���� �������� ����ϱ� ���ؼ� �޸𸮿� �ø����� �ʱⰪ ������ �ʿ��ϱ⿡ �̸� ����
unsigned short PacketManager::Size = 0;
Player PacketManager::PlayerData = { 0, };
EPacketType PacketManager::Type = EPacketType::C2S_Login;

#endif //__PACKET_H__
