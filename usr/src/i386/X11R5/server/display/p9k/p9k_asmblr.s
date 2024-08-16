/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

#ident	"@(#)p9k:p9k/p9k_asmblr.s	1.2"

///						-*-Mode: asm -*-	
///	FILE			
///	
///		p9k_asmblr.s
///	
///	SYNOPSIS
///		
///	DESCRIPTION
///	
///		Assembler support routines for the P9000 display
///		library.
///	
///	SEE ALSO
///	
///		p9k_asm.c : prototypes for the functions defined.
///	
///	HISTORY			
///

	
///	GLOBAL M4 DEFINITIONS START
///	
ifdef(`M4DEBUG',`traceon')dnl
define(`PROLOGUE',`
	pushl	%ebp
	movl	%esp,	%ebp
')dnl
define(`EPILOGUE',`
	popl	%ebp
	ret
')dnl
define(`_SAVE_REGISTERS',`ifelse(len($1),0,,`
	pushl	$1
_SAVE_REGISTERS(shift($@))')dnl
')dnl
define(`SAVE_REGISTERS',`
define(`_SAVED_REGISTER_LIST',`$@')dnl
_SAVE_REGISTERS(translit($@,:,`,'))dnl
')dnl
define(`_RESTORE_REGISTERS',`ifelse(len($1),0,,
`_RESTORE_REGISTERS(shift($@))dnl
	popl	$1')
')
define(`RESTORE_REGISTERS',`
_RESTORE_REGISTERS(translit(_SAVED_REGISTER_LIST,:,`,'))dnl
')dnl
define(`FUNCTION',`
undefine(`_SAVED_REGISTER_LIST')dnl
ifdef(`PROFILING',`
	.data
_tmp$1:	.long	0')
	.text
	.globl	$1
	.align	4
$1:
	PROLOGUE
ifdef(`PROFILING',`
	movl	`$'_tmp$1,	%edx
	call	_mcount')

	SAVE_REGISTERS($2)

')dnl
define(`ENDFUNCTION',`
	RESTORE_REGISTERS

	popl	%ebp
	ret
')dnl
	
///	
///	GLOBAL M4 DEFINITIONS END

define(`data_p',8(%ebp))dnl
define(`count',12(%ebp))dnl
define(`pixel8_address',16(%ebp))dnl
define(`source_step',20(%ebp))dnl
define(`height',24(%ebp))dnl

FUNCTION(p9000_asm_memory_to_screen_transfer_helper, %esi:%edi:%ebx)
	/ Setup registers
	movl data_p,		%esi
	movl pixel8_address,	%edi
	movl count,			%ebx
	movl height, 		%edx
loop_on_height:
	leal (%esi,%ebx,4),	%ecx	/ compute fence
do_scanline:
	movl (%esi), 		%eax
	movl %eax,			(%edi)
	addl `$'4,			%esi
	cmpl %ecx,			%esi
	jl do_scanline
	movl data_p,		%esi
	addl source_step,	%esi
	movl %esi,			data_p
	decl %edx
	jg  loop_on_height
ENDFUNCTION
