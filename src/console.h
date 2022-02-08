/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "types.h"

bool open_console();
void show_console(bool show = true);

#endif