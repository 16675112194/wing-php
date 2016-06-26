
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "php.h"
#include "php_ini.h"
#include "zend_constants.h"
#include "ext/standard/info.h"
#include "php_wing.h"
#include "lib/wing_lib.h"

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