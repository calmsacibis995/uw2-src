/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:cmd/lpsched/lpsched/flt.c	1.4.8.2"
#ident  "$Header: flt.c 1.2 91/06/27 $"

#ifdef	__STDC__
# include	<stdarg.h>
#else
# include	<varargs.h>
#endif

# include	"lpsched.h"

typedef struct fault	FLT;

struct fault
{
    FLT *	next;
    int		type;
    int		i1;
    char *	s1;
    RSTATUS *	r1;
    MESG *	ident;
};

#ifdef	__STDC__
static void free_flt ( FLT * );
static void do_flt_acts ( MESG * );
#else
static void free_flt();
static void do_flt_acts();
#endif

static FLT	Fault_Head = { NULL, 0, 0, NULL, NULL, NULL };
static FLT *	Fault_List = &Fault_Head;

void
#ifdef	__STDC__
add_flt_act ( MESG * md, ... )	/*funcdef*/
#else
add_flt_act (md, va_alist)
    MESG	*md;
    va_dcl
#endif
{
    DEFINE_FNNAME (add_flt_act)

    va_list	arg;
    FLT		*f;

#ifdef	__STDC__
    va_start (arg, md);
#else
    va_start (arg);
#endif

    f = (FLT *)Malloc(sizeof(FLT));
    
    (void) memset((char *)f, 0, sizeof(FLT));
    
    f->type = (int)va_arg(arg, int);
    f->ident = md;
    
    if (md->on_discon == NULL)
	if (mon_discon(md, do_flt_acts))
	    mallocfail();

    switch(f->type)
    {
	case FLT_FILES:
	f->s1 = Strdup((char *)va_arg(arg, char *));
	f->i1 = (int)va_arg(arg, int);
	break;
	
	case FLT_CHANGE:
	f->r1 = (RSTATUS *)va_arg(arg, RSTATUS *);
	break;
    }

    va_end(arg);

    f->next = Fault_List->next;
    Fault_List->next = f;
}


void
#ifdef	__STDC__
del_flt_act ( MESG * md, ... )	/*funcdef*/
#else
del_flt_act (md, va_alist)
    MESG	*md;
    va_dcl
#endif
{
    DEFINE_FNNAME (del_flt_act)

    va_list	arg;
    int		type;
    FLT		*fp;
    FLT		*f;

#ifdef	__STDC__
    va_start(arg, md);
#else
    va_start(arg);
#endif

    type = (int)va_arg(arg, int);
    
    for (f = Fault_List; f->next; f = f->next)
	if (f->next->type == type && f->next->ident == md)
	{
	    fp = f->next;
	    f->next = f->next->next;
	    free_flt(fp);
	    break;
	}

    va_end(arg);
}

/*
 * Procedure:     do_flt_acts
 *
 * Restrictions:
 *               Unlink: None
 *               putrequest: None
*/
static void
#ifdef	__STDC__
do_flt_acts ( MESG * md )	/*funcdef*/
#else
do_flt_acts (md)	/*funcdef*/
    MESG	*md;
#endif
{
    DEFINE_FNNAME (do_flt_acts)

    FLT		*f;
    FLT		*fp;
    char	*file;
    char	id[15];
    
    for (f = Fault_List; f && f->next; f = f->next)
	if (f->next->ident == md)
	{
	    fp = f->next;
	    f->next = f->next->next;

	    switch (fp->type)
	    {
		case FLT_FILES:
		/* remove files created with alloc_files */
		while(fp->i1--)
		{
		    (void) sprintf(id, "%s-%d", fp->s1, fp->i1);
		    file = makepath(Lp_Temp, id, (char *)0);
		    (void) Unlink(file);
		    Free(file);
		}
		break;
		

		case FLT_CHANGE:
		/* clear RS_CHANGE bit, write request file, and schedule */
		fp->r1->request->outcome &= ~RS_CHANGING;
		(void) putrequest(fp->r1->req_file, fp->r1->request);
		if (NEEDS_FILTERING(fp->r1))
		    schedule(/* LP_FILTER */ EV_SLOWF, fp->r1);
		else
		    schedule(/* LP_PRINTER */ EV_INTERF, fp->r1->printer);
		break;
	    }
	    free_flt(fp);
	}
}

static void
#ifdef	__STDC__
free_flt ( FLT * f )	/*funcdef*/
#else
free_flt (f)
    FLT		*f;
#endif
{
    DEFINE_FNNAME (free_flt)

    if (f->s1)
	Free(f->s1);
    Free((char *)f);
}
