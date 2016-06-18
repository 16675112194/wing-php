#include "wing_socket.h"
#include "wing_msg_queue.h"

HANDLE m_hIOCompletionPort;

//post ������Ϣ
void wing_post_queue_msg(int message_id,unsigned long wparam,unsigned long lparam){
		
	wing_msg_queue_element *msg	= new wing_msg_queue_element();  
	if( NULL == msg ) wing_socket_throw_error(WING_BAD_ERROR);

	msg->message_id = message_id;
	msg->wparam		= wparam;
	msg->lparam		= lparam;	
	msg->size		= 0;

	wing_msg_queue_lpush(msg);	
}

//����exϵ�к���
BOOL LoadWSAFun(GUID&funGuid,void*& pFun)
{
	//���������ò������غ���ָ��
	DWORD dwBytes = 0;

	pFun = NULL;

	//��㴴��һ��SOCKET��WSAIoctlʹ�� ����һ��Ҫ��������������

	SOCKET skTemp = ::WSASocket(AF_INET,SOCK_STREAM, IPPROTO_TCP, NULL,0, WSA_FLAG_OVERLAPPED);

	if(INVALID_SOCKET == skTemp)
	{//ͨ����ʾû�������ĳ�ʼ��WinSock����

		return FALSE;

	}

       ::WSAIoctl(skTemp, SIO_GET_EXTENSION_FUNCTION_POINTER,

                     &funGuid,sizeof(funGuid),&pFun,

                     sizeof(pFun), &dwBytes, NULL,NULL);

       ::closesocket(skTemp);

       return NULL != pFun;

}


BOOL WingDisconnectEx(SOCKET hSocket,LPOVERLAPPED lpOverlapped,DWORD dwFlags,DWORD reserved)
{
	GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
	DWORD dwBytes = 0;
	LPFN_DISCONNECTEX lpfnDisconnectEx;
	
	if( 0 != WSAIoctl(hSocket,SIO_GET_EXTENSION_FUNCTION_POINTER,&GuidDisconnectEx,sizeof(GuidDisconnectEx),&lpfnDisconnectEx,sizeof(lpfnDisconnectEx),&dwBytes,NULL,NULL)){
			return false;
	}
	
	return lpfnDisconnectEx(hSocket,lpOverlapped,/*TF_REUSE_SOCKET*/dwFlags,reserved);
}

BOOL WingAcceptEx(SOCKET sListenSocket,SOCKET sAcceptSocket,PVOID lpOutputBuffer,DWORD dwReceiveDataLength,DWORD dwLocalAddressLength,DWORD dwRemoteAddressLength,LPDWORD lpdwBytesReceived,LPOVERLAPPED lpOverlapped)
{
	LPFN_ACCEPTEX lpfnAcceptEx = NULL;     //AcceptEx����ָ��
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes = 0;
	if( 0 != WSAIoctl(sListenSocket,SIO_GET_EXTENSION_FUNCTION_POINTER,&guidAcceptEx,sizeof(guidAcceptEx),&lpfnAcceptEx,sizeof(lpfnAcceptEx),&dwBytes,NULL,NULL)){
		return false;
	}
	return lpfnAcceptEx( sListenSocket,sAcceptSocket,lpOutputBuffer,dwReceiveDataLength,dwLocalAddressLength,dwRemoteAddressLength,lpdwBytesReceived,lpOverlapped);        
}

void WingGetAcceptExSockaddrs(SOCKET sListenSocket,PVOID lpOutputBuffer,DWORD dwReceiveDataLength,DWORD dwLocalAddressLength,DWORD dwRemoteAddressLength,LPSOCKADDR *LocalSockaddr,LPINT LocalSockaddrLength,LPSOCKADDR *RemoteSockaddr,LPINT RemoteSockaddrLength)
{
	LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs = NULL;
	GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	DWORD dwBytes = 0;
	if( 0 != WSAIoctl(sListenSocket,SIO_GET_EXTENSION_FUNCTION_POINTER,&guidGetAcceptExSockaddrs,sizeof(guidGetAcceptExSockaddrs),&lpfnGetAcceptExSockaddrs,sizeof(lpfnGetAcceptExSockaddrs),&dwBytes,NULL,NULL)){
		return;
	}
	return lpfnGetAcceptExSockaddrs( lpOutputBuffer,dwReceiveDataLength,dwLocalAddressLength, dwRemoteAddressLength, LocalSockaddr, LocalSockaddrLength, RemoteSockaddr, RemoteSockaddrLength);   
}

void wing_socket_throw_error( int error_code ){
	exit(WING_BAD_ERROR);
}

/****************************************************
 * @ Ͷ��һ�� recv
 ****************************************************/
bool wing_socket_post_recv(MYOVERLAPPED* &pMyOL){
	
	pMyOL->m_DataBuf.buf	= pMyOL->m_pBuf;  
	pMyOL->m_DataBuf.len	= DATA_BUFSIZE;  
	pMyOL->m_iOpType		= OPE_RECV;

	DWORD RecvBytes=0;
	DWORD Flags=0;

	int code = WSARecv(pMyOL->m_skClient,&(pMyOL->m_DataBuf),1,&RecvBytes,&Flags,&(pMyOL->m_ol),NULL);
	int error_code =  WSAGetLastError();
	//����һ�� WSARecv ����iocp�¼�
	if(0 != code && WSA_IO_PENDING != error_code){
		return false;
	}
	return true;
}
/****************************************************
 * @ accept�ص� 
 * @ �ص�֮����ҪͶ��һ���µ� accept �Դ��´��¿ͻ�������
 * @ ����Ҫͷ��һ��recv ������������
 ****************************************************/
void wing_socket_on_accept(MYOVERLAPPED* &pMyOL){

	int lenHost		= sizeof(sockaddr_in) + 16;
	int lenClient	= sizeof(sockaddr_in) + 16;

	pMyOL->m_isUsed = WING_SOCKET_IS_ALIVE;
	
	int nRet = ::setsockopt(pMyOL->m_skClient, SOL_SOCKET,SO_UPDATE_ACCEPT_CONTEXT,(const char *)&pMyOL->m_skServer,sizeof(pMyOL->m_skServer));

	
	// ���ó�ʱʱ�� ĳЩ������Э�� �ͺ����� ò��ûЧ��
	if( pMyOL->m_timeout > 0 ){
		::setsockopt(pMyOL->m_skClient, SOL_SOCKET,SO_SNDTIMEO, (const char*)&pMyOL->m_timeout,sizeof(pMyOL->m_timeout));
		::setsockopt(pMyOL->m_skClient, SOL_SOCKET,SO_RCVTIMEO, (const char*)&pMyOL->m_timeout,sizeof(pMyOL->m_timeout));
	}

	linger so_linger;
	so_linger.l_onoff = TRUE;
	so_linger.l_linger = 3; //ǿ��closesocket�� ��������3�붺��ʱ�� ��ֹ���ݶ�ʧ
	setsockopt(pMyOL->m_skClient,SOL_SOCKET,SO_LINGER,(const char*)&so_linger,sizeof(so_linger));
	
	int client_size = sizeof(pMyOL->m_addrClient);  
	ZeroMemory(&pMyOL->m_addrClient,sizeof(pMyOL->m_addrClient));
	getpeername(pMyOL->m_skClient,(SOCKADDR *)&pMyOL->m_addrClient,&client_size);  

	wing_post_queue_msg(WM_ONCONNECT, pMyOL->m_skClient,0);
	wing_socket_post_recv(pMyOL);
}
/****************************************************
 * @ ������Ϣ
 ****************************************************/
void _wing_on_send( MYOVERLAPPED*  pOL){
	/*

			//CloseHandle(PerIOData->OVerlapped.hEvent);
			GlobalFree( perIoData ); 

			perIoData = NULL;
		

			memory_sub();

			wing_post_queue_msg(WM_ONSEND,perHandleData->Socket);*/
}

void wing_socket_on_recv(MYOVERLAPPED*  &pOL){
	//������Ϣ
	RECV_MSG *recv_msg	= new RECV_MSG();   
	ZeroMemory(recv_msg,sizeof(RECV_MSG));

	DWORD len = pOL->m_recvBytes+1;
	recv_msg->msg		= new char[len];
	ZeroMemory(recv_msg->msg,len);

	strcpy_s(recv_msg->msg,len,pOL->m_pBuf);
	recv_msg->len		=  len;
	
	//������Ϣ
	wing_post_queue_msg(WM_ONRECV,pOL->m_skClient,(unsigned long)recv_msg);
}

void wing_socket_on_close(MYOVERLAPPED*  &pMyOL){
	
	wing_post_queue_msg(WM_ONCLOSE,pMyOL->m_skClient);

	WingDisconnectEx(pMyOL->m_skClient,NULL,TF_REUSE_SOCKET,0);

	pMyOL->m_iOpType	= OPE_ACCEPT;        //AcceptEx����
	pMyOL->m_isUsed     = WING_SOCKET_IS_SLEEP;
	ZeroMemory(pMyOL->m_pBuf,sizeof(char)*DATA_BUFSIZE);

	
	int error_code = WingAcceptEx(pMyOL->m_skServer,pMyOL->m_skClient,pMyOL->m_pBuf,0,sizeof(SOCKADDR_IN)+16,sizeof(SOCKADDR_IN)+16,NULL, (LPOVERLAPPED)pMyOL);
	int last_error = WSAGetLastError() ;
	
	//���ﾭ���᷵�� 10022���� �����ҽ������
	if( !error_code && WSAECONNRESET != last_error && ERROR_IO_PENDING != last_error ){
		
		if( INVALID_SOCKET != pMyOL->m_skClient ) 
		{
			unsigned long w_client = pMyOL->m_skClient;
			closesocket(pMyOL->m_skClient);
			
			pMyOL->m_iOpType	= OPE_ACCEPT;        //AcceptEx����
			ZeroMemory(pMyOL->m_pBuf,sizeof(char)*DATA_BUFSIZE);

			SOCKET new_client = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,0,0,WSA_FLAG_OVERLAPPED);
				
			if( INVALID_SOCKET != new_client ) 
			{	
				if( !BindIoCompletionCallback((HANDLE)new_client ,wing_icop_thread,0) )
				{
					closesocket(new_client);
					delete pMyOL;
					pMyOL = NULL; 
					return;
				}
				else
				{
							
					pMyOL->m_skClient = new_client;

					int error_code = WingAcceptEx(pMyOL->m_skServer,pMyOL->m_skClient,pMyOL->m_pBuf,0,sizeof(SOCKADDR_IN)+16,sizeof(SOCKADDR_IN)+16,NULL, (LPOVERLAPPED)pMyOL);
					int last_error = WSAGetLastError() ;
					if( !error_code && WSAECONNRESET != last_error && ERROR_IO_PENDING != last_error ){
	
						closesocket(new_client);
						new_client = pMyOL->m_skClient = INVALID_SOCKET;
						delete pMyOL;
						pMyOL = NULL; 
						return;

					}else{
						//��ӵ�mapӳ�� ������Ҫʹ��
						wing_add_to_sockets_map((unsigned long)new_client,(unsigned long)pMyOL);
						wing_remove_from_sockets_map((unsigned long)w_client);
					}
				}
			}
			else
			{
				delete pMyOL;
				pMyOL = NULL; 
				return;		
			}
		}	

		////�����������Ӧ��Ҫ���������� ����������socketΪ0�˾Ͳ�����
		//����onerror ���½�һ�� socket Ȼ������ pMyOL ���� pMyOL��Ҫɾ��
		wing_post_queue_msg(WM_ONERROR,WING_ERROR_ACCEPT,last_error);
		return;
	}

	//ע�������SOCKET���������ú󣬺�����ٴ�������ɶ˿ڵĲ����᷵��һ��������//�Ĵ����������ֱ�ӱ����Լ���
	::BindIoCompletionCallback((HANDLE)pMyOL->m_skClient,wing_icop_thread, 0);

}


VOID CALLBACK wing_icop_thread(DWORD dwErrorCode,DWORD dwBytesTrans,LPOVERLAPPED lpOverlapped)
{
	//IOCP�ص�����
	if( NULL == lpOverlapped )
	{
		//û�����������
		SleepEx(20,TRUE);//�����óɿɾ���״̬
		return;
	}

	int error_code = WSAGetLastError();
	MYOVERLAPPED*  pOL = CONTAINING_RECORD(lpOverlapped, MYOVERLAPPED, m_ol);

	if( 0 != dwErrorCode ) {
		if( 0 == dwBytesTrans || 10054 == error_code || 64 == error_code){
			wing_socket_on_close(pOL);
		}
	}
	pOL->m_recvBytes = dwBytesTrans;
	switch(pOL->m_iOpType)
	{
		case OPE_ACCEPT: //AcceptEx����
		{//�����ӽ����� SOCKET������� pMyOL->m_skClient
			wing_socket_on_accept(pOL);
		}
		break;
		case OPE_RECV:
		{
			if( 0 == dwBytesTrans || 10054 == error_code || 64 == error_code){
				wing_socket_on_close(pOL);
			} else {
				wing_socket_on_recv(pOL);
			}	
		}
		break;
		case OPE_SEND:
		{
			_wing_on_send(pOL);
		}
		break;

	}

}
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

	
	SOCKET m_sockListen = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); 
	if( INVALID_SOCKET == m_sockListen )
	{
		WSACleanup();
		return INVALID_SOCKET;
	}

	//���� SO_REUSEADDR 
	BOOL bReuse = TRUE;
	BOOL bind_status = ::BindIoCompletionCallback((HANDLE)m_sockListen,wing_icop_thread, 0);
	if( !bind_status )
	{
		closesocket(m_sockListen);
		WSACleanup();
		return INVALID_SOCKET;
	}

	if( 0 != ::setsockopt(m_sockListen,SOL_SOCKET,SO_REUSEADDR,(LPCSTR)&bReuse,sizeof(BOOL)) )
	{
		
	}

	struct sockaddr_in ServerAddress; 
	// ����ַ�ṹ��Ϣ
	ZeroMemory(&ServerAddress, sizeof(ServerAddress)); 
	ServerAddress.sin_family = AF_INET; 
	//ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);                     
	ServerAddress.sin_addr.s_addr = inet_addr(listen_ip);          
	ServerAddress.sin_port = htons(port);   


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
	for( int i=0;i<max_connect;i++){
	
		SOCKET client = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,0,0,WSA_FLAG_OVERLAPPED);
		
		if( INVALID_SOCKET == client ) 
		{	
			continue;
		}

		if( !BindIoCompletionCallback((HANDLE)client ,wing_icop_thread,0) )
		{
			closesocket(client);
			continue;
		}

		MYOVERLAPPED *pMyOL = new MYOVERLAPPED();
		if( NULL == pMyOL )
		{
			closesocket(client);
			continue;
		}

		DWORD dwBytes = 0;
		ZeroMemory(pMyOL,sizeof(MYOVERLAPPED));
		
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
		if( !error_code && WSAECONNRESET != last_error && ERROR_IO_PENDING != last_error ){
			
			//zend_error(E_USER_WARNING,"acceptex error with error code:%ld\r\n",last_error);

			closesocket(client);
			client = pMyOL->m_skClient = INVALID_SOCKET;
			delete pMyOL;
			pMyOL = NULL; 

			continue;
		}
		//��ӵ�mapӳ�� ������Ҫʹ��
		wing_add_to_sockets_map( (unsigned long)client , (unsigned long)pMyOL );
	}

	return m_sockListen;
}

void wing_socket_clear(){
	CloseHandle(m_hIOCompletionPort);
}