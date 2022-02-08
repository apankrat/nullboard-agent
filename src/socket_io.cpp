/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#include "socket_io.h"
#include "trace.h"

//
void sk_conn::replenish_buf()
{
	if (fill == buf.size())
		buf.resize( 2*buf.size() );
}

void sk_conn::clear()
{
	shutdown(sk, SD_SEND);
	closesocket(sk);

	sk = -1;
	buf.clear();
	fill = 0;
	pos = 0;
}


//
int sk_wait(SOCKET sk, int what, int timeout_sec)
{
	fd_set   fdx;
	timeval  tv = { timeout_sec, 0 };
	int      rc;

	FD_ZERO(&fdx);
	FD_SET(sk, &fdx);

	rc = select(1, (what & 0x1) ? &fdx : NULL, (what & 0x2) ? &fdx : NULL, NULL, &tv);
	if (rc < 0)
	{
		wsa_error("select");
		return -1;
	}

	return rc;
}

//
int sk_recv(sk_conn & conn)
{
	__enforce(conn.sk != -1);
	__enforce(conn.fill < conn.buf.size());

	size_t cap;
	int rc;

	cap = conn.buf.size() - conn.fill;
	rc = recv(conn.sk, &conn.buf[conn.fill], (int)cap, 0);

	if (rc > 0)
	{
		trace_v("sk_recv() -> read %d bytes\n", rc);

		__enforce(rc <= (int)cap);
		conn.fill += rc;
	}
	else
	if (rc == 0)
	{
		trace_v("sk_recv() -> eof\n", rc);
	}
	else
	if (sk_recv_fatal())
	{
		wsa_error("recv");
		rc = -1; // error
	}
	else
	{
		rc = -2; // no data
	}

	return rc;
}

int sk_recv(sk_conn & conn, int timeout_sec)
{
	int rc;

	rc = sk_recv(conn);
	if (rc != -2)
		return rc;

	rc = sk_wait(conn.sk, 0x01, timeout_sec); // 0x01 = readable
	if (rc < 0)
		return -1; // failed

	if (rc == 0)
	{
		trace_v("sk_recv() timed out\n");
		return -2; // timed out
	}

	rc = sk_recv(conn);
	if (rc == -2)
	{
		trace_v("sk_recv() timed out\n");
		return -2;
	}

	return rc; // got data, got eof or failed
}

//
int sk_send(sk_conn & conn, const ch_range & _buf, int timeout_sec)
{
	ch_range buf = _buf;
	bool post_wait = false;
	int rc;

	__enforce(buf.size);

	while (buf.size)
	{
		rc = send(conn.sk, buf.data, (int)buf.size, 0);
		if (rc > 0)
		{
			trace_v("sk_send() -> sent %d bytes, out of %d\n", rc, buf.size);

			__enforce( rc <= (int)buf.size );
			buf.advance_by(rc);
			post_wait = false;
			continue;
		}

		if (sk_send_fatal())
		{
			wsa_error("send");
			return -1;
		}

		if (post_wait)
		{
			trace_v("sk_send() timed out\n");
			return -1;
		}

		rc = sk_wait(conn.sk, 0x02, timeout_sec);
		if (rc < 0)
			return -1; // failed

		if (rc == 0)
		{
			trace_v("sk_send() timed out\n");
			return -2; // timed out
		}

		post_wait = true;
	}

	return (int)_buf.size; // the original buf size
}
