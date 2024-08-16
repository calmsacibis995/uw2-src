/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident       "@(#)sc:String/incl/String.h	3.13" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992, 1993 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

#ifndef ATTLCSTRINGH 
#define ATTLCSTRINGH

#include <string.h>
//#ifndef hpux
//#include <sysent.h>
//#else
//#include <sys/unistd.h>
//#endif
#include <unistd.h>
#include <stdio.h>
#include <Pool.h>
#include <memory.h>

class Srep_ATTLC;
class Stringsize;
class String;
class Tmpstring;
class Substring;
class ostream;
class istream;

class Srep_ATTLC {         // String Representation
public:
	enum { MAXS = 8, MAXL = 64 };
	static Srep_ATTLC* nullrep_; // nullrep always has refc == 0 and max == 0
	static Pool* Reelp;  // Pool for "reel" small strings
	static Pool* SPoolp; // Pool for smallish strings
	static void initialize() { if (Reelp == 0) doinitialize(); }
	static void doinitialize();
	static Srep_ATTLC* nullrep() { initialize(); return nullrep_; }
	static Srep_ATTLC *new_srep(unsigned);
	static Srep_ATTLC *get_long(unsigned);
	unsigned len;
	unsigned max;
	unsigned char refc;
	char str[MAXS];
	Srep_ATTLC() { len = 0; max = 0; refc = 0; }
	void rcdec() { if(refc && --refc==0)  delete_srep(); }
	void rcinc() { if(refc) refc++; }
	void delete_srep();
};

class Stringsize {	 // for declarations of large Strings
	unsigned int i;
	friend class String;
public:
	Stringsize(unsigned int x) { i = x; }
	~Stringsize() {}
	int size() { return i; }
};
	
class String {		//  the String
	Srep_ATTLC *d;
	friend class Tmpstring;
	friend class Substring;
	String(Srep_ATTLC* r) { d = r; }
	String& newcopy(char);
	void reserve_grow(int) const;
	void overflow() const;
public:
	String() { d = Srep_ATTLC::nullrep(); }
	String(const char*);
	String(const char*,unsigned);
	String(char c) { d = Srep_ATTLC::new_srep(1); d->str[0]=c; }
	String(Stringsize s) { d = Srep_ATTLC::new_srep(s.i); d->len=0; }
	String(const String& s) {
		s.d->rcinc();
		if (s.d->refc == 0 && s.d->max != 0) { s.overflow(); s.d->rcinc(); }
		d = s.d;
	}
	inline String(const Tmpstring&);
	inline String(const Substring&);
	friend String int_to_str(int);
	friend String long_to_str(long);
	inline operator const char*() const;
	~String() { d->rcdec(); }
	/* ASSIGNMENT */
	inline String& operator=(const String&);
	void assign(const char*, unsigned len);
	String& operator=(const char*);
	inline String& operator=(char);
	String& operator=(Stringsize);
	/* CHANGE SIZE */
	void reserve(Stringsize n) const {
		if (n.i >= d->max) reserve_grow(n.i);
	}
	void shrink(int target) {
    		if (target < d->len && target > 0) {
			if(d->refc > 1) {
				Srep_ATTLC* x = Srep_ATTLC::new_srep(d->len);
				if(d->len != 0) memcpy(x->str,d->str,d->len);
				d->rcdec();
				d = x;
			}
    			d->len = target;
		}
	}
	/* APPENDING */
	String& operator+=(const String&);
	void append(const char*, unsigned len);
	String& operator+=(const char*);
	inline String& operator+=(char);
	inline String& put(const String&);
	inline String& put(const char*);
	inline String& put(char);
	/* ADDITION */
	friend Tmpstring operator+(const String&,const String&);
	friend Tmpstring operator+(const String&,const char*);
	friend Tmpstring operator+(const String&,char);
	friend Tmpstring operator+(const char*,const String&);
	friend Tmpstring operator+(char,const String&);
	/* MISCELLANEOUS */
	void pad(int n, int pad_char = -1) {
		if((d->refc > 1) || ((n + d->len) >= d->max)) {
    			reserve_grow(n + d->len);
		}
    		if (pad_char != -1) memset(&d->str[d->len], pad_char, n);
    		d->len += n;
	}
	unsigned length() const { return d->len; } 
	int is_empty() const { return d->len ? 0 : 1; } 
	void make_empty() { assign("", 0); }
	unsigned max() const { return d->max; } 
	int nrefs() const { return d->refc; }
	const Srep_ATTLC *rep() const { return d; }
	inline void uniq() const;
	inline char char_at(unsigned) const;
	char& operator[](unsigned);
	inline char operator[](unsigned) const;
#ifdef INT_INDEXING
	char& operator[](int);
	inline char operator[](int) const;
#endif
	Substring operator()(unsigned,unsigned); 
	Substring operator()(unsigned);
	String operator()(unsigned u,unsigned w) const { return chunk(u,w); } 
	String operator()(unsigned u) const { return chunk(u); }
	String chunk(unsigned,unsigned) const; 
	String chunk(unsigned) const;
	int index(const String&,unsigned pos = 0) const;
	int index(char,unsigned pos = 0) const;
	int hashval() const;
	char* dump(char* s) const { 
		memcpy(s,d->str,d->len); 
		s[d->len]='\0'; return s;
	}
	int getX(char&);
	int get() { char c; return getX(c); }
	int unputX(char&);
	int unput() { char c; return unputX(c); }
	String& unget(char);
	String& unget(const String&);
	inline int firstX(char&) const;			
	inline int lastX(char&) const;
	/* COMPARISON OPERATORS */
	inline int compare(const String&) const;
	inline int compare(const char*) const;
	friend inline int operator==(const String&,const String&);
	friend inline int operator==(const String&,const char*);
	friend inline int operator!=(const String&,const String&);
	friend inline int operator!=(const String&,const char*);
	/* SYSTEM FUNCTIONS */
#if defined(SYSV)
	friend int read(int, String&, unsigned);
#else
	friend int read(int, String&, int);
#endif
	friend int write(int fd, const String& s) {
		return write(fd, s.d->str, s.d->len);
	}
	friend puts(const String&);
	friend fputs(const String&, FILE*);
	/* STRING(3) FUNCTIONS */
	int strchr(char) const;
	int strrchr(char) const;
	int strpbrk(const String&) const;
	int strspn(const String&) const;
	int strcspn(const String&) const;
	/* MISCELLANEOUS */
	int match(const String&) const;
	int match(const char *) const;
	int firstdiff(const String&) const;
	int firstdiff(const char *) const;
	/* INPUT/OUTPUT */
	int read(istream&,int);
	friend /*inline*/ ostream& operator<<(ostream&,const String&);
	friend istream& operator>>(istream&,String&);
	/* CASE FUNCTIONS */
	String upper() const;
	String lower() const;
};

class Tmpstring {  // Class of String temporaries for Addition
	Srep_ATTLC *d;
	char strung;
	friend class String;
	void overflow() const;
public:
	Tmpstring(Srep_ATTLC* r) { strung=0; d = r; }
	Tmpstring(const String& s) {
		strung=0; s.d->rcinc();
		if (s.d->refc == 0 && s.d->max != 0) { s.overflow(); s.d->rcinc(); }
		d = s.d;
	}
	Tmpstring(const Tmpstring& t) {
		strung=0; t.d->rcinc();
		if (t.d->refc == 0 && t.d->max != 0) { t.overflow(); t.d->rcinc(); }
		d = t.d;
	}
	Tmpstring& operator+(const String&);
	Tmpstring& operator+(const char*);
	Tmpstring& operator+(char);
	~Tmpstring() { if(!strung) d->rcdec(); }
#ifndef __cplusplus
	friend ostream& operator<<(ostream&,const Tmpstring&);
#endif
};

class Substring {
	friend class String;
	Substring(const Substring&);
	String *ss;
	int oo;
	int ll;
	Substring(String &i,int o,int l) : ss(&i), oo(o), ll(l) { }
public:
	void operator=(const String&);
	void operator=(const char* s);
	String *it() const { return ss; }
	int offset() const { return oo; }
	int length() const { return ll; }
#ifndef __cplusplus
	friend ostream& operator<<(ostream&,const Substring&);
#endif
};

// inline functions

inline 
String::String(const Tmpstring& t) { ((Tmpstring*) &t)->strung=1; d = t.d; }

inline
String::String(const Substring& s) {
	d = Srep_ATTLC::new_srep(s.ll);
	if(s.ll) memcpy(d->str,s.ss->d->str+s.oo,s.ll);
}

inline
String::operator const char*() const {
	if (d->len >= d->max && d->len)
		((String *) this)->reserve_grow(d->len + 1);
	*(d->str + d->len) = '\0'; 
	return (d->str);
}

inline String&
String::operator=(const String& r) {
	r.d->rcinc();
	if (r.d->refc == 0 && r.d->max != 0) {
		r.overflow();
		r.d->rcinc();
	}
	d->rcdec();
	d = r.d;
	return *this;
}

inline String&
String::operator=(char c) {
	if(d->refc!=1) {
		d->rcdec();
		d = Srep_ATTLC::new_srep(1);
	}
	d->str[0] = c;
	d->len = 1;
	return *this;
}

inline int
String::compare(const String& s) const {
	register int dlen = d->len;
	register int slen = s.d->len;
	register int i = memcmp(d->str,s.d->str,dlen < slen ? dlen : slen);
	return i ? i : (dlen - slen);
}

inline int
String::compare(const char* s) const {
	register int dlen = d->len;
	register int ln = s ? strlen(s) : 0;
	register int i = memcmp(d->str,s, (dlen < ln ? dlen : ln));
	return i ? i : (dlen - ln);
}

inline int 
strcmp(const String& s,const String& t) {return s.compare(t);}
inline int 
strcmp(const String& s,const char* p) {return s.compare(p);}
inline int 
strcmp(const char* p,const String& t) {return -(t.compare(p));}
inline int 
compare(const String& s,const String& t) {return s.compare(t);}

inline int
operator==(const String& t,const String& s) {
	register int dlen = t.d->len;
	if(dlen != s.d->len) return 0;
	else if(dlen && t.d->str[0] != s.d->str[0]) return 0;
	else if(dlen>1 && t.d->str[1] != s.d->str[1]) return 0;
	else return !memcmp(t.d->str,s.d->str,dlen);
}
inline int
operator==(const String& t,const char* s) {
	register int dlen = t.d->len;
	if(!s) return 0;
	else if(dlen && t.d->str[0]!=s[0] ) return 0;
	else if(dlen>1 && s[0]!='\0' && t.d->str[1]!=s[1] ) return 0;
	else return (dlen != strlen(s)) ? 0 : !memcmp(t.d->str,s,dlen);
}

inline int 
operator==(const char* s,const String& t) { return t==s; }

inline int
operator!=(const String& t,const String& s) {
	register int dlen = t.d->len;
	if(dlen != s.d->len) return 1;
	else if(dlen && t.d->str[0] != s.d->str[0]) return 1;
	else if(dlen>1 && t.d->str[1] != s.d->str[1]) return 1;
	else return memcmp(t.d->str,s.d->str,dlen);
}
inline int
operator!=(const String& t,const char* s) {
	register int dlen = t.d->len;
	if(!s) return 1;
	else if(dlen && t.d->str[0]!=s[0] ) return 1;
	else if(dlen>1 && s[0]!='\0' && t.d->str[1]!=s[1] ) return 1;
	else return (dlen != strlen(s)) ? 1 : memcmp(t.d->str,s,dlen);
}

inline int 
operator!=(const char* s,const String& t) { return t!=s; }
inline int 
operator>=(const String& s,const String& t) { return s.compare(t)>=0; }
inline int 
operator>=(const String& t,const char* s) { return t.compare(s)>=0; }
inline int 
operator>=(const char* s,const String& t) { return t.compare(s)<=0; }
inline int 
operator>(const String& s,const String& t) { return s.compare(t)>0; }
inline int 
operator>(const String& t,const char* s) { return t.compare(s)>0; }
inline int 
operator>(const char* s,const String& t) { return t.compare(s)<0; }
inline int 
operator<=(const String& s,const String& t) { return s.compare(t)<=0; }
inline int 
operator<=(const String& t,const char* s) { return t.compare(s)<=0; }
inline int 
operator<=(const char* s,const String& t) { return t.compare(s)>=0; }
inline int 
operator<(const String& s,const String& t) { return s.compare(t)<0; }
inline int 
operator<(const String& t,const char* s) { return t.compare(s)<0; }
inline int 
operator<(const char* s,const String& t) { return t.compare(s)>0; }

/* STRING(3) FUNCTIONS */
inline int 
strchr(const String& s,int c) { return s.strchr((char) c); }
inline int 
strrchr(const String& s,int c){ return s.strrchr((char) c); }
inline int 
strpbrk(const String& s,const String& t) { return s.strpbrk(t);}
inline int 
strpbrk(const char* s,const String& t) { return String(s).strpbrk(t);}
inline int 
strpbrk(const String& s,const char* t) { return s.strpbrk(String(t));}
inline int 
strspn(const String& s,const String& t) { return s.strspn(t);}
inline int 
strspn(const char* s,const String& t) { return String(s).strspn(t);}
inline int 
strspn(const String& s,const char* t) { return s.strspn(String(t));}
inline int 
strcspn(const String& s,const String& t) { return s.strcspn(t);}
inline int 
strcspn(const char* s,const String& t) { return String(s).strcspn(t);}
inline int 
strcspn(const String& s,const char* t) { return s.strcspn(String(t));}

inline char
String::char_at(unsigned i) const {
	return d->str[i];
}

inline char
String::operator[](unsigned i) const {
	return d->str[i];
}

#ifdef INT_INDEXING

inline char
String::operator[](int i) const {
	return d->str[i];
}
#endif

inline int
String::firstX(char &c) const { 
	return d->len > 0 ? (c=d->str[0],1) : 0;
}
inline int
String::lastX(char &c) const { 
	return d->len > 0 ? (c=d->str[d->len - 1],1) : 0;
}
inline int length(const String& s) { return s.length(); }
inline int hashval(const String& s) { return s.hashval(); }

String sgets(istream&);
int fgets(String&, int, FILE*);
int fgets(String&, FILE*);
int gets(String&);

char* Memcpy_String_ATTLC(register char*,register const char*,int);

inline String& 
String::operator+=(char c) {
	if(d->refc != 1 || d->len == d->max - 1) {
		return newcopy(c);
	}
	else {
		d->str[d->len++] = c;
		return *this;
	}
}

inline String& String::put(const String& s) { return *this += s; }
inline String& String::put(const char* s) { return *this += s; }
inline String& String::put(char c) { return *this += c; }

inline void
String::uniq() const {
	String *This = (String *) this;     // cast away const 
	if(d->refc > 1) {
		register Srep_ATTLC* x = Srep_ATTLC::new_srep(d->len);
		memcpy(x->str,d->str,d->len);
		d->rcdec();
		This->d = x;
	}
}

#endif
