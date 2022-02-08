/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "types.h"

//
struct area_info
{
	wstring  folder;
	wstring  url;
};

typedef map<string, area_info> area_map;

//
struct app_config
{
	uint_t    trace;            // error (0) ... debug (4)
	bool      console;
	wstring   path;
	uint32_t  addr;
	uint16_t  port;
	area_map  areas;
	bool      say_hello;        // "up and running"

	app_config()
	{
		trace = 2;          // info
		console = false;
		addr = 0x7F000001;  // 127.0.0.1;
		port = 10001;
//		areas["TestToken"] = { L"TestFolder", L"" }
		say_hello = true;
	}
};

extern app_config conf;

//
bool init_conf();
bool parse_args(int argc, wchar_t ** argv);
bool load_ini();
bool save_ini();

#endif
