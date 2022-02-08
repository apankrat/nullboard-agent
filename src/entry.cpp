/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#include "enforce.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <exception>

/*
 *
 */
#define __try_cpp_exceptions__    try
#define __catch_cpp_exceptions__  \
	catch (std::exception & ) { printf("\nWhoops - std::exception\n"); }

/*
 *
 */
#define __try_seh_exceptions__    __try
#define __catch_seh_exceptions__  __except ( EXCEPTION_EXECUTE_HANDLER ) { printf("\nWhoops - seh::exception\n"); }

/*
 *
 */
void on_assert(const char * exp, const char * file, const char * func, int line)
{
	printf("\nWhoops - assertion failed - line %d\n", line);
	exit( 100 ); // RC_whoops_assert
}

/*
 *
 */
int wmain_app(int argc, wchar_t ** argv);

int wmain_seh(int argc, wchar_t ** argv)
{
	int r = 101; // RC_whoops_seh

	__try_seh_exceptions__
	{
		r = wmain_app(argc, argv);
	}
	__catch_seh_exceptions__

	return r;
}

/*
 *	subsystem: console
 */
int wmain(int argc, wchar_t ** argv)
{
	int r = 102; // RC_whoops_cpp

	__try_cpp_exceptions__
	{
		r = wmain_seh(argc, argv);
	}
	__catch_cpp_exceptions__

	return r;
}

/*
 *	subsystem: windows
 */
int __stdcall wWinMain(HINSTANCE instance, HINSTANCE prev_instance, 
                       LPWSTR _cmd_line, int show_cmd)
{
	extern HINSTANCE sfc_instance;

	sfc_instance = instance;
	return wmain(__argc, __wargv);
}



/*
 *	super-duper crap-o-matic
 */
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
