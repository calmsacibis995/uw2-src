ifdef(`_UTIL_KSYNCH_P_M4', `',`
define(`_UTIL_KSYNCH_P_M4',`1')
	.ident	"@(#)kern-i386sym:util/ksynch_p.m4	1.4"

/
/ symmetry-dependent assembly-language macros.
/

ifdef(`I486BUG3',`
/
/ On steps of the i486 up to C0 (Beta), we must inhibit interrupts until
/ we know that the SLIC lmask timing window is closed.  Errata #3 for the
/ i486 states that if interrupt is presented to the i486, but is removed
/ before the i486 can generate interrupt acknowledge, the chip will behave
/ in an undefined fashion.  The only way this happens on Symmetry is when
/ the interrupt arrives as the SLIC lmask is written--the interrupt gets
/ droppped when the mask takes effect, potentially before the interrupt
/ is acknowledged.  By hard-masking interrupt on the chip, we cause the
/ i486 to ignore the interrupt line, avoiding the problem entirely.
/

define(`__BLOCK_INTR',`	pushf; cli')
define(`__ALLOW_INTR',`	popf')
',`
define(`__BLOCK_INTR',`')	/ nothing
define(`__ALLOW_INTR',`')	/ nothing
')


/
/ Provide delays for the SLIC; used whenever a new, more restrictive
/ lmask is being loaded.  The actual delays are initialized per-processor
/ in "l.slic_delay" and "l.slic_long_delay" to deal with mixed processor
/ board systems (a mixture of 16/20/25Mhz boards each possibily requiring
/ different * delays).
/
/ See comments in sysinit.c on (1) what the purposes of the delays are
/ and (2) how they are computed.
/
define(`SLIC_DELAY',`
	LABEL(`delay');
	movl    _A_KVPLOCAL+_A_L_SLIC_DELAY, $1;
delay: 	decl    $1;
	jns     delay;
	popdef(`delay');
')

define(`SLIC_LONG_DELAY',`
	LABEL(`delay');
	movl    _A_KVPLOCAL+_A_L_SLIC_LONG_DELAY, $1;
delay: 	decl    $1;
	jns     delay;
	popdef(`delay');
')

/
/ Macro for spl().  It is expected that al has the desired ipl. The 
/ previous ipl is returned in ah. This macro will trash edx.
/
define(`__SPL_ASM',`
	LABEL(`loop',`done')
	movb	KVSLIC_LMASK, %ah;	/ get old interrupt mask
	cmpb	%ah,%al;		/ if (new mask == old mask)
	je	done			/	then quit
loop:
	__BLOCK_INTR;					
	movb	%al, KVSLIC_LMASK;	/ raise to new ipl
	movb	KVSLIC_LMASK, %al;	/ read to sync write
	SLIC_DELAY(%edx)		/ give time for SLIC to settle
	__ALLOW_INTR
	SLIC_LONG_DELAY(%edx)		/ give time for SLIC to accept lower
					/ priority interrupt
	testb	$_A_SL_HARDINT, _A_KVSLIC+_A_SL_ICTL
	je	done
	movb	%ah, KVSLIC_LMASK	/ re-load old SLIC mask
	movb	KVSLIC_LMASK, %ah	/ read to synch write
	jmp	loop
done:
	popdef(`loop',`done')
')

/
/ macro for splx(). The first argument is an 8-bit register containing the 
/ desired ipl.
/
define(`__SPLX_ASM',`
	movb	$1, KVSLIC_LMASK
')

/
/ macro to set a dl_t with the current time.  Helps isolate machine specific
/ code. The first argument is a 32-bit register pointing to a dl_t, the second
/ argument is a 32-bit register that will be trashed. The current time 
/ is stored in the dl_t object. 
/
define(`__TIME_INIT_ASM',`
	movl	_A_KVETC, $2;	/ get the current time
	movl	$2, ($1);	/ set the low-order part
	clrl	0x4($1)		/ zero the high-order part
')

/
/ macro to update the cumulative time in a dl_t, given a starting time
/	and using the current time. The first argument is a pointer to
/ 	the dl_t that will be updated based on the current time and the 
/	time in the dl_t pointed to by the second argument. The third arg.
/	is 32-bit spare register we trash. 
/
define(`__TIME_UPDATE_ASM',`
	movl	_A_KVETC, $3;	/ get the current time
	subl	($2), $3;	/ subtract lop of start time
	addl	$3, ($1);	/ add to lop of cum
	clrl	$3;		/ use a reg. for the adcl to avoid m4isms
	adcl	$3, 0x4($1);	/ add carry bit to hop of cum

')


/
/ macro to block all interrupts regardless of current ipl value
/
define(`__DISABLE_ASM',`
	cli
')

/
/ macro to re-allow interrupts according to the current ipl value
/
define(`__ENABLE_ASM',`
	sti
')

')

