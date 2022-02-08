/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#ifndef _TRACE_H_
#define _TRACE_H_

#include "types.h"

void trace_e(const char * format, ...); // 0 errors
void trace_w(const char * format, ...); // 1 warning
void trace_i(const char * format, ...); // 2 info
void trace_v(const char * format, ...); // 3 verbose
void trace_d(const char * format, ...); // 4 debug

bool api_error(const char * func, const char * format = NULL, ...);
bool wsa_error(const char * func); // trace_e( "func() failed with {WSAGetLastError}" ); return false; 

#endif
