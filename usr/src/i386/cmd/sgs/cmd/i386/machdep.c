/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs-cmd:i386/machdep.c	1.19"

#include	"cc.h"
#include        <unistd.h>

static char	*Mach = NULL;	/* will hold the -i386, -i486 or -pentium 
				   option, which is passed to optim	*/

static char	*As_Mach = NULL; /* will hold the -t386, -t486 or -tpentium
                                   option, which is passed to as.  */

static char	*Frame = NULL;  /* -Kframe generates standard code that uses 
				    %ebp as a frame pointer, -Kno_frame, the 
				    default, allows the compilation system to 
				    attempt to eliminate the frame pointer  */

static int	Ilevel = 0;	/* will hold the level of inlining requested */
static int	Inline_Requested = 0;  /* boolean, was inlining explicitly 
				 	  requested by the user?  */

static char     *LoopUnroll = NULL;  /* will be used to control the loop
                                        unrolling optimization. The default
                                        is to turn on loop unrolling in amigo */

#undef CPLUSPLUS_INLINING_ONLY_DEFAULTS_WITH_OPTIMIZATION
	/* Should inlining be done if optimization is not?

	   Depends on how well acomp can generated inlined code without full
	   optimization and also how well debugger deals with inlining; 
	   decision has gone back and forth several times, so define or undef
	   here accordingly.

	   In any case, inlining is done if explicitly asked for.
	*/

void initvars()
{

	if (CCflag) {
#ifdef CPLUSPLUS_INLINING_ONLY_DEFAULTS_WITH_OPTIMIZATION
		Ilevel = -1;	/* C++ inline default depends on other options,
				   indicate no default value for now */
#else
		Ilevel = 2;	/* default to c++_inline for C++ */
#endif
		Inline_Requested = 0;
	}
	return;
}

int Kelse(s)
char *s;
{
	if (strcmp(s, "i386")==0){
		Mach = "-386";
		As_Mach = "-t386";
		}
	else if (strcmp(s, "i486")==0) {
		Mach = "-486";
		As_Mach = "-t486";
		}
	else if (strcmp(s, "pentium")==0){
		Mach = "-pentium";
		As_Mach = "-tpentium";
		}
	else if (strcmp(s, "blended")==0){
		Mach = NULL;  /* This should be changed in the future when
				 optim is changed to accept the -blended mode
				 of compilation as an argument.  Currently,
				 no 386/486/pentium option indicates blended
				 mode in optim */
		As_Mach = NULL;
		}
	else if (strcmp(s, "frame")==0)
		Frame = "-_e";
	else if (strcmp(s, "inline")==0) {
		Ilevel = CCflag ? 3 : 1;
		Inline_Requested = 1;
		}
	else if (CCflag && (strcmp(s, "c++inline")==0) || (strcmp(s, "c++_inline")==0)) {
		Ilevel = 2;
		Inline_Requested = 1;
		}
	else if (CCflag && (strcmp(s, "acompinline")==0) || (strcmp(s, "acomp_inline")==0)) {
		Ilevel = 1;
		Inline_Requested = 1;
		}
	else if (CCflag && (strcmp(s, "allinline")==0) || (strcmp(s, "all_inline")==0)) {
		Ilevel = 4;
		Inline_Requested = 1;
		}
	else if ((strcmp(s, "no_inline")==0) || (strcmp(s, "noinline")==0)) {
		Ilevel = 0;
		Inline_Requested = 0;
		}
	else if ((strcmp(s, "no_frame")==0) || (strcmp(s, "noframe")==0))
		return 1;
	else if (strcmp(s, "ieee")==0) {
		addopt(Xc0, "-2i1");
		addopt(Xc2, "-Kieee");
		}
	else if ((strcmp(s, "no_ieee")==0) || (strcmp(s, "noieee")==0) ){
		addopt(Xc0, "-2i0");
		addopt(Xc2, "-Knoieee");
		}
	else if ((strcmp(s, "loop_unroll") == 0) || (strcmp(s, "lu") == 0))
                        LoopUnroll = "-Glu";
        else if ((strcmp(s,"no_loop_unroll") == 0) || (strcmp(s,"no_lu") == 0)
                || (strcmp(s,"noloop_unroll") == 0) || (strcmp(s,"nolu") == 0))
                        LoopUnroll = "-G~lu";

        else if (strcmp(s,"host") == 0)
                addopt(Xc0, "-2h1"); /* host is the default in acomp */
        else if ((strcmp(s, "no_host") == 0) || (strcmp(s, "nohost") == 0))
                addopt(Xc0, "-2h0");
        else
                return 0;

        return 1;
}

int Yelse(c, np)
int c;
char *np;
{
	return (0);
}

int
Welse(c)
char c;
{
	return (-1);
}

int optelse(c, s)
int c;
char *s;
{
	switch(c) {
	case 'Z':	/* acomp: pack structures for 386 */
		if (!s) {
			error('e', gettxt(":46","Option -Z requires an argument\n"));
			exit(1);
		} else if (*s++ != 'p') {
			error('e', gettxt(":47","Illegal argument to -Z flag\n"));
			exit(1);
		}
		switch ( *s ) {
		case '1':
			if (CCflag) addopt(Xfe, "-Zp1");
			addopt(Xc0, "-Zp1");
			break;
		case '2':
			if (CCflag) addopt(Xfe, "-Zp2");
			addopt(Xc0, "-Zp2");
			break;
		case '4':
			if (CCflag) addopt(Xfe, "-Zp4");
			addopt(Xc0, "-Zp4");
			break;
		case '\0':
			if (CCflag) addopt(Xfe, "-Zp1");
			addopt(Xc0, "-Zp1");
			break;
		default:
			error('e', gettxt(":48","Illegal argument to -Zp flag\n"));
			exit(1);
		}
		return 1;
	}
	return 0;
}

void init_mach_opt()
{
	return;
}

void add_mach_opt()
{
	static char	*Inline= NULL;  /* the inlining argument for acomp */

	if (As_Mach != NULL)
		addopt(Xas, As_Mach);

#ifdef CPLUSPLUS_INLINING_ONLY_DEFAULTS_WITH_OPTIMIZATION
	if (CCflag && (Ilevel == -1))
		/* inlining defaults if optimization on, otherwise doesn't */
		Ilevel = Oflag ? 2 : 0;
#endif

#ifdef CPLUSPLUS_DEBUGGING_INLINED_CODE_DOES_NOT_WORK
	/* This is no longer necessary, due to acomp improvements in this area,
	   but it might become necessary again -- this has been changed back
	   and worth several times already.    */

        /* If C++ and -g has been specified, suppress any inlining.
	 * This is because inlining confuses the debugger.
	 * Also, if inlining was explicitly requested (rather than normal default behaviour), issue warning.
	 */
	if (CCflag && Ilevel && gflag) {
		Ilevel = 0;
		if (Inline_Requested)
			error('w', gettxt(":1553","debugging and inlining mutually exclusive; inlining disabled\n"));
	}
#endif

#define	CPLUSPLUS_BASIC_BLOCK_PROFILING_INLINED_CODE_DOES_NOT_WORK
#ifdef	CPLUSPLUS_BASIC_BLOCK_PROFILING_INLINED_CODE_DOES_NOT_WORK
	/* This still is necessary, but might not be in the future. */

        /* If C++ and -ql has been specified, suppress any inlining.
	 * This is because inlining confuses the basic block profiler.
	 * Also, if inlining was explicitly requested (rather than normal default behaviour), issue warning.
	 */
	if (CCflag && Ilevel && qarg) {
		Ilevel = 0;
		if (Inline_Requested)
			error('w', gettxt(":1544","basic block profiling and inlining mutually exclusive; inlining disabled\n"));
	}
#endif
 
	/* Now we pass the inline level to acomp, but only if the user
	 * has also turned on optimization (since C inlining is only
	 * done with -O), or we are in C++ mode (since C++ inlining is
	 * independent of optimization, for now).
	 */
	if ((CCflag || Oflag) && Ilevel) {
		Inline = stralloc(4);
		(void) sprintf(Inline, "-1I%d", Ilevel);
		addopt(Xc0, Inline);
	}

#ifdef CPLUSPLUS_INLINING_SHOULD_TURN_ON_REGISTER_ALLOCATION
	/* This has not yet been committed to, but is still being considered. */

	/* Since C++ inlining is independent of optimization, we must tell
	 * amigo to do register allocation if the user has not used -O.
	 * Otherwise many benefits of inlining are lost.    */

	if (CCflag && Ilevel && !Oflag)
		addopt(Xc0, "???");
#endif

	if (!Oflag)
		return;

	if (Mach != NULL)
		addopt(Xc2, Mach);
	if (Frame != NULL)
		addopt(Xc2, Frame);
	if (LoopUnroll != NULL)
		addopt(Xc0, LoopUnroll);
	
	return;

}

void mach_defpath()
{
	return;
}

void AVmore()
{
	if (Oflag)	/* pass -O to acomp */
		addopt(AV, "-O");

	if (Eflag || Pflag)
		return;

	if (pflag)
		addopt(AV,"-p");

	return;
}

/*===================================================================*/
/*                                                                   */
/*                      OPTIMIZER                                    */
/*                                                                   */
/*===================================================================*/
int optimize (i)
	int i;
{
	int j;
	
	nlist[AV]= 0;
		addopt(AV,passname(prefix, N_OPTIM));

	addopt(AV,"-I");
	addopt(AV,c_out);
	addopt(AV,"-O");
	addopt(AV,as_in
		 = (Sflag && !qarg) ? setsuf(list[CF][i], "s") : tmp5);
	for (j = 0; j < nlist[Xc2]; j++)
		addopt(AV,list[Xc2][j]);

	list[AV][nlist[AV]] = 0;	/* terminate arg list */

	PARGS;


	if (callsys(passc2, list[AV], NO_TMP4_REDIRECTION, NORMAL_STDOUT)) {
		error('e', "Optimizer failed, %s was not compiled\n", list[CF][i]);
		cflag++;
		eflag++;
		cunlink(c_out);
		cunlink(as_in);
		return(0);
	} else {
        	c_out= as_in;
        	cunlink(tmp2);
        }


#ifdef PERF
	STATS("optimizer");
#endif

	return(1);
}

void option_mach()
{
	pfmt(stderr,MM_NOSTD,":109:\t[-Zp[124]]: pack structure for i386 acomp.\n");

	return;
}
