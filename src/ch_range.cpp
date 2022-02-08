/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#include "ch_range.h"
#include "utils.h"

/*
 *
 */
ch_range ch_range::__whitespace("\r\n \t", 4);

/*
 *
 */
bool ch_range::empty() const
{
	return !size;
}

bool ch_range::match(const ch_range & what) const
{
	return size == what.size &&
		_strnicmp(data, what.data, size) == 0;
}

bool ch_range::is_decimal() const
{
	if (! size)
		return false;

	for (size_t i=0; i<size; i++)
		if (data[i] < '0' || '9' < data[i])
			return false;

	return true;
}

char * ch_range::find(char what, size_t offset) const
{
	if (offset >= size)
		return NULL;

	what = tolower(what);

	auto upto = data + size;

	for (auto p = data + offset; p < upto; p++)
		if (tolower(*p) == what)
			return p;

	return NULL;
}

char * ch_range::find(const ch_range & what, size_t offset) const
{
	if (! what.size || offset + what.size > size)
		return NULL;

	auto upto = data + size - what.size + 1;

	for (auto p = data + offset; p < upto; p++)
	{
		if (*p != *what.data)
			continue;

		if (! _strnicmp(p, what.data, what.size))
			return p;
	}

	return NULL;
}

char * ch_range::find_first_of(const ch_range & what, size_t offset) const
{
	if (! what.size)
		return NULL;

	for ( ; offset < size; offset++)
		if (what.find(data[offset]))
			return data + offset;

	return NULL;
}

char * ch_range::find_first_not_of(const ch_range & what, size_t offset) const
{
	if (! what.size)
		return NULL;

	for ( ; offset < size; offset++)
		if (! what.find(data[offset]))
			return data + offset;

	return NULL;
}

bool ch_range::starts_with(const ch_range & what) const
{
	return what.size &&
	       what.size <= size &&
	       ! _strnicmp(data, what.data, what.size);
}

//
bool ch_range::trim_l(const ch_range & what)
{
	while (size && what.find(data[0])) { data++; size--; }
	return empty();
}

bool ch_range::trim_r(const ch_range & what)
{
	while (size && what.find(data[size-1])) size--;
	return empty();
}

bool ch_range::trim(const ch_range & what)
{
	return trim_l() ? true : trim_r();
}

bool ch_range::split_with(const ch_range & sep, ch_range & tail)
{
	auto p = find(sep);
	if (! p)
	{
		tail = ch_range();
		return false;
	}

	auto end = data + size;

	tail.data = p + sep.size;
	tail.size = end - tail.data;

	size = p - data;
	return true;
}

void ch_range::advance_to(const char * to)
{
	__enforce(data <= to && to <= data + size);

	size = data + size - to;
	data = (char*)to;
}

void ch_range::advance_by(size_t delta)
{
	__enforce(delta <= size);

	data += delta;
	size -= delta;
}

bool ch_range::get_line(ch_range & line)
{
	if (! size)
		return false;

	char * lf = find('\n');

	if (! lf)
	{
		line = *this;
		advance_by(size);
		return true;
	}

	line.data = data;
	line.size = lf - data;

	if (line.size && line.data[line.size-1] == '\r')
		line.size--;

	advance_to(lf+1);
	return true;
}

//
void ch_range::tokenize(const ch_range & sep, ch_range_vec & out, bool relaxed) const
{
	ch_range in = *this;
	ch_range tok;
	auto end = in.data + in.size;

	out.clear();

	for (;;)
	{
		if (in.empty() && relaxed)
			break;

		auto eot = in.find(sep);

		if (! eot)
		{
			out.push_back(in);
			break;
		}

		tok.data = in.data;
		tok.size = eot - in.data;

		if (tok.size || ! relaxed)
			out.push_back(tok);

		in.data = eot + sep.size;
		in.size = end - in.data;
	}
}

bool ch_range::split(const ch_range & sep, ch_range & l, ch_range & r) const
{
	l = *this;
	r = ch_range();

	auto p = find(sep);

	if (! p)
		return false;

	l.data = data;
	l.size = p - l.data;

	r.data = p + sep.size;
	r.size = data + size - r.data;

	return true;
}

bool ch_range::split_alt(const ch_range & sep_set, ch_range & l, ch_range & r) const
{
	l = *this;
	r = ch_range();

	auto p = find_first_of(sep_set);

	if (! p)
		return false;

	l.data = data;
	l.size = p - data;

	auto q = find_first_not_of(sep_set, p - data + 1);

	if (! q)
		return false;

	r.data = q;
	r.size = data + size - q;

	return true;
}

//
int ch_range::scanf(const char * format, ...) const
{
	va_list m;
	int r;

	va_start(m, format);
	r = vscanf(format, m);
	va_end(m);

	return r;
}

int ch_range::vscanf(const char * format, va_list args) const
{
	return __stdio_common_vsscanf(
		_CRT_INTERNAL_LOCAL_SCANF_OPTIONS,
		data, size, format, NULL, args);
}

//
string ch_range::to_str() const
{
	return size ? string(data, size) : string();
}

//
wstring ch_range::to_wstr() const
{
	return ::to_wstr(data, size);
}
