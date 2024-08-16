/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/strparse.h	1.4"

#ifndef STRPARSE_H
#define STRPARSE_H

typedef unchar * VAL;

/*
 * Structures for driving generic print/parse/copy/free routines
 */

struct strhead {
	unsigned char flags;
	unsigned char type;
	unsigned short size;
	char *name; /* Not in all TYPES!!! */
};

struct intstr {
	unsigned char flags;
	unsigned char type; /* TYPE_INT */
	unsigned short size;
	char *name;
};

struct unionstr {
	unsigned char flags;
	unsigned char type; /* TYPE_UNION */
	unsigned short size;
	char *name;
	unsigned char choice_size;
	long choice_offset;
	struct union_fieldstr *fields;
};
/* Notice that ptrstr and arraystr are currently equivalent */

struct ptrstr {
	unsigned char flags;
	unsigned char type; /* TYPE_POINTER */
	unsigned short size;
	struct strhead *typeptr;
};

struct arraystr {
	unsigned char flags;
	unsigned char type; /* TYPE_ARRAY, TYPE_DYNARRAY or TYPE_POINTER */
	unsigned short size;
	struct strhead *typeptr;
};

struct structstr {
	unsigned char flags;
	unsigned char type; /* TYPE_STRUCT */
	unsigned short size;
	char *name;
	struct fieldstr *fields;
};

struct typedefstr {
	unsigned char flags;
	unsigned char type; /* TYPE_TYPEDEF */
	unsigned short size;
	char *name;
	struct strhead *typeptr;
};

struct fieldstr {
	unsigned char flags;
	char *name;
	struct strhead *typeptr;
	ushort offset;
	short assoc_field_num;
};

struct union_fieldstr {
	unsigned char flags;
	char *name;
	struct strhead *typeptr;
	unsigned long tag;
};

union anytype {
	struct strhead head;
	struct intstr t_int;
	struct unionstr t_union;
	struct ptrstr t_ptr;
	struct arraystr t_array;
	struct structstr t_struct;
	struct typedefstr t_typedef;
};

/*
 * Definitions for the kind field of the above structure 
 */

#define TYPE_INT		(1)	/* int or uint */
#define TYPE_UNION		(2)	/* union { } */
#define TYPE_TYPEDEF	(3)	/* typedef */
#define TYPE_STRUCT		(4)	/* structure */
#define TYPE_POINTER	(5)	/* pointer */
#define TYPE_ARRAY		(6)	/* array */
#define TYPE_DYNARRAY	(7)	/* dynamic array, a pointer to a group,
							like a pointer - but parsed differently */

/*
 * General flags for any flags field
 */

#define F_SPECIAL		(1) /* There is a special parser */
#define F_SYMBOLIC		(2) /* There is a symbolic */
#define F_LENGTH_FIELD	(4) /* Field is a length field, only in structures */
#define F_CHOICE_FIELD	(8) /* Field is a choice field, only in structures */
#define F_READONLY		(16) /* No modifications allowed on builtin types - they are const's */
#define F_END_GENFLAGS	(16)

/*
 * Extra flags for dynarray
 */
#define F_EXTERNAL_LENGTH	(2 * F_END_GENFLAGS) /* Length field in surrounding structure */

/*
 * Extra flags for union
 */
#define F_EXTERNAL_TAG		(2 * F_END_GENFLAGS) /* Choice field in surrounding structure */

/*
 * Extra flags for union_fieldstr
 */
#define F_NO_TAG			(2 * F_END_GENFLAGS) /* This field has no tag */

struct buf {
	ulong curspot;
	ulong buflen;
	char *buf;
};

union fd_or_buf {
	int fd;
	struct buf *buf;
};

struct easyio {
	unchar type;
	unchar flags;
	union fd_or_buf fd_or_buf;
};

struct easyiobuf {
	unchar type;
	unchar flags;
	struct buf *buf;
};

#define EZ_FD	1
#define EZ_STR	2

extern struct easyiobuf Stdeasybuf;

struct valtype {
	char *name;
	VAL val;
	struct strhead *type;
};

struct assoc_field {
	VAL *pval;
	struct strhead *type;
};

#endif /* STRPARSE_H */
