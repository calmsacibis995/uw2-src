/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

//						-*-Mode: asm -*-	
//	FILE			
//	
//		m_asmblr.s
//	
//	SYNOPSIS
//		
//	DESCRIPTION
//	
//		Assembler support routines for the MACH display
//		library.
//	
//	SEE ALSO
//	
//		m_asm.c : prototypes for the functions defined.
//	
//	HISTORY			
//

#ident	"@(#)mach:mach/m_asmblr.s	1.3"

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
define(`port',8(%ebp))dnl
define(`count',12(%ebp))dnl
define(`pointer_p',16(%ebp))dnl
//	M4 DEFINITIONS END

	.file	"mach_asm_assembler.s"
// 	packed pixel retrieval functions.
FUNCTION(mach_asm_repz_outl, %esi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%esi
	repz
	outsl
ENDFUNCTION
FUNCTION(mach_asm_repz_outw, %esi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%esi
	repz
	outsw
ENDFUNCTION
FUNCTION(mach_asm_repz_outb, %esi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%esi
	repz
	outsb
ENDFUNCTION
FUNCTION(mach_asm_repz_inl, %edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%edi
	repz
	insl
ENDFUNCTION
FUNCTION(mach_asm_repz_inw, %edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%edi
	repz
	insw
ENDFUNCTION
FUNCTION(mach_asm_repz_inb, %edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%edi
	repz
	insb
ENDFUNCTION

//	Functions for 4 bit mode transfers
//	These functions work for LSB first transfers
FUNCTION(mach_asm_repz_inb_4,%esi:%edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%edi
.inb4loop:
	inb	(%dx)
	movl	%eax,		%esi
	inb	(%dx)
	shll	$4,		%eax
	orl	%esi,		%eax
	stosb
	loop	.inb4loop				
ENDFUNCTION
FUNCTION(mach_asm_repz_inw_4,%edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%edi
	shll	$1,		%ecx	// count *= 2
.inw4loop:
	inw	(%dx)
	shlb	$4,		%ah
	orb	%ah,		%al
	stosb
	loop	.inw4loop
ENDFUNCTION
FUNCTION(mach_asm_repz_inl_4,%esi:%edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%edi
	shll	$1,		%ecx	// count *= 2
.inl4loop:
	inl	(%dx)
	movl	%eax,		%esi
	shlb	$4,		%ah
	orb	%ah,		%al
	xchgl	%eax,		%esi
	andl	$0x00FF,	%esi
	shrl	$16,		%eax
	shlb	$4,		%ah
	orb	%ah,		%al
	shll	$8,		%eax
	orl	%esi,		%eax
	stosw
	loop	.inl4loop				
ENDFUNCTION
FUNCTION(mach_asm_repz_outb_4,%esi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%esi
.outb4loop:
	lodsb
	outb	(%dx)
	shrl	$4,		%eax
	outb	(%dx)
	loop	.outb4loop
ENDFUNCTION
FUNCTION(mach_asm_repz_outw_4,%esi:%edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%esi
.outw4loop:
	lodsw
	movl	%eax,		%edi
	movb	%al,		%ah
	shrb	$4,		%ah
	andl	$0x0F0F,	%eax
	outw	(%dx)
	movl	%edi,		%eax
	shrl	$8,		%eax
	movb	%al,		%ah
	shrb	$4,		%ah
	andl	$0x0F0F,	%eax
	outw	(%dx)
	loop	.outw4loop
ENDFUNCTION
FUNCTION(mach_asm_repz_outl_4,%esi:%edi)
	movl	port,		%edx
	movl	count,		%ecx
	movl	pointer_p,	%esi
	shll	$1,		%ecx	// count *= 2
.outl4loop:
	lodsw
	movl	%eax,		%edi
	movb	%al,		%ah
	shrb	$4,		%ah
	xchgl	%eax,		%edi
	shrl	$8,		%eax
	movb	%al,		%ah
	shrb	$4,		%ah
	shll	$16,		%eax
	orl	%edi,		%eax
	andl	$0x0F0F0F0F,	%eax
	outl	(%dx)
	loop	.outl4loop
ENDFUNCTION

//
//	Register addresses
//
define(`MACH_POINT_DRAW_COMMAND',``$''0x1100)dnl
define(`MACH_REGISTER_CUR_X',``$''34536)dnl		// 0x86E8
define(`MACH_REGISTER_CUR_Y',``$''33512)dnl		// 0x82E8
define(`MACH_REGISTER_EXT_SHORT_STROKE',``$''50926)dnl	// 0xC6EE
define(`MACH_REGISTER_SCAN_X',``$''51950)dnl		// 0xCAEE
define(`MACH_REGISTER_CMD',``$''0x9AE8)
define(`MACH_REGISTER_RD_MASK',``$''0xAEE8)
define(`MACH_REGISTER_AXSTP',``$''0x8AE8)
define(`MACH_REGISTER_DIASTP',``$''0x8EE8)
			
//
// 	Point plotting routine
//
//	mach_asm_plot_points(int count, short *points_p)
//
//	
define(`MACH_ASM_PLOT_POINTS_LOOP_CODE',`
	lodsl					// bring in X, Y as two shorts
	movl	%eax,			%edi	// save X coordinate
	shrl	`$'16,			%eax	// bring Y coordinate to %ax
	movl	MACH_REGISTER_CUR_Y,	%edx
	outw	(%dx)				// program Y
	movl	%edi,			%eax
	movl	MACH_REGISTER_CUR_X,	%edx	// program X
	outw	(%dx)
	incl	%eax				// to next point
	movl	MACH_REGISTER_SCAN_X, 	%edx
	outw	(%dx)
	loop	.asm_plot_points_loop
')

FUNCTION(mach_asm_plot_points,%esi:%edi)

	movl	8(%ebp),	%ecx
	movl	12(%ebp),	%esi

.asm_plot_points_loop:
	//
	// wait for 3 FIFO entries
	//

	cmpl	$3,	mach_graphics_engine_number_of_fifo_entries_free
	jl	.asm_plot_points_get_entries

	subl	$3,	mach_graphics_engine_number_of_fifo_entries_free

	MACH_ASM_PLOT_POINTS_LOOP_CODE

	jmp	.asm_plot_points_end

.asm_plot_points_get_entries:

	pushl	%ecx			// save loopcount
	pushl	$3			// 
	call	mach_register_wait_for_fifo
	addl	$4,		%esp	// pop parameter
	popl	%ecx			// restore loopcount

	MACH_ASM_PLOT_POINTS_LOOP_CODE
	
.asm_plot_points_end:
ENDFUNCTION

//	Drawing a set of solid fill spans.
// 	This is a common operation under WABI.
//
define(`MACH_ASM_SCAN_X_CODE',`
	movl	MACH_REGISTER_CUR_X,	%edx
	outw	(%dx)
	pushl	%eax
	shrl	`$'16,			%eax
	movl	MACH_REGISTER_CUR_Y,	%edx
	outw	(%dx)
	popl	%eax
	addl	%ebx,			%eax
	movl	MACH_REGISTER_SCAN_X,	%edx
	outw	(%dx)
')

FUNCTION(mach_asm_draw_spans_solid,%esi:%edi:%ebx)

	movl	8(%ebp),	%esi	// points_p
	movl	12(%ebp),	%edi	// widths_p
	movl	16(%ebp),	%ecx	// count

.asm_scan_x_next_span:

	lodsl			// bring in X, Y as two shorts
	movl	(%edi),			%ebx
	addl	`$'4,			%edi
	testl	%ebx,			%ebx	// check for zero
	jle	.asm_scan_x_continue

	//
	// wait for 3 FIFO entries
	//
	cmpl	$3,	mach_graphics_engine_number_of_fifo_entries_free
	jl	.asm_scan_x_get_entries

	subl	$3,	mach_graphics_engine_number_of_fifo_entries_free

	MACH_ASM_SCAN_X_CODE

.asm_scan_x_continue:
	decl	%ecx
	jg	.asm_scan_x_next_span
	jmp	.asm_scan_x_end

.asm_scan_x_get_entries:

	pushl	%ecx			// save loopcount
	pushl	%eax			// save *points_p
	pushl	$3			// 
	call	mach_register_wait_for_fifo
	addl	$4,		%esp	// pop parameter
	popl	%eax			// restore *points_p
	popl	%ecx			// restore loopcount

	MACH_ASM_SCAN_X_CODE

	decl	%ecx
	jg	.asm_scan_x_next_span

.asm_scan_x_end:

ENDFUNCTION

// IBM blit code for terminal fonts
//
// The size of each `mach_glyph'.
ifdef(`__DEBUG__',
`define(`SIZEOF_MACH_GLYPH',32)'
`define(`SIZEOF_MACH_GLYPH_SHIFT',5)',
`define(`SIZEOF_MACH_GLYPH',32)'
`define(`SIZEOF_MACH_GLYPH_SHIFT',5)')dnl
	
define(`OFFSET_OF_GLYPH_PLANEMASK',0)		/ ibm_mode_planemask
define(`OFFSET_OF_CACHED_X_COORDINATE',4)	/ ibm_mode_x_coordinate
	
// Other definitions	
define(`MACH_ASM_IBM_TEXT_BLIT_COMMAND',``$''0xC0F3)dnl
define(`font_glyphs_p',8(%ebp))
define(`glyph_list_start_p',12(%ebp))
define(`glyph_list_end_p',16(%ebp))
define(`current_x',20(%ebp))
define(`y_top',24(%ebp))
define(`width',28(%ebp))
	
FUNCTION(mach_asm_ibm_blit_text,%ebx:%esi:%edi)

	// prologue
	movl	glyph_list_end_p,	%ecx	/ fence_p
	xorl	%edi,			%edi	/ current planemask
	movl	glyph_list_start_p,	%esi	/

	.align	4	
.asm_ibm_blit_loop:

	// clear %eax	
	xorl	%eax,			%eax
	
	//	base of the font glyphs array	
	movl	font_glyphs_p,		%ebx
	
	// Get the pointer to the current glyph
	lodsw					/ %eax has the index 
						/ of the glyph
	// multiply by the sizeof the glyph
	shll	`$'SIZEOF_MACH_GLYPH_SHIFT,	%eax

	// retrieve pointer to the glyph
	leal	(%ebx,%eax),	%ebx		/ %ebx == mach_glyph_p
	
	// Main loop to blit a glyph

	// compare this glyphs planemask to the current planemask
	cmpl	OFFSET_OF_GLYPH_PLANEMASK`'(%ebx),	%edi
	jne	.asm_ibm_blit_change_planemask
	
.asm_ibm_blit_change_planemask_resume:
	
	subl	`$4',	mach_graphics_engine_number_of_fifo_entries_free
	
		// get the current X register (interleaved execution)
	movw	MACH_REGISTER_CUR_X,	%dx
	
	jl	.asm_ibm_blit_get_fifo_entries	/ from the `subl'

.asm_ibm_blit_get_fifo_entries_resume:
	
	// Glyphs ibm mode x-coordinate : should be in the cache line
	// due to preceding fetch of the `glyph_planemask'
	movl 	OFFSET_OF_CACHED_X_COORDINATE`'(%ebx), %eax
	
		// Get `width' (interleave execution)
	movl	width,			%ebx
	
	outw	(%dx)	// out to CUR_X
	
	// destination X
	movl	current_x,		%eax
	movw	MACH_REGISTER_DIASTP,	%dx
	outw	(%dx)
	addl	%eax,			%ebx
	
	// Reprogram Y_TOP
	movl	y_top,			%eax
	movw	MACH_REGISTER_AXSTP,	%dx
	outw	(%dx)
	
	// The blit command
	movw 	MACH_ASM_IBM_TEXT_BLIT_COMMAND,	%ax
	movw	MACH_REGISTER_CMD,	%dx
	outw	(%dx)

	// End of one glyph -- loop to the next
	cmpl	%ecx,			%esi
	movl	%ebx,			current_x	/ Update current_x
	jb	.asm_ibm_blit_loop			/ from the `cmpl'
	jmp	.asm_ibm_blit_end_function
	
	.align	4
.asm_ibm_blit_get_fifo_entries:	
	pushl	%ecx
	pushl	%edx
	pushl	`$'4
	call	mach_register_wait_for_fifo
	addl	`$'4,			%esp
	popl	%edx
	popl	%ecx
	jmp	.asm_ibm_blit_get_fifo_entries_resume
	
	.align	4		
.asm_ibm_blit_change_planemask:
	
	cmpl	$1,	mach_graphics_engine_number_of_fifo_entries_free
	jl	.asm_ibm_blit_call_wait_for_fifo
	subl	$1,	mach_graphics_engine_number_of_fifo_entries_free
	
.asm_ibm_blit_call_wait_for_fifo_resume:

	movl	OFFSET_OF_GLYPH_PLANEMASK`'(%ebx),	%eax
	movw	MACH_REGISTER_RD_MASK, 	%dx
	outw	(%dx)
	movl	%eax,			%edi		/ save this planemask
	jmp	.asm_ibm_blit_change_planemask_resume

	.align	4
.asm_ibm_blit_call_wait_for_fifo:		
	pushl	%ecx
	pushl	$1	
	call	mach_register_wait_for_fifo
	addl	$4,			%esp
	popl	%ecx
	jmp	.asm_ibm_blit_call_wait_for_fifo_resume

.asm_ibm_blit_end_function:
	
ENDFUNCTION	
	
// Local Variables:	
// asm-comment-char:	?/
// End:
