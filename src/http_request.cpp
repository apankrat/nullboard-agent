/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#include "http_request.h"
#include "trace.h"

/*
 	OCTET          = <any 8-bit sequence of data>
	CHAR           = <any US-ASCII character (octets 0 - 127)>
	UPALPHA        = <any US-ASCII uppercase letter "A".."Z">
	LOALPHA        = <any US-ASCII lowercase letter "a".."z">
	ALPHA          = UPALPHA | LOALPHA
	DIGIT          = <any US-ASCII digit "0".."9">
	CTL            = <any US-ASCII control character (octets 0 - 31) and DEL (127)>
	CR             = <US-ASCII CR, carriage return (13)>
	LF             = <US-ASCII LF, linefeed (10)>
	SP             = <US-ASCII SP, space (32)>
	HT             = <US-ASCII HT, horizontal-tab (9)>
	<">            = <US-ASCII double-quote mark (34)>

	CRLF           = CR LF
	LWS            = [CRLF] 1*( SP | HT )

	TEXT           = <any OCTET except CTLs, but including LWS>

	---

	request        = request-line
	                  *(message-header CRLF)
	                  CRLF
	                  [ message-body ]

	request-line   = Method SP Request-URI SP HTTP-Version CRLF

	---

	message-header = field-name ":" [ field-value ]
	field-name     = token
	field-value    = *( field-content | LWS )
	field-content  = <the OCTETs making up the field-value and consisting of either 
	                  *TEXT or combinations of token, separators, and quoted-string>

 */

//
static bool parse_headers(const ch_range & _blob, ch_range & request, http_hdr_vec & headers)
{
	ch_range  blob = _blob;
	ch_range_vec  lines;

	blob.trim_r();
	blob.tokenize("\r\n", lines, false); // false = don't coalesce 2+ "\r\n"

	if (lines.size() < 1)
	{
		trace_e("Header count < 1\n");
		return false;
	}

	request = lines[0];
	headers.clear();

	for (size_t i=1, n=lines.size(); i<n; i++)
	{
		auto & h = lines[i];
		http_hdr hdr;

		/*
		 *	"Header field values can be folded onto 
		 *	 multiple lines if the continuation line
		 *	 begins with a space or horizontal tab."
		 *
		 *	We don't support these.
		 */
		if (h.starts_with(" ") || h.starts_with("\t"))
		{
			trace_e("Multi-line header\n");
			return false;
		}

		hdr.name = h;
		if (! hdr.name.split_with(":", hdr.value))
		{
			trace_w("Invalid header [%.*s], skipped\n", __str(h));
			return false;
		}

		hdr.name.trim();
		hdr.value.trim();

		headers.push_back(hdr);
	}

	return true;
}

static bool parse_req_line(const ch_range & str, http_req & v)
{
	ch_range_vec  parts;
	int i = 0;

	str.tokenize(" ", parts, true); // true = treat multiple spaces as one

	if (parts.size() != 3)
		return false;

	v.verb  = parts[0];
	v.uri   = parts[1];
	v.proto = parts[2];

	return true;
}

static void parse_uri(const ch_range & uri, uri_info & v)
{
	v = uri_info();
	v.path = uri;

	if (! v.path.split_with("?", v.query))
		return;
		
	v.query.split_with("#", v.frag);
}

static void parse_query(const ch_range & uri_query, qkv_pair_vec & args)
{
	args.clear();

	if (uri_query.empty())
		return;

	ch_range_vec kvs;
	qkv_pair qkv;

	uri_query.tokenize("&", kvs, true);

	for (auto & kv : kvs)
	{
		qkv.k = kv;
		qkv.k.split_with("=", qkv.v);
		args.push_back(qkv);
	}
}

//
int parse_http_request(sk_conn & conn, http_req & req)
{
	ch_range  blob;
	ch_range  req_line;
	uri_info  uri;
	size_t    eoh;

	eoh = conn.buf.find("\r\n\r\n");
	if (eoh == -1)
		return 0; // not yet

	trace_v("Header break @ %d\n", eoh);

	blob = ch_range( &conn.buf[0], eoh );
	conn.pos = eoh + 4;

	//
	if (! parse_headers(blob, req_line, req.headers))
	{
		trace_e("Unparsable headers\n");
		return -1;
	}

	trace_v("req_line [%.*s]\n", __str(req_line));
	for (auto & h : req.headers)
		trace_v("header   [%-30.*s] [%.*s]\n", __str(h.name), __str(h.value));

	if (req.headers.size() < 1)
	{
		trace_e("Header count < 1\n");
		return -1;
	}

	if (! parse_req_line(req_line, req))
	{
		trace_e("Invalid request line\n");
		return -1;
	}

	trace_i("Parsed as [%.*s] [%.*s] [%.*s]\n", __str(req.verb), __str(req.uri), __str(req.proto));

	parse_uri(req.uri, uri);
	req.path = uri.path;

	trace_v("URI = [%.*s] [%.*s] [%.*s]\n", __str(uri.path), __str(uri.query), __str(uri.frag));

	parse_query(uri.query, req.args);

	for (auto & x : req.args)
		trace_v("Arg [%.*s] = [%.*s]\n", __str(x.k), __str(x.v));

	return +1;
}

/*
 *
 */
static inline
bool from_hex(char ch, uint8_t & val)
{
	if ('0' <= ch && ch <= '9') { val = ch - '0';      return true; }
	if ('A' <= ch && ch <= 'F') { val = ch - 'A' + 10; return true; }
	if ('a' <= ch && ch <= 'f') { val = ch - 'a' + 10; return true; }
	return false;
}

void percent_decode(string & str)
{
	char * src = &str[0];
	char * dst = &str[0];
	char * end = src + str.size();

	while (src < end)
	{
		if (*src == '%' && src+3 <= end)
		{
			uint8_t hi, lo;
			if (from_hex(src[1], hi) && from_hex(src[2], lo))
			{
				src += 3;
				*dst++ = (hi << 4) | lo;
				continue;
			}
		}

		if (src != dst)
			*dst = *src;
		dst++;
		src++;
	}

	if (dst != end)
		str.resize(dst - &str[0]);
}

