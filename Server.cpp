#include <iostream>
#include <WinSock2.h>
#include <map>

#include "Packet.h"

using namespace std;

#pragma comment(lib, "ws2_32")

// 패킷 처리용 함수, 연결 끊기용 함수 생성
void ProcessPacket(SOCKET DataSocket);
void DisconnectPlayer(SOCKET DataSocket);

// 소켓 관리를 위해 필요한 소켓 집합체(구조체) 생성
fd_set ReadSocketList;
fd_set CopyReadSocketList;

// 세션 정보를 담기 위한 map 자료구조 생성
map<SOCKET, Player> SessionList;

int main()
{
	srand((unsigned int)(time(nullptr)));	// 랜덤 난수 활용

	WSAData wsaData;						// 소켓 준비 및 초기화 작업
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET ListenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);		// 소켓 생성

	SOCKADDR_IN ListenSockAddr = { 0 , };		// 생성 및 초기화
	ListenSockAddr.sin_family = AF_INET;		// IPv4 프로토콜
	ListenSockAddr.sin_addr.s_addr = INADDR_ANY;// 모든 IP 주소 접근 허용
	ListenSockAddr.sin_port = htons(22222);		// 포트는 22222 사용

	bind(ListenSocket, (SOCKADDR*)&ListenSockAddr, sizeof(ListenSockAddr));	// 연결 작업

	listen(ListenSocket, 5);				// 수신 대기

	FD_ZERO(&ReadSocketList);				// 소켓 집합체 초기화
	/*
	* [ FD_SET 부연 설명 ]
	* - 함수 설명
	*  : FD_SET 은 주어진 소켓 디스크립터를 소켓 집합에 추가하는 매크로입니다.
	* - 코드 설명
	*  : ReadSocketList 소켓 집합체에 ListenSocket 를 추가해서 해당 소켓의 비동기 이벤트를 감지할
	*    준비를 마칩니다. 이 코드에서는 아래에서 사용할 select 를 사용하기 위해서 합니다.
	* - 소켓 디스크립터
	*  : 네트워크 프로그래밍에서 운영체제가 소켓을 식별하고 관리하기 위한 일종의 핸들 또는 식별자
	*    일반적으로 소켓 디스크립터는 정수로 표현되며 이를 이용해서 소켓을 식별하는데 사용합니다.
	*/
	FD_SET(ListenSocket, &ReadSocketList);	// ListenSocket 모니터링 준비 작업


	TIMEVAL TimeOut;		// select 함수 사용을 위한 시간 구조체
	TimeOut.tv_sec = 0;		// 초 단위
	TimeOut.tv_usec = 10;	// 마이크로 초 단위(100 이면 0.1초)

	while (true)
	{
		// select 를 위한 Copy 소켓 집합체
		CopyReadSocketList = ReadSocketList;
		/*
		* [ select 부연 설명 ]
		* - 함수 설명
		*  : select 함수는 소켓 이벤트를 모니터링 하는 함수입니다.
		* - 코드 설명
		*  : select 함수는 주어진 소켓 집합을 변경하기 때문에 이후의 처리과정에 필요한 원본 소켓 집합체를
		*    복사하고 이후에 작업은 복사한 집합체를 이용해서 작업하기 때문에 복사한 집합체를 사용합니다.
		*/
		int ChangeSocketCount = select(0, &CopyReadSocketList, nullptr, nullptr, &TimeOut);

		// while 문을 통해 아무런 변화가 없더라도 continue 를 통해서 지속적으로 확인하게 합니다.
		if (ChangeSocketCount == 0)
		{
			continue;
		}
		else
		{
			// 변화(이벤트)가 발생시 소켓 집합체의 갯수(사이즈)만큼 순회하며 검증을 시작합니다.
			for (int i = 0; i < (int)ReadSocketList.fd_count; ++i)
			{
				/*
				* [ FD_ISSET 부연 설명 ]
				* - 함수 설명
				*  : FD_ISSET 함수는 주어진 소켓이 주어진 소켓의 집합체에 포함되어 있는지 확인하는 함수입니다.
				*    이를 통해 특정 소켓에 대한 이벤트를 감지할 수 있습니다.
				* - 코드 설명
				*  : 원본 소켓 집합체를 for 문을 통한 순회를 하며 i 번째 소켓을 CopyReadSocketList 집합체의
				*    소켓에 포함되어 있는지 확인 즉, 이벤트가 발생한 소켓이 맞는지 확인해서 맞다면 0이 아닌 값을 반환합니다.
				*/
				if (FD_ISSET(ReadSocketList.fd_array[i], &CopyReadSocketList))
				{
					// 클라이언트 요청의 처리를 위한 작업으로 이벤트가 발생한 소켓이 ListenSocket 의 소켓인지 확인합니다.
					if (ReadSocketList.fd_array[i] == ListenSocket)
					{
						// 이전과 마찬가지로 받아서 처리할 클라이언트의 소켓 주소 준비 작업입니다.
						SOCKADDR_IN ClientSockAddr = { 0 , };				// 생성 및 초기화
						int ClientSockAddrSize = sizeof(ClientSockAddr);	// accept 를 위한 동적 길이를 담은 변수 준비

						// accept 로 소켓을 수락하고 이제 해당 소켓을 FD_SET 을 통해 모니터링할 소켓 집합체에 추가합니다.
						SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientSockAddr, &ClientSockAddrSize);
						FD_SET(ClientSocket, &ReadSocketList);
					}
					else
					{
						// 위에서 소켓 추가작업이 끝난 경우 이 분기에서는 이제 가져온 소켓의 처리 과정을 시작합니다.(함수이용)
						ProcessPacket(ReadSocketList.fd_array[i]);
						//char Buffer[1024] = { 0, };
						//int RecvLength = recv(ReadSocketList.fd_array[i], Buffer, sizeof(Buffer), 0);
						//if (RecvLength <= 0)
						//{
						//	closesocket(ReadSocketList.fd_array[i]);
						//	FD_CLR(ReadSocketList.fd_array[i], &ReadSocketList);
						//}
						//else
						//{
						//	//ProcessPacket
						//	
						//	//for (int j = 0; j < (int)ReadSocketList.fd_count; ++j)
						//	//{
						//	//	int SendLength = send(ReadSocketList.fd_array[j], Buffer, RecvLength, 0);
						//	//}
						//}
					}
				}
			}
		}
	}

	closesocket(ListenSocket);

	WSACleanup();

	return 0;
}


// 받아온 소켓의 데이터를 까서 이를 처리하기 위한 함수입니다.
void ProcessPacket(SOCKET DataSocket)
{
	// 패킷의 헤더를 담아줄 char 형 배열을 준비합니다.
	// Header 에 해당하는 내용은 Packet.h 에 정의되어 있습니다.
	// 초기에 넣은 Size 와 Type 이 Header 에 해당되면 이 둘을 합치면 4 바이트이기에
	// char 형 배열 명은 Header 로, 사이즈는 4 바이트로 했습니다.
	char Header[4] = { 0, };

	// 이제 recv 를 통해서 받아온 소켓으로 부터 Header 에 내용을 담아줍니다.
	// 헤더 필요한 내용은 총 4 바이트 이기 때문에 사이즈르 4로 지정했습니다.(앞부분만 잘라서 담기)
	// 이후 MSG_WAITALL 를 통해 4바이트 데이터를 모두 받기 전까지 대기하게 합니다.
	int RecvLength = recv(DataSocket, Header, 4, MSG_WAITALL);
	if (RecvLength <= 0)
	{
		// 0 보다 작으면 전달받은 소켓의 연결을 끊어주도록 합니다.
		DisconnectPlayer(DataSocket);
		return;
	}

	// 패킷의 사이즈를 담을 변수와 타입의 대한 변수도 만들어줍니다.
	unsigned short DataSize = 0;
	EPacketType PacketType = EPacketType::Max;

	// 헤더 분해, Disassemble Header
	// 받아온 헤더의 정보를 방금 만든 변수에 담아줍니다. 이 변수를 이용해서 후처리 작업을 이어갈 겁니다.
	memcpy(&DataSize, &Header[0], 2);
	memcpy(&PacketType, &Header[2], 2);

	// 네트워크 바이트 순서를 호스트의 바이트 순서로 변환하는 작업을 합니다.(ntohs)
	// 이는 시스템에 따라 바이트 순서가 다르기 때문에 하는 작업이며 이를 해 주어야 정확한 데이터 처리가 가능합니다.
	// 즉, 받아온 패킷의 정보를 올바르게 해석하고 처리하기 위한 작업으로 우리는 클라이언트로 부터 받아오기에
	// 네트워크 바이트 순서로 변경되어 있을 것이기 때문에 받아서 처리하는 호스트에서 이 작업을 합니다.
	DataSize = ntohs(DataSize);
	PacketType = (EPacketType)(ntohs((u_short)PacketType));

	char Data[1024] = { 0, };

	// 데이터 처리, Data
	// 마찬 가지로 아래처럼 모든 Data 를 받기 위해서 MSG_WAITALL 를 통해 다 받을때가지 무한정 대기합니다.
	RecvLength = recv(DataSocket, Data, DataSize, MSG_WAITALL);
	if (RecvLength <= 0)
	{
		// 0 이하이면 아무런 작업을 하지않고 연결을 끊어주도록 합니다.
		DisconnectPlayer(DataSocket);
		return;
	}
	else if (RecvLength > 0)
	{
		// Packet Type
		// 0 보다 크면 어떠한 데이터가 되었든 받아왔기 때문에 처리를 시작합니다.
		switch (PacketType)
		{
			// [ 패킷 타입이 Login 인 경우에 대한 처리입니다. ]
			case EPacketType::C2S_Login:
			{
				// Player 구조체에 가져온 정보를 담습니다.
				Player NewPlayer;
				memcpy(&NewPlayer.ID, &Data[0], 4);	// ID 정보 담기
				memcpy(&NewPlayer.X, &Data[4], 4);	// X 좌표 정보 담기
				memcpy(&NewPlayer.Y, &Data[8], 4);	// Y 좌표 정보 담기

				// Player 가 로그인 했으니 이제 생성(Spawn)에 대한 처리입니다.
				//NewPlayer.ID = ntohl(NewPlayer.ID);
				NewPlayer.ID = (int)DataSocket;		// 소켓 고유 식별자 값을 ID와 캐릭터로 사용
				NewPlayer.X = rand() % 80;			// X 좌표 랜덤 위치 생성
				NewPlayer.Y = rand() % 15;			// Y 좌표 랜덤 위치 생성

				// 만들어진 Player 정보를 세션 정보에 담아줍니다.
				SessionList[DataSocket] = NewPlayer;

				cout << "Connect Client : " << SessionList.size() << endl;

				// S2C_Login
				// 이제 서버가 클라이언트에게 정보를 보냅니다.(리플리케이트와 비교해서 생각하면 됩니다)
				// 만들어진 Player 정보를 클라이언트에게 보내주기 위해서 패킷을 만들어서 담아주는 작업을 합니다.
				// 해당 함수는 Packet.h 에 작성되어 있으니 이를 참고하도록 합니다.
				PacketManager::PlayerData = NewPlayer;
				PacketManager::Type = EPacketType::S2C_Login;
				PacketManager::Size = 12;
				PacketManager::MakePacket(Data);

				// 이후 만들어진 패킷을 다시 클라이언트에게 전송해줍니다.
				int SendLength = send(DataSocket, Data, PacketManager::Size + 4, 0);


				// S2C_Spawn, 현재 모든 플레이어정보를 클라이언트에게 전달합니다.
				char SendData[1024];
				// 세션 리스트에 담긴 모든 클라이언트 소켓에 대한 반복문입니다.
				// 즉, 연결된 모든 클라이언트에게 작업을 해주기 위한 코드입니다.
				for (const auto& Receiver : SessionList)
				{
					// 이번에는 세션에 접속한 모든 클라이언트 정보에 대한 작업을 하기 위한 코드입니다.
					// 아래의 반복문으로 세션 리스트의 i 번째 소켓(클라이언트)에 모든 플레이어 정보를 전달합니다.
					for (const auto& Data : SessionList)
					{
						// 위의 코드의 반복으로, 플레이어 정보를 만들기 위한 코드입니다.
						PacketManager::PlayerData = Data.second;
						PacketManager::Type = EPacketType::S2C_Spawn;
						PacketManager::Size = 12;
						PacketManager::MakePacket(SendData);

						// 현재 연결된 소켓에 플레이어 정보(패킷)를 보내줍니다.
						int SendLength = send(Receiver.first, SendData, PacketManager::Size + 4, 0);

						cout << Receiver.first << " Send Spawn Client " << SendLength << endl;
					}
				}
			}
			break;

			// [ 패킷 타입이 Move 인 경우에 대한 처리입니다. ]
			case EPacketType::C2S_Move:
			{
				// 이번에도 마찬가지로 패킷을 분해해서 필요한 정보를 가져옵니다.
				Player MovePlayer;
				memcpy(&MovePlayer.ID, &Data[0], 4);
				memcpy(&MovePlayer.X, &Data[4], 4);
				memcpy(&MovePlayer.Y, &Data[8], 4);

				// 가져온 정보를 처리를 위해 바이트 순서를 정렬해서 변수에 담습니다.
				MovePlayer.ID = ntohl(MovePlayer.ID);
				MovePlayer.X = ntohl(MovePlayer.X);
				MovePlayer.Y = ntohl(MovePlayer.Y);

				// 받아온 KeyCode 를 이용해서 움직임(좌표 변경)을 구현하도록 합니다.
				int KeyCode = MovePlayer.X;

				if (KeyCode == 'W' || KeyCode == 'w')
				{
					SessionList[MovePlayer.ID].Y--;
				}
				else if (KeyCode == 'S' || KeyCode == 's')
				{
					SessionList[MovePlayer.ID].Y++;
				}
				else if (KeyCode == 'A' || KeyCode == 'a')
				{
					SessionList[MovePlayer.ID].X--;
				}
				else if (KeyCode == 'D' || KeyCode == 'd')
				{
					SessionList[MovePlayer.ID].X++;
				}

				// S2C_Move
				// 이제 변경된 Player 의 위치 정보를 패킷에 담아서 클라이언트에게 전달합니다.
				PacketManager::PlayerData = SessionList[MovePlayer.ID];
				PacketManager::Type = EPacketType::S2C_Move;
				PacketManager::Size = 12;
				PacketManager::MakePacket(Data);

				// 마찬가지로 모든 클라이언트에게 전달하기 위한 반복문입니다.
				for (const auto& Receiver : SessionList)
				{
					int SendLength = send(Receiver.first, Data, PacketManager::Size + 4, 0);
				}

			}
			break;
		}
	}
}


// 연결을 끊을 때 사용하는 함수입니다.
void DisconnectPlayer(SOCKET DataSocket)
{
	// Player 로그아웃을 처리하기 위한 Player 구조체를 생성합니다.
	// 이후 받아온 소켓의 정보를 이 소켓에 담아줍니다.
	Player LogoutPlayer;
	LogoutPlayer = SessionList[DataSocket];

	// 가장 먼저 현재 접속되어 있는 소켓의 정보를 지워줍니다.
	SessionList.erase(DataSocket);

	// 이후 반복문을 통해서 모든 클라이언트에서 동일하게 처리합니다.
	char SendData[1024];
	for (const auto& Receiver : SessionList)
	{
		PacketManager::PlayerData = LogoutPlayer;
		PacketManager::Type = EPacketType::S2C_Logout;
		PacketManager::Size = 12;
		PacketManager::MakePacket(SendData);

		int SendLength = send(Receiver.first, SendData, PacketManager::Size + 4, 0);
		cout << "Send Disconnect Client" << endl;
	}

	// 최종적으로 모니터링할 소켓 집합체에서 로그아웃한 소켓을 제거해주고 해당 소켓을 종료합니다.
	FD_CLR(DataSocket, &ReadSocketList);
	closesocket(DataSocket);
}