#ifndef PHP_IOCP_UTF8_H
#define PHP_IOCP_UTF8_H
#include "Windows.h"
/**
 *@��gbk�ַ���ת��Ϊutf8�ַ���
 *@out_str�����ʼ����api�ڲ�������Ҫ�Զ���ʼ��
 *@ע��ʹ����֮�� ��ס��Ҫ delete[] out_str;�ͷ��ڴ�
 */
void iocp_gbk_to_utf8( _In_ char *in_str, _Out_ char *&out_str );//_Inout_
char* WcharToUtf8( const wchar_t *pwStr );
#endif
