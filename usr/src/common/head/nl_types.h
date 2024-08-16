/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NL_TYPES_H
#define _NL_TYPES_H
#ident	"@(#)sgs-head:common/head/nl_types.h	1.14"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NL_SETMAX
#   define NL_SETMAX	1024
#   define NL_MSGMAX	32767
#   define NL_TEXTMAX	4096
#endif

#define NL_MAXPATHLEN	1024
#define NL_PATH		"NLSPATH"
#define NL_LANG		"LANG"
#define NL_DEF_LANG	"C"
#define NL_SETD		1
#define NL_MAX_OPENED	10

#define NL_CAT_LOCALE	1

#ifndef _NL_ITEM
#   define _NL_ITEM
	typedef int	nl_item;
#endif

typedef struct _nl_catd	*nl_catd;

#ifndef _XOPEN_SOURCE

struct cat_msg
{
	int		msg_nr;		/* message number */
	int		msg_len;	/* actual message len */
	long		msg_off;	/* message offset in the tmp file */
	char		*msg_ptr;	/* pointer to the actual message */
	struct cat_msg	*msg_next;	/* next in list */
};

struct cat_set
{
	int		set_nr;		/* set number */
	int		set_msg_nr;	/* number of messages in the set */
	struct cat_msg	*set_msg;	/* associated message list */
	struct cat_set	*set_next;	/* next in list */
};

struct m_cat_set
{
	int	first_msg;		/* first message number */
	int	last_msg;		/* last message in the set */
};

struct set_info
{
	int			no_sets;
	struct m_cat_set	sn[1];
};

#define CMD_SET		"set"
#define CMD_SET_LEN	3
#define CMD_DELSET	"delset"
#define CMD_DELSET_LEN	6
#define CMD_QUOTE	"quote"
#define CMD_QUOTE_LEN	5

#define XOPEN_DIRECTORY	"/usr/lib/locale/Xopen/LC_MESSAGES"
#define DFLT_MSG	"\01"
#define M_EXTENSION	".m"

#define DEF_NLSPATH	"/usr/lib/locale/%L/LC_MESSAGES/%N:/usr/lib/locale/%L/LC_MESSAGES/%N.cat:/usr/lib/locale/C/LC_MESSAGES/%N:/usr/lib/locale/C/LC_MESSAGES/%N.cat"

struct cat_hdr
{
	long	hdr_magic;		/* magic number */
	int	hdr_set_nr;		/* set nr in file */
	int	hdr_mem;		/* space needed to load the file */
	long	hdr_off_msg_hdr;	/* position of messages headers */
	long	hdr_off_msg;		/* position of messages in file */
};

struct cat_set_hdr
{
	int	shdr_set_nr;	/* set number */
	int	shdr_msg_nr;	/* number of messages in set */
	int	shdr_msg;	/* start offset of messages in list */
};

struct cat_msg_hdr
{
	int	msg_nr;		/* messge number */
	int	msg_len;	/* message len */
	int	msg_ptr;	/* message offset in table */
};

#define CAT_MAGIC	0xFF88FF89

struct _nl_catd
{
	char	type;
	int	set_nr;
	union
	{
		struct malloc_data
		{
			struct cat_set_hdr	*sets;
			struct cat_msg_hdr	*msgs;
			char			*data;
		} m;
		struct gettxt_data
		{
			struct set_info	*sets;
			char		*msgs;
			int		msg_size;
			int		set_size;
		} g;
	} info;
};

typedef struct _nl_catd nl_catd_t;

#define MKMSGS		'M'	/* mkmsgs interfaces */
#define MALLOC		'm'	/* old style malloc */

#define BIN_MKMSGS	"mkmsgs"

#endif /*_XOPEN_SOURCE*/

#ifdef __STDC__

extern int	catclose(nl_catd);
extern char	*catgets(nl_catd, int, int, const char *);
extern nl_catd	catopen(const char *, int);

#ifndef	_XOPEN_SOURCE
extern char	*gettxt(const char *, const char *);
#endif

#else /*!__STDC__*/

extern int	catclose();
extern char	*catgets();
extern nl_catd	catopen();

#ifndef	_XOPEN_SOURCE
extern char	*gettxt();
#endif

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_NL_TYPES_H*/
