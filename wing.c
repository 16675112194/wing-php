/*
  +----------------------------------------------------------------------+
  | WING PHP Version 1.0.0                                                        |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: xiaoan Huang                                                             |
  +----------------------------------------------------------------------+
  | Email: 297341015@qq.com
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
#include "php_wing.h"



#include "lib/wing_msg_queue.h"
#include "lib/wing_socket.h"
#include "lib/wing_lib.h"


#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3) || (PHP_MAJOR_VERSION >= 6)
#undef ZVAL_REFCOUNT
#undef ZVAL_ADDREF
#undef ZVAL_DELREF
#define ZVAL_REFCOUNT Z_REFCOUNT_P
#define ZVAL_ADDREF Z_ADDREF_P
#define ZVAL_DELREF Z_DELREF_P
#endif

/* If you declare any globals in php_wing.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(wing)
*/

/* True global resources - no need for thread safety here */
static int le_wing = 0;
char  *PHP_PATH = NULL;
//timer ���̼����� ���ڿ��ƶ��timer�Ĵ���������
static int wing_timer_count = 0;
//�̼߳����� ���ڶ��߳̿���
static int wing_thread_count =0;

/****************************************************************************
 * @ ��������
 * @ param command Ҫ�ڽ�����ִ�е�ָ�� 
 * @ param params_ex ������� ���ڴ�������ɵ��ӽ��� Ĭ��Ϊ NULL�������������
 * @ param params_ex_len ����������� Ĭ��Ϊ0��������Ϊ params_ex = NULL
 ***************************************************************************/
unsigned long create_process(char *command,char *params_ex=NULL,int params_ex_len=0){
	    
	HANDLE m_hRead = NULL;
	HANDLE m_hWrite = NULL;
	STARTUPINFO sui;    
	PROCESS_INFORMATION pi; // �������������ӽ��̵���Ϣ
	SECURITY_ATTRIBUTES sa;   // �����̴��ݸ��ӽ��̵�һЩ��Ϣ
		
	char *params = NULL;
	int params_len = 0;
    
	sa.bInheritHandle = TRUE; // ���ǵ�����������Ѱɣ�����������ӽ��̼̳и����̵Ĺܵ����
	sa.lpSecurityDescriptor = NULL;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	
	if (!CreatePipe(&m_hRead, &m_hWrite, &sa, 0))
	{
		return WING_ERROR_FAILED;
	}

	ZeroMemory(&sui, sizeof(STARTUPINFO)); // ��һ���ڴ������㣬�����ZeroMemory, �����ٶ�Ҫ����memset
	
	sui.cb = sizeof(STARTUPINFO);
	sui.dwFlags	= STARTF_USESTDHANDLES;  
	sui.hStdInput = m_hRead;
	sui.hStdOutput = m_hWrite;
	sui.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		
	if( params_ex_len >0 && params_ex != NULL ){
		DWORD d;
		if(::WriteFile(m_hWrite,params_ex,params_ex_len,&d,NULL)==FALSE){
			//�澯
			zend_error(E_USER_WARNING,"write params to process error");
		}
	}

	if (!CreateProcess(NULL,command, NULL, NULL, TRUE, 0, NULL, NULL, &sui, &pi))
	{
		CloseHandle(m_hRead);
		CloseHandle(m_hWrite);
		return WING_ERROR_FAILED;
	}
		
	CloseHandle(m_hRead);
	CloseHandle(m_hWrite);
	CloseHandle(pi.hProcess); // �ӽ��̵Ľ��̾��
	CloseHandle(pi.hThread); // �ӽ��̵��߳̾����windows�н��̾���һ���̵߳�������ÿ������������һ���߳���ִ��
		
	return pi.dwProcessId;	
}

/********************************************************************************************************
 * @ ���̲���У�飬����ô˵��������������أ���ô˵�ɣ������������ȷ���Ƿ��������ĺ����첽ִ�е�
 ********************************************************************************************************/
void command_params_check(char* &command_params,int *run_process,int *last_value TSRMLS_DC){
	
	zval **argv         = NULL;
	int argc            = 0;
	HashTable *arr_hash = NULL;
	command_params      = (char*)emalloc(1024);

	memset( command_params , 0 ,1024 );
	
	//char *target = NULL;
	//��ȡ�����в���
	if ( zend_hash_find( &EG(symbol_table), "argv", sizeof("argv"), (void**)&argv) == SUCCESS ){

		zval  **data         = NULL;
		HashPosition pointer = NULL;
		arr_hash	         = Z_ARRVAL_PP(argv);
		argc		         = zend_hash_num_elements(arr_hash);

		for( zend_hash_internal_pointer_reset_ex(arr_hash, &pointer); 
			 zend_hash_get_current_data_ex(arr_hash, (void**) &data, &pointer) == SUCCESS; 
			 zend_hash_move_forward_ex(arr_hash, &pointer)
		) {
			
			if( strcmp((char*)Z_LVAL_PP(data),"wing-process") == 0 ){
				*run_process = 1;
			}

			char *key   = NULL;
			int key_len = 0 ;
			int index   = 0;

			zend_hash_get_current_key_ex(arr_hash, &key, (uint*)&key_len, (ulong*)&index, 0, &pointer);

			if( index > 0 ) {
				char *p = (char*)Z_LVAL_PP(data);
				if(p[0]!='\"')
					sprintf(command_params,"%s \"%s\" ",command_params,p);
					//spprintf(&command_params,0,"%s \"%s\" ",command_params,p);
				else 
					sprintf(command_params,"%s %s ",command_params,p);
					//spprintf(&command_params,0,"%s %s ",command_params,p);
			}

			if(index == argc-1&&last_value != NULL){
				 *last_value= atoi((char*)Z_LVAL_PP(data));
			}
		} 
	}
}
//-----------------------function--end---------------------------------------------------



/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("wing.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_wing_globals, wing_globals)
    STD_PHP_INI_ENTRY("wing.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_wing_globals, wing_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/***************************************************************
 * @ ��ȡ wing php�İ汾�� ����ʹ�ó��� WING_VERSION                
 **************************************************************/
PHP_FUNCTION(wing_version){
	char *string = NULL;
    int len = spprintf(&string, 0, "%s", PHP_WING_VERSION);
    RETURN_STRING(string,0);
}


/***************************************************************
 *@wait process���̵ȴ�
 *@param process id ����id
 *@param timeout �ȴ���ʱʱ�� ��λ����
 *@return exit code �����˳���
 ***************************************************************/
PHP_FUNCTION(wing_process_wait){
	
	int thread_id,timeout = INFINITE;
	if( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "l|l", &thread_id, &timeout ) != SUCCESS ) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	if( thread_id<=0 ) {
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}

	HANDLE handle     = OpenProcess(PROCESS_ALL_ACCESS, FALSE, thread_id);
	DWORD wait_result = 0;
	DWORD wait_status = WaitForSingleObject(handle,timeout);
	 
	if( wait_status != WAIT_OBJECT_0 ) {
		CloseHandle( handle );
		RETURN_LONG(wait_status);
		return;
	}
	if( GetExitCodeProcess(handle,&wait_result) == 0 ) {
		CloseHandle( handle );
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}
	CloseHandle( handle );
	RETURN_LONG(wait_result);
	return;
}

/******************************************************************
 *@�������̣߳�ʹ�ý���ģ��
 *****************************************************************/
PHP_FUNCTION( wing_create_thread ){
	
	wing_thread_count++;

	zval *callback   = NULL;
	zval *retval_ptr = NULL;
	
	MAKE_STD_ZVAL( callback );

	if ( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "z", &callback ) != SUCCESS ) {
		zval_ptr_dtor(&callback);
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	char *command_params = NULL;
	int run_process      = 0;
	int command_index    = 0;
	int last_value       = 0;
	char *command        = NULL;
	unsigned long pid    = 0; 

	command_params_check( command_params, &run_process, &last_value TSRMLS_CC );

	spprintf( &command, 0, "%s %s %s wing-process %ld", PHP_PATH , zend_get_executed_filename( TSRMLS_C ), command_params, wing_thread_count );

	if( !run_process ) {
		pid = create_process( command, NULL, 0 );
		
		efree(command);
		efree(command_params);
		zval_ptr_dtor(&callback);

		RETURN_LONG(pid);	
		return;
	}

	if( wing_thread_count != last_value ) {
		
		efree(command);
		efree(command_params);
		zval_ptr_dtor(&callback);

		RETURN_LONG(WING_NOTICE_IGNORE);
		return;
	}
	
	efree(command);
	efree(command_params);
			
	MAKE_STD_ZVAL(retval_ptr);

	if( SUCCESS != call_user_function(EG(function_table),NULL,callback,retval_ptr,0,NULL TSRMLS_CC ) ){
		
		zval_ptr_dtor(&callback);
		zval_ptr_dtor(&retval_ptr);

		RETURN_LONG(WING_ERROR_CALLBACK_FAILED);
		return;
	}

	zval_ptr_dtor(&callback);
	zval_ptr_dtor(&retval_ptr);
			
	RETURN_LONG(WING_CALLBACK_SUCCESS);
}



/*************************************************************************
 *@get data from create process
 *@�Ӹ����̻�ȡ���� ����ʹ����һ��С���������ܣ�������
 *return string or null
 *************************************************************************/
ZEND_FUNCTION(wing_get_process_params){
	
	HANDLE m_hRead	 = GetStdHandle(STD_INPUT_HANDLE);
	DWORD data_len	 = 1024;
	int step		 = 1024;
	char *buf		 = new char[data_len];
	DWORD dwRead     = 0;
	DWORD lBytesRead = 0;

	ZeroMemory( buf, data_len );
	
	if( !PeekNamedPipe( m_hRead,buf, data_len, &lBytesRead, 0, 0 ) ) {
		
		delete[] buf;
		buf = NULL;
		RETURN_NULL();
		return;
	}

	if( lBytesRead <= 0 ){

		delete[] buf;
		buf = NULL;
		RETURN_NULL();
		return;
	}

	while( lBytesRead >= data_len ){

		delete[] buf;
		buf = NULL;
		data_len += step;
				
		buf = new char[data_len];
				
		ZeroMemory( buf, data_len );
		if( !PeekNamedPipe( m_hRead,buf, data_len, &lBytesRead, 0, 0 ) ){

			delete[] buf;
			buf = NULL;
			RETURN_NULL();
			return;
		}
	}
				
	if ( ReadFile(m_hRead, buf, lBytesRead+1, &dwRead, NULL ) ) {
		// �ӹܵ��ж�ȡ���� 
		ZVAL_STRINGL( return_value, buf, dwRead, 1 );
		delete[] buf;
		buf = NULL;	
		return;
	}
	RETURN_NULL();
	return;
}


/*****************************************************************
 *@create process ��������
 *@param command path ����·��
 *@param command params �����в���
 ****************************************************************/
PHP_FUNCTION(wing_create_process){

	char *exe           = NULL;   //�������̱���Ŀ�ִ���ļ� һ��Ϊ exe bat�ȿ���ֱ�����е��ļ�
	int	  exe_len       = 0;
	char *params        = NULL;   //��ѡ�����в���
	int	  params_len    = 0;
	char *params_ex     = NULL;   //��Ҫ���ݵ��ӽ����ܵĲ��� ͨ��api wing_get_process_params ��ȡ
	int   params_ex_len = 0;
	char *command       = NULL;   //�������̵Ĳ���
	unsigned long pid   = 0;      //����id

	if ( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "s|ss", &exe, &exe_len, &params, &params_len, &params_ex, &params_ex_len ) != SUCCESS ) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}
	
	spprintf( &command, 0, "%s %s\0", exe, params );

	pid = create_process( command, params_ex, params_ex_len );

	efree(command);
	RETURN_LONG(pid);	
	return;
}



/*******************************************************************
 *@�������̣���һ��php�ļ�ֱ�ӷŵ�һ����������ִ��
 *@param command path ����·��
 *@param command params �����в���
 ********************************************************************/
PHP_FUNCTION( wing_create_process_ex ){
	
	char *params         = NULL;
	int	  params_len     = 0;
	char *params_ex	     = NULL;
	int   params_ex_len  = 0;
	char *command        = NULL;
	int   pid            = 0;

	if ( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &params, &params_len, &params_ex, &params_ex_len ) != SUCCESS ) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}
	
	
	spprintf( &command, 0, "%s %s\0", PHP_PATH, params );
	pid = create_process( command, params_ex, params_ex_len );

	efree(command);
	RETURN_LONG(pid);	
	return;
}


/*******************************************************************
 *@ɱ������
 ******************************************************************/
ZEND_FUNCTION(wing_process_kill)
{
	long process_id = 0;
	if( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC,"l",&process_id ) != SUCCESS ) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE,FALSE,process_id);
    if( hProcess == NULL ){
		RETURN_LONG(WING_ERROR_PROCESS_NOT_EXISTS);
        return;
	}
    if( !TerminateProcess( hProcess , 0 ) ) {
		CloseHandle(hProcess);
		RETURN_LONG(WING_ERROR_FAILED);
        return;
	}
	CloseHandle(hProcess);
    RETURN_LONG(WING_SUCCESS);
	return;
}


/***************************************************************
 *@��ȡ��ǰ����id
 **************************************************************/
ZEND_FUNCTION(wing_get_current_process_id){
	ZVAL_LONG(return_value,GetCurrentProcessId());
}


/***************************************************************
 *@���� 0������������ -1 ��ȡ�������� -2 ��������Ϊ�� 
 *@-3����������ʧ�� long handle�����������ɹ�  
 **************************************************************/
ZEND_FUNCTION(wing_create_mutex){

	char *mutex_name     = NULL;
	int   mutex_name_len = 0;
	
	if( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "s", &mutex_name, &mutex_name_len ) != SUCCESS ) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	if( mutex_name_len <= 0 ){
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	/*---��߽繲���ں˶�����Ҫ�Ĳ��� ���ﲻ��
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;*/

	HANDLE m_hMutex =  CreateMutex(NULL,TRUE,mutex_name);//CreateMutex(&sa,TRUE,mutex_name);

    if ( !m_hMutex ) {
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}

    if ( ERROR_ALREADY_EXISTS == GetLastError() ) {
        CloseHandle(m_hMutex);
		RETURN_LONG( ERROR_ALREADY_EXISTS );
        return;
	}

	RETURN_LONG( (long)m_hMutex );
	return;
}


/****************************************************************
 *@�رջ�����
 ****************************************************************/
ZEND_FUNCTION(wing_close_mutex){
	long mutex_handle = 0;
	
	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"l",&mutex_handle) != SUCCESS ) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	if( mutex_handle <= 0 ) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	int status_code = CloseHandle((HANDLE)mutex_handle) ? WING_SUCCESS : WING_ERROR_FAILED ;
	RETURN_LONG( status_code );
	return;
}


/*****************************************************************************************************
 *@�������Ƿ���--ʵ�����岻����Ϊ����id���õ����� �����˳��� ͬ���Ľ���id�������̱�����
 *****************************************************************************************************/
ZEND_FUNCTION(wing_process_isalive)
{
	long process_id = 0;
	HANDLE hProcess = NULL;
	
	if( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC,"l",&process_id) != SUCCESS ) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	if( process_id<= 0 ) {
		RETURN_LONG( WING_ERROR_PARAMETER_ERROR );
		return;
	}

    hProcess = OpenProcess( PROCESS_TERMINATE , FALSE , process_id );
    
	if( hProcess == NULL ){
		RETURN_LONG(WING_ERROR_PROCESS_NOT_EXISTS);
        return;
	}

	CloseHandle( hProcess );
	RETURN_LONG(WING_PROCESS_IS_RUNNING);
	return;
}


/***************************************************************************************************
 *@��ȡ��������
 **************************************************************************************************/
ZEND_FUNCTION(wing_get_env){
	
	char *name     = NULL;
	int   name_len = 0;
	int   size     = 0;
	char *var      = NULL;
	
	if( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC,"s", &name, &name_len ) != SUCCESS ) {
		RETURN_EMPTY_STRING();
		return;
	}

	size = GetEnvironmentVariable(name,NULL,0);

	if( size<=0 || GetLastError() == ERROR_ENVVAR_NOT_FOUND ) {
		RETURN_EMPTY_STRING();
		return;
	}
	
	var = new char[size];  
	
	ZeroMemory( var , size );

	size = GetEnvironmentVariable(name,var,size);

	if (size == 0) {
		delete[] var;
		var = NULL;
		RETURN_EMPTY_STRING();
		return;
	}

	ZVAL_STRINGL( return_value, var, size, 1 );

	delete[] var;
	var = NULL;
	return;
}

/****************************************************************************************************
 * @ ���û����������ӽ��̺͸����̿��Թ������Լ��������̼��ͨ�ŷ�ʽ
 ***************************************************************************************************/
ZEND_FUNCTION(wing_set_env){

	char *name     = NULL;
	zval *value    = NULL;
	int   name_len = 0;
	int   res      = 0;

	MAKE_STD_ZVAL(value);
	
	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sz", &name, &name_len, &value) != SUCCESS ) {
		zval_ptr_dtor( &value );
		RETURN_LONG( WING_ERROR_PARAMETER_ERROR );
		return;
	}
	convert_to_string(value);
	
	res = SetEnvironmentVariableA( name,(LPCTSTR)Z_STRVAL_P(value) ) ? WING_SUCCESS : WING_ERROR_FAILED;
	zval_ptr_dtor(&value);

	RETURN_LONG( res );
}


/**************************************************************************************************
 * @ ��ȡһ���������ڵľ����ļ�·��
 * @ ����˵��ȡphp�İ�װĿ¼������wing php�������ֱ��ʹ�ó��� WING_PHP �������php�İ�װ·��
 *************************************************************************************************/
ZEND_FUNCTION( wing_get_command_path ){ 

	char *name     = 0;
	int   name_len = 0;
	char *path     = NULL;

	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len ) != SUCCESS ) {
		RETURN_EMPTY_STRING();
		return;
	}
	
	path = (char*)emalloc(MAX_PATH);
	
	get_command_path( (const char*)name, path );
	
	RETURN_STRING(path,0);
	return;
}


/*************************************************************************************************
 *@ͨ��WM_COPYDATA���ͽ��̼���Ϣ ֻ�ܷ������ڳ���
 *@ע��ֻ�ܸ����ڳ�����Ϣ
 ************************************************************************************************/
ZEND_FUNCTION(wing_send_msg){
	
	zval *console_title = NULL;
	zval *message_id    = NULL;
	zval *message       = NULL;
	HWND  hwnd          = NULL;

	MAKE_STD_ZVAL( console_title );
	MAKE_STD_ZVAL( message_id );
	MAKE_STD_ZVAL( message );

	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"zzz",&console_title,&message_id,&message ) != SUCCESS) {

		zval_ptr_dtor( &console_title );
		zval_ptr_dtor( &message_id );
		zval_ptr_dtor( &message );

		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	convert_to_string( console_title );
	convert_to_long( message_id );
	convert_to_string( message );

	hwnd = FindWindow( Z_STRVAL_P(console_title), NULL );

	if( hwnd == NULL ) {

		zval_ptr_dtor( &console_title );
		zval_ptr_dtor( &message_id );
		zval_ptr_dtor( &message );
		RETURN_LONG(WING_ERROR_WINDOW_NOT_FOUND);
		return;
	}
	

	COPYDATASTRUCT CopyData; 

	CopyData.dwData	= Z_LVAL_P( message_id );  
	CopyData.cbData	= Z_STRLEN_P( message );  
	CopyData.lpData = Z_STRVAL_P( message );  //WM_COPYDATA
	
	SendMessageA( hwnd, WM_COPYDATA, NULL, (LPARAM)&CopyData );
	
	long status = GetLastError() == 0 ? WING_SUCCESS:WING_ERROR_FAILED;

	zval_ptr_dtor( &console_title );
	zval_ptr_dtor( &message_id );
	zval_ptr_dtor( &message );

	RETURN_LONG(status);
	return;
}


/**********************************************************************
 *@���ڹ��� ��������
 ********************************************************************/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		ExitProcess(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
/*********************************************************************
 *@����һ������ ��������
 *********************************************************************/
ZEND_FUNCTION( wing_create_window ){
	
	zval *console_title = NULL;

	MAKE_STD_ZVAL(console_title);

	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&console_title) != SUCCESS ) {
		zval_ptr_dtor(&console_title);
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}
	convert_to_string( console_title );
	
	HINSTANCE hInstance;
	WNDCLASSEX wcex;

	
	hInstance   = GetModuleHandle(NULL);
	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;//LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32PROJECT1));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;//MAKEINTRESOURCE(IDC_WIN32PROJECT1);
	wcex.lpszClassName	= Z_STRVAL_P(console_title);
	wcex.hIconSm		= NULL;//LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassEx(&wcex);

	HWND hWnd = CreateWindowA(Z_STRVAL_P(console_title),Z_STRVAL_P(console_title), WS_OVERLAPPEDWINDOW,CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	ShowWindow( hWnd, 1 );
    UpdateWindow( hWnd );
	zval_ptr_dtor( &console_title );
	RETURN_LONG( (long)hWnd );
	return;
}
/********************************************************************************
 *@����һ������
 ********************************************************************************/
ZEND_FUNCTION(wing_destory_window){
	long hwnd = 0;
	
	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"l",&hwnd ) != SUCCESS ){
		RETURN_LONG( WING_ERROR_PARAMETER_ERROR );
		return;
	}

	if( hwnd <= 0 ) {
		RETURN_LONG( WING_ERROR_PARAMETER_ERROR );
		return;
	}

	long  status = DestroyWindow((HWND)hwnd) ? WING_SUCCESS : WING_ERROR_FAILED;
	RETURN_LONG(status);
}
/*******************************************************************************
 *@������Ϣѭ�� �������ڱ��� ����
 ******************************************************************************/
ZEND_FUNCTION(wing_message_loop){
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
/*******************************************************************************
 *@�򵥵���Ϣ����
 *******************************************************************************/
ZEND_FUNCTION(wing_message_box){

	char *content = NULL;
	int   c_len   = 0;
	int   t_len   = 0; 
	char *title   = NULL;

	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"ss", &content, &c_len, &title, &t_len) != SUCCESS ) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}
	MessageBox(0,content,title,0);
	RETURN_NULL();
}
/********************************************************************************
 *@��ȡ������Ĵ���
 ********************************************************************************/
ZEND_FUNCTION(wing_get_last_error){
	RETURN_LONG(GetLastError());
}


//---------------------����review��������----------------
/********************************************************************************
 *@���뼶��ʱ��
 *@author yuyi
 *@created 2016-05-15
 ********************************************************************************/
ZEND_FUNCTION(wing_timer){
	wing_timer_count++;
	zval *callback;
	zval *dwMilliseconds;
	MAKE_STD_ZVAL(callback);
	MAKE_STD_ZVAL(dwMilliseconds);

	int max_run_times = 0;
	//�������ʱ��Ϊ1�� 1000 0000��
	long accuracy = 10000;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz|lll",&dwMilliseconds, &callback,&max_run_times,&accuracy) != SUCCESS) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}
	convert_to_long_ex(&dwMilliseconds);

	char *command_params	= "";
	int run_process			= 0;
	int command_index		= 0;
	int last_value			= 0;
	
	command_params_check(command_params,&run_process,&last_value TSRMLS_CC);

	char *command = NULL;
	 
	
	spprintf(&command, 0, "%s %s %s wing-process %ld",PHP_PATH, zend_get_executed_filename(TSRMLS_C),command_params,wing_timer_count);

	if(!run_process){
		unsigned long pid = create_process(command,NULL,0);
		efree(command);
		RETURN_LONG(pid);	
		return;
	}

	if(wing_timer_count!=last_value){
		efree(command);
		RETURN_LONG(WING_NOTICE_IGNORE);
		return;
	}

	efree(command);

	HANDLE hTimer = NULL;
    LARGE_INTEGER liDueTime;
	zval *retval_ptr;
	int times=0;

	LONGLONG time = (-1)*accuracy*Z_LVAL_P(dwMilliseconds);

    //�������ʱ��Ϊ1�� 10000000��
	liDueTime.QuadPart = time;
	char *timername = NULL;
	spprintf(&timername,0,"wing_waitable_timer-%s",create_guid());
    //������ʱ����
    hTimer = CreateWaitableTimer(NULL, TRUE, timername);
	efree(timername);

    if(!hTimer)
    {       
		RETURN_LONG(WING_ERROR_FAILED);	
		return;
    }
 
    if (!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0))
    {         
        CloseHandle(hTimer);
		RETURN_LONG(WING_ERROR_FAILED);	
        return;
    }
 
    //�ȶ�ʱ�����źš�
	while(true)
	{
		if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0)
		{
			CloseHandle(hTimer);
			RETURN_LONG(WING_ERROR_FAILED);	
			return;
		}
		else
		{
			//ʱ�ӵ��
            //SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);//��hTimer��Ϣ����Ϊ���źţ������Ȼ�ͻ᲻�ϵ����

			MAKE_STD_ZVAL(retval_ptr);
			if(SUCCESS != call_user_function(EG(function_table),NULL,callback,retval_ptr,0,NULL TSRMLS_CC)){
				zval_ptr_dtor(&retval_ptr);
				RETURN_LONG(WING_ERROR_FAILED);	
				return;
			}
			
			zval_ptr_dtor(&retval_ptr);

			if(max_run_times>0){
				times++;
				if(times>=max_run_times)break;
			}


			 if (!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0))
			 {         
               CloseHandle(hTimer);
			   RETURN_LONG(WING_ERROR_FAILED);	
               return;
			 }

		}  
	}
	SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);
    CloseHandle(hTimer);
	RETURN_LONG(WING_SUCCESS);	
}


/*****************************************************************************************
 * @ socket
 *****************************************************************************************/

//iocp �������߳�
ZEND_FUNCTION(wing_service){

	zval *onreceive = NULL;
	zval *onconnect = NULL;
	zval *onclose = NULL;
	zval *onerror = NULL;
	zval *service_params = NULL;
	int port = 0;
	char *listen_ip = NULL;
	int timeout = 0;
	int max_connect = 1000;

	MAKE_STD_ZVAL(onreceive);
	MAKE_STD_ZVAL(onconnect);
	MAKE_STD_ZVAL(onclose);
	MAKE_STD_ZVAL(onerror);


	//������ȡ
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z",&service_params) != SUCCESS) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	//���������������
	if(Z_TYPE_P(service_params) != IS_ARRAY){
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}
	
	HashTable *arr_hash = Z_ARRVAL_P(service_params);
	int argc = zend_hash_num_elements(arr_hash);
	zval  **data = NULL;
	HashPosition pointer = NULL;
			
	//�����������
	for( zend_hash_internal_pointer_reset_ex(arr_hash, &pointer); 
		 zend_hash_get_current_data_ex(arr_hash, (void**) &data, &pointer) == SUCCESS; 
		 zend_hash_move_forward_ex(arr_hash, &pointer)) 
	{
        char *key = NULL;
        int key_len = 0;
        long index = 0;
        
		if ( zend_hash_get_current_key_ex(arr_hash, &key, (uint*)&key_len, (ulong*)&index, 0, &pointer) == HASH_KEY_IS_STRING) 
		{
			if( strcmp(key,"port") == 0 )
			{
				port = Z_LVAL_PP(data);
			}
			else if(strcmp(key,"listen")==0)
			{
				listen_ip = Z_STRVAL_PP(data);
			}
			else if(strcmp(key,"onreceive")==0)
			{
				onreceive = *data;
			}
			else if(strcmp(key,"onconnect")==0)
			{
				onconnect = *data;
			}
			else if(strcmp(key,"onclose")==0)
			{
				onclose = *data;
			}
			else if(strcmp(key,"onerror")==0)
			{
				onerror = *data;
			}
			else if(strcmp(key,"timeout")==0)
			{
				timeout = Z_LVAL_PP(data);
			}
			else if(strcmp(key,"max_connect")==0)
			{
				max_connect = Z_LVAL_PP(data);
			}
        } 
    } 

	//��ʼ����Ϣ����
	wing_msg_queue_init();  

	//��ʼ�������socket ���ʧ�ܷ���INVALID_SOCKET
	SOCKET m_sockListen = wing_socket_init((const char *)listen_ip,(const int)port,(const int)max_connect,(const int) timeout);
	if( INVALID_SOCKET == m_sockListen ) 
	{
		wing_socket_clear();
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}
	
	//��Ϣ��������
	wing_msg_queue_element *msg = NULL;//��Ϣ
	zend_printf("sockets pool %ld\r\n",wing_get_sockets_map_size());

	while( true )
	{ 
		//��ȡ��Ϣ û�е�ʱ�������
		wing_msg_queue_get(&msg);
		zend_printf("sockets pool %ld\r\n",wing_get_sockets_map_size());

		//������ϢID���в�ͬ�Ĵ���
		switch(msg->message_id){
			
			case WM_ONCONNECT:
			{
				//�µ�����
				//zend_printf("===================================new connect===================================\r\n");
				
				zval *params[5] = {0};
				zval *retval_ptr = NULL;

				unsigned long socket_connect = (unsigned long)msg->wparam;
				MYOVERLAPPED *lpol = (MYOVERLAPPED *)wing_get_from_sockets_map(socket_connect);

				MAKE_STD_ZVAL(params[0]);
				MAKE_STD_ZVAL(params[1]);
				MAKE_STD_ZVAL(params[2]);
				MAKE_STD_ZVAL(params[3]);
				MAKE_STD_ZVAL(params[4]);

				ZVAL_LONG(params[0],(long)msg->wparam);//socket��Դ
				ZVAL_STRING(params[1],inet_ntoa(lpol->m_addrClient.sin_addr),1);//ip
				ZVAL_LONG(params[2],ntohs(lpol->m_addrClient.sin_port));//port
				ZVAL_LONG(params[3],lpol->m_addrClient.sin_family);//Э������
				ZVAL_STRING(params[4],lpol->m_addrClient.sin_zero,1);//���zero��֪������� ����Ҳֱ��֧�ַ���

				MAKE_STD_ZVAL(retval_ptr);
				
				zend_try{
					//ͨ���ص� �������Ϣ���ظ�php
					if( SUCCESS != call_user_function(EG(function_table),NULL,onconnect,retval_ptr,5,params TSRMLS_CC ) ){
						zend_error(E_USER_WARNING,"onconnect call_user_function fail");
					}
				}
				zend_catch{
					zend_printf("php syntax error\r\n");
				}
				zend_end_try();

				//�ͷ���Դ
				zval_ptr_dtor(&retval_ptr);
				zval_ptr_dtor(&params[0]);
				zval_ptr_dtor(&params[1]);
				zval_ptr_dtor(&params[2]);
				zval_ptr_dtor(&params[3]);
				zval_ptr_dtor(&params[4]);
						  
			}
			break;
			case WM_ONSEND:{
				//zend_printf("onsend\r\n");
					//GetProcessMemoryInfo(handle, &pmc, sizeof(pmc));
					//zend_printf("size-onsend:%d\r\n",pmc.WorkingSetSize-msg->size);	   
			}break;
			//Ŀǰ��ʱû���õ� ������
			case WM_ACCEPT_ERROR:
			{
				//zend_printf("onaccepterror\r\n");
				//zend_printf("accept error\r\n");
				//GetProcessMemoryInfo(handle, &pmc, sizeof(pmc));
				//zend_printf("size-accepterror:%d\r\n",pmc.WorkingSetSize-msg->size);	
			}
			break;
			//�յ���Ϣ
			case WM_ONRECV:
			{
				
				//zend_printf("===================================onrecv===================================\r\n");	
				///
				RECV_MSG *temp = (RECV_MSG*)msg->lparam;
				SOCKET client  = (SOCKET)msg->wparam;

				zval *params[2] = {0};
				zval *retval_ptr = NULL;

				MAKE_STD_ZVAL(params[0]);
				MAKE_STD_ZVAL(params[1]);
				MAKE_STD_ZVAL(retval_ptr);

				ZVAL_LONG(params[0],client);
				ZVAL_STRINGL(params[1],temp->msg,temp->len,1);

				zend_try{
					if( SUCCESS != call_user_function(EG(function_table),NULL,onreceive,retval_ptr,2,params TSRMLS_CC) ){
						zend_error(E_USER_WARNING,"onreceive call_user_function fail");
					}
				}
				zend_catch{
					zend_printf("php syntax error\r\n");
				}
				zend_end_try();

				zval_ptr_dtor(&retval_ptr);
				zval_ptr_dtor(&params[0]);
				zval_ptr_dtor(&params[1]);

				delete[] temp->msg;
				temp->msg = NULL;

				delete temp;
				temp = NULL;
			}
			break;
			//���� _close_socket ����������ر�socket
			case WM_ONCLOSE_EX:{
				//zend_printf("===================================onclose ex===================================\r\n");	

				unsigned long socket_to_close = (unsigned long)msg->wparam;
				MYOVERLAPPED *lpol = (MYOVERLAPPED *)wing_get_from_sockets_map(socket_to_close);
				wing_socket_on_close(lpol);
			}break;
			
			//�ͻ��˵�����
			case WM_ONCLOSE:
			{
				//zend_printf("===================================onclose===================================\r\n");	
				
				//�Ǹ��ͻ��˵�����
				SOCKET client =(SOCKET)msg->wparam;

				zval *params = NULL;
				zval *retval_ptr = NULL;

				MAKE_STD_ZVAL(params);
				ZVAL_LONG(params,(long)client);
				MAKE_STD_ZVAL(retval_ptr);
	 
				zend_try{
					if( SUCCESS != call_user_function(EG(function_table),NULL,onclose,retval_ptr,1,&params TSRMLS_CC ) ){
						zend_error(E_USER_WARNING,"WM_ONCLOSE call_user_function fail\r\n");
					}
				}
				zend_catch{
					zend_printf("php syntax error\r\n");
				}
				zend_end_try();
							 
				zval_ptr_dtor(&retval_ptr);
				zval_ptr_dtor(&params);

			}
			break; 
			//�������� Ŀǰ��ʱҲ��û���õ�
			case WM_ONERROR:{
				
				//zend_printf("===============onerror===============r\n");	
				zval *params[3] = {0};
				zval *retval_ptr = NULL;

				MAKE_STD_ZVAL(params[0]);
				MAKE_STD_ZVAL(params[1]);
				MAKE_STD_ZVAL(params[2]);

				ZVAL_LONG(params[0],(long)msg->eparam);//���������socket
				ZVAL_LONG(params[1],(long)msg->wparam);//�Զ���������
				ZVAL_LONG(params[2],(long)msg->lparam);//WSAGetLasterror ������

				MAKE_STD_ZVAL(retval_ptr);
				
				zend_try{
					if( SUCCESS != call_user_function(EG(function_table),NULL,onerror,retval_ptr,3,params TSRMLS_CC ) ){
						zend_error(E_USER_WARNING,"onerror call_user_function fail");
					}	
				}
				zend_catch{
					zend_printf("php syntax error\r\n");
				}
				zend_end_try();

				zval_ptr_dtor(&retval_ptr);
				zval_ptr_dtor(&params[0]);
				zval_ptr_dtor(&params[1]);
				zval_ptr_dtor(&params[2]);	
			}
			break;
			//�˳����� ��ʱû�в���
			case WM_ONQUIT:
			{
				//��������
				wing_msg_queue_clear();
				
				delete msg;
				msg = NULL;

				RETURN_LONG(WING_SUCCESS);
				return;
			}break;

		}

		delete msg;
		msg = NULL;  
    } 
	wing_socket_clear();
	RETURN_LONG( WING_SUCCESS );
	return;
}
/***********************************
 * @ֹͣ����
 ***********************************/
ZEND_FUNCTION(wing_service_stop){
	wing_post_queue_msg(WM_ONQUIT);
	RETURN_LONG(WING_SUCCESS);
}

/********************************************
 * @ �ر�socket
 * @ param socket
 ********************************************/
ZEND_FUNCTION(wing_close_socket){

	zval *socket = NULL;
	
	if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z",&socket) != SUCCESS ) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	convert_to_long(socket);
	wing_post_queue_msg(WM_ONCLOSE_EX,Z_LVAL_P(socket));

	RETURN_LONG(WING_SUCCESS);
}

/*****************************************
 * @��ȡsocket��Ϣ��ip Э�� �˿� ��
 * @return array //GetAcceptExSockaddrs
 ****************************************/
ZEND_FUNCTION(wing_socket_info){

	zval *socket = NULL;
	MAKE_STD_ZVAL(socket);
	if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z",&socket) != SUCCESS) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	convert_to_long(socket);
	SOCKADDR_IN addr_conn;  
	int nSize = sizeof(addr_conn);  
	
	//memset((void *)&addr_conn,0,sizeof(addr_conn)); 
	ZeroMemory(&addr_conn,sizeof(addr_conn));
	getpeername((SOCKET)Z_LVAL_P(socket),(SOCKADDR *)&addr_conn,&nSize);  
  
	array_init(return_value);
	add_assoc_string(return_value,"sin_addr",inet_ntoa(addr_conn.sin_addr),1);
    add_assoc_long(return_value,"sin_family",addr_conn.sin_family);
	add_assoc_long(return_value,"sin_port",ntohs(addr_conn.sin_port));
	add_assoc_string(return_value,"sin_zero",addr_conn.sin_zero,1);

	return;
}
/*****************************************
 * @ ����socket����
 * @ ͬ�����ͽӿ� û��ʹ��iocp
 ****************************************/
ZEND_FUNCTION(wing_socket_send_msg)
{  

	zval *socket;
	zval *msg;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz",&socket,&msg) != SUCCESS) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}
	convert_to_long(socket);

	//�˴�û��ʹ����ɶ˿� ��ɶ˿ڷ��ͺ� ���ֱ�ӵ��� socketclose 
	//�ر�socket �пӣ������û����ڴ�й©
	if( SOCKET_ERROR == send((SOCKET)Z_LVAL_P(socket),Z_STRVAL_P(msg),Z_STRLEN_P(msg),0)){
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}
	
	RETURN_LONG(WING_SUCCESS);
	return;
}  
/***************************************************
 * @ ʹ��iocp�첽������Ϣ--δ����
 ***************************************************/ 
ZEND_FUNCTION(wing_socket_send_msg_ex){
	zval *socket = NULL;
	zval *msg	 = NULL;
	int close_after_send = 0;//������ر�socket Ĭ��Ϊfalse �� ���� ��û����

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz|l",&socket,&msg,&close_after_send) != SUCCESS) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}
	convert_to_long(socket);

	
	SOCKET sClient		= (SOCKET)Z_LVAL_P(socket);
	char *sendbuf		= Z_STRVAL_P(msg);
	ulong bufsize		= Z_STRLEN_P(msg);
	ulong  Flag			= 0;  
	ulong SendByte		= 0;  

	WSABUF DataBuf;
	WSAOVERLAPPED SendOverlapped;

    if ( sClient == INVALID_SOCKET || sendbuf == NULL || bufsize == 0 )
	{
		RETURN_LONG(WING_ERROR_FAILED);
		return;  
	}
   
	
	SecureZeroMemory((PVOID) & SendOverlapped, sizeof (WSAOVERLAPPED));
	SendOverlapped.hEvent = WSACreateEvent();
    if (SendOverlapped.hEvent == NULL) 
	{
        RETURN_LONG(WING_ERROR_FAILED);
		return;  
    }


	DataBuf.buf	= sendbuf; 
	DataBuf.len	= bufsize; 
	
	int bRet  = WSASend(sClient,&(DataBuf),1,&SendByte,Flag,&(SendOverlapped),NULL);  
	if( bRet != 0 &&  WSAGetLastError() != WSA_IO_PENDING ){
		WSAResetEvent(SendOverlapped.hEvent);
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}

	bRet = WSAWaitForMultipleEvents(1, &SendOverlapped.hEvent, TRUE, INFINITE,
                                      TRUE);
	if ( bRet == WSA_WAIT_FAILED) {
		WSAResetEvent(SendOverlapped.hEvent);
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}

	bRet = WSAGetOverlappedResult(sClient, &SendOverlapped, &SendByte,FALSE, &Flag);
	if ( bRet == FALSE) 
	{
		WSAResetEvent(SendOverlapped.hEvent);
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}

	WSAResetEvent(SendOverlapped.hEvent);
	RETURN_LONG(WING_SUCCESS);
}

//////////////////////////--socket-end--

/*********************************
 * @��ȡʹ�õ��ڴ���Ϣ 
 * @����ʵ��ռ�õ��ڴ��С
 *********************************/
ZEND_FUNCTION(wing_get_memory_used){

	HANDLE handle = GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(handle, &pmc, sizeof(pmc));
	CloseHandle(handle);
	RETURN_LONG( pmc.WorkingSetSize );
	return;
}
//----------------------wing_client class--------------------------
zend_class_entry *wing_client_ce;

ZEND_METHOD(wing_client,close){
	zval *socket = zend_read_property(wing_client_ce,getThis(),"socket",strlen("socket"),0 TSRMLS_CC);
	int isocket = Z_LVAL_P(socket);
	if(isocket<=0) {
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}
	wing_post_queue_msg(WM_ONCLOSE_EX,isocket);
	RETURN_LONG(WING_SUCCESS);
}
ZEND_METHOD(wing_client,send){
	zval *msg;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z",&msg) != SUCCESS) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}
	zval *socket = zend_read_property(wing_client_ce,getThis(),"socket",strlen("socket"),0 TSRMLS_CC);
	
	int isocket = Z_LVAL_P(socket);
	if(isocket<=0) {
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}

	if( SOCKET_ERROR == send((SOCKET)Z_LVAL_P(socket),Z_STRVAL_P(msg),Z_STRLEN_P(msg),0)){
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}
	RETURN_LONG(WING_SUCCESS);
	return;
				
}
static zend_function_entry wing_client_method[]={
	ZEND_ME(wing_client,close,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(wing_client,send,NULL,ZEND_ACC_PUBLIC)
	{NULL,NULL,NULL}
};



//----------------------wing_server class--------------------------
zend_class_entry *wing_server_ce;
ZEND_METHOD(wing_server,__construct){
	//���췽�� ip �˿� ���������
	zval *listen = NULL;
	//MAKE_STD_ZVAL(listen);
	//ZVAL_STRING(listen,"0.0.0.0",1);

	int port = 6998;
	int max_connect = 1000;
	int timeout = 0;
	int active_timeout = 0;
	int tick = 0;

	if( SUCCESS != zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"zl|llll",&listen,&port,&max_connect,&timeout,&active_timeout,&tick)){
	
	}
	zend_update_property_string(  wing_server_ce,getThis(),"listen",         strlen("listen"),         Z_STRVAL_P(listen)   TSRMLS_CC);
	zend_update_property_long(    wing_server_ce,getThis(),"port",           strlen("port"),           port                 TSRMLS_CC);
	zend_update_property_long(    wing_server_ce,getThis(),"max_connect",    strlen("max_connect"),    max_connect          TSRMLS_CC);
	zend_update_property_long(    wing_server_ce,getThis(),"timeout",        strlen("timeout"),        timeout              TSRMLS_CC);
	zend_update_property_long(    wing_server_ce,getThis(),"active_timeout", strlen("active_timeout"), active_timeout       TSRMLS_CC);
	zend_update_property_long(    wing_server_ce,getThis(),"tick",           strlen("tick"),           tick                 TSRMLS_CC);
}
ZEND_METHOD(wing_server,on){
	zval *pro = NULL;
	zval *callback = NULL;
	zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"zz",&pro,&callback);

	zend_update_property(wing_server_ce,getThis(),Z_STRVAL_P(pro),Z_STRLEN_P(pro),callback TSRMLS_CC);
}


int wing_is_call_able(zval **var TSRMLS_DC){
	 char *error;
	bool is_call_able = zend_is_callable_ex(*var, NULL, 0, NULL, NULL, NULL, &error TSRMLS_CC);
	if(error)efree(error);
	return is_call_able?1:0;
}

void wing_call_func( zval **func TSRMLS_DC ,int params_count = 0 ,zval **params = NULL) {
	if( !wing_is_call_able(func TSRMLS_CC) ) return;
	zval *retval_ptr = NULL;
	MAKE_STD_ZVAL(retval_ptr);
	if( SUCCESS != call_user_function( EG(function_table),NULL,*func,retval_ptr,params_count,params TSRMLS_CC ) )
		zend_error(E_USER_WARNING,"call_user_function fail");
	zval_ptr_dtor(&retval_ptr);
}
void wing_create_wing_client(zval **client , wing_myoverlapped *&lpol TSRMLS_DC){
	
	MAKE_STD_ZVAL( *client );
	object_init_ex( *client,wing_client_ce);
				
	//��ʼ��
	if( lpol ) {
		zend_update_property_string( wing_client_ce,*client,"sin_addr",   strlen("sin_addr"),   inet_ntoa(lpol->m_addrClient.sin_addr) TSRMLS_CC);
		zend_update_property_long(   wing_client_ce,*client,"sin_port",   strlen("sin_port"),   ntohs(lpol->m_addrClient.sin_port)     TSRMLS_CC);
		zend_update_property_long(   wing_client_ce,*client,"sin_family", strlen("sin_family"), lpol->m_addrClient.sin_family          TSRMLS_CC);
		zend_update_property_string( wing_client_ce,*client,"sin_zero",   strlen("sin_zero"),   lpol->m_addrClient.sin_zero            TSRMLS_CC);
		zend_update_property_long(   wing_client_ce,*client,"last_active",strlen("last_active"),lpol->m_active                         TSRMLS_CC);
		zend_update_property_long(   wing_client_ce,*client,"socket",     strlen("socket"),     lpol->m_skClient                       TSRMLS_CC);
	}

}


void wing_event_callback(wing_msg_queue_element *&msg,zval *&callback TSRMLS_DC)
{
	zval *wing_client            = NULL;
	wing_myoverlapped *lpol      = (wing_myoverlapped*) msg->lparam;

	zend_printf("active:%ld\r\n",lpol->m_active);

	wing_create_wing_client( &wing_client , lpol TSRMLS_CC);
				
	zend_try
	{
		wing_call_func( &callback TSRMLS_CC,1,&wing_client );
	}
	zend_catch
	{
		//php�ű��﷨����
	}
	zend_end_try();

	//�ͷ���Դ
	zval_ptr_dtor( &wing_client );
}


ZEND_METHOD(wing_server,start){
	//��������
	zval *onreceive      = NULL;
	zval *onconnect      = NULL;
	zval *onclose        = NULL;
	zval *onerror        = NULL;
	zval *ontimeout      = NULL;
	zval *ontick         = NULL;
	zval *service_params = NULL;

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


	onreceive			    = zend_read_property( wing_server_ce,getThis(),"onreceive",	     strlen("onreceive"),	    0 TSRMLS_CC);
	onconnect			    = zend_read_property( wing_server_ce,getThis(),"onconnect",	     strlen("onconnect"),	    0 TSRMLS_CC);
	onclose				    = zend_read_property( wing_server_ce,getThis(),"onclose",	     strlen("onclose"),         0 TSRMLS_CC);
	onerror				    = zend_read_property( wing_server_ce,getThis(),"onerror",	     strlen("onerror"),         0 TSRMLS_CC);
	ontimeout               = zend_read_property( wing_server_ce,getThis(),"ontimeout",	     strlen("ontimeout"),       0 TSRMLS_CC);
	ontick                  = zend_read_property( wing_server_ce,getThis(),"ontick",	     strlen("ontick"),          0 TSRMLS_CC);

	zval *_listen		    = zend_read_property( wing_server_ce,getThis(),"listen",	     strlen("listen"),		    0 TSRMLS_CC);
	zval *_port			    = zend_read_property( wing_server_ce,getThis(),"port",		     strlen("port"),	        0 TSRMLS_CC);
	zval *_max_connect	    = zend_read_property( wing_server_ce,getThis(),"max_connect",    strlen("max_connect"),     0 TSRMLS_CC);
	zval *_timeout		    = zend_read_property( wing_server_ce,getThis(),"timeout",	     strlen("timeout"),         0 TSRMLS_CC);
	zval *_active_timeout   = zend_read_property( wing_server_ce,getThis(),"active_timeout", strlen("active_timeout"),  0 TSRMLS_CC);
	zval *_tick             = zend_read_property( wing_server_ce,getThis(),"tick",           strlen("tick"),            0 TSRMLS_CC);


	timeout				= Z_LVAL_P(_timeout);
	listen_ip			= Z_STRVAL_P(_listen);
	port				= Z_LVAL_P(_port);
	max_connect			= Z_LVAL_P(_max_connect);
	active_timeout		= Z_LVAL_P(_active_timeout);
	tick		        = Z_LVAL_P(_tick);

	//�����������ʱ���
	if( active_timeout > 0 ) {
		zend_printf("timeout event thread start ...\r\n");
		_beginthreadex(NULL, 0, wing_socket_check_active_timeout, (void*)&active_timeout, 0, NULL);  
	}

	//��ʼ����Ϣ����
	wing_msg_queue_init();  


	//��ʼ�������socket ���ʧ�ܷ���INVALID_SOCKET
	SOCKET m_sockListen = wing_socket_init((const char *)listen_ip,(const int)port,(const int)max_connect,(const int) timeout);
	if( INVALID_SOCKET == m_sockListen ) 
	{
		wing_socket_clear();
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}
	
	wing_msg_queue_element *msg = NULL;//��Ϣ

	while( true )
	{ 
		//��ȡ��Ϣ û�е�ʱ�������
		wing_msg_queue_get(&msg);

		switch( msg->message_id ){
			//�ⲿ����Ҫ����debug�۲�
			case WM_THREAD_RUN:
			{
				MYOVERLAPPED *lpol            = (MYOVERLAPPED*) msg->eparam;
				zend_printf("thread run %ld last error %ld socket=%ld\r\n",msg->wparam,msg->lparam,0);
			}break;
			case WM_ADD_CLIENT:
				{
					MYOVERLAPPED *lpol            = (MYOVERLAPPED*) msg->lparam;
					wing_socket_add_client(lpol);
				}break;
			
			case WM_TIMEOUT:
			{
				zend_printf("timeout event happened\r\n");
				wing_event_callback( msg , ontimeout TSRMLS_CC);
			}
			break;
			case WM_ONCONNECT:
			{
				wing_event_callback( msg , onconnect TSRMLS_CC);	  
			}
			break;
			//�յ���Ϣ
			case WM_ONRECV:
			{
				

				char *recv_msg			= (char*)msg->lparam;
				wing_socket client		= (wing_socket)msg->wparam;
				zval *params[2]			= {0};
				wing_myoverlapped *lpol	= (wing_myoverlapped*)wing_get_from_sockets_map((unsigned long)msg->wparam);

				MAKE_STD_ZVAL( params[1] );
				wing_create_wing_client( &params[0] , lpol TSRMLS_CC);
				ZVAL_STRING( params[1] , recv_msg , 1 );
				
				zend_try
				{
					wing_call_func( &onreceive TSRMLS_CC , 2 , params );
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
			}
			break;
			//����������ر�socket
			case WM_ONCLOSE_EX:{
				//zend_printf("===================================onclose ex===================================\r\n");	

				unsigned long socket_to_close = (unsigned long)msg->wparam;
				MYOVERLAPPED *lpol            = (MYOVERLAPPED*)wing_get_from_sockets_map(socket_to_close);
				wing_socket_on_close(lpol);
			}break;
			
			//�ͻ��˵�����
			case WM_ONCLOSE:
			{
				wing_event_callback( msg , onclose TSRMLS_CC);	
			}
			break; 
			//�������� Ŀǰ��ʱҲ��û���õ�
			case WM_ONERROR:{
				
				//zend_printf("===============onerror===============r\n");	
				zval *params[3] = {0};

				MAKE_STD_ZVAL(params[1]);
				MAKE_STD_ZVAL(params[2]);

				MYOVERLAPPED* lpol = (MYOVERLAPPED *)wing_get_from_sockets_map((unsigned long)msg->wparam);
				
				wing_create_wing_client( &params[0] , lpol TSRMLS_CC);

				ZVAL_LONG(params[1],msg->lparam);  //�Զ���������
				ZVAL_LONG(params[2],msg->eparam);  //WSAGetLasterror ������

				zend_try{
					wing_call_func( &onerror TSRMLS_CC , 3 , params );
				}
				zend_catch{
					//php�﷨����
				}
				zend_end_try();

				zval_ptr_dtor(&params[0]);
				zval_ptr_dtor(&params[1]);
				zval_ptr_dtor(&params[2]);	
			}
			break;
			//�˳����� ��ʱû�в���
			case WM_ONQUIT:
			{
				//��������
				wing_msg_queue_clear();
				
				delete msg;
				msg = NULL;

				RETURN_LONG(WING_SUCCESS);
				return;
			}break;

		}

		delete msg;
		msg = NULL;  
    } 
	wing_socket_clear();
	RETURN_LONG( WING_SUCCESS );
	return;
}

static zend_function_entry wing_server_method[]={
	ZEND_ME(wing_server,__construct,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	ZEND_ME(wing_server,on,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(wing_server,start,NULL,ZEND_ACC_PUBLIC)
	{NULL,NULL,NULL}
};


//----------------------wing_server class--------------------------
/* }}} */
/* The previous line is mant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_wing_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_wing_init_globals(zend_wing_globals *wing_globals)
{
	wing_globals->global_value = 0;
	wing_globals->global_string = NULL;
}
*/
/* }}} */
/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(wing)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/

	zend_class_entry  _wing_client_ce;
	INIT_CLASS_ENTRY( _wing_client_ce , "wing_client" , wing_client_method );
	wing_client_ce = zend_register_internal_class( &_wing_client_ce TSRMLS_CC );

	zend_declare_property_string( wing_client_ce,"sin_addr",    strlen("sin_addr"),   "", ZEND_ACC_PUBLIC TSRMLS_CC );
	zend_declare_property_long(   wing_client_ce,"sin_port",    strlen("sin_port"),   0,  ZEND_ACC_PUBLIC TSRMLS_CC );
	zend_declare_property_long(   wing_client_ce,"sin_family",  strlen("sin_family"), 0,  ZEND_ACC_PUBLIC TSRMLS_CC );
	zend_declare_property_string( wing_client_ce,"sin_zero",    strlen("sin_zero"),   "", ZEND_ACC_PUBLIC TSRMLS_CC );
	zend_declare_property_long(   wing_client_ce,"socket",      strlen("socket"),     0,  ZEND_ACC_PUBLIC TSRMLS_CC );
	zend_declare_property_long(   wing_client_ce,"last_active", strlen("last_active"),0,  ZEND_ACC_PUBLIC TSRMLS_CC );
	


	//----wing_server-----
	zend_class_entry  _wing_server_ce;
	INIT_CLASS_ENTRY( _wing_server_ce,"wing_server", wing_server_method );
	wing_server_ce = zend_register_internal_class( &_wing_server_ce TSRMLS_CC );
	
	//�¼��ص����� Ĭ��Ϊnull
	zend_declare_property_null(    wing_server_ce,"onreceive",   strlen("onreceive"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(    wing_server_ce,"onconnect",   strlen("onconnect"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(    wing_server_ce,"onclose",     strlen("onclose"),  ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(    wing_server_ce,"onerror",     strlen("onerror"),  ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(    wing_server_ce,"ontimeout",   strlen("ontimeout"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(    wing_server_ce,"ontick",      strlen("ontick"),   ZEND_ACC_PRIVATE TSRMLS_CC);

	//�˿ںͼ���ip��ַ 
	zend_declare_property_long(   wing_server_ce,"port",           strlen("port"),           6998,      ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string( wing_server_ce,"listen",         strlen("listen"),         "0.0.0.0", ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(   wing_server_ce,"max_connect",    strlen("max_connect"),    1000,      ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(   wing_server_ce,"timeout",        strlen("timeout"),        0,         ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(   wing_server_ce,"tick",           strlen("tick"),           0,         ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(   wing_server_ce,"active_timeout", strlen("active_timeout"), 0,         ZEND_ACC_PRIVATE TSRMLS_CC);


	//ע�᳣��������ȳ�ʼ������
	//REGISTER_STRING_CONSTANT("WING_VERSION",PHP_WING_VERSION,CONST_CS | CONST_PERSISTENT);
	
	
	PHP_PATH = new char[MAX_PATH];
	if( 0 != GetModuleFileName(NULL,PHP_PATH,MAX_PATH) )
	{
		zend_register_string_constant("WING_PHP",                    sizeof("WING_PHP"),                     PHP_PATH,                      CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);
	} 
	zend_register_string_constant( "WING_VERSION",                   sizeof("WING_VERSION"),                 PHP_WING_VERSION,              CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);
	zend_register_long_constant(   "WING_WAIT_TIMEOUT",              sizeof("WING_WAIT_TIMEOUT"),            WAIT_TIMEOUT,                  CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);
	zend_register_long_constant(   "WING_WAIT_FAILED",               sizeof("WING_WAIT_FAILED"),             WAIT_FAILED,                   CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);
	zend_register_long_constant(   "WING_INFINITE",                  sizeof("WING_INFINITE"),                INFINITE,                      CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);
	zend_register_long_constant(   "WING_WAIT_OBJECT_0",             sizeof("WING_WAIT_OBJECT_0"),           WAIT_OBJECT_0,                 CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);
	zend_register_long_constant(   "WING_WAIT_ABANDONED",            sizeof("WING_WAIT_ABANDONED"),          WAIT_ABANDONED,                CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);
	zend_register_long_constant(   "WING_ERROR_ALREADY_EXISTS",      sizeof("WING_ERROR_ALREADY_EXISTS"),    ERROR_ALREADY_EXISTS,          CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);
	zend_register_long_constant(   "WING_ERROR_PARAMETER_ERROR",     sizeof("WING_ERROR_PARAMETER_ERROR"),   WING_ERROR_PARAMETER_ERROR,    CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);
	zend_register_long_constant(   "WING_ERROR_FAILED",              sizeof("WING_ERROR_FAILED"),            WING_ERROR_FAILED,             CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);
	zend_register_long_constant(   "WING_ERROR_CALLBACK_FAILED",     sizeof("WING_ERROR_CALLBACK_FAILED"),   WING_ERROR_CALLBACK_FAILED,    CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);
	zend_register_long_constant(   "WING_CALLBACK_SUCCESS",          sizeof("WING_CALLBACK_SUCCESS"),        WING_CALLBACK_SUCCESS,         CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);
	zend_register_long_constant(   "WING_ERROR_PROCESS_NOT_EXISTS",  sizeof("WING_ERROR_PROCESS_NOT_EXISTS"),WING_ERROR_PROCESS_NOT_EXISTS, CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);
	zend_register_long_constant(   "WING_SUCCESS",                   sizeof("WING_SUCCESS"),                 WING_SUCCESS,                  CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);	
	zend_register_long_constant(   "WING_PROCESS_IS_RUNNING",        sizeof("WING_PROCESS_IS_RUNNING"),      WING_PROCESS_IS_RUNNING,       CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(wing)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	delete[] PHP_PATH;
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(wing)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(wing)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(wing)
{
	php_info_print_table_start();
	php_info_print_table_header( 2, "wing support",    "enabled"          );
	php_info_print_table_row(    2, "version",         PHP_WING_VERSION   );
	php_info_print_table_row(    2, "author","         yuyi"              );
	php_info_print_table_row(    2, "email",           "297341015@qq.com" );
	php_info_print_table_row(    2, "qq-group",        "535218312"        );
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}

/* }}} */

/* {{{ wing_functions[]
 *
 * Every user visible function must have an entry in wing_functions[].
 */
const zend_function_entry wing_functions[] = {
	PHP_FE(wing_version,NULL)
	PHP_FE(wing_create_thread,NULL)
	PHP_FE(wing_create_process,NULL) //wing_thread_wait
	PHP_FE(wing_get_process_params,NULL)
	PHP_FE(wing_create_process_ex,NULL)
	//PHP_FE(wing_thread_wait,NULL)

	PHP_FE(wing_process_wait,NULL)
	//wing_thread_wait �Ǳ���
	ZEND_FALIAS(wing_thread_wait,wing_process_wait,NULL)

	PHP_FE(wing_process_kill,NULL)
	ZEND_FALIAS(wing_thread_kill,wing_process_kill,NULL)
	ZEND_FALIAS(wing_kill_thread,wing_process_kill,NULL)
	ZEND_FALIAS(wing_kill_timer,wing_process_kill,NULL)
	ZEND_FALIAS(wing_kill_process,wing_process_kill,NULL)

	PHP_FE(wing_process_isalive,NULL)
	ZEND_FALIAS(wing_thread_isalive,wing_process_isalive,NULL)

	PHP_FE(wing_get_current_process_id,NULL)
	PHP_FE(wing_create_mutex,NULL)
	PHP_FE(wing_close_mutex,NULL)
	PHP_FE(wing_get_env,NULL)
	PHP_FE(wing_get_command_path,NULL)
	PHP_FE(wing_set_env,NULL)
	PHP_FE(wing_send_msg,NULL)
	PHP_FE(wing_get_last_error,NULL)
	PHP_FE(wing_create_window,NULL)
	PHP_FE(wing_message_loop,NULL)
	PHP_FE(wing_destory_window,NULL)
	PHP_FE(wing_message_box,NULL)
	PHP_FE(wing_timer,NULL)
	PHP_FE(wing_service,NULL)
	ZEND_FALIAS(wing_socket,wing_service,NULL)
	ZEND_FALIAS(wing_tcp_server,wing_service,NULL)

	PHP_FE(wing_socket_info,NULL)
	PHP_FE(wing_socket_send_msg,NULL)
	PHP_FE(wing_service_stop,NULL)
	PHP_FE(wing_close_socket,NULL)
	PHP_FE(wing_get_memory_used,NULL)
	PHP_FE_END	/* Must be the last line in wing_functions[] */
};
/* }}} */

/* {{{ wing_module_entry
 */
zend_module_entry wing_module_entry = {
	STANDARD_MODULE_HEADER,
	"wing",
	wing_functions,
	PHP_MINIT(wing),
	PHP_MSHUTDOWN(wing),
	PHP_RINIT(wing),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(wing),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(wing),
	PHP_WING_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_WING
ZEND_GET_MODULE(wing)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
