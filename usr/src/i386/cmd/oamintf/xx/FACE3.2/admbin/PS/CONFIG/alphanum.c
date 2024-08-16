/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/CONFIG/alphanum.c	1.2"
#ident	"$Header: $"

#define isalphanum(k) (((k >= 'a') && (k <= 'z')) || ((k >= 'A') && (k <= 'Z')) || ((k >= '0') && (k <= '9')) || (k == '_'))

main(argc,argv)
int argc;
char *argv[];
{
 char *p = 0, c;
 p = *(++argv);
 while (c = *p) {
      if (! (isalphanum(c))) exit(1); 
      p++;
 }
exit(0);
}
