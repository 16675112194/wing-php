###about wing php
	wing php��һ��windowsƽ̨��ʵ��php����̡��ػ����̵ȹ��ܵĺ������ϡ�
	php version 5.6.20 win32 ����php�汾��win32 x86 5.6.20
	download(����url):http://windows.php.net/downloads/releases/php-5.6.20-Win32-VC11-x86.zip
	vc11(visual studio 2012) ���뻷��vc11(visual studio 2012)

###download release dll �����İ汾
	php_wing.dll in the root directory of the master branch
	php_wing.dll ���뷢���ļ�λ�ڰ汾���Ŀ¼

###wing_version                                     
	get wing extension version 
	��ȡ��չ�汾��

###wing_create_process                              
	create deamon process , if success then return process id,else return false 
	�����ػ����̣��ɹ����ؽ���id��ʧ�ܷ���false
	demo:
	$process_id = wing_create_process("C:/php/php.exe",__DIR__."/process_run.php");

###wing_process_kill                                
	kill process by processid 
	ͨ������idɱ������
	demo:
	$process_id = wing_create_process("C:/php/php.exe",__DIR__."/process_run.php");
	var_dump($process_id);
	sleep(1);
	wing_process_kill($process_id);

###wing_process_isalive                             
	check process is running,return true means process is still running 
	�жϽ����Ƿ��������У�����true ˵�����̻�������
	demo:
	var_dump(wing_process_isalive($process_id));

###wing_enum_processes                              
	enum all processes 
	ö�����еĽ���
	demo:
	var_dump(wing_enum_processes());
	return:
	  [39]=>
		  array(2) {
			["process_path"]=>
			string(60) "D:\php\php-sdk\phpdev\vc11\x86\php-5.6.20\Release_TS\php.exe"
			["process_id"]=>
			int(24684)
		  }
		  
	or use the code with com to get all processes:
	����ʹ��php com���ö�����еĽ���

	$obj                   = new COM ( 'winmgmts://localhost/root/CIMV2' );
	$processes             = $obj->ExecQuery("Select * from Win32_Process");

	foreach($processes as $process){
		echo "process id:".sprintf("%' 6d",$process->ProcessId) ."=>". $process->CommandLine,"\r\n";
	}
###wing_getpid
	get current process id ��ȡ��ǰ����id
###wing_process_exit
	exit a process �˳�����
	demo��
		$i=0;
		file_put_contents("D:/processid.pid",wing_getpid());
		while(1){
			$i++;
			file_put_contents("D:/1.html",$i,FILE_APPEND);
			sleep(1);
			if($i>10)break;
		}
		//exit process with code 400
		wing_process_exit(400);
	save as file  process_run.php ����Ϊ�ļ� process_run.php
	and create a process like ����һ���ػ�����
	$process_id = wing_create_process("C:/php/php.exe",__DIR__."/process_run.php");	
###wing_create_mutex��wing_close_mutex
	wing_create_mutex params not empty string, return >0 create success,===0 process is running, <0 create failue
	wing_create_mutex ����Ϊ�ǿ��ַ��� ����ֵ 0������������ -1 ��ȡ�������� -2 ��������Ϊ�� -3����������ʧ�� >0�����������ɹ�
	
	wing_close_mutex params is wing_create_mutex return value,return true close success
	wing_close_mutex�Ĳ����� wing_create_mutex�ķ���ֵ������true˵���رճɹ�
	
	run this demo twice at the same time
	ͬһʱ�����д�demo���� �鿴��������Ч��
	$handle = wing_create_mutex("a test mutex");
	// 0������������ -1 ��ȡ�������� -2 ��������Ϊ�� -3����������ʧ�� >0�����������ɹ�
	echo $handle,"\n";
	if($handle===0){
		echo "process is running\n";
		exit;
	}
	if($handle<0){
		echo "create mutex error\n";
		exit;
	}

	register_shutdown_function(function() use($handle){
		wing_close_mutex($handle);
	});
	$i=0;
	while(1){
		echo $i,"\n";
		$i++;
		if($i>10)break;
		sleep(1);
	}
###wing_get_command_path 
	get command path ��ȡ��������·��
	demo:
	wing_get_command_path("php");
	//C:\xampp\php\php.exe
###wing_get_env 
	get environment variable ��ȡ��������
	demo:
	wing_get_env("PATH");
###wing_set_env
	set environment variable ���û�������
	demo:
	var_dump(wing_set_env("yuyi_test","12"));
	echo wing_get_env("yuyi_test");
###wing_create_thread 
	����һ���첽�߳�ִ��php  create async thread 
	$thread_id = wing_create_thread(function(){
    $i=0;
    while(1){
        file_put_contents(__DIR__."/thread_test.log",$i++,FILE_APPEND);
        if($i>3)break;
        sleep(1);
    }

    exit(9);
	});
	if($thread_id<0){
		echo "create thread fail";
		exit;
	}
	if($thread_id == 0) {
		echo "child thread ";
		echo $thread_id;
		//child thread just exit
		exit;
	}

	//$thread_id parent continue running

	//wait for the thread exit
	$exit_code =  wing_thread_wait($thread_id,-1);

	echo $thread_id," is exit with ",$exit_code,"\n";
###about me
	QQ&Email 297341015@qq.com
	QQȺ:535218312