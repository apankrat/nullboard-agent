/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#ifndef _LIBP_ENFORCE_H_
#define _LIBP_ENFORCE_H_

/*
 *	run-time asserts
 */
#define __enforce(exp) (void)( !!(exp) || (enforce_handler(#exp, __FILE__, __FUNCTION__, __LINE__), 0) )

/*
 *	default handler - logs the event and calls abort()
 */
extern void (* enforce_handler)(const char * exp, const char * file, const char * func, int line);

#endif
