#include "wing_utf8.h"

int GBKToUTF8(unsigned char * lpGBKStr,unsigned char * lpUTF8Str,int nUTF8StrLen)
{
	wchar_t * lpUnicodeStr = NULL;
	int nRetLen = 0;

	if(!lpGBKStr)  //���GBK�ַ���ΪNULL������˳�
	return 0;

	nRetLen = ::MultiByteToWideChar(CP_ACP,0,(char *)lpGBKStr,-1,NULL,NULL);  //��ȡת����Unicode���������Ҫ���ַ��ռ䳤��
	lpUnicodeStr = new WCHAR[nRetLen + 1];  //ΪUnicode�ַ����ռ�
	nRetLen = ::MultiByteToWideChar(CP_ACP,0,(char *)lpGBKStr,-1,lpUnicodeStr,nRetLen);  //ת����Unicode����
	if(!nRetLen)  //ת��ʧ��������˳�
	return 0;

	nRetLen = ::WideCharToMultiByte(CP_UTF8,0,lpUnicodeStr,-1,NULL,0,NULL,NULL);  //��ȡת����UTF8���������Ҫ���ַ��ռ䳤��

	if(!lpUTF8Str)  //���������Ϊ���򷵻�ת������Ҫ�Ŀռ��С
	{
	if(lpUnicodeStr)
	delete []lpUnicodeStr;
	return nRetLen;
	}

	if(nUTF8StrLen < nRetLen)  //���������������Ȳ������˳�
	{
	if(lpUnicodeStr)
	delete []lpUnicodeStr;
	return 0;
	}

	nRetLen = ::WideCharToMultiByte(CP_UTF8,0,lpUnicodeStr,-1,(char *)lpUTF8Str,nUTF8StrLen,NULL,NULL);  //ת����UTF8����

	if(lpUnicodeStr)
	delete []lpUnicodeStr;

	return nRetLen;
	}


void iocp_gbk_to_utf8( char *in_str,char *&out_str){
	int len = GBKToUTF8((unsigned char *)in_str,NULL,NULL);

	out_str = new char[len + 1];
	memset(out_str,0,len + 1);
	len = GBKToUTF8((unsigned char *)in_str,(unsigned char *)out_str,len);
	if(!len)
	{
		delete[] out_str;
		out_str = NULL;
	}

}
