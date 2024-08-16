	.file	"mcount.s"

	.ident	"@(#)lprof:libprof/i386/mcount.s	1.1"


/ / /
/ text names
	.text

	.globl	_mcount


/ _mcount: call count increment routine for cc -p/-qp profiling (prof(1))

/
/ This routine is called at the entry to each subroutine compiled with
/ the -p option to cc(1).
/
/ struct counter {
/	void	(*addr)();	/* address of calling routine */
/	long	count;		/* # of times called by caller */
/	struct _SOentry *_SOptr;/* pointer to _SOentry for this function */
/ } *countbase, **%edx;
/ Assume can destroy %eax and %edx.
/


/ Call with %edx pointing to a private, static cell (initially zero);

/ Answer with that cell (now) pointing to a call count entry,
/  consisting of a ``backpointer to the function'' and an incremented
/  call count (allocate a new one for this fcn if cell was zero).

/ All knowledge of call count entry management is handled in the
/  function _mcountNewent() et. al., which all live in lprof:libprof/common/newmon.c


/ _mcount
/       if  nonzero cell contents       //i.e. points at an entry
/         access entry, increment counter, update _curr_SO and return.
/ 
/       else                            //fcn as of yet has no entry; get 1
/         if  a pointer request is still pending..
/           return without an entry - last request unsatisfied, and
/           a recursive request cannot be satisfied.
/ 
/         else
/           get an entry pointer 
/           if  entry pointer == 0
/             return without an entry - profiling is Off.
/ 
/           else
/             initialize this entry: set backpointer to return address,
/             set count to 1 for this call, set pointer to _SOentry,
/	      update _curr_SO.
/             return
/ 
/           fi
/         fi
/       fi



_mcount:
/ Is there a counter struct addr in *%edx?
	movl	(%edx),%eax	/ %eax = ptr to counter
	testl	%eax,%eax	/ if not 0, have called before:
	jnz     .add1           /      skip initialization


/ No prior call is pending, may call now.  Assumes new cell pointer
/  is returned in %eax.
	
	movl    (%esp),%ecx
	pushl	%edx		/	(save counter ptr)
	pushl	%ecx
	call	_search
	popl    %ecx
	pushl	%eax		/ pointer to shared object entry returned
				/ by _search().
	call	_mcountNewent
	addl	$4,%esp		/ restore top of stack
	popl	%edx		/	(restore counter ptr)

	testl	%eax,%eax	/ if NULL, counting has been disabled:
	jz	.ret		/	get out

/ Allocate new counter
	addl	$4,%eax		/ compute ptr to call count (counter.count)
	movl	%eax,(%edx)	/	and store it in caller's ptr
	movl    (%esp),%edx
	movl	%edx,-4(%eax)	/ set proc addr in counter structure

	movl	$1,(%eax)	/ init call count (skip increment)
	jmp	.ret
.add1:
	incl	(%eax)		/ increment counter
	addl	$4,%eax		/ update _curr_SO
	movl	(%eax),%edx
	movl	%edx,_curr_SO
.ret:
	ret


	.data
	.comm 	_curr_SO,4,4
