/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef	_PRIV_H
#define	_PRIV_H

#ident	"@(#)head.usr:priv.h	1.1.7.5"
#ident  "$Header: priv.h 1.3 91/06/21 $"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************
 *
 * Header file used by the user-level privilege commands.
 * Contains several macros used by user-level programs.  The
 * external (user-level) privilege representation consists of:
 *
 *             type | privilege
 *
 * and is defined in <sys/privilege.h>.
 *
 * Also contained in <sys/privilege.h> are the definitions
 * for the privilege position names used by user-level macros
 * pm_work(), pm_max(), pm_fixed(), and pm_inher() below.
 *
 * The syntax for privileges are as follows:
 *
 *                         <pname> ::= <A-Z><0-9>
 *              privilege position ::= P_<pname>
 *        string name of privilege ::= lowercase(<pname>)
 *
 * It also contains macro definitions for the command arguments
 * to the filepriv(), procpriv(), procprivl(), and procprivc()
 * calls in addition to the typedef for the user-level definition
 * of a privilege type and privilege set.
 *
 *****************************************************************/

#include	<sys/privilege.h>

/*****************************************************************
 *
 * The following macros are used to specify which privilege sets
 * are updated with the specified privilege.
 *
 *****************************************************************/

#define	pm_work(p)	(priv_t)(((p) & PS_TYPE) ? -1 : ((p) | PS_WKG))
#define	pm_max(p)	(priv_t)(((p) & PS_TYPE) ? -1 : ((p) | PS_MAX))
#define	pm_fixed(p)	(priv_t)(((p) & PS_TYPE) ? -1 : ((p) | PS_FIX))
#define	pm_inher(p)	(priv_t)(((p) & PS_TYPE) ? -1 : ((p) | PS_INH))

/*****************************************************************
 *								  
 * The following macros are used to simplify the procprivl(3C) and
 * the procprivc(3C) calls for setting/clearing process privileges.
 *
 *****************************************************************/

#define	OWNER_W		pm_work(P_OWNER)
#define	AUDIT_W		pm_work(P_AUDIT)
#define	COMPAT_W	pm_work(P_COMPAT)
#define	DACREAD_W	pm_work(P_DACREAD)
#define	DACWRITE_W	pm_work(P_DACWRITE)
#define	DEV_W		pm_work(P_DEV)
#define	FILESYS_W	pm_work(P_FILESYS)
#define	MACREAD_W	pm_work(P_MACREAD)
#define	MACWRITE_W	pm_work(P_MACWRITE)
#define	MOUNT_W		pm_work(P_MOUNT)
#define	MULTIDIR_W	pm_work(P_MULTIDIR)
#define	SETFLEVEL_W	pm_work(P_SETFLEVEL)
#define	SETPLEVEL_W	pm_work(P_SETPLEVEL)
#define	SETSPRIV_W	pm_work(P_SETSPRIV)
#define	SETUID_W	pm_work(P_SETUID)
#define	SYSOPS_W	pm_work(P_SYSOPS)
#define	SETUPRIV_W	pm_work(P_SETUPRIV)
#define	DRIVER_W	pm_work(P_DRIVER)
#define	RTIME_W		pm_work(P_RTIME)
#define	MACUPGRADE_W	pm_work(P_MACUPGRADE)
#define	FSYSRANGE_W	pm_work(P_FSYSRANGE)
#define	AUDITWR_W	pm_work(P_AUDITWR)
#define	TSHAR_W		pm_work(P_TSHAR)
#define	PLOCK_W		pm_work(P_PLOCK)
#define	ALLPRIVS_W	pm_work(P_ALLPRIVS)

#define	READ_W		DACREAD_W,MACREAD_W
#define	WRITE_W		DACWRITE_W,MACWRITE_W
#define	ACCESS_W	READ_W,WRITE_W
#define	PRIVS_W		ACCESS_W,SETSPRIV_W,FSYSRANGE_W

/*****************************************************************
 *
 * The following are the definitions for the privilege functions
 *
 *****************************************************************/

#if defined(__STDC__)

extern	int	filepriv(const char *, int, priv_t *, int);
extern	int	procpriv(int, priv_t *, int);
extern	int	procprivl(int, ...);
extern	int	procprivc(int, ...);

#else

extern	int	filepriv();
extern	int	procpriv();
extern	int	procprivl();
extern	int	procprivc();

#endif	/* __STDC __ */

#ifdef __cplusplus
}
#endif

#endif	/* _PRIV_H */
