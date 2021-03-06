<?php
/**
 * @author yuyi
 * @created 2016/5/27 17:08
 * @email 297341015@qq.com
 * @windows crontab
 */
date_default_timezone_set("PRC");
$contab_file    = __DIR__."/wing_contab.conf";

/**
 * @crontab 参数标准化格式化
 */
function paramsFormat( $crontab_contents ) {

    $crontabs = explode( "\n", $crontab_contents );

    foreach ( $crontabs as $crontab ){

        $_crontab   = trim($crontab);
        if( strpos( $_crontab, "#" ) === 0 || empty( $_crontab ) )
            continue;

        $_crontab   = preg_replace("/ +/"," ",$_crontab);
        $splits     = explode(" ",$_crontab,7);
        $_params_a  = array_pop($splits);
        $times_tab  = implode(" ",$splits);
        $splits     = explode(" ",$_crontab,7);
        $commands   = array_pop($splits);
        $commands   = trim($commands);
        $_commands  = explode(" ",$commands);
        $exe_path   = $_commands[0];
        $params     = ltrim($_params_a,$exe_path);

        if( strpos($commands,"\"") ===0 ) {
            preg_match("/\"(.*?)\"/", $commands, $matches);
            $exe_path = $matches[0];
            $params   = ltrim($_params_a,$exe_path);
        }
        $exe_path   = trim($exe_path);
        $exe_path   = trim($exe_path,"\"");

        if( !file_exists($exe_path) ) {
            $exe_path = wing_get_command_path($exe_path);
        }

        $params = trim( $params );
        $crontabs_format[] = [ $times_tab, $exe_path, $params ];
    }

    return $crontabs_format;
}


/**
 * @秒规则校验
 */
function checkSecondRule( $times_tab, $start_time ) {

    $tabs       = explode(" ",$times_tab);
    $tab        = $tabs[0];
    $times_b    = date("s");

    if ( $tab == "*" )
         return true;

    if ( strpos($tab, "/") === false ) {
         if (strpos($tab, ",") !== false ) {
            //离散
            $s_temp = explode(",", $tab);
            foreach( $s_temp as $s) {
                if( $times_b == intval($s) ) {
                    return true;
                }
            }
        } else {
            return $times_b == intval($tab);
        }
    }


    $temp  = explode("/", $tab);
    if ( $temp[0] == "*" ) {
        //每多少单位执行
        if ( (time() - $start_time) % $temp[1] == 0 ) {
            return true;
        }
    }

    if ( strpos($temp[0], "-" ) === false )
         return false;

    $s_temp = explode("-", $temp[0]);
    if( ( $s_temp[1] - $s_temp[0]) < $temp[1])  {
          //格式错误
          return false;
    }

    if ($s_temp[0] <= $times_b && $times_b <= $s_temp[1]) {
        //每多少单位执行
        if ((time()-$start_time) % $temp[1]  == 0) {
            return true;
        }
    }

    return false;
}

/**
 * @分钟规则校验
 */
function checkMinuteRule( $times_tab, $start_time ){

    $tabs       = explode(" ",$times_tab);
    $tab        = $tabs[1];
    $times_b    = time();

    if( $tab == "*" )
    {
        return true;
    }

    if( strpos($tab, "/") === false ) {
        if (strpos($tab, ",") !== false) {
            //离散
            $s_temp = explode(",", $tab);
            foreach ($s_temp as $s) {
                if ($times_b == strtotime(date("Y-m-d H:$s:0"))) {
                    return true;
                }
            }
        } else {
            return $times_b == strtotime(date("Y-m-d H:$tab:0"));
        }
    }


    $temp  = explode("/", $tab);
    if ( $temp[0] == "*" ) {
        //每多少单位执行
        if (($times_b-$start_time) % ($temp[1]*60) == 0) {
            return true;
        }
    }

    if (strpos($temp[0], "-") === false)
        return false;

    $s_temp = explode("-", $temp[0]);
    if (($s_temp[1] - $s_temp[0]) < $temp[1]) {
        //格式错误
        return false;
    }

    if ( strtotime(date("Y-m-d H:".$s_temp[0].":0")) <= $times_b &&
         $times_b <= strtotime(date("Y-m-d H:".$s_temp[1].":0"))
    ) {
        //每多少单位执行
        if (($times_b-$start_time) % ($temp[1]*60)  == 0) {
            return true;
        }
    }

    return false;
}



/**
 * @小时规则校验
 */
function checkHourRule( $times_tab, $start_time ) {

    $tabs       = explode(" ",$times_tab);
    $tab        = $tabs[2];
    $times_b    = time();

    if( $tab == "*" )
    {
        return true;
    }

    if ( strpos($tab, "/") === false ) {
        if (strpos($tab, ",") !== false) {
            //离散
            $s_temp = explode(",", $tab);
            foreach ($s_temp as $s) {
                if ($times_b == strtotime(date("Y-m-d $s:0:0"))) {
                    return true;
                }
            }
        } else {
            return $times_b == strtotime(date("Y-m-d $tab:0:0"));
        }
    }


    $temp  = explode("/", $tab);
    if ( $temp[0] == "*" ) {
        //每多少单位执行
        if (($times_b-$start_time) % ($temp[1]*3600) == 0) {
            return true;
        }
    }

    if (strpos($temp[0], "-") === false)
        return false;

    $s_temp = explode("-", $temp[0]);
    if (($s_temp[1] - $s_temp[0]) < $temp[1]) {
        //格式错误
        return false;
    }

    if ( strtotime(date("Y-m-d ".$s_temp[0].":0:0")) <= $times_b &&
        $times_b <= strtotime(date("Y-m-d ".$s_temp[1].":0:0"))
    ) {
        //每多少单位执行
        if (($times_b-$start_time) % ($temp[1]*3600)  == 0) {
            return true;
        }
    }

    return false;
}

/**
 * @日规则校验
 */
function checkDayRule( $times_tab, $start_time ) {
    //##秒 分 时 日 月  星期几
    //*  *  * *  *   *
    $tabs       = explode(" ",$times_tab);
    $tab        = $tabs[3];
    $times_b    = time();

    if ( $tab == "*" )
        return true;

    if ( strpos($tab, "/") === false ) {
        if (strpos($tab, ",") !== false) {
            //离散
            $s_temp = explode(",", $tab);
            foreach ($s_temp as $s) {
                if ($times_b == strtotime(date("Y-m-$s 0:0:0"))) {
                    return true;
                }
            }
        } else {
            return $times_b == strtotime(date("Y-m-$tab 0:0:0"));
        }
    }


    $temp  = explode("/", $tab);
    if ( $temp[0] == "*" ) {
        //每多少单位执行
        if (($times_b-$start_time) % ($temp[1]*86400) == 0) {
            return true;
        }
    }

    if (strpos($temp[0], "-") === false)
        return false;

    $s_temp = explode("-", $temp[0]);
    if (($s_temp[1] - $s_temp[0]) < $temp[1]) {
        //格式错误
        return false;
    }

    if ( strtotime(date("Y-m-".$s_temp[0]." 0:0:0")) <= $times_b &&
        $times_b <= strtotime(date("Y-m-".$s_temp[1]." 0:0:0"))
    ) {
        //每多少单位执行
        if (($times_b-$start_time) % ($temp[1]*86400)  == 0) {
            return true;
        }
    }

    return false;
}


/**
 * @月规则校验，每月默认按照30天计算
 */
function checkMonthRule( $times_tab, $start_time ) {
    //##秒 分 时 日 月  星期几
    //*  *  * *  *   *
    $tabs       = explode(" ",$times_tab);
    $tab        = $tabs[4];
    $times_b    = time();

    if ( $tab == "*" )
        return true;

    if ( strpos($tab, "/") === false ) {
        if (strpos($tab, ",") !== false) {
            //离散
            $s_temp = explode(",", $tab);
            foreach ($s_temp as $s) {
                if ($times_b == strtotime(date("Y-$s-1 0:0:0"))) {
                    return true;
                }
            }
        } else {
            return $times_b == strtotime(date("Y-$tab-1 0:0:0"));
        }
    }


    $temp  = explode("/", $tab);
    if ( $temp[0] == "*" ) {
        //每多少单位执行
        if (($times_b-$start_time) % ($temp[1]*2592000) == 0) {
            return true;
        }
    }

    if (strpos($temp[0], "-") === false)
        return false;

    $s_temp = explode("-", $temp[0]);
    if (($s_temp[1] - $s_temp[0]) < $temp[1]) {
        //格式错误
        return false;
    }

    if ( strtotime(date("Y-".$s_temp[0]."-1 0:0:0")) <= $times_b &&
        $times_b <= strtotime(date("Y-".$s_temp[1]."-1 0:0:0"))
    ) {
        //每多少单位执行
        if (($times_b-$start_time) % ($temp[1]*2592000)  == 0) {
            return true;
        }
    }

    return false;
}

/**
 * @获取指定的下个星期几的具体时间
 */
function getNextWeekTime( $week ) {
    $i=1;
    while ($i<14){
        if(date("w",time()+86400*$i) == $week){
            return strtotime(date("Y-m-d 00:00:00",time()+86400*$i));
        }
        $i++;
    }
    return false;
}


/**
 * @星期规则校验
 */
function checkWeekRule( $times_tab ) {
    //##秒 分 时 日 月  星期几
    //*  *  * *  *   *
    $tabs       = explode(" ",$times_tab);
    $tab        = $tabs[5];
    $times_b    = time();
    $week       = date("w");

    if ( $tab == "*" )
        return true;

    if (strpos($tab, ",") !== false) {
        //离散
        $s_temp = explode(",", $tab);
        foreach ($s_temp as $s) {
            $_time = getNextWeekTime($s);
            if ($week == $s && $times_b == $_time) {
                return true;
            }
        }
    } else {
        $_time = getNextWeekTime($tab);
        return $week == $tab && $times_b == $_time;
    }

    return false;
}

/**
 * @执行crontab任务
 */
function crontab(){

    global $contab_file;
    $crontab_contents   = file_get_contents($contab_file);
    $crontabs_format    = paramsFormat($crontab_contents);
    static $start_times = [];

    foreach ( $crontabs_format as $crontab)  {

        $crontab_key        = md5($crontab[0]."-".$crontab[1]."-".$crontab[2]);

        if( !isset($start_times[$crontab_key]) )
            $start_times[$crontab_key] = time();

        if( checkSecondRule( $crontab[0], $start_times[$crontab_key] ) &&
            checkMinuteRule( $crontab[0], $start_times[$crontab_key] ) &&
            checkHourRule(   $crontab[0], $start_times[$crontab_key] ) &&
            checkDayRule(    $crontab[0], $start_times[$crontab_key] ) &&
            checkMonthRule(  $crontab[0], $start_times[$crontab_key] ) &&
            checkWeekRule(   $crontab[0], $start_times[$crontab_key] )
        ){
            wing_create_process( $crontab[1]." ".$crontab[2] );
        }
    }
    unset( $crontabs_format, $crontab_contents );
}

while( true ) {
    crontab();  //执行crontab
    sleep(1);   //精确到秒
}