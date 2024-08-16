/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_clocks.c	1.2"
/***
 ***	NAME
 ***
 ***		p9k_clocks.c
 ***
 ***	SYNOPSIS
 ***
 ***	DESCRIPTION
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***
 ***	SEE ALSO
 ***
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	AUTHORS
 ***
 ***	HISTORY
 ***
 ***/

PUBLIC

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

struct p9000_clock_functions 
{
	boolean (*clock_initialize_function_p)();
	boolean (*clock_uninitialize_function_p)();
	boolean (*can_support_frequency_function_p)(unsigned long);
};

enum p9000_clock_kind
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, INIT_FUNC, UNINIT_FUNC,\
	 CAN_SUPPORT_FREQ_FUNC, NUM_FREQ,\
	f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15)\
		NAME
#include "p9k_clocks.def"
#undef DEFINE_CLOCK_CHIP						
};  

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
export boolean p9000_clocks_debug = FALSE;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

/***
 ***	Includes.
 ***/

#include "p9k_regs.h"
#include "p9k_state.h"
#include "p9k_opt.h"
#include "p9k.h"

/***
 ***	Constants.
 ***/
#define	P9000_CLOCK_STATE_STAMP\
	(('P' << 0) + ('9' << 1) + ('0' << 2) + ('0' << 3) + ('0' << 4) +\
	 ('_' << 5) + ('C' << 6) + ('L' << 7) + ('O' << 8) + ('C' << 9) +\
	 ('K' << 10) + ('_' << 11) + ('S' << 12) + ('T' << 13) + ('A' << 14) +\
	 ('T' << 15) + ('E' << 16) + 0 )

struct p9000_clock_state
{
	enum p9000_clock_kind clock_id;

	int min_frequency;

	int max_frequency;

	boolean is_programmable;
	
	/*
	 * Valid only for a non programmable clock
	 */

	int frequency_count;
	
	int *frequency_table_p;


#if defined(__DEBUG__)
	int stamp;
#endif
};

/*
 * Clock chip table entry.
 */

struct p9000_clock_chip_table_entry
{
	enum p9000_clock_kind clock_id;
	int	frequency_count;
	int clock_frequency_table[P9000_DEFAULT_CLOCK_CHIP_NUMBER_OF_FREQUENCIES];
};

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/



STATIC
long mul32( short op1, short op2)
{
    return ( ((long) op1) * ((long) op2));
}

STATIC
int div32( long op1, short op2)
{
    return ( (int) (op1 / (long) op2));
}

STATIC
unsigned long calculate_clock_value(unsigned long freq)
{
    unsigned short ftab[16]=
    {
        5100,5320,5850,6070,6440,6680,7350,7560,8090,
        8320,9150,10000,12000,12000,12000,12000
    };

    unsigned short  ref = 5727; /* reference freq 2*14.31818 *100*2*/
    int	    i = 0;              /* index preset field*/
    int	    m = 0;              /* power of 2 divisor field*/
    int	    p;					/* multiplier field*/
    int	    q;					/* divisor field*/
    int	    qs;					/* starting q to prevent integer overflow*/
    int	    bestq = 0;			/* best q so far*/
    int	    bestp = 0;			/* best p so far*/
    int	    bestd = 10000;		/* distance to best combination so far*/
    int	    curd;				/* current distance*/
    int	    curf;				/* current frequency*/
    int	    j;					/* loop counter*/
    unsigned long   data;


	freq /=  10000;

    while(freq < 5000)			
    {
        m += 1;				
        freq *= 2;				
    }

    for (j = 0; j < 16; j++)   	
    {
        if (freq < ftab[j])	
        {
	        i = j;
	        break;
        }
    }

    for (p = 0; p < 128; p++)
    {
        qs = div32(mul32(ref, p+3),0x7fff); 

        for (q = qs; q < 128; q++)
        { 						
            curf = div32(mul32(ref, p+3), (q + 2) << 1);
	        curd = freq - curf;

            if (curd < 0)
            {
                curd = -curd; 
            }

            if (curd < bestd)
	        {
	            bestd = curd;
	            bestp = p;
	            bestq = q;
	        }
        }
    }

    data = ((((long) i) << 17) | (((long) bestp) << 10) | (m << 7) | bestq);
	return data;

}

STATIC boolean
p9000_clock_initialize_icd2061a(struct p9000_screen_state *screen_state_p)
{
	register long 	clock_value; 	
	register long	frequency;
	struct p9000_board_functions *board_functions_p =
		screen_state_p->board_functions_p;

	frequency =
		 screen_state_p->display_mode_p->clock_frequency * 1000;

	/*
	 * If we have a BT485 dac and the frequency is over the
	 * clock doubler threshold then divide the frequency by 2
	 */

	if (screen_state_p->options_p->dac_name ==
		P9000_OPTIONS_DAC_NAME_BT485KPJ110 ||
		screen_state_p->options_p->dac_name ==
		P9000_OPTIONS_DAC_NAME_BT485KPJ135)
	{
		if (screen_state_p->options_p->bt485_clock_doubler_threshold*1000000
			 <= frequency)
		{
			frequency /= 2; 
		}
	}

	clock_value = calculate_clock_value(frequency);

	(*board_functions_p->clock_set_function_p)
		(clock_value | P9000_IC_REG2);

	return TRUE;
}


STATIC boolean
p9000_clock_uninitialize_icd2061a(struct p9000_screen_state *screen_state_p)
{
	return TRUE;
}

STATIC boolean 
p9000_clock_is_supported_frequency(unsigned long freq)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_clock_state *clock_state_p =
		screen_state_p->clock_state_p;
	

	if (clock_state_p->is_programmable)
	{
		if (freq < clock_state_p->min_frequency)
		{
			return FALSE;
		}
		if (freq > clock_state_p->max_frequency)
		{
			return FALSE;
		}

	}
	else
	{
		
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return FALSE;
	}

	
	return TRUE;
}

/*
 * Clock Chip table.
 */


STATIC struct p9000_clock_chip_table_entry
clock_chip_table[] = 
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, INIT_FUNC, UNINIT_FUNC,\
	 CAN_SUPPORT_FREQ_FUNC, NUM_FREQ,\
	f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15)\
	{\
		NAME,NUM_FREQ,\
		{f0, f1, f2, f3, f4, f5, f6, f7,\
		  f8, f9, f10, f11, f12, f13, f14, f15 }\
	}

#include "p9k_clocks.def"

#undef DEFINE_CLOCK_CHIP						
};  

STATIC struct p9000_clock_functions
clock_functions_table[]=
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, INIT_FUNC, UNINIT_FUNC,\
	CAN_SUPPORT_FREQ_FUNC, NUM_FREQ,\
	f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15)\
	{\
		INIT_FUNC,\
		UNINIT_FUNC,\
		CAN_SUPPORT_FREQ_FUNC\
	}

#include "p9k_clocks.def"

#undef DEFINE_CLOCK_CHIP
};

function boolean
p9000_clock__hardware_initialize__(SIScreenRec *si_screen_p,
	struct p9000_options_structure *options_p)
{

	int i;
	enum p9000_clock_kind clock_kind;
	struct p9000_clock_state *clock_state_p;
	struct p9000_screen_state *screen_state_p =
		(struct p9000_screen_state *) si_screen_p->vendorPriv;

	/*
	 * Check if the board layer is overriding the clock
	 */

	if (screen_state_p->clock_state_p)
	{
		return TRUE;
	}

	/*
	 * Determine the type of the clock we have at hand
	 */

	switch(options_p->clock_name)
	{
		case P9000_OPTIONS_CLOCK_NAME_ICD2061A:
			clock_kind = P9000_CLOCK_ICD2061A;
			break;
		default:
				/*CONSTANTCONDITION*/
				ASSERT(0);
	}
	
	for (i=P9000_CLOCK_NULL; i <= P9000_CLOCK_COUNT; ++i)
	{
		if (clock_chip_table[i].clock_id == clock_kind)
		{
			screen_state_p->clock_functions_p =
				&clock_functions_table[i];
			break;
		}

	}

	if (i == P9000_CLOCK_COUNT)
	{
		return FALSE;
	}

	clock_state_p =
		  allocate_and_clear_memory(sizeof(*clock_state_p));

	STAMP_OBJECT(P9000_CLOCK_STATE,clock_state_p);
		
	screen_state_p->clock_state_p =
		clock_state_p;

	clock_state_p->clock_id =
		clock_chip_table[i].clock_id;

	clock_state_p->frequency_count =
		clock_chip_table[i].frequency_count;

	
	
	if (clock_chip_table[i].frequency_count == 0)
	{
		clock_state_p->is_programmable = TRUE;

		clock_state_p->max_frequency =
			clock_chip_table[i].clock_frequency_table[1];

		clock_state_p->min_frequency =
			clock_chip_table[i].clock_frequency_table[0];
	}
	else
	{
		clock_state_p->is_programmable = FALSE;
		
		/*CONSTANTCONDITION*/
		ASSERT(0);
	
		return FALSE;
	}

	return TRUE;

}
