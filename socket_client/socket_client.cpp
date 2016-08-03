/**
 *@����socket�ͻ���
 */
#include "stdafx.h"
#include <WINSOCK2.H>
#include <stdio.h>

 
#define SERVER_ADDRESS "127.0.0.1"
#define PORT           5150
#define MSGSIZE        1024
#pragma comment(lib, "ws2_32.lib")

 

int main()
{

	WSADATA     wsaData;
	SOCKET      sClient;
	SOCKADDR_IN server;
	char        *szMessage = "hello\0";//[MSGSIZE];
	int         ret;

	// Initialize Windows socket library
	WSAStartup(0x0202, &wsaData);

	// Create client socket
	sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Connect to server
	memset(&server, 0, sizeof(SOCKADDR_IN));
	server.sin_family = AF_INET;
	server.sin_addr.S_un.S_addr = inet_addr(SERVER_ADDRESS);
	server.sin_port = htons(PORT);

	connect(sClient, (struct sockaddr *)&server, sizeof(SOCKADDR_IN));

	while (TRUE)
	{

		//printf("Send:");
		//gets_s(szMessage);

		// Send message
		send(sClient, szMessage, strlen(szMessage), 0);

		// Receive message
		char *recvmsg = new char[MSGSIZE];
		memset(recvmsg,0,MSGSIZE);
		ret = recv(sClient, recvmsg, MSGSIZE, 0);
		printf("Received [%d bytes]: ��%s��\n", ret, recvmsg);
		delete[] recvmsg;

	}

	// Clean up
	closesocket(sClient);
	WSACleanup();
	return 0;

}
