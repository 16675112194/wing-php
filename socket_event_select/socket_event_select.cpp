
/**
 *@�¼�ѡ��ģ��
 */
#include "stdafx.h"
#include <winsock2.h>
#include <stdio.h>

 

#define PORT    5150
#define MSGSIZE 1024
#pragma comment(lib, "ws2_32.lib")

int      g_iTotalConn = 0;
SOCKET   g_CliSocketArr[MAXIMUM_WAIT_OBJECTS];
WSAEVENT g_CliEventArr[MAXIMUM_WAIT_OBJECTS];
DWORD WINAPI WorkerThread(LPVOID);
void Cleanup(int index);

 
int main()
{

	WSADATA     wsaData;
	SOCKET      sListen, sClient;
	SOCKADDR_IN local, client;
	DWORD       dwThreadId;
	int         iaddrSize = sizeof(SOCKADDR_IN);

	// Initialize Windows Socket library
	WSAStartup(0x0202, &wsaData);

	// Create listening socket
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Bind
	local.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	local.sin_family = AF_INET;
	local.sin_port = htons(PORT);
	bind(sListen, (struct sockaddr *)&local, sizeof(SOCKADDR_IN));

	// Listen
	listen(sListen, 3);

	// Create worker thread
	CreateThread(NULL, 0, WorkerThread, NULL, 0, &dwThreadId);

	while (TRUE)
	{

		// Accept a connection
		sClient = accept(sListen, (struct sockaddr *)&client, &iaddrSize);

		printf("Accepted client:%s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

		// Associate socket with network event
		g_CliSocketArr[g_iTotalConn] = sClient;//�������ӵ��׽ӿ�
		g_CliEventArr[g_iTotalConn] = WSACreateEvent();//�����¼�������

		//���׽ӿ��Ͻ�һ�����������¼��� �¼����������һ��
		WSAEventSelect( g_CliSocketArr[g_iTotalConn],//�׽ӿ�
						g_CliEventArr[g_iTotalConn],//�¼�����
						FD_READ | FD_CLOSE);//�����¼�

		g_iTotalConn++;
	}
}

 

DWORD WINAPI WorkerThread(LPVOID lpParam)
{

	int              ret, index;
	WSANETWORKEVENTS NetworkEvents;
	char             szMessage[MSGSIZE];

	while (TRUE)
	{ 
	
		//���ص��·��ص��¼�����
		ret = WSAWaitForMultipleEvents( g_iTotalConn,    //�����еľ����Ŀ
										g_CliEventArr,   //ָ��һ���¼������������ָ��
										FALSE,           //T�������Żأ�F��һ���ͻ�
										1000,            //��ʱ���
										FALSE);          //�Ƿ�ִ���������

		if (ret == WSA_WAIT_FAILED || ret == WSA_WAIT_TIMEOUT)
		{
			continue;
		}

 

		index = ret - WSA_WAIT_EVENT_0;

		//���׽ӿ��ϲ�ѯ���¼���������������¼�
		WSAEnumNetworkEvents(g_CliSocketArr[index], g_CliEventArr[index], &NetworkEvents);

		//����FD-READ�����¼�
		if (NetworkEvents.lNetworkEvents & FD_READ)
		{

			// Receive message from client
			ret = recv(g_CliSocketArr[index], szMessage, MSGSIZE, 0);

			if (ret == 0 || (ret == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET))
			{
				Cleanup(index);
			}
			else
			{
				szMessage[ret] = '\0';
				send(g_CliSocketArr[index], szMessage, strlen(szMessage), 0);
			}

		}

		//����FD-CLOSE�����¼�
		if (NetworkEvents.lNetworkEvents & FD_CLOSE)
		{
			Cleanup(index);
		}

	}

	return 0;

}

 

void Cleanup(int index)
{

	closesocket(g_CliSocketArr[index]);
	WSACloseEvent(g_CliEventArr[index]);

	if (index < g_iTotalConn - 1)
	{

		g_CliSocketArr[index] = g_CliSocketArr[g_iTotalConn-1];
		g_CliEventArr[index] = g_CliEventArr[g_iTotalConn- 1];

	}

	g_iTotalConn--;

}

 
