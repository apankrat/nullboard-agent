/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include "socket_io.h"
#include "ch_range.h"

//
struct uri_info
{
	ch_range  path;
	ch_range  query;
	ch_range  frag;
};

//
struct qkv_pair
{
	ch_range  k;
	ch_range  v;
};

typedef vector<qkv_pair> qkv_pair_vec;

//
struct http_hdr
{
	ch_range  raw;

	ch_range  name;
	ch_range  value;
};

typedef vector<http_hdr> http_hdr_vec;

//
struct http_req
{
	// 1st line
	ch_range  verb;
	ch_range  uri;
	ch_range  proto;

	// parsed
	ch_range      path;    // uri.path
	qkv_pair_vec  args;    // uri.query
	http_hdr_vec  headers; // 2nd+ line
};

//
int parse_http_request(sk_conn & conn, http_req & req);

void percent_decode(string & str);

#endif
