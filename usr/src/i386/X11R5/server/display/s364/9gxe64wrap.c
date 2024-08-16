/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/9gxe64wrap.c	1.1"

/***
 ***    NAME
 ***
 ***		9gxe64wrap.c
 ***
 ***    SYNOPSIS
 ***
 ***		9gxe64pro [<delay in sec>]
 ***
 ***    DESCRIPTION
 ***
 ***		This is a standalone utility to fix the wrap around or
 ***	edge duplication problems on #9GXE64pro vision964 based boards.
 ***
 ***    RETURNS
 ***
 ***    MACRO VARIABLES
 ***
 ***
 ***    FILES
 ***
 ***
 ***    SEE ALSO
 ***
 ***    CAVEATS
 ***
 ***    BUGS
 ***
 ***    AUTHORS
 ***
 ***    HISTORY
 ***
 ***/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/kd.h>
#include <sys/errno.h>
#include <sys/inline.h>


/*
 * For the sysi86() call
 */
#include <sys/tss.h>
#include <sys/proc.h>
#include <sys/seg.h>
#include <sys/sysi86.h>


/*
 * v86.h doesn't exist in ESMP
 */
#ifndef SI86IOPL					/* ESMP */
#include <sys/v86.h>
#endif /* SI86IOPL */ 



#if (defined(__STDC__))

/*
 * Rename the function to what is present in the system library.
 */
#define sysi86 _abi_sysi86

/*
 * prototype.
 */
extern int sysi86(int, ...);

#endif /* STDC */





#if (!defined(SET_IOPL))

#ifdef SI86IOPL					/* ESMP */
#define SET_IOPL(iopl) _abi_sysi86(SI86IOPL, (iopl)>>12)
#else  /* SVR-4.2 */
#define SET_IOPL(iopl) _abi_sysi86(SI86V86, V86SC_IOPL, (iopl))
#endif /* SI86IOPL */

#endif /* SET_IOPL */




/***
 *** 	Constants.
 ***/


#define VGA_INPUT_STATUS_ADDRESS 0x3DA



/*
 * Sequencer register access i/o addresses.
 */
#define VGA_SEQUENCER_REGISTER_SEQX				0x3C4 /*(R/W)Sequencer Index*/
#define VGA_SEQUENCER_REGISTER_SEQ_DATA			0x3C5 /*(R/W)Sequencer Data */

/*
 * Sequencer register indices.
 */
#define VGA_SEQUENCER_REGISTER_CLK_MODE_INDEX	0x01  /*(R/W)Clocking Mode  */

/*
 * Clocking Mode Register (Sequencer register) bit definitions.
 */
#define VGA_SEQUENCER_CLK_MODE_SCREEN_OFF	0x20



/*
 * CRTC register i/o addresses.
 */
#define VGA_CRTC_REGISTER_CRTC_ADR_COLOR		0x3D4 /*CRT Controller Index */
#define VGA_CRTC_REGISTER_CRTC_DATA_COLOR		0x3D5 /*CRT Controller Data  */


/*
 *	Required CRTC register indices on Vision964 chip (#9GXE64PRO PCI).
 *	for tri stating vclk.
 */
#define S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT 	0x55 /*Extended DAC Control  */
#define S3_VGA_REGISTER_REG_LOCK2_INDEX 		0x39 /*Register Lock 2       */


/*
 * Extended dac control register CR55 bit definitions.
 */
#define	EX_DAC_CT_TRI_STATE_VCLK				0x80




/***
 *** 	Macros.
 ***/

#define S3_READ_CRTC_REGISTER(CRTC_REGISTER_INDEX,STORE) \
	outb(VGA_CRTC_REGISTER_CRTC_ADR_COLOR, (CRTC_REGISTER_INDEX));\
	STORE = inb(VGA_CRTC_REGISTER_CRTC_DATA_COLOR);

#define S3_WRITE_CRTC_REGISTER(CRTC_REGISTER_INDEX,VALUE) \
	outb(VGA_CRTC_REGISTER_CRTC_ADR_COLOR, (CRTC_REGISTER_INDEX));\
	outb(VGA_CRTC_REGISTER_CRTC_DATA_COLOR, (VALUE));

#define S3_UNLOCK_SYSTEM_REGISTERS() \
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_REG_LOCK2_INDEX,0xa0)

#define S3_READ_SEQUENCER_REGISTER(SEQ_REGISTER_INDEX,STORE) \
	outb( VGA_SEQUENCER_REGISTER_SEQX,(SEQ_REGISTER_INDEX));\
	STORE = inb(VGA_SEQUENCER_REGISTER_SEQ_DATA);			

#define S3_WRITE_SEQUENCER_REGISTER(SEQ_REGISTER_INDEX,VALUE) \
	outb(VGA_SEQUENCER_REGISTER_SEQX,(SEQ_REGISTER_INDEX));\
	outb(VGA_SEQUENCER_REGISTER_SEQ_DATA,(VALUE));			

#define S3_TURN_SCREEN_OFF()\
	{\
		unsigned char 	seq_1;\
		S3_READ_SEQUENCER_REGISTER(VGA_SEQUENCER_REGISTER_CLK_MODE_INDEX,\
			seq_1);\
		seq_1 |= VGA_SEQUENCER_CLK_MODE_SCREEN_OFF;\
		S3_WRITE_SEQUENCER_REGISTER(VGA_SEQUENCER_REGISTER_CLK_MODE_INDEX,\
			seq_1);\
	}

#define S3_TURN_SCREEN_ON()\
	{\
		unsigned char 	seq_1;\
		S3_READ_SEQUENCER_REGISTER(VGA_SEQUENCER_REGISTER_CLK_MODE_INDEX,\
			seq_1);\
		seq_1 &= ~VGA_SEQUENCER_CLK_MODE_SCREEN_OFF;\
		S3_WRITE_SEQUENCER_REGISTER(VGA_SEQUENCER_REGISTER_CLK_MODE_INDEX,\
			seq_1);\
	}


#define S3_WAIT_FOR_VBLANK_INTERVAL()\
{\
	 volatile int __count = 100000;\
	 for(;(__count > 0) && \
		!(inb(VGA_INPUT_STATUS_ADDRESS)&0x8);__count--)\
	 {\
		  ;\
	 }\
}


void tristate_and_enable_vclk();

/*
 * PURPOSE
 *
 *		Routine to fix "wrap around" or "edge duplication"  problem
 *	on #9GXE64pro vision964 based boards.
 *
 *	RETURN VALUE
 *
 *		1 on success
 *		0 on failure
 */
int
main(int argc, char **argv)
{
	/*
	 * Delay between tristating and enabling of vclk.
	 * Fixed at 5 seconds by default.
	 */
	int delay = 5;



	if((argc > 2) || (( argc == 2)  && sscanf(argv[1], "%d", &delay) != 1))
	{
		(void)fprintf(stderr, 
					"\n\tThis utility is to fix the \"wrap around\" or\n"
					"\t\"edge duplication\" problems on #9GXE64pro boards.\n"
					"\n\tThe delay is in the range of 0 to 10 seconds.\n");
		(void)fprintf(stderr, "\n\t\tUsage: %s [<delay in sec>]\n\n", argv[0]);
		exit(0);
	}



	if(delay < 0 || delay >10)
	{
		(void)fprintf(stderr, 
			"\n\tThe delay should to be in the range of 0 to 10 seconds.\n");
		(void)fprintf(stderr, "\n\t\tUsage: %s [<delay in sec>]\n\n", argv[0]);
		exit(0);
	}



	/*
	 * We need access to I/O addresses.  We use the
	 * `sysi86()' call to get access to these.  
	 */
	if (SET_IOPL(PS_IOPL) < 0)
	{
		perror("\nSystem call failed. Can not enable access to I/O ports.\n\n");
		exit(0);
	}


	/*
	 * Call the function to do the job.
	 */
	tristate_and_enable_vclk(delay);


	exit(1);

}

/*
 * PURPOSE
 *
 *		This routine tri states off vclk, waits for the given delay
 *	and enables it again. The delay synchronizes the clock between
 *  the dac(the clock is generated by the PLL in the Ti3025 dac) and 
 *  the chipset.
 *
 * RETURN VALUE
 *
 *		None.
 */
void
tristate_and_enable_vclk(int delay)
{
	unsigned char extended_dac_control;

	S3_TURN_SCREEN_OFF();

	S3_UNLOCK_SYSTEM_REGISTERS();
	
	S3_READ_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT,
		extended_dac_control);

	S3_WAIT_FOR_VBLANK_INTERVAL();

	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT,
		(extended_dac_control | EX_DAC_CT_TRI_STATE_VCLK));

	sleep(delay);
	
	S3_WAIT_FOR_VBLANK_INTERVAL();

	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT,
		(extended_dac_control & ~(EX_DAC_CT_TRI_STATE_VCLK)));

	S3_TURN_SCREEN_ON();

	return;
}
