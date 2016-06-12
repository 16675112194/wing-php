<?php namespace Service;
/**
 * @author yuyi
 * @created 2016/6/3 21:55
 * @email 297341015@qq.com
 */
class Tcp{
    private $config = [
        "port" => 6998,
        "listen" => "0.0.0.0"
    ];


    public function registerCallback($callback_key,$callback_func){
        $this->config[$callback_key] = $callback_func;
    }


    public function onreceive($client,$msg){
        wing_socket_send_msg($client,
            "hello wing\r\n");
        unset($client,$msg);
    }
    public function start(){
        $_self = $this;
        $params["onreceive"] = function($client,$msg) use($_self){
            $_self->onreceive($client,$msg);
        };
        $params["onconnect"]    = function($client){};
        $params["onclose"]      = function(){};
        $params["onerror"]      = function(){};
        $params["port"]         = $this->config["port"];
        $params["listen"]       = $this->config["listen"];
        register_shutdown_function(function(){
            wing_service_stop();
        });
        wing_service($params);
    }
}