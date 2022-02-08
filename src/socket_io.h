/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#ifndef _SOCKET_IO_H_
#define _SOCKET_IO_H_

#include "types.h"
#include "ch_range.h"
#include "trace.h"

//
struct sk_conn
{
	SOCKET  sk;
	string  buf;
	size_t  fill; // of the buf
	size_t  pos;  // in the buf

	sk_conn() { sk = -1; fill = 0; pos = 0; }
	void replenish_buf();
	void clear();
};

//
int sk_errno();

bool sk_recv_fatal();
bool sk_send_fatal();

bool sk_reuseaddr(SOCKET sk);
bool sk_unblock(SOCKET sk);

int sk_recv(sk_conn & conn);
int sk_recv(sk_conn & conn, int timeout_sec);

int sk_send(sk_conn & conn, const ch_range & buf, int timeout_sec = 1);

/*
 *	inlines
 */
inline int sk_errno()
{
	return WSAGetLastError();
}

inline bool sk_accept_fatal()
{
	int e = sk_errno();
	return (e != WSAECONNRESET);
}

inline bool sk_recv_fatal()
{
	int e = sk_errno();
	return (e != WSAEWOULDBLOCK) && (e != WSA_IO_PENDING);
}

inline bool sk_send_fatal()
{
	int e = sk_errno();
	return (e != WSAEWOULDBLOCK) && (e != WSA_IO_PENDING);
}

inline bool sk_reuseaddr(SOCKET sk)
{
	static const int yes = 1;
	if (setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof yes) < 0)
		return wsa_error("reuseaddr");

	return true;
}

inline bool sk_unblock(SOCKET sk)
{
	static u_long yes = 1;
	if (ioctlsocket(sk, FIONBIO, &yes) < 0)
		return wsa_error("sk_unblock");
	return true;
}

#endif
