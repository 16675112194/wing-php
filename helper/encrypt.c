#define _CRT_GETPUTWCHAR_NOINLINE
#include "encrypt.h"
#include <stdio.h>  
#include <string.h>  
#include <conio.h>  
#include <wincrypt.h> 
#include <io.h>
#include "wing_malloc.h"

#define MY_ENCODING_TYPE  (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)  
#define KEYLENGTH  0x00800000  
#define ENCRYPT_ALGORITHM CALG_RC4   
#define ENCRYPT_BLOCK_SIZE 8   
 

HCRYPTKEY GenKeyByPassword(HCRYPTPROV hCryptProv,PCHAR szPassword);  
HCRYPTPROV GetCryptProv();    
/**
 *@�����ļ� szDestination����Ϊ�õ����ļ����ݣ���������ڴ棬���Զ�������� ʹ������Ҫdelete[] szDestination
 */
BOOL WingDecryptFile( PCHAR szSource, PCHAR &szDestination, PCHAR szPassword)   
{   
    FILE *hSource;     
    HCRYPTPROV hCryptProv;   
    HCRYPTKEY hKey;   
    PBYTE pbBuffer;   
    size_t dwBlockLen;   
    DWORD dwBufferLen;   
    DWORD dwCount;   
    int size = 0;
   
    fopen_s( &hSource,szSource, "rb" );   
  
	size = _filelength(_fileno(hSource));
	

	szDestination = (PCHAR)wing_malloc( size+1 );
	if( !szDestination )
	{	
		szDestination = NULL;
		return 0;
	}

	memset( szDestination,0,size+1);

    hCryptProv = GetCryptProv(); 
	if( NULL == hCryptProv)
		return  0;


	

    hKey       = GenKeyByPassword( hCryptProv, szPassword);  

	if( NULL == hKey )
		return 0;
      
    dwBlockLen  = 1000 - 1000 % ENCRYPT_BLOCK_SIZE;   
    dwBufferLen = dwBlockLen;   
 
  
    if(!(pbBuffer = (BYTE *)malloc(dwBufferLen)))  
    {  
		return 0;  
    }  

	char *start = szDestination;
	
	size_t esize = 1;
	size_t read_size = 0;
    do {   
        dwCount = fread( pbBuffer, esize , dwBlockLen, hSource );   
        if(ferror(hSource))  
        {  
            return 0;
        }  

		

        // ���� ����  
        if( !CryptDecrypt( hKey, 0, feof(hSource), 0, pbBuffer, &dwCount ))  
        {  
			return 0; 
        }  

		memcpy(start,pbBuffer,dwCount);
		start += dwCount;
		
		read_size+=dwCount;
		if( read_size >= (size_t)size ) 
			break;
		

    } while( !feof( hSource ) );   
  
    
    if( hSource )  
    {  
        fclose(hSource);   
    }   

    if( pbBuffer )   
    {    
		free(pbBuffer);
	}
   
    if( hKey )  
    {  
        CryptDestroyKey(hKey); 
    }   
  
    if( hCryptProv )  
    {  
        CryptReleaseContext(hCryptProv, 0); 
    }   
  
    return 1;  
} 
  
   
/**
 * @����ԭ��szSource�ļ������ܺ�����ݴ洢��szDestination�ļ���   
 * @szSource��ԭ���ļ���  
 * @szDestination�����ܺ����ݴ洢�ļ�  
 * @szPassword���û���������� 
 */
BOOL WingEncryptFile( PCHAR szSource, PCHAR szDestination,  PCHAR szPassword)  
{  

    FILE *hSource;   
    FILE *hDestination;   
  
    HCRYPTPROV hCryptProv;   
    HCRYPTKEY hKey;   
  
  
    PBYTE pbBuffer;   
    size_t dwBlockLen;   
    DWORD dwBufferLen;   
    DWORD dwCount;   
   
	fopen_s( &hSource,szSource,"rb" );
	fopen_s( &hDestination,szDestination,"wb" );

	size_t size = _filelength(_fileno(hSource));
  
    //��ȡ���ܷ����߾��  
    hCryptProv = GetCryptProv();  
	if( NULL == hCryptProv )
		return 0;
  
    hKey = GenKeyByPassword( hCryptProv, szPassword);  
	if( NULL == hKey )
		return  0;
         
    // ��Ϊ�����㷨��ENCRYPT_BLOCK_SIZE ��С����ܣ����Ա����ܵ�  
    // ���ݳ��ȱ�����ENCRYPT_BLOCK_SIZE �����������������һ�μ��ܵ�  
    // ���ݳ��ȡ�  
  
    dwBlockLen = 1000 - 1000 % ENCRYPT_BLOCK_SIZE;   

    // ȷ�����ܺ��������ݿ��С. ���Ƿ�������ģʽ������������ɶ����Ŀռ�    

    if(ENCRYPT_BLOCK_SIZE > 1)   
        dwBufferLen = dwBlockLen + ENCRYPT_BLOCK_SIZE;   
    else   
        dwBufferLen = dwBlockLen;   
      
    // �����ڴ�ռ�.   
    if( !(pbBuffer = (BYTE *)malloc(dwBufferLen)) )  
    {  
        return 0;    
	}  
  
	size_t read_size = 0;

    do   
    {      
        dwCount = fread(pbBuffer, 1, dwBlockLen, hSource);   
        if( ferror(hSource) )  
        {   
            return 0;
        }  

        if(!CryptEncrypt(  
            hKey,           //��Կ  
            0,              //�������ͬʱ����ɢ�кͼ��ܣ����ﴫ��һ��ɢ�ж���  
            feof(hSource),  //��������һ�������ܵĿ飬����TRUE.�����������FALSE.  
                            //����ͨ���ж��Ƿ��ļ�β�������Ƿ�Ϊ���һ�顣  
            0,              //����  
            pbBuffer,       //���뱻�������ݣ�������ܺ������  
            &dwCount,       //���뱻��������ʵ�ʳ��ȣ�������ܺ����ݳ���  
            dwBufferLen))   //pbBuffer�Ĵ�С��  
        {   
           return 0;
        }   
  
        // �Ѽ��ܺ�����д�������ļ���   
  
        fwrite(pbBuffer, 1, dwCount, hDestination);   
        if(ferror(hDestination))  
        {   
            return 0;
        }  

		read_size += dwCount;
		if( read_size >= size ) 
			break;
  
    }   while(!feof(hSource));   
  

    if(hSource)  
    {  
        fclose(hSource);
    }  
    if( hDestination )  
    {  
        fclose(hDestination);  
    }  
  
    if( pbBuffer )   
    {    
		free(pbBuffer);  
	}
 
    if(hKey)  
    {  
        CryptDestroyKey(hKey);  
    }   
  
    if( hCryptProv )  
    {  
        CryptReleaseContext(hCryptProv, 0);  
    }  
    return 1;   
}
  
  
/**
 * @ ��ȡ�����ṩ�߾��
 */
HCRYPTPROV GetCryptProv()  
{  
    HCRYPTPROV hCryptProv;          

    if( !CryptAcquireContext( &hCryptProv, NULL,  MS_ENHANCED_PROV, PROV_RSA_FULL,  0) )
    {  
        if(!CryptAcquireContext(&hCryptProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET))  
        {  
           return NULL;  
        }  
    }  
    return hCryptProv;  
}  
 
  
HCRYPTKEY GenKeyByPassword(HCRYPTPROV hCryptProv,PCHAR szPassword)  
{  
    HCRYPTKEY hKey;   
    HCRYPTHASH hHash;  

    if( !CryptCreateHash( hCryptProv, CALG_MD5, 0, 0, &hHash ) )  
    {  
		return NULL;
    }    

    if( !CryptHashData( hHash, (BYTE *)szPassword, (DWORD)strlen(szPassword), 0))  
    {  
        return NULL;  
    }  

    if( !CryptDeriveKey( hCryptProv, ENCRYPT_ALGORITHM, hHash, KEYLENGTH, &hKey ) )  
    {  
       return NULL;  
    }  

    if( hHash )   
    {  
        CryptDestroyHash(hHash);   
        hHash = 0;  
    }  
 
    return hKey;  
}  