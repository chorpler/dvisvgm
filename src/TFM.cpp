/*************************************************************************
** TFM.cpp                                                              **
**                                                                      **
** This file is part of dvisvgm -- the DVI to SVG converter             **
** Copyright (C) 2005-2009 Martin Gieseking <martin.gieseking@uos.de>   **
**                                                                      **
** This program is free software; you can redistribute it and/or        **
** modify it under the terms of the GNU General Public License as       **
** published by the Free Software Foundation; either version 3 of       **
** the License, or (at your option) any later version.                  **
**                                                                      **
** This program is distributed in the hope that it will be useful, but  **
** WITHOUT ANY WARRANTY; without even the implied warranty of           **
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the         **
** GNU General Public License for more details.                         **
**                                                                      **
** You should have received a copy of the GNU General Public License    **
** along with this program; if not, see <http://www.gnu.org/licenses/>. **
*************************************************************************/

#include <iostream>
#include <fstream>
#include <vector>
#include "FileFinder.h"
#include "Message.h"
#include "MetafontWrapper.h"
#include "TFM.h"

using namespace std;


/** Reads a single unsigned integer value of given size (max. 4 bytes).
 *  @param[in] is characters are read from this stream
 *  @param[in] n number of bytes to be read */
static UInt32 read_unsigned (istream &is, int n) {
	UInt32 ret = 0;
	for (int i=n-1; i >= 0 && !is.eof(); i--) {
		UInt32 b = is.get();
		ret |= b << (8*i);
	}
	return ret;
}


/** Reads a sequence of n TFM words (4 Bytes each).
 *  @param[in]  is reads from this stream
 *  @param[out] v the read words
 *  @param[in]  n number of words to be read
 *  @return dynamically allocated array containing the read words
 *          (must be deleted by the caller) */
template <typename T>
static void read_words (istream &is, vector<T> &v, unsigned n) {
	v.clear();
	v.resize(n);
	for (unsigned i=0; i < n; i++)
		v[i] = read_unsigned(is, 4);
}


/** Converts a TFM fix point value to double. */
static double fix2double (FixWord fix) {
	return double(fix)/(1 << 20);
}


TFM::TFM () : _checksum(0), _firstChar(0), _lastChar(0), _designSize(0)
{
}


TFM::TFM (istream &is) {
	readFromStream(is);
}


TFM* TFM::createFromFile (const char *fontname) {
	string filename = string(fontname) + ".tfm";
	const char *path = FileFinder::lookup(filename);
	ifstream ifs(path, ios_base::binary);
	if (ifs)
		return new TFM(ifs);
	return 0;
}


bool TFM::readFromStream (istream &is) {
	is.seekg(2, ios_base::beg);        // skip file size
	UInt16 lh = read_unsigned(is, 2);  // length of header in 4 byte words
	_firstChar= read_unsigned(is, 2);  // smalles character code in font
	_lastChar = read_unsigned(is, 2);  // largest character code in font
	UInt16 nw = read_unsigned(is, 2);  // number of words in width table
	UInt16 nh = read_unsigned(is, 2);  // number of words in height table
	UInt16 nd = read_unsigned(is, 2);  // number of words in depth table
	UInt16 ni = read_unsigned(is, 2);  // number of words in italic corr. table
//	UInt16 nl = read_unsigned(is, 2);  // number of words in lig/kern table
//	UInt16 nk = read_unsigned(is, 2);  // number of words in kern table
//	UInt16 ne = read_unsigned(is, 2);  // number of words in ext. char table
//	UInt16 np = read_unsigned(is, 2);  // number of font parameter words

	is.seekg(8, ios_base::cur);        // move to header (skip above commented bytes)
	_checksum = read_unsigned(is, 4);
	_designSize = read_unsigned(is, 4);
	is.seekg(24+lh*4, ios_base::beg);  // move to char info table
	read_words(is, _charInfoTable, _lastChar-_firstChar+1);
	read_words(is, _widthTable, nw);
	read_words(is, _heightTable, nh);
	read_words(is, _depthTable, nd);
	read_words(is, _italicTable, ni);
	return true;
}


/** Returns the design size of this font in TeX point units. */
double TFM::getDesignSize () const {
	return fix2double(_designSize);
}


// the char info word for each character consists of 4 bytes holding the following information:
// width index w, height index (h), depth index (d), italic correction index (it),
// tag (tg) and a remainder:
//
// byte 1   | byte 2    | byte 3    | byte 4
// xxxxxxxx | xxxx xxxx | xxxxxx xx | xxxxxxxx
// w        | h    d    | it     tg | remainder

/** Returns the width of char c in TeX point units. */
double TFM::getCharWidth (int c) const {
	if (c < _firstChar || c > _lastChar || unsigned(c-_firstChar) >= _charInfoTable.size())
		return 0;
	int index = (_charInfoTable[c-_firstChar] >> 24) & 0xFF;
	return fix2double(_widthTable[index]) * fix2double(_designSize);
}


/** Returns the height of char c in TeX point units. */
double TFM::getCharHeight (int c) const {
	if (c < _firstChar || c > _lastChar || unsigned(c-_firstChar) >= _charInfoTable.size())
		return 0;
	int index = (_charInfoTable[c-_firstChar] >> 20) & 0x0F;
	return fix2double(_heightTable[index]) * fix2double(_designSize);
}


/** Returns the depth of char c in TeX point units. */
double TFM::getCharDepth (int c) const {
	if (c < _firstChar || c > _lastChar || unsigned(c-_firstChar) >= _charInfoTable.size())
		return 0;
	int index = (_charInfoTable[c-_firstChar] >> 16) & 0x0F;
	return fix2double(_depthTable[index]) * fix2double(_designSize);
}


/** Returns the italic correction of char c in TeX point units. */
double TFM::getItalicCorr (int c) const {
	if (c < _firstChar || c > _lastChar || unsigned(c-_firstChar) >= _charInfoTable.size())
		return 0;
	int index = (_charInfoTable[c-_firstChar] >> 10) & 0x3F;
	return fix2double(_italicTable[index]) * fix2double(_designSize);
}

