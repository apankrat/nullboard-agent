/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <map>

using std::string;
using std::wstring;
using std::vector;
using std::map;

typedef unsigned int uint_t;

#ifdef _DEBUG
#  define __D
#else
#  define __D          / ## /
#endif

#include <winsock2.h>
#include <windows.h>

typedef DWORD dword;

#include "enforce.h"

#endif
