/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwsetup:link.h	1.4"
// link.h

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

#ifndef LINK_H
#define LINK_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/filio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <Xm/Xm.h>

struct link_handle {		// holds all info relevant to one link
  int pipefd1[2],pipefd2[2];
  int pid;
  FILE *fpin,*fpout;
};

extern "C" {
	int kill (pid_t, int);
}

extern void link_open (struct link_handle *l, char path_name[], char name[], char param1[], char param2[], char param3[]);

extern void link_close (struct link_handle *l);

extern int link_read (struct link_handle *l, char s[]);

extern int link_input_waiting (struct link_handle *l);

extern void link_write_char (struct link_handle *l, char c);

extern void link_write (struct link_handle *l, char s[]);

extern void link_kill (struct link_handle *l);

#endif
