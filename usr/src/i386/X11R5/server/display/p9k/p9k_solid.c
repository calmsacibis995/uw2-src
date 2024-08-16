/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_solid.c	1.3"
/***
 ***	NAME
 ***
 ***		p9k_solid.c : solid fills for the p9000 display library.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "p9k_solid.h"
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

#include <sidep.h>
#include "stdenv.h"
#include "p9k_opt.h"

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
export enum debug_level p9000_solid_debug = DEBUG_LEVEL_NONE;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

struct p9000_solid_state
{
	unsigned int raster;
	unsigned char plane_mask;
	unsigned int command;
	unsigned char foreground_color;
};

/***
 ***	Includes.
 ***/

#include <limits.h>
#include "p9k_gs.h"
#include "p9k_state.h"
#include "p9k_regs.h"
#include "p9k_gbls.h"
#include "p9k_misc.h"

/***
 ***	Constants.
 ***/


/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/


/* 
 * @doc:p9000_fill_rectangle_solid:
 *
 * Fill a series of solid rectangles.
 *
 * @enddoc
 */

STATIC SIBool
p9000_solid_fill_rectangles(
	SIint32 x_origin,			/* origin of the drawable */
	SIint32 y_origin,			/* */
	SIint32 count,				/* number of rectangles */
	SIRectOutlineP rect_p)		/* list of rectangles  */
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	struct p9000_solid_state *solid_state_p =
		graphics_state_p->solid_state_p;
	unsigned int status;

	/*
	 * Assertions.
	 */

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));

	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		   SGFillSolidFG ||
		   graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		   SGFillSolidBG);


#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_solid,SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
			"(p9000_solid_fill_rectangles)\n"
			"{\n"
			"\tx_origin = %ld\n"
			"\ty_origin = %ld\n"
			"\tcount = %ld\n"
			"\trect_p = %p\n"
			"}\n",
			x_origin, y_origin, count, (void *) rect_p);
	}
#endif
	/*
	 * Check input parameter values.
	 */

	if (count <= 0 || solid_state_p->command == 0)
	{
		return (SI_SUCCEED);
	}

	if (!P9000_REGISTER_IS_VALID_ORIGIN(x_origin,y_origin))
	{
		return SI_FAIL;
	}

	P9000_STATE_SET_RASTER(register_state_p,solid_state_p->raster);
	
	P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP(screen_state_p);


	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

	P9000_REGISTER_SET_FOREGROUND_COLOR((register_state_p->fground =
		solid_state_p->foreground_color));

	P9000_REGISTER_SET_PLANE_MASK((register_state_p->pmask =
		solid_state_p->plane_mask));
		

	/*
	 * Fill in the origin into the parameter engine w_off.xy
	 * register. 
	 */

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_W_OFF_XY,
		P9000_PACK_XY(x_origin, y_origin));
	
	if (solid_state_p->command == P9000_ADDRESS_BLIT_COMMAND)
	{
		for (;count--;rect_p++)
		{
			register int xl = rect_p->x;
			register int xr = xl + rect_p->width - 1;
			register int yt = rect_p->y;
			register int yb = yt + rect_p->height - 1;


#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_solid,INTERNAL))
		{
			(void) fprintf(debug_stream_p,
				"\t{\n"
				"\t\txl = %d\n"
				"\t\tyt = %d\n"
				"\t\txr = %d\n"
				"\t\tyb = %d\n"
				"\t}\n",
			   xl, yt, xr, yb);
		}
#endif
			
			
		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_0,
			P9000_PARAMETER_COORDINATE_XY_16,
			P9000_PARAMETER_COORDINATE_REL,
			P9000_PACK_XY(xl,yt));

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_1,
			P9000_PARAMETER_COORDINATE_XY_16,
			P9000_PARAMETER_COORDINATE_REL,
			P9000_PACK_XY(xr,yb));
		 
		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_2,
			P9000_PARAMETER_COORDINATE_XY_16,
			P9000_PARAMETER_COORDINATE_REL,
			P9000_PACK_XY(xl,yt));

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_3,
			P9000_PARAMETER_COORDINATE_XY_16,
			P9000_PARAMETER_COORDINATE_REL,
			P9000_PACK_XY(xr,yb));

			do
			{
				status = P9000_INITIATE_BLIT_COMMAND();
			} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
			
			
		}
	}
	else
	{
		for (;count--;rect_p++)
		{
			register int xl = rect_p->x;
			register int xr = xl + rect_p->width;
			register int yt = rect_p->y;
			register int yb = yt + rect_p->height;


#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_solid,INTERNAL))
		{
			(void) fprintf(debug_stream_p,
				"\t{\n"
				"\t\txl = %d\n"
				"\t\tyt = %d\n"
				"\t\txr = %d\n"
				"\t\tyb = %d\n"
				"\t}\n",
			   xl, yt, xr, yb);
		}
#endif
			/*
			 * xl, yt
			 */

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_0,
				P9000_PARAMETER_COORDINATE_X_32,
				P9000_PARAMETER_COORDINATE_REL,
				xl);

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_0,
				P9000_PARAMETER_COORDINATE_Y_32,
				P9000_PARAMETER_COORDINATE_REL,
				yt);
			
			/*
			 * xl, yb
			 */

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_1,
				P9000_PARAMETER_COORDINATE_X_32,
				P9000_PARAMETER_COORDINATE_REL,
				xl);

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_1,
				P9000_PARAMETER_COORDINATE_Y_32,
				P9000_PARAMETER_COORDINATE_REL,
				yb);

			/*
			 * xr, yb
			 */

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_2,
				P9000_PARAMETER_COORDINATE_X_32,
				P9000_PARAMETER_COORDINATE_REL,
				xr);

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_2,
				P9000_PARAMETER_COORDINATE_Y_32,
				P9000_PARAMETER_COORDINATE_REL,
				yb);
			/*
			 * xr, yt
			 */

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_3,
				P9000_PARAMETER_COORDINATE_X_32,
				P9000_PARAMETER_COORDINATE_REL,
				xr);

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_3,
				P9000_PARAMETER_COORDINATE_Y_32,
				P9000_PARAMETER_COORDINATE_REL,
				yt);
			
			
			do
			{
				status = P9000_INITIATE_QUAD_COMMAND();
			} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
			
			
			/*
			 * Check the command ...
			 */
			
			if (status & P9000_STATUS_QUAD_SOFTWARE)
			{
				p9000_global_reissue_quad_command(xl, yt, xr, yb);
			}
				
		}
	}
	
	return (SI_SUCCEED);

}

/*
 * @doc:p9000_solid_fill_spans:
 *
 * Fill a series of spans with the current foreground color.
 * This is implemented using horizontal line draws as these have been
 * found to be very fast.
 *
 * @enddoc
 */

STATIC SIBool
p9000_solid_fill_spans(
	SIint32 count, 
	register SIPointP points_p,
	register SIint32 *widths_p)
{
	unsigned int raster;
	unsigned int status;

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	const SIPointP points_fence_p = points_p + count;
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_solid, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_solid_fill_spans)\n"
					   "{\n"
					   "\tcount = %ld\n"
					   "\tpoints_p = 0x%p\n"
					   "\twidths_p = 0x%p\n"
					   "}\n",
					   count, (void *) points_p, (void *) widths_p);
	}
#endif 

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	raster = P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
											  graphics_state_p);
	
	/*
	 * Fill in the origin into the parameter engine w_off.xy
	 * register. 
	 */

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_W_OFF_XY,
		0);

	P9000_STATE_SET_RASTER(register_state_p, 
						   raster | P9000_RASTER_QUAD_OVERSIZED_MODE);

	/*
	 * Set the clipping rectangle to the whole screen.
	 */

	P9000_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN(screen_state_p);
	
	/*
	 * Synchronize the registers to the SI specified values.
	 */

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));

	do
	{

		register int width = *widths_p++;
		
#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_solid, INTERNAL))
		{
			(void) fprintf(debug_stream_p, 
						   "\t\t(%d, %d) + %d\n",
						   points_p->x, points_p->y, 
						   width);
		}
#endif 
		
		/*
		 * Draw a horizontal line at the given Y coordinate.
		 */

		if (width > 0)
		{

			/*
			 * Write the meta coordinate registers.  x1, y1 and x2, y2.
			 */

			P9000_WRITE_META_COORDINATE_REGISTER(
				P9000_META_COORDINATE_VTYPE_LINE,
				P9000_META_COORDINATE_ABS,
				P9000_META_COORDINATE_YX_XY,
				P9000_PACK_XY(points_p->x, points_p->y));
											 
			P9000_WRITE_META_COORDINATE_REGISTER(
				P9000_META_COORDINATE_VTYPE_LINE,
				P9000_META_COORDINATE_ABS,
				P9000_META_COORDINATE_YX_XY,
				P9000_PACK_XY(points_p->x + width - 1, points_p->y));


			/*
			 * Initiate the draw operation.  We don't check the status
			 * of the drawing as SI is responsible for handing us
			 * clipped spans.
			 */

			do
			{
				status = P9000_INITIATE_QUAD_COMMAND();
			} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
			
		}
		
	} while (++points_p < points_fence_p);
	
	return (SI_SUCCEED);

}



/*
 * @doc:p9000_solid_SI_1_0_solid_rectangles:
 *
 * Fill a series of rectangles handed down in SI_1_0 format.
 * This function basically sets up for drawing like the R5 entry
 * point, but defers calling the fill function entry point to the
 * convertor function.
 *
 * @enddoc
 */

STATIC SIBool
p9000_solid_SI_1_0_solid_rectangles(
	SIint32 count,
	SIRectP rect_p)
{
	unsigned int raster;

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	unsigned int status;

	/*
	 * Assertions.
	 */

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));

	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		   SGFillSolidFG ||
		   graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		   SGFillSolidBG);


#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_solid,SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
			"(p9000_solid_fill_rectangle)\n"
			"{\n"
			"\tcount = %ld\n"
			"\trect_p = %p\n"
			"}\n",
			 count, (void *) rect_p);
	}
#endif
	/*
	 * Check input parameter values.
	 */

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}


	if (graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		   SGFillSolidFG) 
	{
		raster = 
			P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,graphics_state_p);
	}
	else
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
	}

	P9000_STATE_SET_RASTER(register_state_p,raster);
	
	P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP(screen_state_p);

	/*
	 * Synchronize the registers to the SI specified values.
	 */

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));

	/*
	 * Fill in the origin into the parameter engine w_off.xy
	 * register. 
	 */

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_W_OFF_XY,
		P9000_PACK_XY(0,0));
	

	for (;count--;++rect_p)
	{
		register int xl = rect_p->ul.x;
		register int xr = rect_p->lr.x;
		register int yt = rect_p->ul.y;
		register int yb = rect_p->lr.y;


#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_solid,INTERNAL))
		{
			(void) fprintf(debug_stream_p,
				"\t{\n"
				"\t\txl = %d\n"
				"\t\tyt = %d\n"
				"\t\txr = %d\n"
				"\t\tyb = %d\n"
				"\t}\n",
			   xl, yt, xr, yb);
		}
#endif
		/*
		 * xl, yt
		 */

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_0,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			xl);

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_0,
			P9000_PARAMETER_COORDINATE_Y_32,
			P9000_PARAMETER_COORDINATE_REL,
			yt);
		
		/*
		 * xl, yb
		 */

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_1,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			xl);

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_1,
			P9000_PARAMETER_COORDINATE_Y_32,
			P9000_PARAMETER_COORDINATE_REL,
			yb);

		/*
		 * xr, yb
		 */

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_2,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			xr);

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_2,
			P9000_PARAMETER_COORDINATE_Y_32,
			P9000_PARAMETER_COORDINATE_REL,
			yb);
		/*
		 * xr, yt
		 */

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_3,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			xr);

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_3,
			P9000_PARAMETER_COORDINATE_Y_32,
			P9000_PARAMETER_COORDINATE_REL,
			yt);
		
		
		do
		{
			status = P9000_INITIATE_QUAD_COMMAND();
		} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
		
		
		/*
		 * Check the command ...
		 */
		
		if (status & P9000_STATUS_QUAD_SOFTWARE)
		{
			p9000_global_reissue_quad_command(xl, yt, xr, yb);
		}
			
	}
	
	return (SI_SUCCEED);

}

/*
 *@doc:p9000_solid_update_state:
 * Function called during gs change time, calculates raster,planemask, 
 * command the color to program for the current SGmode and pmask
 */

STATIC void
p9000_solid_update_state(struct p9000_solid_state *solid_state_p,
	struct p9000_graphics_state *graphics_state_p)
{

	unsigned int si_pmask =
		 graphics_state_p->generic_state.si_graphics_state.SGpmask & 0xFF;

	unsigned int si_rop =
		 graphics_state_p->generic_state.si_graphics_state.SGmode;

	unsigned int si_fg =
		 graphics_state_p->generic_state.si_graphics_state.SGfg;

#if defined(__DEBUG__)
	if (DEBUG_LEVEL_MATCH(p9000_solid,SCAFFOLDING))
	{
		(void)fprintf(debug_stream_p,
		"(p9000_solid_update_state){\n"
		"\t pmask = 0x%x\n"
		"\t rop = 0x%x\n"
		"\tfg = 0x%x\n"
		"}\n",
		si_pmask,si_rop,
		si_fg);
	}
#endif

		
	
	switch(si_rop)
	{
		case GXclear:
			solid_state_p->foreground_color = 0;

			solid_state_p->plane_mask =
				si_pmask;

			solid_state_p->raster =
				P9000_FOREGROUND_COLOR_MASK;

			solid_state_p->command =
				P9000_ADDRESS_QUAD_COMMAND;

			break;

		case GXand:
			solid_state_p->foreground_color =
				0;

			solid_state_p->plane_mask =
				~si_fg & si_pmask;

			solid_state_p->raster =
				P9000_FOREGROUND_COLOR_MASK;

			solid_state_p->command =
				P9000_ADDRESS_QUAD_COMMAND;

			break;
		case GXandReverse:
			solid_state_p->foreground_color =
				si_fg;

			solid_state_p->plane_mask =
				si_pmask;

			solid_state_p->raster =
				P9000_FOREGROUND_COLOR_MASK & ~P9000_SOURCE_MASK;

			solid_state_p->command =
				P9000_ADDRESS_BLIT_COMMAND;

			break;
		case GXandInverted:
			solid_state_p->foreground_color =
				0;
			solid_state_p->plane_mask =
				si_fg & si_pmask;

			solid_state_p->raster =
				P9000_FOREGROUND_COLOR_MASK;

			solid_state_p->command =
				P9000_ADDRESS_BLIT_COMMAND;

			break;
		case GXcopy:
			solid_state_p->foreground_color =
				si_fg;
			solid_state_p->plane_mask =
				si_pmask;
			solid_state_p->raster =
				P9000_FOREGROUND_COLOR_MASK;

			solid_state_p->command =
			 	P9000_ADDRESS_QUAD_COMMAND;
			break;

		case GXnoop:
			solid_state_p->foreground_color = 0;
			solid_state_p->plane_mask = 0;
			solid_state_p->raster = 0;
			solid_state_p->command = 0;

			break;
		case GXxor:
			solid_state_p->foreground_color = si_fg;
			solid_state_p->plane_mask = si_pmask;
			solid_state_p->raster =
				(~P9000_FOREGROUND_COLOR_MASK & P9000_SOURCE_MASK) |
				(P9000_FOREGROUND_COLOR_MASK & ~P9000_SOURCE_MASK);
			solid_state_p->command =
				P9000_ADDRESS_BLIT_COMMAND;
			break;

		case GXor:
			solid_state_p->foreground_color =
				si_fg;
			solid_state_p->plane_mask =
				si_pmask;
			solid_state_p->raster =
				P9000_SOURCE_MASK | P9000_FOREGROUND_COLOR_MASK;
			solid_state_p->command =
				P9000_ADDRESS_BLIT_COMMAND;
			break;

		case GXnor:
			solid_state_p->foreground_color = ~si_fg;
			solid_state_p->plane_mask = si_pmask;
			solid_state_p->raster =
				~P9000_SOURCE_MASK & P9000_FOREGROUND_COLOR_MASK;
			solid_state_p->command =
				P9000_ADDRESS_BLIT_COMMAND;
			break;
		case GXequiv:
			solid_state_p->foreground_color = si_fg;
			solid_state_p->plane_mask = si_pmask;
			solid_state_p->raster = (P9000_FOREGROUND_COLOR_MASK & 
				P9000_SOURCE_MASK) | (~P9000_FOREGROUND_COLOR_MASK & 
				~P9000_SOURCE_MASK);
			solid_state_p->command = P9000_ADDRESS_BLIT_COMMAND;
			break;
		case GXinvert:
			solid_state_p->foreground_color =
				0;	/*Don't care*/
			solid_state_p->plane_mask = si_pmask;
			solid_state_p->raster =
				~P9000_SOURCE_MASK;
			solid_state_p->command =
				P9000_ADDRESS_BLIT_COMMAND;
			break;

		case GXorReverse:
			solid_state_p->foreground_color =
				si_fg;
			solid_state_p->plane_mask =
				si_pmask;
			solid_state_p->raster =
				P9000_FOREGROUND_COLOR_MASK | ~P9000_SOURCE_MASK;
			solid_state_p->command =
				P9000_ADDRESS_BLIT_COMMAND;
			break;

		case GXcopyInverted:
			solid_state_p->foreground_color =
				~si_fg;
			solid_state_p->plane_mask =
				si_pmask;
			solid_state_p->raster =
				P9000_FOREGROUND_COLOR_MASK;
			solid_state_p->command =
				P9000_ADDRESS_QUAD_COMMAND;

			break;
		case GXorInverted:
			solid_state_p->foreground_color =
				si_fg;
			solid_state_p->plane_mask =
				si_pmask;
			solid_state_p->raster =
				~P9000_FOREGROUND_COLOR_MASK | P9000_SOURCE_MASK;
			solid_state_p->command =
				P9000_ADDRESS_BLIT_COMMAND;

			break;
		case GXnand:
			solid_state_p->foreground_color =
				~si_fg;
			solid_state_p->plane_mask =
				si_pmask;
			solid_state_p->raster =
				~P9000_SOURCE_MASK | P9000_FOREGROUND_COLOR_MASK;
			solid_state_p->command =
				P9000_ADDRESS_BLIT_COMMAND;
			break;
		case GXset:
			solid_state_p->foreground_color =
				si_fg;
			solid_state_p->plane_mask =
				si_pmask;
			solid_state_p->raster =
				~0;
			solid_state_p->command =
				P9000_ADDRESS_QUAD_COMMAND;
			break;
	}
}

/*
 * @doc:p9000_solid__gs_change__:
 *
 * Handle a change of graphics state.
 * 
 * @enddoc
 */

function void
p9000_solid__gs_change__(void)
{

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	struct p9000_solid_state *solid_state_p =
		 graphics_state_p->solid_state_p;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));

	if (solid_state_p == NULL)
	{
		solid_state_p = 
			graphics_state_p->solid_state_p  =
			allocate_and_clear_memory(sizeof(struct p9000_solid_state));
	}
		
	if (graphics_state_p->generic_state.si_state_flags &
		 (SetSGmode | SetSGpmask | SetSGfg))
	{
		p9000_solid_update_state(solid_state_p,graphics_state_p);
	}

	if ((graphics_state_p->
		generic_state.si_graphics_state.SGfillmode !=
		SGFillSolidFG) &&
		(graphics_state_p->
		generic_state.si_graphics_state.SGfillmode !=
		SGFillSolidBG)) 
	 {
		return;
	 }

	if(!(screen_state_p->options_p->rectfill_options & 
	  P9000_OPTIONS_RECTFILL_OPTIONS_USE_SOLID_FILL_RECT))
	{
		screen_state_p->generic_state.screen_functions_p->
			si_poly_fillrect =
			graphics_state_p->generic_si_functions.si_poly_fillrect;

		return;
	}


	if (screen_state_p->generic_state.screen_sdd_version_number >=
		DM_SI_VERSION_1_1)
	{
		screen_state_p->generic_state.screen_functions_p->
			si_poly_fillrect = 
			p9000_solid_fill_rectangles;
	}
	else /*Backward compatibility support.*/
	{
		screen_state_p->generic_state.screen_functions_p->
			si_poly_fillrect = 
			(SIBool (*)(SIint32, SIint32, SIint32, SIRectOutlineP))
				p9000_solid_SI_1_0_solid_rectangles;
	}
}


/*
 * @doc:p9000_solid__initialize__:
 * 
 * Initialization.
 * 
 * @enddoc
 */

function void
p9000_solid__initialize__(SIScreenRec *si_screen_p,
						struct p9000_options_structure *options_p)
{
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	P9000_CURRENT_SCREEN_STATE_DECLARE();


	if (options_p->rectfill_options &
		 P9000_OPTIONS_RECTFILL_OPTIONS_USE_SOLID_FILL_RECT)
	{
		flags_p->SIavail_fpoly |= RECTANGLE_AVAIL;
	}

	if (options_p->spansfill_options &
		 P9000_OPTIONS_SPANSFILL_OPTIONS_USE_SOLID_FILL_SPANS)
	{
		flags_p->SIavail_spans |= SPANS_AVAIL;
		functions_p->si_fillspans = p9000_solid_fill_spans;
	}

	functions_p->si_poly_fconvex =
		(SIBool (*)())p9000_global_no_operation_fail;
	functions_p->si_poly_fgeneral =
		 (SIBool (*)())p9000_global_no_operation_fail;
	functions_p->si_poly_fillrect =
		 (SIBool (*)())p9000_global_no_operation_fail;

}
