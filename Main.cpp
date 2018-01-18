#include <windows.h>
#include <share.h>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <tchar.h>
#include <algorithm>

#include "CommunicationSocket.h"

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

void CommandDirectoryContentsChanged( LPTSTR watch_directory_path );
void ResultDirectoryContentsChanged( LPTSTR watch_directory_path, TcpCommunicationSocket & communication_socket );
void SubdirectoryChanged( LPTSTR watch_directory_root_path );
bool UpdateWatchDirectory( LPTSTR watch_directory_path, const HANDLE dwChangeHandles[ 2 ] );
void InitializeWatchDirectory( LPTSTR watch_directory_path, HANDLE dwChangeHandles[ 2 ] );

using namespace std;

void Send_Mouse()
{
	//int x_change = 0;
	//int y_change = 0;
	int wheel_change = 0;
	//mouse_event( MOUSEEVENTF_RIGHTDOWN, x_change, y_change, wheel_change, 0 );
	//mouse_event( MOUSEEVENTF_RIGHTUP, x_change, y_change, wheel_change, 0 );
	////mouse_event( MOUSEEVENTF_LEFTDOWN, x_change, y_change, wheel_change, 0 );
	////mouse_event( MOUSEEVENTF_LEFTUP, x_change, y_change, wheel_change, 0 );

	//int x_absolute = 10000;
	//int y_absolute = 1000;
	//mouse_event( MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, x_absolute, y_absolute, wheel_change, 0 );

	Sleep( 2000 );
	HWND active_window = GetActiveWindow();
	HWND focused_window = GetFocus();
	HWND foreground_window = GetForegroundWindow();
	HWND window = FindWindow( "TaskManagerWindow", "Task Manager" );

	char test1[ 129 ];
	GetWindowText( active_window, test1, 128 );
	char test2[ 129 ];
	GetWindowText( focused_window, test2, 128 );
	char test3[ 129 ];
	GetWindowText( foreground_window, test3, 128 );
	//SetFocus( focused_window );

	cout << test1 << "\n";
	//SetCursorPos( 200, 200 );
	//cin.get();
	cout << test2 << "\n";
	//SetCursorPos( 400, 400 );
	//cin.get();
	cout << test3 << "\n";

	LPCTSTR WindowName = "Processing";
	HWND Find = FindWindow( NULL, WindowName );
	while( Find == NULL )
	{
		Sleep( 1000 );
		Find = FindWindow( NULL, WindowName );
	}
	ShowWindow( Find, SW_MAXIMIZE );

	//bool success = BringWindowToTop( Find );
	//SwitchToThisWindow( Find, false );
	SetForegroundWindow( Find );
	HWND notsub_window = FindWindow( NULL, "Network security: LAN Manager authentication level Properties" );
	HWND sub_window = FindWindowEx( Find, NULL, NULL, "Network security: LAN Manager authentication level Properties" );

	//HWND Find = FindWindow( NULL, WindowName );
	//DWORD why = GetLastError();
	//if( string( test3 ) == "Omnic" )
	//	ShowWindow( foreground_window, SW_MAXIMIZE );
	//SetCursorPos( 200, 200 );

	//// Keyboard codes: https://msdn.microsoft.com/en-us/library/aa299374(v=vs.60).aspx
	//DWORD KEY_DOWN = 0;
	////keybd_event( VK_CONTROL, 0x1D, KEY_DOWN, 0 );
	////keybd_event( VK_SHIFT, 0x28, KEY_DOWN, 0 );
	//keybd_event( 'E', 0x45, KEY_DOWN, 0 );
	//keybd_event( 'E', 0x45, KEYEVENTF_KEYUP, 0 );
	////keybd_event( VK_SHIFT, 0x28, KEYEVENTF_KEYUP, 0 );
	////keybd_event( VK_CONTROL, 0x1D, KEYEVENTF_KEYUP, 0 );

	////int hotkey_id = 1;
	////RegisterHotKey( NULL, hotkey_id, MOD_ALT, 'A' );
}

bool Build_Command( const std::string & just_command, const std::string & command_arguements, Command & new_command )
{
	if( just_command.size() < 0 || just_command[0] == '#' ) // Comment line
		return false;

	new_command.held_key = Command::HOLD_NONE;
	string lowercase_command( just_command );
	std::transform( just_command.begin(), just_command.end(), lowercase_command.begin(), ::tolower );

	if( lowercase_command == "mousemove" )
	{
		size_t end_of_x = command_arguements.find_first_of( " \t\n" );
		string after_x = command_arguements.substr( end_of_x );
		size_t start_of_y = after_x.find_first_not_of( " \t\n" );
		string y_arguement = after_x.substr( start_of_y );

		new_command.operation = Command::MOUSE;
		new_command.mouse_button = Command::NO_CLICK;
		new_command.mouse_x = atoi( command_arguements.c_str() );
		new_command.mouse_y = atoi( y_arguement.c_str() );
	}
	else if( lowercase_command == "mouselclick" )
	{
		size_t end_of_x = command_arguements.find_first_of( " \t\n" );
		string after_x = command_arguements.substr( end_of_x );
		size_t start_of_y = after_x.find_first_not_of( " \t\n" );
		string y_arguement = after_x.substr( start_of_y );

		new_command.operation = Command::MOUSE;
		new_command.mouse_button = Command::LCLICK;
		new_command.mouse_x = atoi( command_arguements.c_str() );
		new_command.mouse_y = atoi( y_arguement.c_str() );
	}
	else if( lowercase_command == "text" )
	{
		new_command.operation = Command::KEYBOARD;
		new_command.keyboard_string = command_arguements;
	}
	else if( lowercase_command == "ctrltext" )
	{
		new_command.operation = Command::KEYBOARD;
		new_command.held_key = Command::HOLD_CTRL;
		new_command.keyboard_string = command_arguements;
	}
	else if( lowercase_command == "alttext" )
	{
		new_command.operation = Command::KEYBOARD;
		new_command.held_key = Command::HOLD_ALT;
		new_command.keyboard_string = command_arguements;
	}
	else if( lowercase_command == "shifttext" )
	{
		new_command.operation = Command::KEYBOARD;
		new_command.held_key = Command::HOLD_SHIFT;
		new_command.keyboard_string = command_arguements;
	}
	else if( lowercase_command == "waitfor" )
	{
		new_command.operation = Command::WINDOW_OPEN;
		new_command.keyboard_string = command_arguements;
	}
	else if( lowercase_command == "enter" )
	{
		new_command.operation = Command::HIT_ENTER;
	}
	else if( lowercase_command == "tab" )
	{
		new_command.operation = Command::HIT_TAB;
		new_command.keyboard_string = command_arguements;
	}
	else if( lowercase_command == "down" )
	{
		new_command.operation = Command::HIT_DOWN;
	}
	else if( lowercase_command == "left" )
	{
		new_command.operation = Command::HIT_LEFT;
	}
	else if( lowercase_command == "right" )
	{
		new_command.operation = Command::HIT_UP;
	}
	else if( lowercase_command == "up" )
	{
		new_command.operation = Command::HIT_RIGHT;
	}
	else if( lowercase_command == "ctrlf4" )
	{
		new_command.operation = Command::HIT_CTRLF4;
	}
	else
		return false;

	return true;
}

// Simple file parser that takes all characters before the first whitespace of each line as a command,
// and then the rest of the line (including whitespace) as the argument.  For example to type "Hello there!" on
// keyboard, you simply have a line saying text Hello there!
std::vector<Command> Load_Commands_File( const std::string & file_name )
{
	//FILE* file_ptr = _fsopen( file_name.c_str(), "r", _SH_DENYWR );
	//while( file_ptr == NULL )
	//{
	//	Sleep( 1000 );
	//	file_ptr = _fsopen( file_name.c_str(), "r", _SH_DENYWR );
	//}

	ifstream file( file_name.c_str() );
	if( !file.is_open() )
		return std::vector<Command>();

	std::vector<Command> commands_from_file;
	while( !file.eof() )
	{
		string current_line;
		std::getline( file, current_line );

		size_t first_nonwhitespace = current_line.find_first_not_of( " \t\n" );
		if( first_nonwhitespace == std::string::npos )
			continue;
		string ltrimmed = current_line.substr( first_nonwhitespace );

		size_t end_of_command = ltrimmed.find_first_of( " \t\n" );
		if( end_of_command == std::string::npos )
			end_of_command = ltrimmed.size();

		string just_command = ltrimmed.substr( 0, end_of_command );
		string command_arguements;
		if( end_of_command < ltrimmed.size() )
			command_arguements = ltrimmed.substr( end_of_command + 1 );

		Command new_command;
		if( Build_Command( just_command, command_arguements, new_command ) )
			commands_from_file.push_back( new_command );
	}

	return commands_from_file;
}

void Run_Command( const Command & command )
{
	static bool initialized = false;
	static char ascii_to_scancode[ 256 ];
	static char scancode_shift_to_ascii[] =
	{
		0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
		'_', '+', 0, '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
		'{', '}', 0, 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
		':', '\"', '~', 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
		0, 0, 0, ' '
	};
	static char scancode_to_ascii[] =
	{
		0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
		'-', '=', 0, '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
		'[', ']', 0, 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
		';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
		0, 0, 0, ' '
	};
	if( !initialized )
	{
		for( int i = 0; i < sizeof( scancode_shift_to_ascii ) / sizeof( char ); i++ )
			ascii_to_scancode[ scancode_shift_to_ascii[ i ] ] = i;
		for( int i2 = 0; i2 < sizeof( scancode_to_ascii ) / sizeof( char ); i2++ )
			ascii_to_scancode[ scancode_to_ascii[ i2 ] ] = i2;
	}
	// Keyboard codes: https://msdn.microsoft.com/en-us/library/aa299374(v=vs.60).aspx
	DWORD KEY_DOWN = 0;
	switch( command.held_key )
	{
	case Command::HOLD_ALT:
		keybd_event( VK_MENU, 0x38, KEY_DOWN, 0 );
	break;
	case Command::HOLD_CTRL:
		keybd_event( VK_CONTROL, 0x1D, KEY_DOWN, 0 );
	break;
	case Command::HOLD_SHIFT:
		keybd_event( VK_SHIFT, 0x28, KEY_DOWN, 0 );
	break;
	}

	switch( command.operation )
	{
		case Command::KEYBOARD:
		{
			for( unsigned int i = 0; i < command.keyboard_string.size(); i++ )
			{
				char character = command.keyboard_string[ i ];
				bool is_capital = character >= 'A' && character <= 'Z';
				if( is_capital )
					keybd_event( VK_SHIFT, 0x28, KEY_DOWN, 0 );

				//cout << (int)ascii_to_scancode[ character ] << ' ';
				//keybd_event( 'e', 18, KEY_DOWN, 0 );
				//keybd_event( 'e', 18, KEYEVENTF_KEYUP, 0 );

				keybd_event( VkKeyScan( character ), ascii_to_scancode[ character ], KEY_DOWN, 0 );
				keybd_event( VkKeyScan( character ), ascii_to_scancode[ character ], KEYEVENTF_KEYUP, 0 );

				if( is_capital )
					keybd_event( VK_SHIFT, 0x28, KEYEVENTF_KEYUP, 0 );
			}

		}
		break;
		case Command::MOUSE:
		{
			int x_change = 0;
			int y_change = 0;
			int wheel_change = 0;
			SetCursorPos( command.mouse_x, command.mouse_y );
			switch( command.mouse_button )
			{
			case Command::LCLICK:
				mouse_event( MOUSEEVENTF_LEFTDOWN, x_change, y_change, wheel_change, 0 );
				mouse_event( MOUSEEVENTF_LEFTUP, x_change, y_change, wheel_change, 0 );
			break;
			case Command::RCLICK:
				mouse_event( MOUSEEVENTF_RIGHTDOWN, x_change, y_change, wheel_change, 0 );
				mouse_event( MOUSEEVENTF_RIGHTUP, x_change, y_change, wheel_change, 0 );
			break;
			default:
			break;
			}
		}
		break;
		case Command::WINDOW_OPEN: // Waits for window to exist, blocking until it does, then gives it attention
		{
			HWND Find = FindWindow( NULL, command.keyboard_string.c_str() );
			while( Find == NULL )
			{
				Sleep( 1000 );
				Find = FindWindow( NULL, command.keyboard_string.c_str() );
			}
			SetForegroundWindow( Find );
		}
		break;
		case Command::HIT_ENTER:
			keybd_event( VkKeyScan( 0x0D ), 0x1C, KEY_DOWN, 0 );
			keybd_event( VkKeyScan( 0x0D ), 0x1C, KEYEVENTF_KEYUP, 0 );
		break;
		case Command::HIT_TAB:
		{
			int number_of_tabs = atoi( command.keyboard_string.c_str() );
			for( int i = 0; i < number_of_tabs; i++ )
			{
				keybd_event( VkKeyScan( 0x09 ), 0x0F, KEY_DOWN, 0 );
				keybd_event( VkKeyScan( 0x09 ), 0x0F, KEYEVENTF_KEYUP, 0 );
			}
		}
		break;
		case Command::HIT_DOWN:
			keybd_event( VK_DOWN, 208, KEY_DOWN, 0 );
			keybd_event( VK_DOWN, 208, KEYEVENTF_KEYUP, 0 );
		break;
		case Command::HIT_LEFT:
			keybd_event( VK_LEFT, 203, KEY_DOWN, 0 );
			keybd_event( VK_LEFT, 203, KEYEVENTF_KEYUP, 0 );
		break;
		case Command::HIT_UP:
			keybd_event( VK_UP, 200, KEY_DOWN, 0 );
			keybd_event( VK_UP, 200, KEYEVENTF_KEYUP, 0 );
		break;
		case Command::HIT_RIGHT:
			keybd_event( VK_RIGHT, 205, KEY_DOWN, 0 );
			keybd_event( VK_RIGHT, 205, KEYEVENTF_KEYUP, 0 );
		break;
		case Command::HIT_CTRLF4:
			keybd_event( VK_CONTROL, 0x1D, KEY_DOWN, 0 );
				keybd_event( VK_F4, 0x3E, KEY_DOWN, 0 );
				keybd_event( VK_F4, 0x3E, KEYEVENTF_KEYUP, 0 );
			keybd_event( VK_CONTROL, 0x1D, KEYEVENTF_KEYUP, 0 );
		break;
	}

	switch( command.held_key )
	{
	case Command::HOLD_ALT:
		keybd_event( VK_MENU, 0x38, KEYEVENTF_KEYUP, 0 );
	break;
	case Command::HOLD_CTRL:
		keybd_event( VK_CONTROL, 0x1D, KEYEVENTF_KEYUP, 0 );
	break;
	case Command::HOLD_SHIFT:
		keybd_event( VK_SHIFT, 0x28, KEYEVENTF_KEYUP, 0 );
	break;
	}
}

HWND OpenOmnicSoftware()
{
	LPCTSTR WindowName = "OMNIC - [Window1]";
	HWND FindOmnic = FindWindow( NULL, WindowName );
	if( FindOmnic == NULL )
	{
		printf( "Omnic not found, reopen it if it is opened\n" );
		TCHAR omnic_path[ 256 ];
		TCHAR output_path[ 256 ];
		GetPrivateProfileString( _T( "Folders" ), _T( "Omnic_Executable_Location" ), _T( "" ), omnic_path, 256, _T( ".\\config.ini" ) );
		GetPrivateProfileString( _T( "Folders" ), _T( "Write_Output_Folder" ), _T( "" ), output_path, 256, _T( ".\\config.ini" ) );
		std::string change_directory_for_output( output_path );
		change_directory_for_output = "cd " + change_directory_for_output + " && " + string( omnic_path );
		system( change_directory_for_output.c_str() );
	}
	while( FindOmnic == NULL )
	{
		FindOmnic = FindWindow( NULL, WindowName );
		Sleep( 2000 );
	}
	ShowWindow( FindOmnic, SW_MAXIMIZE );
	{ // Clear first window with Ctrl + F4 so future commands start with blank slate
		DWORD KEY_DOWN = 0;
		keybd_event( VK_CONTROL, 0x1D, KEY_DOWN, 0 );
		keybd_event( VK_F4, 0x3E, KEY_DOWN, 0 );
		keybd_event( VK_F4, 0x3E, KEYEVENTF_KEYUP, 0 );
		keybd_event( VK_CONTROL, 0x1D, KEYEVENTF_KEYUP, 0 );
	}

	SetForegroundWindow( FindOmnic );

	return FindOmnic;
}

void main()
{
	//Send_Mouse();
	//std::vector<Command> commands = Load_Commands_File( "commands.txt" );
	//for( int command_i = 0; command_i < commands.size(); command_i++ )
	//	Run_Command( commands[ command_i ] );
	TCHAR read_folder[ 256 ];
	GetPrivateProfileString( _T( "Folders" ), _T( "Read_Commands_Folder" ), _T( "" ), read_folder, 256, _T( ".\\config.ini" ) );
	TCHAR write_folder[ 256 ];
	GetPrivateProfileString( _T( "Folders" ), _T( "Write_Output_Folder" ), _T( "" ), write_folder, 256, _T( ".\\config.ini" ) );

//#ifndef RYAN_COMPUTER
//	HWND FindOmnic = OpenOmnicSoftware();
//#endif
	
	CommandDirectoryContentsChanged( read_folder ); // Run any command files already in the folder
	HANDLE Command_File_Change_Handles[ 2 ];
	HANDLE Result_File_Change_Handles[ 2 ];
	InitializeWatchDirectory( read_folder, Command_File_Change_Handles );
	InitializeWatchDirectory( write_folder, Result_File_Change_Handles );

	// Change notification is set. Now wait on both notification 
	// handles and refresh accordingly. 
	TcpCommunicationSocket communicator;
	UdpListenerSocket listen_for_controller;
	listen_for_controller.ListenOnPort( 6542, "Omnic Controller" );
	//communicator.ConnectToHost( 6542, "127.0.0.1" );
	while( true )
	{
		if( UpdateWatchDirectory( read_folder, Command_File_Change_Handles ) )
			CommandDirectoryContentsChanged( read_folder );
		if( UpdateWatchDirectory( write_folder, Result_File_Change_Handles ) )
			ResultDirectoryContentsChanged( write_folder, communicator );
		listen_for_controller.Update( communicator );
		communicator.Update();
		//communicator.SendFile( "Simple File.txt" );
	}

	WSACleanup(); //Clean up Winsock
}

// Code adapted from https://msdn.microsoft.com/en-us/library/windows/desktop/aa365261(v=vs.85).aspx
void InitializeWatchDirectory( LPTSTR watch_directory_path, HANDLE dwChangeHandles[ 2 ] )
{
	DWORD dwWaitStatus;
	TCHAR lpDrive[ 4 ];
	TCHAR lpFile[ _MAX_FNAME ];
	TCHAR lpExt[ _MAX_EXT ];

	_tsplitpath( watch_directory_path, lpDrive, NULL, lpFile, lpExt ); // Changed from safe version to it will compile for windows NT

	lpDrive[ 2 ] = (TCHAR)'\\';
	lpDrive[ 3 ] = (TCHAR)'\0';

	// Watch the directory for file creation and deletion. 

	dwChangeHandles[ 0 ] = FindFirstChangeNotification(
		watch_directory_path,                         // directory to watch 
		FALSE,                         // do not watch subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME ); // watch file name changes 

	if( dwChangeHandles[ 0 ] == INVALID_HANDLE_VALUE )
	{
		printf( "\n ERROR: FindFirstChangeNotification function failed.\n" );
		ExitProcess( GetLastError() );
	}

	// Watch the subtree for directory creation and deletion. 

	dwChangeHandles[ 1 ] = FindFirstChangeNotification(
		lpDrive,                       // directory to watch 
		TRUE,                          // watch the subtree 
		FILE_NOTIFY_CHANGE_DIR_NAME );  // watch dir name changes 

	if( dwChangeHandles[ 1 ] == INVALID_HANDLE_VALUE )
	{
		printf( "\n ERROR: FindFirstChangeNotification function failed.\n" );
		ExitProcess( GetLastError() );
	}


	// Make a final validation check on our handles.

	if( (dwChangeHandles[ 0 ] == NULL) || (dwChangeHandles[ 1 ] == NULL) )
	{
		printf( "\n ERROR: Unexpected NULL from FindFirstChangeNotification.\n" );
		ExitProcess( GetLastError() );
	}

	printf( ("Watching Folder: " + string( watch_directory_path ) + "\n").c_str() );
}

bool UpdateWatchDirectory( LPTSTR watch_directory_path, const HANDLE dwChangeHandles[ 2 ] )
{
	// Wait for notification.
	DWORD dwWaitStatus = WaitForMultipleObjects( 2, dwChangeHandles,
											FALSE, 500 );

	switch( dwWaitStatus )
	{
		case WAIT_OBJECT_0:

		// A file was created, renamed, or deleted in the directory.
		// Refresh this directory and restart the notification.

		if( FindNextChangeNotification( dwChangeHandles[ 0 ] ) == FALSE )
		{
			printf( "\n ERROR: FindNextChangeNotification function failed.\n" );
			ExitProcess( GetLastError() );
		}
		return true;
		break;

		case WAIT_OBJECT_0 + 1:

		// A directory was created, renamed, or deleted.
		// Refresh the tree and restart the notification.

		SubdirectoryChanged( watch_directory_path );
		if( FindNextChangeNotification( dwChangeHandles[ 1 ] ) == FALSE )
		{
			printf( "\n ERROR: FindNextChangeNotification function failed.\n" );
			ExitProcess( GetLastError() );
		}
		break;

		case WAIT_TIMEOUT:

		// A timeout occurred, this would happen if some value other 
		// than INFINITE is used in the Wait call and no changes occur.
		// In a single-threaded environment you might not want an
		// INFINITE wait.

		//printf( "\nNo changes in the timeout period.\n" );
		break;

		default:
		printf( "\n ERROR: Unhandled dwWaitStatus.\n" );
		ExitProcess( GetLastError() );
		break;
	}
	return false;
}

void ResultDirectoryContentsChanged( LPTSTR watch_directory_path, TcpCommunicationSocket & communication_socket )
{
	string full_directory_path = watch_directory_path;

	if( full_directory_path.size() > (MAX_PATH - 3) )
	{
		_tprintf( TEXT( "\nDirectory path is too long.\n" ) );
		return;
	}

	//_tprintf( TEXT( "Target directory is %s\n" ), watch_directory_path );

	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.

	string path_with_wildcard = full_directory_path + "\\*.csv";

	// Find the first file in the directory.
	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile( path_with_wildcard.c_str(), &ffd );

	if( INVALID_HANDLE_VALUE == hFind )
	{
		return;
	}

	// List all the files in the directory with some info about them.

	do
	{
		if( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			_tprintf( TEXT( "  %s   <DIR>\n" ), ffd.cFileName );
		}
		else
		{
			LARGE_INTEGER filesize;
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			_tprintf( TEXT( "  %s   %ld bytes\n" ), ffd.cFileName, filesize.QuadPart );

			Sleep( 3000 );
			communication_socket.SendFile( watch_directory_path + string( "\\" ) + ffd.cFileName );
			DeleteFile( (watch_directory_path + string( "\\" ) + ffd.cFileName).c_str() );
		}
	} while( FindNextFile( hFind, &ffd ) != 0 );
}

void CommandDirectoryContentsChanged( LPTSTR watch_directory_path )
{
	// This is where you might place code to refresh your
	// directory listing, but not the subtree because it
	// would not be necessary.

	//printf( TEXT( "Directory (%s) changed.\n" ), watch_directory_path );
	// Find the first file in the directory.

	// Check that the input path plus 3 is not longer than MAX_PATH.
	// Three characters are for the "\*" plus NULL appended below.

	string full_directory_path = watch_directory_path;

	if( full_directory_path.size() > (MAX_PATH - 3) )
	{
		_tprintf( TEXT( "\nDirectory path is too long.\n" ) );
		return;
	}

	_tprintf( TEXT( "\nTarget directory is %s\n\n" ), watch_directory_path );

	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.

	string path_with_wildcard = full_directory_path + "\\*.command";

	// Find the first file in the directory.
	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile( path_with_wildcard.c_str(), &ffd );

	if( INVALID_HANDLE_VALUE == hFind )
	{
		return;
	}

	// List all the files in the directory with some info about them.

	do
	{
		if( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			_tprintf( TEXT( "  %s   <DIR>\n" ), ffd.cFileName );
		}
		else
		{
			LARGE_INTEGER filesize;
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			_tprintf( TEXT( "  %s   %ld bytes\n" ), ffd.cFileName, filesize.QuadPart );

			Sleep( 3000 );
			std::vector<Command> commands = Load_Commands_File( (watch_directory_path + string( "\\" ) + ffd.cFileName).c_str() );
			for( int command_i = 0; command_i < commands.size(); command_i++ )
				Run_Command( commands[ command_i ] );
			DeleteFile( (watch_directory_path + string( "\\" ) + ffd.cFileName).c_str() );
		}
	} while( FindNextFile( hFind, &ffd ) != 0 );
}

void SubdirectoryChanged( LPTSTR watch_directory_root_path )
{
	// This is where you might place code to refresh your
	// directory listing, including the subtree.

	printf( TEXT( "Directory tree (%s) changed.\n" ), watch_directory_root_path );
}

