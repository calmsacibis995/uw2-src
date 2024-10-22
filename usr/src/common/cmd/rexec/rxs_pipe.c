/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rexec:rxs_pipe.c	1.2.2.5"
#ident  "$Header: $"

#include <sys/byteorder.h>
#include <sys/types.h>
#include <rx.h>
#include <unistd.h>
#include <string.h>

/* externally defined routines */

extern	void	logit();
extern	int	rxputm();


/* externally defined global variables */

extern	int	Net_fd;		/* network connection fd */


/* locally defined global variables */

int	Pipe_open;	/* error pipe open flag */
int	Pipe_fd;	/* parent side error pipe fd */
int	Childpipe_fd;	/* child side error pipe fd */



/*
 * makepipe()
 *
 * This routine creates the pipe to be used for passing stderr data
 *
 */

int
makepipe()
{
	int	errfdarr[2];	/* file descriptors for pipe() call */

	if (pipe(errfdarr) < 0)
		return(-1);

	Pipe_fd = errfdarr[0];
	Childpipe_fd = errfdarr[1];

	Pipe_open = 1;

	return(0);
}


/*
 * pipe_hup()
 *
 * This routine is called when a HUP occurs on the parent side of the error pipe
 */

void
pipe_hup()
{
	logit("pipe: HUP from pipe");
	(void) close(Pipe_fd);
	Pipe_open = 0;
}


/*
 * pipe_msg()
 *
 * This routine handles messages which come over the error pipe
 *
 */

void
pipe_msg()
{
	char	databuf[RX_MAXDATASZ];
	struct data_msg	data_msg;
	int	cc;

	logit("pipe: data from pipe");

	if ((cc = read(Pipe_fd, databuf, sizeof(databuf))) < 0) {
		logit("pipe: read failed");
		return;
	}

	data_msg.fd = htonl(2);		/* 2 is stderr fd */
	data_msg.len = htonl(cc);
	(void) memcpy(data_msg.buf, databuf, (unsigned int) cc);

	if (rxputm(Net_fd, RXM_DATA, (long) RX_DATA_MSG_SZ(cc),
		   (char *) &data_msg) < 0) {
		logit("pipe: rxputm failed");
	}
}
