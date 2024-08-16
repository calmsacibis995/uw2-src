/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:getutx.c	1.16"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*	Routines to read and write the /etc/utmpx file.			*/

#ifdef __STDC__
	#pragma weak getutxent = _getutxent
	#pragma weak getutxid = _getutxid
	#pragma weak getutxline = _getutxline
	#pragma weak makeutx = _makeutx
	#pragma weak modutx = _modutx
	#pragma weak pututxline = _pututxline
	#pragma weak setutxent = _setutxent
	#pragma weak endutxent = _endutxent
	#pragma weak utmpxname = _utmpxname
	#pragma weak updutmp = _updutmp
	#pragma weak updwtmpx = _updwtmpx
#endif

#define idcmp   _GU_idcmp
#define allocid _GU_allocid
#define sendpid _GU_sendpid
#define _setlvl _GU__setlvl
#include	"synonyms.h"
#include	<stdio.h>
#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<utmpx.h>
#include	<errno.h>
#include	<mac.h>
#include	<fcntl.h>
#include	<string.h>
#include	<unistd.h>
#include	<ctype.h>
#include        <stdlib.h>
#include        <limits.h>

#define IDLEN	4	/* length of id field in utmp */
#define SC_WILDC	0xff	/* wild char for utmp ids */
#define	MAXFILE	79	/* Maximum pathname length for "utmpx" file */

#define MAXVAL  ((int)UCHAR_MAX)        /* max value for an id 'character' */
# define IPIPE	"/etc/.initpipe"	/* FIFO to send pids to init */


/*
 * format of message sent to init
 */

struct	pidrec {
	int	pd_type;	/* command type */
	pid_t	pd_pid;		/* pid */
};

/*
 * pd_type's
 */

# define ADDPID 1	/* add a pid to "godchild" list */
# define REMPID 2	/* remove a pid to "godchild" list */


#ifdef	DEBUG
#undef	UTMPX_FILE
#define	UTMPX_FILE "utmpx"
#undef	UTMP_FILE
#define	UTMP_FILE "utmp"
#endif

extern void	_setutxent();
#if defined(__STDC__)
extern int              updutmp(const struct utmpx *);
extern int              synchutmp(const char *, const char *);
extern int              idcmp(const char *, const char *);
extern int              allocid(char *, const char *);
static int              lockutx(void);
static void             unlockutx(void);
extern void             sendpid(int, pid_t);
void            _setlvl(const char *);
#else /* defined(__STDC__) */
extern int              updutmp();
extern int              synchutmp();
extern int              idcmp();
extern int              allocid();
static int              lockutx();
static void             unlockutx();
extern void             sendpid();
void            _setlvl();
#endif /* defined(__STDC__) */

static int fd = -1;	/* File descriptor for the utmpx file. */
static int fd_u = -1;	/* File descriptor for the utmp file. */
static const char *utmpxfile = UTMPX_FILE;      /* Names of current "utmpx" */
static const char *utmpfile = UTMP_FILE;        /* and "utmp" like files.   */

#ifdef ERRDEBUG
static long loc_utmp;	/* Where in "utmpx" the current "ubuf" was found.*/
#endif

static struct utmpx ubuf;	/* Copy of last entry read in. */


/* "getutxent" gets the next entry in the utmpx file.
 */

struct utmpx *getutxent()
{
	extern int fd;
	extern struct utmpx ubuf;
	register char *u;
	register int i;

/* If the "utmpx" file is not open, attempt to open it for
 * reading.  If there is no file, attempt to create one.  If
 * both attempts fail, return NULL.  If the file exists, but
 * isn't readable and writeable, do not attempt to create.
 */

        if (fd < 0) {
		_setlvl(utmpxfile);
                if ((fd = open(utmpxfile, O_RDWR)) < 0) {

/* If the open failed for permissions, try opening it only for
 * reading.  All "pututxline()" later will fail the writes.
 */
                if ((fd = open(utmpxfile, O_RDONLY)) < 0)
                                return(NULL);
                goto read_from_file;
                }
                if (access(utmpfile, F_OK) < 0) {
			_setlvl(utmpfile);
                        if ((fd_u = open(utmpfile, O_RDWR)) < 0)  {
                                close(fd);
                                fd = -1;
                                return(NULL);
                        }
                } 
 
                if (lockutx() || synchutmp(utmpfile, utmpxfile) ) {
                        close(fd);
                        fd = -1;
                        close(fd_u);    /* releases lock if held */
                        fd_u = -1;
                        return(NULL);
                }
                else {
                        unlockutx();
                }
        }

/* Try to read in the next entry from the utmpx file.  */
        /*
         * Come here if we open file in O_RDONLY mode.
         */
read_from_file: ;
	if (read(fd,&ubuf,sizeof(ubuf)) != sizeof(ubuf)) {

/* Make sure ubuf is zeroed. */
		for (i=0,u=(char *)(&ubuf); i<sizeof(ubuf); i++) *u++ = '\0';
		return(NULL);
	}

	return(&ubuf);
}

/*	"getutxid" finds the specified entry in the utmpx file.  If	*/
/*	it can't find it, it returns NULL.				*/

struct utmpx *getutxid(entry)
const struct utmpx *entry;
{
	extern struct utmpx ubuf;
	register short type;

/* Start looking for entry.  Look in our current buffer before */
/* reading in new entries. */
	do {

/* If there is no entry in "ubuf", skip to the read. */
		if (ubuf.ut_type != EMPTY) {
			switch(entry->ut_type) {

/* Do not look for an entry if the user sent us an EMPTY entry. */
			case EMPTY:
				return(NULL);

/* For RUN_LVL, BOOT_TIME, OLD_TIME, and NEW_TIME entries, only */
/* the types have to match.  If they do, return the address of */
/* internal buffer. */
			case RUN_LVL:
			case BOOT_TIME:
			case OLD_TIME:
			case NEW_TIME:
				if (entry->ut_type == ubuf.ut_type) return(&ubuf);
				break;

/* For INIT_PROCESS, LOGIN_PROCESS, USER_PROCESS, and DEAD_PROCESS */
/* the type of the entry in "ubuf", must be one of the above and */
/* id's must match. */
			case INIT_PROCESS:
			case LOGIN_PROCESS:
			case USER_PROCESS:
			case DEAD_PROCESS:
				if (((type = ubuf.ut_type) == INIT_PROCESS
					|| type == LOGIN_PROCESS
					|| type == USER_PROCESS
					|| type == DEAD_PROCESS)
				    && ubuf.ut_id[0] == entry->ut_id[0]
				    && ubuf.ut_id[1] == entry->ut_id[1]
				    && ubuf.ut_id[2] == entry->ut_id[2]
				    && ubuf.ut_id[3] == entry->ut_id[3])
					return(&ubuf);
				break;

/* Do not search for illegal types of entry. */
			default:
				return(NULL);
			}
		}
	} while (getutxent() != NULL);

/* Return NULL since the proper entry wasn't found. */
	return(NULL);
}

/* "getutxline" searches the "utmpx" file for a LOGIN_PROCESS or
 * USER_PROCESS with the same "line" as the specified "entry".
 */

struct utmpx *getutxline(entry)
const struct utmpx *entry;
{
	extern struct utmpx ubuf;
	register struct utmpx *cur;

/* Start by using the entry currently incore.  This prevents */
/* doing reads that aren't necessary. */
	cur = &ubuf;
	do {
/* If the current entry is the one we are interested in, return */
/* a pointer to it. */
		if (cur->ut_type != EMPTY && (cur->ut_type == LOGIN_PROCESS
		    || cur->ut_type == USER_PROCESS) && strncmp(&entry->ut_line[0],
		    &cur->ut_line[0],sizeof(cur->ut_line)) == 0) return(cur);
	} while ((cur = getutxent()) != NULL);

/* Since entry wasn't found, return NULL. */
	return(NULL);
}

/*	"pututxline" writes the structure sent into the utmpx file.	*/
/*	If there is already an entry with the same id, then it is	*/
/*	overwritten, otherwise a new entry is made at the end of the	*/
/*	utmpx file.							*/

/*
 * note that pututxline locks the files and releases the lock before returning.
 * Hence; if the files were locked on entry to here, they will not be locked
 * on return to the caller.
 */
 
struct utmpx *
pututxline(entry)
const struct utmpx *    entry;
{
        int                     fc;
        off_t                   eof;
        struct utmpx *          answer;
        const struct utmpx *    newp;
        struct utmpx            tmpbuf;

        if (entry == (struct utmpx *)NULL)
                return((struct utmpx *)NULL);
/*
 * if caller has simply modified our ubuf and given it back to us, take a copy
 * of it; set up a pointer to the new entry, wherever it is.
 */
        if (entry == & ubuf) {
                tmpbuf = ubuf;
                newp = & tmpbuf;
        }
        else {
                newp = entry;
        }
/*
 * if the utmpx file is not yet open, call getutxent to open it; if it is
 * open but the caller has modified our buffer, rewind by one entry and call
 * getutxent to re-read into the buffer, so we can do proper checks.
 */
        if (fd < 0) {
                (void) getutxent();
        }
        else if (entry == & ubuf) {
                (void) lseek(fd, -(off_t)sizeof(*newp), SEEK_CUR);
                (void) getutxent();
        }

        if (
                fd < 0
                ||
                (fc=fcntl(fd, F_GETFL, NULL)) == -1
                ||
                (fc & O_RDWR) != O_RDWR
           ) {
                return(NULL);
        }

/* Find the proper entry in the utmpx file.  Start at the current */
/* location.  If it isn't found from here to the end of the */
/* file, then reset to the beginning of the file and try again. */
/* If it still isn't found, then write a new entry at the end of */
/* the file.  (Locking the files to make sure records appear in the */
/* same order in case of simultaneous extension via the getut and the */
/* getutx routines, and making sure the location is an integral number of */
/* utmpx structures into the file incase the file is scribbled.) */
 
        if ( (answer = getutxid(newp))  ==  NULL ) {
                setutxent();
                answer = getutxid(newp);
        }
 
        if (lockutx()) {
                return(NULL);
        }
 
        if (answer == NULL) {
                eof = lseek(fd, (off_t)0, SEEK_END);
                if ((eof %= sizeof(*newp)) != 0) {
                        eof = sizeof(*newp) - eof;
                        (void) lseek(fd, eof, SEEK_CUR);
                }
        }
        else {
                (void) lseek(fd, -(off_t)sizeof(*newp), SEEK_CUR);
        }
/*
 * write out the caller's data. If the write succeeds, then update the
 * parallel utmp file. If this update fails, rewind the utmpx pointer and
 * re-write the original version of the entry we have just overwritten.
 * If both writes succeed, copy the caller's data to our standard buffer as it
 * is now the 'current' entry.
 */
        if (write(fd, newp, sizeof(*newp)) != sizeof(*newp)) {
                answer = NULL;
        }
        else if (updutmp(newp)) {
                (void) lseek(fd, -(off_t)sizeof(*newp), SEEK_CUR);
                (void) write(fd, & ubuf, sizeof(ubuf));
                answer = NULL;
        }
        else {
                ubuf = * newp;
                answer = & ubuf;
        }
 
        unlockutx();
 
        return(answer);
}

/*	"setutxent" just resets the utmpx file back to the beginning.	*/

void
setutxent()
{
	register char *ptr;
	register int i;
	extern int fd;
	extern struct utmpx ubuf;

	if (fd != -1) lseek(fd,0L,0);

/* Zero the stored copy of the last entry read, since we are */
/* resetting to the beginning of the file. */

	for (i=0,ptr=(char*)&ubuf; i < sizeof(ubuf);i++) *ptr++ = '\0';
}

/*      "endutxent" closes the utmpx file and the parallel utmp file.   */
/*      Note that this also releases any locks being held on them.      */
/*      Clear the buffer to show no current entry.                      */

void
endutxent()
{
	extern int fd;
	extern struct utmpx ubuf;
	register char *ptr;
	register int i;

	if (fd != -1) close(fd);
	fd = -1;
        if (fd_u != -1) close(fd_u);
        fd_u = -1;
	for (i=0,ptr= (char *)(&ubuf); i < sizeof(ubuf);i++) *ptr++ = '\0';
}

/*	"utmpxname" allows the user to read a file other than the	*/
/*	normal "utmpx" file.						*/


/*      use malloc() implementation to keep similar to getut.c, and     */
/*      since it gives a better use of resources with shared libraries. */

int
utmpxname(newfile)
const char *newfile;
{
        static char *saveptr;
        static int savelen = 0;
        int len;

/* Determine if the new filename will fit.  If not, return 0. */
        if ((len = strlen(newfile)) > MAXFILE) return (0);
/* The name of the utmpx file has to end with 'x' */
        if (newfile[len-1] != 'x') return(0);

        /* malloc enough space for utmp, utmpx, and null bytes */
        if (len > savelen)
        {
                if (saveptr)
                        free(saveptr);
                if ((saveptr = malloc(2 * len + 1)) == 0)
                        return (0);
                savelen = len;
        }

        /* copy in the new file name. */
        utmpfile = (const char *)saveptr;
        (void)strcpy(saveptr, newfile);
        saveptr[len-1] = '\0';                          /* knock off the 'x' */
        utmpxfile = (const char *)saveptr + len;
        (void)strcpy(saveptr + len, newfile);

/* Make sure everything is reset to the beginning state. */
        endutxent();
        return(1);
}

/* "updutmp" updates the utmp file, uses same algorithm as 
 * pututxline so that the records end up in the same spot.
 */
/* "updutmp" updates the utmp file with a single record just written to
 * the utmpx file. We presume that the two files are in sychrony, so just
 * calculate where in utmp this record should be written (based on the
 * current utmpx file pointer) and convert & write it.
 *
 * Return 0 for success, 1 for failure.
 */
 
int
updutmp(entry)
const struct utmpx *entry;
{
        int             fc;
        int             retval;
        off_t           s_off;          /* offset into source file */
        off_t           t_off;          /* offset into target file */
        struct utmp     t_buf;          /* new record for target file */
 
/*
 * Check if the utmp file is open and open it if necessary.
 * Get the current file pointer for the utmpx file.
 */
        if (fd_u < 0) {
                _setlvl(utmpfile);
                if ((fd_u = open(utmpfile, O_RDWR)) < 0 )
                        return 1;
        }
 
        if ((s_off = lseek(fd, (off_t)0, SEEK_CUR)) == -1)
                return 1;
 
/*
 * Generate a utmp type record from the given utmpx type record.
 * Calculate where to write the utmp record - the utmpx file pointer is
 * currently set to the record after the one we're dealing with.
 * Seek to the correct place in the utmp file and write the record.
 */
                getutmp(entry, &t_buf);
 
                t_off = ( (s_off / sizeof(*entry)) - 1 )  *  sizeof(t_buf);
 
                if (
                        lseek(fd_u, t_off, SEEK_SET) == -1
                        ||
                        write(fd_u, &t_buf, sizeof(t_buf)) != sizeof(t_buf)
                   ) {
                        retval = 1;
                }
                else {
                        retval = 0;
                }
 
        return(retval);
}


/*
 * If one of wtmp and wtmpx files exist, create the other, and the record.
 * If they both exist add the record.
 */
void
updwtmpx(filex, utx)
	const char *filex;
	struct utmpx *utx;
{
	char file[MAXFILE+1];
	struct utmp ut;
        struct flock flock;
	int fd, fdx;

	(void) strcpy(file, filex);
	file[strlen(filex) - 1] = '\0';

	_setlvl(file);
	_setlvl(filex);
	fd = open(file, O_WRONLY | O_APPEND);
	fdx = open(filex, O_WRONLY | O_APPEND);

        if (fd < 0) {
                if (fdx < 0)
                        return;
                close(fdx);
                return;
        } else if (fdx < 0) {
                close(fd);
                return;
        }
 
 
        /* Lock the wtmp file to prevent simultaneous syncs - they probably */
        /* do no harm, but are inefficient. Also makes the writes atomic to */
        /* stop them triggering a sync if another process is executing this */
        /* code just behind us. Since simultaneous synchs are harmless, don't */        /* worry if the lock request fails. Note that if a sync is done, the */
        /* lock will be released by the close at the end of the sync, and */
        /* thus won't be held across the two writes. The sync will have */
        /* updated both files; the chance of the time between the sync and */
        /* the first write being enough to trigger a second sync is */
        /* vanishingly small. */
 
        flock.l_type = F_WRLCK;
        flock.l_whence = SEEK_SET;
        flock.l_start = 0;
        flock.l_len = 0;
 
        (void) fcntl(fd, F_SETLKW, &flock);


	/* Both files exist, synch them */
	if (synchutmp(file, filex))
		goto done;

        /* both files are opened in append mode, so no need to seek to end */
	getutmp(utx, &ut);
	write(fd, &ut, sizeof(struct utmp));
	write(fdx, utx, sizeof(struct utmpx));

done:
	close(fd);
	close(fdx);
}


/*
 * makeutx - create a utmpx entry, recycling an id if a dead entry is found
 *      which matches the requested id, supporting wild cards in the request.
 *      Also inform init about the new entry.
 *      To avoid leaving a window between updating utmpx and informing init,
 *      send the message to init as soon as it looks likely that a utmpx
 *      entry will be written. If the calling process evaporates before utmpx
 *      gets updated, then init will try to remove a non-existent entry; this
 *      is harmless.
 *
 *      args:   utmpx - point to utmpx structure to be created
 */


struct utmpx *makeutx(utmp)
register struct utmpx *utmp;
{
 	register int i;			/* scratch variable */
	register struct utmpx *utp;	/* "current" utmpx entry being examined */
	int wild;			/* flag, true iff wild card
char seen */
	unsigned char saveid[IDLEN];	/* the last id we matched that was
                                           NOT a dead proc */

	wild = 0;
	for (i = 0; i < IDLEN; i++) {
		if ((unsigned char) utmp->ut_id[i] == SC_WILDC) {
			wild = 1;
			break;
		}
	}

	if (wild) {

/*
 * try to lock the utmpx and utmp files, only needed if we're doing
 * wildcard matching
 */

		if (lockutx()) {
			return((struct utmpx *) NULL);
		}

		setutxent();
		/* find the first alphanumeric character */
		for (i = 0; i < MAXVAL; ++i) {
			if (isalnum(i))
                                break;
		}
		(void) memset(saveid, i, IDLEN);
		while (utp = getutxent()) {
			if (idcmp(utmp->ut_id, utp->ut_id)) {
                                continue;
			}
			else {
                                if (utp->ut_type == DEAD_PROCESS) {
                                        break;
                                }
                                else {
                                        (void) memcpy(saveid, utp->ut_id, IDLEN);
                                }
			}
		}
		if (utp) {

/*
 * found an unused entry, reuse it
 */

			(void) memcpy((char *)(utmp->ut_id), utp->ut_id, IDLEN);
                        sendpid(ADDPID, (pid_t)utmp->ut_pid);
			utp = pututxline(utmp);
			if (utp) 
                                updwtmpx(WTMPX_FILE, utp);
			endutxent();
			return(utp);
		}
		else {

/*
 * nothing available, try to allocate an id
 */

                        if (allocid(utmp->ut_id, (char *) saveid)) {
                                endutxent();
                                return((struct utmpx *) NULL);
                        }
                        else {
                                sendpid(ADDPID, (pid_t)utmp->ut_pid);
                              	utp = pututxline(utmp);
                                if (utp) 
                                        updwtmpx(WTMPX_FILE, utp);
                                endutxent();
                                return(utp);
                        }
		}
	}
        else {
                sendpid(ADDPID, (pid_t)utmp->ut_pid);
                utp = pututxline(utmp);
                if (utp)
                        updwtmpx(WTMPX_FILE, utp);
                endutxent();
		return(utp);
	}
}


/*
 * modutx - modify a utmpx entry.  Also notify init about new pids or
 *	old pids that it no longer needs to care about
 *
 *	args:	utp- point to utmpx structure to be created
 */

struct utmpx *modutx(utp)
const struct utmpx *utp;
{
	register int i;				/* scratch variable
*/
	struct utmpx utmp;			/* holding area */
	register struct utmpx *ucp = &utmp;	/* and a pointer to
it */
	struct utmpx *up;			/* "current" utmpx entry being examined */

	for (i = 0; i < IDLEN; ++i) {
                if ((unsigned char) utp->ut_id[i] == SC_WILDC) {
                        endutxent();
                        return((struct utmpx *) NULL);
                }
	}
/*
 * If caller has simply modified our ubuf and given it back to us, take a copy
 * of it. In this case, it's likely that we are positioned just past the entry
 * we want, so lseek back to it and try.
 */
        if (utp == &ubuf) {
                utmp = *utp;
                (void) lseek(fd, -(off_t)sizeof(*ucp), SEEK_CUR);
                while (up = getutxent()) {
                        if (idcmp(ucp->ut_id, up->ut_id) == 0)
                                break;
                }
        }
        else {
                ucp = (struct utmpx *) utp;
                up = NULL;
        }
 
        if (up == NULL) {
                setutxent();
                while (up = getutxent()) {
                        if (idcmp(ucp->ut_id, up->ut_id) == 0)
                                break;
                }
        }
        if (up != NULL) {
                /* only get here if ids are the same, i.e. found right entry */
                if (ucp->ut_pid != up->ut_pid) {
                        sendpid(REMPID, (pid_t)up->ut_pid);
                        sendpid(ADDPID, (pid_t)ucp->ut_pid);
                }
        }
 
	up = pututxline(ucp);
	if (ucp->ut_type == DEAD_PROCESS)
		sendpid(REMPID, (pid_t)ucp->ut_pid);
	if (up)
		updwtmpx(WTMPX_FILE, up);
	endutxent();
	return(up);
}


/*
 * idcmp - compare two id strings, return 0 if same, non-zero if not *
 *	args:	s1 - first id string
 *		s2 - second id string
 */


/*
 *      Share the routines idcmp() and allocid() with the utmp handling code,
 *      using the functions in getut.c
 */
/*
 * lockutx - lock utmpx and utmp files
 */

/*
 * lockutx and unlockutx - lock and unlock the utmp and utmpx files.
 *
 * the lock takes the form of an advisory mode lock on the utmp file starting
 * at the beginning of the file and continuing to the largest possible
 * file offset.
 *
 * it should be held while scanning the file to resolve id wildcards, and
 * when writing to the files.
 */

static int
lockutx()
{
        struct flock    flock;

        if (fd < 0) {                   /* if utmpx file not open */
                (void) getutxent();     /* open utmpx, create & sync utmp */
        }

        flock.l_type = F_WRLCK;
        flock.l_whence = SEEK_SET;
        flock.l_start = 0;
        flock.l_len = 0;

        if (
                fd < 0                  /* if utmpx open failed, */
                ||                      /* or can't open utmp, */
                fd_u < 0  &&  (fd_u = open(utmpfile, O_RDWR)) < 0
                ||                      /* or can't lock utmp */
                fcntl(fd_u, F_SETLKW, &flock) == -1
           ) {
                return(-1);
        }
        return(0);
}        
 
static void
unlockutx()
{
        struct flock    flock;
 
        if (fd_u >= 0) {
 
                flock.l_type = F_UNLCK;
                flock.l_whence = SEEK_SET;
                flock.l_start = 0;
                flock.l_len = 0;
 
                (void) fcntl(fd_u, F_SETLK, &flock);
        }
}


/*
 * sendpid - send message to init to add or remove a pid from the
 *	"godchild" list
 *
 *	args:	cmd - ADDPID or REMPID
 *		pid - pid of "godchild"
 */


/*
 *      Routine sendpid() is shared with
 *      the utmp handling routines in getut.c
 */


#ifdef  ERRDEBUG
#include        <stdio.h>

gxdebug(format,arg1,arg2,arg3,arg4,arg5,arg6)
char *format;
int arg1,arg2,arg3,arg4,arg5,arg6;
{
        register FILE *fp;
        register int errnum;

        if ((fp = fopen("/etc/dbg.getut","a+")) == NULL) return;
        fprintf(fp,format,arg1,arg2,arg3,arg4,arg5,arg6);
        fclose(fp);
}
#endif


/*
 * Procedure:	_setlvl
 *
 * Notes:	sets the proper lid on the specified file
 *		if all conditions are satisfied.
*/
/*
 *      Routine _setlevel() is shared with
 *      the utmp handling routines in getut.c
 */
