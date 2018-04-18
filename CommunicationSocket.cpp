#include "CommunicationSocket.h"
#include <stdio.h>
#include <fstream>
#include <vector>
#include <algorithm>

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


std::vector<Command> Make_Commands_From_Vector( const std::vector<std::string> & split_into_lines );
bool Build_Command( const std::string & just_command, const std::string & command_arguements, Command & new_command );
std::vector<Command> Parse_Partial_Incoming_Message( std::string & partial_message );

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
		this->peer_ip_address = IPAddress;
		this->peer_port = port_number;

		return true; //Success
	}
}

std::string TcpCommunicationSocket::Get_Peer_IP() const
{
	return this->peer_ip_address;
}

int TcpCommunicationSocket::Get_Peer_Port() const
{
	return this->peer_port;
}

vector<Command> TcpCommunicationSocket::Update()
{
	if( !this->socket_connected )
		return vector<Command>();

	u_long nonblocking = 1;
	int iResult = ioctlsocket( s, FIONBIO, &nonblocking );
	if( iResult != NO_ERROR )
		printf( "ioctlsocket failed with error: %ld\n", iResult );

	struct sockaddr_in si_other;
	int slen = sizeof( si_other );
	//printf( "Waiting for data..." );
	//fflush( stdout );


	string ping_message = "Ping\n";
	if( sendto( s, ping_message.c_str(), ping_message.size(), 0, (struct sockaddr*) &si_other, slen ) == SOCKET_ERROR )
	{
		int error = WSAGetLastError();
		if( error == WSAEWOULDBLOCK )
		{
			////printf( "Non blocking\n" );
			//return vector<Command>();
		}
		else
		{
			this->socket_connected = false;
			printf( "Disconnected from %s:%d\n", this->Get_Peer_IP().c_str(), this->Get_Peer_Port() );
			printf( "recvfrom() failed with error code : %d\n", WSAGetLastError() );
			return vector<Command>();
		}
	}

	//clear the buffer by filling null, it might have previously received data
	const int BUFLEN = 512;  //Max length of buffer
	char buf[ BUFLEN ];
	memset( buf, '\0', BUFLEN );
	//try to receive some data, this is a blocking call
	int recv_len;
	if( (recv_len = recvfrom( s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen )) == SOCKET_ERROR )
	{
		int error = WSAGetLastError();
		if( error == WSAEWOULDBLOCK )
		{
			//printf( "Non blocking\n" );
			return vector<Command>();
		}
		else if( error == WSAECONNRESET )
		{
			this->socket_connected = false;
			printf( "Disconnected from %s:%d\n", this->Get_Peer_IP().c_str(), this->Get_Peer_Port() );
			return vector<Command>();
		}
		else

		{
			this->socket_connected = false;
			printf( "Disconnected from %s:%d\n", this->Get_Peer_IP().c_str(), this->Get_Peer_Port() );
			printf( "recvfrom() failed with error code : %d\n", WSAGetLastError() );
			return vector<Command>();
			//exit( EXIT_FAILURE );
		}
	}

	if( recv_len == -1 )
		return vector<Command>();

	string data = buf;

	if( data == "PING\n" )
		return vector<Command>();

	//print details of the client/peer and the data received
	printf( "Received packet from %s:%d\n", this->Get_Peer_IP().c_str(), this->Get_Peer_Port() );
	//printf( "Data: %s\n", buf );

	partial_message.insert( partial_message.end(), buf, buf + recv_len );

	return Parse_Partial_Incoming_Message( this->partial_message );

	////now reply the client with the same data
	//if( sendto( s, buf, recv_len, 0, (struct sockaddr*) &si_other, slen ) == SOCKET_ERROR )
	//{
	//	printf( "sendto() failed with error code : %d\n", WSAGetLastError() );
	//	//exit( EXIT_FAILURE );
	//}
}

// This big horrible mess just reads the possibly incomplete input, splits them into commands,
// and if it is a file, it puts the lines back together to check if the full file is there yet
static vector<Command> Parse_Partial_Incoming_Message( std::string & partial_message)
{
	vector<string> split_by_line = split( partial_message, '\n' );
	vector<Command> output_list;

	int original_number_of_lines = split_by_line.size();
	for( int i = 0; i < original_number_of_lines - 1; i++ )
	{
		string & current_line = split_by_line[ i ];

		size_t first_nonwhitespace = current_line.find_first_not_of( " \t\n" );
		if( first_nonwhitespace == std::string::npos )
			continue;
		string ltrimmed = current_line.substr( first_nonwhitespace );

		size_t end_of_command = ltrimmed.find_first_of( " \t\n" );
		if( end_of_command == std::string::npos )
			end_of_command = ltrimmed.size();

		string just_command = ltrimmed.substr( 0, end_of_command );

		string lowercase_command( just_command );
		std::transform( just_command.begin(), just_command.end(), lowercase_command.begin(), ::tolower );
		string command_arguements;
		if( end_of_command < ltrimmed.size() )
			command_arguements = ltrimmed.substr( end_of_command + 1 );

		if( lowercase_command == "file" )
		{
			unsigned int length_of_file = atoi( command_arguements.c_str() );
			unsigned int length_of_header = current_line.size() + 1;

			partial_message = join( split_by_line.begin() + i, split_by_line.end(), std::string( "\n" ) );

			if( partial_message.size() >= length_of_header + length_of_file )
			{
				vector<string> split_by_line = split( partial_message.substr( length_of_header, length_of_file ), '\n' );
				partial_message = partial_message.substr( length_of_header + length_of_file );
				vector<Command> add_this = Make_Commands_From_Vector( split_by_line );
				output_list.insert( output_list.end(), add_this.begin(), add_this.end() );

				// Since we screwup up the order, manually fix it
				int new_number_of_lines = split( partial_message, '\n' ).size();
				i += original_number_of_lines - new_number_of_lines - 1;
			}
			else
				break; // We don't want to use any of the data until everything is here
		}
		else
		{
			Command new_command;
			if( Build_Command( just_command, command_arguements, new_command ) )
				output_list.push_back( new_command );
		}
	}

	return output_list;
}

void TcpCommunicationSocket::SendMessage( std::string message )
{
	if( !this->socket_connected )
		return;

	struct sockaddr_in si_other;
	int slen = sizeof( si_other );

	u_long nonblocking = 1;
	int iResult = ioctlsocket( s, FIONBIO, &nonblocking );
	if( iResult != NO_ERROR )
		printf( "ioctlsocket failed with error: %ld\n", iResult );

	if( sendto( s, &(message[ 0 ]), message.size(), 0, (struct sockaddr*) &si_other, slen ) == SOCKET_ERROR )
	{
		printf( "File sending sendto() failed with error code : %d\n", WSAGetLastError() );
		//exit( EXIT_FAILURE );
	}
}
void TcpCommunicationSocket::SendFile( std::string path_to_file, std::string file_name )
{
	if( !this->socket_connected )
		return;

	if( path_to_file.empty() || file_name.empty() )
		return;
	printf( "Sending file: %s\n", file_name.c_str() );

	vector<char> file_data;
	{
		ifstream file( (path_to_file + "\\" + file_name).c_str(), ios::binary | ios::ate); // ios::ate opens file with pointer at end of file
		if( !file.is_open() )
			return;
		//std::copy( std::istreambuf_iterator<char>( file ),
		//		   std::istreambuf_iterator<char>(),
		//		   this->partial_message.message.begin() );

		streampos size = file.tellg();
		file_data.resize( size );
		file.seekg( 0, ios::beg );
		file.read( &file_data[ 0 ], size );
		file.close();
	}

	//string test_out( (std::istreambuf_iterator<char>( file )),
	//					  ( std::istreambuf_iterator<char>()) );

	struct sockaddr_in si_other;
	int slen = sizeof( si_other );
	//if( sendto( s, &this->partial_message.message[ 0 ], this->partial_message.message.size(), 0, (struct sockaddr*) &si_other, slen ) == SOCKET_ERROR )
	char size_as_string[ 64 ];
	_itoa_s( file_data.size(), size_as_string, 64, 10 );

	u_long nonblocking = 1;
	int iResult = ioctlsocket( s, FIONBIO, &nonblocking );
	if( iResult != NO_ERROR )
		printf( "ioctlsocket failed with error: %ld\n", iResult );

	string header_info = "FILE \"" + file_name + "\" " + string( size_as_string ) + "\n";
	if( sendto( s, header_info.c_str(), header_info.size(), 0, (struct sockaddr*) &si_other, slen ) == SOCKET_ERROR )
	{
		printf( "File sending header sendto() failed with error code : %d\n", WSAGetLastError() );
		//exit( EXIT_FAILURE );
	}
	if( sendto( s, &(file_data[ 0 ]), file_data.size(), 0, (struct sockaddr*) &si_other, slen ) == SOCKET_ERROR )
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

	if( !connect_on_message_available.socket_connected && buf == this->id_string )
	{
		//print details of the client/peer and the data received
		char* peer_ip_address = inet_ntoa( si_other.sin_addr );
		int peer_port = ntohs( si_other.sin_port );
		printf( "Received packet from %s:%d\n", peer_ip_address, peer_port );
		printf( "Data: %s\n", buf );

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
