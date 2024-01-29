#include <iostream>
#include <WinSock2.h>
#include <map>

#include "Packet.h"

using namespace std;

#pragma comment(lib, "ws2_32")

// ��Ŷ ó���� �Լ�, ���� ����� �Լ� ����
void ProcessPacket(SOCKET DataSocket);
void DisconnectPlayer(SOCKET DataSocket);

// ���� ������ ���� �ʿ��� ���� ����ü(����ü) ����
fd_set ReadSocketList;
fd_set CopyReadSocketList;

// ���� ������ ��� ���� map �ڷᱸ�� ����
map<SOCKET, Player> SessionList;

int main()
{
	srand((unsigned int)(time(nullptr)));	// ���� ���� Ȱ��

	WSAData wsaData;						// ���� �غ� �� �ʱ�ȭ �۾�
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET ListenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);		// ���� ����

	SOCKADDR_IN ListenSockAddr = { 0 , };		// ���� �� �ʱ�ȭ
	ListenSockAddr.sin_family = AF_INET;		// IPv4 ��������
	ListenSockAddr.sin_addr.s_addr = INADDR_ANY;// ��� IP �ּ� ���� ���
	ListenSockAddr.sin_port = htons(22222);		// ��Ʈ�� 22222 ���

	bind(ListenSocket, (SOCKADDR*)&ListenSockAddr, sizeof(ListenSockAddr));	// ���� �۾�

	listen(ListenSocket, 5);				// ���� ���

	FD_ZERO(&ReadSocketList);				// ���� ����ü �ʱ�ȭ
	/*
	* [ FD_SET �ο� ���� ]
	* - �Լ� ����
	*  : FD_SET �� �־��� ���� ��ũ���͸� ���� ���տ� �߰��ϴ� ��ũ���Դϴ�.
	* - �ڵ� ����
	*  : ReadSocketList ���� ����ü�� ListenSocket �� �߰��ؼ� �ش� ������ �񵿱� �̺�Ʈ�� ������
	*    �غ� ��Ĩ�ϴ�. �� �ڵ忡���� �Ʒ����� ����� select �� ����ϱ� ���ؼ� �մϴ�.
	* - ���� ��ũ����
	*  : ��Ʈ��ũ ���α׷��ֿ��� �ü���� ������ �ĺ��ϰ� �����ϱ� ���� ������ �ڵ� �Ǵ� �ĺ���
	*    �Ϲ������� ���� ��ũ���ʹ� ������ ǥ���Ǹ� �̸� �̿��ؼ� ������ �ĺ��ϴµ� ����մϴ�.
	*/
	FD_SET(ListenSocket, &ReadSocketList);	// ListenSocket ����͸� �غ� �۾�


	TIMEVAL TimeOut;		// select �Լ� ����� ���� �ð� ����ü
	TimeOut.tv_sec = 0;		// �� ����
	TimeOut.tv_usec = 10;	// ����ũ�� �� ����(100 �̸� 0.1��)

	while (true)
	{
		// select �� ���� Copy ���� ����ü
		CopyReadSocketList = ReadSocketList;
		/*
		* [ select �ο� ���� ]
		* - �Լ� ����
		*  : select �Լ��� ���� �̺�Ʈ�� ����͸� �ϴ� �Լ��Դϴ�.
		* - �ڵ� ����
		*  : select �Լ��� �־��� ���� ������ �����ϱ� ������ ������ ó�������� �ʿ��� ���� ���� ����ü��
		*    �����ϰ� ���Ŀ� �۾��� ������ ����ü�� �̿��ؼ� �۾��ϱ� ������ ������ ����ü�� ����մϴ�.
		*/
		int ChangeSocketCount = select(0, &CopyReadSocketList, nullptr, nullptr, &TimeOut);

		// while ���� ���� �ƹ��� ��ȭ�� ������ continue �� ���ؼ� ���������� Ȯ���ϰ� �մϴ�.
		if (ChangeSocketCount == 0)
		{
			continue;
		}
		else
		{
			// ��ȭ(�̺�Ʈ)�� �߻��� ���� ����ü�� ����(������)��ŭ ��ȸ�ϸ� ������ �����մϴ�.
			for (int i = 0; i < (int)ReadSocketList.fd_count; ++i)
			{
				/*
				* [ FD_ISSET �ο� ���� ]
				* - �Լ� ����
				*  : FD_ISSET �Լ��� �־��� ������ �־��� ������ ����ü�� ���ԵǾ� �ִ��� Ȯ���ϴ� �Լ��Դϴ�.
				*    �̸� ���� Ư�� ���Ͽ� ���� �̺�Ʈ�� ������ �� �ֽ��ϴ�.
				* - �ڵ� ����
				*  : ���� ���� ����ü�� for ���� ���� ��ȸ�� �ϸ� i ��° ������ CopyReadSocketList ����ü��
				*    ���Ͽ� ���ԵǾ� �ִ��� Ȯ�� ��, �̺�Ʈ�� �߻��� ������ �´��� Ȯ���ؼ� �´ٸ� 0�� �ƴ� ���� ��ȯ�մϴ�.
				*/
				if (FD_ISSET(ReadSocketList.fd_array[i], &CopyReadSocketList))
				{
					// Ŭ���̾�Ʈ ��û�� ó���� ���� �۾����� �̺�Ʈ�� �߻��� ������ ListenSocket �� �������� Ȯ���մϴ�.
					if (ReadSocketList.fd_array[i] == ListenSocket)
					{
						// ������ ���������� �޾Ƽ� ó���� Ŭ���̾�Ʈ�� ���� �ּ� �غ� �۾��Դϴ�.
						SOCKADDR_IN ClientSockAddr = { 0 , };				// ���� �� �ʱ�ȭ
						int ClientSockAddrSize = sizeof(ClientSockAddr);	// accept �� ���� ���� ���̸� ���� ���� �غ�

						// accept �� ������ �����ϰ� ���� �ش� ������ FD_SET �� ���� ����͸��� ���� ����ü�� �߰��մϴ�.
						SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientSockAddr, &ClientSockAddrSize);
						FD_SET(ClientSocket, &ReadSocketList);
					}
					else
					{
						// ������ ���� �߰��۾��� ���� ��� �� �б⿡���� ���� ������ ������ ó�� ������ �����մϴ�.(�Լ��̿�)
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


// �޾ƿ� ������ �����͸� � �̸� ó���ϱ� ���� �Լ��Դϴ�.
void ProcessPacket(SOCKET DataSocket)
{
	// ��Ŷ�� ����� ����� char �� �迭�� �غ��մϴ�.
	// Header �� �ش��ϴ� ������ Packet.h �� ���ǵǾ� �ֽ��ϴ�.
	// �ʱ⿡ ���� Size �� Type �� Header �� �ش�Ǹ� �� ���� ��ġ�� 4 ����Ʈ�̱⿡
	// char �� �迭 ���� Header ��, ������� 4 ����Ʈ�� �߽��ϴ�.
	char Header[4] = { 0, };

	// ���� recv �� ���ؼ� �޾ƿ� �������� ���� Header �� ������ ����ݴϴ�.
	// ��� �ʿ��� ������ �� 4 ����Ʈ �̱� ������ ����� 4�� �����߽��ϴ�.(�պκи� �߶� ���)
	// ���� MSG_WAITALL �� ���� 4����Ʈ �����͸� ��� �ޱ� ������ ����ϰ� �մϴ�.
	int RecvLength = recv(DataSocket, Header, 4, MSG_WAITALL);
	if (RecvLength <= 0)
	{
		// 0 ���� ������ ���޹��� ������ ������ �����ֵ��� �մϴ�.
		DisconnectPlayer(DataSocket);
		return;
	}

	// ��Ŷ�� ����� ���� ������ Ÿ���� ���� ������ ������ݴϴ�.
	unsigned short DataSize = 0;
	EPacketType PacketType = EPacketType::Max;

	// ��� ����, Disassemble Header
	// �޾ƿ� ����� ������ ��� ���� ������ ����ݴϴ�. �� ������ �̿��ؼ� ��ó�� �۾��� �̾ �̴ϴ�.
	memcpy(&DataSize, &Header[0], 2);
	memcpy(&PacketType, &Header[2], 2);

	// ��Ʈ��ũ ����Ʈ ������ ȣ��Ʈ�� ����Ʈ ������ ��ȯ�ϴ� �۾��� �մϴ�.(ntohs)
	// �̴� �ý��ۿ� ���� ����Ʈ ������ �ٸ��� ������ �ϴ� �۾��̸� �̸� �� �־�� ��Ȯ�� ������ ó���� �����մϴ�.
	// ��, �޾ƿ� ��Ŷ�� ������ �ùٸ��� �ؼ��ϰ� ó���ϱ� ���� �۾����� �츮�� Ŭ���̾�Ʈ�� ���� �޾ƿ��⿡
	// ��Ʈ��ũ ����Ʈ ������ ����Ǿ� ���� ���̱� ������ �޾Ƽ� ó���ϴ� ȣ��Ʈ���� �� �۾��� �մϴ�.
	DataSize = ntohs(DataSize);
	PacketType = (EPacketType)(ntohs((u_short)PacketType));

	char Data[1024] = { 0, };

	// ������ ó��, Data
	// ���� ������ �Ʒ�ó�� ��� Data �� �ޱ� ���ؼ� MSG_WAITALL �� ���� �� ���������� ������ ����մϴ�.
	RecvLength = recv(DataSocket, Data, DataSize, MSG_WAITALL);
	if (RecvLength <= 0)
	{
		// 0 �����̸� �ƹ��� �۾��� �����ʰ� ������ �����ֵ��� �մϴ�.
		DisconnectPlayer(DataSocket);
		return;
	}
	else if (RecvLength > 0)
	{
		// Packet Type
		// 0 ���� ũ�� ��� �����Ͱ� �Ǿ��� �޾ƿԱ� ������ ó���� �����մϴ�.
		switch (PacketType)
		{
			// [ ��Ŷ Ÿ���� Login �� ��쿡 ���� ó���Դϴ�. ]
			case EPacketType::C2S_Login:
			{
				// Player ����ü�� ������ ������ ����ϴ�.
				Player NewPlayer;
				memcpy(&NewPlayer.ID, &Data[0], 4);	// ID ���� ���
				memcpy(&NewPlayer.X, &Data[4], 4);	// X ��ǥ ���� ���
				memcpy(&NewPlayer.Y, &Data[8], 4);	// Y ��ǥ ���� ���

				// Player �� �α��� ������ ���� ����(Spawn)�� ���� ó���Դϴ�.
				//NewPlayer.ID = ntohl(NewPlayer.ID);
				NewPlayer.ID = (int)DataSocket;		// ���� ���� �ĺ��� ���� ID�� ĳ���ͷ� ���
				NewPlayer.X = rand() % 80;			// X ��ǥ ���� ��ġ ����
				NewPlayer.Y = rand() % 15;			// Y ��ǥ ���� ��ġ ����

				// ������� Player ������ ���� ������ ����ݴϴ�.
				SessionList[DataSocket] = NewPlayer;

				cout << "Connect Client : " << SessionList.size() << endl;

				// S2C_Login
				// ���� ������ Ŭ���̾�Ʈ���� ������ �����ϴ�.(���ø�����Ʈ�� ���ؼ� �����ϸ� �˴ϴ�)
				// ������� Player ������ Ŭ���̾�Ʈ���� �����ֱ� ���ؼ� ��Ŷ�� ���� ����ִ� �۾��� �մϴ�.
				// �ش� �Լ��� Packet.h �� �ۼ��Ǿ� ������ �̸� �����ϵ��� �մϴ�.
				PacketManager::PlayerData = NewPlayer;
				PacketManager::Type = EPacketType::S2C_Login;
				PacketManager::Size = 12;
				PacketManager::MakePacket(Data);

				// ���� ������� ��Ŷ�� �ٽ� Ŭ���̾�Ʈ���� �������ݴϴ�.
				int SendLength = send(DataSocket, Data, PacketManager::Size + 4, 0);


				// S2C_Spawn, ���� ��� �÷��̾������� Ŭ���̾�Ʈ���� �����մϴ�.
				char SendData[1024];
				// ���� ����Ʈ�� ��� ��� Ŭ���̾�Ʈ ���Ͽ� ���� �ݺ����Դϴ�.
				// ��, ����� ��� Ŭ���̾�Ʈ���� �۾��� ���ֱ� ���� �ڵ��Դϴ�.
				for (const auto& Receiver : SessionList)
				{
					// �̹����� ���ǿ� ������ ��� Ŭ���̾�Ʈ ������ ���� �۾��� �ϱ� ���� �ڵ��Դϴ�.
					// �Ʒ��� �ݺ������� ���� ����Ʈ�� i ��° ����(Ŭ���̾�Ʈ)�� ��� �÷��̾� ������ �����մϴ�.
					for (const auto& Data : SessionList)
					{
						// ���� �ڵ��� �ݺ�����, �÷��̾� ������ ����� ���� �ڵ��Դϴ�.
						PacketManager::PlayerData = Data.second;
						PacketManager::Type = EPacketType::S2C_Spawn;
						PacketManager::Size = 12;
						PacketManager::MakePacket(SendData);

						// ���� ����� ���Ͽ� �÷��̾� ����(��Ŷ)�� �����ݴϴ�.
						int SendLength = send(Receiver.first, SendData, PacketManager::Size + 4, 0);

						cout << Receiver.first << " Send Spawn Client " << SendLength << endl;
					}
				}
			}
			break;

			// [ ��Ŷ Ÿ���� Move �� ��쿡 ���� ó���Դϴ�. ]
			case EPacketType::C2S_Move:
			{
				// �̹����� ���������� ��Ŷ�� �����ؼ� �ʿ��� ������ �����ɴϴ�.
				Player MovePlayer;
				memcpy(&MovePlayer.ID, &Data[0], 4);
				memcpy(&MovePlayer.X, &Data[4], 4);
				memcpy(&MovePlayer.Y, &Data[8], 4);

				// ������ ������ ó���� ���� ����Ʈ ������ �����ؼ� ������ ����ϴ�.
				MovePlayer.ID = ntohl(MovePlayer.ID);
				MovePlayer.X = ntohl(MovePlayer.X);
				MovePlayer.Y = ntohl(MovePlayer.Y);

				// �޾ƿ� KeyCode �� �̿��ؼ� ������(��ǥ ����)�� �����ϵ��� �մϴ�.
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
				// ���� ����� Player �� ��ġ ������ ��Ŷ�� ��Ƽ� Ŭ���̾�Ʈ���� �����մϴ�.
				PacketManager::PlayerData = SessionList[MovePlayer.ID];
				PacketManager::Type = EPacketType::S2C_Move;
				PacketManager::Size = 12;
				PacketManager::MakePacket(Data);

				// ���������� ��� Ŭ���̾�Ʈ���� �����ϱ� ���� �ݺ����Դϴ�.
				for (const auto& Receiver : SessionList)
				{
					int SendLength = send(Receiver.first, Data, PacketManager::Size + 4, 0);
				}

			}
			break;
		}
	}
}


// ������ ���� �� ����ϴ� �Լ��Դϴ�.
void DisconnectPlayer(SOCKET DataSocket)
{
	// Player �α׾ƿ��� ó���ϱ� ���� Player ����ü�� �����մϴ�.
	// ���� �޾ƿ� ������ ������ �� ���Ͽ� ����ݴϴ�.
	Player LogoutPlayer;
	LogoutPlayer = SessionList[DataSocket];

	// ���� ���� ���� ���ӵǾ� �ִ� ������ ������ �����ݴϴ�.
	SessionList.erase(DataSocket);

	// ���� �ݺ����� ���ؼ� ��� Ŭ���̾�Ʈ���� �����ϰ� ó���մϴ�.
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

	// ���������� ����͸��� ���� ����ü���� �α׾ƿ��� ������ �������ְ� �ش� ������ �����մϴ�.
	FD_CLR(DataSocket, &ReadSocketList);
	closesocket(DataSocket);
}