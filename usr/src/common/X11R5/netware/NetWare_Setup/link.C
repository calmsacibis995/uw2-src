#ident	"@(#)nwsetup:link.C	1.3"
// link.c

///////////////////////////////////////////////////////////////////
// Copyright (c) 1992 Marshall Brain
// Copyright (c) 1992 Digital Equipment Corp.
// Copyright (c) 1993 Novell
// All Rights Reserved
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NOVELL
//
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
///////////////////////////////////////////////////////////////////

#include "link.h"

void link_open (struct link_handle *l, char path_name[], char name[], char param1[], char param2[], char param3[])
{
	pipe (l->pipefd1);
	pipe (l->pipefd2);
	if ((l->pid = fork ()) == 0) {	// child process
		close (l->pipefd1[0]);
		close (1);
		dup (l->pipefd1[1]);
		close (2);		// 2 new lines
		dup (l->pipefd1[1]);
		close (l->pipefd2[1]);
		close (0);
		dup (l->pipefd2[0]);
		execl (path_name, name, param1, param2, param3, (char *) 0);
	}
	else {				// parent process
		l->fpin = fdopen (l->pipefd1[0], "r");
		l->fpout = fdopen (l->pipefd2[1], "w");
		close (l->pipefd1[1]); 
		close (l->pipefd2[0]); 
	}
}

void link_close (struct link_handle *l)
{
//	wait ((union wait*) 0);
	wait ((int *) 0);
	close (l->pipefd1[1]);
	close (l->pipefd2[0]);
	fclose (l->fpin);
	fclose (l->fpout);
	l->pid = 0;
}

int link_read (struct link_handle *l, char s[])
{
	int eof_flag;

	if (fgets (s, 100, l->fpin) == NULL)
		eof_flag = 1;		// linked-to process has terminated on its own
	else {
		s[strlen (s) - 1] = '\0';	// lose the newline character
		eof_flag = 0;
	}
	return (eof_flag);
}

int link_input_waiting (struct link_handle *l)
{
	int num;

	ioctl (l->pipefd1[0], FIONREAD, &num);	// see how many chars in buffer
	return num;
}

void link_write_char (struct link_handle *l, char c)
{
	fprintf (l->fpout, "%c", c);
	fflush (l->fpout);
}

void link_write (struct link_handle *l, char s[])
{
	fprintf (l->fpout, "%s\n", s);
	fflush (l->fpout);
}

void link_kill (struct link_handle *l)
{
	kill (l->pid, SIGKILL);
	link_close (l);
}
