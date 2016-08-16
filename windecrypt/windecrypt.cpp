// �����ļ�����
//

#include "stdafx.h"

#include <stdio.h>  
#include <string.h>  
#include <conio.h>  
#include <windows.h>  
#include <wincrypt.h>  
#define MY_ENCODING_TYPE  (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)  
#define KEYLENGTH  0x00800000  
void HandleError(char *s);  
HCRYPTPROV GetCryptProv();  
  
#define ENCRYPT_ALGORITHM CALG_RC4   
#define ENCRYPT_BLOCK_SIZE 8   
   
BOOL DecryptFile(  
     PCHAR szSource,   
     PCHAR szDestination,   
     CHAR *szPassword);   
  
HCRYPTKEY GenKeyByPassword(HCRYPTPROV hCryptProv,PCHAR szPassword);  
HCRYPTKEY GenKeyFromFile(HCRYPTPROV hCryptProv,FILE* hSource);  
  
void main(void)   
{   
    //--------------------------------------------------------------------  
    // �����������ʼ��.  
  
    PCHAR szSource = "D:/2.txt";   
    PCHAR szDestination = "D:/3.txt";   
    PCHAR szPassword = "123456";   
    char  response;  
  
   
    //�����ļ�  
    if(!DecryptFile(szSource, szDestination, szPassword))  
    {  
        printf("\nError decrypting file. \n");   
    }  
    else  
    {   
        printf("\n���ļ� %s �Ľ��ܳɹ���. \n", szSource);  
        printf("�����ܵ��ļ��� %s .\n",szDestination);  
    }  
  
} // End main  
  
//-------------------------------------------------------------------  
// ���ܣ���������szSource�ļ������ܺ�����ݴ洢��szDestination�ļ���  
// ����:  
//  szSource�������ļ���  
//  szDestination�����ܺ����ݴ洢�ļ�  
//  szPassword���û����������  
  
static BOOL DecryptFile(  
     PCHAR szSource,   
     PCHAR szDestination,   
     PCHAR szPassword)   
{   
    //--------------------------------------------------------------------  
    // �ֲ������������ʼ��.  
  
    FILE *hSource;   
    FILE *hDestination;   
  
    HCRYPTPROV hCryptProv;   
    HCRYPTKEY hKey;   
  
    PBYTE pbBuffer;   
    DWORD dwBlockLen;   
    DWORD dwBufferLen;   
    DWORD dwCount;   
  
    BOOL status = FALSE;   
   
    //--------------------------------------------------------------------  
    // �������ļ�.   
    fopen_s(&hSource,szSource,"rb");   
   
    //--------------------------------------------------------------------  
    // ��Ŀ���ļ������ڴ洢���ܺ������.   
  
   fopen_s(&hDestination,szDestination,"wb");
    
    //��ȡ���ܷ����߾��  
    hCryptProv = GetCryptProv();  
  
    //��ȡ�򴴽��Ự��Կ  
    if(!szPassword|| strcmp(szPassword,"")==0 )   
    {   
        //--------------------------------------------------------------------  
        //�������ļ����뱣��ĻỰ��Կ   
  
        hKey = GenKeyFromFile( hCryptProv,hSource);  
          
    }   
    else   
    {   
        //--------------------------------------------------------------------  
        // ͨ�������������´����Ự��Կ.   
   
        hKey=GenKeyByPassword( hCryptProv, szPassword);  
    }   
   
  
    // ����һ�ν��ܵ����ݳ��ȣ�����ENCRYPT_BLOCK_SIZE ��������  
  
    dwBlockLen = 1000 - 1000 % ENCRYPT_BLOCK_SIZE;   
    dwBufferLen = dwBlockLen;   
  
    //--------------------------------------------------------------------  
    // �����ڴ�ռ�.   
  
    if(!(pbBuffer = (BYTE *)malloc(dwBufferLen)))  
    {  
       HandleError("�����ڴ治��!\n");   
    }  
    //--------------------------------------------------------------------  
    // ���������ļ������ܺ����ݱ�����Ŀ���ļ�   
  
    do {   
        //--------------------------------------------------------------------  
        // ÿ�δ������ļ��ж�ȡdwBlockLen�ֽ�����.   
  
        dwCount = fread(  
             pbBuffer,   
             1,   
             dwBlockLen,   
             hSource);   
        if(ferror(hSource))  
        {  
            HandleError("��ȡ�����ļ�����!");  
        }  
        //--------------------------------------------------------------------  
        // ���� ����  
        if(!CryptDecrypt(  
              hKey,   
              0,   
              feof(hSource),   
              0,   
              pbBuffer,   
              &dwCount))  
        {  
           HandleError("Error during CryptDecrypt!");   
        }  
        //--------------------------------------------------------------------  
        // �ѽ��ܺ������д��Ŀ���ļ���.   
  
        fwrite(  
            pbBuffer,   
            1,   
            dwCount,   
            hDestination);   
        if(ferror(hDestination))  
        {  
           HandleError("Error writing plaintext!");   
        }  
    }   while(!feof(hSource));   
  
    status = TRUE;   
  
    //--------------------------------------------------------------------  
    // �ر��ļ�  
    if(hSource)  
    {  
        if(fclose(hSource))  
            HandleError("�ر�ԭ�ļ�����");  
    }  
    if(hDestination)  
    {  
        if(fclose(hDestination))  
            HandleError("�ر�Ŀ���ļ�����");  
    }   
   
    //--------------------------------------------------------------------  
    // �ͷ��ڴ�ռ�   
  
    if(pbBuffer)   
         free(pbBuffer);   
   
    //--------------------------------------------------------------------  
    // ���ٻỰ��Կ  
  
    if(hKey)  
    {  
        if(!(CryptDestroyKey(hKey)))  
            HandleError("Error during CryptDestroyKey");  
    }   
  
    //--------------------------------------------------------------------  
    // �ͷ�CSP���  
    if(hCryptProv)  
    {  
        if(!(CryptReleaseContext(hCryptProv, 0)))  
            HandleError("Error during CryptReleaseContext");  
    }   
  
    return status;  
} // end Decryptfile  
  
  
//��ȡ�����ṩ�߾��  
HCRYPTPROV GetCryptProv()  
{  
    HCRYPTPROV hCryptProv;                      // ���ܷ����ṩ�߾��  
      
    //��ȡ�����ṩ�߾��  
    if(CryptAcquireContext(  
                &hCryptProv,         // ���ܷ����ṩ�߾��  
                NULL,                // ��Կ������,����ʹ�õ�½�û���  
                MS_ENHANCED_PROV,         // ���ܷ����ṩ��       
                PROV_RSA_FULL,       // ���ܷ����ṩ������,�����ṩ���ܺ�ǩ���ȹ���  
                0))                  // ��־  
    {  
        printf("���ܷ����ṩ�߾����ȡ�ɹ�!\n");  
    }  
    else  
    {  
          
        //���½���һ���µ���Կ��  
        if(!CryptAcquireContext(&hCryptProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET))  
        {  
           HandleError("���½���һ���µ���Կ������!");  
        }  
   
    }  
    return hCryptProv;  
}  
  
  
//  HandleError��������������ӡ������Ϣ�����˳�����  
void HandleError(char *s)  
{  
    printf("����ִ�з�������!\n");  
    printf("%s\n",s);  
    printf("�������Ϊ: %x.\n",GetLastError());  
    printf("������ִֹ��!\n");  
    exit(1);  
}  
  
// GenKeyFromFile���������ļ��е����Ự��Կ  
// ������hCryptProv CSP���  
//       hSource   ����Ự��Կ���ļ�  
HCRYPTKEY GenKeyFromFile(HCRYPTPROV hCryptProv,FILE* hSource)  
{  
    HCRYPTKEY hKey;   
  
    PBYTE pbKeyBlob;   
    DWORD dwKeyBlobLen;   
  
    //�������ļ��л�ȡ��Կ���ݿ鳤�ȣ��������ڴ�ռ�.   
    fread(&dwKeyBlobLen, sizeof(DWORD), 1, hSource);   
    if(ferror(hSource) || feof(hSource))  
    {  
        HandleError("��ȡ�����ļ�����Կ���ݿ鳤�ȳ���!");   
    }  
    if(!(pbKeyBlob = (BYTE *)malloc(dwKeyBlobLen)))  
    {  
        HandleError("�ڴ�������.");   
    }  
    //--------------------------------------------------------------------  
    // �������ļ��л�ȡ��Կ���ݿ�  
  
    fread(pbKeyBlob, 1, dwKeyBlobLen, hSource);   
    if(ferror(hSource) || feof(hSource))  
    {  
        HandleError("��ȡ�����ļ�����Կ���ݿ����!\n");   
    }  
    //--------------------------------------------------------------------  
    // ����Ự��Կ�� CSP.   
    if(!CryptImportKey(  
          hCryptProv,   
          pbKeyBlob,   
          dwKeyBlobLen,   
          0,   
          0,   
          &hKey))  
    {  
       HandleError("Error during CryptImportKey!");   
    }  
  
    if(pbKeyBlob)   
        free(pbKeyBlob);  
      
    //���ص����ĻỰ��Կ  
    return hKey;  
}  
  
// GenKeyByPassword��ͨ���������봴���Ự��Կ  
// ������hCryptProv CSP���  
//       szPassword ��������  
HCRYPTKEY GenKeyByPassword(HCRYPTPROV hCryptProv,PCHAR szPassword)  
{  
    HCRYPTKEY hKey;   
    HCRYPTHASH hHash;  
    //-------------------------------------------------------------------  
    // ������ϣ���.    
  
    if(CryptCreateHash(  
           hCryptProv,   
           CALG_MD5,   
           0,   
           0,   
           &hHash))  
        {  
            printf("һ����ϣ����Ѿ�������. \n");  
        }  
        else  
        {   
             HandleError("Error during CryptCreateHash!\n");  
        }    
    //-------------------------------------------------------------------  
    // ������������Ĺ�ϣֵ.   
  
    if(CryptHashData(  
           hHash,   
           (BYTE *)szPassword,   
           strlen(szPassword),   
           0))  
     {  
        printf("�������Ѿ�����ӵ��˹�ϣ����. \n");  
     }  
     else  
     {  
        HandleError("Error during CryptHashData. \n");   
     }  
    //-------------------------------------------------------------------  
    // ͨ����ϣֵ�����Ự��Կ.  
  
    if(CryptDeriveKey(  
           hCryptProv,   
           ENCRYPT_ALGORITHM,   
           hHash,   
           KEYLENGTH,   
           &hKey))  
     {  
       printf("���������Ĺ�ϣֵ�����һ��������Կ. \n");   
     }  
     else  
     {  
       HandleError("Error during CryptDeriveKey!\n");   
     }  
    //-------------------------------------------------------------------  
    // ���ٹ�ϣ���.   
  
    if(hHash)   
    {  
        if(!(CryptDestroyHash(hHash)))  
           HandleError("Error during CryptDestroyHash");   
        hHash = 0;  
    }  
          
    //���ش����ĻỰ��Կ  
    return hKey;  
}  