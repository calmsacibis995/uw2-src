/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:svc/name.c	1.11"
#ident	"$Header: $"

#include <util/param.h>
#include <util/types.h>
#include <acc/priv/privilege.h>
#include <util/sysmacros.h>
#include <svc/systm.h>
#include <proc/signal.h>
#include <proc/cred.h>
#include <proc/resource.h>
#include <proc/user.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <proc/proc.h>
#include <util/var.h>
#include <util/inline.h>
#include <io/uio.h>
#include <svc/utsname.h>
#include <fs/statvfs.h>
#include <proc/ucontext.h>
#include <proc/procset.h>
#include <proc/session.h>
#include <svc/time.h>
#include <svc/sysconfig.h>
#include <svc/systeminfo.h>
#include <proc/unistd.h>
#include <mem/kmem.h>
#include <acc/dac/acl.h>
#include <util/engine.h>
#include <proc/proc_hier.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/cmn_err.h>


STATIC char	*version = VERSION;	/* VERSION and RELEASE are defined at */
STATIC char	*release = RELEASE;	/* compile time, on the command line */

extern char	architecture[];		/* from name config file */
extern char	hw_provider[];		/* from name config file */
extern char	hw_serial[];		/* from name config file */
extern char     initfile[];             /* from name config file */
extern int	exec_ncargs;		/* from proc.cf/Space.c  */

/*
 *+ Reader/Writer spin lock which protects the writable member(s)
 *+ of the utsname structure, and the secure rpc domain (srpc_domain).
 *+ Currently, only utsname.nodename and utsname.sysname are writable,
 *+ all other fields of the utsname structure are constant once the
 *+ system is initialized.
 */
STATIC LKINFO_DECL(uname_lockinfo, "SU::uname_lock", 0);

STATIC rwlock_t uname_lock;

/*
 * void inituname(void)
 *	Initialize uname info.
 *
 * Calling/Exit State:
 *	One time initialization routine called during system
 *	startup.
 *
 * Remarks:
 *	The uname_lock is used to protect the writable fields
 *	utsname.nodename, utsname.sysname, and srpc_domain.
 */
void
inituname(void)
{
	/*
	 * Get the release and version of the system.
	 */
	if (release[0] != '\0') {
		strncpy(utsname.release, release, SYS_NMLN-1);
		utsname.release[SYS_NMLN-1] = '\0';
	}
	if (version[0] != '\0') {
		strncpy(utsname.version, version, SYS_NMLN-1);
		utsname.version[SYS_NMLN-1] = '\0';
	}

	RW_INIT(&uname_lock, PROC_HIER_BASE, PL1, &uname_lockinfo, KM_NOSLEEP);
}

/*
 * void printuname(const char *)
 *	Parameterize the supplied string with fields from the
 *	utsname structure (and related strings) and print it.
 *
 * Calling/Exit State:
 *	One time routine called during system startup
 *	to print the title[] strings.
 */

void
printuname(const char *fmt)
{
	char	*str;
	int	c;

	while ((c = *fmt++) != '\0') {
		if (c != '%') {
			cmn_err(CE_CONT, "%c", c);
			continue;
		}

		switch (c = *fmt++) {

		case 'a':
			str = architecture;	/* %a */
			break;

		case 'i':
			str = initfile;		/* %i */
			break;

		case 'm':
			str = utsname.machine;	/* %m */
			break;

		case 'n':
			str = utsname.nodename;	/* %n */
			break;

		case 'p':
			str = hw_provider;	/* %p */
			break;

		case 'r':
			str = utsname.release;	/* %r */
			break;

		case 's':
			str = utsname.sysname;	/* %s */
			break;

		case 'v':
			str = utsname.version;	/* %v */
			break;

		case '\0':
			--fmt;			/* %<NUL> */
			/* FALLTHROUGH */

		case '%':
			str = "%";		/* %% */
			break;

		default:
			cmn_err(CE_CONT, "%%%c", c);
			continue;
		}
		cmn_err(CE_CONT, "%s", str);
	}
	cmn_err(CE_CONT, "\n");
}

/* Enhanced Application Compatibility Support */
/*
** Check the code in "svc/sco.c" when making any implementation changes to
** avoid breaking the SCO-compatible equivalent of this function.
*/
/* End Enhanced Application Compatibility Support */

struct sysconfiga {
	int which;
};

/*
 * int sysconfig(struct sysconfiga *uap, rval_t *rvp)
 *	Undocumented _sysconfig(2) system call handler.
 *	Return various system configuration parameters.
 *
 * Calling/Exit State:
 *	No special locking issues here.  This function does not
 *	do much!
 */
int
sysconfig(struct sysconfiga *uap, rval_t *rvp)
{

	switch (uap->which) {

	case _CONFIG_CLK_TCK:
		rvp->r_val1 = HZ;
		break;

	case _CONFIG_NGROUPS:
		/* Maximum number of supplementary groups. */
		rvp->r_val1 = ngroups_max;
		break;

	case _CONFIG_OPEN_FILES:
		/* Maximum number of open files (soft limit). */
		rvp->r_val1 = u.u_rlimits->rl_limits[RLIMIT_NOFILE].rlim_cur;
		break;

	case _CONFIG_CHILD_MAX:
		/* Maximum number of processes per real uid. */
		rvp->r_val1 = v.v_maxup;
		break;

	case _CONFIG_POSIX_VER:
		rvp->r_val1 = _POSIX_VERSION;	/* current POSIX version */
		break;
	
	case _CONFIG_PAGESIZE:
		rvp->r_val1 = PAGESIZE;
		break;

	case _CONFIG_XOPEN_VER:
		rvp->r_val1 = _XOPEN_VERSION;	/* current XOPEN version */
		break;

	case _CONFIG_NACLS_MAX:
                rvp->r_val1 = acl_getmax();	/* for Enhanced Security */
                break;

	case _CONFIG_NPROC:
		/* Max # of processes system wide. */
		rvp->r_val1 = v.v_proc;
		break;

	case _CONFIG_NENGINE:			/* # engines in system */
                rvp->r_val1 = Nengine;
		break;

	case _CONFIG_NENGINE_ONLN:		/* # engines online now */
                rvp->r_val1 = nonline;
		break;

	case _CONFIG_ARG_MAX:
		rvp->r_val1 = exec_ncargs;	/* max length of exec args*/
		break;

	default:
		return EINVAL;
	}
	
	return 0;
}


struct uname {
	struct utsname *cbuf;
};

/*
 * int nuname(struct uname *uap, rval_t *rvp)
 *	New uname system call.  Supports larger fields.
 *
 * Calling/Exit State:
 *	No spin locks can be held by the caller.  This
 *	function can block (via copyout).
 */
/* ARGSUSED */
int
nuname(struct uname *uap, rval_t *rvp)
{
        struct utsname *buf = uap->cbuf;
	char name[SYS_NMLN];

	ASSERT(KS_HOLD0LOCKS());

	getutsname(utsname.sysname, name);
	if (copyout(name, buf->sysname, strlen(utsname.sysname) + 1))
		return EFAULT;

	getutsname(utsname.nodename, name);
	if (copyout(name, buf->nodename, strlen(name) + 1))
		return EFAULT;

	if (copyout(utsname.release, buf->release, strlen(utsname.release) + 1))
		return EFAULT;

	if (copyout(utsname.version, buf->version, strlen(utsname.version) + 1))
		return EFAULT;

	if (copyout(utsname.machine, buf->machine, strlen(utsname.machine) + 1))
		return EFAULT;

	rvp->r_val1 = 1;
	return 0;
}

struct systeminfoa {
	int command;
	char *buf;
	long count;
};

/*
 * STATIC int strout(char *str, struct systeminfoa *uap, rval_t *rvp)
 *	Service routine for the systeminfo() function below.
 *
 * Calling/Exit State:
 *	This function can block (via copyout, subyte), no locks can
 *	be held by the caller.
 */
STATIC int
strout(char *str, struct systeminfoa *uap, rval_t *rvp)
{
        int strcnt, getcnt;

	ASSERT(KS_HOLD0LOCKS());

	strcnt = strlen(str);
        getcnt = (strcnt >= uap->count) ? uap->count : strcnt + 1;

        if (copyout(str, uap->buf, getcnt))
                return EFAULT;

        if (strcnt >= uap->count && subyte(uap->buf + uap->count - 1, 0) < 0)
                return EFAULT;

        rvp->r_val1 = strcnt + 1;
        return 0;
}

/*
 * int systeminfo(struct systeminfoa *uap, rval_t *rvp)
 *	Sysinfo(2) system call handler.  Get and set system
 *	information strings.
 *
 * Calling/Exit State:
 *	This function can block (via copyout/copyin).  No spin
 *	locks can be held by the caller.
 */
/* ARGSUSED */
int
systeminfo(struct systeminfoa *uap, rval_t *rvp)
{
	int error;
	char name[SYS_NMLN];

	ASSERT(KS_HOLD0LOCKS());

	switch (uap->command) {

	case SI_SYSNAME:
		getutsname(utsname.sysname, name);
		error = strout(name, uap, rvp);
		break;

	case SI_HOSTNAME:
		getutsname(utsname.nodename, name);
		error = strout(name, uap, rvp);
		break;

	case SI_RELEASE:
		error = strout(utsname.release, uap, rvp);
		break;

	case SI_VERSION:
		error = strout(utsname.version, uap, rvp);
		break;

	case SI_MACHINE:
		error = strout(utsname.machine, uap, rvp);
		break;

	case SI_ARCHITECTURE:
		error = strout(architecture, uap, rvp);
		break;

	case SI_HW_SERIAL:
		error = strout(hw_serial, uap, rvp);
		break;

	case SI_INITTAB_NAME:
                error = strout(initfile, uap, rvp);
                break;

	case SI_HW_PROVIDER:
		error = strout(hw_provider, uap, rvp);
		break;

	case SI_SRPC_DOMAIN:
		getutsname(srpc_domain, name);
		error = strout(name, uap, rvp);
		break;

	case SI_SET_HOSTNAME:
	{
		size_t len;

		if (pm_denied(u.u_lwpp->l_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}

		if ((error = copyinstr(uap->buf, name, SYS_NMLN, &len)) != 0)
			break;

		/* 
		 * Must be non-NULL string and string must be less
		 * than SYS_NMLN chars.
		 */
		if (len < 2 || (len == SYS_NMLN && name[SYS_NMLN-1] != '\0')) {
			error = EINVAL;
			break;
		}

		setutsname(utsname.nodename, name);
		rvp->r_val1 = len;
		break;
	}

	case SI_SET_SRPC_DOMAIN:
	{
		size_t len;

		if (pm_denied(u.u_lwpp->l_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}
		if ((error = copyinstr(uap->buf, name, SYS_NMLN, &len)) != 0)
			break;
		/*
		 * If string passed in is longer than length
		 * allowed for domain name, fail.
		 */
		if (len == SYS_NMLN && name[SYS_NMLN-1] != '\0') {
			error = EINVAL;
			break;
		}

		/*
		 * Update the global srpc_domain variable.
		 */
		setutsname(srpc_domain, name);
		rvp->r_val1 = len;
		break;
	}

	default:
		error = EINVAL;
		break;
	}

	return error;
}

/*
 * void getutsname(char *utsnp, char *name)
 *	Return a consistent copy of an element of the utsname
 *	structure.
 *
 * Calling/Exit State:
 *	This function acquires the uname_lock in read mode to
 *	obtain a consistent snapshot of the utsname element.
 *	Callers	must be aware of lock hierarchy.  This function
 *	does not block.
 *
 * Remarks:
 *	The passed in character array 'name' must be large enough to
 *	contain utsname string.  A size of SYS_NMLN will always suffice.
 *	This function should be used by other kernel components which
 *	need to get a copy of an entry in the utsname structure.
 */
void
getutsname(char *utsnp, char *name)
{
	pl_t pl;

	pl = RW_RDLOCK(&uname_lock, PL1);
	strcpy(name, utsnp);
	RW_UNLOCK(&uname_lock, pl);
}

/*
 * void setutsname(char *utsnp, char *newname)
 *	Atomically update a member of the utsname structure.
 *	The address of the member to be updated is passed in
 *	via 'utsnp', the new name to be copied in is supplied
 *	via 'newname'.
 *
 * Calling/Exit State:
 *	This function acquires the uname_lock in write mode,
 *	the caller must be aware of lock hierarchy.
 *	
 * Remarks:
 *	The caller must assure that 'newname' is not larger
 *	than the corresponding entry in the utsname structure.
 */
void
setutsname(char *utsnp, char *newname)
{
	pl_t	pl;

	/*
	 * Copy the name to the passed in field of the utsname structure.
	 */
	pl = RW_WRLOCK(&uname_lock, PL1);
	strcpy(utsnp, newname);
	RW_UNLOCK(&uname_lock, pl);
}
