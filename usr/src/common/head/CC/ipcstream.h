/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:ipc/incl/ipcstream.h	3.2" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

/*ident	"@(#)ctrans:incl/ipcstream.h	1.1.1.2" */
#ifndef IPCSTREAMH
#define IPCSTREAMH

#include <iostream.h>
#include <ipcmonitor.h>
#include <String.h>

class ipcbuf;
class ipcstream;

// an attachment to a point in the file system for ipc 
class ipc_attachment {
	friend class ipcstream;
public: 
	static char ftok_id;
	ipc_attachment(const String& path, const char* param =0);
	ipc_attachment(const char* path, const char* param =0);
	~ipc_attachment();
	void listen();
	operator const void*()const { return okay ? this : 0; }
	int operator!()const { return !okay; } 
	int fail()const	{ return !okay; }
	int bad()const	{ return !okay; }
	int good()const	{ return okay; }
	inline const char* username();
	inline int uid();
	inline int gid();
	inline char flags();	
	ipcstream accept();
	void reject(int, const char*);
private:
	String att_name;
	String last_connection;
	int xfd;
	int okay;
	Monitor* sema1;
	Monitor* sema2;
	char* user_p;
	int uid_p;
	int gid_p;
	int attached;
	void check_info() { if (user_p==0) listen(); }
	void initialize_attachment();
	ipcbuf*	create_ipcbuf();
	ipc_attachment(const ipc_attachment&);  // declared but not defined
	ipc_attachment&	operator=(const ipc_attachment&);  // ditto
};

// a stream buffer for ipc 
class  ipcbuf : public streambuf {
	friend class ipcstream;
	friend class ipc_attachment;
	ipcbuf(int in_fd, int out_fd);
public:
	ipcbuf();
	ipcbuf(const String& path, const char* param); 
	ipcbuf(const char* path, const char* param); 
	int is_open() const { return opened; }
	int close();
	~ipcbuf();
	virtual int overflow(int=EOF);
	virtual int underflow();
	virtual int sync();
	virtual streambuf* setbuf(char*  p, int len);
	virtual streambuf* set_in_buf(char*  p, int len);
	virtual streambuf* set_out_buf(char* p, int len);
	int remote_errno;
	String remote_reason;
protected:
	int in_xfd;	
	int out_xfd;	
	char opened;
	char owner;
	char* in_start;
	char lahead[2];
	char* aux_base() { return aux_x_base; }
	char* aux_ebuf() { return aux_x_base+aux_x_blen; }
	int aux_blen() const { return aux_x_blen; }
	void aux_setb(char* b, char* eb, int a = 0 ) {
		if ( aux_alloc && aux_x_base ) delete aux_x_base;
		aux_x_base = b;
		aux_x_blen= (eb>b) ? (eb-b) : 0;
		aux_alloc = a;
	}
	int in_unbuffered() const { return x_get_unbuf; }
	void in_unbuffered(int unb) { x_get_unbuf = (unb!=0); }
	int out_unbuffered() const { return x_put_unbuf; }
	void out_unbuffered(int unb) { x_put_unbuf = (unb!=0); }
	int in_allocate() { return streambuf::allocate(); }
	int allocate(); // declared but not defined
	int out_allocate() {
		if (aux_x_base==0 && !out_unbuffered()) 
			return do_out_allocate();
		else	return 0;
	}
	virtual int do_out_allocate();
private:
	short aux_alloc;
	short x_get_unbuf;	// user requested no input buffer
	short x_put_unbuf;	// user requested no output buffer
	char* aux_x_base;	// use for output buffer	
	int aux_x_blen;
	String in_pip;
	String out_pip;
	ipcbuf*	open (const char* path, const char* param);
	ipcbuf*	open (const String& path, const char* param);
	ipcbuf(String in_pipe, String out_pipe);
};

class ipcstream : public iostream {
	friend class ipc_attachment;
public:
	ipcstream(const String& path, const char* param =0);
	ipcstream(const char* path, const char* param =0);
	ipcstream(ipc_attachment&);
	~ipcstream();
	static char ftok_id;
	void close();
	void setbuf(char*  p, int l);
	ipcbuf*	rdbuf() { return (ipcbuf*)bp; }
protected:
	inline void verify(int);
	int checkbuf() { return bp ? rdbuf()->opened : 0; }
private:
	//ipcstream(int in_fd, int out_fd);
	void open(const char* name, const char* param);
	void open(const String& name, const char* param);
};

inline const char*
ipc_attachment::username()
{
	check_info();
	return user_p;
}

inline int
ipc_attachment::uid()
{
	check_info();
	return uid_p;
}

inline int
ipc_attachment::gid()
{
	check_info();
	return gid_p;
}

#endif
