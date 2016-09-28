#include "php_wing.h"

extern char* PHP_PATH;
/**
 *@wait process���̵ȴ�
 *@param process id ����id
 *@param timeout �ȴ���ʱʱ�� ��λ����
 *@return exit code �����˳���
 */
PHP_FUNCTION( wing_process_wait ){
	
	int process_id,timeout = INFINITE;

	if( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "l|l", &process_id, &timeout ) != SUCCESS ) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	if( process_id <= 0 ) {
		RETURN_LONG(WING_ERROR_FAILED);
		return;
	}

	HANDLE handle     = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id );
	DWORD wait_result = 0;
	DWORD wait_status = WaitForSingleObject( handle,timeout );
	 
	if( wait_status != WAIT_OBJECT_0 ) {
		CloseHandle( handle );
		RETURN_LONG( wait_status );
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

/**
 *@get data from create process
 *@�Ӹ����̻�ȡ���� ����ʹ����һ��С���������ܣ�������
 *return string or null
 */
ZEND_FUNCTION( wing_get_process_params ){
	
	HANDLE m_hRead	 = GetStdHandle(STD_INPUT_HANDLE);
	DWORD data_len	 = 1024;
	int step		 = 1024;
	char *buf		 = (char*)emalloc(data_len);
	DWORD dwRead     = 0;
	DWORD lBytesRead = 0;

	ZeroMemory( buf, data_len );
	
	if( !PeekNamedPipe( m_hRead,buf, data_len, &lBytesRead, 0, 0 ) ) {
		
		efree( buf );
		buf = NULL;
		RETURN_NULL();
		return;
	}

	if( lBytesRead <= 0 ){

		efree( buf );
		buf = NULL;
		RETURN_NULL();
		return;
	}

	while( lBytesRead >= data_len ){

		efree( buf );
		buf = NULL;
		data_len += step;
				
		buf = (char*)emalloc(data_len);
				
		ZeroMemory( buf, data_len );
		if( !PeekNamedPipe( m_hRead,buf, data_len, &lBytesRead, 0, 0 ) ){

			efree( buf );
			buf = NULL;
			RETURN_NULL();
			return;
		}
	}
				
	if ( ReadFile( m_hRead, buf, lBytesRead+1, &dwRead, NULL ) ) {
		// �ӹܵ��ж�ȡ���� 
		ZVAL_STRINGL( return_value, buf, dwRead, 1 );
		efree( buf );
		buf = NULL;	
		return;
	}
	RETURN_NULL();
	return;
}



/**
 *@create process ��������
 *@param command path ����·��
 *@param command params �����в���
 */
PHP_FUNCTION( wing_create_process ){

	char *exe           = NULL;   //�������̱���Ŀ�ִ���ļ� һ��Ϊ exe bat�ȿ���ֱ�����е��ļ�
	int	  exe_len       = 0;
	char *output_file     = NULL;   //������������ļ�
	int	  output_file_len = 0;

	if ( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &exe, &exe_len, &output_file, &output_file_len ) != SUCCESS ) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}
	

	//HANDLE m_hRead         = NULL;
	//HANDLE m_hWrite        = NULL;
	STARTUPINFO sui;    
	PROCESS_INFORMATION pi;                        // �������������ӽ��̵���Ϣ
	SECURITY_ATTRIBUTES sa;                        // �����̴��ݸ��ӽ��̵�һЩ��Ϣ
		
	
    
	sa.bInheritHandle       = TRUE;                // ���ǵ�����������Ѱɣ�����������ӽ��̼̳и����̵Ĺܵ����
	sa.lpSecurityDescriptor = NULL;
	sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
	
	/*if (!CreatePipe(&m_hRead, &m_hWrite, &sa, 0))
	{
		RETURN_LONG( WING_ERROR_FAILED );
		return;
	}*/

	
    SECURITY_ATTRIBUTES *psa=NULL;  
    DWORD dwShareMode=FILE_SHARE_READ|FILE_SHARE_WRITE;  
    OSVERSIONINFO osVersion={0};  
    osVersion.dwOSVersionInfoSize =sizeof ( osVersion );  
    if ( GetVersionEx ( &osVersion ) )  
    {  
        if ( osVersion.dwPlatformId ==VER_PLATFORM_WIN32_NT )  
        {  
            psa=&sa;  
            dwShareMode|=FILE_SHARE_DELETE;  
        }  
    }  


	HANDLE hConsoleRedirect=CreateFile (  
                                output_file,  
                                GENERIC_WRITE,  
                                dwShareMode,  
                                psa,  
                                OPEN_ALWAYS,  
                                FILE_ATTRIBUTE_NORMAL,  
                                NULL );  

	ZeroMemory(&sui, sizeof(STARTUPINFO));         // ��һ���ڴ������㣬�����ZeroMemory, �����ٶ�Ҫ����memset
	
	sui.cb         = sizeof(STARTUPINFO);
	sui.dwFlags	   = STARTF_USESTDHANDLES;  
	sui.hStdInput  = NULL;//m_hRead;
	sui.hStdOutput = hConsoleRedirect;//m_hWrite;
	sui.hStdError  = hConsoleRedirect;//GetStdHandle(STD_ERROR_HANDLE);
	sui.wShowWindow= SW_HIDE;

	/*if( params_len >0 ) {	
		DWORD byteWrite  = 0;
		if( ::WriteFile( m_hWrite, params, params_len, &byteWrite, NULL ) == FALSE ) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "write data to process error");
		}
	}*/

	if ( !CreateProcess( NULL,exe, NULL, NULL, TRUE, 0, NULL, NULL, &sui, &pi ) ) {
		  CloseHandle(hConsoleRedirect);
		 // CloseHandle(m_hWrite);
		  RETURN_LONG( WING_ERROR_FAILED );
		  return;
	}
		
	//CloseHandle( m_hRead );
	CloseHandle( hConsoleRedirect );
	CloseHandle( pi.hProcess );  // �ӽ��̵Ľ��̾��
	CloseHandle( pi.hThread );   // �ӽ��̵��߳̾����windows�н��̾���һ���̵߳�������ÿ������������һ���߳���ִ��

	RETURN_LONG(  pi.dwProcessId );	
	return;
}



/**
 *@�������̣���һ��php�ļ�ֱ�ӷŵ�һ����������ִ��
 *@param command path ����·��
 *@param command params �����в���
 */
PHP_FUNCTION( wing_create_process_ex ){
	
	char *php_file       = NULL;
	int	  php_file_len   = 0;
	char *output_file	     = NULL;
	int   output_file_len     = 0;
	char *command        = NULL;

	if ( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &php_file, &php_file_len, &output_file, &output_file_len ) != SUCCESS ) {
		 RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		 return;
	}
	
	
	spprintf( &command, 0, "%s %s\0", PHP_PATH, php_file );

	//HANDLE m_hRead         = NULL;
	//HANDLE m_hWrite        = NULL;
	STARTUPINFO sui;    
	PROCESS_INFORMATION pi;                        // �������������ӽ��̵���Ϣ
	SECURITY_ATTRIBUTES sa;                        // �����̴��ݸ��ӽ��̵�һЩ��Ϣ
		
	
    
	sa.bInheritHandle       = TRUE;                // ���ǵ�����������Ѱɣ�����������ӽ��̼̳и����̵Ĺܵ����
	sa.lpSecurityDescriptor = NULL;
	sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
	
	/*if (!CreatePipe(&m_hRead, &m_hWrite, &sa, 0))
	{
		efree(command);
		RETURN_LONG( WING_ERROR_FAILED );
		return;
	}*/

	SECURITY_ATTRIBUTES *psa=NULL;  
    DWORD dwShareMode=FILE_SHARE_READ|FILE_SHARE_WRITE;  
    OSVERSIONINFO osVersion={0};  
    osVersion.dwOSVersionInfoSize =sizeof ( osVersion );  
    if ( GetVersionEx ( &osVersion ) )  
    {  
        if ( osVersion.dwPlatformId ==VER_PLATFORM_WIN32_NT )  
        {  
            psa=&sa;  
            dwShareMode|=FILE_SHARE_DELETE;  
        }  
    }  


	HANDLE hConsoleRedirect=CreateFile (  
                                output_file,  
                                GENERIC_WRITE,  
                                dwShareMode,  
                                psa,  
                                OPEN_ALWAYS,  
                                FILE_ATTRIBUTE_NORMAL,  
                                NULL );  

	ZeroMemory(&sui, sizeof(STARTUPINFO));         // ��һ���ڴ������㣬�����ZeroMemory, �����ٶ�Ҫ����memset
	
	sui.cb         = sizeof(STARTUPINFO);
	sui.dwFlags	   = STARTF_USESTDHANDLES;  
	sui.hStdInput  = NULL;//m_hRead;
	sui.hStdOutput = hConsoleRedirect;//m_hWrite;
	sui.hStdError  = hConsoleRedirect;//GetStdHandle(STD_ERROR_HANDLE);
		
	/*if( params_len >0 ) {	
		DWORD byteWrite  = 0;
		if( ::WriteFile( m_hWrite, params, params_len, &byteWrite, NULL ) == FALSE ) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "write data to process error");
		}
	}*/

	if ( !CreateProcess( NULL, command , NULL, NULL, TRUE, 0, NULL, NULL, &sui, &pi ) ) {
		  CloseHandle(hConsoleRedirect);
		 // CloseHandle(m_hWrite);
		  efree(command);
		  RETURN_LONG( WING_ERROR_FAILED );
		  return;
	}
		
	CloseHandle( hConsoleRedirect );
	//CloseHandle( m_hWrite );
	CloseHandle( pi.hProcess );  // �ӽ��̵Ľ��̾��
	CloseHandle( pi.hThread );   // �ӽ��̵��߳̾����windows�н��̾���һ���̵߳�������ÿ������������һ���߳���ִ��

	efree(command);
	RETURN_LONG(  pi.dwProcessId );	
	
	return;
}


/**
 *@ɱ������
 */
ZEND_FUNCTION( wing_process_kill )
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
    RETURN_LONG(WING_ERROR_SUCCESS);
	return;
}


/**
 *@��ȡ��ǰ����id
 */
ZEND_FUNCTION( wing_get_current_process_id ){
	ZVAL_LONG( return_value, GetCurrentProcessId() );
}


/**
 *@��ȡwindows�ں˶�������ô���
 *@param int  �ں˶���������php�ڲ���ʾ��Ϊint����
 *@return int
 */
ZEND_FUNCTION( wing_query_object ){

	int handle = 0;

	if( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "l", &handle ) != SUCCESS ) 
	{
		RETURN_LONG(0);
		return;
	}

	RETURN_LONG( WingQueryObject( (HANDLE)handle ) );
	return;
}

/**
 *@���� 0������������ -1 ��ȡ�������� -2 ��������Ϊ�� 
 *@-3����������ʧ�� long handle�����������ɹ�  
 */
ZEND_FUNCTION( wing_create_mutex ) {

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

	//---��߽繲���ں˶�����Ҫ�Ĳ���
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	HANDLE m_hMutex =  CreateMutex( &sa ,TRUE, mutex_name );//CreateMutex(&sa,TRUE,mutex_name);

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

/**
 *@�رջ�����
 */
ZEND_FUNCTION( wing_close_mutex )
{
	long mutex_handle = 0;
	
	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"l",&mutex_handle) != SUCCESS ) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	if( mutex_handle <= 0 ) {
		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	int status_code = CloseHandle( (HANDLE)mutex_handle ) ? WING_ERROR_SUCCESS : WING_ERROR_FAILED ;
	RETURN_LONG( status_code );
	return;
}



void wing_query_process_item( zval *&return_value, PROCESSINFO process )
{

		zval *item;
		MAKE_STD_ZVAL( item );
		array_init( item );

		add_assoc_string(    item, "process_name",   process.process_name,  1 );
		delete[] process.process_name;
				
		add_assoc_string(    item, "command_line",   process.command_line,  1 );
		delete[] process.command_line;
			
		add_assoc_string(    item, "file_name",      process.file_name,     1 );
		delete[] process.file_name;
	
		add_assoc_string(    item, "file_path",      process.file_path,     1 );
		delete[] process.file_path;

		add_assoc_long(      item,"process_id",        process.process_id          );
		add_assoc_long(      item,"parent_process_id", process.parent_process_id   );
		add_assoc_long(      item,"working_set_size",  process.working_set_size    );
		add_assoc_long(      item,"base_priority",     process.base_priority       );
		add_assoc_long(      item,"thread_count",      process.thread_count        );
		add_assoc_long(      item,"handle_count",      process.handle_count        );
		add_assoc_long(      item,"cpu_time",          process.cpu_time            );

		add_next_index_zval( return_value, item );

}

ZEND_FUNCTION( wing_query_process ){

	zval *keyword     = NULL;
	int search_by     = 0;

	MAKE_STD_ZVAL( keyword );
	ZVAL_STRING( keyword ,"",1 );

	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zl", &keyword, &search_by ) != SUCCESS ) 
	{
		if( keyword ) 
			zval_ptr_dtor( &keyword );

		RETURN_EMPTY_STRING();
		return;
	}

	PROCESSINFO *all_process = NULL;
	
	//��һ�δ�null����ʵ�ʵĽ�������
	int count   = WingQueryProcess( all_process , 0 );
	
	all_process = new PROCESSINFO[count];
	count       = WingQueryProcess( all_process , count );

	array_init( return_value );


	for( int i = 0; i < count ; i++ ) 
	{
		if( Z_TYPE_P(keyword) == IS_NULL || Z_STRLEN_P(keyword) == 0  ) 
		{
			wing_query_process_item( return_value, all_process[i] );
		}
		else
		{
			//���������
			if( Z_TYPE_P( keyword ) == IS_LONG || is_numeric_string(Z_STRVAL_P(keyword),Z_STRLEN_P(keyword),NULL,NULL,0) ) 
			{
				int _keyword = 0;
			
				if( Z_TYPE_P( keyword ) == IS_LONG )
					_keyword = Z_LVAL_P( keyword );

				else if( is_numeric_string(Z_STRVAL_P(keyword),Z_STRLEN_P(keyword),NULL,NULL,0) )
					_keyword = zend_atoi( Z_STRVAL_P(keyword) , Z_STRLEN_P(keyword) );
			
				if( search_by <= 0 )
				{
					if( _keyword ==  all_process[i].process_id ||  _keyword == all_process[i].parent_process_id ){
						wing_query_process_item( return_value, all_process[i] );
					}

				}
				else if( search_by == WING_SEARCH_BY_PROCESS_ID ) 
				{

					if( _keyword ==  all_process[i].process_id ){
						wing_query_process_item( return_value, all_process[i] );
					}

				}
				else if( search_by == WING_SEARCH_BY_PARENT_PROCESS_ID ) 
				{
				
					if( _keyword ==  all_process[i].parent_process_id ){
						wing_query_process_item( return_value, all_process[i] );
					}
				
				}
			}


			//������ַ���
			if( Z_TYPE_P(keyword) == IS_STRING && Z_STRLEN_P(keyword) > 0  ) 
			{
				//��ָ����ѯ�ֶ�
				if( search_by <= 0 ){

					if( ( all_process[i].process_name != NULL && strlen(all_process[i].process_name) > 0 && strstr( all_process[i].process_name , (const char*)Z_STRVAL_P(keyword) ) != NULL ) ||
						( all_process[i].command_line != NULL && strlen(all_process[i].command_line) > 0 && strstr( all_process[i].command_line , (const char*)Z_STRVAL_P(keyword) ) != NULL ) ||
						( all_process[i].file_path    != NULL && strlen(all_process[i].file_path)    > 0 && strstr( all_process[i].file_path ,    (const char*)Z_STRVAL_P(keyword) ) != NULL )
					)
					{
						wing_query_process_item( return_value, all_process[i] );
					}

				}
				else if( search_by == WING_SEARCH_BY_PROCESS_NAME )
				{
			
					if( all_process[i].process_name != NULL && strlen(all_process[i].process_name) > 0 && strstr( all_process[i].process_name , (const char*)Z_STRVAL_P(keyword) ) != NULL )
					{
						wing_query_process_item( return_value, all_process[i] );
					}
			
				}
				else if( search_by == WING_SEARCH_BY_COMMAND_LINE ) 
				{
			
					if( all_process[i].command_line != NULL && strlen(all_process[i].command_line) > 0 && strstr( all_process[i].command_line , (const char*)Z_STRVAL_P(keyword) ) != NULL )
					{
						wing_query_process_item( return_value, all_process[i] );
					}
			
				}
				else if( search_by == WING_SEARCH_BY_PROCESS_FILE_PATH ) 
				{
				
					if( all_process[i].file_path != NULL && strlen(all_process[i].file_path)    > 0 && strstr( all_process[i].file_path , (const char*)Z_STRVAL_P(keyword) )    != NULL )
					{
						wing_query_process_item( return_value, all_process[i] );
					}

				}
			}
		
		}
	}
	delete[] all_process;

}

/**
 * @��ȡʹ�õ��ڴ���Ϣ 
 * @����ʵ��ռ�õ��ڴ��С
 */
ZEND_FUNCTION( wing_get_memory_used ) {

	int process_id = 0;
	HANDLE handle  = INVALID_HANDLE_VALUE;
	
	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|l",&process_id) != SUCCESS) {
		RETURN_LONG( 0 );
		return;
	}

	if( process_id <= 0)
		handle = GetCurrentProcess();

	else
		handle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, process_id );

	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo( handle, &pmc, sizeof(pmc) );
	CloseHandle( handle );
	RETURN_LONG( pmc.WorkingSetSize );
	return;
}



