/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#ifndef _CH_RANGE_H_
#define _CH_RANGE_H_

#include "types.h"
#include "enforce.h"

struct ch_range
{
	char  * data;
	size_t  size;

	ch_range()                           { size = 0;             data = NULL;                   }
	ch_range(char * _data)               { size = strlen(_data); data = _data;                  }
	ch_range(char * _data, size_t _size) { size = _size;         data = _data;                  }
	ch_range(string & _str)              { size = _str.size();   data = size ? &_str[0] : NULL; }

	//
	bool empty() const;
	bool match(const ch_range & what) const;

	bool is_decimal() const;

	char * find(char what, size_t offset = 0) const;
	char * find(const ch_range & what, size_t offset = 0) const;

	char * find_first_of(const ch_range & what, size_t offset = 0) const;
	char * find_first_not_of(const ch_range & what, size_t offset = 0) const;

	bool starts_with(const ch_range & what) const;

	//
	bool trim_l(const ch_range & what = __whitespace);
	bool trim_r(const ch_range & what = __whitespace);
	bool trim  (const ch_range & what = __whitespace); // trim_l(), trim_r(), empty()

	bool split_with(const ch_range & sep, ch_range & tail);
	void advance_to(const char * to);
	void advance_by(size_t delta);

	bool get_line(ch_range & line);

	//
	void tokenize(const ch_range & sep, vector<ch_range> & out, bool relaxed) const;
	bool split(const ch_range & sep, ch_range & left, ch_range & right) const;
	bool split_alt(const ch_range & sep_set, ch_range & left, ch_range & right) const;

	//
	int scanf(const char * format, ...) const;
	int vscanf(const char * format, va_list args) const;

	//
	string to_str() const;
	wstring to_wstr() const;

	//
	static ch_range __whitespace;
};

typedef vector<ch_range> ch_range_vec;

//
#define __str(x)   (x).size, (x).data

#endif
