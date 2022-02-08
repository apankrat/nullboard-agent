/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#include "console.h"

static HANDLE console = NULL;

bool open_console()
{
	DWORD mode;
	FILE * foo;

	__enforce(! console);

	if (! AllocConsole())
		return false;

	ShowWindow(GetConsoleWindow(), SW_HIDE);

	console = GetStdHandle(STD_INPUT_HANDLE);
	if (! console)
	{
		FreeConsole();
		return false;
	}

	//
	SetConsoleOutputCP(CP_UTF8);

	if (GetConsoleMode(console, &mode) && ! (mode & ENABLE_QUICK_EDIT_MODE))
	{
		SetConsoleMode(console, mode | ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE);
	}

	//
	freopen_s(&foo, "CONIN$",  "r", stdin);
	freopen_s(&foo, "CONOUT$", "w", stdout);
	freopen_s(&foo, "CONOUT$", "w", stderr);

	setvbuf( stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	return true;
}

void show_console(bool show)
{
	ShowWindow(GetConsoleWindow(), show ? SW_SHOW : SW_HIDE);
}
