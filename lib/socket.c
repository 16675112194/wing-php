/*******************************
 *@author yuyi
 *@email 297341015@qq.com
 *@created 2016-05-20
 ******************************/
#include <Winsock2.h>
#include "Windows.h"
#include "socket.h"
#define PORT 5150 

/*
unsigned int __stdcall  socket_worker(LPVOID ComlpetionPortID)  
{  
	HANDLE ComplectionPort = (HANDLE) ComlpetionPortID;  
	DWORD BytesTransferred;  
	LPOVERLAPPED Overlapped;  
	LPPER_HANDLE_DATA PerHandleData;  
	LPPER_IO_OPERATION_DATA PerIOData;  
	DWORD SendBytes,RecvBytes;  
	DWORD Flags;  

	while (TRUE)  
	{  

		if (GetQueuedCompletionStatus(ComplectionPort,&BytesTransferred,(LPDWORD)&PerHandleData,(LPOVERLAPPED*)&PerIOData,INFINITE) == 0)  
		{  
			//printf("GetQueuedCompletionStatus failed with error%d ",GetLastError());  
			return 0;
		 }  

		//���ȼ���׽������Ƿ������������������ر��׽��ֲ������ͬ�׽�����ص�SOCKET_INFORATION �ṹ��  
		if (BytesTransferred == 0) 
		{  

			  //printf("Closing Socket %d ",PerHandleData->Socket);  
			 if (closesocket(PerHandleData->Socket) == SOCKET_ERROR)  
			 {  
				//printf("closesocket failed with error %d ",WSAGetLastError());  
				return 0;  
			 }  

			GlobalFree(PerHandleData);  
			GlobalFree(PerIOData);  
			continue;  
		}  

   
		//���BytesRecv���Ƿ����0������ǣ�˵��WSARecv���øո���ɣ������ôӼ���ɵ�WSARecv���÷��ص�BytesTransferredֵ����BytesRecv��  
		if (PerIOData->BytesRecv == 0)  
		{  
			PerIOData->BytesRecv = BytesTransferred;  
			PerIOData->BytesSend = 0; 
		}
		else
		{  
		  PerIOData->BytesRecv +=BytesTransferred;  
		}  


		if (PerIOData->BytesRecv > PerIOData->BytesSend)  
		{  

			   //������һ��WSASend()������ΪWSASendi ����ȷ����������������ֽڣ�����WSASend����ֱ�������������յ����ֽ�  
			  ZeroMemory(&(PerIOData->OVerlapped),sizeof(OVERLAPPED));  
			  PerIOData->DATABuf.buf = PerIOData->Buffer + PerIOData->BytesSend;  
			  PerIOData->DATABuf.len = PerIOData->BytesRecv - PerIOData->BytesSend;  
			  if (WSASend(PerHandleData->Socket,&(PerIOData->DATABuf),1,&SendBytes,0,&(PerIOData->OVerlapped),NULL) ==SOCKET_ERROR )  
			  {  
				if (WSAGetLastError() != ERROR_IO_PENDING)  
				{  
				  printf("WSASend() fialed with error %d ",WSAGetLastError());  
				  return 0;  
				}  
			  }  
		}  
		else  
		{  
			  PerIOData->BytesRecv = 0;  
			  //Now that is no more bytes to send post another WSARecv() request  
			  //���ڼ����������  
			  Flags = 0;  
			  ZeroMemory(&(PerIOData->OVerlapped),sizeof(OVERLAPPED));  
			  PerIOData->DATABuf.buf = PerIOData->Buffer;  
			  PerIOData->DATABuf.len = DATA_BUFSIZE;  
			  if (WSARecv(PerHandleData->Socket,&(PerIOData->DATABuf),1,&RecvBytes,&Flags,&(PerIOData->OVerlapped),NULL) == SOCKET_ERROR)  
			  {  
				if (WSAGetLastError() != ERROR_IO_PENDING)  
				{  
				 // printf("WSARecv() failed with error %d ",WSAGetLastError());  
				  return 0;  
				}  
			  }  
		}  
	}  
	return 0;
}  


bool socket_service(){

	//1��������ɶ˿�
	HANDLE m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0 ); 
	if(m_hIOCompletionPort == NULL){
		return false;
	}

	//2������cpu�������������߳�
	//��ȡcpu���� ���ں��洴��worker����
	SYSTEM_INFO si; 
	GetSystemInfo(&si); 
	int m_nProcessors = si.dwNumberOfProcessors; 
	// ����CPU����������*2���߳�
	int m_nThreads = 2 * m_nProcessors; 
	//HANDLE* m_phWorkerThreads = new HANDLE[m_nThreads]; 
	for (int i = 0; i < m_nThreads; i++) 
	{ 
		// m_phWorkerThreads[i] = CreateThread(NULL,NULL,socket_worker,0,0,0);
		 _beginthreadex(NULL, 0, socket_worker, NULL, 0, NULL); //::CreateThread(0, 0, _WorkerThread, ��); 
	} 


	// ��ʼ��Socket��
	WSADATA wsaData; 
	if(WSAStartup(MAKEWORD(2,2), &wsaData)!=0)
		return false; 
	
	//WSACleanup( );

	//��ʼ��Socket
	
	// ������Ҫ�ر�ע�⣬���Ҫʹ���ص�I/O�Ļ����������Ҫʹ��WSASocket����ʼ��Socket
	// ע�������и�WSA_FLAG_OVERLAPPED����
	SOCKET m_sockListen = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); 
	if(m_sockListen == INVALID_SOCKET){
		WSACleanup();
		return false;
	}


	struct sockaddr_in ServerAddress; 
	// ����ַ�ṹ��Ϣ
	ZeroMemory((char *)&ServerAddress, sizeof(ServerAddress)); 
	ServerAddress.sin_family = AF_INET; 
	// �������ѡ����κ�һ�����õĵ�ַ���������Լ�ָ����һ��IP��ַ
	//ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);                     
	ServerAddress.sin_addr.s_addr = inet_addr("0.0.0.0");          
	ServerAddress.sin_port = htons(6998);   


	// �󶨶˿�
	if (SOCKET_ERROR == bind(m_sockListen, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress))){
		closesocket(m_sockListen);
		WSACleanup();
		return false;
	}  
	// ��ʼ����
	if( 0 != listen(m_sockListen,SOMAXCONN)){
		closesocket(m_sockListen);
		WSACleanup();
		return false;
	}


	SOCKET accept ;
	PER_HANDLE_DATA *PerHandleData;
	LPPER_IO_OPERATION_DATA PerIOData;
	DWORD SendBytes,RecvBytes;  
    DWORD Flags;  

	while(true){

		 accept = WSAAccept(m_sockListen,NULL,NULL,NULL,0);
		 if( SOCKET_ERROR == accept){
			closesocket(m_sockListen);
			WSACleanup();
			return false;
		 }
		
		 PerHandleData = (PER_HANDLE_DATA*)GlobalAlloc(GPTR,sizeof(PER_HANDLE_DATA));
		 PerHandleData->Socket = accept;

		 if( NULL == CreateIoCompletionPort ((HANDLE )accept ,m_hIOCompletionPort , (ULONG_PTR )PerHandleData ,0)){
			closesocket(accept);
			closesocket(m_sockListen);
			WSACleanup();
			return false;
		 }

		PerIOData = (LPPER_IO_OPERATION_DATA)GlobalAlloc(GPTR,sizeof(PER_IO_OPERATION_DATA));
		if ( PerIOData == NULL )
		{
			closesocket(accept);
			closesocket(m_sockListen);
			WSACleanup();
			return false;
		} 

		ZeroMemory(&(PerIOData->OVerlapped),sizeof(OVERLAPPED)); 
		PerIOData->BytesRecv = 0;
		PerIOData->BytesSend = 0; 
		PerIOData->DATABuf.len = DATA_BUFSIZE; 
		PerIOData->DATABuf.buf = PerIOData->Buffer; 
		Flags = 0; 

		if (WSARecv(accept,&(PerIOData->DATABuf),1,&RecvBytes,&Flags,&(PerIOData->OVerlapped),NULL) == SOCKET_ERROR) 
		{ 
			if (WSAGetLastError() != ERROR_IO_PENDING) 
			{
				closesocket(accept);
				closesocket(m_sockListen);
				WSACleanup();
				return false; 
			} 
		}
	}
	return true;
}
*/