// WingString.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "Windows.h"

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

	void* data();
	int type();
	char* c_str();
	wchar_t* w_str();

	void append( const char *_str, int size = 0 );
	void append( WingString *_str );
	void append( const wchar_t *_str,int size = 0 );

	BOOL toUTF8( );
	void print();
	void trim();
};

WingString::WingString( char *_str, int _size ){
	if( _size <= 0 ) 
		_size = WING_CHAR_SIZE( _str );
	this->str      = malloc( _size );
	this->str_size = _size;
	this->str_type = WING_STR_IS_CHAR;

	memset( this->str, 0, _size );
	memcpy( this->str, _str, _size ); 
}

WingString::WingString( wchar_t *_str, int _size ){
	if( _size <= 0 ) 
		_size = WING_WCHAR_SIZE( _str );
	this->str      = malloc( _size );
	this->str_size = _size;
	this->str_type = WING_STR_IS_WCHAR;

	memset( this->str, 0x0, _size );
	memcpy( this->str, _str, _size ); 
}

WingString::WingString(){
	this->str      = NULL;
	this->str_size = 0;
	this->str_type = WING_STR_IS_UNKNOW;
}

WingString::~WingString(){
	if( this->str != NULL ) {
		free( this->str );
		this->str = NULL;
	}
	this->str_size = 0;
	this->str_type = WING_STR_IS_UNKNOW;
}

unsigned int WingString::size(){
	return this->str_size;
}
unsigned int WingString::length(){
	
	switch( this->str_type )
	{
		case WING_STR_IS_CHAR:
			return (unsigned int)(  this->str_size/sizeof(char)  -1 );
			break;
		case WING_STR_IS_WCHAR:
			return (unsigned int)(  this->str_size/sizeof(wchar_t) -1 );
			break;
		default:
			return 0;
	}
	return 0;
}




/**
 *@׷���ַ���
 */
void WingString::append( const wchar_t *_str, int size ){
		
	if( _str == NULL )
    {
		return;
    }

	if( size <=0 ) 
		size = WING_WCHAR_SIZE( _str );

	if( this->str_type == WING_STR_IS_UNKNOW )
	{
		this->str_type = WING_STR_IS_WCHAR;
		this->str      = malloc( size );
		this->str_size = size;

		memcpy( this->str, _str, size );
		return;
	}

	if( this->str_type == WING_STR_IS_CHAR ){
		
		char *res = wing_str_wchar_to_char( (const wchar_t*)_str );

		int len       = WING_CHAR_SIZE( res );
		int new_len   = this->str_size + len - 1 ;

		char *new_str = (char*)malloc(new_len);

		memset( new_str , 0 , new_len );

		char *str_begin = new_str;
		memcpy( str_begin , this->str , this->str_size - 1 );

		str_begin += (this->str_size - 1);
		memcpy( str_begin , res , len );

		free( this->str );
		free( res );

		this->str      = new_str;
		this->str_size = new_len;
		return;
	}

	if( this->str_type == WING_STR_IS_WCHAR ) {
	    
		int wl       = sizeof(wchar_t);
		int new_size = this->str_size + size - wl;
		
		wchar_t* res = (wchar_t*)malloc( new_size );

		memset( res, 0x0, new_size );

		wsprintf( res, L"%s%s", this->str, _str );
	
		free( this->str );

		this->str      = res;
		this->str_size = new_size;
	}

}

/**
 *@׷���ַ���
 */
void WingString::append( const char *_str, int size ){
		
	if( _str == NULL )
    {
		return;
    }

	if( size <=0 ) 
		size = WING_CHAR_SIZE( _str );

	//��������ʱ�� û�г�ʼ��
	if( this->str_type == WING_STR_IS_UNKNOW )
	{
		this->str_type = WING_STR_IS_CHAR;
		this->str      = malloc( size );
		this->str_size = size;

		memcpy( this->str, _str, size );
		return;
	}

	if( this->str_type == WING_STR_IS_CHAR ){
		
		int new_size = this->str_size - 1 + size;
		char *res = (char*)malloc( new_size );
		memset( res, 0, new_size );

		char *str_start = res;
		memcpy( str_start, this->str, this->str_size - 1 );
		str_start +=  this->str_size - 1;

		memcpy( str_start, _str, size );

		free( this->str );

		this->str      = res;
		this->str_size = new_size;

		return;
	}

	if( this->str_type == WING_STR_IS_WCHAR ) {

		wchar_t* buf = wing_str_char_to_wchar( (const char *)_str );
		int new_size = WING_WCHAR_SIZE( buf ) - sizeof(wchar_t) + this->str_size;

		wchar_t* buffer = (wchar_t*)malloc(new_size);
		memset( buffer, 0x0, new_size );

		wsprintf( buffer, L"%s%s", this->str, buf );
		free( this->str );
		free( buf );

		this->str      = buffer;
		this->str_size = new_size;
	}

}


/**
 *@׷���ַ���
 */
void WingString::append( WingString *_str ){
		
	if( this->str_type == WING_STR_IS_UNKNOW ) {

		int size       = _str->size();
		this->str      = malloc( size );
		this->str_size = size;
		this->str_type = _str->type();

		memset( this->str, 0 , size );
		memcpy( this->str, _str->data(), size );

	}

}
void * WingString::data(){
	return this->str;
}
int WingString::type(){
	return this->str_type;
}
char* WingString::c_str(){
	return (char*)this->str;
}
wchar_t* WingString::w_str(){
	return (wchar_t*)this->str;
}
/**
 *@��ӡ�ַ���
 */
void WingString::print(){

	if( this->str_type == WING_STR_IS_CHAR )
		printf("---char:size=%ld,len=%ld,%s---\r\n",this->size(),this->length(),this->str);
	else if( this->str_type == WING_STR_IS_WCHAR )
		wprintf_s(L"---wchar_t:size=%ld,len=%ld,%s---\r\n",this->size(),this->length(),this->str);
}

/**
 * @�ַ���ת��Ϊutf8���룬��ı�����
 */
BOOL WingString::toUTF8()
{
	char *utf8_str = wing_str_char_to_utf8( ( const char* )str );
	if( utf8_str != NULL ){
		free( str );
		this->str = utf8_str;
		return 1;
	}
	return 0;
}

/**
 *@ȥ�����˿ո� ����ı�����
 */
void WingString::trim(){
	
	if( this->str == NULL || this->str_size <= 0 ) 
		return;
	if( this->str_type == WING_STR_IS_CHAR )
	{
		wing_str_trim( (char*)this->str );
		this->str_size = strlen((char*)this->str )+1;
	}
}
//----WingString end------------------------


char* wing_str_wchar_to_char( _In_ const wchar_t* _str ){
	//����õ�����Ŀ���ַ����ĳ��ȿռ� ���� \0������
	int len = WideCharToMultiByte( CP_UTF8, 0, _str, -1, NULL, 0, NULL, NULL );
	if (len <= 0)
	{
		return NULL;
	}

	char *res = (char*)malloc( len );
	memset( res, 0, len );

	WideCharToMultiByte( CP_UTF8, 0, _str, -1, res, len, NULL, NULL );

	return res;	
}
wchar_t* wing_str_char_to_wchar( _In_ const char* _str ){
	
	int size     = WING_CHAR_SIZE( _str );
	int len      = MultiByteToWideChar(CP_ACP,0,(const char *)_str,size-1,NULL,0);

	int buf_size = (len+1)*sizeof(wchar_t);
	wchar_t* buf = (wchar_t*)malloc( buf_size );
	memset( buf, 0x0, buf_size );
	MultiByteToWideChar( CP_ACP,0,(const char *)_str,size-1,buf,len);   

	return buf;
}
char* wing_str_char_to_utf8( _In_ const char* str ){
	
	if( str == NULL )
		return NULL;

	wchar_t* unicode_str = NULL;
	int utf8_str_size    = 0;

	utf8_str_size      = ::MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, NULL );                   //��ȡת����Unicode���������Ҫ���ַ��ռ䳤��
	unicode_str        = new wchar_t[utf8_str_size + 1];                                                   //ΪUnicode�ַ����ռ�
	utf8_str_size      = ::MultiByteToWideChar( CP_ACP, 0, str, -1, unicode_str, utf8_str_size );  //ת����Unicode����
	
	if( !utf8_str_size )                                                                                 //ת��ʧ��������˳�
	{
		if( unicode_str ) 
			delete[] unicode_str;
		return 0;
	}

	utf8_str_size  = WideCharToMultiByte(CP_UTF8,0,unicode_str,-1,NULL,0,NULL,NULL);                    //��ȡת����UTF8���������Ҫ���ַ��ռ䳤��
	char *utf8_str = new char[utf8_str_size+1];

	memset(utf8_str,0,utf8_str_size+1);

	utf8_str_size = WideCharToMultiByte( CP_UTF8, 0, unicode_str, -1, (char *)utf8_str, utf8_str_size+1, NULL, NULL );  
	                                                                                                    //ת����UTF8����
	if( unicode_str )
		delete []unicode_str;

	if( !utf8_str_size )
		return 0;

	return utf8_str;
}

/**
 *@ȥ���ַ������˿ո�
 */
void wing_str_trim( _Inout_ char* str ,int size ){
	if( str == NULL ) 
		return;
	if( size <= 0 )
		size = strlen( str );
	
	int len     = size;
	char *start = str;  
    char *end   = str + len - 1;  
  
	//�ҵ���һ����Ϊ�յ��ַ�
    while (1)   
    {     
        char c = *start;  
        if (!isspace(c))  
            break;  
  
        start++;  
        if (start > end)  
        {     
            str[0] = '\0';  
            return;  
        }    
    }     
  
	//�ҵ����һ����Ϊ�յ��ַ�
    while (1)   
    {     
        char c = *end;  
        if ( !isspace(c) && c != '\0' )  //���ݴ������size����strlen
            break;  
  
        end--;  
        if (start > end)  
        {     
            str[0] = '\0';  
            return;  
        }  
    }  
  
	//��������
    memmove(str, start, end - start + 1);  
	//���һ��ֵ����
    str[end - start + 1] = '\0';  
}




int _tmain(int argc, _TCHAR* argv[])
{
	WingString a("123");
	a.append(L"456");
	a.print();

	//WingString a2(L"123");
	//a2.append(L"456");
	//a2.print();

	WingString a3(L"123");
	a3.append("456");
	a3.print();


	//char s[] = " 999   ";
	//wing_str_trim( s, strlen(s)+1 );
	//printf("==>%s<==\r\n",s);

	//WingString a5("������");
	//a5.print();
	//a5.toUTF8();
	//a5.print();

	//WingString a6(" 999   ");
	//a6.print();
	//a6.trim();
	//a6.print();


	return 0;
}



