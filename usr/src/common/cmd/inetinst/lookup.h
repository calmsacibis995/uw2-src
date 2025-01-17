/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)inetinst:lookup.h	1.1"

#define WHITESPACE        " \t\n"
#define EOS '\0'
#define MEDIUMBUFSIZE    1024 
#define HOST_ALLOC_SIZE    50
#define ETCCONF_PATH    "/etc/resolv.conf"
#define NETCONFIG	"/etc/netconfig" 
#define ETCCOLS		3
#define GLYPH_COL       0
#define icon_width      7
#define icon_height     7
#define VISIBLE_ITEMS   6


typedef unsigned char ADDR[4];
typedef unsigned char HADDR[8];

typedef enum {
	ERROR,
	INFO,
	WARN
} msgType;

typedef enum _readHostsReturn { 
	NewList, 
	SameList, 
	Failure 
} readHostsReturn;

typedef struct _hostEntry {
	char	*name;
	ADDR	addr;
} hostEntry;

typedef struct _hostList {
	hostEntry	*list;
	int		count;
} hostList;

typedef struct _etcLine{
	char *line;
	struct _etcLine *next;
	struct _etcLine *prev;
} etcLine;

typedef struct _etcHostEntry {
	hostEntry	etcHost;
	char		*comment;
	char		**aliases;
	etcLine		*line;		/*pointer to line */
} etcHostEntry;

typedef struct _etcHostList {
	etcHostEntry		*list;
	int			count;
	etcLine			*commentList;
} etcHostList;

typedef struct _dnsList {
	hostEntry	domain;
	hostList	domainList;
	hostList	systemList;
	hostList	nameServerList;
	struct _dnsList *prev;
	struct _dnsList *next;
} dnsList;

typedef struct _resolvConf {
	char			*domain;
	hostList		serverList; /*name field is used as comment */
} resolvConf;

