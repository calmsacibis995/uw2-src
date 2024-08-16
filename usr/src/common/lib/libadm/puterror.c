/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/puterror.c	1.1.7.5"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>
#include <pfmt.h>
#include <unistd.h>

extern int	ckwidth;
extern int	ckindent;
extern int	puttext();
extern void	*calloc(),
		free();

void
puterror(fp, defmesg, error)
FILE *fp;
char *defmesg, *error;
{
	char	*tmp=NULL;
	int	n,msg_len,indent;
	
	msg_len = pfmt(fp,MM_NOGET|MM_ERROR,"");

	if(error == NULL) {
		/* use default message since no error was provided */
		if (! defmesg)
			defmesg = gettxt("uxadm:49","invalid input");
		tmp = calloc(strlen(defmesg)+1, sizeof(char));
		(void) strcpy(tmp, defmesg);
	} else  {
		n = strlen(error);
		if(defmesg != NULL) {
			if(error[0] == '~') {
				/* prepend default message */
				tmp = calloc(n+strlen(defmesg)+1, sizeof(char));
				(void) strcpy(tmp, defmesg);
				(void) strcat(tmp, "\n");
				(void) strcat(tmp, ++error);
			} else if(n && (error[n-1] == '~')) {
				/* append default message */
				tmp = calloc(n+strlen(defmesg), sizeof(char));
				(void) strcpy(tmp, error);
				tmp[n-2] = '\0';
				(void) strcat(tmp, "\n");
				(void) strcat(tmp, defmesg);
			} else {
				tmp = calloc(n+1, sizeof(char));
				(void) strcpy(tmp, error);
			}
		} else {
			tmp = calloc(n+1, sizeof(char));
			(void) strcpy(tmp, error);
		}
	}
	indent = ckindent;
	if ((ckwidth-msg_len) < (int)strlen(tmp))
		(void) fputc('\n',fp);
	else
		indent = 0;
	if (tmp){	/* <--just in case (error != NULL && defmesg == NULL) */
		(void) puttext(fp, tmp, indent, ckwidth);
		(void) fputc('\n', fp);
		free(tmp);
	}
}
