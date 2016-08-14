
#include "php_wing.h"
#include "wing_ntdll.h"

//windows------------------------------------------------------------------------------------------------------------------
/**
 *@ͨ��WM_COPYDATA���ͽ��̼���Ϣ ֻ�ܷ������ڳ���
 *@ע��ֻ�ܸ����ڳ�����Ϣ
 */
ZEND_FUNCTION( wing_windows_send_msg ){
	
	char *console_title   = NULL;
	int console_title_len = 0;
	int message_id        = 0;
	char *message         = NULL;
	int message_len       = 0;
	HWND  hwnd            = NULL;


	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sls",&console_title,&console_title_len,&message_id,&message,&message_len ) != SUCCESS) {

		RETURN_LONG(WING_ERROR_PARAMETER_ERROR);
		return;
	}

	hwnd = FindWindow( console_title, NULL );

	if( hwnd == NULL ) {
		RETURN_LONG(WING_ERROR_WINDOW_NOT_FOUND);
		return;
	}
	

	COPYDATASTRUCT CopyData; 

	CopyData.dwData	=  message_id ;  
	CopyData.cbData	= message_len;  
	CopyData.lpData =  message ;  //WM_COPYDATA
	
	SendMessageA( hwnd, WM_COPYDATA, NULL, (LPARAM)&CopyData );
	
	long status = GetLastError() == 0 ? WING_ERROR_SUCCESS:WING_ERROR_FAILED;

	RETURN_LONG(status);
	return;
}

/**
 *@��ȡwindows�汾
 */
ZEND_FUNCTION( wing_windows_version )
{
	RETURN_LONG(  WingWindowsVersion() );
    return;
}
