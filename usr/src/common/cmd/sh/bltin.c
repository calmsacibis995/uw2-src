/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sh:common/cmd/sh/bltin.c	1.3.20.5"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/sh/bltin.c,v 1.1 91/02/28 20:08:08 ccs Exp $"
/*
 *
 * UNIX shell
 *
 */


#include	"defs.h"
#include	<errno.h>
#include	"sym.h"
#include	"hash.h"
#include	<sys/types.h>
#include	<sys/times.h>
#include	<sys/time.h>
#include	<mac.h>
#include	<priv.h>
#include	<pfmt.h>
#include	<string.h>

extern	unsigned long	umask();
extern	pid_t	fork();
extern	pid_t	wait();
extern	int	chdir();

extern	int	test();
extern	int	echo();
extern	void	systrap();
extern	void	sysulimit();
extern	void	syswait();
extern	int	syspriv();
extern	void	sysstop();
extern	void	syssusp();
extern	void	sysfgbg();
extern	void	sysjobs();
extern	void	syskill();

void
builtin(type, argc, argv, t)
int type, argc;
unsigned char **argv;
struct trenod	*t;
{
	short index = initio(t->treio, (type != SYSEXEC));
	unsigned char *a1 = argv[1];
	int c, newmode = -1, mode, indx, pid, status;
static	const struct {
		const char *id, *msg;
	} charmode[] = {
		":823", "virtual",
		":824", "real"
	};

	switch (type)		
	{

	case SYSSUSP:
		syssusp(argc,argv);
		break;

	case SYSSTOP:
		sysstop(argc,argv);
		break;

	case SYSKILL:
		syskill(argc,argv);
		break;

	case SYSFGBG:
		sysfgbg(argc,argv);
		break;

	case SYSJOBS:
		sysjobs(argc,argv);
		break;

	case SYSDOT:
		if (a1)
		{
			register int	f;

			if ((f = pathopen(getpath(a1), a1)) < 0)
				failed(0, a1, notfound, notfoundid);
			else
				execexp(0, f);
		}
		break;

	case SYSTIMES:
		{
			struct tms tms;

			(void)times(&tms);
			prt(tms.tms_cutime);
			prc_buff(SP);
			prt(tms.tms_cstime);
			prc_buff(NL);
		}
		break;

	case SYSEXIT:
		flags |= forcexit;	/* force exit */
		exitsh(a1 ? stoi(a1) : retval);

	case SYSNULL:	/*FALLTHROUGH*/
		break;

	case SYSCONT:
		if (loopcnt)
		{
			execbrk = breakcnt = 1;
			if (a1)
				breakcnt = stoi(a1);
			if (breakcnt > loopcnt)
				breakcnt = loopcnt;
			else
				breakcnt = -breakcnt;
		}
		break;

	case SYSBREAK:
		if (loopcnt)
		{
			execbrk = breakcnt = 1;
			if (a1)
				breakcnt = stoi(a1);
			if (breakcnt > loopcnt)
				breakcnt = loopcnt;
		}
		break;

	case SYSTRAP:
		systrap(argc,argv);
		break;

	case SYSEXEC:
		argv++;

		/*
		 *	Fix for IO redirection problems
		 *	within functions
		 */
		if(a1)	{
			ioset = 0;
			t->treio = 0;
		}
		else {
			setmode(0);
			break;
		}
		/* FALLTHROUGH */

#ifdef RES	/* Research includes login as part of the shell */

	case SYSLOGIN:
		if (!endjobs(JOB_STOPPED|JOB_RUNNING))
			break;
		oldsigs();
		execa(argv, -1);
		done(0);
#else

	case SYSNEWGRP:
		if (flags & rshflg)
			failed(SYSNEWGRP, argv[0], restricted, restrictedid);
		else if (!endjobs(JOB_STOPPED|JOB_RUNNING))
			break;
		else
		{
			flags |= forcexit; /* bad exec will terminate shell */
			oldsigs();
			rmtemp(0);
			rmfunctmp();
#ifdef ACCT
			doacct();
#endif
			execa(argv, -1);
			done(0);
		}

#endif

	case SYSCD:	/*FALLTHROUGH*/
		if (flags & rshflg)
			failed(SYSCD, argv[0], restricted, restrictedid);
		else if ((a1 && *a1) || (a1 == 0 && (a1 = homenod.namval)))
		{
			unsigned char *cdpath;
			unsigned char *dir;
			int f;

			if ((cdpath = cdpnod.namval) == 0 ||
			     *a1 == '/' ||
			     cf(a1, ".") == 0 ||
			     cf(a1, "..") == 0 ||
			     (*a1 == '.' && (*(a1+1) == '/' || *(a1+1) == '.' && *(a1+2) == '/')))
				cdpath = (unsigned char *)nullstr;

			do
			{
				dir = cdpath;
				cdpath = catpath(cdpath,a1);
			}
			while ((f = chdir(curstak()) < 0) && cdpath);

			if (f) {
				switch(errno) {
						case EMULTIHOP:
							failure(SYSCD, a1, emultihop, emultihopid);
							break;
						case ENOTDIR:
							failure(SYSCD, a1, enotdir, enotdirid);
							break;
						case ENOENT:
							failure(SYSCD, a1, enoent, enoentid);
							break;
						case EACCES:
							failure(SYSCD, a1, eacces, eaccesid);
							break;
						case ENOLINK:
							failure(SYSCD, a1, enolink, enolinkid);
							break;
						default: 
						failure(SYSCD, a1, baddir, baddirid);
						break;
						}
			}
			else 
			{
				cwd(curstak());
				if (cf(nullstr, dir) &&
				    *dir != ':' &&
					any('/', curstak()) &&
					flags & prompt)
				{
					prs_buff(cwdget());
					prc_buff(NL);
				}
			}
			zapcd();
		}
		else 
		{
			if (a1)
				error(SYSCD, nulldir, nulldirid);
			else
				error(SYSCD, nohome, nohomeid);
		}

		break;

	case SYSSHFT:
		{
			int places;

			places = a1 ? stoi(a1) : 1;

			if ((dolc -= places) < 0)
			{
				dolc = 0;
				error(SYSSHFT, badshift, badshiftid);
			}
			else
				dolv += places;
		}			

		break;

	case SYSWAIT:
		syswait(argc,argv);
		break;

	case SYSREAD:
		{
			int savopterr = opterr;
			int savoptind = optind;

			opterr = 0;
			optind = 1;
			read_rflag = 0;
			while ((c = getopt(argc, (char **)argv, "r")) != -1)
				switch (c)	{
				case 'r':
					if (!read_rflag)	{
						read_rflag++;
						break;
					}
					/* else fall through */
				case '?':
					opterr = savopterr;
					optind = savoptind;
					prusage(SYSREAD, readuse, readuseid);
					exitsh(ERROR);
				}
			argc -= read_rflag;
			if(argc < 2)	{
				opterr = savopterr;
				optind = savoptind;
				failed(SYSREAD, argv[0], mssgargn, mssgargnid);
			}
			argv += read_rflag;
			rwait = 1;
			exitval = readvar(&argv[1]);
			opterr = savopterr;
			optind = savoptind;
			rwait = 0;
			break;
		}

	case SYSSET:
		if (a1)
		{
			int	cnt;

			cnt = options(argc, argv);
			if (cnt > 1)
				setargs(argv + argc - cnt);
		}
		else if (comptr(t)->comset == 0)
		{
			/*
			 * scan name chain and print
			 */
			namscan(printnam);
		}
		break;

	case SYSRDONLY:
		exitval = 0;
		if (a1)
		{
			while (*++argv)
				attrib(lookup(*argv), N_RDONLY);
		}
		else
			namscan(printro);

		break;

	case SYSXPORT:
		{
			struct namnod 	*n;

			exitval = 0;
			if (a1)
			{
				while (*++argv)
				{
					n = lookup(*argv);
					if (n->namflg & N_FUNCTN)
						error(SYSXPORT, badexport, badexportid);
					else
						attrib(n, N_EXPORT);
				}
			}
			else
				namscan(printexp);
		}
		break;

	case SYSEVAL:
		if (a1)
			execexp(a1, &argv[2]);
		break;

#ifndef RES	
	case SYSULIMIT:
		sysulimit(argc, argv);
		break;
			
	case SYSUMASK:
		sysumask(argc, argv);
		break;

#endif

	case SYSTST:
		exitval = test(argc, argv);
		break;

	case SYSECHO:
		exitval = echo(argc, argv);
		break;

	case SYSHASH:
		exitval = 0;

		if (a1)
		{
			if (a1[0] == '-')
			{
				if (a1[1] == 'r')
					zaphash();
				else
					error(SYSHASH, badopt, badoptid);
			}
			else
			{
				while (*++argv)
				{
					if (hashtype(hash_cmd(*argv)) == NOTFOUND)
						failed(SYSHASH, *argv, notfound, notfoundid);
				}
			}
		}
		else
			hashpr();

		break;

	case SYSPWD:
		{
			exitval = 0;
			cwdprint();
		}
		break;

	case SYSRETURN:
		if (funcnt == 0)
			error(SYSRETURN, badreturn, badreturnid);

		execbrk = 1;
		exitval = (a1 ? stoi(a1) : retval);
		break;
	
	case SYSTYPE:
		exitval = 0;
		if (a1)
		{
			while (*++argv)	{
				exitval += what_is_path(*argv);
			}

			if((exitval = exitval != 0) && (flags & errflg))  {
				flushb();
				exitsh(exitval);
			}
		}
		break;

	case SYSUNS:
		exitval = 0;
		if (a1)
		{
			while (*++argv)
				unset_name(*argv);
		}
		setwidth();
		break;

	case SYSGETOPT: {
		int getoptval;
		struct namnod *n;
		extern unsigned char numbuf[];
		unsigned char *varnam = argv[2];
		unsigned char c[2];
		if(argc < 3) {
			error_fail(SYSGETOPT, mssgargn, mssgargnid);
			break;
		}
		exitval = 0;
		n = lookup("OPTIND");
		optind = stoi(n->namval);
		if(argc > 3) {
			argv[2] = dolv[0];
			getoptval = getopt(argc-2, (char **)&argv[2], (char *)argv[1]);
		}
		else
			getoptval = getopt(dolc+1, (char **)dolv, (char *)argv[1]);
		if(getoptval == -1) {
			itos(optind);
			assign(n, numbuf);
			n = lookup(varnam);
			assign(n, nullstr);
			exitval = 1;
			break;
		}
		argv[2] = varnam;
		itos(optind);
		assign(n, numbuf);
		c[0] = (char)getoptval;
		c[1] = 0;
		n = lookup(varnam);
		assign(n, c);
		n = lookup("OPTARG");
		assign(n, optarg);
		}
		break;

        case SYSPRIV:
		exitval = syspriv(argv,argc);
		break;

	case SYSMLD:
		optind = 1;
		flushb();
		set_label(SYSMLD);
		if((mode = mldmode(MLD_QUERY)) < 0){
			error_fail(SYSMLD, notsupport, notsupportid);
			exitval = 3;
			break;
		}
		while ((c=getopt(argc, (char **)argv,"rv")) != -1){
			switch (c) {
			case 'r':
				if (newmode == MLD_VIRT) {
					pfmt(stderr, MM_ERROR, NULL);
					prs(gettxt(invalcombid,invalcomb));
					newline();
					pfmt(stderr, MM_ACTION, usage,
						gettxt(mldusageid, mldusage));
					exitsh(ERROR);
				}
				newmode = MLD_REAL;
				break;
			case 'v':
				if (newmode == MLD_REAL) {
					pfmt(stderr, MM_ERROR, NULL);
					prs(gettxt(invalcombid,invalcomb));
					newline();
					pfmt(stderr, MM_ACTION, usage,
						gettxt(mldusageid, mldusage));
					exitsh(ERROR);
				}
				newmode = MLD_VIRT;
				break;
			default:
				pfmt(stderr, MM_ACTION, usage,
					gettxt(mldusageid, mldusage));
				exitsh(ERROR);
			}
		}

		/*
		 * If no options specified, just print current mode.
		 */
	
		if ((argc <= optind) && (newmode == -1)) {
			prs_buff(gettxt(mldmodeisid,mldmodeis));
			prs_buff(gettxt(charmode[mode].id,charmode[mode].msg));
			prs_buff("\n");
			break;
		}
	
		/*
		 * If a command line was specified, exec that command.
		 */
	
		if (argc > optind) {
			if (pid=fork()) {
				if (pid == -1) {
					switch (errno) {
					case ENOMEM:
						deallocjob();
						error(SYSMLD,noswap,noswapid);
						break;
					default:
						deallocjob();
						error(SYSMLD,nofork,noforkid);
						break;
					}
					break;
				} else {
					while(wait(&status) != pid);
					exitval = status >> 8;
					break;
				}
			}
			/*
			 * If a mode was specified, attempt to change to
			 * that mode (for the duration of this process).
			 */
		
			if((newmode != -1) && (mldmode(newmode) < 0)){
				error(SYSMLD,nosetmode,nosetmodeid);
			}
			flags |= forcexit; /* bad exec will terminate shell */
			oldsigs();
			rmtemp(0);
			rmfunctmp();
#ifdef ACCT
			doacct();
#endif
			for(indx=1;(*argv[indx] == '-')&&(indx < argc);++indx);
			if((indx == argc) || (indx < 2)){
				pfmt(stderr, MM_ACTION, usage,
					gettxt(mldusageid, mldusage));
				exitsh(ERROR);
			}
			execa(&argv[indx],-1);
			done(0);
		}
		/*
		 * If a mode was specified, attempt to change to that mode (for
		 * the duration of this process).
		 */
		if((newmode != -1) && (mldmode(newmode) < 0)){
			error(SYSMLD,nosetmode,nosetmodeid);
		}
		break;

	default:
		prs_buff(gettxt(":479", "Unknown builtin\n"));
	}
	flushb();
	restore(index);
	chktrap();
}
