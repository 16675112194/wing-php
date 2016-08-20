#ifndef __WING_STRING_H__
#define __WING_STRING_H__
#include "Windows.h"
#include <locale.h>
#define WING_STR_IS_CHAR   1
#define WING_STR_IS_WCHAR  2
#define WING_STR_IS_UNKNOW 3

#define WING_CHAR_SIZE(str)  (strlen((char*)str)+1)*sizeof(char)
#define WING_WCHAR_SIZE(str) (wcslen((wchar_t*)str)+1)*sizeof(wchar_t)

/**
 *@sizeΪ�ַ�������,����sizeof ������ ���Ѿ����ݲ��Ҷ����ư�ȫ
 */
void      wing_str_trim( _Inout_ char* str ,int size = 0 );
char*     wing_str_wchar_to_char( _In_ const wchar_t* str );
wchar_t*  wing_str_char_to_wchar( _In_ const char* str );
char*     wing_str_char_to_utf8( _In_ const char* str );
char*     wing_str_wchar_to_utf8( _In_ const wchar_t* str );

/**
 *@---- WingString ----
 *@�ַ��������װ 
 *@ע�� size ��Ϊռ���ڴ��ֽ� �����ַ������ȣ��� size=sizeof(*data) ���� size = (strlen(str)+1)*sizeof(char)
 */
class WingString{

private:
	void *str;
	unsigned int str_size;
	unsigned int str_type;

public:

	//���캯��
	WingString( char *_str, int _size = 0 );
	WingString( wchar_t *_str, int _size = 0 );
	WingString( );
	~WingString( );

	unsigned int size();
	unsigned int length();

	void* copy();
	void* data();
	int   type();
	char* c_str();
	wchar_t* w_str();

	void append( const char *_str, int size = 0 );
	void append( WingString *_str );
	void append( const wchar_t *_str,int size = 0 );

	BOOL toUTF8( );
	void print();
	void trim();
};



#endif