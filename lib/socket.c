/*



//������ɶ˿�
	HANDLE m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0 ); 
	if(m_hIOCompletionPort==NULL){
		RETURN_BOOL(0);
		return;
	}
	//��ȡcpu���� ���ں��洴��worker����
	SYSTEM_INFO si; 
	GetSystemInfo(&si); 
	int m_nProcessors = si.dwNumberOfProcessors; 


	// ����CPU����������*2���߳�
	int m_nThreads = 2 * m_nProcessors; 
	HANDLE* m_phWorkerThreads = new HANDLE[m_nThreads]; 
	for (int i = 0; i < m_nThreads; i++) 
	{ 
		 m_phWorkerThreads[i] = ::CreateThread(0, 0, _WorkerThread, ��); 
	} 


	// ��ʼ��Socket��
	WSADATA wsaData; 
	WSAStartup(MAKEWORD(2,2), &wsaData); 
	//��ʼ��Socket
	struct sockaddr_in ServerAddress; 
	// ������Ҫ�ر�ע�⣬���Ҫʹ���ص�I/O�Ļ����������Ҫʹ��WSASocket����ʼ��Socket
	// ע�������и�WSA_FLAG_OVERLAPPED����
	SOCKET m_sockListen = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); 
	// ����ַ�ṹ��Ϣ
	ZeroMemory((char *)&ServerAddress, sizeof(ServerAddress)); 
	ServerAddress.sin_family = AF_INET; 
	// �������ѡ����κ�һ�����õĵ�ַ���������Լ�ָ����һ��IP��ַ
	//ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);                     
	ServerAddress.sin_addr.s_addr = inet_addr("0.0.0.0");          
	ServerAddress.sin_port = htons(11111);                           
	// �󶨶˿�
	if (SOCKET_ERROR == bind(m_sockListen, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress)))  
	// ��ʼ����
	listen(m_sockListen,SOMAXCONN);


*/