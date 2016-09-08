
#include "php_wing.h"
#include "helper/encrypt.h"
#include <ctype.h>  
#include "helper/hardware_info.h"
#include "helper/wing_malloc.h"


/**
 *@phpԴ�����
 */
ZEND_FUNCTION( wing_encrypt_file )
{
	
	char *input_file = NULL;
	int input_file_len = 0;

	char *output_file = NULL;
	int output_file_len = 0;

	char *encrypt_password = NULL;
	int password_len = 0;

	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"ss|s",&input_file,&input_file_len,&output_file,&output_file_len,&encrypt_password,&password_len) != SUCCESS) {
		RETURN_BOOL( 0 );
		return;
	}

	int needfree = 0;
	//�õ�Ӳ����������
	if( encrypt_password == NULL || strlen(encrypt_password) == 0 )
	{
		char *processor_id;
		char *serial_number;

		get_cpu_id( processor_id );
		get_serial_number( serial_number );

		if( processor_id != NULL && serial_number != NULL )
		{	
			spprintf( &encrypt_password, 0, "%s%s", processor_id, serial_number );
			needfree = 1;
			wing_free( processor_id );
			wing_free( serial_number );
		}else{
			RETURN_BOOL(0);
			return;
		}
	}

	//ִ���ļ�����
	int res = WingEncryptFile( input_file , output_file , encrypt_password );
	if( needfree ) 
		efree( encrypt_password );
	RETURN_BOOL( res );
	return;
}

/**
 *@ִ�м��ܹ���phpԴ���ļ�
 */
ZEND_FUNCTION( wing_run_file )
{

	char *input_file       = NULL;
	int input_file_len     = 0;
	char *encrypt_password = NULL;
	int password_len       = 0;

	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s|s",&input_file,&input_file_len,&encrypt_password,&password_len) != SUCCESS) {
		RETURN_LONG( 0 );
		return;
	}

	int needfree = 0;
	//��ȡӲ����������
	if( encrypt_password == NULL || strlen(encrypt_password) == 0 )
	{
		char *processor_id;
		char *serial_number;

		get_cpu_id( processor_id );
		get_serial_number( serial_number );

		if( processor_id != NULL && serial_number != NULL )
		{	
			spprintf( &encrypt_password, 0, "%s%s", processor_id, serial_number );
			needfree = 1;
			
			wing_free( processor_id );
			wing_free( serial_number );

			processor_id  = NULL;
			serial_number = NULL;

		}else
		{
			if( processor_id ) {
				wing_free( processor_id );
				processor_id = NULL;
			}
			if( serial_number ) {
				wing_free( serial_number );
				serial_number = NULL;
			}
			RETURN_BOOL(0);
			return;
		}
	}


	char *php_code = NULL;
	//���ܵõ�Դ��
	WingDecryptFile( input_file , php_code , encrypt_password );

	if( NULL == php_code )
	{	
		if( needfree ) 
			efree( encrypt_password );

		RETURN_BOOL(0);
		return;
	}

	int code_len   = strlen( php_code );
	char *run_code = php_code;

	//����<?phpͷ
	if( php_code[0] == '<')
		run_code += 5;

	//ִ��Դ��
	char *eval   = zend_make_compiled_string_description("wing run encrypt code" TSRMLS_CC);
    int retval   = zend_eval_string( run_code, NULL, eval TSRMLS_CC);

	if( eval ) 
	{
		efree( eval );
		eval = NULL;
	}
    wing_free( php_code );
	php_code = NULL;

	if( needfree ) 
		efree( encrypt_password );

	RETURN_BOOL( 1 );
	return;
}
