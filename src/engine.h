/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#ifndef _ENGINE_H_
#define _ENGINE_H_

#include "types.h"

bool   init_engine();
HANDLE start_engine();
void   stop_engine();

void on_engine_activity(); // a callback for the ui

#endif
