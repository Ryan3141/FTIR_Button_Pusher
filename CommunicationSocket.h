#pragma once

//CONNECT TO REMOTE HOST (CLIENT APPLICATION)
//Include the needed header files.
//Don't forget to link libws2_32.a to your program as well
#include <winsock.h>
#include <string>
#include <vector>

struct SocketMessage
{
	int size;
	std::vector<char> message;
};

class TcpCommunicationSocket
{
public:
	TcpCommunicationSocket();
	~TcpCommunicationSocket();
	//CONNECTTOHOST – Connects to a remote host
	bool ConnectToHost( int port_number, char* IPAddress );
	int ListenOnPort( int port_number );
	void Update();
	void CloseConnection();
	void SendFile( std::string path_to_file );
	bool socket_connected;

private:
	SOCKET s; //Socket handle
	WSADATA w;
	SocketMessage partial_message;
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
