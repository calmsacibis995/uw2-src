	.ident	"@(#)kern-i386sym:proc/cswtch_p.m4	1.2"
	.ident	"$Header: $"

/
/ Platform-specific resume() code for i386sym
/

define(`__RESUME_P',`
	/
	/ Compute and load the global priority into the slic.
	/
	pushl	%ecx
	movl	_A_L_PRI(%ebx),%eax
	movl	slic_prishift,%ecx
	shrl	%cl,%eax
	negl	%eax
	leal	[31<<2](,%eax,4),%eax	/ slic->sl_ipl = 31 -
	movl	%eax,_A_KVSLIC+_A_SL_IPL / ((pri >> slic_prishift) << 2)
	popl	%ecx
')
