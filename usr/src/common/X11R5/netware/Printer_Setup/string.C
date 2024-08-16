/*
 * Copyright (c) 1987, 1988, 1989, 1990, 1991 Stanford University
 * Copyright (c) 1991 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Stanford and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Stanford and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 *
 * IN NO EVENT SHALL STANFORD OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

#include <X11/Xlib.h>
#include "uw_string.h"
#include <ctype.h>
#include <string.h>

/*
 * Just to be sure ...
 */

extern "C" {
#ifndef tolower
    extern int tolower(int);
#endif
#ifndef toupper
    extern int toupper(int);
#endif
    extern long int strtol(const char*, char**, int);
    extern double strtod(const char*, char**);
}

UString::UString() {
    data_ = NULL;
    length_ = 0;
}

UString::UString(const char* s) {
    data_ = s;
    length_ = strlen(s);
}

UString::UString(const char* s, int n) {
    data_ = s;
    length_ = n;
}

UString::UString(const UString& s) {
    data_ = s.data_;
    length_ = s.length_;
}

UString::~UString() { }

unsigned long UString::hash() const {
    const char* p;
    unsigned long v = 0;
    if (length_ == -1) {
	for (p = data_; *p != '\0'; p++) {
	    v = (v << 1) ^ (*p);
	}
	UString* s = (UString*)this;
	s->length_ = p - data_;
    } else {
	const char* q = &data_[length_];
	for (p = data_; p < q; p++) {
	    v = (v << 1) ^ (*p);
	}
    }
    unsigned long t = v >> 10;
    t ^= (t >> 10);
    return v ^ t;
}

UString& UString::operator =(const UString& s) {
    data_ = s.data_;
    length_ = s.length_;
    return *this;
}

UString& UString::operator =(const char* s) {
    data_ = s;
    length_ = strlen(s);
    return *this;
}

Uboolean UString::operator ==(const UString& s) const {
    return length_ == s.length_ && strncmp(data_, s.data_, length_) == 0;
}

Uboolean UString::operator ==(const char* s) const {
    return strncmp(data_, s, length_) == 0;
}

Uboolean UString::operator !=(const UString& s) const {
    return length_ != s.length_ || strncmp(data_, s.data_, length_) != 0;
}

Uboolean UString::operator !=(const char* s) const {
    return strncmp(data_, s, length_) != 0;
}

Uboolean UString::operator >(const UString& s) const {
    return strncmp(data_, s.data_, length_) > 0;
}

Uboolean UString::operator >(const char* s) const {
    return strncmp(data_, s, length_) > 0;
}

Uboolean UString::operator >=(const UString& s) const {
    return strncmp(data_, s.data_, length_) >= 0;
}

Uboolean UString::operator >=(const char* s) const {
    return strncmp(data_, s, length_) >= 0;
}

Uboolean UString::operator <(const UString& s) const {
    return strncmp(data_, s.data_, length_) < 0;
}

Uboolean UString::operator <(const char* s) const {
    return strncmp(data_, s, length_) < 0;
}

Uboolean UString::operator <=(const UString& s) const {
    return strncmp(data_, s.data_, length_) <= 0;
}

Uboolean UString::operator <=(const char* s) const {
    return strncmp(data_, s, length_) <= 0;
}

Uboolean UString::case_insensitive_equal(const UString& s) const {
    if (length() != s.length()) {
	return False;
    }
    const char* p = string();
    const char* p2 = s.string();
    const char* q = p + length();
    for (; p < q; p++, p2++) {
	int c1 = *p;
	int c2 = *p2;
	if (c1 != c2 && tolower(c1) != tolower(c2)) {
	    return False;
	}
    }
    return True;
}

Uboolean UString::case_insensitive_equal(const char* s) const {
    return case_insensitive_equal(UString(s));
}

/*
 * A negative value for start initializes the position at the end
 * of the string before indexing.  Any negative length makes
 * the substring extend to the end of the string.
 */

UString UString::substr(int start, int length) const {
    if (start >= length_ || start < -length_) {
	/* should raise exception */
	return *this;
    }
    int pos = (start >= 0) ? start : (length_ + start);
    if (pos + length > length_) {
	/* should raise exception */
	return *this;
    }
    int len = (length >= 0) ? length : (length_ - pos);
    return UString(data_ + pos, len);
}

void UString::set_to_substr(int start, int length) {
    if (start > length_ || start < -length_) {
	/* should raise exception */
	return;
    }
    int pos = (start >= 0) ? start : (length_ + start);
    if (pos + length > length_) {
	/* should raise exception */
	return;
    }
    int len = (length >= 0) ? length : (length_ - pos);
    data_ += pos;
    length_ = len;
}

Uboolean UString::null_terminated() const { return False; }

void UString::set_value(const char* s) {
    data_ = s;
    length_ = strlen(s);
}

void UString::set_value(const char* s, int len) {
    data_ = s;
    length_ = len;
}

/*
 * A negative value for start initializes the position to the end
 * of the string before indexing and searches right-to-left.
 */

int UString::search(int start, u_char c) const {
    if (start >= length_ || start < -length_) {
	/* should raise exception */
	return -1;
    }
    if (start >= 0) {
	const char* end = data_ + length_;
	for (const char* p = data_ + start; p < end; p++) {
	    if (*p == c) {
		return p - data_;
	    }
	}
    } else {
	for (const char* p = data_ + length_ + start; p >= data_; p--) {
	    if (*p == c) {
		return p - data_;
	    }
	}
    }
    return -1;
}

/*
 * Convert a string to binary value.
 */

Uboolean UString::convert(int& value) const {
    NullTerminatedUString s(*this);
    const char* str = s.string();
    char* ptr;
    value = (int)strtol(str, &ptr, 0);
    return ptr != str;
}

Uboolean UString::convert(long& value) const {
    NullTerminatedUString s(*this);
    const char* str = s.string();
    char* ptr;
    value = strtol(str, &ptr, 0);
    return ptr != str;
}

Uboolean UString::convert(float& value) const {
    NullTerminatedUString s(*this);
    const char* str = s.string();
    char* ptr;
    value = (float)strtod(str, &ptr);
    return ptr != str;
}

Uboolean UString::convert(double& value) const {
    NullTerminatedUString s(*this);
    const char* str = s.string();
    char* ptr;
    value = strtod(str, &ptr);
    return ptr != str;
}

/* class CopyUString */

CopyUString::CopyUString() : UString() { }

CopyUString::CopyUString(const char* s) : UString() {
    set_value(s);
}

CopyUString::CopyUString(const char* s, int length) : UString() {
    set_value(s, length);
}

CopyUString::CopyUString(const UString& s) : UString() {
    set_value(s.string(), s.length());
}

CopyUString::CopyUString(const CopyUString& s) : UString() {
    set_value(s.string(), s.length());
}

CopyUString::~CopyUString() {
    free();
}

UString& CopyUString::operator =(const UString& s) {
    free();
    set_value(s.string(), s.length());
    return *this;
}

UString& CopyUString::operator =(const char* s) {
    free();
    set_value(s);
    return *this;
}

Uboolean CopyUString::null_terminated() const { return True; }

void CopyUString::set_value(const char* s) {
    set_value(s, strlen(s));
}

/*
 * Guarantee null-terminated string for compatibility with printf et al.
 */

void CopyUString::set_value(const char* s, int len) {
    char* ns = new char[len + 1];
    ns[len] = '\0';
    UString::set_value(strncpy(ns, s, len), len);
}

void CopyUString::free() {
    char* s = (char*)(string());
    delete s;
}

/*
 * class NullTerminatedUString
 */

NullTerminatedUString::NullTerminatedUString() : UString() {
    allocated_ = False;
}

NullTerminatedUString::NullTerminatedUString(const UString& s) : UString() {
    assign(s);
}

NullTerminatedUString::NullTerminatedUString(
    const NullTerminatedUString& s
) : UString() {
    allocated_ = False;
    UString::set_value(s.string(), s.length());
}

NullTerminatedUString::~NullTerminatedUString() {
    free();
}

UString& NullTerminatedUString::operator =(const UString& s) {
    free();
    assign(s);
    return *this;
}

UString& NullTerminatedUString::operator =(const char* s) {
    free();
    allocated_ = False;
    UString::set_value(s, strlen(s));
    return *this;
}

Uboolean NullTerminatedUString::null_terminated() const { return True; }

void NullTerminatedUString::assign(const UString& s) {
    if (s.null_terminated()) {
	allocated_ = False;
	UString::set_value(s.string(), s.length());
    } else {
	allocated_ = True;
	int len = s.length();
	char* ns = new char[len + 1];
	ns[len] = '\0';
	UString::set_value(strncpy(ns, s.string(), len), len);
    }
}

void NullTerminatedUString::free() {
    if (allocated_) {
	char* s = (char*)(string());
	delete s;
	allocated_ = False;
    }
}
