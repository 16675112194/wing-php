#include <iostream>
#include <hash_map>
using namespace std;
/*
hash_map<unsigned long,unsigned long> wing_sockets_hash_map;

void wing_add_to_sockets_map(unsigned long socket,unsigned long ovl){
	if( socket <=0 || ovl<=0 ) return;
	wing_sockets_hash_map[socket] = ovl;
}

unsigned long wing_get_from_sockets_map(unsigned long socket){
	if( wing_sockets_hash_map.find(socket) == wing_sockets_hash_map.end() ) return 0;
	return wing_sockets_hash_map[socket];
}

void wing_remove_from_sockets_map(unsigned long socket){
	if( socket <=0 ) return;
	wing_sockets_hash_map.erase(socket);
}

unsigned int wing_get_sockets_map_size(){
	return wing_sockets_hash_map.size();
}

hash_map<unsigned long,unsigned long> wing_get_sockets_map(){
	return wing_sockets_hash_map;
}*/