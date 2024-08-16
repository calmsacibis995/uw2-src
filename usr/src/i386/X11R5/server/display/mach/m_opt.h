/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_opt.h	1.8"
#if (! defined(__M_OPT_INCLUDED__))

#define __M_OPT_INCLUDED__



#include "stdenv.h"
#include "global.h"


enum mach_options_si_interface_version
{
	MACH_OPTIONS_SI_INTERFACE_VERSION_1_0,
	MACH_OPTIONS_SI_INTERFACE_VERSION_1_1,
	MACH_OPTIONS_SI_INTERFACE_VERSION_AUTO_CONFIGURE,
	mach_options_si_interface_version_end_enumeration
};

enum mach_options_verbose_startup
{
	MACH_OPTIONS_VERBOSE_STARTUP_YES,
	MACH_OPTIONS_VERBOSE_STARTUP_NO,
	mach_options_verbose_startup_end_enumeration
};

enum mach_options_cursor_type
{
	MACH_OPTIONS_CURSOR_TYPE_AUTO_CONFIGURE,
	MACH_OPTIONS_CURSOR_TYPE_HARDWARE_CURSOR,
	MACH_OPTIONS_CURSOR_TYPE_SOFTWARE_CURSOR,
	mach_options_cursor_type_end_enumeration
};

enum mach_options_cursor_byte_swap
{
	MACH_OPTIONS_CURSOR_BYTE_SWAP_ENABLED,
	MACH_OPTIONS_CURSOR_BYTE_SWAP_DISABLED,
	MACH_OPTIONS_CURSOR_BYTE_SWAP_AUTO_CONFIGURE,
	mach_options_cursor_byte_swap_end_enumeration
};

enum mach_options_dac_name
{
	MACH_OPTIONS_DAC_NAME_ATI_68830,
	MACH_OPTIONS_DAC_NAME_SIERRA_SC11_48X,
	MACH_OPTIONS_DAC_NAME_ATT_20C491,
	MACH_OPTIONS_DAC_NAME_TI_TLC_34075,
	MACH_OPTIONS_DAC_NAME_ATI_68875_BFN,
	MACH_OPTIONS_DAC_NAME_ATI_68875_CFN,
	MACH_OPTIONS_DAC_NAME_BT_478,
	MACH_OPTIONS_DAC_NAME_IMS_G178J_80Z,
	MACH_OPTIONS_DAC_NAME_BT_481,
	MACH_OPTIONS_DAC_NAME_IMS_G176J_80Z,
	MACH_OPTIONS_DAC_NAME_BT_476,
	MACH_OPTIONS_DAC_NAME_AUTO_DETECT,
	mach_options_dac_name_end_enumeration
};

enum mach_options_dac_rgb_width
{
	MACH_OPTIONS_DAC_RGB_WIDTH_6,
	MACH_OPTIONS_DAC_RGB_WIDTH_8,
	MACH_OPTIONS_DAC_RGB_WIDTH_DEFAULT,
	mach_options_dac_rgb_width_end_enumeration
};

enum mach_options_dac_24_bit_color_mode
{
	MACH_OPTIONS_DAC_24_BIT_COLOR_MODE_RGBA,
	MACH_OPTIONS_DAC_24_BIT_COLOR_MODE_ABGR,
	mach_options_dac_24_bit_color_mode_end_enumeration
};

enum mach_options_dac_16_bit_color_mode
{
	MACH_OPTIONS_DAC_16_BIT_COLOR_MODE_555,
	MACH_OPTIONS_DAC_16_BIT_COLOR_MODE_565,
	MACH_OPTIONS_DAC_16_BIT_COLOR_MODE_655,
	MACH_OPTIONS_DAC_16_BIT_COLOR_MODE_664,
	mach_options_dac_16_bit_color_mode_end_enumeration
};

enum mach_options_monitor_sync_type
{
	MACH_OPTIONS_MONITOR_SYNC_TYPE_COMPOSITE_SYNC,
	MACH_OPTIONS_MONITOR_SYNC_TYPE_SYNC_ON_GREEN,
	MACH_OPTIONS_MONITOR_SYNC_TYPE_SEPARATE_SYNC,
	mach_options_monitor_sync_type_end_enumeration
};

enum mach_options_chipset_name
{
	MACH_OPTIONS_CHIPSET_NAME_MACH_8,
	MACH_OPTIONS_CHIPSET_NAME_MACH_32,
	MACH_OPTIONS_CHIPSET_NAME_AUTO_DETECT,
	mach_options_chipset_name_end_enumeration
};

enum mach_options_clock_chip_name
{
	MACH_OPTIONS_CLOCK_CHIP_NAME_18810,
	MACH_OPTIONS_CLOCK_CHIP_NAME_18810_2,
	MACH_OPTIONS_CLOCK_CHIP_NAME_18811_0,
	MACH_OPTIONS_CLOCK_CHIP_NAME_18811_1,
	MACH_OPTIONS_CLOCK_CHIP_NAME_18812_0,
	mach_options_clock_chip_name_end_enumeration
};

enum mach_options_io_bus_width
{
	MACH_OPTIONS_IO_BUS_WIDTH_16_BIT,
	MACH_OPTIONS_IO_BUS_WIDTH_8_BIT,
	MACH_OPTIONS_IO_BUS_WIDTH_AUTO_DETECT,
	mach_options_io_bus_width_end_enumeration
};

enum mach_options_horizontal_line_draw_optimizations
{
	MACH_OPTIONS_HORIZONTAL_LINE_DRAW_OPTIMIZATIONS_ENABLED,
	MACH_OPTIONS_HORIZONTAL_LINE_DRAW_OPTIMIZATIONS_DISABLED,
	mach_options_horizontal_line_draw_optimizations_end_enumeration
};

enum mach_options_passthrough_override
{
	MACH_OPTIONS_PASSTHROUGH_OVERRIDE_ENABLED,
	MACH_OPTIONS_PASSTHROUGH_OVERRIDE_DISABLED,
	mach_options_passthrough_override_end_enumeration
};


#define	MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_GRAY	1
#define	MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_GRAY_SCALE	2
#define	MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_COLOR	4
#define	MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_PSEUDO_COLOR	8


#define	MACH_OPTIONS_RECTFILL_OPTIONS_USE_SOLID_FILL_RECT	1
#define	MACH_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT	2
#define	MACH_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT	4
#define	MACH_OPTIONS_RECTFILL_OPTIONS_USE_PATTERN_REGISTERS	8
#define	MACH_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY	16
#define	MACH_OPTIONS_RECTFILL_OPTIONS_USE_LARGE_OFFSCREEN_AREAS	32
#define	MACH_OPTIONS_RECTFILL_OPTIONS_USE_IBM_MODE	64


#define	MACH_OPTIONS_BITBLT_OPTIONS_USE_SS_BITBLT	1
#define	MACH_OPTIONS_BITBLT_OPTIONS_USE_MS_BITBLT	2
#define	MACH_OPTIONS_BITBLT_OPTIONS_USE_SM_BITBLT	4
#define	MACH_OPTIONS_BITBLT_OPTIONS_USE_MS_STPLBLT	8
#define	MACH_OPTIONS_BITBLT_OPTIONS_USE_IBM_MODE	16


#define	MACH_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW	1
#define	MACH_OPTIONS_LINEDRAW_OPTIONS_USE_PATTERN_REGISTERS	2
#define	MACH_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES	4
#define	MACH_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW	8
#define	MACH_OPTIONS_LINEDRAW_OPTIONS_USE_IBM_MODE	16


#define	MACH_OPTIONS_SPANSFILL_OPTIONS_USE_SOLID_FILL	1
#define	MACH_OPTIONS_SPANSFILL_OPTIONS_USE_STIPPLE_FILL	2
#define	MACH_OPTIONS_SPANSFILL_OPTIONS_USE_TILE_FILL	4
#define	MACH_OPTIONS_SPANSFILL_OPTIONS_USE_PATTERN_REGISTERS	8


#define	MACH_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINT	1


#define	MACH_OPTIONS_FONTDRAW_OPTIONS_DRAW_NON_TERMINAL_FONTS	1
#define	MACH_OPTIONS_FONTDRAW_OPTIONS_DRAW_TERMINAL_FONTS	2
#define	MACH_OPTIONS_FONTDRAW_OPTIONS_USE_OFFSCREEN_MEMORY	4
#define	MACH_OPTIONS_FONTDRAW_OPTIONS_USE_IBM_MODE	8




struct mach_options_structure
{
	unsigned int bitblt_options;
	enum mach_options_chipset_name chipset_name;
	char *chipset_revision;
	enum mach_options_clock_chip_name clock_chip_name;
	char *crtc_parameters;
	int crtc_start_offset;
	enum mach_options_cursor_byte_swap cursor_byte_swap;
	char *cursor_max_size;
	enum mach_options_cursor_type cursor_type;
	enum mach_options_dac_16_bit_color_mode dac_16_bit_color_mode;
	enum mach_options_dac_24_bit_color_mode dac_24_bit_color_mode;
	int dac_access_delay_count;
	int dac_blank_adjust;
	int dac_max_frequency;
	enum mach_options_dac_name dac_name;
	int dac_pixel_delay;
	enum mach_options_dac_rgb_width dac_rgb_width;
	unsigned int fontdraw_options;
	char *glyph_cache_size;
	int graphics_engine_fifo_blocking_factor;
	int graphics_engine_loop_timeout_count;
	int graphics_engine_micro_delay_count;
	enum mach_options_horizontal_line_draw_optimizations horizontal_line_draw_optimizations;
	enum mach_options_io_bus_width io_bus_width;
	unsigned int linedraw_options;
	int max_number_of_glyphs_in_downloadable_font;
	char *modedb_string;
	enum mach_options_monitor_sync_type monitor_sync_type;
	int number_of_downloadable_cursors;
	int number_of_downloadable_fonts;
	int number_of_graphics_states;
	int offscreen_stipple_padded_height;
	int offscreen_stipple_padded_width;
	int offscreen_tile_padded_height;
	int offscreen_tile_padded_width;
	int omm_full_coalesce_watermark;
	int omm_hash_list_size;
	int omm_horizontal_constraint;
	char *omm_named_allocation_list;
	int omm_neighbour_list_increment;
	int omm_vertical_constraint;
	char *overscan_color;
	int overscan_h;
	int overscan_v;
	enum mach_options_passthrough_override passthrough_override;
	unsigned int pointdraw_options;
	unsigned int rectfill_options;
	unsigned int screen_8_4_bit_visual_list;
	enum mach_options_si_interface_version si_interface_version;
	unsigned int spansfill_options;
	char *static_colormap_description_file;
	char *stipple_best_size;
	char *tile_best_size;
	enum mach_options_verbose_startup verbose_startup;
	char *video_memory_dimensions;
	int vram_fifo_depth;
	int vt_switch_save_lines;

};

/*
 * Names of the option defaults
 */

#define MACH_OPTIONS_BITBLT_OPTIONS_DEFAULT\
	( MACH_OPTIONS_BITBLT_OPTIONS_USE_SS_BITBLT |MACH_OPTIONS_BITBLT_OPTIONS_USE_MS_BITBLT |MACH_OPTIONS_BITBLT_OPTIONS_USE_SM_BITBLT |MACH_OPTIONS_BITBLT_OPTIONS_USE_MS_STPLBLT |MACH_OPTIONS_BITBLT_OPTIONS_USE_IBM_MODE )
#define MACH_OPTIONS_CHIPSET_NAME_DEFAULT\
	MACH_OPTIONS_CHIPSET_NAME_AUTO_DETECT
#define MACH_OPTIONS_CHIPSET_REVISION_DEFAULT 0
#define MACH_OPTIONS_CLOCK_CHIP_NAME_DEFAULT\
	MACH_OPTIONS_CLOCK_CHIP_NAME_18811_1
#define MACH_OPTIONS_CRTC_PARAMETERS_DEFAULT 0
#define MACH_OPTIONS_CRTC_START_OFFSET_DEFAULT\
	0
#define MACH_OPTIONS_CURSOR_BYTE_SWAP_DEFAULT\
	MACH_OPTIONS_CURSOR_BYTE_SWAP_AUTO_CONFIGURE
#define MACH_OPTIONS_CURSOR_MAX_SIZE_DEFAULT 0
#define MACH_OPTIONS_CURSOR_TYPE_DEFAULT\
	MACH_OPTIONS_CURSOR_TYPE_AUTO_CONFIGURE
#define MACH_OPTIONS_DAC_16_BIT_COLOR_MODE_DEFAULT\
	MACH_OPTIONS_DAC_16_BIT_COLOR_MODE_555
#define MACH_OPTIONS_DAC_24_BIT_COLOR_MODE_DEFAULT\
	MACH_OPTIONS_DAC_24_BIT_COLOR_MODE_RGBA
#define MACH_OPTIONS_DAC_ACCESS_DELAY_COUNT_DEFAULT\
	10
#define MACH_OPTIONS_DAC_BLANK_ADJUST_DEFAULT\
	-1
#define MACH_OPTIONS_DAC_MAX_FREQUENCY_DEFAULT\
	0
#define MACH_OPTIONS_DAC_NAME_DEFAULT\
	MACH_OPTIONS_DAC_NAME_AUTO_DETECT
#define MACH_OPTIONS_DAC_PIXEL_DELAY_DEFAULT\
	-1
#define MACH_OPTIONS_DAC_RGB_WIDTH_DEFAULT\
	MACH_OPTIONS_DAC_RGB_WIDTH_DEFAULT
#define MACH_OPTIONS_FONTDRAW_OPTIONS_DEFAULT\
	( MACH_OPTIONS_FONTDRAW_OPTIONS_DRAW_NON_TERMINAL_FONTS |MACH_OPTIONS_FONTDRAW_OPTIONS_DRAW_TERMINAL_FONTS |MACH_OPTIONS_FONTDRAW_OPTIONS_USE_OFFSCREEN_MEMORY |MACH_OPTIONS_FONTDRAW_OPTIONS_USE_IBM_MODE )
#define MACH_OPTIONS_GLYPH_CACHE_SIZE_DEFAULT\
	"128X128"
#define MACH_OPTIONS_GRAPHICS_ENGINE_FIFO_BLOCKING_FACTOR_DEFAULT\
	8
#define MACH_OPTIONS_GRAPHICS_ENGINE_LOOP_TIMEOUT_COUNT_DEFAULT\
	200000
#define MACH_OPTIONS_GRAPHICS_ENGINE_MICRO_DELAY_COUNT_DEFAULT\
	40
#define MACH_OPTIONS_HORIZONTAL_LINE_DRAW_OPTIMIZATIONS_DEFAULT\
	MACH_OPTIONS_HORIZONTAL_LINE_DRAW_OPTIMIZATIONS_ENABLED
#define MACH_OPTIONS_IO_BUS_WIDTH_DEFAULT\
	MACH_OPTIONS_IO_BUS_WIDTH_AUTO_DETECT
#define MACH_OPTIONS_LINEDRAW_OPTIONS_DEFAULT\
	( MACH_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW |MACH_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW |MACH_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES |MACH_OPTIONS_LINEDRAW_OPTIONS_USE_PATTERN_REGISTERS |MACH_OPTIONS_LINEDRAW_OPTIONS_USE_IBM_MODE )
#define MACH_OPTIONS_MAX_NUMBER_OF_GLYPHS_IN_DOWNLOADABLE_FONT_DEFAULT\
	256
#define MACH_OPTIONS_MODEDB_STRING_DEFAULT 0
#define MACH_OPTIONS_MONITOR_SYNC_TYPE_DEFAULT\
	MACH_OPTIONS_MONITOR_SYNC_TYPE_COMPOSITE_SYNC
#define MACH_OPTIONS_NUMBER_OF_DOWNLOADABLE_CURSORS_DEFAULT\
	1
#define MACH_OPTIONS_NUMBER_OF_DOWNLOADABLE_FONTS_DEFAULT\
	32
#define MACH_OPTIONS_NUMBER_OF_GRAPHICS_STATES_DEFAULT\
	8
#define MACH_OPTIONS_OFFSCREEN_STIPPLE_PADDED_HEIGHT_DEFAULT\
	32
#define MACH_OPTIONS_OFFSCREEN_STIPPLE_PADDED_WIDTH_DEFAULT\
	128
#define MACH_OPTIONS_OFFSCREEN_TILE_PADDED_HEIGHT_DEFAULT\
	32
#define MACH_OPTIONS_OFFSCREEN_TILE_PADDED_WIDTH_DEFAULT\
	128
#define MACH_OPTIONS_OMM_FULL_COALESCE_WATERMARK_DEFAULT\
	2
#define MACH_OPTIONS_OMM_HASH_LIST_SIZE_DEFAULT\
	512
#define MACH_OPTIONS_OMM_HORIZONTAL_CONSTRAINT_DEFAULT\
	0
#define MACH_OPTIONS_OMM_NAMED_ALLOCATION_LIST_DEFAULT\
	""
#define MACH_OPTIONS_OMM_NEIGHBOUR_LIST_INCREMENT_DEFAULT\
	20
#define MACH_OPTIONS_OMM_VERTICAL_CONSTRAINT_DEFAULT\
	0
#define MACH_OPTIONS_OVERSCAN_COLOR_DEFAULT 0
#define MACH_OPTIONS_OVERSCAN_H_DEFAULT\
	0
#define MACH_OPTIONS_OVERSCAN_V_DEFAULT\
	0
#define MACH_OPTIONS_PASSTHROUGH_OVERRIDE_DEFAULT\
	MACH_OPTIONS_PASSTHROUGH_OVERRIDE_DISABLED
#define MACH_OPTIONS_POINTDRAW_OPTIONS_DEFAULT\
	( MACH_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINT )
#define MACH_OPTIONS_RECTFILL_OPTIONS_DEFAULT\
	( MACH_OPTIONS_RECTFILL_OPTIONS_USE_SOLID_FILL_RECT |MACH_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT |MACH_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT |MACH_OPTIONS_RECTFILL_OPTIONS_USE_PATTERN_REGISTERS |MACH_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY |MACH_OPTIONS_RECTFILL_OPTIONS_USE_LARGE_OFFSCREEN_AREAS |MACH_OPTIONS_RECTFILL_OPTIONS_USE_IBM_MODE )
#define MACH_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_DEFAULT 0
#define MACH_OPTIONS_SI_INTERFACE_VERSION_DEFAULT\
	MACH_OPTIONS_SI_INTERFACE_VERSION_AUTO_CONFIGURE
#define MACH_OPTIONS_SPANSFILL_OPTIONS_DEFAULT\
	( MACH_OPTIONS_SPANSFILL_OPTIONS_USE_SOLID_FILL |MACH_OPTIONS_SPANSFILL_OPTIONS_USE_STIPPLE_FILL |MACH_OPTIONS_SPANSFILL_OPTIONS_USE_TILE_FILL |MACH_OPTIONS_SPANSFILL_OPTIONS_USE_PATTERN_REGISTERS )
#define MACH_OPTIONS_STATIC_COLORMAP_DESCRIPTION_FILE_DEFAULT 0
#define MACH_OPTIONS_STIPPLE_BEST_SIZE_DEFAULT\
	"32X32"
#define MACH_OPTIONS_TILE_BEST_SIZE_DEFAULT\
	"32X32"
#define MACH_OPTIONS_VERBOSE_STARTUP_DEFAULT\
	MACH_OPTIONS_VERBOSE_STARTUP_NO
#define MACH_OPTIONS_VIDEO_MEMORY_DIMENSIONS_DEFAULT 0
#define MACH_OPTIONS_VRAM_FIFO_DEPTH_DEFAULT\
	8
#define MACH_OPTIONS_VT_SWITCH_SAVE_LINES_DEFAULT\
	-1


#if (defined(__DEBUG__))
extern boolean	mach_options_debug ;
#endif


extern struct mach_options_structure *
mach_options_parse (struct mach_options_structure *option_struct_p,
			   const char *option_string_p)
;


#endif