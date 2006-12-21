/*
 * FBReader -- electronic book reader
 * Copyright (C) 2004-2006 Nikolay Pultsin <geometer@mawhrin.net>
 * Copyright (C) 2005 Mikhail Sobolev <mss@mawhrin.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <algorithm>
#include <cctype>

#include <ZLInputStream.h>
#include <ZLXMLReader.h>
#include <ZLFile.h>
#include <ZLStringUtil.h>
#include <ZLUnicodeUtil.h>

#include "HtmlReader.h"
#include "HtmlEntityCollection.h"

std::string HtmlReader::decodeURL(const std::string &encoded) {
	std::string decoded;
	const int len = encoded.length();
	decoded.reserve(len);
	for (int i = 0; i < len; i++) {
		if ((encoded[i] == '%') && (i < len - 2)) {
			const char *end = encoded.data() + i + 3;
			char **endp = const_cast<char**>(&end);
			decoded += (char)strtol(encoded.data() + i + 1, endp, 16);
			i += 2;
		} else {
			decoded += encoded[i];
		}
	}
	return decoded;
}

HtmlReader::HtmlReader(const std::string &encoding) {
	myConverter = ZLEncodingConverter::createConverter(encoding);
}

HtmlReader::~HtmlReader() {
}

void HtmlReader::setTag(HtmlTag &tag, const std::string &name) {
	tag.Attributes.clear();

	if (name.length() == 0) {
		tag.Name = name;
		return;
	}

	tag.Start = name[0] != '/';
	if (tag.Start) {
		tag.Name = name;
	} else {
		tag.Name = name.substr(1);
	}

	const size_t len = std::min(tag.Name.length(), (size_t)10);
	for (size_t i = 0; i < len; ++i) {
		tag.Name[i] = toupper(tag.Name[i]);
	}
}

enum ParseState {
	PS_TEXT,
	PS_TAGSTART,
	PS_TAGNAME,
	PS_ATTRIBUTENAME,
	PS_ATTRIBUTEVALUE,
	PS_SKIPTAG,
	PS_COMMENT,
	PS_SPECIAL,
	PS_SPECIAL_IN_ATTRIBUTEVALUE,
};

enum SpecialType {
	ST_UNKNOWN,
	ST_NUM,
	ST_NAME,
	ST_DEC,
	ST_HEX
};

static bool allowSymbol(SpecialType type, char ch) {
	return
		((type == ST_NAME) && isalpha(ch)) ||
		((type == ST_DEC) && isdigit(ch)) ||
		((type == ST_HEX) && isxdigit(ch));
}

static int specialSymbolNumber(SpecialType type, const std::string &txt) {
	char *end = 0;
	switch (type) {
		case ST_NAME:
			return HtmlEntityCollection::symbolNumber(txt);
		case ST_DEC:
			return strtol(txt.c_str() + 1, &end, 10);
		case ST_HEX:
			return strtol(txt.c_str() + 2, &end, 16);
		default:
			return 0;
	}
}

void HtmlReader::readDocument(ZLInputStream &stream) {
	if (!stream.open()) {
		return;
	}

	startDocumentHandler();

	ParseState state = PS_TEXT;
	SpecialType state_special = ST_UNKNOWN;
	std::string currentString;
	std::string specialString;
	int quotationCounter = 0;
	HtmlTag currentTag;
	char endOfComment[2] = "\0";
	
	const size_t BUFSIZE = 2048;
	char *buffer = new char[BUFSIZE];
	size_t length;
	do {
		length = stream.read(buffer, BUFSIZE);
		char *start = buffer;
		char *endOfBuffer = buffer + length;
		for (char *ptr = buffer; ptr < endOfBuffer; ++ptr) {
			switch (state) {
				case PS_TEXT:
					if (*ptr == '<') {
						if (!characterDataHandler(start, ptr - start, true)) {
							goto endOfProcessing;
						}
						start = ptr + 1;
						state = PS_TAGSTART;
					}
					if (*ptr == '&') {
						if (!characterDataHandler(start, ptr - start, true)) {
							goto endOfProcessing;
						}
						start = ptr + 1;
						state = PS_SPECIAL;
						state_special = ST_UNKNOWN;
					}
					break;
				case PS_SPECIAL:
				case PS_SPECIAL_IN_ATTRIBUTEVALUE:
					if (state_special == ST_UNKNOWN) {
						if (*ptr == '#') {
							state_special = ST_NUM;
						} else if (isalpha(*ptr)) {
							state_special = ST_NAME;
						} else {
							start = ptr;
							state = (state == PS_SPECIAL) ? PS_TEXT : PS_ATTRIBUTEVALUE;
						}
					} else if (state_special == ST_NUM) {
						if (*ptr == 'x') {
							state_special = ST_HEX;
						} else if (isdigit(*ptr)) {
							state_special = ST_DEC;
						} else {
							start = ptr;
							state = (state == PS_SPECIAL) ? PS_TEXT : PS_ATTRIBUTEVALUE;
						}
					} else {
						if (*ptr == ';') {
							specialString.append(start, ptr - start);
							int number = specialSymbolNumber(state_special, specialString);
							if (number != 0) {
								char buffer[4];
								int len = ZLUnicodeUtil::ucs2ToUtf8(buffer, number);
								if (state == PS_SPECIAL) {
									characterDataHandler(buffer, len, false);
								} else {
									currentString.append(buffer, len);
								}
							} else {
								specialString = "&" + specialString + ";";
								if (state == PS_SPECIAL) {
									characterDataHandler(specialString.c_str(), specialString.length(), false);
								} else {
									currentString += specialString;
								}
							}
							specialString.erase();
							start = ptr + 1;
							state = (state == PS_SPECIAL) ? PS_TEXT : PS_ATTRIBUTEVALUE;
						} else if (!allowSymbol(state_special, *ptr)) {
							start = ptr;
							state = (state == PS_SPECIAL) ? PS_TEXT : PS_ATTRIBUTEVALUE;
						}
					}
					break;
				case PS_TAGSTART:
					state = (*ptr == '!') ? PS_COMMENT : PS_TAGNAME;
					break;
				case PS_COMMENT:
					if ((endOfComment[0] == '\0') && (*ptr != '-')) {
						state = PS_TAGNAME;
					} else if ((endOfComment[0] == '-') && (endOfComment[1] == '-') && (*ptr == '>')) {
						start = ptr + 1;
						state = PS_TEXT;
						endOfComment[0] = '\0';
						endOfComment[1] = '\0';
					} else {
						endOfComment[0] = endOfComment[1];
						endOfComment[1] = *ptr;
					}
					break;
				case PS_TAGNAME:
					if ((*ptr == '>') || isspace(*ptr)) {
						currentString.append(start, ptr - start);
						start = ptr + 1;
						setTag(currentTag, currentString);
						currentString.erase();
						if (currentTag.Name == "") {
							state = (*ptr == '>') ? PS_TEXT : PS_SKIPTAG;
						} else {
							if (*ptr == '>') {
								if (!tagHandler(currentTag)) {
									goto endOfProcessing;
								}
								state = PS_TEXT;
							} else {
								state = PS_ATTRIBUTENAME;
							}
						}
					}
					break;
				case PS_ATTRIBUTENAME:
					if ((*ptr == '>') || (*ptr == '=') || isspace(*ptr)) {
						if ((ptr != start) || !currentString.empty()) {
							currentString.append(start, ptr - start);
							for (unsigned int i = 0; i < currentString.length(); ++i) {
								currentString[i] = toupper(currentString[i]);
							}
							currentTag.addAttribute(currentString);
							currentString.erase();
						}
						start = ptr + 1;
						if (*ptr == '>') {
							if (!tagHandler(currentTag)) {
								goto endOfProcessing;
							}
							state = PS_TEXT;
						} else {
							state = (*ptr == '=') ? PS_ATTRIBUTEVALUE : PS_ATTRIBUTENAME;
						}
					}
					break;
				case PS_ATTRIBUTEVALUE:
					if (*ptr == '"') {
						if (((ptr == start) && currentString.empty()) || (quotationCounter > 0)) {
							++quotationCounter;
						}
					} else if (*ptr == '&') {
						currentString.append(start, ptr - start);
						start = ptr + 1;
						state = PS_SPECIAL_IN_ATTRIBUTEVALUE;
						state_special = ST_UNKNOWN;
					} else if ((quotationCounter != 1) && ((*ptr == '/') || (*ptr == '>') || isspace(*ptr))) {
						if ((ptr != start) || !currentString.empty()) {
							currentString.append(start, ptr - start);
							if (currentString[0] == '"') {
								currentString = currentString.substr(1, currentString.length() - 2);
							}
							currentTag.setLastAttributeValue(currentString);
							currentString.erase();
							quotationCounter = 0;
						}
						start = ptr + 1;
						if (*ptr == '>') {
							if (!tagHandler(currentTag)) {
								goto endOfProcessing;
							}
							state = PS_TEXT;
						} else {
							state = PS_ATTRIBUTENAME;
						}
					}
					break;
				case PS_SKIPTAG:
					if (*ptr == '>') {
						start = ptr + 1;
						state = PS_TEXT;
					}
					break;
			}
		}
		if (start != endOfBuffer) {
			switch (state) {
				case PS_TEXT:
					if (!characterDataHandler(start, endOfBuffer - start, true)) {
						goto endOfProcessing;
					}
					break;
				case PS_TAGNAME:
				case PS_ATTRIBUTENAME:
				case PS_ATTRIBUTEVALUE:
					currentString.append(start, endOfBuffer - start);
					break;
				case PS_SPECIAL:
				case PS_SPECIAL_IN_ATTRIBUTEVALUE:
					specialString.append(start, endOfBuffer - start);
					break;
				case PS_TAGSTART:
				case PS_SKIPTAG:
				case PS_COMMENT:
					break;
			}
		}
	} while (length == BUFSIZE);
endOfProcessing:
	delete[] buffer;

	endDocumentHandler();

	stream.close();
}