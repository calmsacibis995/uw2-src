/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/devices/viper/viper_pci.h	1.2"
#if (! defined(__VIPER_PCI_INCLUDED__))

#define __VIPER_PCI_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include "stdenv.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

/*
 *	Forward declarations.
 */


/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern enum debug_level viper_pci_debug ;
#endif

/*
 *	Current module state.
 */

extern void 
viper_pci_dac_write(unsigned int index, unsigned int value)
;

extern unsigned int 
viper_pci_dac_read(unsigned int index)
;

extern unsigned int
viper_pci_detect(struct p9000_screen_state *screen_state_p)
;

extern boolean
viper_pci_uninit(struct p9000_screen_state *screen_state_p)
;

extern boolean
viper_pci_init(struct p9000_screen_state *screen_state_p)
;


#endif
