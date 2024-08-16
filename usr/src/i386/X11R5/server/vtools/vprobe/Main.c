/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vtools:vprobe/Main.c	1.10"

/*
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: mit/server/ddx/x386/SuperProbe/Main.c,v 2.4 1993/10/07 13:54:46 dawes Exp $ */

#include "Probe.h"
#include "../common.h"

char MyName[20];
Word vgaIOBase;
Bool Verbose = FALSE;
Byte Chip_data = 0xFF;
Byte *Bios_Base = (Byte *)0;
Bool NoBIOS = FALSE;

static Bool No16Bits = FALSE;
static Bool Exclusions = FALSE;
static Bool Mask10 = FALSE;
static Range *Excl_List = NULL;
static char *NoProbe = (char *)0;

static char *StdStuff = "MDA, Hercules, CGA, MCGA, EGA, VGA";

/* 
 * Since there is no better way to do than cycling thru each detection
 * routine, let us limit it to only those we support - but for stand-alone
 * 'vprobe' utility, we let it go through all the routines.
 * Blindly going through all the routines is causing some side-effects
 * on some of the latest graphics chips. 8/31/94
 */ 
static Chip_Descriptor *SVGA_Descriptors[] = {
    &S3_Descriptor,
    &Tseng_Descriptor,
    &Cirrus_Descriptor,
    &ATI_Descriptor,
    &Trident_Descriptor,
    &WD_Descriptor,
    &Compaq_Descriptor,
    &NCR_Descriptor,
#ifndef _VPROBE_FUNC_
    &Ahead_Descriptor,
    &MX_Descriptor,
    &AcuMos_Descriptor,
    &RealTek_Descriptor,
    &Primus_Descriptor,
    &Yamaha_Descriptor,
    &Oak_Descriptor,
    &Genoa_Descriptor,
    &Video7_Descriptor,
    &CT_Descriptor, /* I think this is screwing people up, so put it last */
#endif /* _VPROBE_FUNC_ */
#if 0
    &AL_Descriptor, /* I think this is dangerous, & no one has heard of it */
#endif
    NULL
};

static Chip_Descriptor *CoProc_Descriptors[] = {
    &ATIMach_Descriptor,
    &IBM8514_Descriptor,	/* Make this the last 8514-type entry */
    NULL
};

#ifdef __STDC__
static int num_tokens(char *list, char delim)
#else
static int num_tokens(list, delim)
char *list;
char delim;
#endif
{
    char *p = list;
    int cnt = 1;

    while ((p = strchr(p, delim)) != NULL)
    {
    	p++;
    	cnt++;
    }
    return(cnt);
}

#ifdef __STDC__
static void ParseExclusionList(Range *excl_list, char *list)
#else
static void ParseExclusionList(excl_list, list)
Range *excl_list;
char *list;
#endif
{
    char *p = list, *p1, c;
    Bool done = FALSE, lo = TRUE;
    int i = 0;

    while (!done)
    {
    	p1 = strpbrk(p, "-,");
    	if (p1)
    	{
    	    c = *p1;
    	    *p1 = '\0';
    	    if ((c ==  '-') && (p == p1))
    	    {
    	    	fprintf(stderr, "%s: Unbounded range in exclusion\n", MyName);
    	    	exit(1);
    	    }
    	    else if (p == p1)
    	    {
    	    	p++;
    	    	continue;
    	    }
    	}
    	else
    	{
    	    if ((!lo) && (p == p1))
    	    {
    	    	fprintf(stderr, "%s: Unbounded range in exclusion\n", MyName);
    	    	exit(1);
    	    }
    	    done = TRUE;
    	}
    	if (lo)
    	{
    	    excl_list[i].lo = (Word)StrToUL(p);
    	}
    	else
    	{
    	    excl_list[i].hi = (Word)StrToUL(p);
    	}
    	if (!done)
    	{
    	    if (c == '-')
    	    {
    	    	lo = FALSE;
    	    }
    	    else
    	    {
    	    	if (lo)
    	    	{
    	    	    excl_list[i].hi = (Word)-1;
    	    	}
    	    	else
    	    	{
    	    	    lo = TRUE;
    	    	}
    	    	i++;
    	    }
    	    p = p1 + 1;
    	}
    	else
    	{
    	    if (lo)
    	    {
    	    	excl_list[i].hi = (Word)-1;
    	    }
    	    i++;
    	}
    }
    excl_list[i].lo = (Word)-1;
}

#ifdef __STDC__
static Bool TestChip(Chip_Descriptor *chip_p, int *Chipset)
#else
static Bool TestChip(chip_p, Chipset)
Chip_Descriptor *chip_p;
int *Chipset;
#endif
{
    char *p, *p1, name[64];

    if ((No16Bits) && (chip_p->bit16))
    {
    	if (Verbose)
    	{
    	    printf("\tSkipping %s (16-bit registers)...\n", chip_p->name);
    	    fflush(stdout);
    	}
    	return(FALSE);
    }
    if ((NoBIOS) && (chip_p->uses_bios))
    {
	if (Verbose)
	{
	    printf("\tSkipping %s (needs BIOS read)...\n", chip_p->name);
	    fflush(stdout);
	}
	return(FALSE);
    }
    if ((Exclusions) && (Excluded(Excl_List, chip_p, Mask10)))
    {
    	if (Verbose)
    	{
    	    printf("\tskipping %s (exclusion list)...\n", chip_p->name);
    	    fflush(stdout);
    	}
    	return(FALSE);
    }
    if (NoProbe != (char *)0)
    {
	p1 = p = NoProbe;
    	while (p)
    	{
    	    p1 = strchr(p1, ',');
    	    if (p1 != NULL)
    	    {
    	    	(void)strncpy(name, p, (p1-p));
		name[p1-p] = '\0';
		p1++;
    	    }
	    else
	    {
		(void)strcpy(name, p);
	    }
	    if (StrCaseCmp(name, chip_p->name) == 0)
	    {
		if (Verbose)
    		{
    	    	    printf("\tskipping %s (noprobe list)...\n", chip_p->name);
    	    	    fflush(stdout);
    		}
    		return(FALSE);
	    }
    	    p = p1;
	}
    }
    if (Verbose)
    {
    	printf("\tProbing %s...\n", chip_p->name);
    	fflush(stdout);
    }
    if (chip_p->f(Chipset))
    {
    	return(TRUE);
    }
    return(FALSE);
}

#ifdef __STDC__
static void PrintInfo(void)
#else
static void PrintInfo()
#endif
{
    Chip_Descriptor *chip_p;
    int i, len;

    PRINT_VERSION;
    putchar('\n');
    printf("%s can detect the following standard video hardware:\n", MyName);
    printf("\t%s\n", StdStuff);
    printf("%s can detect the following SVGA chipsets/vendors:\n", MyName);
    len = 0;
    putchar('\t');
    for (i=0; SVGA_Descriptors[i] != NULL; i++)
    {
        chip_p = SVGA_Descriptors[i];
	len += strlen(chip_p->name) + 2;
	if (len > 70)
	{
	    printf("\n\t");
	    len = strlen(chip_p->name) + 2;
	}
        printf("%s, ", chip_p->name);
    }
    putchar('\n');
    printf("%s can detect the following graphics coprocessors/vendors:\n", 
	   MyName);
    len = 0;
    putchar('\t');
    for (i=0; CoProc_Descriptors[i] != NULL; i++)
    {
        chip_p = CoProc_Descriptors[i];
	len += strlen(chip_p->name) + 2;
	if (len > 70)
	{
	    printf("\n\t");
	    len = strlen(chip_p->name) + 2;
	}
        printf("%s, ", chip_p->name);
    }
    putchar('\n');
}

#ifdef __STDC__
static Byte *FindBios(void)
#else
static Byte *FindBios()
#endif
{
    int i, score[7];
    Byte buf[3];
    Byte *base = (Byte *)0;

    for (i=0; i < 7; i++)
    {
	score[i] = 1;
    }
    for (i=0; i < 7; i++)
    {
	buf[0] = buf[1] = buf[2] = (Byte)0;
	if (ReadBIOS((unsigned)(0xC0000+(i<<15)), buf, 3) != 3)
	{
	    score[i] = 0;
	}
	else if ((buf[0] != 0x55) || (buf[1] != 0xAA) || (buf[2] < 48))
	{
	    score[i] = 0;
	}
    }
    for (i=6; i >= 0; i--)
    {
	if (score[i] != 0)
	{
	    base = (Byte *)(0xC0000+(i<<15));
	}
    }
    return(base);
}

#if (defined(SVR4) && defined(i386))
#include <stdio.h>
#include <sys/tss.h>
#include <sys/proc.h>
#include <sys/seg.h>
#include <sys/sysi86.h>
/*
 * v86.h doesn't exist in ESMP
 */
#ifndef SI86IOPL					/* ESMP */
#include <sys/v86.h>
#endif

#define sysi86 _abi_sysi86

/*
 * prototype.
 */
extern int sysi86(int, ...);

#ifdef SI86IOPL					/* ESMP */
#define SET_IOPL(iopl) _abi_sysi86(SI86IOPL, (iopl)>>12)
#else  /* SVR-4.2 */
#define SET_IOPL(iopl) _abi_sysi86(SI86V86, V86SC_IOPL, (iopl))
#endif
#endif

int
enable_video()
{
	/*
	 * To have accesses to many I/O addresses, we use the
	 * `sysi86()' call to get access to these. 
	 */
	if (SET_IOPL(PS_IOPL) < 0)
	{
		fprintf(stderr,"Can not enable video \n");
		return -1;
	}
	return 1;
}

disable_video()
{
	SET_IOPL(0);
}

#ifndef _VPROBE_FUNC_

#ifdef __STDC__
int main(int argc, char *argv[])
#else
int main(argc, argv)
int argc;
char *argv[];
#endif

#else

int
vprobe(BoardInfoRec *boardinfo)
#endif
{
    char *p, *order = NULL;
    Byte copyright[3];
    Word Ports[10];		/* For whatever we need */
    int Primary = -1;
    int Secondary = -1;
    int RamDac = -1;
    int CoProc = -1;
    int i, cnt;
    Chip_Descriptor defaultMatched;
    Chip_Descriptor *chip_p;
    Chip_Descriptor *matched = &defaultMatched;
    Bool flag;
    extern char *SVGA_Names[NUM_VENDORS+1][CHPS_PER_VENDOR];
    extern char *RamDac_Names[];

    matched->m = NullEntry;

#ifndef _VPROBE_FUNC_

    p = strrchr(argv[0], '/');
    if (p == NULL)
    	p = argv[0];
    else
    	p++;
    (void)strcpy(MyName, p);

    for (i=1; i < argc; i++)
    {
    	if (strncmp(argv[i], "-v", 2) == 0)
    	{
    	    Verbose = TRUE;
    	    continue;
    	}
    	else if (strncmp(argv[i], "-no16", 5) == 0)
    	{
    	    No16Bits = TRUE;
    	    continue;
    	}
    	else if ((strncmp(argv[i], "-excl", 5) == 0) || 
    		 (strncmp(argv[i], "-x", 2) == 0))
    	{
	    if (Excl_List != NULL)
	    {
		fprintf(stderr, 
			"%s: Warning - only one exclusion list allowed. ",
			MyName);
		fprintf(stderr, " The last one will be used\n");
	    }
    	    i++;
	    if (i == argc)
	    {
		fprintf(stderr, "%s: Exclusion list not specified\n", MyName);
		return(1);
	    }
    	    cnt = num_tokens(argv[i], ',');
    	    Excl_List = (Range *)calloc(cnt+1, sizeof(Range));
    	    ParseExclusionList(Excl_List, argv[i]);
    	    Exclusions = TRUE;
    	    continue;
    	}
    	else if ((strncmp(argv[i], "-mask10", 7) == 0) ||
    	 	 (strncmp(argv[i], "-msk10", 6) == 0) ||
    	 	 (strncmp(argv[i], "-10", 3) == 0))
    	{
    	    Mask10 = TRUE;
    	}
    	else if (strncmp(argv[i], "-o", 2) == 0)
    	{
	    if (order != NULL)
	    {
		fprintf(stderr, 
			"%s: Warning - only one order list allowed. ",
			MyName);
		fprintf(stderr, " The last one will be used\n");
	    }
	    i++;
	    if (i == argc)
	    {
		fprintf(stderr, "%s: Order list not specified\n", MyName);
		return(1);
	    }
    	    order = argv[i];
    	}
	else if (strncmp(argv[i], "-nopr", 5) == 0)
	{
	    if (NoProbe != NULL)
	    {
		fprintf(stderr,
			"%s: Warning - only one noprobe list allowed. ",
			MyName);
		fprintf(stderr, " The last one will be used\n");
	    }
	    i++;
	    if (i == argc)
	    {
		fprintf(stderr, "%s: noprobe list not specified\n", MyName);
		return(1);
	    }
	    NoProbe = argv[i];
	}
	else if (strncmp(argv[i], "-bios", 5) == 0)
	{
	    i++;
	    if (i == argc)
	    {
		fprintf(stderr, "%s: BIOS base address not specified\n", 
			MyName);
		return(1);
	    }
	    Bios_Base = (Byte *)StrToUL(argv[i]);
	}
	else if (strncmp(argv[i], "-no_bios", 8) == 0)
	{
	    NoBIOS = TRUE;
	}
	else if (strncmp(argv[i], "-in", 3) == 0)
	{
	    PrintInfo();
	    return(0);
	}
	else if ((strncmp(argv[i], "-?", 2) == 0) ||
		 (strncmp(argv[i], "-h", 2) == 0))
	{
	    PRINT_VERSION;
	    printf("	(vprobe is derived from David Wexelblat's SuperProbe utility)\n");
	    printf("\n%s accepts the following options:\n", MyName);
	    printf("\t-verbose\tGive lots of information\n");
	    printf("\t-no16\t\tSkip any test that requires 16-bit ports\n");
	    printf("\t-excl list\tSkip any test that requires port on list\n");
	    printf("\t-mask10\t\tMask registers to 10 bits before comparing\n");
	    printf("\t\t\twith the exclusion list\n");
	    printf("\t-order list\tPerform test on the specified chipsets\n");
	    printf("\t\t\tin the specified order\n");
	    printf("\t-noprobe list\tDon't probe for any chipsets specified\n");
	    printf("\t-bios base\tSet BIOS base address to 'base'\n");
	    printf("\t-no_bios\tDon't read BIOS & assume EGA/VGA as primary\n");
	    printf("\t-info\t\tPrint a list of the capabilities of %s\n",
		   MyName);
	    printf("\nRefer to the manual page '%s' for complete details\n",
		   "setvideomode");
	    return(0);
	}
    	else
    	{
    	    fprintf(stderr, "%s: Unknown option '%s'\n", MyName, argv[i]);
    	    return(1);
    	}
    }

#endif /* _VPROBE_FUNC_ */

    /*
     * just in case, if the machine hangs, safe
     */
    system ("/sbin/sync");

    if (enable_video() < 0)
    {
    	fprintf(stderr, "%s: Cannot enable video\n", MyName);
    	return(-1);
    }
		
    if (!NoBIOS)
    {
	if (Bios_Base == (Byte *)0)
	{
	    Bios_Base = FindBios();
	    if (Bios_Base == (Byte *)0)
	    {
		fprintf(stderr, "%s: Could not determine BIOS base address\n",
			MyName);
		return(-1);
	    }
	}
    }
    if (Verbose)
    {
	if (NoBIOS)
	{
	    printf("Assuming an EGA/VGA is present\n\n");
	}
	else
	{
	    printf("BIOS Base address = 0x%x\n\n", (int)Bios_Base);
	}
	fflush(stdout);
    }

    if (!NoBIOS)
    {
        /*
         * Look for 'IBM' at Bios_Base+0x1E of the BIOS.  It will be there for
         * an EGA or VGA.
         */
        if (ReadBIOS(COPYRIGHT_BASE, copyright, 3) < 0)
        {
    	    fprintf(stderr, "%s: Failed to read BIOS\n", MyName);
    	    return(1);
        }
    }
#if 0
    if ((NoBIOS) ||
        ((copyright[0] == 'I') && 
         (copyright[1] == 'B') &&
         (copyright[2] == 'M')))
#endif
	/*
	 * As of today ie: 9/28/94, almost every card supports VGA, but there
	 * is no guarantee that 'IBM' can be read from all the new cards,
	 * especially PCI (etw32p) cards.... so a minor hack :-)
	 */
    if ( 1 )
    {
    	/*
    	 * It's an EGA or VGA
    	 */
    	Ports[0] = 0x3CC;
    	EnableIOPorts(1, Ports);
    	vgaIOBase = (inp(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
    	if (vgaIOBase == 0x3D0)
    	{
    	    /*
	     	 * Color - probe for secondary mono.
    	     */
    	    if (Probe_MDA(&Secondary))
    	    {
    	    	(void)Probe_Herc(&Secondary);
    	    }
    	}
    	else
    	{
    	    /*
    	     * Mono - probe for secondary color.
    	     */
    	    (void)Probe_CGA(&Secondary);
    	}
    
    	if (!Probe_VGA(&Primary))
    	{
    	    Primary = CHIP_EGA;
    	}
    
    	/*
     	 * If it's a VGA, do SVGA probing
     	 */
    	if (IS_VGA(Primary))
    	{
	    if (Verbose)
	    {
		printf("Found VGA; doing Super-VGA Probes...\n");
		fflush(stdout);
	    }
	    matched = &VGA_Descriptor;

    	    if (order == NULL)
    	    {
    	    	/* 
    	    	 * Use default ordering.
    	    	 */
    	    	for (i=0; SVGA_Descriptors[i] != NULL; i++)
    	    	{
    	    	    chip_p = SVGA_Descriptors[i];
    	    	    if (TestChip(chip_p, &Primary))
    	    	    {
			matched = chip_p;
    	    	    	break;
    	    	    }
    	    	}
    	    }
    	    else
    	    {
    	        /*
    	         * Use user specified order
    	         */
    	        p = order;
    	        while (p)
    	        {
    	            order = strchr(order, ',');
    	            if (order != NULL)
    	            {
    	                *order = '\0';
			order++;
    	            }
    	    	    flag = FALSE;
    	            for (i=0; SVGA_Descriptors[i] != NULL; i++)
		    {
    	    	    	chip_p = SVGA_Descriptors[i];
    	    	    	if (StrCaseCmp(p, chip_p->name) == 0)
    	    	    	{
    	    		    flag = TRUE;
    	    		    break;
    	    	        }
    	    	    }
    	    	    if (flag)
    	    	    {
    	    	        if (TestChip(chip_p, &Primary))
    	    	        {
			    matched = chip_p;
    	    		    break;
    	    	        }
    	    	    }
    	    	    else
    	    	    {
    	    	        fprintf(stderr, "%s: Chip class '%s' not known\n",
    	    		        MyName, p);
    	    	    }
    	    	    p = order;
    	        }
    	    }
    	}

	/*
	 * If this chipset doesn't exclude probing for a coprocessor,
	 * then look for one.
	 */
	if (matched->check_coproc)
	{
	    if (Verbose)
	    {
		printf("\nDoing Graphics CoProcessor Probes...\n");
		fflush(stdout);
	    }
    	    for (i=0; CoProc_Descriptors[i] != NULL; i++)
    	    {
    	        chip_p = CoProc_Descriptors[i];
    	        if (TestChip(chip_p, &CoProc))
    	        {
    	    	    break;
    	    	}
	    }
	}
    }
    else if (Probe_MDA(&Primary))
    {
    	(void)Probe_Herc(&Primary);
    	(void)Probe_CGA(&Secondary);
    }
/*
    else if (Probe_CGA(&Primary))
    {
    	if (Probe_MDA(&Secondary))
    	{
    	    (void)Probe_Herc(&Secondary);
    	}
    }
*/

    putchar('\n');
    if (Primary == -1)
    {
#ifndef _VPROBE_FUNC_
    	printf("Could not identify any video\n");
#endif
    }
    else 
    {
    	if (IS_MDA(Primary))
    	{
		/*printf("MDA \n"); */
    	}
    	else if (IS_HERC(Primary))
    	{
    	    /* printf("Hercules\n"); */
    	    Print_Herc_Name(Primary);
    	}
    	else if (IS_CGA(Primary))
    	{
    	    /* printf("CGA\n"); */
    	}
    	else if (IS_MCGA(Primary))
    	{
    	    /* printf("MCGA\n"); */
    	}
    	else if (IS_EGA(Primary))
    	{
    	    /* printf("EGA\n"); */
    	}
    	else if (IS_VGA(Primary))
    	{
    	    printf("Generic VGA (or unknown SVGA)\n");
    	}
    	else if (IS_SVGA(Primary))
    	{
    	    Probe_RamDac(Primary, &RamDac);
#ifndef _VPROBE_FUNC_
    	    Print_SVGA_Name(Primary);
			(CoProc != -1) ? 
	    			Print_Memory(chip_p->m())
	    			: Print_Memory(matched->m());
    	    Print_RamDac_Name(RamDac);
#endif
    	}
	if (CoProc != -1)
	{
#ifdef _VPROBE_FUNC_
		int class = COPROC_CLASS(CoProc);
		int chip = COPROC_CHIP(CoProc);
		extern char *CoProc_Names[NUM_CP_TYPES][CHPS_PER_CPTYPE];

		boardinfo->co_processor = CoProc_Names[class][chip];
#else
		printf("\tAttached graphics coprocessor:\n");
		Print_CoProc_Name(CoProc);
#endif
	}
#ifdef _VPROBE_FUNC_
	else
		boardinfo->co_processor = NULL;
#endif
    }
    if (Secondary != -1)
    {
    	printf("Second video: ");
    	if (IS_MDA(Secondary))
    	{
    	}
    	else if (IS_HERC(Secondary))
    	{
    	    printf("Hercules\n");
    	    Print_Herc_Name(Secondary);
    	}
    	else if (IS_CGA(Secondary))
    	{
    	    printf("CGA\n");
    	}
    	else if (IS_MCGA(Secondary))
    	{
    	    printf("MCGA\n");
    	}
    }

#ifdef _VPROBE_FUNC_
	if (Primary > 0)
	{
		boardinfo->chipname = 
			SVGA_Names[SVGA_VENDOR(Primary)][SVGA_CHIP(Primary)];
		boardinfo->memory = (CoProc != -1) ? 
		    chip_p->m() : matched->m();
		boardinfo->ramdac = RamDac_Names[DAC_CHIP(RamDac)];
	}
	else
	{
		boardinfo->chipname =  NULL;
		boardinfo->memory = 0;
		boardinfo->ramdac = NULL;
	}
#endif

	disable_video();
	return (1);
}

int
NullEntry ()
{
	return -1;
}
