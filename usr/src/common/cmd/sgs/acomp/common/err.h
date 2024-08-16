/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acomp:common/err.h	55.2"
/* err.h */

#ifdef	__STDC__
/* prototype forces strings into read-only space */
extern void cerror(const char *, ...);
extern void uerror(const char *, ...);
extern void werror(const char *, ...);
extern void ulerror(int, const char *, ...);
extern void wlerror(int, const char *, ...);
extern void wlferror(int, const char *, const char *, ...);
extern void yyerror(const char *);
extern int er_getline(void);
extern char *er_curname(void);
#ifndef NODBG
extern void dprintf(const char *, ...);
#endif
#else	/* def __STDC__ */
extern void cerror(), uerror(), werror(), ulerror(), wlerror(), wlferror(), 
	    yyerror();
extern int er_getline();
extern char *er_curname();
#ifndef NODBG
extern void dprintf();
#endif
#endif	/* def __STDC__ */

extern void er_filename();
extern void er_markline();

#ifndef LINT
#define	UERROR uerror
#define	WERROR werror
#define ULERROR ulerror
#define WLERROR wlerror
#define WLFERROR wlferror
#else
#define	UERROR luerror
#define	WERROR lwerror
#define ULERROR lulerror
#define WLERROR lwlerror
#endif

#define	DPRINTF dprintf


extern int nerrors;			/* number of errors so far */

#ifdef	NODBG
#define	DEBUG(cond, print)
#else
#define	DEBUG(cond, print) if (cond) DPRINTF print
#endif
