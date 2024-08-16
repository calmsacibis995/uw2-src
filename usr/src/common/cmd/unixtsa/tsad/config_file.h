/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/tsad/config_file.h	1.2"

#include	<netdb.h>
#include	<limits.h>

/********************************************************
 ****** General useful functions and definitions ********
 ********************************************************/


#define	MAXPROTOLEN	8
#define MAXCODESETLEN	16

#define	TCP_NAME	"tcp"
#define SPX_NAME	"spx"

struct tsaconfigent {
	char hostName[MAXHOSTNAMELEN] ;
	char protocol[MAXPROTOLEN] ;  // "tcp", "spx"
	char tsaCodeset[MAXCODESETLEN] ;
	char engineCodeset[MAXCODESETLEN] ;
	char engineLocale[MAXCODESETLEN] ;
} ;

void settsaconfigent(char *fileName) ;
struct tsaconfigent *gettsaconfigent() ;
void endtsaent() ;
