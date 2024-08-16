/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/dos_util/pci_dump.c	1.1"

/***
 ***	NAME
 ***			pci_dump.c
 ***		
 ***	SYNOPSIS
 ***
 ***
 ***	DESCRIPTION
 ***	
 ***		DOS Utility for reading the P9000 and DAC base addresses.
 ***
 *** 		This utility uses the standard PCI BIOS function calls. First
 ***		it attempts to find a matching device (DEVICE_ID=0x9001 and
 ***		VENDOR_ID =  0x100E), if this succeeds then it reads the
 ***		configuration words at 0x14,0x16,0x10,0x12 offsets to using
 ***		the READ_CONFIG_WORD sub-function.
 ***
 ***		This code can be compiled on any version of Turbo C or MSC
 ***		compiler or any DOS C compiler with the int86 library call.
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***
 ***	FILES
 ***
 ***
 ***	SEE ALSO
 ***
 ***              PCI BIOS specs
 ***              POWER 90001 PCI BUS BOARD App note
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	AUTHORS
 ***
 ***		
 ***
 ***	HISTORY
 ***
 ***		27_June_1994 Created
 ***
 ***/

#include <stdio.h>
#include <dos.h>


/*
 * Some useful constants
 */

#define 	PCI_BIOS_INTERRUPT_NUMBER				0x1AU
#define		PCI_FUNCTION_ID							0xB1U

/*
 * PCI BIOS sub-function codes
 */

#define 	PCI_BIOS_PRESENT						0x1U
#define		PCI_FIND_DEVICE							0x2U
#define		PCI_READ_CONFIG_WORD					0x9U
#define		PCI_WRITE_CONFIG_WORD					0x0CU


/*
 * P9000  specific data
 */

#define		PCI_DEVICE_ID							0x9001
#define		PCI_VENDOR_ID							0x100E

#define		PCI_P9000_BASE_REGISTER					0x10
#define		PCI_P9000_DAC_BASE_REGISTER				0x14

/*
 * BIOS call return codes
 */

#define		SUCCESSFUL								0


unsigned int bx_value = 0;

/*
 * Returns 1 if device found and suitably initailizes bus no,
 * and device number variables (bx value)
 */

int
find_device()
{
	union REGS in_regs;
	union REGS out_regs;


	in_regs.x.ax =  (PCI_FUNCTION_ID << 8) |  PCI_FIND_DEVICE;

	in_regs.x.cx = PCI_DEVICE_ID;
	in_regs.x.dx = PCI_VENDOR_ID;

	in_regs.x.si = 	0;

	int86(PCI_BIOS_INTERRUPT_NUMBER, &in_regs, &out_regs);

	if (out_regs.h.ah != SUCCESSFUL)
	{
		fprintf(stderr,"Unable to find PCI device\n");
		return 0;
	}
	

	bx_value = out_regs.x.bx;


#if defined(DEBUG)
	printf("bx_value = %x\n", bx_value);
#endif
	
	return 1;

}

unsigned int read_config_word(int word_no)
{
	union REGS in_regs;
	union REGS out_regs;

	in_regs.x.ax =  (PCI_FUNCTION_ID << 8) |  PCI_READ_CONFIG_WORD;
	in_regs.x.bx =  bx_value;
	in_regs.x.di =  word_no;
	
	int86(PCI_BIOS_INTERRUPT_NUMBER, &in_regs, &out_regs);

	if (out_regs.h.ah != SUCCESSFUL)
	{
		fprintf(stderr,"Error reading configuration word: %x\n",
			word_no);
	}

#if defined(DEBUG)
	printf("Config_word(%x) = %x\n", word_no,out_regs.x.cx);
#endif

	return out_regs.x.cx;

}

void write_config_word(int word_no, unsigned int data)
{
	union REGS in_regs;
	union REGS out_regs;

	in_regs.x.ax =  (PCI_FUNCTION_ID << 8) |  PCI_WRITE_CONFIG_WORD;
	in_regs.x.bx =  bx_value;
	in_regs.x.di =  word_no;
	in_regs.x.cx = data;
	
	int86(PCI_BIOS_INTERRUPT_NUMBER, &in_regs, &out_regs);

	if (out_regs.h.ah != SUCCESSFUL)
	{
		fprintf(stderr,"Error writing configuration word: %x\n",
			word_no);
	}


}

main (int argc, char **argv)
{
	unsigned int p9000_base_hi, p9000_base_lo;
	unsigned int dac_base_hi, dac_base_lo;
	extern char *optarg;
	unsigned long data;
	extern int optind;
	int c;


	if (!find_device())
	{
		exit(0);
	}
	
	if (argc == 1)
	{
		p9000_base_lo = read_config_word(PCI_P9000_BASE_REGISTER);
		p9000_base_hi = read_config_word(PCI_P9000_BASE_REGISTER + 2); 

		printf("P9000 BASE ADDRESS: 0x%04x%04x\n",
			p9000_base_hi,p9000_base_lo);

		dac_base_lo = read_config_word(PCI_P9000_DAC_BASE_REGISTER);
		dac_base_hi = read_config_word(PCI_P9000_DAC_BASE_REGISTER + 2); 
		
		printf("DAC   BASE ADDRESS: 0x%04x%04x\n",
			dac_base_hi,dac_base_lo & 0xF000 );
	}
	else
	{
		if (strcmp(argv[1],"-p") == 0)
		{
			if (argc > 2)
			{
					sscanf(argv[2],"%i", &data);
					write_config_word(PCI_P9000_BASE_REGISTER,
						data & 0xFFFF);
					write_config_word(PCI_P9000_BASE_REGISTER+2,
						data >> 16);
			}
			else
			{
				fprintf(stderr,"Insufficient no of args\n");
			}
		}
		
		if (strcmp(argv[1],"-d") == 0)
		{
			if (argc > 2)
			{
					sscanf(argv[2],"%i", &data);
					write_config_word(PCI_P9000_DAC_BASE_REGISTER,
						data & 0xFFFF);
					write_config_word(PCI_P9000_DAC_BASE_REGISTER+2,
						data >> 16);
			}
			else
			{
				fprintf(stderr,"Insufficient no of args\n");
			}
		}
	}

}

