###����wing php
	wing php windowsƽ̨��ʵ��php����̡�Դ����ܡ�tcp��http��websocket��com...
	php����汾5.6.20 ���뻷��ΪVisual Studio 2012 X86 (VC11-x86)

###�����İ汾
	php_wing.dll ���뷢��dll�ļ�λ�ڰ汾��master��֧��Ŀ¼

###�ĵ�                               
	http://www.itdfy.com/git/

##QQȺ
    535218312
	
##�����ػ�����
    $process_id = wing_create_process_ex(__DIR__ . "/service.php start ", __DIR__ . "\\log\\output_base.log");
    ��һ������Ϊ��Ҫ���ػ�������ִ�еĳ���ָ��ڶ�������Ϊ��������ض���ָ���ļ���������˼�^��^	
	��wing_create_process��Ψһ�����ǣ�wing_create_process��Ҫָ����ִ���ļ���wing_create_process_ex����Ҫ
	�磺$process_id = wing_create_process_ex("C:/php/bin/php.exe ".__DIR__ . "/service.php start ", __DIR__ . "\\log\\output_base.log");
	
    $wing_version = wing_version()
	��ȡ�汾��api��echo wing_version();Ҳ����ʹ�ó���WING_VERSION
	
    $error_code = wing_get_last_error();//$error_code = wing_wsa_get_last_error();
	����api��Ϊ����������Ĵ������
	
   
   $error_msg = wing_get_error_msg($error_code)
   ��wing_get_last_error��wing_wsa_get_last_errorת��Ϊ�����ַ���
   
   $memory_usage = wing_get_memory_used();
   ��ȡ����ʵ��ռ�õ��ڴ��С����λΪ�ֽ�

##һ�����ӣ�
   $handle = wing_create_mutex("a test mutex"); //������ں˶��� $handle �ᱻ�ӽ��̼̳�

    wing_set_env("data","��仰���������ӽ���");
    $command = WING_PHP." ".__DIR__."/wing_create_process_runer.php";
    //$process_id = wing_create_process( $command, __DIR__."\\process_output.log");
    $process_id = wing_create_process_ex(__DIR__."/wing_create_process_runer.php",__DIR__."\\process_output.log");
    //wing_create_process_exר��php�ļ��Ĵ������̷�ʽ ����php�ļ���Ϊһ�������Ľ�����ִ��
    echo "����id:",$process_id,"\r\n";


    //wing_process_kill( $process_id ); //�����������apiɱ���������еĽ���



    /*if( com_find_process( $command ) ) { //Ϊ�˲�����δ��� ����ȥ�� wing_process_kill��ע��
    //����ʹ�����ַ�ʽ�жϽ����Ƿ��������� �ɿ�
    echo $process_id,"��������\r\n";
    }else{
    echo $process_id,"δ����\r\n";
    }*/

    //sleep(60);
    //�鿴���ü�����
    echo "���ü���",wing_query_object( $handle ),"\r\n";


    //���ַ���Ҳ�ܿɿ� ������Щ�������ڰ�ȫȨ�޵�ԭ�� �����޷�������ȡ ���ʧ�� ���ʱ��com��һ�������ѡ��
    $process_command = wing_query_process( "wing_create_process_runer.php" ) ;
    var_dump( $process_command );
    if( is_array( $process_command) && count( $process_command) > 0 ) {
    echo $process_id,"��������\r\n";
    }



    $wait_code = wing_process_wait( $process_id, WING_INFINITE ); //������ʱ�ȴ��ӽ����˳� �ڶ�������Ϊ��ѡ���� Ĭ��ΪWING_INFINITE
    switch( $wait_code ) {
    case WING_ERROR_FAILED :
            echo "�ȴ�ʧ��\r\n";
        break;

    case WING_WAIT_ABANDONED :
            echo 'û���ͷ�mutex����hHandleΪmutexʱ�����ӵ��mutex���߳��ڽ���ʱû���ͷź��Ķ���������˷���ֵ����\r\n';
        break;

    case WING_WAIT_TIMEOUT://����wing_process_wait�ڶ���������Ϊ WING_INFINITE��Ч
        echo "�ȴ���ʱ\r\n";
        break;
    
    default:
        echo "�����˳��룺",$wait_code,"\r\n"; //���ӽ��̵���exitʱ����Ĳ���
    }


    //�鿴���ü����� ���ӽ����˳�֮ǰС��1 Ҳ����ͨ�����ַ�ʽȥ�ж��ӽ����Ƿ�������~
    echo "���ü���",wing_query_object( $handle ),"\r\n";
    //wing_query_object �޷�ʶ��$handle�Ƿ���Ч ����wing_close_mutex����ǰʹ�ô˺���

    wing_close_mutex($handle);
    $handle = 0; //��ס close֮�� $handle=0 ���� ����Ҫ ��ֹ���� ������



    wing_process_kill( $process_id );
	ǿ����ֹһ�����̣��ܱ�����������ʵ��Ӧ����Ҫ����һЩ�����Ե����⣬��ֹ���ݶ�ʧ
	
    $current_process_id = wing_get_current_process_id();
	��ȡ��ǰ����id
	
    $quote_times = wing_query_object( $handle );
	��ȡ�����������ô���
	
    $handle = wing_create_mutex() ;
	����һ�������ں˶���

	wing_close_mutex($handle);
	�ر�һ�������ں˶���
	
    $process_info = wing_query_process( $key_word );
	��ѯϵͳ����

    $env = wing_get_env($key);
	��ȡ��������
	
    wing_set_env( $key, $value );
	���û�������
	
    $path = wing_get_command_path( $aommand );
	��ȡ�������ڵ�����·��
	
    $command_line = wing_get_command_line();
	��ȡ��������
	
    wing_override_function($func,$param,$code);
	��дϵͳ�������磺wing_override_function("header",'$header','global $http; $http->setHeaders($header);');
	��д��ϵͳ��header������header����дΪ����Ϊ$header�������ľ���ʵ��Ϊglobal $http; $http->setHeaders($header);��
	�� function header( $header ){global $http; $http->setHeaders($header);}
						
    wing_windows_send_msg($title,$msg);
	����̸������������Ϣ��ʵ��ԭ��Ϊwindows��copydata
	
    wing_windows_version();
	��ȡwindows��ϵͳ�汾
	$version = wing_windows_version();
	switch( $version ) {
		case WING_WINDOWS_ANCIENT:      echo "windows ancient\r\n";break;
		case WING_WINDOWS_XP:           echo "windows xp\r\n";break;
		case WING_WINDOWS_SERVER_2003:  echo "windows server 2003\r\n";break;
		case WING_WINDOWS_VISTA:        echo "windows vista\r\n";break;
		case WING_WINDOWS_7:            echo "windows 7\r\n";break;
		case WING_WINDOWS_8:            echo "windows 8\r\n";break;
		case WING_WINDOWS_8_1:          echo "windows 8.1\r\n";break;
		case WING_WINDOWS_10:           echo "windows 10\r\n";break;
		case WING_WINDOWS_NEW:          echo "windows new\r\n";break;
		case WING_ERROR_NT:             echo "some error happened\r\n"; break;
		default :                       echo "unknow version\r\n";
	}

