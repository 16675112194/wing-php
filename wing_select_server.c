
#include "php_wing.h"

//-------wing_select_server-----------------------------------------------------------------------

#define MSGSIZE    10240                       //������Ϣ���泤��
CRITICAL_SECTION select_lock;                  //�¿ͻ������ӽ�������ȫ�ֱ����������ؼ�����
CRITICAL_SECTION close_socket_lock;            //�ͻ��˵����Ƴ�ȫ�ֱ����������ؼ�����
unsigned long wing_select_clients_num = 0;     //�����ӵ�socket����
unsigned long recv_and_send_timeout   = 0;     //recv send ��ʱʱ��
unsigned long close_socket_count      = 0;     //���رյ�socket����
unsigned long max_connection          = 0;     //���������
unsigned long  *close_socket_arr      = NULL;  //���رյ�socket
SOCKET *wing_server_clients_arr       = NULL;  //�����ӵ�socket 65534�����̬�˿���
extern zend_class_entry *wing_sclient_ce;

//��Ҫ�Ƴ���socket�ڵ�
struct CLOSE_ITEM{
	SOCKET socket; //��Ҫ�Ƴ���socket
	int time;      //��Ҫ�Ƴ���socket�����ʱ�䣬5��֮������
};



/**
 *@��ʼ��һЩȫ�ֱ���
 */
void wing_select_server_init( int _max_connection = 65534 ){
	InitializeCriticalSection( &select_lock );
	InitializeCriticalSection( &close_socket_lock );
	wing_server_clients_arr = new SOCKET[ _max_connection ];
	close_socket_arr        = new unsigned long[_max_connection];
	max_connection          = _max_connection;
}
/**
 *@�ͷ�ȫ����Դ
 */
void wing_select_server_clear(){
	delete[] wing_server_clients_arr;
	delete[] close_socket_arr;
	DeleteCriticalSection( &select_lock );
	DeleteCriticalSection( &close_socket_lock );
}

/**
 *@���socket�ͻ��˵�ȫ�ֱ��������¿ͻ������ӽ�������
 */
void wing_select_server_clients_append( SOCKET client ){
	if( wing_select_clients_num >= max_connection ) {
		//�Ѵﵽ���������
		return;
	}
	EnterCriticalSection(&select_lock);
	wing_server_clients_arr[wing_select_clients_num++] = client;
	LeaveCriticalSection(&select_lock);
}
/**
 *@��ȫ�ֱ������Ƴ�socket������ʱ��ִ��
 */
void wing_select_server_clients_remove( int i ){
	EnterCriticalSection(&select_lock);
	wing_server_clients_arr[i] = INVALID_SOCKET;
					
	for( unsigned long f=i; f < wing_select_clients_num-1; f++ )
		wing_server_clients_arr[f] = wing_server_clients_arr[f+1];

	wing_server_clients_arr[wing_select_clients_num-1] = INVALID_SOCKET;
	wing_select_clients_num -- ;
	LeaveCriticalSection(&select_lock);
}


/**
 *@������������ӽ����Ŀͻ��˶���
 */
void select_create_wing_sclient(zval *&client , SELECT_ITEM *&item TSRMLS_DC){
	
	MAKE_STD_ZVAL(  client );
	object_init_ex( client,wing_sclient_ce);
				
	//��ʼ��
	if( item ) {
		zend_update_property_string( wing_sclient_ce, client,"sin_addr",    strlen("sin_addr"),   inet_ntoa(item->addr.sin_addr) TSRMLS_CC );
		zend_update_property_long(   wing_sclient_ce, client,"sin_port",    strlen("sin_port"),   ntohs(item->addr.sin_port)     TSRMLS_CC );
		zend_update_property_long(   wing_sclient_ce, client,"sin_family",  strlen("sin_family"), item->addr.sin_family          TSRMLS_CC );
		zend_update_property_string( wing_sclient_ce, client,"sin_zero",    strlen("sin_zero"),   item->addr.sin_zero            TSRMLS_CC );
		zend_update_property_long(   wing_sclient_ce, client,"last_active", strlen("last_active"),item->active                   TSRMLS_CC );
		zend_update_property_long(   wing_sclient_ce, client,"socket",      strlen("socket"),     item->socket                   TSRMLS_CC );
		zend_update_property_long(   wing_sclient_ce, client,"online",      strlen("online"),     item->online                   TSRMLS_CC );
		zend_update_property_long(   wing_sclient_ce, client,"client_type", strlen("client_type"),CLIENT_SELECT                  TSRMLS_CC );
	}

}

/**
 *@select ģ���¿ͻ��������¼�
 */
void select_onconnect( SELECT_ITEM *&item){

	wing_select_server_clients_append( item->socket );

	item->active = time(NULL);
	item->online = 1;

	SOCKET socket = item->socket;

	// ���÷��͡����ճ�ʱʱ�� ��λΪ����
	if( recv_and_send_timeout > 0 )
	{
		if( setsockopt( socket, SOL_SOCKET,SO_SNDTIMEO, (const char*)&recv_and_send_timeout,sizeof(recv_and_send_timeout)) !=0 )
		{
			//setsockoptʧ��
			iocp_post_queue_msg( WM_ONERROR, (unsigned long)item, WSAGetLastError() );
		}
		if( setsockopt( socket, SOL_SOCKET,SO_RCVTIMEO, (const char*)&recv_and_send_timeout,sizeof(recv_and_send_timeout)) != 0 )
		{
			//setsockoptʧ��
			iocp_post_queue_msg( WM_ONERROR, (unsigned long)item, WSAGetLastError() );
		}
	}
	
	linger so_linger;
	so_linger.l_onoff	= TRUE;
	so_linger.l_linger	= 0; //�ܾ�close wait״̬
	setsockopt( socket,SOL_SOCKET,SO_LINGER,(const char*)&so_linger,sizeof(so_linger) );

	int dt		= 1;
	DWORD dw	= 0;
	tcp_keepalive live ;     
	live.keepaliveinterval	= 5000;     //����֮�� �೤ʱ�䷢���޻ ��ʼ���������� ��λΪ���� 
	live.keepalivetime		= 1000;     //�೤ʱ�䷢��һ�������� 1������ 60000 �Դ�����     
	live.onoff				= TRUE;     //�Ƿ��� keepalive

	if( setsockopt( socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&dt, sizeof(dt) ) != 0 )
	{
		//setsockoptʧ��
		iocp_post_queue_msg( WM_ONERROR, (unsigned long)item, WSAGetLastError() );
	}           
	
	if( WSAIoctl(   socket, SIO_KEEPALIVE_VALS, &live, sizeof(live), NULL, 0, &dw, NULL , NULL ) != 0 )
	{
		//WSAIoctl ����
		iocp_post_queue_msg( WM_ONERROR, (unsigned long)item, WSAGetLastError() );
	}

	iocp_post_queue_msg( WM_ONCONNECT, (unsigned long)item );
}

/**
 *@���߻ص�
 */
void select_onclose( SELECT_ITEM *&item )
{
	if( shutdown( item->socket, SD_BOTH ) != 0 ) {
		iocp_post_queue_msg( WM_ONERROR, (unsigned long)item, WSAGetLastError() );
	}
		
	CLOSE_ITEM *close_item = new CLOSE_ITEM();
	close_item->socket     = item->socket;
	close_item->time       = time(NULL);

	EnterCriticalSection(&close_socket_lock);
	close_socket_arr[close_socket_count++] = (unsigned long)close_item;			
	LeaveCriticalSection(&close_socket_lock);

	iocp_post_queue_msg( WM_ONCLOSE ,(unsigned long)item );
}

/**
 *@�յ���Ϣ�ص�
 */
void select_onrecv( SELECT_ITEM *&item )
{

	//��ȡ�ͻ���ip��ַ���˿���Ϣ
	int client_size = sizeof(item->addr);  
	ZeroMemory( &item->addr , sizeof(item->addr) );
	
	if( getpeername( item->socket , (SOCKADDR *)&item->addr , &client_size ) != 0 ) 
    {
		//getpeernameʧ��
		iocp_post_queue_msg( WM_ONERROR, (unsigned long)item, WSAGetLastError() );
	}

	iocp_post_queue_msg( WM_ONRECV , (unsigned long)item);
}

/**
 *@accept�����߳�
 */
unsigned int __stdcall  wing_select_server_accept( PVOID params ) {

	SOCKET sListen = *(SOCKET*)(params);
	int iaddrSize  = sizeof(SOCKADDR_IN);

	while (TRUE)
	{
		if( wing_select_clients_num >= max_connection ) 
		{
			//�Ѵﵽ���������
			Sleep(10);
			continue;
		}

		SELECT_ITEM *item = new SELECT_ITEM();
		item->online = 1;
		item->active = time(NULL);

		item->socket = accept( sListen, (struct sockaddr *)&item->addr, &iaddrSize );
		if( INVALID_SOCKET == item->socket ) 
		{
			delete item;
			continue;
		}

		select_onconnect( item );
		
	}
	return 0;
}


/**
 *@select�����߳�
 */
unsigned int __stdcall  wing_select_server_worder( PVOID params )
{
	unsigned long  i   = 0;
	unsigned long  ret = 0;
	struct timeval tv  = {1, 0};
	char *szMessage    = new char[MSGSIZE];
	fd_set fdread;
	
	while (TRUE)
	{
		if( wing_select_clients_num <= 0 ) 
		{
			Sleep(10);
			continue;
		}

		FD_ZERO( &fdread );                              //��fdread��ʼ���ռ�

		for (i = 0; i < wing_select_clients_num; i++)
		{
			FD_SET(wing_server_clients_arr[i], &fdread); //��Ҫ�����׽ӿڼ��뵽������
		}

		ret = select( 0, &fdread, NULL, NULL, &tv );     //ÿ��һ��ʱ�䣬���ɶ��Ե��׽ӿ�
		if( ret < 0 ) {                                  //�������ش���
			//iocp_post_queue_msg( WM_ONERROR, (unsigned long)0, WSAGetLastError() );
		}
		if (ret == 0 )                                   //��ʱ����
		{
			continue;
		}

		for ( i = 0; i < wing_select_clients_num; i++ )
		{
			if ( FD_ISSET( wing_server_clients_arr[i], &fdread ) )                           //����ɶ�
			{
				memset( szMessage, 0, MSGSIZE );
				ret = recv( wing_server_clients_arr[i], szMessage, MSGSIZE, 0 );             //������Ϣ

				if (ret == 0 || (ret == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)) //�ͻ��˵���
				{
					SELECT_ITEM *item = new SELECT_ITEM();

					item->online = 0;
					item->active = 0;
					item->socket = wing_server_clients_arr[i];

					select_onclose( item );
					wing_select_server_clients_remove( i );
				}
				else
				{
					if( ret <= 0 ) {
						continue;
					}

					SELECT_ITEM *item = new SELECT_ITEM();

					item->online     = 1;
					item->active     = time(NULL);
					item->socket     = wing_server_clients_arr[i];
					item->recv       = new char[ret+1];
					item->recv_bytes = ret;

					memset( item->recv, 0, ret+1 );
					memcpy( item->recv, szMessage ,ret );

					select_onrecv( item );
				}
			}
		}
	}

	delete[] szMessage;
	return 0;
}


/**
 *@�ͷ�socket��Դ�̣߳�shutdown֮���ӳ�5���ͷţ������ԣ�Ч���ȶ����۹�����
 */
unsigned int __stdcall  wing_close_socket_thread( PVOID params ) {
	
	CLOSE_ITEM *close_item = NULL;
	unsigned long i         = 0;
	unsigned long now_time  = 0;
	
	while( 1 ) {

		if( close_socket_count<=0 ) 
		{
			Sleep(10);
			continue;
		}

		EnterCriticalSection(&close_socket_lock);
		
		for( i=0; i < close_socket_count; i++ ) 
		{
			close_item = (CLOSE_ITEM *)close_socket_arr[i];
			now_time   = time(NULL);

			if( ( now_time-close_item->time ) >= 5 ) 
			{
				closesocket( close_item->socket );                  //����socket������socket��Դ			
				for( unsigned long f = i; f < close_socket_count-1; f++ )  
				{
					close_socket_arr[f] = close_socket_arr[f+1];     //��������
				}
				close_socket_arr[close_socket_count-1] = 0;          //���һ������
				close_socket_count--;                                //Ԫ�ؼ���������
				delete close_item;                                   //ɾ�� �����ڴ�
			}
		}
				
		LeaveCriticalSection(&close_socket_lock);
		Sleep(100);
	}
	return 0;
}

zend_class_entry *wing_select_server_ce;

/***
 *@���췽��
 */
ZEND_METHOD( wing_select_server, __construct )
{

	char *listen         = "0.0.0.0";   //����ip
	int   listen_len     = 0;           //ip��������
	int   port           = 6998;        //�����˿�
	int   max_connect    = 1000;        //��������� Ҳ���ǲ�������
	int   timeout        = 0;           //�շ���ʱʱ��
	int   active_timeout = 0;           //�೤ʱ�䲻���ʱ

	if( SUCCESS != zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "|slllll", &listen, &listen_len, &port, &max_connect, &timeout, &active_timeout ) ) {
		return;
	}
	zend_update_property_string(  wing_select_server_ce, getThis(), "listen",         strlen("listen"),         listen               TSRMLS_CC );
	zend_update_property_long(    wing_select_server_ce, getThis(), "port",           strlen("port"),           port                 TSRMLS_CC );
	zend_update_property_long(    wing_select_server_ce, getThis(), "max_connect",    strlen("max_connect"),    max_connect          TSRMLS_CC );
	zend_update_property_long(    wing_select_server_ce, getThis(), "timeout",        strlen("timeout"),        timeout              TSRMLS_CC );
	zend_update_property_long(    wing_select_server_ce, getThis(), "active_timeout", strlen("active_timeout"), active_timeout       TSRMLS_CC );

}
/***
 *@���¼��ص�
 */
ZEND_METHOD( wing_select_server, on )
{

	char *pro      = NULL;
	int   pro_len  = 0;
	zval *callback = NULL;

	if( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "sz", &pro, &pro_len, &callback ) != SUCCESS ) {
		return;
	}

	zend_update_property( wing_select_server_ce, getThis(), pro, pro_len, callback TSRMLS_CC );
}

/***
 *@��ʼ����
 */
ZEND_METHOD( wing_select_server, start )
{
	
	//��������
	zval *onreceive      = NULL;
	zval *onconnect      = NULL;
	zval *onclose        = NULL;
	zval *onerror        = NULL;
	zval *ontimeout      = NULL;
	zval *onsend         = NULL;

	int port           = 0;
	char *listen_ip    = NULL;
	int active_timeout = 0;

	MAKE_STD_ZVAL( onreceive );
	MAKE_STD_ZVAL( onconnect );
	MAKE_STD_ZVAL( onclose );
	MAKE_STD_ZVAL( onerror );


	onreceive			    = zend_read_property( wing_select_server_ce, getThis(),"onreceive",	     strlen("onreceive"),	    0 TSRMLS_CC);
	onconnect			    = zend_read_property( wing_select_server_ce, getThis(),"onconnect",	     strlen("onconnect"),	    0 TSRMLS_CC);
	onclose				    = zend_read_property( wing_select_server_ce, getThis(),"onclose",	     strlen("onclose"),         0 TSRMLS_CC);
	onerror				    = zend_read_property( wing_select_server_ce, getThis(),"onerror",	     strlen("onerror"),         0 TSRMLS_CC);
	ontimeout               = zend_read_property( wing_select_server_ce, getThis(),"ontimeout",	     strlen("ontimeout"),       0 TSRMLS_CC);
	onsend                  = zend_read_property( wing_select_server_ce, getThis(),"onsend",	     strlen("onsend"),          0 TSRMLS_CC);

	zval *_listen		    = zend_read_property( wing_select_server_ce, getThis(),"listen",	     strlen("listen"),		    0 TSRMLS_CC);
	zval *_port			    = zend_read_property( wing_select_server_ce, getThis(),"port",		     strlen("port"),	        0 TSRMLS_CC);
	zval *_max_connect	    = zend_read_property( wing_select_server_ce, getThis(),"max_connect",    strlen("max_connect"),     0 TSRMLS_CC);
	zval *_timeout		    = zend_read_property( wing_select_server_ce, getThis(),"timeout",	     strlen("timeout"),         0 TSRMLS_CC);
	zval *_active_timeout   = zend_read_property( wing_select_server_ce, getThis(),"active_timeout", strlen("active_timeout"),  0 TSRMLS_CC);


	recv_and_send_timeout = Z_LVAL_P(_timeout);
	listen_ip			  = Z_STRVAL_P(_listen);
	port				  = Z_LVAL_P(_port);
	max_connection		  = Z_LVAL_P(_max_connect);
	active_timeout		  = Z_LVAL_P(_active_timeout);

	zend_printf("==================== wing select server %s ====================\r\n", PHP_WING_VERSION );
	

	//---start---------------------------------------------------------
	//��ʼ�������socket ���ʧ�ܷ���INVALID_SOCKET
	WSADATA wsaData; 
	if( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 )
	{
		return; 
	}

	// ����Ƿ�����������汾���׽��ֿ�   
	if( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 )
	{
        WSACleanup(); 
        return;  
    }  

	//����sokket
	SOCKET m_sockListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if( INVALID_SOCKET == m_sockListen )
	{
		WSACleanup();
		return;
	}

	BOOL bReuse = 1;
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
	

	iocp_message_queue_init();  //��ʼ����Ϣ����
	wing_select_server_init();  //��ʼ��һЩȫ�ֱ���

	HANDLE _thread;

	//����accept�¿ͻ��˵��߳�
	_thread=(HANDLE)_beginthreadex(NULL, 0, wing_select_server_accept, (void*)&m_sockListen, 0, NULL); 
	if( !_thread ) 
	{
		closesocket(m_sockListen);
		WSACleanup();
		iocp_message_queue_clear();
		wing_select_server_clear();
		return;
	}
	CloseHandle( _thread );
	
	//����select���߳�
	 _thread = (HANDLE)_beginthreadex( NULL, 0, wing_select_server_worder, NULL, 0, NULL ); 
	if( !_thread ) 
	{
		closesocket(m_sockListen);
		WSACleanup();
		iocp_message_queue_clear();
		wing_select_server_clear();
		return;
	}
	CloseHandle( _thread );

	//�����ͷ�socket��Դ���߳�
	 _thread = (HANDLE)_beginthreadex (NULL, 0, wing_close_socket_thread, NULL, 0, NULL ); 
	if( !_thread ) 
	{
		closesocket(m_sockListen);
		WSACleanup();
		iocp_message_queue_clear();
		wing_select_server_clear();
		return;
	}
	CloseHandle( _thread );

	SELECT_ITEM *item               = NULL;  //��Ϣ����
	zval *wing_sclient              = NULL;  //sclient���ӵ�����˵Ŀͻ���
	iocp_message_queue_element *msg = NULL;  //��Ϣ�ڵ�

	while( true )
	{ 
		//��ȡ��Ϣ û�е�ʱ�������
		iocp_message_queue_get(msg);

		switch( msg->message_id )
		{
		    case WM_ONSEND:
			{
				
				SOCKET send_socket     = (SOCKET)msg->wparam;
				long   send_status     = msg->lparam;

				item = new SELECT_ITEM();

				item->online  = 1;
				item->active  = time(NULL);
				item->socket  = send_socket;
				int iaddrSize = sizeof(SOCKADDR_IN);
				memset(&item->addr,0,iaddrSize);

				if( INVALID_SOCKET != item->socket )
				{
					getpeername( item->socket , (SOCKADDR *)&item->addr , &iaddrSize ); 
				}
				
				zval *send_params[2]	= {0};
				
				MAKE_STD_ZVAL( send_params[1] );

				//ʵ����һ������
				select_create_wing_sclient( send_params[0] , item TSRMLS_CC);

				ZVAL_LONG( send_params[1] , send_status );
				
				zend_try
				{
					iocp_call_func( &onsend TSRMLS_CC , 2 , send_params );
				}
				zend_catch
				{
					//php�﷨����
				}
				zend_end_try();

				zval_ptr_dtor( &send_params[0] );
				zval_ptr_dtor( &send_params[1] );


				delete item;
				item = NULL;
			}
			break;
			case WM_ONCONNECT:
			{
				item =  (SELECT_ITEM*)msg->wparam;
				
				//wing_sclient ʵ����һ������
				select_create_wing_sclient( wing_sclient , item TSRMLS_CC);
				
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

				wing_sclient = NULL;

				delete item;
				item = NULL;

			}
			break;
			case WM_ONCLOSE:
			{
				item =  (SELECT_ITEM*)msg->wparam;
				
				select_create_wing_sclient( wing_sclient , item TSRMLS_CC);
				
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

				wing_sclient = NULL;

				delete item;
				item = NULL;
			}
			break;
			case WM_ONRECV:
			{
				item =  (SELECT_ITEM*)msg->wparam;
			
				zval *recv_params[2]			= {0};
				
				MAKE_STD_ZVAL( recv_params[1] );

				select_create_wing_sclient( recv_params[0] , item TSRMLS_CC);


				ZVAL_STRINGL( recv_params[1] , item->recv , item->recv_bytes, 1 );
				
				zend_try
				{
					iocp_call_func( &onreceive TSRMLS_CC , 2 , recv_params );
				}
				zend_catch
				{
					//php�﷨����
				}
				zend_end_try();
				
				zval_ptr_dtor( &recv_params[0] );
				zval_ptr_dtor( &recv_params[1] );


				delete[] item->recv;
				item->recv = NULL;

				delete item;
				item = NULL;
			}
			break;
			case WM_ONERROR:
			{
				//���ﲻ��item����ɾ��
				SELECT_ITEM *item     = (SELECT_ITEM*)msg->wparam;
				int last_error        = (DWORD)msg->lparam;
				
			
				//��ȡ�������Ӧ�Ĵ�������
				HLOCAL hlocal     = NULL;
				DWORD systemlocal = MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL);
				BOOL fok          = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER , NULL , last_error, systemlocal , (LPSTR)&hlocal , 0 , NULL );
				if( !fok ) 
				{
					if( hlocal ) 
					{
						LocalFree( hlocal );
						hlocal = NULL;
					}

					HMODULE hDll  = LoadLibraryEx("netmsg.dll",NULL,DONT_RESOLVE_DLL_REFERENCES);
					if( NULL != hDll ) 
					{
						 fok  = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER , hDll , last_error, systemlocal , (LPSTR)&hlocal , 0 , NULL );
						 FreeLibrary( hDll );
					}
				}

				zval *params[3] = {0};

				MAKE_STD_ZVAL( params[1] );
				MAKE_STD_ZVAL( params[2] );
				
				select_create_wing_sclient( params[0] , item TSRMLS_CC);
				ZVAL_LONG( params[1], msg->lparam );              //�Զ���������

				if( fok && hlocal != NULL ) 
				{

					char *gbk_error_msg   = (char*)LocalLock( hlocal );
					char *utf8_error_msg  = NULL;

					//��gbkת��Ϊutf8
					iocp_gbk_to_utf8( gbk_error_msg, utf8_error_msg );
					
					if( utf8_error_msg )
					{
						ZVAL_STRING( params[2], utf8_error_msg, 1 );  //WSAGetLasterror ����
						delete[] utf8_error_msg;
					}
					else
					{
						ZVAL_STRING( params[2], gbk_error_msg, 1 );  //WSAGetLasterror ����
					}
			
					LocalFree( hlocal );
					
				}else{
					
					ZVAL_STRING( params[2], "unknow error", 1 );     //WSAGetLasterror ����
					
				}

				zend_try
				{
					iocp_call_func( &onerror TSRMLS_CC , 3 , params );
				}
				zend_catch
				{
					//php�﷨����
				}
				zend_end_try();	

				
				zval_ptr_dtor( &params[0] );
				zval_ptr_dtor( &params[1] );
				zval_ptr_dtor( &params[2] );
			}
			break;
		}

		delete msg;
		msg = NULL;  
	}

	closesocket(m_sockListen);
	WSACleanup();
	iocp_message_queue_clear();
	wing_select_server_clear();
	return;
}

