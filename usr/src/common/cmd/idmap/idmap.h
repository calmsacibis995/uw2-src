/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)idmap:idmap.h	1.4.3.2"
#ident  "$Header: idmap.h 2.0 91/07/12 $"

#define	MODULE_ID	"UX:idmap"
#define	MAPDIR		"/etc/idmap"
#define	ATTRMAP		"attrmap"
#define	LOGFILE		"/var/adm/log/idmap.log"
#define	OLDLOGFILE	"/var/adm/log/idmap.log.old"
#define	IDATA		"idata"
#define	UIDATA		"uidata"
#define	DOTMAP		".map"
#define	DOTTMP		".tmp"
#define	IDATA_MODE	((mode_t) 0664)
#define	ATTR_MODE	IDATA_MODE
#define	UIDATA_MODE	IDATA_MODE
#define	SECURE_MODE	((mode_t) 0000)
#define	LOGFILE_MODE	((mode_t) 0660)
#define	DIR_MODE	((mode_t) 0775)
#define	IDATA_LEVEL	"SYS_PUBLIC"
#define	ATTR_LEVEL	IDATA_LEVEL
#define	UIDATA_LEVEL	IDATA_LEVEL
#define	DIR_LEVEL	IDATA_LEVEL
#define	IDMAP_LOGIN	"sys"
#define	IDMAP_GROUP	"sys"

#define	MAXLOGLEN	100 * 512
#define	MAXFILE		64
#define	MAXLINE		512
#define	MAXFIELDS	10	/* this cannot be more than 10 */

/* name field information structure */
typedef struct {
	char type;
	char *value;
} FIELD;

/* id mapping return codes for check_entry() function */

#define	IE_NOERROR	0	/* no errors */
#define	IE_SYNTAX	1	/* syntax error */
#define	IE_MANDATORY	2	/* mandatory field missing */
#define	IE_DUPLICATE	3	/* duplicate remote name/value field */
#define	IE_ORDER	4	/* entry out of order */
#define	IE_NOUSER	5	/* mapped to login not in passwd file */
#define	IE_NOFIELD	6	/* field not present for %n macro */
