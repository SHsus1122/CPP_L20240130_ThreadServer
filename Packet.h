#pragma once
#ifndef __PACKET_H__
#define __PACKET_H__

#include <cstring>
#include <Winsock2.h>

// 열거형 멤버들의 사이즈는 unsigned short 로 지정했기 때문에 모두 2바이트
// 열거형 멤버에 대한 값을 의미하는 100 은 하나만 지정하면 그 뒤로는 1씩 증가한 값이 적용
// 따라서 S2C_Spawn 은 101, Max 는 108 이 됩니다.
// Max 는 해당 열거형의 끝으로 패킷 유형의 종류가 더 이상 없다는 것을 의미합니다.
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
	// 패킷의 생성시점 부터 메모리에 올려서 전역적으로 사용하기 위해서 static 사용
	static unsigned short Size;
	static Player PlayerData;
	static EPacketType Type;

	void static MakePacket(char* Buffer)
	{
		// 자료를 만드는 형태는 아래와 같습니다. [] -> 1바이트 , 우리는 배열에 쪼개서 넣은 것(형태) 처럼 만들었습니다.
		// Size Type ID       X        Y
		// [][] [][] [][][][] [][][][] [][][][]
		// htons 를 사용한 이유는 Size 의 순서가 뒤바뀌면 결과값이 바뀌기 때문
		unsigned short Data = htons(Size);		// Player 정보 Size
		memcpy(&Buffer[0], &Data, 2);			// 2바이트 짜리 만들어서 Buffer 에 넣기

		Data = htons((unsigned short)(Type));	// Player 종류 Type
		memcpy(&Buffer[2], &Data, 2);			// 2바이트 짜리 만들어서 Buffer 에 넣기

		int Data2 = htonl(PlayerData.ID);		// Player ID
		memcpy(&Buffer[4], &Data2, 4);			// 4바이트 짜리 만들어서 Buffer 에 넣기

		Data2 = htonl(PlayerData.X);			// Player X 좌표
		memcpy(&Buffer[8], &Data2, 4);			// 4바이트 짜리 만들어서 Buffer 에 넣기

		Data2 = htonl(PlayerData.Y);			// Player Y 좌표
		memcpy(&Buffer[12], &Data2, 4);			// 4바이트 짜리 만들어서 Buffer 에 넣기
	}
};

// static 으로 전역으로 사용하기 위해서 메모리에 올릴때는 초기값 설정이 필요하기에 이를 설정
unsigned short PacketManager::Size = 0;
Player PacketManager::PlayerData = { 0, };
EPacketType PacketManager::Type = EPacketType::C2S_Login;

#endif //__PACKET_H__
