/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/getline.c	1.2.2.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)getline.c	1.2 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	getline - read line from a file

    SYNOPSIS
	int getline(char *ptr2line, int max, FILE *f)

    DESCRIPTION
	Read a line from the file "f", assign it to "line" and
	return number of bytes in length. Unlike fgets(),
	this function does NOT stop at a NUL byte.
*/

int getline(ptr2line, max, f)
char	*ptr2line;
int	max;
FILE	*f;
{
	int	i,ch;
	for (i=0; i < max-1 && (ch=getc(f)) != EOF;)
		if ((ptr2line[i++] = (char)ch) == '\n') break;
	ptr2line[i] = '\0';
	return(i);
}
