#include "termio.h"

#include <stdio.h>
#include <stdlib.h>

#include "utils/misc.h"

#if defined(_WIN32) && !defined(__CYGWIN__)

#include <conio.h>
#include <windows.h>

namespace termio
{

	static HANDLE stdin_handle()
	{
		return GetStdHandle( STD_INPUT_HANDLE );
	}

	bool isKeyPressed()
	{
		return _kbhit() != 0;
	}

	void setBlockingInput( bool enabled )
	{
		HANDLE h = stdin_handle();
		DWORD mode = 0;
		if( !GetConsoleMode( h, &mode ) )
			return;

		if( !enabled )
			mode &= ~( ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT );
		else
			mode |= ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT;

		SetConsoleMode( h, mode );
	}

	void discardInput()
	{
		while( _kbhit() )
			_getch();
	}

	void setEchoEnabled( bool enabled )
	{
		HANDLE h = stdin_handle();
		DWORD mode = 0;
		if( !GetConsoleMode( h, &mode ) )
			return;

		if( enabled )
			mode |= ENABLE_ECHO_INPUT;
		else
			mode &= ~ENABLE_ECHO_INPUT;

		SetConsoleMode( h, mode );
	}

}

#else

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

namespace termio
{

	bool isKeyPressed()
	{
		struct timeval tv;
		fd_set fds;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);
		select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
		return FD_ISSET(STDIN_FILENO, &fds);
	}

	void setBlockingInput( bool enabled )
	{
		struct termios ttystate;

		tcgetattr(STDIN_FILENO, &ttystate);

		if( !enabled )
		{
			ttystate.c_lflag &= ~ICANON;
			ttystate.c_cc[VMIN] = 1;
		}
		else
		{
			ttystate.c_lflag |= ICANON;
		}
		tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
	}

	void discardInput()
	{
		tcflush( STDIN_FILENO, TCIFLUSH );
	}

	void setEchoEnabled( bool enabled )
	{
		if( enabled )
			SYSTEM( "stty echo" );
		else
			SYSTEM( "stty -echo" );
	}

}

#endif
