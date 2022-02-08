/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#include "engine.h"
#include "socket_io.h"
#include "http_request.h"
#include "utils.h"
#include "trace.h"
#include "config.h"

//
struct the_engine
{
	the_engine()  { srv = -1; enough = false; self = NULL; }
	~the_engine() { closesocket(srv); }

	bool init();
	void run();
	void stop();

	bool recv_payload(size_t bytes);

	bool send_cors_ok();
	bool send_ok();

	bool handle_api_request (http_req & req);
	bool handle_put_test    (area_info & area, ch_range_vec & args);
	bool handle_put_config  (area_info & area, ch_range_vec & args);
	bool handle_put_board   (area_info & area, ch_range_vec & args, const string & id);
	bool handle_del_board   (area_info & area, const ch_range & id);

	//
	SOCKET   srv;
	bool     enough;
	sk_conn  conn;
	HANDLE   self;
};

/*
 *	misc
 */
static bool init_winsock()
{
	static WSADATA wsa_data = { 0 };
	int rc;

	if (wsa_data.wHighVersion)
		return true;

	rc = WSAStartup(MAKEWORD(2, 0), &wsa_data);
	if (rc != 0)
	{
		trace_e("WSAStartup() failed with %d\n", rc);
		return false;
	}

	return true;
}

static string nope(const char * details, int code, const char * desc)
{
	string r;
	char  why[64] = { 0 };

	snprintf(why, sizeof(why)-1, "HTTP/1.1 %d %s\r\n", code, desc);

	r = why;
	r += "Access-Control-Allow-Origin: *\r\n"
	     "Cache-Control: no-cache\r\n"
	     "Content-Type: text/plain\r\n"
	     "\r\n";

	return r + details;
}

static string nope_400(const char * details)
{
	return nope(details, 400, "Bad request");
}

static string nope_500(const char * details)
{
	return nope(details, 500, "Internal error");
}

/*
 *	public
 */
bool the_engine::init()
{
	sockaddr_in addr = { AF_INET };

	__enforce(srv == -1);

	if (! init_winsock())
		return false;

	srv = socket(AF_INET, SOCK_STREAM, 0);
	if (srv == -1)
		return wsa_error("socket");

	trace_v("Server socket created\n");

//	if (! sk_reuseaddr(srv))
//		goto err;

	addr.sin_port = htons(conf.port);
	addr.sin_addr.S_un.S_addr = htonl(conf.addr);

	if (bind(srv, (sockaddr*)&addr, sizeof addr) < 0)
	{
		wsa_error("bind");
		goto err;
	}

	trace_v("Server socket bound to %s\n", sa_to_str(addr).c_str());

	if (listen(srv, 16) < 0)
	{
		wsa_error("listen");
		goto err;
	}

	trace_i("Server is up\n");
	return true;

err:
	closesocket(srv);
	srv = -1;

	return false;
}

void the_engine::run()
{
	__enforce(srv != -1);

	// one loop per connection

	while (! enough)
	{
		sockaddr_in peer = { AF_INET };
		int alen = sizeof(peer);
		http_req req;

		__enforce(conn.sk == -1);

		//
		conn.sk = accept(srv, (sockaddr*)&peer, &alen);
		if (conn.sk < 0)
		{
			wsa_error("accept");

			if (sk_accept_fatal())
				break;

			continue;
		}

		trace_i("Connection accepted from %s\n", sa_to_str(peer).c_str());

		if (! sk_unblock(conn.sk))
			goto drop;

		conn.buf.resize(8*1024);

		// read HTTP request headers - ie just read up to \r\n\r\n

		for (;;)
		{
			int rc;

			rc = sk_recv(conn, 2); // 2 seconds
			if (rc <= 0)
				goto drop;

			rc = parse_http_request(conn, req);
			if (rc < 0)
				goto drop; // something's malformed

			if (rc > 0)
				break;

			conn.replenish_buf();
		}

		//
		if (req.verb.match("options"))
		{
			send_cors_ok();
			goto drop;
		}

		if (req.verb.match("put") || req.verb.match("delete"))
		{
			handle_api_request(req);
			goto drop;
		}

		sk_send(conn, "HTTP/1.1 405 Unsupported Method\r\n");

drop:
		conn.clear();
		trace_i("Connection closed\n\n");

		on_engine_activity();
	}

	closesocket(srv);
	srv = -1;

	trace_i("Server shut down\n");
}

void the_engine::stop()
{
	if (srv == -1)
		return;

	enough = true;
	closesocket(srv);
	WaitForSingleObject(self, -1);
}

/*
 *	private
 */
bool the_engine::recv_payload(size_t bytes)
{
	while (conn.fill < conn.pos + bytes)
	{
		conn.replenish_buf();

		int rc = sk_recv(conn);
		if (rc <= 0)
			return false;
	}

	trace_v("Payload:\n-------\n%.*s\n-------\n", conn.fill-conn.pos, conn.buf.data()+conn.pos);

	return true;
}

bool the_engine::send_cors_ok()
{
	const char * open_bar =
		"HTTP/1.1 204 No Content\r\n"
		"Allow: OPTIONS, GET, PUT, DELETE\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Access-Control-Allow-Headers: *\r\n"
		"Access-Control-Allow-Methods: OPTIONS, GET, PUT, DELETE\r\n"
		"Cache-Control: no-cache\r\n";

	return sk_send(conn, (char*)open_bar) > 0;
}

bool the_engine::send_ok()
{
	const char * ok =
		"HTTP/1.1 204 OK\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Cache-Control: no-cache\r\n";

	return sk_send(conn, (char*)ok) > 0;
}

/*
 *	put     /test
 *	put     /config
 *	put     /board/<board-id>
 *	delete  /board/<board-id>
 */
bool the_engine::handle_api_request(http_req & req)
{
	ch_range_vec parts;

	if (! req.path.starts_with("/"))
	{
		trace_e("Invalid path 1\n");
		sk_send(conn, nope_400("Invalid path, 1"));
		return false;
	}

	req.path.tokenize("/", parts, true);

	if (parts.size() < 1)
	{
		trace_e("Invalid path 2\n");
		sk_send(conn, nope_400("Invalid path, 2"));
		return false;
	}

	//
	ch_range  * clen = NULL;
	ch_range  * auth = NULL;
	area_info * area = NULL;
	size_t bytes;
	int n;

	for (auto & h : req.headers)
		if (h.name.match("content-length"))
			clen = &h.value;
		else
		if (h.name.match("x-access-token"))
			auth = &h.value;

	if (! auth)
	{
		trace_i("No X-Access-Token header\n");
		sk_send(conn, nope_400("No Access-Token header"));
		return false;
	}

	for (auto & a : conf.areas)
	{
		if (auth->match( (string&)a.first ))
		{
			area = &a.second;
			break;
		}
	}

	if (! area)
	{
		trace_i("X-Access-Token mismatch\n");
		sk_send(conn, nope("Invalid access token", 403, "Access denied"));
		return false;
	}

	if (req.verb.match("put"))
	{
		ch_range      bulk;
		ch_range_vec  args;

		if (! clen)
		{
			trace_e("No Content-Length header\n");
			sk_send(conn, nope_400("No Content-Length header"));
			return false;
		}

		if (clen->scanf("%zu%n", &bytes, &n) != 1 || n != clen->size)
		{
			trace_e("Invalid Content-Length header\n");
			sk_send(conn, nope_400("Invalid Content-Length header"));
			return false;
		}

		if (! recv_payload(bytes))
			return false;

		/*
			Content-Type: application/x-www-form-urlencoded; charset=UTF-8
			data=%7B%22format%22%3...&meta=%7B%22...
		*/

		bulk = ch_range( (char*)conn.buf.data() + conn.pos, conn.fill - conn.pos );
		bulk.tokenize("&", args, false);

		if (parts.size() == 1 && parts[0].match("config"))
			return handle_put_config(*area, args);

		if (parts.size() == 2 && parts[0].match("board"))
			return handle_put_board(*area, args, parts[1].to_str());

		trace_e("Invalid PUT request\n");
	}
	else
	if (req.verb.match("delete"))
	{
		if (parts.size() == 2 && parts[0].match("board"))
			return handle_del_board(*area, parts[1]);

		trace_e("Invalid DELETE request\n");
	}

	sk_send(conn, nope_400("Invalid request"));
	return false;
}

//
bool the_engine::handle_put_test(area_info & area, ch_range_vec & args)
{
	string self;

	for (auto & arg : args)
	{
		ch_range k, v;

		if (! arg.split("=", k, v))
			continue;

		if (k.match("self")) self = v.to_str();
	}

	percent_decode(self);

	//
	if (self.size())
	{
		area.url = to_wstr(self);
		save_ini();
	}

	return send_ok();
}

bool the_engine::handle_put_config(area_info & area, ch_range_vec & args)
{
	wstring  path;
	wstring  file;
	string   self;
	string   data;

	trace_i("put /config\n");

	for (auto & arg : args)
	{
		ch_range k, v;

		if (! arg.split("=", k, v))
			continue;

		if (k.match("self")) self = v.to_str(); else;
		if (k.match("conf")) data = v.to_str();
	}

	percent_decode(self);
	percent_decode(data);

	//
	path = conf.path + L"\\" + area.folder;

	if (! make_path(path))
	{
		trace_e("Failed to create [%S] folder\n", path.c_str());
		sk_send(conn, nope_500("make_path() failed"));
		return false;
	}

	if (data.size())
	{
		file = path + L"\\app-config.json";
		if (! save_file(file, data))
		{
			trace_e("Failed to save [%S]\n", file.c_str());
			sk_send(conn, nope_500("save_file() failed"));
			return false;
		}
	}

	if (self.size())
	{
		area.url = to_wstr(self);
		save_ini();
	}

	return send_ok();
}

bool the_engine::handle_put_board(area_info & area, ch_range_vec & args, const string & _id)
{
	ch_range  id_str( (string&)_id );
	uint64_t  id_u64;
	wstring   path;
	wstring   file;
	string    self;
	string    data;
	string    meta;

	trace_i("put /board/%.*s\n", __str(id_str));

	//
	if (! id_str.is_decimal() || ! id_str.scanf("%I64u", &id_u64))
	{
		trace_e("Invalid board id\n");
		sk_send(conn, nope_400("Invalid board ID"));
		return false;
	}

	path = conf.path + L"\\" + area.folder + L"\\" + id_str.to_wstr();

	//
	for (auto & arg : args)
	{
		ch_range k, v;

		if (! arg.split("=", k, v))
			continue;

		if (k.match("self")) self = v.to_str(); else;
		if (k.match("data")) data = v.to_str(); else
		if (k.match("meta")) meta = v.to_str();
	}

	percent_decode(self);
	percent_decode(data);
	percent_decode(meta);

/*
	self: { }
	meta: {"title":"1232","current":3,"ui_spot":0,"history":[3,2,1],"backups":[]}
	data: {"format":20190412,"id":1618261845169,"revision":3,"title":"1232","lists":[{"title":"List","notes":[{"text":"123","raw":false,"min":false}]}]}
 */

	if (! make_path(path))
	{
		trace_e("Failed to create [%S] folder\n", path.c_str());
		sk_send(conn, nope_500("make_path() failed"));
		return false;
	}

	if (meta.size())
	{
		trace_v("Meta:\n-------\n%s\n-------\n", meta.data());

		file = path + L"\\meta.json";
		if (! save_file(file, meta))
		{
			trace_e("Failed to save [%S]\n", file.c_str());
			sk_send(conn, nope_500("save_file() failed"));
			return false;
		}

		trace_i("meta saved in [%S]\n", file.c_str());
	}

	if (data.size())
	{
		ch_range foo(data);
		char   * ptr;
		uint_t   rev = 0;
		wchar_t  name[64] = { 0 };

		trace_v("Data:\n-------\n%s\n-------\n", data.data());

		ptr = foo.find("\"revision\":");
		if (! ptr)
		{
			trace_e("Failed to find board revision\n");
			sk_send(conn, nope_400("No revision in board data"));
			return false;
		}

		foo.advance_to(ptr + 11);
		if (! foo.scanf("%u", &rev))
		{
			trace_e("Invalid board revision\n");
			sk_send(conn, nope_400("Bad board revision"));
			return false;
		}

		wsprintf(name, L"rev-%08u.nbx", rev);
		file = path + L"\\" + name;

		if (! save_file(file, data))
		{
			trace_e("Failed to save [%S]\n", file.c_str());
			sk_send(conn, nope_500("save_file() failed"));
			return false;
		}

		trace_i("data saved in [%S]\n", file.c_str());
	}

	if (self.size())
	{
		area.url = to_wstr(self);
		save_ini();
	}

	return send_ok();
}

//
bool the_engine::handle_del_board(area_info & area, const ch_range & id)
{
	uint64_t  board_id;
	wstring   path;
	wstring   arch;

	// id references conn.buf (!)

	trace_i("delete /board/%.*s\n", __str(id));

	if (! id.is_decimal() || ! id.scanf("%I64u", &board_id))
	{
		trace_e("Invalid board id\n");
		sk_send(conn, nope_400("Invalid board ID"));
		return false;
	}

	path = conf.path + L"\\" + area.folder + L"\\" + id.to_wstr();
	arch = conf.path + L"\\" + area.folder + L"\\$DeletedBoards";

	if (! folder_exists(path))
	{
		trace_e("Non-existent board\n");
		sk_send(conn, nope_400("Non-existent board"));
		return false;
	}

	if (! make_path(arch))
	{
		trace_e("Failed to create [%S] folder\n", arch.c_str());
		sk_send(conn, nope_500("make_path() failed"));
		return false;
	}

	arch += L"\\" + id.to_wstr();

	if (! MoveFileEx(path.c_str(), arch.c_str(), 0))
	{
		trace_e("MoveFileEx() failed %lu\n", GetLastError());
		trace_i("[%S] -> [%S]\n", path.c_str(), arch.c_str());
		sk_send(conn, nope_500("move_file() failed"));
		return false;
	}

	return send_ok();
}

//
static the_engine en;

static dword __stdcall en_thread(void * p)
{
	((the_engine*)p)->run();
	return 0;
}

//
bool init_engine()
{
	return en.init();
}

HANDLE start_engine()
{
	__enforce(en.srv != -1);
	en.self = CreateThread(NULL, 0, en_thread, &en, 0, NULL);
	return en.self;
}

void stop_engine()
{
	en.stop();
}
