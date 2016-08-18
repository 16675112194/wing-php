#ifndef __WING_STRING_H__
#define __WING_STRING_H__

#include "Windows.h"

class WingString{

private:
	char *str;
	unsigned int len;

public:

	//�����������캯��
	WingString( const char *_str, int size );
	WingString( char *_str , int size , int dup );
	WingString();
	WingString( const wchar_t *_str );

	~WingString();

	unsigned length();
	char* c_str();
	void append( const char *_str, int size );
	void append( WingString *_str );
	void append( const wchar_t *_str );
	BOOL toUTF8( );
	void print();
	void trim();
};


#endif