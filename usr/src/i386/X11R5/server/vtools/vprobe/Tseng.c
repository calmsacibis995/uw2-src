/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vtools:vprobe/Tseng.c	1.3"


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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Tseng.c,v 2.2 1993/09/21 15:20:50 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000, 0x000, 0x3BF, 0x3CD};
static int is_w32;   
        /* 
         * there is no fool-proof way to find amount of memory in case 
         *  ET4000/W32, ET4000/W32i or ET4000/W32p cards
         */
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

Chip_Descriptor Tseng_Descriptor = {
    "Tseng",
    Probe_Tseng,
    TsengMemory,
    Ports,
    NUMPORTS,
    FALSE,
    FALSE,
    FALSE
};

#ifdef __STDC__
Bool Probe_Tseng(int *Chipset)
#else
Bool Probe_Tseng(Chipset)
int *Chipset;
#endif
{
    Bool result = FALSE;
    Byte old, old1;

    /* Add CRTC to enabled ports */
    Ports[0] = CRTC_IDX;
    Ports[1] = CRTC_REG;
    Ports[2] = inp(0x3CC) & 0x01 ? 0x3D8 : 0x3B8;
    EnableIOPorts(NUMPORTS, Ports);
    old = inp(0x3BF);
    old1 = inp(Ports[2]);
    outp(0x3BF, 0x03);
    outp(Ports[2], 0xA0);
    if (tstrg(0x3CD, 0x3F)) 
    {
        /* 
         * It is a Tseng; now figure out which one, and
         * set Chipset.
         */
        result = TRUE;
        if (!testinx(CRTC_IDX, 0x1B))
        {
            if (testinx2(CRTC_IDX, 0x30, 0x1F))
            {
                unsigned char what_etw32;

                is_w32=1;
                /*  
                 *  any one of ET4000/W32, ET4000/W32i or ET4000/W32p models.
                 */
                outp( 0x217A, 0xEC );
                what_etw32 = inp( 0x217B );
                switch ( what_etw32 & 0xf0 ) 
                {

                  case 0x10:
                         *Chipset = CHIP_ET4000_W32i;
                         break;

                  case 0x30:
                         *Chipset = CHIP_ET4000_W32i_REVB;
                         break;

                  case 0x20:
                         *Chipset = CHIP_ET4000_W32p_REVA;
                         break;

                  case 0x50:
                         *Chipset = CHIP_ET4000_W32p_REVB;
                         break;

                  case 0x70:
                         *Chipset = CHIP_ET4000_W32p_REVC;
                         break;

                   default:    
                         *Chipset = CHIP_ET4000_W32;
                                 
                }
       
            }
            else
            {
                *Chipset = CHIP_ET4000;
            }
        }
        else
        {
            *Chipset = CHIP_ET3000;
        }
    }
    outp(Ports[2], old1);
    outp(0x3BF, old);
    DisableIOPorts(NUMPORTS, Ports);
    return(result);
}

TsengMemory()
{
    int vgaIOBase = (inp(0x3CC) & 0x01) ? 0x3d0 : 0x3b0;
    int temp;
    int mem = -1;

    if ( is_w32 ) 
    {
  
       /* 
        *  There is no fool-proof way to find memory in case
        *  of W32/W32i/W32p cards.  
        */
       return (-1);
     
    }
    outp(vgaIOBase+0x04, 0x37);
    temp = inp(vgaIOBase+0x05);
    switch(temp & 0x03) {
       case 1:
       mem = 256;
       break;
       case 2:
       mem = 512;
       break;
       case 3:
       mem = 1024;
       break;
       case 4:
       mem = 2048;
       break;
    }
    if (temp & 0x80)
        mem <<= 1;

    return (mem);
}
