// linux_test.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
#ifdef WIN32
	printf("windows");
#else
	printf("other");
#endif
	return 0;
}

