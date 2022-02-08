/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#include "trace.h"
#include "config.h"

void vtracef(uint_t level, const char * prefix, const char * format, va_list args, const char * suffix = NULL)
{
	if (level > conf.trace)
		return;

	if (prefix) printf("%s", prefix);
	
	vprintf(format, args);

	if (suffix) printf("%s", suffix);
}

void trace_e(const char * format, ...)
{
	va_list m;
	va_start(m, format);
	vtracef(0, "e: ", format, m);
	va_end(m);
}

void trace_w(const char * format, ...)
{
	va_list m;
	va_start(m, format);
	vtracef(1, "w: ", format, m);
	va_end(m);
}

void trace_i(const char * format, ...)
{
	va_list m;
	va_start(m, format);
	vtracef(2, "i: ", format, m);
	va_end(m);
}

void trace_v(const char * format, ...)
{
	va_list m;
	va_start(m, format);
	vtracef(3, "v: ", format, m);
	va_end(m);
}

void trace_d(const char * format, ...)
{
	va_list m;
	va_start(m, format);
	vtracef(4, "d: ", format, m);
	va_end(m);
}

//
bool api_error(const char * func, const char * format, ...)
{
	trace_e("%s() failed with %lu\n", func, GetLastError());

	if (format)
	{
		va_list m;
		va_start(m, format);
		vtracef(0, "e: * ", format, m, "\n");
		va_end(m);
	}

	return false;
}

bool wsa_error(const char * func)
{
	trace_e("%s() failed with %lu\n", func, WSAGetLastError());
	return false;
}
