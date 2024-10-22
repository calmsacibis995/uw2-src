/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/libbrmeth.d/brio.c	1.4.5.2"
#ident  "$Header: brio.c 1.2 91/06/21 $"

#include	<sys/types.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<signal.h>
#include	<backup.h>
#include	<bktypes.h>
#include	<sys/stat.h>

extern int	brlog();
extern int	read();
extern int	write();

extern int	bklevels;

int
slow_open(name, flags, mode)
char	*name;
int	flags;
mode_t	mode;
{

	register int	k;
	int		fd = -1;

	for (k = 0; (k < 10) && (fd < 0); k++) {
		BEGIN_CRITICAL_REGION;

		fd = open(name, flags, mode);

		END_CRITICAL_REGION;

		if (fd < 0)
			sleep(5);
	}
	return(fd);
} /* slow_open()  */

int
safe_read(fd, buf, size)
int	fd;
char	*buf;
int	size;
{
	int	ret;

	errno = 0;

	while(((ret = read(fd, buf, (unsigned) size)) < 0) && (errno == EINTR))
		continue;

	return(ret);
} /* safe_read */

int
safe_write(fd, buf, size)
int	fd;
char	*buf;
int	size;
{
	int	ret;

	errno = 0;

	while(((ret = write(fd, buf, (unsigned) size)) < 0) && (errno == EINTR))
		continue;

	return(ret);
} /* safe_write */

int
safe_stat(path, st)
char		*path;
struct stat	*st;
{
	int	ret;

	errno = 0;

	while(((ret = lstat(path, st)) < 0) && (errno == EINTR))
		continue;
	return(ret);
} /* safe_stat */
