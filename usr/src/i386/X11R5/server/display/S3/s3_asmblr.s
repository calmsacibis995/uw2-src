/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

#ident	"@(#)S3:S3/s3_asmblr.s	1.2"

//	FILE
//	
//		s3_asmblr.s
//	
//	SYNOPSIS
//		
//	DESCRIPTION
//	
//		Assembler support routines for the S3 display
//		library.
//	
//	SEE ALSO
//	
//		s3_asm.c : prototypes for the functions defined.
//	
//	HISTORY			
//
//	M4 DEFINITIONS START
ifdef(`M4DEBUG',`traceon')dnl
define(`PROLOGUE',`
	pushl	%ebp
	movl	%esp,		%ebp
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
define(`port',8(%ebp))dnl
define(`count',12(%ebp))dnl
define(`pointer_p',16(%ebp))dnl
//	M4 DEFINITIONS END

	.file	"s3_asmblr.s"
// 	packed pixel retrieval functions.
FUNCTION(s3_asm_repz_outl, %esi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%esi
	repz
	outsl
ENDFUNCTION
FUNCTION(s3_asm_repz_outw, %esi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%esi
	repz
	outsw
ENDFUNCTION
FUNCTION(s3_asm_repz_outb, %esi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%esi
	repz
	outsb
ENDFUNCTION
FUNCTION(s3_asm_repz_inl, %edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%edi
	repz
	insl
ENDFUNCTION
FUNCTION(s3_asm_repz_inw, %edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%edi
	repz
	insw
ENDFUNCTION
FUNCTION(s3_asm_repz_inb, %edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%edi
	repz
	insb
ENDFUNCTION

//	Functions for 4 bit mode transfers
FUNCTION(s3_asm_repz_inb_4,%esi:%edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%edi
.inb4loop:
	inb	(%dx)
	movl	%eax,		%esi
	shll	$4,			%eax
	shrl	$4,			%esi
	andl	$0x000F,	%esi
	orl		%esi,		%eax
	stosb
	loop	.inb4loop				
ENDFUNCTION
FUNCTION(s3_asm_repz_inw_4,%esi:%edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%edi
.inw4loop:
	inw	(%dx)
	movl	%eax,		%esi
	shll	$4,			%eax
	shrl	$4,			%esi
	andl	$0xF0F0,	%eax
	andl	$0x0F0F,	%esi
	orl		%esi,		%eax
	stosw
	loop	.inw4loop
ENDFUNCTION
FUNCTION(s3_asm_repz_inl_4,%esi:%edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%edi
.inl4loop:
	inl	(%dx)
	movl	%eax,		%esi
	shll	$4,			%eax
	shrl	$4,			%esi
	andl	$0xF0F0F0F0,%eax
	andl	$0x0F0F0F0F,%esi
	orl		%esi,		%eax
	stosl
	loop	.inl4loop				
ENDFUNCTION
FUNCTION(s3_asm_repz_outb_4,%esi:%edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%esi
.outb4loop:
	lodsb
	movl	%eax,		%edi
	shll	$4,			%eax
	shrl	$4,			%edi
	andl	$0x000F,	%edi
	orl		%edi,		%eax
	outb	(%dx)
	loop	.outb4loop
ENDFUNCTION
FUNCTION(s3_asm_repz_outw_4,%esi:%edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%esi
.outw4loop:
	lodsw
	movl	%eax,		%edi
	shll	$4,			%eax
	shrl	$4,			%edi
	andl	$0xF0F0,	%eax
	andl	$0x0F0F,	%edi
	orl		%edi,		%eax
	outw	(%dx)
	loop	.outw4loop
ENDFUNCTION
FUNCTION(s3_asm_repz_outl_4,%esi:%edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%esi
.outl4loop:
	lodsl
	movl	%eax,		%edi
	shll	$4,			%eax
	shrl	$4,			%edi
	andl	$0xF0F0F0F0,	%eax
	andl	$0x0F0F0F0F,	%edi
	orl		%edi,		%eax
	outl	(%dx)
	loop	.outl4loop
ENDFUNCTION
