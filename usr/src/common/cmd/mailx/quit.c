/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:quit.c	1.10.3.6"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident "@(#)quit.c	1.20 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "rcv.h"

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Rcv -- receive mail rationally.
 *
 * Termination processing.
 */

static void		writeback ARGS((void));

/*
 * Save all of the undetermined messages at the top of "mbox"
 * Save all untouched messages back in the system mailbox.
 * Remove the system mailbox, if none saved there.
 */

void
quit()
{
	int mboxcount, p, modify, autohold, anystat, holdbit, nohold;
	FILE *ibuf, *obuf, *fbuf, *readstat;
	register struct message *mp;
	char *id;
	int appending;
	const char *mbox = Getf("MBOX");

	/*
	 * If we are read only, we can't do anything,
	 * so just return quickly.
	 */

	if (readonly)
		return;
	/*
	 * See if there any messages to save in mbox.  If no, we
	 * can save copying mbox to /tmp and back.
	 *
	 * Check also to see if any files need to be preserved.
	 * Delete all untouched messages to keep them out of mbox.
	 * If all the messages are to be preserved, just exit with
	 * a message.
	 *
	 * If the luser has sent mail to himself, refuse to do
	 * anything with the mailbox, unless mail locking works.
	 */

#ifndef CANLOCK
	if (selfsent) {
		pfmt(stdout, MM_NOSTD, ":324:You have new mail.\n");
		return;
	}
#endif

	/*
	 * Adjust the message flags in each message.
	 */

	anystat = 0;
	autohold = value("hold") != NOSTR;
	appending = value("append") != NOSTR;
	holdbit = autohold ? MPRESERVE : MBOX;
	nohold = MBOX|MSAVED|MDELETED|MPRESERVE;
	if (value("keepsave") != NOSTR)
		nohold &= ~MSAVED;
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW) {
			receipt(mp);
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & MSTATUS)
			anystat++;
		if ((mp->m_flag & MTOUCH) == 0)
			mp->m_flag |= MPRESERVE;
		if ((mp->m_flag & nohold) == 0)
			mp->m_flag |= holdbit;
	}
	modify = 0;
	if (Tflag != NOSTR) {
		if ((readstat = fopen(Tflag, "w")) == NULL)
			Tflag = NOSTR;
	}
	for (mboxcount = 0, p = 0, mp = &message[0]; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MBOX)
			mboxcount++;
		if (mp->m_flag & MPRESERVE)
			p++;
		if (mp->m_flag & MODIFY)
			modify++;
		if (Tflag != NOSTR && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			id = hfield("message-id", mp, addone);
			if (id != NOSTR)
				fprintf(readstat, "%s\n", id);
			else {
				id = hfield("article-id", mp, addone);
				if (id != NOSTR)
					fprintf(readstat, "%s\n", id);
			}
		}
	}
	if (Tflag != NOSTR)
		fclose(readstat);
	if (p == msgCount && !modify && !anystat) {
		if (p == 1)
			pfmt(stdout, MM_NOSTD, heldonemsg, mailname);
		else
			pfmt(stdout, MM_NOSTD, heldmsgs, p, mailname);
		return;
	}
	if (mboxcount == 0) {
		writeback();
		return;
	}

	/*
	 * Create another temporary file and copy user's mbox file
	 * therein.  If there is no mbox, copy nothing.
	 * If s/he has specified "append" don't copy the mailbox,
	 * just copy saveable entries at the end.
	 */

	if (!appending) {
		if ((obuf = fopen(tempQuit, "w")) == NULL) {
			pfmt(stderr, MM_ERROR, badopen,
				tempQuit, Strerror(errno));
			return;
		}
		if ((ibuf = fopen(tempQuit, "r")) == NULL) {
			pfmt(stderr, MM_ERROR, badopen,
				tempQuit, Strerror(errno));
			removefile(tempQuit);
			fclose(obuf);
			return;
		}
		removefile(tempQuit);
		if ((fbuf = fopen(mbox, "r")) != NULL) {
			copystream(fbuf, obuf);
			fclose(fbuf);
		}
		if (ferror(obuf)) {
			pfmt(stderr, MM_ERROR, badwrite,
				tempQuit, Strerror(errno));
			fclose(ibuf);
			fclose(obuf);
			return;
		}
		fclose(obuf);
		close(creat(mbox, MBOXPERM));
		if ((obuf = fopen(mbox, "w")) == NULL) {
			pfmt(stderr, MM_ERROR, badopen,
				mbox, Strerror(errno));
			fclose(ibuf);
			return;
		}
	} else {		/* we are appending */
		if ((obuf = fopen(mbox, "a")) == NULL) {
			pfmt(stderr, MM_ERROR, badopen,
				mbox, Strerror(errno));
			return;
		}
	}

	copyowner(mbox, Getf("HOME"));
	for (mp = &message[0]; mp < &message[msgCount]; mp++)
		if (mp->m_flag & MBOX)
			if (send(mp, obuf, 0, 0, 1) < 0) {
				pfmt(stderr, MM_ERROR, badwrite, 
					mbox, Strerror(errno));
				if (!appending)
					fclose(ibuf);
				fclose(obuf);
				return;
			}

	/*
	 * Copy the user's old mbox contents back
	 * to the end of the stuff we just saved.
	 * If we are appending, this is unnecessary.
	 */

	if (!appending) {
		rewind(ibuf);
		copystream(ibuf, obuf);
		fclose(ibuf);
		fflush(obuf);
	}
	if (ferror(obuf)) {
		pfmt(stderr, MM_ERROR, badwrite, mbox, Strerror(errno));
		fclose(obuf);
		return;
	}
	fclose(obuf);
	if (mboxcount == 1)
		pfmt(stdout, MM_NOSTD, ":325:Saved 1 message in %s\n",
			mbox);
	else
		pfmt(stdout, MM_NOSTD, ":326:Saved %d messages in %s\n",
			mboxcount, mbox);

	/*
	 * Now we are ready to copy back preserved files to
	 * the system mailbox, if any were requested.
	 */
	writeback();
}

/*
 * Copy the mode from one file to another.
 */
static int
copymode(newfile, oldfile)
const char *newfile, *oldfile;
{
	struct stat statb;
	if (stat(oldfile, &statb) == -1)
		return 0;
	if (chmod(newfile, statb.st_mode) == -1)
		return 0;
	if (chown(newfile, statb.st_uid, statb.st_gid) == -1)
		return 0;
	return 1;
}

/*
 * Copy the mode from one file to another.
 */
static int
copyowner(newfile, oldfile)
const char *newfile, *oldfile;
{
	struct stat statb;
	if (stat(oldfile, &statb) == -1)
		return 0;
	if (chown(newfile, statb.st_uid, statb.st_gid) == -1)
		return 0;
	return 1;
}

/*
 * Preserve all the appropriate messages back in the system
 * mailbox, and print a nice message indicated how many were
 * saved.  Incorporate any new mail that we found.
 */
static void
writeback()
{
	register struct message *mp;
	register int copybackcnt, success = 0, havenewmail = 0;
	struct stat st;
	FILE *mbuf = 0, *sbuf = 0;
	void (*fhup)(), (*fint)(), (*fquit)(), (*fpipe)();

	fhup = sigset(SIGHUP, SIG_IGN);
	fint = sigset(SIGINT, SIG_IGN);
	fquit = sigset(SIGQUIT, SIG_IGN);
	fpipe = sigset(SIGPIPE, SIG_IGN);

	lockmail();

	/* open :saved/user for writing */
	sprintf(tempResid, "%s/:saved/%s", maildir, lockname);
	(void) PRIV(sbuf = fopen(tempResid, "a"));
	if (sbuf == NULL) {
		pfmt(stderr, MM_ERROR, badopen, tempResid, Strerror(errno));
		goto die;
	}

	/* set the modes on the :saved/user file */
	if (!PRIV(copymode(tempResid, mailname))) {
		pfmt(stderr, MM_ERROR, badchmod, tempResid, Strerror(errno));
		goto die;
	}

	/* copy mail file to :saved */
	copybackcnt = 0;
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
		if ((mp->m_flag&MPRESERVE)||(mp->m_flag&MTOUCH)==0) {
			copybackcnt++;
			if (send(mp, sbuf, 0, 0, 1) < 0) {
				pfmt(stderr, MM_ERROR, badwrite, tempResid, Strerror(errno));
				goto die;
			}
		}
	}

	/* open mailfile for reading */
	(void) PRIV(mbuf = fopen(mailname, "r"));
	if (mbuf == NULL) {
		pfmt(stderr, MM_ERROR, badopen, mailname, Strerror(errno));
		goto die;
	}

	/* copy any new mail to end of :saved/user */
	fstat(fileno(mbuf), &st);
	if (st.st_size > mailsize) {
		havenewmail = 1;
		fseek(mbuf, mailsize, 0);
		if (!copystream(mbuf, sbuf)) {
			pfmt(stderr, MM_ERROR, badwrite, tempResid, Strerror(errno));
			goto die;
		}
	}
	fclose(mbuf);
	mbuf = 0;

	/* check our writes to make sure they worked */
	fflush(sbuf);
	if (ferror(sbuf)) {
		pfmt(stderr, MM_ERROR, badwrite, tempResid, Strerror(errno));
		goto die;
	}

	/* rename /var/mail/:saved/user to /var/mail/user */
	if (PRIV(rename(tempResid, mailname))) {
		pfmt(stderr, MM_ERROR, badrename, tempResid, mailname, Strerror(errno));
		goto die;
	}

	unlockmail();

	/* reset the timestamp and report success */
	/* Note: all success messages are done after the mailbox is unlocked */
	/* to prevent potential blockage in case of certain tty errors. */
	alter(mailname);
	if (havenewmail)
		pfmt(stdout, MM_NOSTD, newmailarrived);
	if (copybackcnt == 1)
		pfmt(stdout, MM_NOSTD, heldonemsg, mailname);
	else if (copybackcnt > 1)
		pfmt(stdout, MM_NOSTD, heldmsgs, copybackcnt, mailname);

	/* remove empty mailboxes */
	if ((fsize(sbuf) == 0) && (value("keep") == NOSTR)) {
		struct stat	statb;
		if (stat(mailname, &statb) >= 0)
			(void) PRIV(delempty(statb.st_mode, mailname));
	}
	fclose(sbuf);
	sbuf = 0;
	success = 1;

die:
	/* clean up from unsuccessful operations */
	if (mbuf)
		fclose(mbuf);

	if (sbuf) {	/* something bad happened with :saved */
		fclose(sbuf);
		(void) PRIV(removefile(tempResid));
	}

	if (!success)
		unlockmail();

	/* reset our signals */
	sigset(SIGHUP, fhup);
	sigset(SIGINT, fint);
	sigset(SIGQUIT, fquit);
	sigset(SIGPIPE, fpipe);
}

void
lockmail()
{
    (void) PRIV(maillock(lockname,10));
}

void
unlockmail()
{
    (void) PRIV(mailunlock());
}
