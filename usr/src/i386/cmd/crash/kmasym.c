/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/kmasym.c	1.1"
#ident "$Header: kmasym.c 1.1 91/07/23 $"

#include <stdio.h>
#include <sys/mod.h>

char *Addr2Symbol(addr)
unsigned long addr;
{
	static char symbol[MAXSYMNMLEN + 12];
	unsigned long offset;

	if(getksym(symbol, &addr, &offset) != 0) {
		sprintf(symbol,"%-#8x",addr);
		return(symbol);
	}
	if(offset != 0) 
		sprintf(symbol+strlen(symbol)," + %x",offset);

	return(symbol);
}
