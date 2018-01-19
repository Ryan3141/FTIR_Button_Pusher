#pragma once

//CONNECT TO REMOTE HOST (CLIENT APPLICATION)
//Include the needed header files.
//Don't forget to link libws2_32.a to your program as well
#include <winsock.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

#ifndef RYAN_COMPUTER
#define _itoa_s(number, buffer, size, radix) itoa(number, buffer, radix)
#endif

struct Command // A poorly structured type to contain all the information about a single command
{
	enum Operation { MOUSE, KEYBOARD, WINDOW_OPEN, HIT_ENTER, HIT_TAB, HIT_DOWN, HIT_LEFT, HIT_UP, HIT_RIGHT, HIT_CTRLF4 };
	enum Press { NO_CLICK, LCLICK, RCLICK };
	enum Hold_Key { HOLD_NONE, HOLD_CTRL, HOLD_ALT, HOLD_SHIFT };
	Press mouse_button;
	Hold_Key held_key;
	Operation operation;
	int mouse_x;
	int mouse_y;

	std::string keyboard_string;
};


class TcpCommunicationSocket
{
public:
	TcpCommunicationSocket();
	~TcpCommunicationSocket();
	//CONNECTTOHOST – Connects to a remote host
	bool ConnectToHost( int port_number, char* IPAddress );
	int ListenOnPort( int port_number );
	std::vector<Command> Update();
	void CloseConnection();
	void SendFile( std::string path_to_file );
	std::string Get_Peer_IP() const;
	int Get_Peer_Port() const;
	bool socket_connected;

private:
	std::string peer_ip_address;
	int peer_port;
	SOCKET s; //Socket handle
	WSADATA w;
	std::string partial_message;
};

class UdpListenerSocket
{
public:
	~UdpListenerSocket();
	//CONNECTTOHOST – Connects to a remote host
	int ListenOnPort( int port_number, const std::string & id_string );
	void Update( TcpCommunicationSocket & connect_on_message_available );
	void CloseConnection();

private:
	SOCKET s; //Socket handle
	WSADATA w;
	std::string id_string;
	int listen_port;
};

// Taken from https://stackoverflow.com/questions/236129/the-most-elegant-way-to-iterate-the-words-of-a-string?page=1&tab=votes#tab-top
template<typename Out>
inline void split( const std::string &s, char delim, Out result )
{
	std::stringstream ss( s );
	std::string item;
	while( std::getline( ss, item, delim ) )
	{
		*(result++) = item;
	}
}

inline std::vector<std::string> split( const std::string &s, char delim )
{
	std::vector<std::string> elems;
	split( s, delim, std::back_inserter( elems ) );
	return elems;
}

// Taken from https://stackoverflow.com/questions/1430757/c-vector-to-string
template <class T, class A>
inline T join( const A &begin, const A &end, const T &t )
{
	T result;
	for( A it = begin;
		 it != end;
		 it++ )
	{
		if( !result.empty() )
			result.append( t );
		result.append( *it );
	}
	return result;
}
