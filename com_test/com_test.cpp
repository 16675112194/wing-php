// com_test.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"


#define _WIN32_DCOM  
#include <iostream>  
using namespace std;  
#include <comdef.h>  
#include <Wbemidl.h>  
  
# pragma comment(lib, "wbemuuid.lib")  
  
int main(int argc, char **argv)  
{  
	/**
	BSTR strClassProp = SysAllocString(L"__CLASS");
HRESULT hr;
hr = pObj->Get(strClassProp, 0, &v, 0, 0);
SysFreeString(strClassProp);
	*/
    HRESULT hres;  
  
    // Step 1: --------------------------------------------------  
    // Initialize COM. ------------------------------------------  
  
    hres =  CoInitializeEx(0, COINIT_MULTITHREADED);   
    if (FAILED(hres))  
    {  
        cout << "Failed to initialize COM library. Error code = 0x"   
            << hex << hres << endl;  
        return 1;                  // Program has failed.  
    }  
  
    // Step 2: --------------------------------------------------  
    // Set general COM security levels --------------------------  
    // Note: If you are using Windows 2000, you need to specify -  
    // the default authentication credentials for a user by using  
    // a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----  
    // parameter of CoInitializeSecurity ------------------------  
  
    hres =  CoInitializeSecurity(  
        NULL,   
        -1,                          // COM authentication  
        NULL,                        // Authentication services  
        NULL,                        // Reserved  
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication   
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation    
        NULL,                        // Authentication info  
        EOAC_NONE,                   // Additional capabilities   
        NULL                         // Reserved  
        );  
  
                        
    if (FAILED(hres))  
    {  
        cout << "Failed to initialize security. Error code = 0x"   
            << hex << hres << endl;  
        CoUninitialize();  
        return 1;                    // Program has failed.  
    }  
      
    // Step 3: ---------------------------------------------------  
    // Obtain the initial locator to WMI -------------------------  
  
    IWbemLocator *pLoc = NULL;  
  
    hres = CoCreateInstance(  
        CLSID_WbemLocator,               
        0,   
        CLSCTX_INPROC_SERVER,   
        IID_IWbemLocator, (LPVOID *) &pLoc);  
   
    if (FAILED(hres))  
    {  
        cout << "Failed to create IWbemLocator object."  
            << " Err code = 0x"  
            << hex << hres << endl;  
        CoUninitialize();  
        return 1;                 // Program has failed.  
    }  
  
    // Step 4: -----------------------------------------------------  
    // Connect to WMI through the IWbemLocator::ConnectServer method  
  
    IWbemServices *pSvc = NULL;  
   
    // Connect to the root\cimv2 namespace with  
    // the current user and obtain pointer pSvc  
    // to make IWbemServices calls.  
    hres = pLoc->ConnectServer(  
         _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace  
         NULL,                    // User name. NULL = current user  
         NULL,                    // User password. NULL = current  
         0,                       // Locale. NULL indicates current  
         NULL,                    // Security flags.  
         0,                       // Authority (e.g. Kerberos)  
         0,                       // Context object   
         &pSvc                    // pointer to IWbemServices proxy  
         );  
      
    if (FAILED(hres))  
    {  
        cout << "Could not connect. Error code = 0x"   
             << hex << hres << endl;  
        pLoc->Release();       
        CoUninitialize();  
        return 1;                // Program has failed.  
    }  
  
    cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;  
  
  
    // Step 5: --------------------------------------------------  
    // Set security levels on the proxy -------------------------  
  
    hres = CoSetProxyBlanket(  
       pSvc,                        // Indicates the proxy to set  
       RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx  
       RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx  
       NULL,                        // Server principal name   
       RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx   
       RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx  
       NULL,                        // client identity  
       EOAC_NONE                    // proxy capabilities   
    );  
  
    if (FAILED(hres))  
    {  
        cout << "Could not set proxy blanket. Error code = 0x"   
            << hex << hres << endl;  
        pSvc->Release();  
        pLoc->Release();       
        CoUninitialize();  
        return 1;               // Program has failed.  
    }  
  
    // Step 6: --------------------------------------------------  
    // Use the IWbemServices pointer to make requests of WMI ----  
  
    // For example, get the name of the operating system  
    IEnumWbemClassObject* pEnumerator = NULL;  
    hres = pSvc->ExecQuery(  
        bstr_t("WQL"),   
        bstr_t("Select * from Win32_Process "),  
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY ,   
        NULL,  
        &pEnumerator);  
      
    if (FAILED(hres))  
    {  
        cout << "Query for operating system name failed."  
            << " Error code = 0x"   
            << hex << hres << endl;  
        pSvc->Release();  
        pLoc->Release();  
        CoUninitialize();  
        return 1;               // Program has failed.  
    }  
  
    // Step 7: -------------------------------------------------  
    // Get the data from the query in step 6 -------------------  
   
    IWbemClassObject *pclsObj;  
    ULONG uReturn = 0;  
     
    while (pEnumerator)  
    {  
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,   
            &pclsObj, &uReturn);  
  
        if(0 == uReturn)  
        {  
            break;  
        }  
  
        VARIANT vtProp;  
  
		
        // Get the value of the Name property  
        hr = pclsObj->Get(L"CommandLine", 0, &vtProp, 0, 0);  

		if (SUCCEEDED(hr) && (V_VT(&vtProp) == VT_BSTR)){
			 wcout << " CommandLine : " << vtProp.bstrVal << "\r\n\r\n\r\n";  
		}
		else
           {
			   wcout <<"get commandline faile\r\n\r\n\r\n"; 

		}
        VariantClear(&vtProp);  



		VARIANT vtProp2;  

		 hr = pclsObj->Get(L"ExecutablePath", 0, &vtProp2, 0, 0);  

		if (SUCCEEDED(hr) && (V_VT(&vtProp2) == VT_BSTR)){
			 wcout << " ExecutablePath : " << vtProp2.bstrVal << "\r\n\r\n\r\n";  
		}
		else
           {
			   wcout <<"get ExecutablePath faile\r\n\r\n\r\n"; 

		}
        VariantClear(&vtProp2); 



  
        pclsObj->Release();  
    }  
  








    // Cleanup  
    // ========  
      
    pSvc->Release();  
    pLoc->Release();  
    pEnumerator->Release();  
   // pclsObj->Release();  
    CoUninitialize();  
  
    return 0;   // Program successfully completed.  
   
}  

