<?php
/**
 * @author yuyi
 * @created 2016/6/2 23:38
 * @email 297341015@qq.com
 */
for($i=0;$i<1;$i++){
    wing_create_process_ex(__DIR__."/wing_service_client.php");
    wing_create_process_ex(__DIR__."/wing_test.php");
    echo $i+1,"\r\n";
}