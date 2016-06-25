#include "wing_socket.h"
#include "wing_msg_queue.h"
#include "synchapi.h"

SOCKET m_sockListen;//����socket

/*********************************************************************
 * @ post ������Ϣ
 *********************************************************************/
void wing_post_queue_msg( int message_id,unsigned long wparam,unsigned long lparam,unsigned long eparam )
{
		
	wing_msg_queue_element *msg	= new wing_msg_queue_element();  

	if( NULL == msg ) 
		wing_socket_throw_error(WING_BAD_ERROR);           //���������ش���

	msg->message_id = message_id;                          //��Ϣid
	msg->wparam		= wparam;                              //����1
	msg->lparam		= lparam;	                           //����2
	msg->size		= 0;                                   //�ڴ� ��ʱû�� ȥ����
	msg->eparam		= eparam;                              //��չ���� 3

	wing_msg_queue_lpush(msg);	
}

/*********************************************************************
 * @ ����wsa exϵ�к��� ���ﲢû���õ�������� 
 * @ �������ÿ�ζ��ᴴ��һ����ʱ��socket ���˷���Դ�� ��ľ��
 *********************************************************************/
BOOL WingLoadWSAFunc(GUID &funGuid,void* &pFun)
{
	//���������ò������غ���ָ��
	DWORD dwBytes = 0;
	pFun = NULL;

	//��㴴��һ��SOCKET��WSAIoctlʹ�� ����һ��Ҫ��������������
	SOCKET skTemp = ::WSASocket(AF_INET,SOCK_STREAM, IPPROTO_TCP, NULL,0, WSA_FLAG_OVERLAPPED);
	if(INVALID_SOCKET == skTemp)
	{
		//ͨ����ʾû�������ĳ�ʼ��WinSock����
		return FALSE;
	}
    ::WSAIoctl(skTemp, SIO_GET_EXTENSION_FUNCTION_POINTER, &funGuid,sizeof(funGuid),&pFun,sizeof(pFun), &dwBytes, NULL,NULL);
    ::closesocket(skTemp);

    return NULL != pFun;
}

/*********************************************************************
 * @ �Ͽ�socket���� socke����
 *********************************************************************/
BOOL WingDisconnectEx( SOCKET hSocket , LPOVERLAPPED lpOverlapped , DWORD dwFlags , DWORD reserved )
{
	if( !hSocket || !lpOverlapped ) 
	{	
		return 0;
	}
	GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
	DWORD dwBytes = 0;
	LPFN_DISCONNECTEX lpfnDisconnectEx; 

	if( 0 != WSAIoctl( hSocket,SIO_GET_EXTENSION_FUNCTION_POINTER,&GuidDisconnectEx,sizeof(GuidDisconnectEx),&lpfnDisconnectEx,sizeof(lpfnDisconnectEx),&dwBytes,NULL,NULL))
	{
		return 0;
	}

	return lpfnDisconnectEx(hSocket,lpOverlapped,/*TF_REUSE_SOCKET*/dwFlags,reserved);
}

/*********************************************************************
 * @ Ͷ��acceptex
 *********************************************************************/
BOOL WingAcceptEx(SOCKET sListenSocket,SOCKET sAcceptSocket,PVOID lpOutputBuffer,DWORD dwReceiveDataLength,DWORD dwLocalAddressLength,DWORD dwRemoteAddressLength,LPDWORD lpdwBytesReceived,LPOVERLAPPED lpOverlapped)
{
	if( !sListenSocket || !lpOverlapped ) 
	{	
		return 0;
	}
	GUID guidAcceptEx	= WSAID_ACCEPTEX;
	DWORD dwBytes		= 0;
	LPFN_ACCEPTEX lpfnAcceptEx;

	if( 0 != WSAIoctl(sListenSocket,SIO_GET_EXTENSION_FUNCTION_POINTER,&guidAcceptEx,sizeof(guidAcceptEx),&lpfnAcceptEx,sizeof(lpfnAcceptEx),&dwBytes,NULL,NULL))
	{
		return 0;
	}

	return lpfnAcceptEx( sListenSocket,sAcceptSocket,lpOutputBuffer,dwReceiveDataLength,dwLocalAddressLength,dwRemoteAddressLength,lpdwBytesReceived,lpOverlapped);        
}

/***********************************************************************
 * @ ��ȡacceptex���ӽ�����socket ip��ַ�Ͷ˿� 
 * @ ������ʵ��Ӧ���� ���������ò���ֵ�ģ�ֵ������䵽 lpOutputBuffer�ˣ�
 * @ ��Ҫ�Լ�ȥ��������������ʼ��ַ+10����������û��ʹ�ô�api
***********************************************************************/
void WingGetAcceptExSockaddrs( 
		SOCKET sListenSocket , 
		PVOID lpOutputBuffer,
		DWORD dwReceiveDataLength,
		DWORD dwLocalAddressLength,
		DWORD dwRemoteAddressLength,
		LPSOCKADDR *LocalSockaddr,
		LPINT LocalSockaddrLength,
		LPSOCKADDR *RemoteSockaddr,
		LPINT RemoteSockaddrLength
	)
{
	LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs = NULL;
	GUID guidGetAcceptExSockaddrs	= WSAID_GETACCEPTEXSOCKADDRS;
	DWORD dwBytes					= 0;
	int ret = WSAIoctl( 
						sListenSocket,
						SIO_GET_EXTENSION_FUNCTION_POINTER,
						&guidGetAcceptExSockaddrs,
						sizeof(guidGetAcceptExSockaddrs),
						&lpfnGetAcceptExSockaddrs,
						sizeof(lpfnGetAcceptExSockaddrs),
						&dwBytes,
						NULL,
						NULL
					);
	if( 0 != ret )
	{
		return;
	}
	return lpfnGetAcceptExSockaddrs( 
					lpOutputBuffer,
					dwReceiveDataLength,
					dwLocalAddressLength, 
					dwRemoteAddressLength, 
					LocalSockaddr, 
					LocalSockaddrLength,
					RemoteSockaddr, 
					RemoteSockaddrLength
				);   
}
//���ش��� ֱ��exit
void wing_socket_throw_error( int error_code ){
	exit(WING_BAD_ERROR);
}

/****************************************************
 * @ Ͷ��һ�� recv ��acceptex ֮�� ֻ��Ҫ����һ��
 ****************************************************/
bool wing_socket_post_recv( MYOVERLAPPED* &pMyOL )
{
	
	pMyOL->m_DataBuf.buf	= pMyOL->m_pBuf;  
	pMyOL->m_DataBuf.len	= DATA_BUFSIZE;  
	pMyOL->m_iOpType		= OPE_RECV;

	DWORD RecvBytes			= 0;
	DWORD Flags				= 0;

	int code		= WSARecv(pMyOL->m_skClient,&(pMyOL->m_DataBuf),1,&RecvBytes,&Flags,&(pMyOL->m_ol),NULL);
	int error_code	= WSAGetLastError();
	//����һ�� WSARecv ����iocp�¼�
	if( 0 != code && WSA_IO_PENDING != error_code )
	{
		return false;
	}
	return true;
}


/****************************************************
 * @ accept�ص� �µĿͻ������ӽ�����
 * @ �ص�֮����ҪͶ��һ���µ� accept �Դ��´��¿ͻ�������
 * @ ����Ҫͷ��һ��recv ������������
 ****************************************************/
void wing_socket_on_accept(MYOVERLAPPED* &pMyOL){

	int lenHost		= sizeof(sockaddr_in) + 16;
	int lenClient	= sizeof(sockaddr_in) + 16;

	pMyOL->m_isUsed = WING_SOCKET_IS_ALIVE;
	
	setsockopt(pMyOL->m_skClient, SOL_SOCKET,SO_UPDATE_ACCEPT_CONTEXT,(const char *)&pMyOL->m_skServer,sizeof(pMyOL->m_skServer));

	
	// ���÷��͡����ճ�ʱʱ�� 
	if( pMyOL->m_timeout > 0 )
	{
		setsockopt(pMyOL->m_skClient, SOL_SOCKET,SO_SNDTIMEO, (const char*)&pMyOL->m_timeout,sizeof(pMyOL->m_timeout));
		setsockopt(pMyOL->m_skClient, SOL_SOCKET,SO_RCVTIMEO, (const char*)&pMyOL->m_timeout,sizeof(pMyOL->m_timeout));
	}

	linger so_linger;
	so_linger.l_onoff	= TRUE;
	so_linger.l_linger	= 0; //�ܾ�close wait״̬
	setsockopt(pMyOL->m_skClient,SOL_SOCKET,SO_LINGER,(const char*)&so_linger,sizeof(so_linger));
	
	//��ȡ�ͻ���ip��ַ���˿���Ϣ
	int client_size = sizeof(pMyOL->m_addrClient);  
	ZeroMemory( &pMyOL->m_addrClient , sizeof(pMyOL->m_addrClient) );
	getpeername( pMyOL->m_skClient , (SOCKADDR *)&pMyOL->m_addrClient , &client_size );  

	//����keep alive �����쳣���߼��
	/*int dt		= 1;
	DWORD dw	= 0;
	tcp_keepalive live;     
	live.keepaliveinterval	= 5000;   //����֮�� �೤ʱ�䷢���޻ ��ʼ���������� ��λΪ���� 
	live.keepalivetime		= 1000;   //�೤ʱ�䷢��һ�������� 1������ 60000 �Դ�����     
	live.onoff				= TRUE;   //�Ƿ��� keepalive

	//keepalive ���� �����п� ����֪����ô�� ������keep alive�Ժ� disconnectex �ᱨ����Ч�ľ����(An invalid handle was specified��)
	//���߳��Լ���������ͻ��iocpҪ�������������⣬δ֪
	setsockopt( pMyOL->m_skClient, SOL_SOCKET, SO_KEEPALIVE, (char *)&dt,sizeof(dt) );               
	WSAIoctl(   pMyOL->m_skClient, SIO_KEEPALIVE_VALS, &live, sizeof(live), NULL, 0, &dw, &pMyOL->m_ol , NULL );*/	

	//post onconnect ������Ϣ
	wing_post_queue_msg(WM_ONCONNECT, pMyOL->m_skClient,0);
	//Ͷ��һ�� recv
	if( !wing_socket_post_recv( pMyOL ) )
	{
		wing_post_queue_msg( WM_ONERROR, pMyOL->m_skClient, WING_ERROR_POST_RECV, WSAGetLastError() );
	}
	
}

/****************************************************
 * @ ������Ϣ��� WSASend���У���ʱû�ã�
 * @ ��û����ȫ���˺���������ԭ���Լ�����
 ****************************************************/
void wing_socket_on_send( MYOVERLAPPED*  pOL){

}

/**************************************************
 * @ �յ��ͻ�����Ϣ
 **************************************************/
void wing_socket_on_recv(MYOVERLAPPED*  &pOL){
	
	char *recvmsg = new char[DATA_BUFSIZE];	    //������Ϣ
	ZeroMemory(recvmsg,DATA_BUFSIZE);           //����

	strcpy(recvmsg,pOL->m_pBuf);                //��Ϣ����

	ZeroMemory(pOL->m_pBuf,DATA_BUFSIZE);       //����������

	//������Ϣ����Ϣ����
	wing_post_queue_msg(WM_ONRECV,pOL->m_skClient,(unsigned long)recvmsg);
}


/*****************************************************************
 * @ �ͻ��˵����ˣ���ִ��socket�����Լ�����һ�´����socket
 * @ �Լ�������socket������µ�socket
 * @��������������socket����������������ܱ�֤socket�ص��ȶ��ԣ�
 *****************************************************************/
void wing_socket_on_close( MYOVERLAPPED*  &pMyOL )
{
	//���͵����¼�
	wing_post_queue_msg( WM_ONCLOSE , pMyOL->m_skClient );

	//socket����
	WingDisconnectEx( pMyOL->m_skClient , &pMyOL->m_ol , TF_REUSE_SOCKET , 0 );
	
	pMyOL->m_iOpType	= OPE_ACCEPT;                     //AcceptEx����
	pMyOL->m_isUsed     = WING_SOCKET_IS_SLEEP;           //����socket״̬Ϊ����״̬������������ж���Щsocket��ǰ�ǻ��
	ZeroMemory(pMyOL->m_pBuf,sizeof(char)*DATA_BUFSIZE);  //����������

	//Ͷ��һ��acceptex
	int error_code = WingAcceptEx( pMyOL->m_skServer,pMyOL->m_skClient,pMyOL->m_pBuf,0,sizeof(SOCKADDR_IN)+16,sizeof(SOCKADDR_IN)+16,NULL, (LPOVERLAPPED)pMyOL );
	int last_error = WSAGetLastError() ;

	unsigned long w_client = (unsigned long)pMyOL->m_skClient;
	
	if( !error_code && ERROR_IO_PENDING != last_error )
	{
		
		//���socket�����˴��� �����ǿ��õ�socket
		if( INVALID_SOCKET != pMyOL->m_skClient ) 
		{
			//�ص���������socket
			closesocket(pMyOL->m_skClient);
			
			//����һ���µ�socket
			SOCKET new_client   = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,0,0,WSA_FLAG_OVERLAPPED);
				
			//����������µ�socket����
			if( INVALID_SOCKET != new_client ) 
			{	
				//�������ɶ˿ڴ���
				if( !BindIoCompletionCallback((HANDLE)new_client ,wing_icop_thread,0) )
				{
					//���ϵ�socket �ӹ�ϵӳ�����Ƴ�
					wing_remove_from_sockets_map((unsigned long)w_client);
					//�رողŴ������µ�socket
					closesocket(new_client);
					new_client = INVALID_SOCKET;
					//������Դ
					delete pMyOL;
					pMyOL = NULL; 
					wing_post_queue_msg(WM_ONERROR,w_client,WING_ERROR_ACCEPT,last_error);
					return;
				}
				else
				{
					//�����iocp�ɹ�		
					pMyOL->m_skClient = new_client;

					int error_code = WingAcceptEx(pMyOL->m_skServer,pMyOL->m_skClient,pMyOL->m_pBuf,0,sizeof(SOCKADDR_IN)+16,sizeof(SOCKADDR_IN)+16,NULL, (LPOVERLAPPED)pMyOL);
					int last_error = WSAGetLastError() ;
					
					//���acceptex���� �����ø�����ľ�� ��л��������ѭ��
					if( !error_code && ERROR_IO_PENDING != last_error ){
						
						//���ϵ�socket�ӹ�ϵӳ���Ƴ�
						wing_remove_from_sockets_map((unsigned long)w_client);
						//�رյ��µ�socket
						closesocket(new_client);
						new_client = pMyOL->m_skClient = INVALID_SOCKET;
						//������Դ
						delete pMyOL;
						pMyOL = NULL; 
						wing_post_queue_msg(WM_ONERROR,w_client,WING_ERROR_ACCEPT,last_error);
						return;

					}else{
						//���û���� ��ӵ�mapӳ�� ������Ҫʹ��
						wing_add_to_sockets_map( (unsigned long)new_client, (unsigned long)pMyOL );
						wing_remove_from_sockets_map( (unsigned long)w_client );
					}
				}
			}
			else
			{
				wing_remove_from_sockets_map((unsigned long)w_client);
				delete pMyOL;
				pMyOL = NULL; 
				wing_post_queue_msg(WM_ONERROR,w_client,WING_ERROR_ACCEPT,last_error);
				return;		
			}
		}	

		////�����������Ӧ��Ҫ���������� ����������socketΪ0�˾Ͳ�����
		//����onerror ���½�һ�� socket Ȼ������ pMyOL ���� pMyOL��Ҫɾ��
		wing_post_queue_msg(WM_ONERROR,w_client,WING_ERROR_ACCEPT,last_error);
		return;
	}

	//ע�������SOCKET���������ú󣬺�����ٴ�������ɶ˿ڵĲ����᷵��һ�������õĴ����������ֱ�ӱ����Լ���
	::BindIoCompletionCallback((HANDLE)pMyOL->m_skClient,wing_icop_thread, 0);
}

/********************************************************************************************
 * @ iocp�����̳߳أ�����鿴api BindIoCompletionCallback
 ********************************************************************************************/
VOID CALLBACK wing_icop_thread(DWORD dwErrorCode,DWORD dwBytesTrans,LPOVERLAPPED lpOverlapped)
{
	int error_code = WSAGetLastError();
	//IOCP�ص�����
	if( NULL == lpOverlapped )
	{
		//��������������õģ������۲����
		wing_post_queue_msg(WM_THREAD_RUN,dwErrorCode,error_code);
		//û�����������
		SleepEx(20,TRUE);//�����óɿɾ���״̬
		return;
	}
	//��������������õģ������۲����
	wing_post_queue_msg(WM_THREAD_RUN,dwErrorCode,error_code);

	//���ﻹԭovl
	wing_myoverlapped*  pOL = CONTAINING_RECORD(lpOverlapped, wing_myoverlapped, m_ol);

	if( 0 != dwErrorCode ) {
		//���������жϿͻ��˵��ߵ�
		if( 0 == dwBytesTrans || 10054 == error_code || 64 == error_code){
			wing_socket_on_close(pOL);
			return;
		}
	}
	
	switch( pOL->m_iOpType )
	{
		case OPE_ACCEPT: //AcceptEx����
		{
			//�����ӽ����� SOCKET������� pMyOL->m_skClient
			wing_socket_on_accept(pOL);
		}
		break;
		case OPE_RECV:
		{
			pOL->m_recvBytes = dwBytesTrans;
			//�յ��ŵ���Ϣ ��Ҫ�ж�һ���Ƿ����
			if( 0 == dwBytesTrans || 10054 == error_code || 64 == error_code){
				wing_socket_on_close(pOL);
			} else {
				wing_socket_on_recv(pOL);
			}	
		}
		break;
		case OPE_SEND:
		{
			//�첽���� ���ﻹûʵ��
			wing_socket_on_send(pOL);
		}
		break;

	}

}

/****************************************************************************************************
 * @ socket��ʼ��
 * @ �����ֱ�Ϊ 
 * @ const char *listen_ip ������ip
 * @ const int port �����Ķ˿� 
 * @ const int max_connect �����������Ҳ����socket������
 * @ const int timeout ���պͷ��ͳ�ʱʱ�� Ĭ��Ϊ0����������ʱ
 * @ return ����ֵ Ϊsocket��Դ
 ****************************************************************************************************/
SOCKET wing_socket_init(const char *listen_ip,const int port,const int max_connect,const int timeout){
	
	//��ʼ��Socket
	WSADATA wsaData; 
	if( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 )
	{
		return INVALID_SOCKET; 
	}

	// ����Ƿ�����������汾���׽��ֿ�   
	if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
        WSACleanup();  
        return INVALID_SOCKET;  
    }  

	//����sokket
	m_sockListen = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); 
	if( INVALID_SOCKET == m_sockListen )
	{
		WSACleanup();
		return INVALID_SOCKET;
	}

	//����ɶ˿��̳߳�
	BOOL bReuse = TRUE;
	BOOL bind_status = ::BindIoCompletionCallback((HANDLE)m_sockListen,wing_icop_thread, 0);
	if( !bind_status )
	{
		closesocket(m_sockListen);
		WSACleanup();
		return INVALID_SOCKET;
	}


	//��ѡ���� SO_REUSEADDR 
	if( 0 != ::setsockopt(m_sockListen,SOL_SOCKET,SO_REUSEADDR,(LPCSTR)&bReuse,sizeof(BOOL)) )
	{
		//���ô��� �����Ȳ����� ��Ϊ�ǿ�ѡ������
	}


	// ����ַ�ṹ��Ϣ
	struct sockaddr_in ServerAddress; 
	ZeroMemory(&ServerAddress, sizeof(ServerAddress)); 

	ServerAddress.sin_family		= AF_INET;                    
	ServerAddress.sin_addr.s_addr	= inet_addr(listen_ip);          
	ServerAddress.sin_port			= htons(port);   


	// �󶨶˿�
	if ( SOCKET_ERROR == bind( m_sockListen, (struct sockaddr *) &ServerAddress, sizeof( ServerAddress ) ) )
	{
		closesocket(m_sockListen);
		WSACleanup();
		return INVALID_SOCKET;
	}  

	// ��ʼ����
	if( 0 != listen( m_sockListen , SOMAXCONN ) )
	{
		closesocket(m_sockListen);
		WSACleanup();
		return INVALID_SOCKET;
	}

	//socket ��
	for( int i = 0 ; i < max_connect ; i++ ) 
	{
	
		SOCKET client = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,0,0,WSA_FLAG_OVERLAPPED);
		if( INVALID_SOCKET == client ) 
		{	
			continue;
		}

		if( !BindIoCompletionCallback( (HANDLE)client ,wing_icop_thread,0) )
		{
			closesocket(client);
			continue;
		}

		wing_myoverlapped *pMyOL = new wing_myoverlapped();
		if( NULL == pMyOL )
		{
			closesocket(client);
			continue;
		}

		DWORD dwBytes = 0;
		ZeroMemory(pMyOL,sizeof(wing_myoverlapped));
		
		pMyOL->m_iOpType	= OPE_ACCEPT;
		pMyOL->m_skServer	= m_sockListen;
		pMyOL->m_skClient	= client;
		pMyOL->m_timeout	= timeout;
		pMyOL->m_isUsed     = WING_SOCKET_IS_SLEEP;

		int server_size = sizeof(pMyOL->m_addrServer);  
		ZeroMemory(&pMyOL->m_addrServer,server_size);
		getpeername(pMyOL->m_skServer,(SOCKADDR *)&pMyOL->m_addrServer,&server_size);  

		int error_code = WingAcceptEx( m_sockListen,pMyOL->m_skClient,pMyOL->m_pBuf,0,sizeof(SOCKADDR_IN)+16,sizeof(SOCKADDR_IN)+16,NULL, (LPOVERLAPPED)pMyOL );
		int last_error = WSAGetLastError() ;
		if( !error_code && WSAECONNRESET != last_error && ERROR_IO_PENDING != last_error )
		{
			
			closesocket( client );
			client = pMyOL->m_skClient = INVALID_SOCKET;
			delete pMyOL;
			pMyOL = NULL; 

			continue;
		}
		//��ӵ�hash mapӳ�� ������Ҫʹ��
		wing_add_to_sockets_map( (unsigned long)client , (unsigned long)pMyOL );
	}

	return m_sockListen;
}

/***********************************************************************
 * @ ��Դ����һ���ڷ������ش��������Ҫ�رշ����ʱ��ʹ��
 **********************************************************************/
void wing_socket_clear(){
	//CloseHandle(m_hIOCompletionPort);
	//�Ͱ汾��ϵͳ������Ҫʹ�õ�������� �����Ȳ���������
	//PostQueuedCompletionStatus(m_hIOCompletionPort, 0xFFFFFFFF, 0, NULL);
	if( INVALID_SOCKET != m_sockListen ) 
		closesocket(m_sockListen);
	WSACleanup();
}