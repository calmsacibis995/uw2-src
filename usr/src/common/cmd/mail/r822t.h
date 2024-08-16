/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/r822t.h	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)r822t.h	1.2 'attmail mail(1) command'"
#ifndef r822t_h
#define r822t_h

/*
** Flag values for r822_addr_parse and r822_find_field
**
**		r822_LOOSE_UNFOLD  unfold using rules for unstructured fields
**		r822_ALLOW_FROM_   allow From_ header as field name
**
** Other useful flags (some are SMTP and/or GMS specific)
**
**		r822_ADDRESSES     an addressing header field
**		r822_FILTER_IN     filter during input
**		r822_FILTER_OUT    filter during output
**		r822_SIMPLE_SMTP   okay for simple SMTP service
**		r822_HAS_VALUE     next link is a value for an option
**		r822_NO_COMMENT    don't apply comments to address
**		r822_IS_FROM_ADDR  address is a FROM: address
**		r822_DEBUGGING     debugging mode on
**		r822_INTERESTING   not your average header
*/
#define r822_ALLOW_FROM_      (1<< 7)
#define r822_LOOSE_UNFOLD     (1<<11)
#define r822_ADDRESSES        (1<<13)
#define r822_FILTER_IN        (1<<14)
#define r822_FILTER_OUT       (1<<15)
#define r822_SIMPLE_SMTP      (1<<16)
#define r822_HAS_VALUE        (1<<17)
#define r822_NO_COMMENT       (1<<18)
#define r822_IS_FROM_ADDR     (1<<19)
#define r822_DEBUGGING        (1<<20)
#define r822_INTERESTING      (1<<21)

typedef struct r822_FILE r822_FILE;

/*
** For character-at-a-time I/O supporting arbitrary amounts of pushback.
*/
struct r822_FILE
{
	string *pushback;
	FILE *fp;
};

extern void init_r822_FILE ARGS((r822_FILE *this, FILE *sfp));
extern void fini_r822_FILE ARGS((r822_FILE *this));
extern r822_FILE *new_r822_FILE ARGS(());
extern void delete_r822_FILE ARGS((r822_FILE *this));

extern int  r822_find_field_name ARGS((int (*getter)(void*), int (*ungetter)(int, void*), void *blob, int flags, string *fn));
extern int  r822_find_field_body ARGS((int (*getter)(void*), int (*ungetter)(int, void*), void *blob, int flags, string *fb));
extern int  r822_fgetc ARGS((r822_FILE *rfp));
extern int  r822_ungetc ARGS((int chint, r822_FILE *rfp));
extern r822_subthing *r822_slash_options ARGS((string *str));
extern int  r822_slash_options_all ARGS((r822_address *ap));

#endif
