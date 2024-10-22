/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)truss:i386/cmd/truss/codes.h	1.1.1.2"
#ident	"$Header: codes.h 1.1 91/07/09 $"

#include <sys/sysi86.h>
static CONST struct ioc {
	unsigned int code;
	CONST char * name;
} ioc[] = {
	{ TIOCGETP	, "TIOCGETP"	},
	{ TIOCSETP	, "TIOCSETP"	},
#ifdef EI_RESET
	{ EI_RESET	, "EI_RESET"	},
	{ EI_LOAD	, "EI_LOAD"	},
	{ EI_FCF	, "EI_FCF"	},
	{ EI_SYSGEN	, "EI_SYSGEN"	},
	{ EI_SETID	, "EI_SETID"	},
	{ EI_TURNON	, "EI_TURNON"	},
	{ EI_ALLOC	, "EI_ALLOC"	},
	{ EI_TERM	, "EI_TERM"	},
	{ EI_TURNOFF	, "EI_TURNOFF"	},
	{ EI_SETA	, "EI_SETA"	},
	{ EI_GETA	, "EI_GETA"	},
#endif
#ifdef IFFORMAT
	{ IFFORMAT	, "IFFORMAT"	},
	{ IFBCHECK	, "IFBCHECK"	},
	{ IFCONFIRM	, "IFCONFIRM"	},
#endif
#ifdef LIOCGETP
	{ LIOCGETP	, "LIOCGETP"	},
	{ LIOCSETP	, "LIOCSETP"	},
	{ LIOCGETS	, "LIOCGETS"	},
	{ LIOCSETS	, "LIOCSETS"	},
#endif
#ifdef DIOCGETC
	{ DIOCGETC	, "DIOCGETC"	},
	{ DIOCGETB	, "DIOCGETB"	},
	{ DIOCSETE	, "DIOCSETE"	},
#endif
#ifdef JBOOT
	{ JBOOT		, "JBOOT"	},
	{ JTERM		, "JTERM"	},
	{ JMPX		, "JMPX"	},
#ifdef JTIMO
	{ JTIMO		, "JTIMO"	},
#endif
	{ JWINSIZE	, "JWINSIZE"	},
	{ JTIMOM	, "JTIMOM"	},
	{ JZOMBOOT	, "JZOMBOOT"	},
	{ JAGENT	, "JAGENT"	},
	{ JTRUN		, "JTRUN"	},
#endif
#ifdef NISETA
	{ NISETA	, "NISETA"	},
	{ NIGETA	, "NIGETA"	},
	{ SUPBUF	, "SUPBUF"	},
	{ RDBUF		, "RDBUF"	},
	{ NIERRNO	, "NIERRNO"	},
	{ STATGET	, "STATGET"	},
	{ NISTATUS	, "NISTATUS"	},
	{ NIPUMP	, "NIPUMP"	},
	{ NIRESET	, "NIRESET"	},
	{ NISELGRP	, "NISELGRP"	},
	{ NISELECT	, "NISELECT"	},
#endif
	{ STGET		, "STGET"	},
	{ STSET		, "STSET"	},
	{ STTHROW	, "STTHROW"	},
	{ STWLINE	, "STWLINE"	},
#ifdef STTSV
	{ STTSV		, "STTSV"	},
#endif
	{ I_NREAD	, "I_NREAD"	},
	{ I_PUSH	, "I_PUSH"	},
	{ I_POP		, "I_POP"	},
	{ I_LOOK	, "I_LOOK"	},
	{ I_FLUSH	, "I_FLUSH"	},
	{ I_SRDOPT	, "I_SRDOPT"	},
	{ I_GRDOPT	, "I_GRDOPT"	},
	{ I_STR		, "I_STR"	},
	{ I_SETSIG	, "I_SETSIG"	},
	{ I_GETSIG	, "I_GETSIG"	},
	{ I_FIND	, "I_FIND"	},
	{ I_LINK	, "I_LINK"	},
	{ I_UNLINK	, "I_UNLINK"	},
	{ I_PEEK	, "I_PEEK"	},
	{ I_FDINSERT	, "I_FDINSERT"	},
	{ I_SENDFD	, "I_SENDFD"	},
	{ I_RECVFD	, "I_RECVFD"	},
	{ I_SWROPT	, "I_SWROPT"	},
	{ I_GWROPT	, "I_GWROPT"	},
	{ I_LIST	, "I_LIST"	},
	{ I_PLINK	, "I_PLINK"	},
	{ I_PUNLINK	, "I_PUNLINK"	},
#ifdef GEM
	{ I_SETEV	, "I_SETEV"	},
	{ I_GETEV	, "I_GETEV"	},
	{ I_STREV	, "I_STREV"	},
	{ I_UNSTREV	, "I_UNSTREV"	},
#endif /* GEM */
	{ I_FLUSHBAND	, "I_FLUSHBAND"	},
	{ I_CKBAND	, "I_CKBAND"	},
	{ I_GETBAND	, "I_GETBAND"	},
	{ I_ATMARK	, "I_ATMARK"	},
	{ I_SETCLTIME	, "I_SETCLTIME"	},
	{ I_GETCLTIME	, "I_GETCLTIME"	},
	{ I_CANPUT	, "I_CANPUT"	},
	{ TCGETA	, "TCGETA"	},
	{ TCSETA	, "TCSETA"	},
	{ TCSETAW	, "TCSETAW"	},
	{ TCSETAF	, "TCSETAF"	},
	{ TCSBRK	, "TCSBRK"	},
	{ TCXONC	, "TCXONC"	},
	{ TCFLSH	, "TCFLSH"	},
	{ TCDSET	, "TCDSET"	},
#ifdef TI_GETINFO
	{ TI_GETINFO	, "TI_GETINFO"	},
	{ TI_OPTMGMT	, "TI_OPTMGMT"	},
	{ TI_BIND	, "TI_BIND"	},
	{ TI_UNBIND	, "TI_UNBIND"	},
#endif
	{ LDOPEN	, "LDOPEN"	},
	{ LDCLOSE	, "LDCLOSE"	},
	{ LDCHG		, "LDCHG"	},
	{ LDGETT	, "LDGETT"	},
	{ LDSETT	, "LDSETT"	},
#ifdef V_PREAD
	{ V_PREAD	, "V_PREAD"	},
	{ V_PWRITE	, "V_PWRITE"	},
	{ V_PDREAD	, "V_PDREAD"	},
	{ V_PDWRITE	, "V_PDWRITE"	},
#endif
#ifdef PIOCSTATUS
	{ PIOCSTATUS	, "PIOCSTATUS"	},
	{ PIOCSTOP	, "PIOCSTOP"	},
	{ PIOCWSTOP	, "PIOCWSTOP"	},
	{ PIOCRUN	, "PIOCRUN"	},
	{ PIOCGTRACE	, "PIOCGTRACE"	},
	{ PIOCSTRACE	, "PIOCSTRACE"	},
	{ PIOCSSIG	, "PIOCSSIG"	},
	{ PIOCKILL	, "PIOCKILL"	},
	{ PIOCUNKILL	, "PIOCUNKILL"	},
	{ PIOCGHOLD	, "PIOCGHOLD"	},
	{ PIOCSHOLD	, "PIOCSHOLD"	},
	{ PIOCMAXSIG	, "PIOCMAXSIG"	},
	{ PIOCACTION	, "PIOCACTION"	},
	{ PIOCGFAULT	, "PIOCGFAULT"	},
	{ PIOCSFAULT	, "PIOCSFAULT"	},
	{ PIOCCFAULT	, "PIOCCFAULT"	},
	{ PIOCGENTRY	, "PIOCGENTRY"	},
	{ PIOCSENTRY	, "PIOCSENTRY"	},
	{ PIOCGEXIT	, "PIOCGEXIT"	},
	{ PIOCSEXIT	, "PIOCSEXIT"	},
	{ PIOCSFORK	, "PIOCSFORK"	},
	{ PIOCRFORK	, "PIOCRFORK"	},
	{ PIOCSRLC	, "PIOCSRLC"	},
	{ PIOCRRLC	, "PIOCRRLC"	},
	{ PIOCGREG	, "PIOCGREG"	},
	{ PIOCSREG	, "PIOCSREG"	},
	{ PIOCGFPREG	, "PIOCGFPREG"	},
	{ PIOCSFPREG	, "PIOCSFPREG"	},
	{ PIOCNICE	, "PIOCNICE"	},
	{ PIOCPSINFO	, "PIOCPSINFO"	},
	{ PIOCNMAP	, "PIOCNMAP"	},
	{ PIOCMAP	, "PIOCMAP"	},
	{ PIOCOPENM	, "PIOCOPENM"	},
	{ PIOCCRED	, "PIOCCRED"	},
	{ PIOCGROUPS	, "PIOCGROUPS"	},
	{ PIOCGETPR	, "PIOCGETPR"	},
	{ PIOCGETU	, "PIOCGETU"	},
#endif
#ifdef PUMP
	{ PUMP		, "PUMP"	},
#endif
	{ TIOCKBON	, "TIOCKBON"	},
	{ TIOCKBOF	, "TIOCKBOF"	},
	{ KBENABLED	, "KBENABLED"	},
	{ RTS_TOG	, "RTS_TOG"	},
	{ TIOCGWINSZ	, "TIOCGWINSZ"	},
	{ TIOCSWINSZ	, "TIOCSWINSZ"	},
	{ TCGETS	, "TCGETS"	},
	{ TCSETS	, "TCSETS"	},
	{ TCSETSW	, "TCSETSW"	},
	{ TCSETSF	, "TCSETSF"	},
	{ TIOCGETD	, "TIOCGETD"	},
	{ TIOCSETD	, "TIOCSETD"	},
	{ TIOCHPCL	, "TIOCHPCL"	},
	{ TIOCSETN	, "TIOCSETN"	},
	{ TIOCEXCL	, "TIOCEXCL"	},
	{ TIOCNXCL	, "TIOCNXCL"	},
	{ TIOCFLUSH	, "TIOCFLUSH"	},
	{ TIOCSETC	, "TIOCSETC"	},
	{ TIOCGETC	, "TIOCGETC"	},
	{ TIOCGPGRP	, "TIOCGPGRP"	},
	{ TIOCSPGRP	, "TIOCSPGRP"	},
	{ TIOCGSID	, "TIOCGSID"	},
	{ TIOCSTI	, "TIOCSTI"	},
	{ TIOCMSET	, "TIOCMSET"	},
	{ TIOCMBIS	, "TIOCMBIS"	},
	{ TIOCMBIC	, "TIOCMBIC"	},
	{ TIOCMGET	, "TIOCMGET"	},
	{ TIOCREMOTE	, "TIOCREMOTE"	},
	{ TIOCSIGNAL	, "TIOCSIGNAL"	},
	{ LDGMAP	, "LDGMAP"	},
	{ LDSMAP	, "LDSMAP"	},
	{ LDNMAP	, "LDNMAP"	},
	{ DIOCGETP	, "DIOCGETP"	},
	{ DIOCSETP	, "DIOCSETP"	},
	{ FIORDCHK	, "FIORDCHK"	},
	{ TCGETX	, "TCGETX"	},
	{ TCSETX	, "TCSETX"	},
	{ TCSETXW	, "TCSETXW"	},
	{ TCSETXF	, "TCSETXF"	},
	{ FIOCLEX	, "FIOCLEX"	},
	{ FIONCLEX	, "FIONCLEX"	},
	{ FIONREAD	, "FIONREAD"	},
	{ FIONBIO	, "FIONBIO"	},
	{ FIOASYNC	, "FIOASYNC"	},
	{ FIOSETOWN	, "FIOSETOWN"	},
	{ FIOGETOWN	, "FIOGETOWN"	},
	{ 0		,  NULL		}
};
