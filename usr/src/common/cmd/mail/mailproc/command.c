/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mailproc/command.c	1.1.1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)command.c	1.2 'attmail mail(1) command'"
/* from ferret SCCSid command.c 3.1 */
#include "../mail/libmail.h"
#include "mailproc.h"

Cmd	*Cmdlist = NULL;

/* parse a command file into a series of Cmd */
/* lines kept in the list pointed by Cmdlist. */
int cmdparse(fname)
char *fname;				/* file name */
{
	int	clines;			/* number command lines */
	Cmd	*cp;			/* command pointer */
	Cmd	*last;			/* last command */
	FILE	*fp;			/* file pointer */
	char	buf[sizeof(cp->cmdbuf)];/* holds line from file */

	if ((fp = fopen(fname, "r")) == NULL)
	{
		(void) pfmt(stderr, MM_ERROR, ":565:Cannot open command file %s: %s\n",fname, Strerror(errno));
		return -1;
	}

	clines = 0;
	cp = NULL;
	while (fgets(buf, sizeof(buf), fp))
	{
		if ((buf[0] == '\n') || (buf[0] == '#'))
			continue;
		if ((cp = new_Cmd(buf)) == NULL)
		{
			(void) fclose(fp);
			(void) pfmt(stderr, MM_ERROR, ":382:Problem allocating memory\n");
			return -1;
		}
		if (last) last->next = cp;
		if (!Cmdlist) Cmdlist = cp;
		last = cp;
		clines++;
	}

	(void) fclose(fp);
	return clines;
}

/* allocate and initialize a new Cmd */
Cmd *new_Cmd(cmdbuf)
char *cmdbuf;
{
	Cmd *ret = (Cmd*) malloc(sizeof(Cmd));
	if (!ret) return 0;

	strcpy(ret->cmdbuf, cmdbuf);
	ret->next = NULL;
	ret->flp = new_Fromlist();
	if (!ret->flp)
	{
		free(ret);
		return 0;
	}
	return ret;
}
