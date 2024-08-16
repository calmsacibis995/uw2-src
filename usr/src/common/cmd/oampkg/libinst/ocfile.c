/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)oampkg:common/cmd/oampkg/libinst/ocfile.c	1.4.9.6"
#ident  "$Header: $"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <pkglocs.h>
#include <pfmt.h>

static	struct stat	buf;

static char
	contents[PATH_MAX],
	t_contents[PATH_MAX];

extern int	errno;
extern char	*prog;
extern int	warnflag;

extern int	creat(),
		close(),
		lockf(),
		unlink(),
		fsync(),
		setlvl();
extern time_t	time();
extern void	logerr(),
		progerr(),
		quit(),
		echo();
		
#define user_pub 4 /* the USER_PUBLIC level */

int
ocfile(mapfp, tmpfp)
FILE	**mapfp, **tmpfp;
{
	int	fd;

	*mapfp = *tmpfp = NULL;

	/* 
	 * We open the file for update because we don't want any other
	 * process either reading or writing the contents file while we're
	 * working with it.
	 */
	(void) sprintf(contents, "%s/contents", PKGADM);

	if((*mapfp = fopen(contents, "r")) == NULL) {
		if(errno == ENOENT) {
			fd = creat(contents, 0644);
			if(fd < 0) {
				progerr("uxpkgtools:64:unable to create contents file <%s>", 
					contents);
				logerr("uxpkgtools:65:(errno %d)", errno);
				return(-1);
			}
			echo("uxpkgtools:66:## Software contents file initialized");
			(void) close(fd);
			*mapfp = fopen(contents, "r");
		}
		if(*mapfp == NULL) {
			progerr("uxpkgtools:67:unable to open contents file <%s>", contents);
			logerr("uxpkgtools:65:(errno %d)", errno);
			return(-1);
		}
	}

	/*
	 * Retain the current attributes for the contents file to be used for
	 * resettting after we're done changing it.  This is done so the pkgchk
	 * command won't complain about the owner and uid associated with the
	 * contents files changing.
	 */
	(void) stat(contents, &buf);

	(void) sprintf(t_contents, "%s/t.contents", PKGADM);
	if((*tmpfp = fopen(t_contents, "w")) == NULL) {
		progerr("uxpkgtools:68:unable to open <%s> for writing", t_contents);
		logerr("uxpkgtools:65:(errno %d)", errno);
		(void) fclose(*tmpfp);
		*mapfp = NULL;
		return(-1);
	}

	if(lockf(fileno(*tmpfp), F_TLOCK, 0)) {
		progerr("uxpkgtools:69:unable to lock <%s> for modification", t_contents);
		logerr("uxpkgtools:65:(errno %d)", errno);
		(void) fclose(*mapfp);
		(void) fclose(*tmpfp);
		*tmpfp = NULL;
		*mapfp = NULL;
		return(-1);
	}
	return(0);
}

int
swapcfile(tmpfp, pkginst)
FILE	*tmpfp;
char	*pkginst;
{
	char	s_contents[PATH_MAX];
	time_t	clock;
	level_t	lvl;

	if(pkginst == NULL) {
		if(fclose(tmpfp)) {
			logerr("uxpkgtools:70:WARNING:unable to close <%s>", t_contents);
			logerr("uxpkgtools:65:(errno %d)", errno);
			warnflag++;
		}
		if(unlink(t_contents)) {
			logerr("uxpkgtools:70:WARNING:unable to close <%s>", t_contents);
			logerr("uxpkgtools:65:(errno %d)", errno);
			warnflag++;
		}
		return(0);
	}
		
	/* need to modify file */
	(void) time(&clock);
	(void) pfmt(tmpfp, MM_NOSTD, "uxpkgtools:71:# Last modified by %s for %s package\n# %s",
		prog, pkginst, ctime(&clock));
	if(fflush(tmpfp)) {
		progerr("uxpkgtools:72:unable to update contents file");
		logerr("uxpkgtools:73:fflush failed (errno %d)", errno);
		return(-1);
	}
	if(fsync(fileno(tmpfp))) {
		progerr("uxpkgtools:72:unable to update contents file");
		logerr("uxpkgtools:74:fsync failed (errno %d)", errno);
		return(-1);
	}
		
	(void) sprintf(s_contents, "%s/s.contents", PKGADM);
	if(rename(contents, s_contents)) {
		progerr("uxpkgtools:72:unable to update contents file");
		logerr("uxpkgtools:75:rename(%s, %s) failed (errno %d)",
			contents, s_contents, errno);
		return(-1);
	}
	if(rename(t_contents, contents)) {
		progerr("uxpkgtools:76:unable to establish contents file");
		logerr("uxpkgtools:75:rename(%s, %s) failed (errno %d)",
			t_contents, contents, errno);
		if(rename(s_contents, contents)) {
			progerr("uxpkgtools:77:attempt to restore <%s> failed", 
				contents);
			logerr("uxpkgtools:75:rename(%s, %s) failed (errno %d)",
				s_contents, contents, errno);
		}
		return(-1);
	}

	if(unlink(s_contents)) {
		logerr("uxpkgtools:78:WARNING:unable to unlink <%s>", s_contents);
		logerr("uxpkgtools:65:(errno %d)", errno);
		warnflag++;
	}

	/*
	 * If the temporary contents file is shorter than the original contents
	 * file, the extraneous entries will appear after the two lines which
	 * should denote the end of file.  This would give rise to "bad read from
	 * contents file" message on future installations.  To prevent this,
	 * truncate the extraneous entries now.
	 */
	if(ftruncate(fileno(tmpfp), (off_t)ftell(tmpfp))) {
		progerr("uxpkgtools:79:unable to truncate contents file");
		logerr("uxpkgtools:80:ftruncate failed (errno %d)", errno);
		return(-1);
	}
	if(fclose(tmpfp)) {
		progerr("uxpkgtools:72:unable to update contents file");
		logerr("uxpkgtools:81:fclose failed (errno %d)", errno);
		return(-1);
	}

	/* set level on contents */
	lvl = user_pub;
	(void) setlvl(contents, &lvl);

	/* Reset contents file attributes to original attributes */ 
	(void) chmod(contents, buf.st_mode);
	(void) chown(contents, buf.st_uid, buf.st_gid);

	return(0);
}

	

