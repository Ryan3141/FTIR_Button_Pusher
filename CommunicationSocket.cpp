#include "CommunicationSocket.h"
#include <stdio.h>
#include <fstream>
#include <vector>

////Return the IP address of a domain name
//
//DECLARE_STDCALL_P( struct hostent * ) gethostbyname( const char* );
//
////Convert a string address (i.e., "127.0.0.1") to an IP address. Note that  
////this function returns the address into the correct byte order for us so 
////that we do not need to do any conversions (see next section)
//
//unsigned long PASCAL inet_addr( const char* );


//// Convert from normal byte order to network changed order
//u_long PASCAL htonl( u_long ); //Host to network long
//u_long PASCAL ntohl( u_long ); //Network to host long
//
//u_short PASCAL htons( u_short ); //Host to network short
//u_short PASCAL ntohs( u_short ); //Network to host short

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


using namespace std;

//LISTENONPORT – Listens on a specified port for incoming connections 
//or data
int TcpCommunicationSocket::ListenOnPort( int port_number )
{
	int error = WSAStartup( 0x0202, &w );   // Fill in WSA info

	if( error )
	{
		return false; //For some reason we couldn't start Winsock
	}

	if( w.wVersion != 0x0202 ) //Wrong Winsock version?
	{
		WSACleanup();
		return false;
	}

	SOCKADDR_IN addr; // The address structure for a TCP socket

	addr.sin_family = AF_INET;      // Address family
	addr.sin_port = htons( port_number );   // Assign port to this socket

	//Accept a connection from any IP using INADDR_ANY
	//You could pass inet_addr("0.0.0.0") instead to accomplish the 
	//same thing. If you want only to watch for a connection from a 
	//specific IP, specify that //instead.
	addr.sin_addr.s_addr = htonl( INADDR_ANY );

	s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ); // Create socket

	if( s == INVALID_SOCKET )
	{
		return false; //Don't continue if we couldn't create a //socket!!
	}

	if( bind( s, (LPSOCKADDR)&addr, sizeof( addr ) ) == SOCKET_ERROR )
	{
		//We couldn't bind (this will happen if you try to bind to the same  
		//socket more than once)
		return false;
	}

	//Now we can start listening (allowing as many connections as possible to  
	//be made at the same time using SOMAXCONN). You could specify any 
	//integer value equal to or lesser than SOMAXCONN instead for custom 
	//purposes). The function will not //return until a connection request is 
	//made
	auto results = listen( s, SOMAXCONN );
	if( listen( s, SOMAXCONN ) == SOCKET_ERROR )
		printf( "listen function failed with error: %d\n", WSAGetLastError() );
//	listen( s, SOMAXCONN );

	u_long nonblocking = 1;
	int iResult = ioctlsocket( s, FIONBIO, &nonblocking );
	if( iResult != NO_ERROR )
		printf( "ioctlsocket failed with error: %ld\n", iResult );

	return 0;
}

bool TcpCommunicationSocket::ConnectToHost( int port_number, char* IPAddress )
{
	//Start up Winsock…
	WSADATA wsadata;

	int error = WSAStartup( 0x0202, &wsadata );

	//Did something happen?
	if( error )
		return false;

	//Did we get the right Winsock version?
	if( wsadata.wVersion != 0x0202 )
	{
		WSACleanup(); //Clean up Winsock
		return false;
	}

	//Fill out the information needed to initialize a socket…
	SOCKADDR_IN target; //Socket address information

	target.sin_family = AF_INET; // address family Internet
	target.sin_port = htons( port_number ); //Port to connect on
	target.sin_addr.s_addr = inet_addr( IPAddress ); //Target IP

	s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ); //Create socket
	if( s == INVALID_SOCKET )
	{
		return false; //Couldn't create the socket
	}

	u_long nonblocking = 0;
	int iResult = ioctlsocket( s, FIONBIO, &nonblocking );
	if( iResult != NO_ERROR )
		printf( "ioctlsocket failed with error: %ld\n", iResult );

	//Try connecting...

	if( connect( s, (SOCKADDR *)&target, sizeof( target ) ) == SOCKET_ERROR )
	{
		printf( "connect function failed with error: %d\n", WSAGetLastError() );
		// WSAEWOULDBLOCK
		this->socket_connected = false;
		return false; //Couldn't connect
	}
	else
	{
		printf( "Successfully connected to %s:%d\n", IPAddress, port_number );
		this->socket_connected = true;
		return true; //Success
	}
}

void TcpCommunicationSocket::Update()
{
	if( !socket_connected )
		return;

	u_long nonblocking = 1;
	int iResult = ioctlsocket( s, FIONBIO, &nonblocking );
	if( iResult != NO_ERROR )
		printf( "ioctlsocket failed with error: %ld\n", iResult );

	const int BUFLEN = 512;  //Max length of buffer
	char buf[ BUFLEN ];
	int recv_len;
	struct sockaddr_in si_other;
	int slen = sizeof( si_other );
	//printf( "Waiting for data..." );
	//fflush( stdout );

	//clear the buffer by filling null, it might have previously received data
	memset( buf, '\0', BUFLEN );

	int iSendResult = send( s, "Sup bra\n", 8, 0 );

	//try to receive some data, this is a blocking call
	if( (recv_len = recvfrom( s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen )) == SOCKET_ERROR )
	{
		int error = WSAGetLastError();
		if( error == WSAEWOULDBLOCK )
		{
			//printf( "Non blocking\n" );
			return;
		}
		else
		{
			printf( "recvfrom() failed with error code : %d\n", WSAGetLastError() );
			//exit( EXIT_FAILURE );
		}
	}

	string data = buf;

	if( data == "PING\n" )
		return;

	//print details of the client/peer and the data received
	printf( "Received packet from %s:%d\n", inet_ntoa( si_other.sin_addr ), ntohs( si_other.sin_port ) );
	printf( "Data: %s\n", buf );

	//now reply the client with the same data
	if( sendto( s, buf, recv_len, 0, (struct sockaddr*) &si_other, slen ) == SOCKET_ERROR )
	{
		printf( "sendto() failed with error code : %d\n", WSAGetLastError() );
		//exit( EXIT_FAILURE );
	}
}

void TcpCommunicationSocket::SendFile( std::string path_to_file )
{
	if( !this->socket_connected )
		return;

	printf( "Sending file: %s\n", path_to_file.c_str() );
	ifstream file( path_to_file, ios::binary );
	if( path_to_file.empty() )
		return;

	//std::copy( std::istreambuf_iterator<char>( file ),
	//		   std::istreambuf_iterator<char>(),
	//		   this->partial_message.message.begin() );
	vector<char> test_out( (std::istreambuf_iterator<char>( file )),
						  ( std::istreambuf_iterator<char>()) );

	struct sockaddr_in si_other;
	int slen = sizeof( si_other );
	//if( sendto( s, &this->partial_message.message[ 0 ], this->partial_message.message.size(), 0, (struct sockaddr*) &si_other, slen ) == SOCKET_ERROR )
	char size_as_string[ 64 ];
	_itoa_s( test_out.size(), size_as_string, 64, 10 );
	string header_info = "FILE " + string( size_as_string ) + "\n";
	if( sendto( s, header_info.c_str(), header_info.size(), 0, (struct sockaddr*) &si_other, slen ) == SOCKET_ERROR )
	{
		printf( "File sending header sendto() failed with error code : %d\n", WSAGetLastError() );
		//exit( EXIT_FAILURE );
	}
	if( sendto( s, &(test_out[ 0 ]), test_out.size(), 0, (struct sockaddr*) &si_other, slen ) == SOCKET_ERROR )
	{
		printf( "File sending sendto() failed with error code : %d\n", WSAGetLastError() );
		//exit( EXIT_FAILURE );
	}
}

//CLOSECONNECTION – shuts down the socket and closes any connection on it
void TcpCommunicationSocket::CloseConnection()
{
	//Close the socket if it exists
	if( s )
		closesocket( s );
}

TcpCommunicationSocket::TcpCommunicationSocket()
{
	socket_connected = false;
}

TcpCommunicationSocket::~TcpCommunicationSocket()
{
	CloseConnection();
}

//LISTENONPORT – Listens on a specified port for incoming connections 
//or data
int UdpListenerSocket::ListenOnPort( int port_number, const string & id_string )
{
	struct sockaddr_in server;
	WSADATA wsa;
	this->id_string = id_string;
	this->listen_port = port_number;


	//Initialise winsock
	//printf( "\nInitialising Winsock..." );
	if( WSAStartup( MAKEWORD( 2, 2 ), &wsa ) != 0 )
	{
		printf( "Failed. Error Code : %d\n", WSAGetLastError() );
		exit( EXIT_FAILURE );
	}
	//printf( "Initialised.\n" );

	//Create a socket
	if( (s = socket( AF_INET, SOCK_DGRAM, 0 )) == INVALID_SOCKET )
	{
		printf( "Could not create socket : %d\n", WSAGetLastError() );
	}
	//printf( "Socket created.\n" );

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( port_number );

	//Bind
	if( bind( s, (struct sockaddr *)&server, sizeof( server ) ) == SOCKET_ERROR )
	{
		printf( "Bind failed with error code : %d\n", WSAGetLastError() );
		exit( EXIT_FAILURE );
	}
	//puts( "Bind done" );

	u_long nonblocking = 1;
	int iResult = ioctlsocket( s, FIONBIO, &nonblocking );
	if( iResult != NO_ERROR )
		printf( "ioctlsocket failed with error: %ld\n", iResult );

	printf( "Listening to UDP packets on port %d for %s\n", port_number, id_string.c_str() );

	return 0;
}

void UdpListenerSocket::Update( TcpCommunicationSocket & connect_on_message_available )
{
	const int BUFLEN = 512;  //Max length of buffer
	char buf[ BUFLEN ];
	int recv_len;
	struct sockaddr_in si_other;
	int slen = sizeof( si_other );
	//printf( "Waiting for data..." );
	//fflush( stdout );

	//clear the buffer by filling null, it might have previously received data
	memset( buf, '\0', BUFLEN );

	//try to receive some data, this is a blocking call
	if( (recv_len = recvfrom( s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen )) == SOCKET_ERROR )
	{
		int error = WSAGetLastError();
		if( error == WSAEWOULDBLOCK )
		{
			//printf( "Non blocking\n" );
			return;
		}
		else
		{
			printf( "recvfrom() failed with error code : %d\n", WSAGetLastError() );
			return;
			//exit( EXIT_FAILURE );
		}
	}

	//print details of the client/peer and the data received
	char* peer_ip_address = inet_ntoa( si_other.sin_addr );
	int peer_port = ntohs( si_other.sin_port );
	printf( "Received packet from %s:%d\n", peer_ip_address, peer_port );
	printf( "Data: %s\n", buf );

	if( buf == this->id_string )
	{
		connect_on_message_available.CloseConnection(); // Remove any previous connection
		connect_on_message_available.ConnectToHost( this->listen_port, peer_ip_address ); // Make new connection
	}

	////now reply the client with the same data
	//if( sendto( s, buf, recv_len, 0, (struct sockaddr*) &si_other, slen ) == SOCKET_ERROR )
	//{
	//	printf( "sendto() failed with error code : %d", WSAGetLastError() );
	//	exit( EXIT_FAILURE );
	//}
}

//CLOSECONNECTION – shuts down the socket and closes any connection on it
void UdpListenerSocket::CloseConnection()
{
	//Close the socket if it exists
	if( s )
		closesocket( s );
}

UdpListenerSocket::~UdpListenerSocket()
{
	CloseConnection();
}
