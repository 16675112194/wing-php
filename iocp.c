/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend_constants.h"
#include "ext/standard/info.h"

#include "php_iocp.h"


#include <Winsock2.h>
#include "Windows.h"
#include "Winbase.h"

#include "tlhelp32.h"
#include "Psapi.h"
#include "Winternl.h"
#include "Processthreadsapi.h"
#include "Shlwapi.h"
#include "Strsafe.h"
#include "Mmsystem.h"
#include "mstcpip.h"
#include "process.h"
#include <mswsock.h>

#include <ws2tcpip.h>//ipv4 ipv6
//int getaddrinfo( const char *hostname, const char *service, const struct addrinfo *hints, struct addrinfo **result );
//https://msdn.microsoft.com/en-us/library/windows/desktop/ms742203(v=vs.85).aspx

#pragma comment(lib,"Kernel32.lib")
#pragma comment(lib,"Shlwapi.lib")
#pragma comment(lib,"Psapi.lib")
#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,"Ws2_32.lib")

#include "message_queue.h"
#include "iocp_message_queue.h"
#include "iocp_utf8.h"
//#include "iocp.h"
//#include <hash_map>
//#include "iocp_socket_map.h"
extern void iocp_add_to_map( unsigned long socket,unsigned long ovl );
extern unsigned long iocp_get_form_map( unsigned long socket );
extern void iocp_remove_form_map( unsigned long socket );
//extern unsigned int __stdcall  iocp_free_thread( PVOID params );
/* If you declare any globals in php_iocp.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(iocp)
*/

/* True global resources - no need for thread safety here */
static int le_iocp;

//�Զ�����Ϣ
#define WM_ONCONNECT		WM_USER + 60
#define WM_ACCEPT_ERROR		WM_USER + 61
#define WM_ONERROR			WM_USER + 62
#define WM_ONCLOSE			WM_USER + 63
#define WM_ONRECV			WM_USER + 64
#define WM_ONQUIT           WM_USER + 65
#define WM_ONCLOSE_EX		WM_USER + 66
#define WM_ONSEND			WM_USER + 67
#define WM_THREAD_RUN		WM_USER + 71
#define WM_TIMEOUT          WM_USER + 72
#define WM_ADD_CLIENT       WM_USER + 73
#define WM_ONCLOSE2			WM_USER + 75
#define WM_ONBEAT			WM_USER + 76
#define WM_POST_RECV_ERR    WM_USER + 77

#define WING_ERROR_SUCCESS				   1
#define WING_ERROR_CALLBACK_SUCCESS		   0
#define WING_ERROR_PARAMETER_ERROR	      -1
#define WING_ERROR_FAILED			      -2
//#define WING_NOTICE_IGNORE			      -3
#define WING_ERROR_CALLBACK_FAILED        -4
#define WING_ERROR_PROCESS_NOT_EXISTS     -5
#define WING_ERROR_WINDOW_NOT_FOUND       -6
#define WING_ERROR_PROCESS_IS_RUNNING      1


#define DATA_BUFSIZE 1024
#define OP_ACCEPT 1
#define OP_RECV   2
#define OP_SEND   3
//iocp��Ϣ�ṹ��
struct iocp_overlapped{
	OVERLAPPED	m_ol;                          //�첽����
	int			m_iOpType;                     //��������
	SOCKET		m_skServer;                    //�����socket
	SOCKET		m_skClient;                    //�ͻ���
	DWORD		m_recvBytes;                   //���յ���Ϣ����
	char		m_pBuf[DATA_BUFSIZE];          //������Ϣ�Ļ�����
	WSABUF		m_DataBuf;                     //��Ϣ����
	int			m_timeout;                     //���ó�ʱ
	SOCKADDR_IN m_addrClient;                  //�ͻ���ʵ�ʵĵ�ַ
	SOCKADDR_IN m_addrServer;                  //�����ʵ�ʵĵ�ַ
	int			m_isUsed;                      //��־�Ƿ��ѱ�����ʹ�� 1�ѱ����� 0������
	unsigned    m_active;                      //���Ļʱ��
	//LPVOID      m_client;                    //wing_client ����
	int         m_isCrashed;                   //�Ƿ���������Ҫ���� 0�� 1��
	int         m_online;                      //�Ƿ����� 1���� 0������
	int         m_usenum;                      //���ü�����

	//void (*handler)(int,struct tag_socket_data*);   data->handler(res, data);  ����һ���ӿ� ��������ô��
};

struct iocp_send_node{
	SOCKET socket;
	char   *msg;
};

void iocp_onclose( iocp_overlapped*  &pOL );
BOOL iocp_create_client(SOCKET m_sockListen , int timeout);
void iocp_onrecv( iocp_overlapped*  &pOL);

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("iocp.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_iocp_globals, iocp_globals)
    STD_PHP_INI_ENTRY("iocp.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_iocp_globals, iocp_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string (string arg)
   Return a string to confirm that the module is compiled in */


/**
 * @ Ͷ��acceptex
 */
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

/**
 * @ �Ͽ�socket���� socke����
 */
BOOL WingDisconnectEx( SOCKET hSocket , LPOVERLAPPED lpOverlapped , DWORD dwFlags = TF_REUSE_SOCKET , DWORD reserved = 0 )
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


/**
 *@����ص��������ӳ��л��ղ���socket����Ҫ��������socket������������socket��Դ�ľ�������޷�����
 */
void iocp_onerror( iocp_overlapped*  &pOL ){
	
	//char* error_message; 
    //FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, WSAGetLastError(),MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&error_message, 0, NULL );   
	//::MessageBoxA(0,error_message,"error",0);

	CancelIoEx((HANDLE)pOL->m_skClient,&pOL->m_ol);
	shutdown( pOL->m_skClient, SD_BOTH);

	iocp_create_client( pOL->m_skServer , pOL->m_timeout );
	pOL->m_usenum = 0;

	//iocp_remove_form_map �����������socket
}

/**
 * @ Ͷ��һ�� recv ��acceptex ֮�� ֻ��Ҫ����һ��
 */
bool iocp_post_recv( iocp_overlapped* &pOL )
{
	
	pOL->m_DataBuf.buf	= pOL->m_pBuf;  
	pOL->m_DataBuf.len	= DATA_BUFSIZE;  
	pOL->m_iOpType		= OP_RECV;

	DWORD RecvBytes		= 0;
	DWORD Flags			= 0;

	int code		    = WSARecv(pOL->m_skClient,&(pOL->m_DataBuf),1,&RecvBytes,&Flags,&(pOL->m_ol),NULL);
	int error_code	    = WSAGetLastError();

	//����һ�� WSARecv ����iocp�¼�
	if( 0 != code )
	{
		if( WSA_IO_PENDING != error_code ) 
		{
			iocp_post_queue_msg( WM_ONERROR,(unsigned long)pOL, WSAGetLastError(),-1);
			iocp_onerror(pOL);
			return false;
		}
	}
	else
	{
		//recv ���
		iocp_onrecv( pOL );
	}
	return true;
}


/**
 *@iocp�¿ͻ��������¼�
 */
void iocp_onconnect( iocp_overlapped*  &pOL ){
	
	pOL->m_online = 1;
	pOL->m_active = time(NULL);
	
	if( setsockopt( pOL->m_skClient, SOL_SOCKET,SO_UPDATE_ACCEPT_CONTEXT,(const char *)&pOL->m_skServer,sizeof(pOL->m_skServer) ) != 0 )
	{
		//setsockoptʧ��
		iocp_post_queue_msg( WM_ONERROR, (unsigned long)pOL, WSAGetLastError() );
	}

	// ���÷��͡����ճ�ʱʱ�� ��λΪ����
	if( pOL->m_timeout > 0 )
	{
		if( setsockopt( pOL->m_skClient, SOL_SOCKET,SO_SNDTIMEO, (const char*)&pOL->m_timeout,sizeof(pOL->m_timeout)) !=0 )
		{
			//setsockoptʧ��
			iocp_post_queue_msg( WM_ONERROR, (unsigned long)pOL, WSAGetLastError() );
		}
		if( setsockopt( pOL->m_skClient, SOL_SOCKET,SO_RCVTIMEO, (const char*)&pOL->m_timeout,sizeof(pOL->m_timeout)) != 0 )
		{
			//setsockoptʧ��
			iocp_post_queue_msg( WM_ONERROR, (unsigned long)pOL, WSAGetLastError() );
		}
	}
	
	linger so_linger;
	so_linger.l_onoff	= TRUE;
	so_linger.l_linger	= 0; //�ܾ�close wait״̬
	setsockopt( pOL->m_skClient,SOL_SOCKET,SO_LINGER,(const char*)&so_linger,sizeof(so_linger) );

	//��ȡ�ͻ���ip��ַ���˿���Ϣ
	int client_size = sizeof(pOL->m_addrClient);  
	ZeroMemory( &pOL->m_addrClient , sizeof(pOL->m_addrClient) );
	
	if( getpeername( pOL->m_skClient , (SOCKADDR *)&pOL->m_addrClient , &client_size ) != 0 ) 
	{
		//getpeernameʧ��
		iocp_post_queue_msg( WM_ONERROR, (unsigned long)pOL, WSAGetLastError() );
	}

	//keepalive ���� �����п� ����֪����ô�� ������keep alive�Ժ� disconnectex �ᱨ����Ч�ľ����(An invalid handle was specified��)
	//���߳��Լ���������ͻ��iocpҪ�������������⣬δ֪
	int dt		= 1;
	DWORD dw	= 0;
	tcp_keepalive live ;     
	live.keepaliveinterval	= 5000;     //����֮�� �೤ʱ�䷢���޻ ��ʼ���������� ��λΪ���� 
	live.keepalivetime		= 1000;     //�೤ʱ�䷢��һ�������� 1������ 60000 �Դ�����     
	live.onoff				= TRUE;     //�Ƿ��� keepalive

	if( setsockopt( pOL->m_skClient, SOL_SOCKET, SO_KEEPALIVE, (char *)&dt, sizeof(dt) ) != 0 )
	{
		//setsockoptʧ��
		iocp_post_queue_msg( WM_ONERROR, (unsigned long)pOL, WSAGetLastError() );
	}           
	
	if( WSAIoctl(   pOL->m_skClient, SIO_KEEPALIVE_VALS, &live, sizeof(live), NULL, 0, &dw, &pOL->m_ol , NULL ) != 0 )
	{
		//WSAIoctl ����
		iocp_post_queue_msg( WM_ONERROR, (unsigned long)pOL, WSAGetLastError() );
	}

	iocp_post_queue_msg( WM_ONCONNECT, (unsigned long)pOL );
	iocp_post_recv( pOL );
}


void iocp_onclose( iocp_overlapped*  &pOL ){
	
	iocp_post_queue_msg( WM_ONCLOSE , (unsigned long) pOL,WSAGetLastError());
	
	SOCKET m_sockListen = pOL->m_skServer;
	SOCKET m_client     = pOL->m_skClient;
	int timeout         = pOL->m_timeout;

	//socket��������
	if( !WingDisconnectEx( pOL->m_skClient , &pOL->m_ol ) && WSA_IO_PENDING != WSAGetLastError()) {
		iocp_post_queue_msg( WM_ONERROR, (unsigned long)pOL, WSAGetLastError(), -2 );
		iocp_onerror( pOL );
		return;
	}

	pOL->m_online   = 0;                                //����
	pOL->m_active   = time(NULL);                       //���Ļʱ��
	pOL->m_iOpType	= OP_ACCEPT;                        //AcceptEx����
	pOL->m_isUsed   = 0;                                //����socket״̬Ϊ����״̬������������ж���Щsocket��ǰ�ǻ��
	ZeroMemory(pOL->m_pBuf,sizeof(char)*DATA_BUFSIZE);  //����������

	//Ͷ��һ��acceptex
	int error_code     = WingAcceptEx( pOL->m_skServer,pOL->m_skClient,pOL->m_pBuf,0,sizeof(SOCKADDR_IN)+16,sizeof(SOCKADDR_IN)+16,NULL, (LPOVERLAPPED)pOL );
	int last_error     = WSAGetLastError() ;
	
	if( !error_code && ERROR_IO_PENDING != last_error ){
		iocp_post_queue_msg( WM_ONERROR,(unsigned long)pOL, WSAGetLastError(), -3 );
		iocp_onerror(pOL);
	}
}

/**
 * @ �յ��ͻ�����Ϣ
 */
void iocp_onrecv( iocp_overlapped*  &pOL){
	
	pOL->m_active = time(NULL);
	int size = strlen(pOL->m_pBuf)+1;
	char *recvmsg = new char[size];	    //������Ϣ
	ZeroMemory(recvmsg,size);           //����

	strcpy(recvmsg,pOL->m_pBuf);                //��Ϣ����
	//printf("recv:%s\r\n",pOL->m_pBuf);
	ZeroMemory(pOL->m_pBuf,DATA_BUFSIZE);       //����������

	//������Ϣ����Ϣ����
	iocp_post_queue_msg(WM_ONRECV,(unsigned long)pOL, (unsigned long)recvmsg, WSAGetLastError());
}

void iocp_onbeat(iocp_overlapped*  &pOL,DWORD dwErrorCode=0,unsigned long _error_code=0){
	//printf("on beat %ld\r\n",pOL->m_skClient);WM_ONBEAT
	iocp_post_queue_msg(WM_ONBEAT,(unsigned long)pOL, WSAGetLastError(),_error_code);
}

/**
 *@�������
 */
void iocp_onsend( iocp_overlapped*  &pOL , DWORD sendBytes = 0 ){

}

/**
 * @ iocp�����̳߳� BindIoCompletionCallback
 */
VOID CALLBACK icop_worker_thread( DWORD dwErrorCode,DWORD dwBytesTrans,LPOVERLAPPED lpOverlapped )
{

	//int error_code = WSAGetLastError();
	//IOCP�ص�����
	if( NULL == lpOverlapped  )
	{
		//��������������õģ������۲����
		//wing_post_queue_msg(WM_THREAD_RUN,dwErrorCode,error_code);
		//û�����������
		SleepEx(20,TRUE);//�����óɿɾ���״̬
		return;
	}

	

	//���ﻹԭovl
	iocp_overlapped*  pOL = CONTAINING_RECORD(lpOverlapped, iocp_overlapped, m_ol);

	iocp_onbeat( pOL, dwErrorCode, WSAGetLastError() );


	//if( dwErrorCode == ERROR_SUCCESS && ERROR_IO_PENDING == WSAGetLastError() ) {
		//return;
	//}


	/*if( dwErrorCode != ERROR_SUCCESS ) {
		switch( pOL->m_iOpType ) {
		case OP_SEND:
			iocp_onsend( pOL , dwBytesTrans);
			break;
		}
		iocp_onclose(pOL);
		return;
	}*/

	//����ж�ò��ûʲô�� ��Ϊ�߳����� δ��ɵ�io������뵽����
	//if( !HasOverlappedIoCompleted( &pOL->m_ol )  ) {
		//pOL->m_ol.Internal == STATUS_PENDING //io δ���
		//iocp_onbeat( pOL, -1, WSAGetLastError() );
		//return;
	//}

	//unsigned long _error_code =  RtlNtStatusToDosError(dwErrorCode);

	//ERROR_IO_PENDING ��997�� �߳��ڲ���������� ����δ����أ������� ���� �ر��ǿ������������֮��
	//if( ERROR_SUCCESS == dwErrorCode && ERROR_IO_PENDING ==  WSAGetLastError() ) 
	{
		//����
		// iocp_onbeat( pOL, dwErrorCode, WSAGetLastError() );
		// WSASetLastError(ERROR_SUCCESS);
		// return;
	}

	//��������������õģ������۲����
	//wing_post_queue_msg(WM_THREAD_RUN,dwErrorCode,error_code,(unsigned long)pOL);

	if( ERROR_SUCCESS != dwErrorCode        || 
		WSAECONNRESET ==  WSAGetLastError() || 
		ERROR_NETNAME_DELETED ==  WSAGetLastError()  ) {
		//���������жϿͻ��˵��ߵ�
		iocp_onclose(pOL);
		return;
	}

	if( ERROR_IO_PENDING !=  WSAGetLastError() && ERROR_SUCCESS == WSAGetLastError() ) {
		
	}

	
	switch( pOL->m_iOpType )
	{
		case OP_ACCEPT: //AcceptEx���� WSAGetLastError == 997 �ܶ�ʱ�� ����������������ӵ�ʱ�� why��
		{
			//�����ӽ����� SOCKET������� pMyOL->m_skClient
			iocp_onconnect( pOL );
		}
		break;
		case OP_RECV:
		{
			pOL->m_recvBytes = dwBytesTrans;
			//�յ��ŵ���Ϣ ��Ҫ�ж�һ���Ƿ����
			if( 0 == dwBytesTrans || WSAECONNRESET ==  WSAGetLastError() || ERROR_NETNAME_DELETED ==  WSAGetLastError()){
				
				if( ERROR_IO_PENDING !=  WSAGetLastError() )
					iocp_onclose( pOL );
				
			} 
			else{
				pOL->m_recvBytes = dwBytesTrans;
				iocp_onrecv( pOL );
			}	
		}
		break;
		

	}

}


unsigned int __stdcall  iocp_free_thread( PVOID params ){
	/*while( 1 ) {
		hash_map<unsigned long,unsigned long>::iterator it = iocp_sockets_hash_map.begin();
		while( it != iocp_sockets_hash_map.end() ){
			iocp_overlapped *ovl = (iocp_overlapped *)it->second;
			if( (time(NULL)- ovl->m_active) >=5 && ovl->m_usenum <=0 ) {
				iocp_remove_form_map( ovl->m_skClient );
				closesocket( ovl->m_skClient );
				delete ovl;
			}	
		}
		Sleep(5000);
	}*/
	return 0;
}


/**
 *@�����߳�
 */
DWORD WINAPI iocp_begin_send_thread(PVOID *_node) {

	iocp_send_node *node = (iocp_send_node*)_node;

	unsigned long _socket = (unsigned long)node->socket;

	if( SOCKET_ERROR == send( node->socket ,node->msg ,strlen( node->msg ),0)){
		delete[] node->msg;
		delete node;

		//iocp_overlapped *povl = (iocp_overlapped *)iocp_get_form_map( (unsigned long)node->socket );
		iocp_post_queue_msg( WM_ONSEND,_socket, 0 );

		return 0;
	}

	delete[] node->msg;
	delete node;

	//iocp_overlapped *povl = (iocp_overlapped *)iocp_get_form_map( (unsigned long)node->socket );
	iocp_post_queue_msg( WM_ONSEND,_socket, 1 );

	/*WSABUF buf;
	DWORD dwlen, flags=0;
	int ret;

	buf.buf="GET / HTTP/1.0\r\n\r\n";
	buf.len=16;

	// dwlen must be given but we can ignore it.  flags can be the normal recv() flags.
	ret=WSASend(data->sock, &buf, 1, &dwlen, 0, &data->ovl, NULL);
	if(ret!=SOCKET_ERROR || WSAGetLastError()==WSA_IO_PENDING) {
		puts("send began successfully!");
	}
	else {
		free(data);
		puts("unable to begin recv!");
	}*/

	return 1;
}

/**
 *@������Ϣ
 */
BOOL iocp_socket_send( SOCKET socket,char *&msg , BOOL async = true) {

	if( !async ) {
		return SOCKET_ERROR != send( socket , msg ,strlen( msg ), 0 );
	}

	iocp_send_node *node = new iocp_send_node();
	node->socket         = socket;
	unsigned long len    = strlen(msg)+1;
	node->msg            = new char[len];
	
	memset( node->msg, 0, len );
	strcpy( node->msg, msg );

	BOOL ret = QueueUserWorkItem( (LPTHREAD_START_ROUTINE)iocp_begin_send_thread, node, WT_EXECUTEINIOTHREAD);
	if( !ret ) {
		delete[] node->msg;
		delete node;

		//iocp_overlapped *povl = (iocp_overlapped *)iocp_get_form_map( (unsigned long)socket );
		iocp_post_queue_msg( WM_ONSEND,(unsigned long)socket, 0 );

		return 0;
	}
	return 1;
}

/**
 *@����socket�ͻ��ˣ����ӳ�Ԫ�أ�
 */
BOOL iocp_create_client( SOCKET m_sockListen , int timeout ){

	SOCKET client = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,0,0,WSA_FLAG_OVERLAPPED);
	if( INVALID_SOCKET == client ) 
	{	
		return 0;
	}

	if( !BindIoCompletionCallback( (HANDLE)client ,icop_worker_thread,0) )
	{
		closesocket(client);
		return 0;
	}

	iocp_overlapped *povl = new iocp_overlapped();
	if( NULL == povl )
	{
		closesocket(client);
		return 0;
	}

	DWORD dwBytes = 0;
	ZeroMemory(povl,sizeof(iocp_overlapped));
		
	povl->m_ol.Offset     = 0;
	povl->m_ol.OffsetHigh = 0;
	povl->m_iOpType	      = OP_ACCEPT;
	povl->m_skServer      = m_sockListen;
	povl->m_skClient      = client;
	povl->m_timeout	      = timeout;
	povl->m_isUsed        = 0;
	povl->m_active        = 0; 
	povl->m_isCrashed     = 0;
	povl->m_online        = 0;
	povl->m_usenum        = 1;

	int server_size = sizeof(povl->m_addrServer);

	ZeroMemory( &povl->m_addrServer,server_size );
	getpeername( povl->m_skServer, (SOCKADDR *)&povl->m_addrServer, &server_size );  

	int error_code = WingAcceptEx( m_sockListen,povl->m_skClient,povl->m_pBuf,0,sizeof(SOCKADDR_IN)+16,sizeof(SOCKADDR_IN)+16,NULL, (LPOVERLAPPED)povl );
	int last_error = WSAGetLastError() ;
	if( !error_code && ERROR_IO_PENDING != last_error && WSAECONNRESET != last_error) 
	{
			
		closesocket( client );
		client = povl->m_skClient = INVALID_SOCKET;
		delete povl;
		povl = NULL; 

		return 0;
	}

	iocp_add_to_map( (unsigned long)client , (unsigned long)povl );
	return 1;
}

//socket -sclient------------------------------------------------------------
zend_class_entry *wing_sclient_ce;

ZEND_METHOD(wing_sclient,close){

	zval *_online = zend_read_property( wing_sclient_ce ,getThis(),"online",strlen("online"),0 TSRMLS_CC);
	int online    = Z_LVAL_P(_online);

	//����ͻ��������� �����˳�
	if( !online ){
		RETURN_LONG( WING_ERROR_SUCCESS );
		return;
	}

	zend_update_property_long( wing_sclient_ce, getThis(),"online",     strlen("online"),     0                       TSRMLS_CC);
	zval *socket = zend_read_property(wing_sclient_ce,getThis(),"socket",strlen("socket"),0 TSRMLS_CC);
	int isocket = Z_LVAL_P(socket);

	if(isocket<=0) {
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}
	
	iocp_overlapped *povl = (iocp_overlapped *)iocp_get_form_map( isocket );
	//iocp_post_queue_msg( WM_ONCLOSE, (unsigned long)povl );
	iocp_onclose( povl );
	
	RETURN_LONG(WING_ERROR_SUCCESS);
}


/** 
 *@������Ϣ ֧��ͬ�����첽
 */
ZEND_METHOD( wing_sclient,send ){
	
	char *msg    = NULL;
	int  msg_len = 0;
	

	if ( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "s",&msg, &msg_len ) != SUCCESS ) 
	{
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}
	zval *socket = zend_read_property( wing_sclient_ce ,getThis(),"socket",strlen("socket"),0 TSRMLS_CC);
	
	int isocket = Z_LVAL_P(socket);
	if( isocket <= 0 ) 
	{
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}

	if( !iocp_socket_send( (SOCKET)Z_LVAL_P(socket) , msg, 1 )) 
	{
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}
	RETURN_LONG(WING_ERROR_SUCCESS);
	return;
				
}




static zend_function_entry wing_sclient_method[]={
	ZEND_ME( wing_sclient,close, NULL,ZEND_ACC_PUBLIC )
	ZEND_ME( wing_sclient,send,  NULL,ZEND_ACC_PUBLIC )
	{NULL,NULL,NULL}
};

/**
 *@������������ӽ����Ŀͻ��˶���
 */
void iocp_create_wing_sclient(zval *&client , iocp_overlapped *&lpol TSRMLS_DC){
	
	MAKE_STD_ZVAL(  client );
	object_init_ex( client,wing_sclient_ce);
				
	//��ʼ��
	if( lpol ) {
		zend_update_property_string( wing_sclient_ce, client,"sin_addr",   strlen("sin_addr"),   inet_ntoa(lpol->m_addrClient.sin_addr) TSRMLS_CC);
		zend_update_property_long(   wing_sclient_ce, client,"sin_port",   strlen("sin_port"),   ntohs(lpol->m_addrClient.sin_port)     TSRMLS_CC);
		zend_update_property_long(   wing_sclient_ce, client,"sin_family", strlen("sin_family"), lpol->m_addrClient.sin_family          TSRMLS_CC);
		zend_update_property_string( wing_sclient_ce, client,"sin_zero",   strlen("sin_zero"),   lpol->m_addrClient.sin_zero            TSRMLS_CC);
		zend_update_property_long(   wing_sclient_ce, client,"last_active",strlen("last_active"),lpol->m_active                         TSRMLS_CC);
		zend_update_property_long(   wing_sclient_ce, client,"socket",     strlen("socket"),     lpol->m_skClient                       TSRMLS_CC);
		zend_update_property_long(   wing_sclient_ce, client,"online",     strlen("online"),     lpol->m_online                         TSRMLS_CC );
	}

}

zend_bool iocp_is_call_able(zval **var TSRMLS_DC){

	char *error = NULL;
	zend_bool is_call_able = zend_is_callable_ex(*var, NULL, 0, NULL, NULL, NULL, &error TSRMLS_CC);
	if( error ) 
		efree( error );
	return is_call_able ? 1 : 0;

}

void iocp_call_func( zval **func TSRMLS_DC ,int params_count  ,zval **params ) {
	
	if( !iocp_is_call_able( func TSRMLS_CC) ) {
		return;
	}

	zval *retval_ptr = NULL;
	MAKE_STD_ZVAL(retval_ptr);

	if( SUCCESS != call_user_function( EG(function_table),NULL, *func,retval_ptr,params_count,params TSRMLS_CC ) ) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "call user func fail");
	}
	zval_ptr_dtor(&retval_ptr);
}


//------------------------------------------------------------------------------
zend_class_entry *wing_server_ce;

/***
 *@���췽��
 */
ZEND_METHOD(wing_server,__construct){

	char *listen         = "0.0.0.0";   //����ip
	int   listen_len     = 0;           //ip��������
	int   port           = 6998;        //�����˿�
	int   max_connect    = 1000;        //��������� Ҳ���ǲ�������
	int   timeout        = 0;           //�շ���ʱʱ��
	int   active_timeout = 0;           //�೤ʱ�䲻���ʱ
	int   tick           = 0;           //����ʱ��


	if( SUCCESS != zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "|slllll", &listen, &listen_len, &port, &max_connect, &timeout, &active_timeout, &tick ) ) {
		return;
	}
	zend_update_property_string(  wing_server_ce, getThis(), "listen",         strlen("listen"),         listen               TSRMLS_CC);
	zend_update_property_long(    wing_server_ce, getThis(), "port",           strlen("port"),           port                 TSRMLS_CC);
	zend_update_property_long(    wing_server_ce, getThis(), "max_connect",    strlen("max_connect"),    max_connect          TSRMLS_CC);
	zend_update_property_long(    wing_server_ce, getThis(), "timeout",        strlen("timeout"),        timeout              TSRMLS_CC);
	zend_update_property_long(    wing_server_ce, getThis(), "active_timeout", strlen("active_timeout"), active_timeout       TSRMLS_CC);
	zend_update_property_long(    wing_server_ce, getThis(), "tick",           strlen("tick"),           tick                 TSRMLS_CC);

}
/***
 *@���¼��ص�
 */
ZEND_METHOD(wing_server,on){

	char *pro      = NULL;
	int   pro_len  = 0;
	zval *callback = NULL;

	zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "sz", &pro, &pro_len, &callback );
	zend_update_property( wing_server_ce, getThis(), pro,pro_len, callback TSRMLS_CC );
	
}

/***
 *@��ʼ����
 */
ZEND_METHOD(wing_server,start){
	
	//��������
	zval *onreceive      = NULL;
	zval *onconnect      = NULL;
	zval *onclose        = NULL;
	zval *onerror        = NULL;
	zval *ontimeout      = NULL;
	zval *ontick         = NULL;
	zval *onsend         = NULL;

	int port           = 0;
	char *listen_ip    = NULL;
	int timeout        = 0;
	int max_connect    = 1000;
	int active_timeout = 0;
	int tick           = 0;

	MAKE_STD_ZVAL( onreceive );
	MAKE_STD_ZVAL( onconnect );
	MAKE_STD_ZVAL( onclose );
	MAKE_STD_ZVAL( onerror );


	onreceive			    = zend_read_property( wing_server_ce, getThis(),"onreceive",	     strlen("onreceive"),	    0 TSRMLS_CC);
	onconnect			    = zend_read_property( wing_server_ce, getThis(),"onconnect",	     strlen("onconnect"),	    0 TSRMLS_CC);
	onclose				    = zend_read_property( wing_server_ce, getThis(),"onclose",	         strlen("onclose"),         0 TSRMLS_CC);
	onerror				    = zend_read_property( wing_server_ce, getThis(),"onerror",	         strlen("onerror"),         0 TSRMLS_CC);
	ontimeout               = zend_read_property( wing_server_ce, getThis(),"ontimeout",	     strlen("ontimeout"),       0 TSRMLS_CC);
	ontick                  = zend_read_property( wing_server_ce, getThis(),"ontick",	         strlen("ontick"),          0 TSRMLS_CC);
	onsend                  = zend_read_property( wing_server_ce, getThis(),"onsend",	         strlen("onsend"),          0 TSRMLS_CC);

	zval *_listen		    = zend_read_property( wing_server_ce, getThis(),"listen",	         strlen("listen"),		    0 TSRMLS_CC);
	zval *_port			    = zend_read_property( wing_server_ce, getThis(),"port",		         strlen("port"),	        0 TSRMLS_CC);
	zval *_max_connect	    = zend_read_property( wing_server_ce, getThis(),"max_connect",       strlen("max_connect"),     0 TSRMLS_CC);
	zval *_timeout		    = zend_read_property( wing_server_ce, getThis(),"timeout",	         strlen("timeout"),         0 TSRMLS_CC);
	zval *_active_timeout   = zend_read_property( wing_server_ce, getThis(),"active_timeout",    strlen("active_timeout"),  0 TSRMLS_CC);
	zval *_tick             = zend_read_property( wing_server_ce, getThis(),"tick",              strlen("tick"),            0 TSRMLS_CC);


	timeout				= Z_LVAL_P(_timeout);
	listen_ip			= Z_STRVAL_P(_listen);
	port				= Z_LVAL_P(_port);
	max_connect			= Z_LVAL_P(_max_connect);
	active_timeout		= Z_LVAL_P(_active_timeout);
	tick		        = Z_LVAL_P(_tick);

	zend_printf("-------------------------iocp start with------------------------------------\r\n");
	zend_printf("-------------------------ip:%s-------------------------\r\n",listen_ip);
	zend_printf("-------------------------port:%d-------------------------\r\n",port);
	zend_printf("----------------------------------------------------------------------------\r\n");
	//��ʼ����Ϣ����
	iocp_message_queue_init();  

	//---start---------------------------------------------------------
	//��ʼ�������socket ���ʧ�ܷ���INVALID_SOCKET
	WSADATA wsaData; 
	if( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 )
	{
		return; 
	}

	// ����Ƿ�����������汾���׽��ֿ�   
	if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
        WSACleanup();  
        return;  
    }  

	//����sokket
	SOCKET m_sockListen = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); 
	if( INVALID_SOCKET == m_sockListen )
	{
		WSACleanup();
		return;
	}

	//����ɶ˿��̳߳�
	BOOL bReuse      = TRUE;
	BOOL bind_status = ::BindIoCompletionCallback((HANDLE)m_sockListen,icop_worker_thread, 0);
	if( !bind_status )
	{
		closesocket(m_sockListen);
		WSACleanup();
		return;
	}


	HANDLE free_thread = (HANDLE)_beginthreadex(NULL, 0, iocp_free_thread, /*(void*)&active_timeout*/NULL, 0, NULL); 
	CloseHandle( free_thread );

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
		return;
	}  

	// ��ʼ����
	if( 0 != listen( m_sockListen , SOMAXCONN ) )
	{
		closesocket(m_sockListen);
		WSACleanup();
		return;
	}

	//socket ��
	for( int i = 0 ; i < max_connect ; i++ ) 
	{
		iocp_create_client( m_sockListen , timeout );
	
		/*SOCKET client = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,0,0,WSA_FLAG_OVERLAPPED);
		if( INVALID_SOCKET == client ) 
		{	
			continue;
		}

		if( !BindIoCompletionCallback( (HANDLE)client ,icop_worker_thread,0) )
		{
			closesocket(client);
			continue;
		}

		iocp_overlapped *povl = new iocp_overlapped();
		if( NULL == povl )
		{
			closesocket(client);
			continue;
		}

		DWORD dwBytes = 0;
		ZeroMemory(povl,sizeof(iocp_overlapped));
		
		povl->m_iOpType	   = OP_ACCEPT;
		povl->m_skServer   = m_sockListen;
		povl->m_skClient   = client;
		povl->m_timeout	   = timeout;
		povl->m_isUsed     = 0;
		povl->m_active     = 0; 
		povl->m_isCrashed  = 0;
		povl->m_online     = 0;
		povl->m_usenum     = 1;

		int server_size = sizeof(povl->m_addrServer);  
		ZeroMemory(&povl->m_addrServer,server_size);
		getpeername(povl->m_skServer,(SOCKADDR *)&povl->m_addrServer,&server_size);  

		int error_code = WingAcceptEx( m_sockListen,povl->m_skClient,povl->m_pBuf,0,sizeof(SOCKADDR_IN)+16,sizeof(SOCKADDR_IN)+16,NULL, (LPOVERLAPPED)povl );
		int last_error = WSAGetLastError() ;
		if( !error_code && ERROR_IO_PENDING != last_error ) //WSAECONNRESET != last_error
		{
			
			closesocket( client );
			client = povl->m_skClient = INVALID_SOCKET;
			delete povl;
			povl = NULL; 

			continue;
		}
		//��ӵ�hash mapӳ�� ������Ҫʹ��
		//wing_add_to_sockets_map( (unsigned long)client , (unsigned long)pMyOL );*/
	}



	iocp_message_queue_element *msg = NULL;//��Ϣ
	zend_printf("start message loop\r\n");
	while( true )
	{ 
		//��ȡ��Ϣ û�е�ʱ�������
		iocp_message_queue_get(msg);
		zend_printf("message_id:%ld\r\n",msg->message_id);
		switch( msg->message_id ){
		    case WM_ONSEND:
			{
				//unsigned long socket    = msg->wparam;
			    iocp_overlapped *povl   = (iocp_overlapped*)iocp_get_form_map(msg->wparam);
				long send_status        = msg->lparam;
			
				zval *params[2]			= {0};
				
				MAKE_STD_ZVAL( params[0] );
				MAKE_STD_ZVAL( params[1] );

				iocp_create_wing_sclient( params[0] , povl TSRMLS_CC);

				ZVAL_LONG( params[1] , send_status );
				
				zend_try
				{
					iocp_call_func( &onsend TSRMLS_CC , 2 , params );
				}
				zend_catch
				{
					//php�﷨����
				}
				zend_end_try();

				zval_ptr_dtor( &params[0] );
				zval_ptr_dtor( &params[1] );

				zend_printf("WM_ONSEND\r\n");
			}
			break;
			case WM_ONCONNECT:
			{
				iocp_overlapped *povl = (iocp_overlapped*)msg->wparam;
				zend_printf("WM_ONCONNECT %ld\r\n\r\n",povl->m_skClient);

				zval *wing_sclient            = NULL;
				iocp_create_wing_sclient( wing_sclient , povl TSRMLS_CC);
				
				zend_try
				{
					iocp_call_func( &onconnect TSRMLS_CC, 1, &wing_sclient );
				}
				zend_catch
				{
					//php�ű��﷨����
				}
				zend_end_try();

				//�ͷ���Դ
				zval_ptr_dtor( &wing_sclient );
				zend_printf("WM_ONCONNECT\r\n");

			}
			break;
			case WM_ONCLOSE:
			{
				iocp_overlapped *povl = (iocp_overlapped*)msg->wparam;
				zend_printf("WM_ONCLOSE %ld\r\n\r\n",povl->m_skClient);

				zval *wing_sclient            = NULL;
				iocp_create_wing_sclient( wing_sclient , povl TSRMLS_CC);
				
				zend_try
				{
					iocp_call_func( &onclose TSRMLS_CC, 1, &wing_sclient );
				}
				zend_catch
				{
					//php�ű��﷨����
				}
				zend_end_try();

				//�ͷ���Դ
				zval_ptr_dtor( &wing_sclient );
		

			}
			break;
			case WM_ONERROR:
			{
				iocp_overlapped *povl = (iocp_overlapped*)msg->wparam;
				int last_error        = (DWORD)msg->lparam;
				
				zend_printf("WM_ONERROR %ld=>error=>%ld,mark:%ld\r\n\r\n",povl->m_skClient,last_error,msg->eparam);
				
				HLOCAL hlocal     = NULL;
				DWORD systemlocal = MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL);
				BOOL fok          = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER , NULL , last_error, systemlocal , (LPSTR)&hlocal , 0 , NULL );
				if( !fok ) {
					HMODULE hDll  = LoadLibraryEx("netmsg.dll",NULL,DONT_RESOLVE_DLL_REFERENCES);
					if( NULL != hDll ) {
						 fok  = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER , hDll , last_error, systemlocal , (LPSTR)&hlocal , 0 , NULL );
						 FreeLibrary( hDll );
					}
				}

				zval *params[3] = {0};

				MAKE_STD_ZVAL(params[1]);
				MAKE_STD_ZVAL(params[2]);
				
				iocp_create_wing_sclient( params[0] , povl TSRMLS_CC);
				ZVAL_LONG( params[1], msg->lparam );              //�Զ���������

				if( fok && hlocal != NULL ) {

					char *_error_msg = (char*)LocalLock( hlocal );


					char *error_msg = NULL;
					iocp_gbk_to_utf8( _error_msg, error_msg );
					
					if( error_msg )
					{
						ZVAL_STRING( params[2], error_msg, 1 );  //WSAGetLasterror ����
						delete[] error_msg;
					}
					else
					{
						ZVAL_STRING( params[2], _error_msg, 1 );  //WSAGetLasterror ����
					}
					

					zend_try{
						iocp_call_func( &onerror TSRMLS_CC , 3 , params );
					}
					zend_catch{
						//php�﷨����
					}
					zend_end_try();

					LocalFree( hlocal );
					
				}else{
					
					ZVAL_STRING( params[2], "unknow error", 1 );  //WSAGetLasterror ����
					zend_try{
						iocp_call_func( &onerror TSRMLS_CC , 3 , params );
					}
					zend_catch{
						//php�﷨����
					}
					zend_end_try();	
				}

				zval_ptr_dtor( &params[0] );
				zval_ptr_dtor( &params[1] );
				zval_ptr_dtor( &params[2] );

			}
			break;
			case WM_ONRECV:
			{
				iocp_overlapped *povl   = (iocp_overlapped*)msg->wparam;
				char *recv_msg          = (char*)msg->lparam;
				long error_code         = msg->eparam;
				zval *params[2]			= {0};

				zend_printf("WM_ONRECV from %ld=>%s\r\nlast error:%ld\r\n\r\n",povl->m_skClient,recv_msg,error_code);

				
				
				MAKE_STD_ZVAL( params[0] );
				MAKE_STD_ZVAL( params[1] );
				iocp_create_wing_sclient( params[0] , povl TSRMLS_CC);
				ZVAL_STRING( params[1] , recv_msg , 1 );
				
				zend_try
				{
					iocp_call_func( &onreceive TSRMLS_CC , 2 , params );
				}
				zend_catch
				{
					//php�﷨����
				}
				zend_end_try();

				zval_ptr_dtor( &params[0] );
				zval_ptr_dtor( &params[1] );

				delete[] recv_msg;
				recv_msg = NULL;
				zend_printf("WM_ONRECV\r\n");
			}
			break;
			case WM_ONBEAT:
			{
				iocp_overlapped *povl = (iocp_overlapped*)msg->wparam;
				zend_printf("WM_ONBEAT %ld,last error:%ld,error code:%ld,type:%ld\r\n\r\n",povl->m_skClient,msg->lparam,msg->eparam,povl->m_iOpType);
			}
			break;
		}
		delete msg;
		msg = NULL;  
	}
	return;
}


static zend_function_entry wing_server_methods[]={
	ZEND_ME(wing_server,__construct,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	ZEND_ME(wing_server,on,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(wing_server,start,NULL,ZEND_ACC_PUBLIC)
	{NULL,NULL,NULL}
};


/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_iocp_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_iocp_init_globals(zend_iocp_globals *iocp_globals)
{
	iocp_globals->global_value = 0;
	iocp_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(iocp)
{
	zend_class_entry  _wing_server_ce;
	INIT_CLASS_ENTRY( _wing_server_ce,"wing_server", wing_server_methods );
	wing_server_ce = zend_register_internal_class( &_wing_server_ce TSRMLS_CC );
	
	//�¼��ص����� Ĭ��Ϊnull
	zend_declare_property_null(    wing_server_ce,"onreceive",   strlen("onreceive"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(    wing_server_ce,"onconnect",   strlen("onconnect"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(    wing_server_ce,"onclose",     strlen("onclose"),  ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(    wing_server_ce,"onerror",     strlen("onerror"),  ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(    wing_server_ce,"ontimeout",   strlen("ontimeout"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(    wing_server_ce,"ontick",      strlen("ontick"),   ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(    wing_server_ce,"onsend",      strlen("onsend"),   ZEND_ACC_PRIVATE TSRMLS_CC);
	

	//�˿ںͼ���ip��ַ 
	zend_declare_property_long(   wing_server_ce,"port",           strlen("port"),           6998,      ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string( wing_server_ce,"listen",         strlen("listen"),         "0.0.0.0", ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(   wing_server_ce,"max_connect",    strlen("max_connect"),    1000,      ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(   wing_server_ce,"timeout",        strlen("timeout"),        0,         ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(   wing_server_ce,"tick",           strlen("tick"),           0,         ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(   wing_server_ce,"active_timeout", strlen("active_timeout"), 0,         ZEND_ACC_PRIVATE TSRMLS_CC);


	//sclient-------------------------
	zend_class_entry  _wing_sclient_ce;
	INIT_CLASS_ENTRY( _wing_sclient_ce , "wing_sclient" , wing_sclient_method );
	wing_sclient_ce = zend_register_internal_class( &_wing_sclient_ce TSRMLS_CC );

	zend_declare_property_string( wing_sclient_ce,"sin_addr",    strlen("sin_addr"),   "", ZEND_ACC_PUBLIC TSRMLS_CC );
	zend_declare_property_long(   wing_sclient_ce,"sin_port",    strlen("sin_port"),   0,  ZEND_ACC_PUBLIC TSRMLS_CC );
	zend_declare_property_long(   wing_sclient_ce,"sin_family",  strlen("sin_family"), 0,  ZEND_ACC_PUBLIC TSRMLS_CC );
	zend_declare_property_string( wing_sclient_ce,"sin_zero",    strlen("sin_zero"),   "", ZEND_ACC_PUBLIC TSRMLS_CC );
	zend_declare_property_long(   wing_sclient_ce,"socket",      strlen("socket"),     0,  ZEND_ACC_PUBLIC TSRMLS_CC );
	zend_declare_property_long(   wing_sclient_ce,"last_active", strlen("last_active"),0,  ZEND_ACC_PUBLIC TSRMLS_CC );
	zend_declare_property_long(   wing_sclient_ce,"online",      strlen("online"),     0,  ZEND_ACC_PUBLIC TSRMLS_CC );

	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(iocp)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(iocp)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(iocp)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(iocp)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "iocp support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ iocp_functions[]
 *
 * Every user visible function must have an entry in iocp_functions[].
 */
const zend_function_entry iocp_functions[] = {
	//PHP_FE(confirm_iocp_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE_END	/* Must be the last line in iocp_functions[] */
};
/* }}} */

/* {{{ iocp_module_entry
 */
zend_module_entry iocp_module_entry = {
	STANDARD_MODULE_HEADER,
	"iocp",
	iocp_functions,
	PHP_MINIT(iocp),
	PHP_MSHUTDOWN(iocp),
	PHP_RINIT(iocp),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(iocp),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(iocp),
	PHP_IOCP_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_IOCP
ZEND_GET_MODULE(iocp)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
