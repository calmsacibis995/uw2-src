/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/ckpath.c	1.3.5.10"
#ident  "$Header: $"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pfmt.h>
#include "valtools.h"
#include <malloc.h>
#include <unistd.h>

extern void	*calloc(),
		free(),
		puthelp(),
		puterror(),
		putprmpt();
extern int	creat(),
		close(),
		getinput();
extern int	check_quit();

#define	I_SYNTAX	"uxadm:11"
#define E_SYNTAX	"does not meet suggested filename syntax standard"
#define	I_READ		"uxadm:12"
#define E_READ		"is not readable"
#define	I_WRITE		"uxadm:13"
#define E_WRITE		"is not writable"
#define	I_EXEC		"uxadm:14"
#define E_EXEC		"is not executable"
#define	I_CREAT		"uxadm:15"
#define E_CREAT		"cannot be created"
#define	I_ABSOLUTE	"uxadm:16"
#define E_ABSOLUTE	"must begin with a slash (/)"
#define	I_RELATIVE	"uxadm:17"
#define E_RELATIVE	"must not begin with a slash (/)"
#define	I_EXIST		"uxadm:18"
#define E_EXIST		"does not exist"
#define	I_NEXIST	"uxadm:19"
#define E_NEXIST	"must not already exist"
#define	I_BLK		"uxadm:20"
#define E_BLK		"must specify a block special device"
#define	I_CHR		"uxadm:21"
#define E_CHR		"must specify a character special device"
#define	I_DIR		"uxadm:22"
#define E_DIR		"must specify a directory"
#define	I_REG		"uxadm:23"
#define E_REG		"must be a regular file"
#define	I_NONZERO	"uxadm:24"
#define E_NONZERO	"must be a file of non-zero length"

#define	X_READ		"uxadm:25"
#define H_READ		"must be readable"
#define	X_WRITE		"uxadm:26"
#define H_WRITE		"must be writable"
#define	X_EXEC		"uxadm:27"
#define H_EXEC		"must be executable"
#define	X_CREAT		"uxadm:28"
#define H_CREAT		"will be created if it does not exist"
#define	X_ABSOLUTE	I_ABSOLUTE
#define H_ABSOLUTE	E_ABSOLUTE
#define	X_RELATIVE	I_RELATIVE
#define H_RELATIVE	E_RELATIVE
#define	X_EXIST		"uxadm:29"
#define H_EXIST		"must already exist"
#define	X_NEXIST	"uxadm:19"
#define H_NEXIST	"must not already exist"
#define	X_BLK		I_BLK
#define H_BLK		E_BLK
#define	X_CHR		I_CHR
#define H_CHR		E_CHR
#define	X_DIR		I_DIR
#define H_DIR		E_DIR
#define	X_REG		I_REG
#define H_REG		E_REG
#define	X_NONZERO	I_NONZERO
#define H_NONZERO	E_NONZERO

#define MSGSIZ	1024
#define	STDHELP_ID	"uxadm:30"
#define STDHELP \
	"A pathname is a filename, optionally preceded by parent directories." 

static char	*errstr;
static char	*badset = "*?[]{}()<> \t'`\"\\|^";

static void
addhlp(msg, text)
char	*msg, *text;
{
	static int count;

	if(text == NULL) {
		count = 0;
		return;
	}
	if(!count++)
		(void) strcat(msg, (const char *)gettxt("uxadm:31", " The pathname you enter:"));
	(void) strcat(msg, "\\n\\t-\\ ");
	(void) strcat(msg, text);
}

static char *
sethlp(pflags)
int	pflags;
{
	char	*msg;

	if ((msg = (char *) calloc(MSGSIZ, sizeof(char))) == NULL)
		return(msg);
	addhlp(msg, (char *)0); /* initialize count */
	(void) strcpy(msg, (const char *)gettxt(STDHELP_ID, STDHELP));

	if(pflags & P_EXIST)
		addhlp(msg, gettxt(X_EXIST, H_EXIST));
	else if(pflags & P_NEXIST)
		addhlp(msg, gettxt(X_NEXIST, H_NEXIST));

	if(pflags & P_ABSOLUTE)
		addhlp(msg, gettxt(X_ABSOLUTE, H_ABSOLUTE));
	else if(pflags & P_RELATIVE)
		addhlp(msg, gettxt(X_RELATIVE, H_RELATIVE));

	if(pflags & P_READ)
		addhlp(msg, gettxt(X_READ, H_READ));
	if(pflags & P_WRITE)
		addhlp(msg, gettxt(X_WRITE, H_WRITE));
	if(pflags & P_EXEC)
		addhlp(msg, gettxt(X_EXEC, H_EXEC));
	if(pflags & P_CREAT)
		addhlp(msg, gettxt(X_CREAT, H_CREAT));

	if(pflags & P_BLK)
		addhlp(msg, gettxt(X_BLK, H_BLK));
	else if(pflags & P_CHR)
		addhlp(msg, gettxt(X_CHR, H_CHR));
	else if(pflags & P_DIR)
		addhlp(msg, gettxt(X_DIR, H_DIR));
	else if(pflags & P_REG)
		addhlp(msg, gettxt(X_REG, H_REG));

	if(pflags & P_NONZERO)
		addhlp(msg, gettxt(X_NONZERO, H_NONZERO));

	return(msg);
}

ckpath_stx(pflags)
int	pflags;
{
	if(((pflags & P_ABSOLUTE) && (pflags & P_RELATIVE)) ||
	   ((pflags & P_NEXIST) && (pflags & 
		(P_EXIST|P_NONZERO|P_READ|P_WRITE|P_EXEC))) ||
	   ((pflags & P_CREAT) && (pflags & (P_EXIST|P_NEXIST|P_BLK|P_CHR))) ||
	   ((pflags & P_BLK) && (pflags & (P_CHR|P_REG|P_DIR|P_NONZERO))) ||
	   ((pflags & P_CHR) && (pflags & (P_REG|P_DIR|P_NONZERO))) ||
	   ((pflags & P_DIR) && (pflags & P_REG))) {
		return(1);
	}
	return(0);
}

int
ckpath_val(path, pflags)
char	*path;
int	pflags;
{
	struct stat status;
	int	fd;
	char	*pt;
	char	*cmd;

	if((pflags & P_RELATIVE) && (*path == '/')) {
		errstr = gettxt(I_RELATIVE, E_RELATIVE);
		return(1);
	}
	if((pflags & P_ABSOLUTE) && (*path != '/')) {
		errstr = gettxt(I_ABSOLUTE, E_ABSOLUTE);
		return(1);
	}
	if(stat(path, &status)) {
		if(pflags & P_EXIST) {
			errstr = gettxt(I_EXIST, E_EXIST);
			return(1);
		}
		for(pt=path; *pt; pt++) {
			if(!isprint(*pt) || strchr(badset, *pt)) {
				errstr = gettxt(I_SYNTAX, E_SYNTAX);
				return(1);
			}
		}
		if ((pflags & P_CREAT) && (pflags & P_DIR)) {
			cmd = malloc(strlen(path)+7);
			if (mkdir(path,0755) == -1) {
				errstr = gettxt(I_CREAT, E_CREAT);
				return(1);
			}
		}
		else
			if(pflags & P_CREAT) {
				if((fd = creat(path, 0644)) < 0) {
					errstr = gettxt(I_CREAT, E_CREAT);
					return(1);
				}
				(void) close(fd);
			}
		return(0);
	} else if(pflags & P_NEXIST) {
		errstr = gettxt(I_NEXIST, E_NEXIST);
		return(1);
	}
	if((status.st_mode & S_IFMT) == S_IFREG) {
		/* check non zero status */
		if((pflags & P_NONZERO) && (status.st_size < 1)) {
			errstr = gettxt(I_NONZERO, E_NONZERO);
			return(1);
		}
	}
	if((pflags & P_CHR) && ((status.st_mode & S_IFMT) != S_IFCHR)) {
		errstr = gettxt(I_CHR, E_CHR);
		return(1);
	}
	if((pflags & P_BLK) && ((status.st_mode & S_IFMT) != S_IFBLK)) {
		errstr = gettxt(I_BLK, E_BLK);
		return(1);
	}
	if((pflags & P_DIR) && ((status.st_mode & S_IFMT) != S_IFDIR)) {
		errstr = gettxt(I_DIR, E_DIR);
		return(1);
	}
	if((pflags & P_REG) && ((status.st_mode & S_IFMT) != S_IFREG)) {
		errstr = gettxt(I_REG, E_REG);
		return(1);
	}
	if((pflags & P_READ) && !(status.st_mode & S_IREAD)) {
		errstr = gettxt(I_READ, E_READ);
		return(1);
	}
	if((pflags & P_WRITE) && !(status.st_mode & S_IWRITE)) {
		errstr = gettxt(I_WRITE, E_WRITE);
		return(1);
	}
	if((pflags & P_EXEC) && !(status.st_mode & S_IEXEC)) {
		errstr = gettxt(I_EXEC, E_EXEC);
		return(1);
	}
	return(0);
}

void
ckpath_err(pflags, error, input)
int	pflags;
char	*error, *input;
{
	char	buffer[2048];
	char	*defhlp;

	if(input) {
		if(ckpath_val(input, pflags)) {
			(void) sprintf(buffer, (const char *)gettxt("uxadm:32", "Pathname %s."), errstr);
			puterror(stdout, buffer, error);
			return;
		}
	}
	defhlp = sethlp(pflags);
	puterror(stdout, defhlp, error);
	free(defhlp);
}

void
ckpath_hlp(pflags, help)
int	pflags;
char	*help;
{
	char	*defhlp;

	defhlp = sethlp(pflags);
	puthelp(stdout, defhlp, help);
	free(defhlp);
}

int
ckpath(pathval, pflags, defstr, error, help, prompt)
char *pathval;
int pflags;
char *defstr, *error, *help, *prompt;
{
	char	*defhlp=NULL,
		input[128],
		buffer[256];
	char *defprompt = NULL;

	if((pathval == NULL) || ckpath_stx(pflags))
		return(2); /* usage error */

	if(!prompt) {
		if(pflags & P_ABSOLUTE)
			prompt = gettxt("uxadm:33", "Enter an absolute pathname");
		else if(pflags & P_RELATIVE)
			prompt = gettxt("uxadm:34", "Enter a relative pathname");
		else
			prompt = gettxt("uxadm:35", "Enter a pathname");
	} else if(strchr(prompt, '~')) {
		if(pflags & P_ABSOLUTE)
			defprompt = gettxt("uxadm:33", "Enter an absolute pathname");
		else if(pflags & P_RELATIVE)
			defprompt = gettxt("uxadm:34", "Enter a relative pathname");
		else
			defprompt = gettxt("uxadm:35", "Enter a pathname");
	}
	defhlp = sethlp(pflags);
	if (!defhlp)
		return(1);

start:
	putprmpt(stderr, prompt, NULL, defstr, defprompt);
	if(getinput(input)) {
		free(defhlp);
		return(1);
	}

	if(strlen(input) == 0) {
		if(defstr) {
			(void) strcpy(pathval, defstr);
			free(defhlp);
			return(0);
		}
		puterror(stderr, NULL, gettxt("uxadm:36", "Input is required."));
		goto start;
	}
	if(!strcmp(input, "?")) {
		puthelp(stderr, defhlp, help);
		goto start;
	}
	if (!check_quit(input)) {
		free(defhlp);
		return(3);
	}

	if(ckpath_val(input, pflags)) {
		(void) sprintf(buffer, (const char *)gettxt("uxadm:32", "Pathname %s."), errstr);
		puterror(stderr, buffer, error);
		goto start;
	}
	(void) strcpy(pathval, input);
	free(defhlp);
	return(0);
}
