/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#ifndef _UTILS_H_
#define _UTILS_H_

#include "types.h"
#include "ch_range.h"

// file system

bool get_special_folder(int csid, wstring & path);
bool get_local_app_data(wstring & path);
bool folder_exists(const wstring & path);
bool make_path(const wstring & path);
bool save_file(const wstring & file, const ch_range & data);
bool read_file(const wstring & file, string & data, size_t size_cap = 1024*1024);

// string conversions

string  to_utf8(const wchar_t * str, size_t len = -1);
wstring to_wstr(const char    * str, size_t len = -1);

inline string  to_utf8(const wstring & str) { return to_utf8(str.data(), str.size()); }
inline wstring to_wstr(const string  & str) { return to_wstr(str.data(), str.size()); }

// string formatting

string ip_to_str(uint32_t addr);
string sa_to_str(uint32_t addr, uint16_t port);
string sa_to_str(const sockaddr_in & sa);

#endif