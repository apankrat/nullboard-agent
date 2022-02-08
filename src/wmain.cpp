/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#include "types.h"
#include "socket_io.h"
#include "http_request.h"
#include "utils.h"
#include "trace.h"
#include "config.h"
#include "console.h"

#include "engine.h"
#include "ui.h"

//
BOOL __stdcall on_console_event(dword ctrl)
{
	if (ctrl == CTRL_C_EVENT)
	{
		trace_v("Ctrl-C\n");
		stop_ui();
		return TRUE;
	}

	if (ctrl == CTRL_CLOSE_EVENT)
	{
		stop_ui();
		return TRUE;
	}

	return FALSE;
}

//
int wmain_alt(int argc, wchar_t ** argv)
{
	HANDLE en = NULL;
	HANDLE ui = NULL;

	if (! open_console())  // hidden by default
		return 10;

	SetConsoleCtrlHandler(on_console_event, TRUE);

	if (! init_conf())     // conf.path = %LocalAppData%\Nullboard
		return 20;

	if (! parse_args(argc, argv))
		return 30;

	if (! load_ini())
		return 40;

	if (! make_path(conf.path))
		return 50;

	if (! init_engine())
		return 60;

	en = start_engine();

	ui = start_ui();

	for (;;)
	{
		HANDLE foo[2] = { en, ui };

		auto rc = WaitForMultipleObjects(2, foo, FALSE, 100);

		if (rc == WAIT_OBJECT_0 + 1)
		{
			trace_v("UI stopped\n");
			stop_engine();
			break;
		}

		if (rc == WAIT_OBJECT_0)
		{
			trace_v("Engine stopped\n");
			stop_ui();
			return 70;
		}
	}

	return 0;
}

int wmain_app(int argc, wchar_t ** argv)
{
	int rc = wmain_alt(argc, argv);

	if (rc != 0)
	{
		show_console(true);

		printf("\nPress Enter to exit...");
		getchar();
	}

	return rc;
}
