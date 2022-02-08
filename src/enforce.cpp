/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#include "enforce.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

//
static void on_enforce(const char * exp, const char * file, const char * func, int line)
{
	printf("assert(%s) failed, line %d, file %s\n", exp, line, file);
	abort();
}

void (* enforce_handler)(const char *, const char *, const char *, int) = on_enforce;
