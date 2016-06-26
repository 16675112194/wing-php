#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "php.h"
#include "php_ini.h"
#include "zend_constants.h"
#include "ext/standard/info.h"
#include "php_wing.h"
#include "lib/wing_lib.h"

/***************************************************************
 * @ ��ȡ wing php�İ汾�� ����ʹ�ó��� WING_VERSION                
 **************************************************************/
PHP_FUNCTION(wing_version){
	char *string = NULL;
    int len = spprintf(&string, 0, "%s", PHP_WING_VERSION);
    RETURN_STRING(string,0);
}

/****************************************************************
 *@��ȡ������Ĵ���
 ****************************************************************/
ZEND_FUNCTION(wing_get_last_error){
	RETURN_LONG(GetLastError());
}

ZEND_FUNCTION(wing_wsa_get_last_error){
	RETURN_LONG(WSAGetLastError());
}

