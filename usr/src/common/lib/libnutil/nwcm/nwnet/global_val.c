/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/nwcm/nwnet/global_val.c	1.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:lib/libnwcm/global_val.c	1.2"
#ident	"$Id: global_val.c,v 1.22 1994/09/06 17:55:24 mark Exp $"

/*
 * Copyright 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

/*
 * gloval_val.c
 *
 *	NetWare for UNIX Configuration Manager
 *
 *	Global validation functions for the configuration manager.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

#include <sys/nwportable.h>

/*
**	This is always invalid, for tokens not allowed on this platform
*/
/*ARGSUSED*/
int
NWCMInvalidate(long value)
{
	return (0);
}

/*ARGSUSED*/
int
NWCMValidateHashBuckets(long value)
{
    /*XXX*/
    return 1;
}

/*ARGSUSED*/
int
NWCMValidateRouterPath(char * value)
{
    /*XXX*/
    return 1;
}

/*
 * Validate a proposed NetWare server_name 
 */
int
NWCMValidateServerName(char * name)
{
char *ptr;
size_t length;

        /* Check for correct length */
        length=strlen(name);
        if( (length < 2) || (length > NWMAX_SERVER_NAME_LENGTH -1) )
	  {
	    return 0;
	  }

        /* Check for illegal characters */
        ptr = name;
        while((*ptr) != 0)
	  {
	    switch(*ptr)
	      {
	      case ' ':
	      case '\t':
	      case '\n':
	      case '\'':
	      case ',':
	      case '.':
              case '?':
              case '+':
              case '!':
              case '@':
              case '#':
              case '%':
              case '^':
              case '&':
              case '*':
              case '(':
              case ')':
	      case ':':
	      case ';':
	      case '<':
	      case '>':
	      case '/':
		return(0);
	      default:
		ptr++;
		continue;
	      }
	  }
	return(1);
}
/*
 * int strcmpi(s1, s2)
 *
 *  Like strcmp(), but case insensitive.
 */

static int
strcmpi(char * s1, char * s2)
{
    int r;

    while (*s1 && *s2)
        if ((r = (int) (toupper(*s1++) - toupper(*s2++))) != 0)
            return r;
    return (int) (toupper(*s1) - toupper(*s2));
}
