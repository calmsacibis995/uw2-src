/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/obj/intp.c	1.6"
#ident	"$Header: $"

#include <util/types.h>
#include <util/param.h>
#include <svc/errno.h>
#include <fs/vnode.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <io/uio.h>
#include <fs/pathname.h>
#include <proc/exec.h>
#include <mem/kmem.h>
#include <svc/systm.h>

#define	INTPSZ	256		/* max size of '#!' line allowed */

/*
 * The intpdata structure holds the interpreter name & argument data.
 */
struct intpdata {	
	char	*line1p;	/* points to dyn. alloc. buffer */
	int	intp_ssz;	/* size of compacted pathname & arg */
	char	*intp_name;	/* points to name part */
	char	*intp_arg;	/* points to arg part (optional) */
};

/*
 * STATIC int getintphead(struct intpdata *idatap, exhda_t *ehdp)
 *	Crack open a '#!' line.
 *
 * Calling/Exit State:
 *	No spinlocks can be held on entry, none are held on return.
 */
STATIC int
getintphead(struct intpdata *idatap, exhda_t *ehdp)
{
	register int error;
	register char *cp, *linep;
	int rdsz;
	int ssz = 0;

	ASSERT(KS_HOLD0LOCKS());

	/* Read the entire line and confirm that it starts with '#!'. */
	rdsz = (INTPSZ > ehdp->exhda_vnsize) ? ehdp->exhda_vnsize : INTPSZ;
	if ((error = exhd_read(ehdp, 0, rdsz, (void **)&idatap->line1p)) != 0)
		return (error);

	/*
	 * line1p points to a buffer allocated by exhd_read(), this
	 * buffer is released automatically by exhd_release().
	 */
	linep = idatap->line1p;

	/*
	 * Check magic number; this is not really necessary since
	 * this is how we got here in the first place.
	 */
	if (linep[0] != '#' || linep[1] != '!')
		return (ENOEXEC);

	/* Blank all white space and find the newline. */
	cp = &linep[2];
	linep += rdsz;
	for (; cp < linep && *cp != '\n'; cp++)
		if (*cp == '\t')
			*cp = ' ';
	if (cp >= linep)
		return (E2BIG);

	ASSERT(*cp == '\n');
	*cp = '\0';

	/*
	 * Locate the beginning and end of the interpreter name.
	 * In addition to the name, one additional argument may
	 * optionally be included here, to be prepended to the
	 * arguments provided on the command line.  Thus, for
	 * example, you can say
	 *
	 * 	#! /usr/bin/awk -f
	 */
	for (cp = &idatap->line1p[2]; *cp == ' '; cp++)
		;
	if (*cp == '\0')
		return (ENOEXEC);
	idatap->intp_name = cp;
	while (*cp && *cp != ' ') {
		ssz++;
		cp++;
	}
	ssz++;
	if (*cp == '\0')
		idatap->intp_arg = NULL;
	else {
		*cp++ = '\0';
		while (*cp == ' ')
			cp++;
		if (*cp == '\0')
			idatap->intp_arg = NULL;
		else {
			idatap->intp_arg = cp;
			while (*cp && *cp != ' ') {
				ssz++;
				cp++;
			}
			*cp = '\0';
			ssz++;
		}
	}
	idatap->intp_ssz = ssz;
	return (0);
}

/*
 * int intpexec(vnode_t *vp, struct uarg *args, int level, long *execsz,
 *		exhda_t *ehdp)
 *	Exec an intp file (magic number '#!').
 *
 * Calling/Exit State:
 *	No spin locks can be held on entry, no spin locks are held on
 *	return.
 */
int
intpexec(vnode_t *vp, struct uarg *args, int level, long *execsz,
	exhda_t *ehdp)
{
	vnode_t *nvp;
	int num, error = 0;
	struct intpdata idata;
	struct pathname intppn;


	ASSERT(KS_HOLD0LOCKS());

	if (level != 0) 	/* Can't recurse. */
		return (ENOEXEC);

	if ((error = getintphead(&idata, ehdp)) != 0)
		goto bad;

	/* Look up the vnode of the interpreter. */
	if (error = pn_get(idata.intp_name, UIO_SYSSPACE, &intppn))
		goto bad;

	if (error = lookuppn(&intppn, FOLLOW, NULLVPP, &nvp)) {
		pn_free(&intppn);
		goto bad;
	}
	pn_free(&intppn);

	num = 1;
	if (idata.intp_arg)
		num++;
	args->prefixc = num;
	args->prefixp = (vaddr_t)&idata.intp_name;
	args->prefixsize = idata.intp_ssz;
	
	/*
	 * For setid scripts, the filename is replaced with
	 * a filename of the form "/dev/fd/n", which when opened
	 * will yield a descriptor to the script file.  This closes
	 * a security hole where a setid script is exec'd, and then
	 * replaced with a totally different file, thus causing an
	 * interpreter which is running setid to interpret a different
	 * script.
	 * We do not open the file to be interpreted here, but simply
	 * record the vnode of the file.  When the process is known
	 * to be singly threaded, it calls intpopen() to open the file,
	 * obtain a file descriptor to the open file, and construct
	 * a filename of the form "/dev/fd/n".
	 * This must be done when the process is known to be singly
	 * threaded to prevent a race in a process which consists of
	 * multiple LWPs: another LWP in the process could close
	 * the file descriptor and open/dup/dup2 a new file which
	 * corresponds to the file descriptor (hence causing the
	 * interpreter to interpret a different script, just what
	 * we were trying to prevent in the first place!).
	 */
	if (args->setid)
		args->intpvp = vp;
	
	/* Exec new vnode (the interpreter). */
	error = gexec(&nvp, args, ++level, execsz);
	VN_RELE(nvp);

bad:
	return (error);
}

/*
 * int intpopen(vnode_t *vp, char *fname)
 *	Open the vnode given by 'vp', obtain a file descriptor
 *	which corresponds to the open file, and construct a name
 *	of the form "/dev/fd/n" which when opened corresponds
 *	to the vnode 'vp'.
 *
 * Calling/Exit State:
 *	To prevent races with other LWPs in the process, the process
 *	must be singly threaded when it calls this function.
 *	No spin locks can be held by the caller on entry, none are
 *	held on exit.
 *
 * Remarks:
 *	The caller must assure that 'fname' is large enough.
 */
int
intpopen(vnode_t *vp, char *fname)
{
	int error, fd;

	ASSERT(u.u_procp->p_nlwp == 1);
	ASSERT(KS_HOLD0LOCKS());

	strcpy(fname, "/dev/fd/");
	if (error = execopen(&vp, &fd))
		return (error);
	numtos(fd, &fname[8]);
	return (0);
}
