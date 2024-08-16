ifdef(`_UTIL_KSYNCH_P_M4', `',`
define(`_UTIL_KSYNCH_P_M4',`1')
	.ident	"@(#)kern-i386at:util/ksynch_p.m4	1.12"
	.ident	"$Header: $"

/
/ Platform dependent assembly-language macros.
/


/
/ MACRO
/ __SPL_ASM
/	Set priority level (new pl greater than or equal to current pl)
/ 
/ Calling/Exit State:
/	On entry, the new priority level is passed in %eax
/	On exit, the previous ipl is in %eax
/
/ Remarks:
/	This macro is allowed to trash %edx, but this implementation
/	does not.
/
/	The new priority level specified in %eax is presumed to be
/	greater than or equal to the old pl.
/
/	This macro is used to generate NON-DEBUG version of spin locks
/	and both DEBUG and non-DEBUG versions of read/write locks.
/
define(`__SPL_ASM',`
ifdef(`UNIPROC',`
	movb	%al, %dl
	movb	ipl, %al
	movb	%dl, ipl
',`
	pushl	%ecx
	pushl	%eax
	call	spl
	addl	$ 4, %esp
	popl	%ecx
')
')

/
/ MACRO
/ __SPLX_ASM(newpl)
/	Set priority level (new pl less than or equal to current pl)
/
/ Calling/Exit State:
/	On entry, the new priority level is passed in %eax
/	On exit, the previous ipl is in %eax
/
/ Remarks:
/	This macro trashes %edx.
/
/	This macro is used to generate NON-DEBUG version of spin locks
/	and both DEBUG and non-DEBUG versions of read/write locks.
/
define(`__SPLX_ASM',`
ifdef(`UNIPROC',`
	LABEL(`done')
	movl	%eax, %edx
	movb	ipl, %al
	movb	%dl, ipl
	cmpl	%edx, picipl
	jbe	done
	call	splunwind
done:
	popdef(`done')
',`
	pushl	%ecx
	pushl	%eax
	call	splx
	addl	$ 4, %esp
	popl	%ecx
')
')

/
/ MACRO
/ __TIME_INIT_ASM(dl_t *dltp, tmp-reg)
/	Set a dl_t with the current time.
/
/ Calling/Exit Conditions:
/	dltp is a 32-bit register argument and points to the dl_t 
/	object to be initialized.
/	tmp-reg is a 32-bit register that may be trashed.
/
/ Remarks:
/	It is necessary to push %eax, %ecx and %edx registers
/	because the assumption is that the macro arguments are 
/	passed in these registers which can be trashed by the
/	call to psm_usec_time().
/
/	Used by lock code to maintain stats. Helps isolate machine
/	specific code because different platforms may provide
/	different mechanism to provide microsecond resolution clock
/	value. For example, the Sequent platform provide a free
/	running memory mapped microsecond clock that can be read
/	whereas the AT architecture require to program the 8254
/	programmable interrupt timer chip to get the microsecond
/	clock value. 
/
/ PERF:
/	It is possible to optimize the macro by bypassing the
/	saving of all the arguments and the %eax, %ecx and %edx.
/
define(`__TIME_INIT_ASM',`
	pushl	$1
	pushl	$2
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	call	psm_usec_time	/ get the current time
	movl	%eax, 12(%esp)	/ save the return value in temp reg.
	popl	%edx
	popl	%ecx
	popl	%eax
	popl	$2
	popl	$1
	movl	$2, ($1)	/ set the low-order part
	clrl	0x4($1)		/ zero the high-order part
')

/
/ MACRO
/ __TIME_UPDATE_ASM(dl_t *cum_timep, dl_t *start_timep, tmp-reg)
/	Update the cumulative time in a cum_timep, given a starting
/	time and using the current time.
/
/ Calling/Exit Conditions:
/	The cum_timep is a 32-bit register pointing to a dl_t object
/		which contains a cumulative time.
/	The start_timep is a 32-bit register pointing to a dl_t object
/		which contains a start time.
/	The tmp-reg is a 32-bit register which may be trashed.
/
/ Remarks:
/	It is necessary to save the three arguments because
/	the assumption is that the macro arguments are passed
/	in %eax, %ecx, %edx which can be trashed by the call
/	to psm_usec_time().
/
/	Used by lock code to maintain stats. Helps isolate machine
/	specific code because different platforms may provide
/	different mechanism to provide microsec. resolution clock
/	value. For example, the Sequent platform provide a free
/	running memory mapped microsec. clock that can be read
/	whereas the AT architecture require to program the 8254
/	programmable interrupt timer chip to get the microsec.
/	clock value. 
/
/ PERF:
/	It is possible to optimize the macro by bypassing the
/	saving of all the arguments in the %eax, %ecx and %edx.
/
define(`__TIME_UPDATE_ASM',`
	pushl	$1
	pushl	$2
	pushl	$3		/ push all the arguments on stack
	call	psm_usec_time	/ get the current timers counter value
	movl	%eax, (%esp)	/ save the return value in temp reg.
	popl	$3
	popl	$2
	popl	$1
	subl	($2), $3	/ subtract lop of start time
	addl	$3, ($1)	/ add to lop of cum
	clrl	$3		/ use a reg. for the adcl to avoid m4isms.
	adcl	$3, 0x4($1)	/ add carry bit to hop a sum
')

/
/ MACRO
/ __DISABLE_ASM
/	Block all interrupts regardless of current ipl value
/
define(`__DISABLE_ASM',`
	cli
')

/
/ MACRO
/ __ENABLE_ASM
/	Allow interrupt according to the current ipl value.
/
define(`__ENABLE_ASM',`
	LABEL(`done')
	cmpl	`$'0, shadow_if
	je	done
	sti
done:
	popdef(`done')
')

')
