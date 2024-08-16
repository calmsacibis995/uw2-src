.ident	"@(#)libc-i386:sys/sleep.s	1.1"


/
/ 
/ unsigned sleep(unsigned seconds)
/	Implements the sleep() function.
/
/ Calling/Exit State:
/	Returns the number of seconds not slept.
/
	.file	"sleep.s"
	
	.text


_fwdef_(`sleep'):
	MCOUNT
	movl	$SLEEP,%eax
	lcall	$0x7,$0
	ret
