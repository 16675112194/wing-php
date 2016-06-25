#ifndef PHP_WING_SOCKET
#define PHP_WING_SOCKET
#include <Winsock2.h>
#include "Windows.h"
#include "Winbase.h"

#include "tlhelp32.h"
#include "Psapi.h"
#include "Winternl.h"
#include "Processthreadsapi.h"
#include "Shlwapi.h"
#include "Strsafe.h"
#include "Mmsystem.h"
#include "mstcpip.h"
#include "process.h"
#include <mswsock.h>

#include <ws2tcpip.h>//ipv4 ipv6
//int getaddrinfo( const char *hostname, const char *service, const struct addrinfo *hints, struct addrinfo **result );
//https://msdn.microsoft.com/en-us/library/windows/desktop/ms742203(v=vs.85).aspx

#pragma comment(lib,"Kernel32.lib")
#pragma comment(lib,"Shlwapi.lib")
#pragma comment(lib,"Psapi.lib")
#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,"Ws2_32.lib")

#define DATA_BUFSIZE	1024
//#define SEND_DATA_BUFSIZE 1048576 //1024x1024 1M

//iocp�Ķ������� accept �µ����� recv �յ���Ϣ send�첽����
#define OPE_RECV		1
#define OPE_SEND		2
#define OPE_ACCEPT		3

//�Զ�����Ϣ
#define WM_ONCONNECT		WM_USER + 60
#define WM_ACCEPT_ERROR		WM_USER + 61
#define WM_ONERROR			WM_USER + 62
#define WM_ONCLOSE			WM_USER + 63
#define WM_ONRECV			WM_USER + 64
#define WM_ONQUIT           WM_USER + 65
#define WM_ONCLOSE_EX		WM_USER + 66
#define WM_ONSEND			WM_USER + 67
#define WM_THREAD_RUN		WM_USER + 71
#define WM_TIMEOUT          WM_USER + 72
#define WM_ADD_CLIENT       WM_USER + 73

//������Ϣ
#define WM_TEST				WM_USER+68
#define WM_TEST2			WM_USER+69
#define WM_TEST3			WM_USER+70

//������
#define WING_ERROR_CLOSE_SOCKET -4001
#define WING_ERROR_ACCEPT		-4002
#define WING_ERROR_MALLOC		-4003
#define WING_BAD_ERROR			-4
#define WING_ERROR_KEEP_ALIVE	-4004
#define WING_ERROR_POST_RECV    -4005
#define WING_BAD_ERROR          -4006


#define WING_SOCKET_IS_ALIVE 1 //���socket�Ƿ񼤻�״̬
#define WING_SOCKET_IS_SLEEP 0 //�Ǽ���״̬

//iocp��Ϣ�ṹ��
struct MYOVERLAPPED{
	OVERLAPPED	m_ol;                          //�첽����
	int			m_iOpType;                     //��������
	SOCKET		m_skServer;                    //�����socket
	SOCKET		m_skClient;                    //�ͻ���
	DWORD		m_recvBytes;                   //���յ���Ϣ����
	char		m_pBuf[DATA_BUFSIZE];          //������Ϣ�Ļ�����
	WSABUF		m_DataBuf;                     //��Ϣ����
	int			m_timeout;                     //���ó�ʱ
	SOCKADDR_IN m_addrClient;                  //�ͻ���ʵ�ʵĵ�ַ
	SOCKADDR_IN m_addrServer;                  //�����ʵ�ʵĵ�ַ
	int			m_isUsed;                      //��־�Ƿ��ѱ�����ʹ�� 1�ѱ����� 0������
	unsigned    m_active;                      //���Ļʱ��
	//LPVOID      m_client;                    //wing_client ����
};
typedef SOCKET wing_socket;
typedef MYOVERLAPPED wing_myoverlapped;
typedef LPOVERLAPPED wing_lpoverlapped;
//��Ϣ���д��ݵ���Ϣ
typedef struct{
	long len;
	char *msg;//[DATA_BUFSIZE+1]; 
} RECV_MSG;
typedef RECV_MSG wing_msg;

//iocp �̳߳� �����߳�
VOID CALLBACK wing_icop_thread(DWORD dwErrorCode,DWORD dwBytesTrans,LPOVERLAPPED lpOverlapped);

//socket MYOVERLAPPED hash map �����洢��Ӧ�Ĺ�ϵӳ��
//-------------socket MYOVERLAPPED map------------------------------------
//���ӳ���ϵ
 void				wing_add_to_sockets_map(unsigned long socket,unsigned long ovl);
//ͨ��socket��ȡovl
 unsigned long	wing_get_from_sockets_map(unsigned long socket);
//�Ƴ�ӳ���ϵ
 void				wing_remove_from_sockets_map(unsigned long socket);
//��ȡhash map���� ������������ һ������������� ĳЩsocket�ᱻ�Ƴ��� ��ɿ��õ�socketԽ��Խ��
//����������socket�쳣�رյ��������� Ҳ����˵socket�ص�socket�����쳣������֮�� ���Զ�����µĲ���
 unsigned int		wing_get_sockets_map_size();

//�쳣�˳�
void   wing_socket_throw_error( int error_code );
//�����socket��ʼ��
SOCKET wing_socket_init(const char *listen_ip,const int port,const int max_connect = 10000,const int timeout = 0);
//socket���������Դ����
void   wing_socket_clear();

unsigned int __stdcall wing_socket_check_active_timeout(PVOID params);
void wing_socket_add_client(wing_myoverlapped *&pMyOL);

//���߻ص�����
void   wing_socket_on_close(MYOVERLAPPED*  &pMyOL);
//post��Ϣ�� ��Ϣ����
void   wing_post_queue_msg(int message_id,unsigned long wparam=0,unsigned long lparam=0,unsigned long eparam=0);
//�Ͽ�����
BOOL WingDisconnectEx(SOCKET hSocket,LPOVERLAPPED lpOverlapped,DWORD dwFlags,DWORD reserved);
//�����µ�����
BOOL WingAcceptEx(SOCKET sListenSocket,SOCKET sAcceptSocket,PVOID lpOutputBuffer,DWORD dwReceiveDataLength,DWORD dwLocalAddressLength,DWORD dwRemoteAddressLength,LPDWORD lpdwBytesReceived,LPOVERLAPPED lpOverlapped);

#endif