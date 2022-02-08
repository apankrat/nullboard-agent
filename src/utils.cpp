/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#include "utils.h"
#include "trace.h"
#include <shlobj.h>

bool get_special_folder(int csid, wstring & path)
{
	wchar_t buf[MAX_PATH+1] = { 0 };

	if (! SHGetSpecialFolderPath(NULL, buf, csid, FALSE))
	{
		api_error("SHGetSpecialFolderPath", "csid: %d", csid);
		return false;
	}

	if (buf[0] == 0)
	{
		trace_e("SHGetSpecialFolderPath(%d) is malformed", csid);
		return false;
	}

	path = buf;
	return true;
}

bool get_local_app_data(wstring & path)
{
	return get_special_folder(CSIDL_LOCAL_APPDATA, path);
}

bool folder_exists(const wstring & path)
{
	auto attrs = GetFileAttributes(path.c_str());
	return (attrs != -1) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool make_path(const wstring & path)
{
	if (path.empty())
		return false;

	auto attrs = GetFileAttributes(path.c_str());

	if (attrs != -1)
		return (attrs & FILE_ATTRIBUTE_DIRECTORY);

	auto pos = path.find_last_of(L'\\');

	if (pos != -1 && ! make_path( path.substr(0, pos) ))
		return false;

	if (! CreateDirectory(path.c_str(), NULL))
	{
		api_error("CreateDirectory", "%s", to_utf8(path).c_str());
		return false;
	}

	return true;
}

bool save_file(const wstring & file, const ch_range & data)
{
	HANDLE h;
	DWORD bytes = 0;

	h = CreateFile(file.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE)
	{
		api_error("CreateFile", "%s", to_utf8(file).c_str());
		return false;
	}

	if (! WriteFile(h, data.data, (dword)data.size, &bytes, NULL) || bytes != data.size)
	{
		api_error("WriteFile", "%s %lu %lu", to_utf8(file).c_str(), data.size, bytes);
		CloseHandle(h);
		return false;
	}

	CloseHandle(h);
	return true;
}

bool read_file(const wstring & file, string & data, size_t size_cap)
{
	HANDLE h;
	uint64_t size;
	DWORD bytes = 0;

	data.clear();

	h = CreateFile(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE)
	{
		api_error("CreateFile", "%s", to_utf8(file).c_str());
		return false;
	}

	if (! GetFileSizeEx(h, (LARGE_INTEGER*)&size) || size > size_cap)
	{
		api_error("GetFileSizeEx", "%s", to_utf8(file).c_str());
		CloseHandle(h);
		return false;
	}

	if (! size)
	{
		CloseHandle(h);
		return true;
	}

	data.resize( (size_t)size );
	if (! ReadFile(h, &data[0], (dword)data.size(), &bytes, NULL) || bytes != data.size())
	{
		api_error("ReadFile", "%s %lu %lu", to_utf8(file).c_str(), data.size(), bytes);
		CloseHandle(h);
		return false;
	}

	CloseHandle(h);
	return true;
}

//
string to_utf8(const wchar_t * str, size_t len)
{
	string s;
	int s_len;

	if (len == -1)
		len = wcslen(str);

	if (! len)
		return s;

	s_len = WideCharToMultiByte(CP_UTF8, 0, str, (int)len, NULL, 0, NULL, NULL);
	if (s_len <= 0)
		return s;

	s.resize(s_len);
	s_len = WideCharToMultiByte(CP_UTF8, 0, str, (int)len, &s[0], s_len, NULL, NULL);

	__enforce(s_len == (int)s.size());
	return s;
}

wstring to_wstr(const char * str, size_t len)
{
	wstring w;
	int w_len;

	if (len == -1)
		len = strlen(str);

	if (! len)
		return w;

	w_len = MultiByteToWideChar(CP_UTF8, 0, str, (int)len, NULL, 0);
	if (w_len <= 0)
		return w;

	w.resize(w_len);
	w_len = MultiByteToWideChar(CP_UTF8, 0, str, (int)len, &w[0], w_len);

	__enforce(w_len == (int)w.size());
	return w;
}

//
string ip_to_str(uint32_t addr)
{
	char buf[32] = { 0 };
	snprintf(buf, 15, "%u.%u.%u.%u", 
		(addr >> 24) & 0xff, (addr >> 16) & 0xff, 
		(addr >> 8) & 0xff, addr & 0xff);
	return buf;
}

string sa_to_str(uint32_t addr, uint16_t port)
{
	char buf[32] = { 0 };
	snprintf(buf, 15+6, "%u.%u.%u.%u:%u", 
		(addr >> 24) & 0xff, (addr >> 16) & 0xff, 
		(addr >> 8) & 0xff, addr & 0xff, port);
	return buf;
}

string sa_to_str(const sockaddr_in & sa)
{
	return sa_to_str(htonl(sa.sin_addr.S_un.S_addr), htons(sa.sin_port));
}
