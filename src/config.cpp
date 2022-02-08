/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#include "config.h"
#include "trace.h"
#include "utils.h"
#include "console.h"

//
app_config conf;

//
bool syntax()
{
	show_console();
	conf.console = true;

	trace_i("Syntax: nullboard-agent.exe [-c <etc-path>] [-v|-vv] [-d]\n");
	return false;
}

bool init_conf()
{
	if (! get_local_app_data(conf.path))
		return false;

	conf.path += L"\\Nullboard";
	return true;
}

bool parse_args(int argc, wchar_t ** argv)
{
	for (int i=1; i<argc; i++)
	{
		trace_d("argv: [%s]\n", to_utf8(argv[i]).c_str());

		if (! wcsncmp(argv[i], L"-v", 2))
		{
			for (wchar_t * p = argv[i]+1; *p == L'v'; p++)
				conf.trace++;

			if (conf.trace > 4) conf.trace = 4;

			trace_v("conf.trace: level %u\n", conf.trace);
			continue;
		}

		if (! wcscmp(argv[i], L"-d"))
		{
			show_console();
			conf.console = true;
			trace_v("conf.console: %u\n", conf.console);
			continue;
		}

		if (! wcscmp(argv[i], L"-c"))
		{
			if (++i == argc)
				return syntax();

			conf.path = argv[i];
			trace_v("conf.path: [%s]\n", to_utf8(conf.path).c_str());
			continue;
		}

//		if (! wcscmp(argv[i], L"-a"))
//		{
//			if (++i == argc)
//				return syntax();
//
//			auto a = argv[i];
//			auto p = wcschr(a, L':');
//
//			if (! p || p == a || p[1] == 0)
//				return syntax();
//
//			auto t = to_utf8(a, p-a);
//			auto f = p+1;
//
//			conf.areas[t] = f;
//			trace_v("conf.area: token [%s], folder [%s]\n", t.c_str(), to_utf8(f).c_str());
//			continue;
//		}

		return syntax();
	}

	return true;
}

bool load_ini()
{
	wstring   file = conf.path + L"\\settings.ini";
	string    data;
	ch_range  blob;
	ch_range  line;
	int       line_i = 0;

	if (! read_file(file, data) || data.empty())
	{
		trace_v("%s not found or empty\n", to_utf8(file).c_str());
		return true;
	}

	trace_v("Parsing %s\n", to_utf8(file).c_str());

	blob = ch_range(data);

	while (blob.get_line(line))
	{
		ch_range k, v;

		line_i++;

		if (line.trim())
			continue;

		if (line.starts_with("#"))
			continue;

		if (! line.split_alt(" \t", k, v))
			continue;

		trace_d("[%.*s] -> [%.*s]\n", __str(k), __str(v));

		if (k.match("trace"))
		{
			if (! v.scanf("%u", &conf.trace) || conf.trace < 2)
				goto malformed;

			trace_v("conf.trace: level %u\n", conf.trace);
			continue;
		}

		if (k.match("console"))
		{
			conf.console = v.match("1") || v.match("yes");
			show_console(conf.console);
			trace_v("conf.console: %u\n", conf.console);
			continue;
		}

		if (k.match("area"))
		{
			vector<ch_range> parts;

			v.tokenize("|", parts, false);

			if (parts.size() != 3 || parts[0].empty() || parts[1].empty())
				goto malformed;

			conf.areas[ parts[0].to_str() ] = { parts[1].to_wstr(), parts[2].to_wstr() };

			trace_v("conf.area: token [%.*s], folder [%.*s], page [%.*s]\n",
				__str(parts[0]), __str(parts[1]), __str(parts[2]));
			continue;
		}

		if (k.match("listen"))
		{
			uint_t x[5];

			if (v.scanf("%u.%u.%u.%u:%u", x, x+1, x+2, x+3, x+4) != 5 ||
			    x[0] > 0xff || x[1] > 0xff || x[2] > 0xff || x[3] > 0xff ||
			    x[4] > 0xffff)
				goto malformed;

			conf.addr = 0;
			for (int i=0; i<4; i++) conf.addr = (conf.addr << 8) | x[i];
			conf.port = x[4];

			trace_v("conf.listen: %s\n", sa_to_str(conf.addr, conf.port).c_str());
			continue;
		}

		if (k.match("say_hello"))
		{
			conf.say_hello = v.match("1");
			trace_v("conf.say_hello: %u\n", conf.say_hello);
			continue;
		}

		trace_v("Unknown \"%.*s\" entry in line %d in %s\n",
			__str(k), line_i, to_utf8(file).c_str());
		continue;

malformed:
		trace_e("Invalid \"%.*s\" entry in line %d in %s\n",
			__str(k), line_i, to_utf8(file).c_str());
		return false;
	}

	return true;
}

//
string stringf(const char * format, ...); // import from libp

static string key_str(const char * str, size_t min_len = 20)
{
	string x = str;

	if (x.size() < min_len)
		x.append(min_len - x.size(), ' ');

	return x;
}

bool save_ini()
{
	wstring  file = conf.path + L"\\settings.ini";
	string   text;

	text += key_str("trace")     + stringf("%u\r\n", conf.trace);
	text += key_str("console")   + stringf("%u\r\n", conf.console);
	text += key_str("listen")    + sa_to_str(conf.addr, conf.port) + "\r\n";
	text += key_str("say_hello") + stringf("%u\r\n", conf.say_hello);

	text += "\r\n";

	for (auto & a : conf.areas)
		text += key_str("area") + a.first + "|" + to_utf8(a.second.folder) + "|" + to_utf8(a.second.url) + "\r\n";

	return save_file(file, text);
}
