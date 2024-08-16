/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:getut.c	1.19.5.3"

/*	Routines to read and write the /etc/utmp file.			*/
/*									*/
#ifdef __STDC__
	#pragma weak endutent = _endutent
	#pragma weak getutent = _getutent
	#pragma weak getutid = _getutid
	#pragma weak getutline = _getutline
	#pragma weak getutmp = _getutmp
	#pragma weak getutmpx = _getutmpx
	#pragma weak makeut = _makeut
	#pragma weak modut = _modut
	#pragma weak pututline = _pututline
	#pragma weak setutent = _setutent
	#pragma weak synchutmp = _synchutmp
	#pragma weak updutfile = _updutfile
	#pragma weak updutxfile = _updutxfile
	#pragma weak updutmpx = _updutmpx
	#pragma weak updwtmp = _updwtmp
	#pragma weak utmpname = _utmpname
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
#include	<stdlib.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<utime.h>
#include        <limits.h>

#define IDLEN	4	/* length of id field in utmp */
#define SC_WILDC	0xff	/* wild char for utmp ids */
#define	MAXFILE	79	/* Maximum pathname length for "utmp" file */
#define MAXVAL  ((int)UCHAR_MAX)        /* max value for an id 'character' */
#define IPIPE	"/etc/.initpipe"	/* FIFO to send pids to init */

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

#ifdef ut_time
#undef ut_time
#endif

#ifdef	DEBUG
#undef	UTMP_FILE
#define	UTMP_FILE "utmp"
#undef	UTMPX_FILE
#define	UTMPX_FILE "utmpx"
#endif

#if __STDC__
extern int              updutmpx(const struct utmp *);
extern int              synchutmp(const char *, const char *);
extern int              updutfile(const char *, const char *);
extern int              updutxfile(const char *, const char *);
extern struct utmp *    makeut(struct utmp *);
extern struct utmp *    modut(struct utmp *);
extern int              idcmp(const char *, const char *);
extern int              allocid(char *, const char *);
static int              lockut(void);
static void             unlockut(void);
extern void             sendpid(int, pid_t);
void            _setlvl(const char *);
#else /* !__STDC__ */
extern int              updutmpx();
extern int              synchutmp();
extern int              updutfile();
extern int              updutxfile();
extern struct utmp *    makeut();
extern struct utmp *    modut();
extern int              idcmp();
extern int              allocid();
static int              lockut();
static void             unlockut();
extern void             sendpid();
void                    _setlvl();
#endif /* !__STDC__ */

static int fd = -1;	/* File descriptor for the utmp file. */
static int fd_u = -1;	/* File descriptor for the utmpx file. */
static const char *utmpfile = UTMP_FILE;	/* Name of the current */
static const char *utmpxfile = UTMPX_FILE;	/* "utmp" like file.   */

#ifdef ERRDEBUG
static long loc_utmp;	/* Where in "utmp" the current "ubuf" was found.*/
#endif

static struct utmp ubuf;	/* Copy of last entry read in. */


/* "getutent" gets the next entry in the utmp file.
 */

struct utmp *getutent()
{
	extern int fd;
	extern struct utmp ubuf;
	register char *u;
	register int i;
	int fdx;

/* If the "utmp" file is not open, attempt to open it for
 * reading.  If there is no file, attempt to create one.  If
 * both attempts fail, return NULL.  If the file exists, but
 * isn't readable and writeable, do not attempt to create.
 */

	if (fd < 0) {
		_setlvl(utmpfile);
		if ((fd = open(utmpfile, O_RDWR)) < 0) {

/* If the open failed for permissions, try opening it only for
 * reading.  All "pututline()" later will fail the writes.
 */
			if ((fd = open(utmpfile, O_RDONLY)) < 0)
				return(NULL);
		} else {
			if (access(utmpxfile, F_OK) < 0) {
				_setlvl(utmpxfile);
				if ((fdx = open(utmpxfile, O_RDWR)) < 0) {
                                	close(fd);
                                	fd = -1;
					return(NULL);
				}
				close(fdx);
			}

                	if ( lockut()  ||  synchutmp(utmpfile, utmpxfile) ) {
                        	close(fd);      /* releases lock if held */
                        	fd = -1;
                        	return(NULL);
                	} else {
                        	unlockut();
                	}
            	}
	}

/* Try to read in the next entry from the utmp file.  */
	if (read(fd,&ubuf,sizeof(ubuf)) != sizeof(ubuf)) {

/* Make sure ubuf is zeroed. */
		for (i=0,u=(char *)(&ubuf); i<sizeof(ubuf); i++) *u++ = '\0';
		return(NULL);
	}

	return(&ubuf);
}

/*	"getutid" finds the specified entry in the utmp file.  If	*/
/*	it can't find it, it returns NULL.				*/

struct utmp *getutid(entry)
register const struct utmp *entry;
{
	extern struct utmp ubuf;
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
	} while (getutent() != NULL);

/* Return NULL since the proper entry wasn't found. */
	return(NULL);
}

/* "getutline" searches the "utmp" file for a LOGIN_PROCESS or
 * USER_PROCESS with the same "line" as the specified "entry".
 */

struct utmp *getutline(entry)
register const struct utmp *entry;
{
	extern struct utmp ubuf;
	register struct utmp *cur;

/* Start by using the entry currently incore.  This prevents */
/* doing reads that aren't necessary. */
	cur = &ubuf;
	do {
/* If the current entry is the one we are interested in, return */
/* a pointer to it. */
		if (cur->ut_type != EMPTY && (cur->ut_type == LOGIN_PROCESS
		    || cur->ut_type == USER_PROCESS) && strncmp(&entry->ut_line[0],
		    &cur->ut_line[0],sizeof(cur->ut_line)) == 0) return(cur);
	} while ((cur = getutent()) != NULL);

/* Since entry wasn't found, return NULL. */
	return(NULL);
}

/*	"pututline" writes the structure sent into the utmp file.	*/
/*	If there is already an entry with the same id, then it is	*/
/*	overwritten, otherwise a new entry is made at the end of the	*/
/*	utmp file.							*/
/*
 * note that pututline locks the files and releases the lock before returning.
 * Hence; if the files were locked on entry to here, they will not be locked
 * on return to the caller.
 */

struct utmp *
pututline(entry)
const struct utmp *     entry;
{
        int                     fc;
        off_t                   eof;
        struct utmp *           answer;
        const struct utmp *     newp;
        struct utmp             tmpbuf;
 
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
 * if the utmp file is not yet open, call getutent to open it; if it is
 * open but the caller has modified our buffer, rewind by one entry and call
 * getutent to re-read into the buffer, so we can do proper checks.
 */
        if (fd < 0) {
                (void) getutent();
        }
        else if (entry == & ubuf) {
                (void) lseek(fd, -(off_t)sizeof(*newp), SEEK_CUR);
                (void) getutent();
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
 
/* Find the proper entry in the utmp file.  Start at the current */
/* location.  If it isn't found from here to the end of the */
/* file, then reset to the beginning of the file and try again. */
/* If it still isn't found, then write a new entry at the end of */
/* the file.  (Locking the files to make sure records appear in the */
/* same order in case of simultaneous extension via the getut and the */
/* getutx routines, and making sure the location is an integral number of */
/* utmp structures into the file incase the file is scribbled.) */
 
        if ( (answer = getutid(newp))  ==  NULL ) {
                setutent();
                answer = getutid(newp);
        }

        if (lockut()) {
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
 * parallel utmpx file. If this update fails, rewind the utmp pointer and
 * re-write the original version of the entry we have just overwritten.
 * If both writes succeed, copy the caller's data to our standard buffer as it
 * is now the 'current' entry.
 */
        if (write(fd, newp, sizeof(*newp)) != sizeof(*newp)) {
                answer = NULL;
        }
        else if (updutmpx(newp)) {
                (void) lseek(fd, -(off_t)sizeof(*newp), SEEK_CUR);
                (void) write(fd, & ubuf, sizeof(ubuf));
                answer = NULL;
        }
        else {
                ubuf = * newp;
                answer = & ubuf;
        }

        unlockut();

        return(answer);
}

/*	"setutent" just resets the utmp file back to the beginning.	*/

void
setutent()
{
	register char *ptr;
	register int i;
	extern int fd;
	extern struct utmp ubuf;

	if (fd != -1) lseek(fd,0L,0);

/* Zero the stored copy of the last entry read, since we are */
/* resetting to the beginning of the file. */

	for (i=0,ptr=(char*)&ubuf; i < sizeof(ubuf);i++) *ptr++ = '\0';
}

/*      "endutent" closes the utmp file and the parallel utmpx file.    */
/*      Note that this also releases any locks being held on them.      */
/*      Clear the buffer to show no current entry.			*/

void
endutent()
{
	extern int fd;
	extern struct utmp ubuf;
	register char *ptr;
	register int i;

	if (fd != -1) close(fd);
	fd = -1;
        if (fd_u != -1) close(fd_u);
        fd_u = -1;
	for (i=0,ptr= (char *)(&ubuf); i < sizeof(ubuf);i++) *ptr++ = '\0';
}

/*	"utmpname" allows the user to read a file other than the	*/
/*	normal "utmp" file.						*/
utmpname(newfile)
const char *newfile;
{
	static char *saveptr;
	static int savelen = 0;
	int len;

/* Determine if the new filename will fit.  If not, return 0. */
	if ((len = strlen(newfile)) >= MAXFILE) return (0);

	/* malloc enough space for utmp, utmpx, and null bytes */
	if (len > savelen)
	{
		if (saveptr)
			free(saveptr);
		if ((saveptr = malloc(2 * len + 3)) == 0)
			return (0);
		savelen = len;
	}

	/* copy in the new file name. */
	utmpfile = (const char *)saveptr;
	(void)strcpy(saveptr, newfile);
        utmpxfile = (const char *)saveptr + len + 1;
        (void)strcpy(saveptr + len + 1, newfile);
        strcat(saveptr + len + 1, "x");

/* Make sure everything is reset to the beginning state. */
	endutent();
	return(1);
}

/* "updutmpx" updates the utmpx file with a single record just written to
 * the utmp file. We presume that the two files are in sychrony, so just
 * calculate where in utmpx this record should be written (based on the
 * current utmp file pointer) and convert & write it.
 *
 * Return 0 for success, 1 for failure.
 */

int
updutmpx(entry)
const struct utmp *entry;
{
        int             fc;
        off_t           s_off;          /* offset into source file */
        off_t           t_off;          /* offset into target file */
        struct utmpx    t_buf;          /* new record for target file */

/*
 * Check if the utmpx file is open and open it if necessary.
 * Get the current file pointer for the utmp file.
 */
        if (fd_u < 0) {
                _setlvl(utmpxfile);
                if ((fd_u = open(utmpxfile, O_RDWR)) < 0 )
                        return 1;
        }
                                         
        if ((s_off = lseek(fd, (off_t)0, SEEK_CUR)) == -1)
                return 1;
 
 
/*
 * Generate a utmpx type record from the given utmp type record.
 * Calculate where to write the utmpx record - the utmp file pointer is
 * currently set to the record after the one we're dealing with.
 * Seek to the correct place in the utmpx file and write the record.
 */
        getutmpx(entry, &t_buf);
 
        t_off = ( (s_off / sizeof(*entry)) - 1 )  *  sizeof(t_buf);
 
        if (lseek(fd_u, t_off, SEEK_SET) == -1
            ||
            write(fd_u, &t_buf, sizeof(t_buf)) != sizeof(t_buf))
                return 1;
 
        return 0;
}

void
updwtmp(file, ut)
const char *file;
struct utmp *ut;
{
        char filex[MAXFILE+1];
        struct utmpx utx;
        struct flock flock;
        int fd, fdx;
 
        (void) strcpy(filex, file);
        (void) strcat(filex, "x");

        _setlvl(file);
        _setlvl(filex);
        fd = open(file, O_WRONLY | O_APPEND);
        fdx = open(filex, O_WRONLY | O_APPEND);

        if (fd < 0) {
                if (fdx < 0)
                        return;
                if ((fd = open(file, O_WRONLY|O_CREAT|O_APPEND, 0644)) < 0) {
                        close(fdx);
                        return;
                }
        } else if (fdx < 0) {
                if ((fdx = open(filex, O_WRONLY|O_CREAT|O_APPEND, 0644)) < 0) {
                        close(fd);
                        return;
                }
        }


        /* Both files exist, synch them */

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

        if (synchutmp(file, filex) == 0) {
                (void) write(fd, ut, sizeof(struct utmp));
                getutmpx(ut, &utx);
                (void) write(fdx, &utx, sizeof(struct utmpx));
        }

        close(fd);
        close(fdx);
}


/*
 * "getutmp" - convert a utmpx record to a utmp record.
 */
void
getutmp(utx, ut)
        const struct utmpx *utx;
        struct utmp  *ut;
{
        (void) strncpy(ut->ut_user, utx->ut_user, sizeof(ut->ut_user));
        (void) strncpy(ut->ut_line, utx->ut_line, sizeof(ut->ut_line));
	(void) memcpy(ut->ut_id, utx->ut_id, sizeof(utx->ut_id));
        ut->ut_pid = utx->ut_pid;
        ut->ut_type = utx->ut_type;
        ut->ut_exit = utx->ut_exit;
        ut->ut_time = utx->ut_tv.tv_sec;
}


/*
 * "getutmpx" - convert a utmp record to a utmpx record.
 */
void
getutmpx(ut, utx)
	const struct utmp *ut;
	struct utmpx *utx;
{
        (void) strncpy(utx->ut_user, ut->ut_user, sizeof(ut->ut_user));
	(void) memset(&utx->ut_user[sizeof(ut->ut_user)], '\0',
	    sizeof(utx->ut_user) - sizeof(ut->ut_user));
        (void) strncpy(utx->ut_line, ut->ut_line, sizeof(ut->ut_line));
	(void) memset(&utx->ut_line[sizeof(ut->ut_line)], '\0',
	    sizeof(utx->ut_line) - sizeof(ut->ut_line));
	(void) memcpy(utx->ut_id, ut->ut_id, sizeof(ut->ut_id));
        utx->ut_pid = ut->ut_pid;
        utx->ut_type = ut->ut_type;
        utx->ut_exit = ut->ut_exit;
        utx->ut_tv.tv_sec = ut->ut_time;
        utx->ut_tv.tv_usec = 0;
	utx->ut_session = 0;
	(void) memset(utx->pad, 0, sizeof(utx->pad));
	(void) memset(utx->ut_host, '\0', sizeof(utx->ut_host));
}


/* "synchutmp" make sure utmp and utmpx files are in synch.
 * Returns an error code if the files are not multiples
 * of their respective struct size. Updates the out of 
 * date file.
*/
synchutmp(utf, utxf)
const char *    utf;
const char *    utxf;
{
	struct stat stbuf, stxbuf;

	if (stat(utf, &stbuf) == 0 &&
				stat(utxf, &stxbuf) == 0) {
		/* Make sure file is a multiple of 'utmp'  entries long */
		if((stbuf.st_size % sizeof(struct utmp)) != 0 ||
		   (stxbuf.st_size % sizeof(struct utmpx)) != 0) {
			errno = EINVAL;
			return(1);
		}

                /*
                 * Synchronise the files if they contain a different number of
                 * records - assume that the larger one is correct. Note that
                 * locks are used to make extension of the two files atomic,
                 * and to stop multiple simultaneous sync calls.
                 */
                if ( (stbuf.st_size / sizeof(struct utmp))
                                >  (stxbuf.st_size / sizeof(struct utmpx)) )
                {
                        return(updutxfile(utf, utxf));
                }
                else if ( (stbuf.st_size / sizeof(struct utmp))
                                <  (stxbuf.st_size / sizeof(struct utmpx)) )
                {
                        return(updutfile(utf, utxf));
                }
				
		if (abs(stxbuf.st_mtime-stbuf.st_mtime) >= MOD_WIN) {
			/* files are out of sync */
			if (stxbuf.st_mtime > stbuf.st_mtime) 
				return(updutfile(utf, utxf));
			else 
				return(updutxfile(utf, utxf));
		}
		return(0);
	}
	return(1);
}



/* "updutfile" updates the utmp file using the contents of the
 * umptx file.
 */
updutfile(utf, utxf)
const char *    utf;
const char *    utxf;
{
	struct utmpx utx;
	struct utmp  ut;
	int fd1, fd2;

	if ((fd1 = open(utf, O_RDWR|O_TRUNC)) < 0)
		return(1);

	if ((fd2 = open(utxf, O_RDONLY)) < 0) {
		close(fd1);
		return(1);
	}

	while (read(fd2, &utx, sizeof(utx)) == sizeof(utx)) {
		getutmp(&utx, &ut);
		if (write(fd1, &ut, sizeof(ut)) != sizeof(ut)) {
			close(fd1);
			close(fd2);
			return(1);
		}
	}
	close(fd1);
	close(fd2);
	utime(utxf, NULL);
	return(0);
}


/* "updutxfile" updates the utmpx file using the contents of the 
 * utmp file. Tries to preserve the host information as much
 * as possible.
 */
updutxfile(utf, utxf)
const char *    utf;
const char *    utxf;
{
	struct utmp  ut;
	struct utmpx utx;
	int fd1, fd2;
	int n1, cnt=0;

	if ((fd1 = open(utf, O_RDONLY)) < 0)
		return(1);
	if ((fd2 = open(utxf, O_RDWR|O_CREAT)) < 0) {
		close(fd1);
		return(1);
	}

	/* As long as the entries match, copy the records from the
	 * utmpx file to keep the host information.
	 */
	while ((n1 = read(fd1, &ut, sizeof(ut))) == sizeof(ut)) {
		if (read(fd2, &utx, sizeof(utx)) != sizeof(utx)) 
			break;
                if (ut.ut_pid != (short)utx.ut_pid || ut.ut_type != utx.ut_type
		   || !memcmp(ut.ut_id, utx.ut_id, sizeof(ut.ut_id))
		   || ! memcmp(ut.ut_line, utx.ut_line, sizeof(ut.ut_line))) {
			getutmpx(&ut, &utx);
			lseek(fd2, -(long)sizeof(struct utmpx), 1);
			if (write(fd2, &utx, sizeof(utx)) != sizeof(utx)) {
				close(fd1);
				close(fd2);
				return(1);
			}
		}
		cnt += sizeof(struct utmpx); 
	}

	/* out of date file is shorter, copy from the up to date file
	 * to the new file.
	 */
	if (n1 > 0) {
		do {
			getutmpx(&ut, &utx);
			if (write(fd2, &utx, sizeof(utx)) != sizeof(utx)) {
				close(fd1);
				close(fd2);
				return(1);
			}
		} while ((n1 = read(fd1, &ut, sizeof(ut))) == sizeof(ut));
	} else {
		/* out of date file was longer, truncate it */
                (void) ftruncate(fd2, cnt);
	}

	close(fd1);
	close(fd2);
	utime(utf, NULL);
	return(0);
}

/*
 * makeut - create a utmp entry, recycling an id if a dead entry is found
 *      which matches the requested id, supporting wild cards in the request.
 *      Generate a new id if wildcards are specified and no id can be reused.
 *      Also inform init about the new entry.
 *      To avoid leaving a window between updating utmp and informing init,
 *      send the message to init as soon as it looks likely that a utmp
 *      entry will be written. If the calling process evaporates before utmp
 *      gets updated, then init will try to remove a non-existent entry; this
 *      doesn't matter.
 *
 *      args:   utmp - point to utmp structure to be created
 */
 
struct utmp *
makeut(utmp)
register struct utmp *utmp;
{
        register int i, j;              /* scratch variables */
        register struct utmp *utp;      /* utmp entry being examined */
        int wild;                       /* flag, true iff wild card char seen */        char saveid[IDLEN];             /* the last id we matched */
 
        for (i = 0, wild = 0; i < IDLEN  &&  wild == 0; i++) {
                if ((unsigned char) utmp->ut_id[i] == SC_WILDC) {
                        wild = 1;
                }        
        }
 
        if (wild == 1) {
/*
 * Record to be written contains 'wildcard' characters - we need to scan the
 * utmp file to resolve them. Lock the utmp file to prevent clashes.
 */
                if (lockut())
                        return(NULL);
/*
 * Initialise saveid to be full of the lowest sequential character which can
 * be used to replace a wildcard.
 */
                for (i = 0; i <= MAXVAL; ++i) {
                        if (isalnum(i)  &&  i != SC_WILDC)
                                break;
                }
                for (j = 0; j < IDLEN; ++j) {
                        saveid[j] = i;
                }
/*
 * Scan through the utmp file looking for an entry which has an id matching
 * ours (wildcards active), and which is dead. If we find one, we use its
 * id and overwrite it. While we scan, update saveid to contain the highest
 * id to date which matches our wildcard containing id.
 */
                setutent();
                while (utp = getutent()) {
                        if (idcmp(utmp->ut_id, utp->ut_id) == 0) {
                                if (utp->ut_type == DEAD_PROCESS) {
                                        break;
                                }
                                else {
                                        for (j = 0; j < IDLEN; ++j) {
                                                saveid[j] = utp->ut_id[j];
                                        }
                                }
                        }
                }
 
                if (utp) {
/*
 * We've found an available id in a dead slot. Update the id field in the
 * caller's structure, inform init, then write away this record.
 */
                        for (j = 0; j < IDLEN; ++j) {
                                utmp->ut_id[j] = utp->ut_id[j];
                        }
                        sendpid(ADDPID, (pid_t)utmp->ut_pid);
                        utp = pututline(utmp);
                }
                else {
/*               
 * No dead entries for re-use. Call allocid to generate the next valid id
 * greater than that in saveid (which we have set to the greatest to date).
 * Allocid will update the caller's id field if there are any valid ids left,
 * so if allocid succeeds, inform init and write away the record.
 */
                        if (allocid(utmp->ut_id, saveid)) {
                                utp = NULL;
                        }
                        else {
                                sendpid(ADDPID, (pid_t)utmp->ut_pid);
                                utp = pututline(utmp);
                        }
                }
/*
 * no need to unlock the files - it has either been done already during a
 * call to pututline, or it will be done shortly when we call endutent to
 * close the files.
 */
        }
        else {
/*
 * No wildcards - inform init and write away the record as given.
 */
                sendpid(ADDPID, (pid_t)utmp->ut_pid);
                utp = pututline(utmp);
        }
 
        if (utp) {
/*
 * We've succesfully updated the utmp file.
 * Add an equivalent entry to the wtmp file.
 */
                updwtmp(WTMP_FILE, utp);
        }
 
        endutent();                     /* close utmp files, clear locks */
 
        return(utp);
}


/*
 * modut - modify a utmp entry.	 Also notify init about new pids or
 *	old pids that it no longer needs to care about
 *
 *	args:	utmp - point to utmp structure to be created
 */


struct utmp *modut(utp)
register struct utmp *utp;
{
	register int i;				/* scratch variable
*/
	struct utmp utmp;			/* holding area */
	register struct utmp *ucp = &utmp;	/* and a pointer to
it */
	struct utmp *up;			/* "current" utmp entry being examined */

        for (i = 0; i < IDLEN; ++i) {
                if ((unsigned char) utp->ut_id[i] == SC_WILDC) {
                        endutent();
                        return((struct utmp *) NULL);
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
                while (up = getutent()) {
                        if (idcmp(ucp->ut_id, up->ut_id) == 0)
                                break;
                }
        }
        else {
                ucp = utp;
                up = NULL;
        }
 
        if (up == NULL) {
                setutent();
                while (up = getutent()) {
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
        up = pututline(ucp);
	if (ucp->ut_type == DEAD_PROCESS)
		sendpid(REMPID, (pid_t)ucp->ut_pid);
	if (up)
                updwtmp(WTMP_FILE, up);
	endutent();
	return(up);
}



/*
 * idcmp - compare two id strings, return 0 if same, non-zero if not *
 *	args:	s1 - first id string
 *		s2 - second id string
 */


int
idcmp(s1, s2)
register const char *s1;
register const char *s2;
{
        register int i;         /* scratch variable */

        for (i = 0; i < IDLEN; ++i) {
                if (*s1 != *s2  &&  (unsigned char)*s1 != SC_WILDC) {
                        return(-1);
                }
                s1++;
                s2++;
        }
        return(0);
}


/*
 * allocid - allocate an unused id for utmp, either by recycling a
 *	DEAD_PROCESS entry or creating a new one.  This routine only *	gets called if a wild card character was specified.
 *
 *	args:	srcid - pattern for new id
 *		saveid - last id matching pattern for a non-dead process
 */


int
allocid(srcid, saveid)
register char *srcid;           /* id with wildcards to be updated */
register const char *saveid;    /* last id allocated in this sequence */
{
        register int    new;
        register int    i;
        int             carry;
        char            copyid[IDLEN];

        carry = -1;                     /* pretend carry from 'last' wildcard */
        for (i = IDLEN-1; i >= 0; i--) {
                if ((unsigned char)srcid[i] == SC_WILDC) {      /* replace ? */
                        if (carry == -1) {                      /* allocate ? */                                carry = 0;
                                new = (unsigned char)saveid[i]; /* seed char */
                                do {
                                        if (new++ == MAXVAL) {
                                                carry = -1;
                                                new = 0;
                                        }
                                } while (!isalnum(new)  ||  new == SC_WILDC);
                                copyid[i] = new;
                        }
                        else {
                                copyid[i] = saveid[i];
                        }
                }
                else {
                        copyid[i] = srcid[i];
                }
        }

        if (carry == 0) {
                for (i = 0; i < IDLEN; ++i) {
                        srcid[i] = copyid[i];
                }
        }

        return (carry);
}


/*
 * lockut and unlockut - lock and unlock the utmp and utmpx files.
 *
 * the lock takes the form of an advisory mode lock on the utmp file starting
 * at the beginning of the file and continuing to the largest possible
 * file offset.
 *
 * it should be held while scanning the file to resolve id wildcards, and
 * when writing to the files.
 */

static int
lockut()
{
        struct flock    flock;

        if (fd < 0) {                   /* if utmp file not open */
                (void) getutent();      /* open it & update utmpx */
        }

        flock.l_type = F_WRLCK;
        flock.l_whence = SEEK_SET;
        flock.l_start = 0;
        flock.l_len = 0;

        if (fd < 0  ||  fcntl(fd, F_SETLKW, &flock) == -1) {
                return(-1);
        }
        return(0);
}

static void
unlockut()
{
        struct flock    flock;

        if (fd >= 0) {

                flock.l_type = F_UNLCK;
                flock.l_whence = SEEK_SET;
                flock.l_start = 0;
                flock.l_len = 0;

                (void) fcntl(fd, F_SETLK, &flock);
        }
}


/*
 * sendpid - send message to init to add or remove a pid from the
 *	"godchild" list
 *
 *	args:	cmd - ADDPID or REMPID
 *		pid - pid of "godchild"
 */


void
sendpid(cmd, pid)
int cmd;
pid_t pid;
{
	int pfd;		/* file desc. for init pipe */
	struct pidrec prec;	/* place for message to be built */

/*
 * if for some reason init didn't open .initpipe, open it read/write
 * here to avoid sending SIGPIPE to the calling process
 */

	pfd = open(IPIPE, O_RDWR);
	if (pfd < 0)
		return;
	prec.pd_pid = pid;
	prec.pd_type = cmd;
	(void) write(pfd, &prec, sizeof(struct pidrec));
	(void) close(pfd);
}


#ifdef  ERRDEBUG
#include        <stdio.h>

gdebug(format,arg1,arg2,arg3,arg4,arg5,arg6)
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
void
_setlvl(filep)
	const	char	*filep;
{
	static	int	_fd;
	static	level_t	f_lid = 1;	/* 1 = ``SYS_PUBLIC'' */
	struct	stat	f_buf;

	/*
	 * stat() the file to determine if it exists.  If not, create it.
	*/
	if (stat(filep, &f_buf) < 0) {
		_fd = creat(filep, 0644);
		(void) close(_fd);
	}
	/*
	 * the specified file did exist.
	*/
	else {
		/*
		 * if the file had a valid lid, return.
		*/
		if (f_buf.st_level) {
			return;
		}
	}
	/*
	 * if the routine made it to here, set the
	 * lid on the specified file to ``SYS_PUBLIC''.
	*/
	(void) lvlfile(filep, MAC_SET, &f_lid);

	return;
}
