/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)S3:S3/s3_options.h	1.14"

#if (! defined(__S3_OPTIONS_INCLUDED__))

#define __S3_OPTIONS_INCLUDED__



#include "stdenv.h"
#include "global.h"


enum s3_options_si_interface_version
{
	S3_OPTIONS_SI_INTERFACE_VERSION_1_0,
	S3_OPTIONS_SI_INTERFACE_VERSION_1_1,
	S3_OPTIONS_SI_INTERFACE_VERSION_AUTO_CONFIGURE,
	s3_options_si_interface_version_end_enumeration
};

enum s3_options_cursor_type
{
	S3_OPTIONS_CURSOR_TYPE_HARDWARE_CURSOR,
	S3_OPTIONS_CURSOR_TYPE_SOFTWARE_CURSOR,
	s3_options_cursor_type_end_enumeration
};

enum s3_options_monitor_sync_type
{
	S3_OPTIONS_MONITOR_SYNC_TYPE_COMPOSITE_SYNC,
	S3_OPTIONS_MONITOR_SYNC_TYPE_SYNC_ON_GREEN,
	S3_OPTIONS_MONITOR_SYNC_TYPE_SEPARATE_SYNC,
	s3_options_monitor_sync_type_end_enumeration
};

enum s3_options_vsync_polarity
{
	S3_OPTIONS_VSYNC_POLARITY_POSITIVE,
	S3_OPTIONS_VSYNC_POLARITY_NEGATIVE,
	S3_OPTIONS_VSYNC_POLARITY_DEFAULT,
	s3_options_vsync_polarity_end_enumeration
};

enum s3_options_hsync_polarity
{
	S3_OPTIONS_HSYNC_POLARITY_POSITIVE,
	S3_OPTIONS_HSYNC_POLARITY_NEGATIVE,
	S3_OPTIONS_HSYNC_POLARITY_DEFAULT,
	s3_options_hsync_polarity_end_enumeration
};

enum s3_options_wait_state_control
{
	S3_OPTIONS_WAIT_STATE_CONTROL_YES,
	S3_OPTIONS_WAIT_STATE_CONTROL_NO,
	s3_options_wait_state_control_end_enumeration
};

enum s3_options_enable_write_posting
{
	S3_OPTIONS_ENABLE_WRITE_POSTING_YES,
	S3_OPTIONS_ENABLE_WRITE_POSTING_NO,
	s3_options_enable_write_posting_end_enumeration
};

enum s3_options_enable_eprom_write
{
	S3_OPTIONS_ENABLE_EPROM_WRITE_YES,
	S3_OPTIONS_ENABLE_EPROM_WRITE_NO,
	s3_options_enable_eprom_write_end_enumeration
};

enum s3_options_enable_split_transfers
{
	S3_OPTIONS_ENABLE_SPLIT_TRANSFERS_YES,
	S3_OPTIONS_ENABLE_SPLIT_TRANSFERS_NO,
	s3_options_enable_split_transfers_end_enumeration
};

enum s3_options_enable_nibble_swap
{
	S3_OPTIONS_ENABLE_NIBBLE_SWAP_YES,
	S3_OPTIONS_ENABLE_NIBBLE_SWAP_NO,
	s3_options_enable_nibble_swap_end_enumeration
};

enum s3_options_enable_nibble_write_control
{
	S3_OPTIONS_ENABLE_NIBBLE_WRITE_CONTROL_YES,
	S3_OPTIONS_ENABLE_NIBBLE_WRITE_CONTROL_NO,
	s3_options_enable_nibble_write_control_end_enumeration
};

enum s3_options_vram_addressing_mode
{
	S3_OPTIONS_VRAM_ADDRESSING_MODE_PARALLEL,
	S3_OPTIONS_VRAM_ADDRESSING_MODE_SERIAL,
	s3_options_vram_addressing_mode_end_enumeration
};

enum s3_options_limit_write_posting
{
	S3_OPTIONS_LIMIT_WRITE_POSTING_YES,
	S3_OPTIONS_LIMIT_WRITE_POSTING_NO,
	s3_options_limit_write_posting_end_enumeration
};

enum s3_options_enable_read_ahead_cache
{
	S3_OPTIONS_ENABLE_READ_AHEAD_CACHE_YES,
	S3_OPTIONS_ENABLE_READ_AHEAD_CACHE_NO,
	s3_options_enable_read_ahead_cache_end_enumeration
};

enum s3_options_latch_isa_addr
{
	S3_OPTIONS_LATCH_ISA_ADDR_YES,
	S3_OPTIONS_LATCH_ISA_ADDR_NO,
	s3_options_latch_isa_addr_end_enumeration
};

enum s3_options_serial_access_mode_control
{
	S3_OPTIONS_SERIAL_ACCESS_MODE_CONTROL_512,
	S3_OPTIONS_SERIAL_ACCESS_MODE_CONTROL_256,
	s3_options_serial_access_mode_control_end_enumeration
};

enum s3_options_ras_m_clk
{
	S3_OPTIONS_RAS_M_CLK_7,
	S3_OPTIONS_RAS_M_CLK_6,
	s3_options_ras_m_clk_end_enumeration
};

enum s3_options_use_save_unders
{
	S3_OPTIONS_USE_SAVE_UNDERS_NO,
	S3_OPTIONS_USE_SAVE_UNDERS_YES,
	s3_options_use_save_unders_end_enumeration
};

enum s3_options_chipset_name
{
	S3_OPTIONS_CHIPSET_NAME_86C801,
	S3_OPTIONS_CHIPSET_NAME_86C911,
	S3_OPTIONS_CHIPSET_NAME_86C924,
	S3_OPTIONS_CHIPSET_NAME_86C928,
	S3_OPTIONS_CHIPSET_NAME_86C928_PCI,
	S3_OPTIONS_CHIPSET_NAME_AUTO_DETECT,
	s3_options_chipset_name_end_enumeration
};

enum s3_options_stepping_number
{
	S3_OPTIONS_STEPPING_NUMBER_A_STEP,
	S3_OPTIONS_STEPPING_NUMBER_B_STEP,
	S3_OPTIONS_STEPPING_NUMBER_C_STEP,
	S3_OPTIONS_STEPPING_NUMBER_D_STEP,
	S3_OPTIONS_STEPPING_NUMBER_E_STEP,
	S3_OPTIONS_STEPPING_NUMBER_G_STEP,
	S3_OPTIONS_STEPPING_NUMBER_AUTO_DETECT,
	s3_options_stepping_number_end_enumeration
};

enum s3_options_clock_chip_name
{
	S3_OPTIONS_CLOCK_CHIP_NAME_APPROXIMATE_VALUES,
	S3_OPTIONS_CLOCK_CHIP_NAME_CHRONTEL_CH9204B,
	S3_OPTIONS_CLOCK_CHIP_NAME_CHRONTEL_CH9204C,
	S3_OPTIONS_CLOCK_CHIP_NAME_ICD_2061,
	S3_OPTIONS_CLOCK_CHIP_NAME_ICD_2061A,
	S3_OPTIONS_CLOCK_CHIP_NAME_AV9194_56,
	S3_OPTIONS_CLOCK_CHIP_NAME_AV9194_07,
	S3_OPTIONS_CLOCK_CHIP_NAME_AV9194_11,
	S3_OPTIONS_CLOCK_CHIP_NAME_CHRONTEL_CH9294G,
	S3_OPTIONS_CLOCK_CHIP_NAME_TI_3025,
	S3_OPTIONS_CLOCK_CHIP_NAME_UNKNOWN,
	s3_options_clock_chip_name_end_enumeration
};

enum s3_options_s3_bus_width
{
	S3_OPTIONS_S3_BUS_WIDTH_16_BIT,
	S3_OPTIONS_S3_BUS_WIDTH_8_BIT,
	s3_options_s3_bus_width_end_enumeration
};

enum s3_options_fast_rmw
{
	S3_OPTIONS_FAST_RMW_YES,
	S3_OPTIONS_FAST_RMW_NO,
	s3_options_fast_rmw_end_enumeration
};

enum s3_options_enable_alternate_ioport_address
{
	S3_OPTIONS_ENABLE_ALTERNATE_IOPORT_ADDRESS_YES,
	S3_OPTIONS_ENABLE_ALTERNATE_IOPORT_ADDRESS_NO,
	s3_options_enable_alternate_ioport_address_end_enumeration
};

enum s3_options_dac_name
{
	S3_OPTIONS_DAC_NAME_ATT20C491,
	S3_OPTIONS_DAC_NAME_SC15025,
	S3_OPTIONS_DAC_NAME_SC11481_6_2_3,
	S3_OPTIONS_DAC_NAME_SC11485_7,
	S3_OPTIONS_DAC_NAME_SC11484_8,
	S3_OPTIONS_DAC_NAME_SC11489,
	S3_OPTIONS_DAC_NAME_SC11471_6,
	S3_OPTIONS_DAC_NAME_SC11478,
	S3_OPTIONS_DAC_NAME_IMSG171,
	S3_OPTIONS_DAC_NAME_IMSG176,
	S3_OPTIONS_DAC_NAME_IMSG178,
	S3_OPTIONS_DAC_NAME_BT471_6,
	S3_OPTIONS_DAC_NAME_BT478,
	S3_OPTIONS_DAC_NAME_BT485KPJ110,
	S3_OPTIONS_DAC_NAME_BT485KPJ135,
	S3_OPTIONS_DAC_NAME_W82C478,
	S3_OPTIONS_DAC_NAME_W82C476,
	S3_OPTIONS_DAC_NAME_W82C490,
	S3_OPTIONS_DAC_NAME_TR9C1710,
	S3_OPTIONS_DAC_NAME_ATT20C490,
	S3_OPTIONS_DAC_NAME_SS2410,
	S3_OPTIONS_DAC_NAME_TI3020,
	S3_OPTIONS_DAC_NAME_TI3025,
	S3_OPTIONS_DAC_NAME_UNKNOWN_DAC,
	s3_options_dac_name_end_enumeration
};

enum s3_options_use_clock_doubler
{
	S3_OPTIONS_USE_CLOCK_DOUBLER_NO,
	S3_OPTIONS_USE_CLOCK_DOUBLER_AUTO_CONFIGURE,
	s3_options_use_clock_doubler_end_enumeration
};

enum s3_options_use_dac_external_sid_mode
{
	S3_OPTIONS_USE_DAC_EXTERNAL_SID_MODE_NO,
	S3_OPTIONS_USE_DAC_EXTERNAL_SID_MODE_AUTO_CONFIGURE,
	s3_options_use_dac_external_sid_mode_end_enumeration
};

enum s3_options_dac_rgb_width
{
	S3_OPTIONS_DAC_RGB_WIDTH_6,
	S3_OPTIONS_DAC_RGB_WIDTH_8,
	S3_OPTIONS_DAC_RGB_WIDTH_DEFAULT,
	s3_options_dac_rgb_width_end_enumeration
};

enum s3_options_dac_24_bit_color_mode
{
	S3_OPTIONS_DAC_24_BIT_COLOR_MODE_RGBA,
	S3_OPTIONS_DAC_24_BIT_COLOR_MODE_ABGR,
	s3_options_dac_24_bit_color_mode_end_enumeration
};

enum s3_options_dac_16_bit_color_mode
{
	S3_OPTIONS_DAC_16_BIT_COLOR_MODE_555,
	S3_OPTIONS_DAC_16_BIT_COLOR_MODE_565,
	S3_OPTIONS_DAC_16_BIT_COLOR_MODE_655,
	S3_OPTIONS_DAC_16_BIT_COLOR_MODE_664,
	s3_options_dac_16_bit_color_mode_end_enumeration
};

enum s3_options_override_ss_stippling
{
	S3_OPTIONS_OVERRIDE_SS_STIPPLING_YES,
	S3_OPTIONS_OVERRIDE_SS_STIPPLING_NO,
	s3_options_override_ss_stippling_end_enumeration
};


#define	S3_OPTIONS_MEMORY_AND_REGISTER_ACCESS_MODE_IO_ACCESS	1
#define	S3_OPTIONS_MEMORY_AND_REGISTER_ACCESS_MODE_MMIO_READ_ENHANCED_REGS	2
#define	S3_OPTIONS_MEMORY_AND_REGISTER_ACCESS_MODE_MMIO_WRITE_ENHANCED_REGS	4
#define	S3_OPTIONS_MEMORY_AND_REGISTER_ACCESS_MODE_MMIO_READ_PIXTRANS	8
#define	S3_OPTIONS_MEMORY_AND_REGISTER_ACCESS_MODE_MMIO_WRITE_PIXTRANS	16
#define	S3_OPTIONS_MEMORY_AND_REGISTER_ACCESS_MODE_LFB_ACCESS	32


#define	S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_GRAY	1
#define	S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_GRAY_SCALE	2
#define	S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_STATIC_COLOR	4
#define	S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_PSEUDO_COLOR	8
#define	S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_TRUE_COLOR	16
#define	S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_DIRECT_COLOR	32


#define	S3_OPTIONS_RECTFILL_OPTIONS_USE_SOLID_FILL_RECT	1
#define	S3_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT	2
#define	S3_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT	4
#define	S3_OPTIONS_RECTFILL_OPTIONS_USE_GE_PATFILL	8
#define	S3_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY	16
#define	S3_OPTIONS_RECTFILL_OPTIONS_REDUCE_STIPPLES	32
#define	S3_OPTIONS_RECTFILL_OPTIONS_REDUCE_TILES	64
#define	S3_OPTIONS_RECTFILL_OPTIONS_NONE	128


#define	S3_OPTIONS_BITBLT_OPTIONS_USE_SS_BITBLT	1
#define	S3_OPTIONS_BITBLT_OPTIONS_USE_MS_BITBLT	2
#define	S3_OPTIONS_BITBLT_OPTIONS_USE_SM_BITBLT	4
#define	S3_OPTIONS_BITBLT_OPTIONS_USE_MS_STPLBLT	8
#define	S3_OPTIONS_BITBLT_OPTIONS_NONE	16


#define	S3_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW	1
#define	S3_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES	2
#define	S3_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW	4
#define	S3_OPTIONS_LINEDRAW_OPTIONS_USE_DASHED_LINE	8
#define	S3_OPTIONS_LINEDRAW_OPTIONS_NONE	16


#define	S3_OPTIONS_SPANSFILL_OPTIONS_USE_SOLID_FILL	1
#define	S3_OPTIONS_SPANSFILL_OPTIONS_USE_STIPPLE_FILL	2
#define	S3_OPTIONS_SPANSFILL_OPTIONS_USE_TILE_FILL	4
#define	S3_OPTIONS_SPANSFILL_OPTIONS_USE_GE_PATFILL	8
#define	S3_OPTIONS_SPANSFILL_OPTIONS_USE_OFFSCREEN_MEMORY	16
#define	S3_OPTIONS_SPANSFILL_OPTIONS_NONE	32


#define	S3_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINT	1
#define	S3_OPTIONS_POINTDRAW_OPTIONS_NONE	2


#define	S3_OPTIONS_FONTDRAW_OPTIONS_DRAW_NON_TERMINAL_FONTS	1
#define	S3_OPTIONS_FONTDRAW_OPTIONS_DRAW_TERMINAL_FONTS	2
#define	S3_OPTIONS_FONTDRAW_OPTIONS_USE_OFFSCREEN_MEMORY	4
#define	S3_OPTIONS_FONTDRAW_OPTIONS_ASSEMBLE_GLYPHS	8
#define	S3_OPTIONS_FONTDRAW_OPTIONS_NONE	16


#define	S3_OPTIONS_ARCDRAW_OPTIONS_DRAW_ONE_BIT_ARCS	1




struct s3_options_structure
{
	int arc_cache_size;
	unsigned int arcdraw_options;
	unsigned int bitblt_options;
	int bitmap_reduction_threshold;
	enum s3_options_chipset_name chipset_name;
	enum s3_options_clock_chip_name clock_chip_name;
	int clock_doubler_threshold;
	char *crtc_parameters;
	int crtc_start_offset;
	int crtc_sync_timeout_count;
	char *cursor_max_size;
	enum s3_options_cursor_type cursor_type;
	enum s3_options_dac_16_bit_color_mode dac_16_bit_color_mode;
	enum s3_options_dac_24_bit_color_mode dac_24_bit_color_mode;
	int dac_access_delay_count;
	int dac_external_sid_threshold;
	int dac_max_frequency;
	enum s3_options_dac_name dac_name;
	enum s3_options_dac_rgb_width dac_rgb_width;
	int decode_wait_control;
	int display_skew;
	enum s3_options_enable_alternate_ioport_address enable_alternate_ioport_address;
	enum s3_options_enable_eprom_write enable_eprom_write;
	enum s3_options_enable_nibble_swap enable_nibble_swap;
	enum s3_options_enable_nibble_write_control enable_nibble_write_control;
	enum s3_options_enable_read_ahead_cache enable_read_ahead_cache;
	enum s3_options_enable_split_transfers enable_split_transfers;
	enum s3_options_enable_write_posting enable_write_posting;
	enum s3_options_fast_rmw fast_rmw;
	unsigned int fontdraw_options;
	char *glyph_cache_size;
	int graphics_engine_fifo_blocking_factor;
	int graphics_engine_loop_timeout_count;
	int graphics_engine_micro_delay_count;
	int horizontal_skew;
	enum s3_options_hsync_polarity hsync_polarity;
	enum s3_options_latch_isa_addr latch_isa_addr;
	enum s3_options_limit_write_posting limit_write_posting;
	int linear_frame_buffer_size;
	unsigned int linedraw_options;
	int max_fully_cacheable_font_size;
	int max_number_of_glyphs_in_downloadable_font;
	int maximum_offscreen_downloadable_bitmap_height;
	int maximum_offscreen_downloadable_bitmap_width;
	unsigned int memory_and_register_access_mode;
	char *modedb_string;
	enum s3_options_monitor_sync_type monitor_sync_type;
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
	enum s3_options_override_ss_stippling override_ss_stippling;
	unsigned int pointdraw_options;
	int rac_extra_prefetch;
	enum s3_options_ras_m_clk ras_m_clk;
	int read_wait_control;
	unsigned int rectfill_options;
	char *register_values_string;
	enum s3_options_s3_bus_width s3_bus_width;
	unsigned int screen_8_4_bit_visual_list;
	enum s3_options_serial_access_mode_control serial_access_mode_control;
	enum s3_options_si_interface_version si_interface_version;
	unsigned int spansfill_options;
	char *static_colormap_description_file;
	enum s3_options_stepping_number stepping_number;
	char *stipple_best_size;
	char *tile_best_size;
	enum s3_options_use_clock_doubler use_clock_doubler;
	enum s3_options_use_dac_external_sid_mode use_dac_external_sid_mode;
	enum s3_options_use_save_unders use_save_unders;
	int verbose_startup;
	enum s3_options_vram_addressing_mode vram_addressing_mode;
	enum s3_options_vsync_polarity vsync_polarity;
	int vt_switch_save_lines;
	enum s3_options_wait_state_control wait_state_control;

};

/*
 * Names of the option defaults
 */

#define S3_OPTIONS_ARC_CACHE_SIZE_DEFAULT\
	16
#define S3_OPTIONS_ARCDRAW_OPTIONS_DEFAULT\
	( S3_OPTIONS_ARCDRAW_OPTIONS_DRAW_ONE_BIT_ARCS )
#define S3_OPTIONS_BITBLT_OPTIONS_DEFAULT\
	( S3_OPTIONS_BITBLT_OPTIONS_USE_MS_BITBLT |S3_OPTIONS_BITBLT_OPTIONS_USE_SM_BITBLT |S3_OPTIONS_BITBLT_OPTIONS_USE_SS_BITBLT |S3_OPTIONS_BITBLT_OPTIONS_USE_MS_STPLBLT )
#define S3_OPTIONS_BITMAP_REDUCTION_THRESHOLD_DEFAULT\
	32
#define S3_OPTIONS_CHIPSET_NAME_DEFAULT\
	S3_OPTIONS_CHIPSET_NAME_AUTO_DETECT
#define S3_OPTIONS_CLOCK_CHIP_NAME_DEFAULT\
	S3_OPTIONS_CLOCK_CHIP_NAME_UNKNOWN
#define S3_OPTIONS_CLOCK_DOUBLER_THRESHOLD_DEFAULT\
	0
#define S3_OPTIONS_CRTC_PARAMETERS_DEFAULT 0
#define S3_OPTIONS_CRTC_START_OFFSET_DEFAULT\
	0
#define S3_OPTIONS_CRTC_SYNC_TIMEOUT_COUNT_DEFAULT\
	100000
#define S3_OPTIONS_CURSOR_MAX_SIZE_DEFAULT 0
#define S3_OPTIONS_CURSOR_TYPE_DEFAULT\
	S3_OPTIONS_CURSOR_TYPE_HARDWARE_CURSOR
#define S3_OPTIONS_DAC_16_BIT_COLOR_MODE_DEFAULT\
	S3_OPTIONS_DAC_16_BIT_COLOR_MODE_555
#define S3_OPTIONS_DAC_24_BIT_COLOR_MODE_DEFAULT\
	S3_OPTIONS_DAC_24_BIT_COLOR_MODE_RGBA
#define S3_OPTIONS_DAC_ACCESS_DELAY_COUNT_DEFAULT\
	1000
#define S3_OPTIONS_DAC_EXTERNAL_SID_THRESHOLD_DEFAULT\
	0
#define S3_OPTIONS_DAC_MAX_FREQUENCY_DEFAULT\
	0
#define S3_OPTIONS_DAC_NAME_DEFAULT\
	S3_OPTIONS_DAC_NAME_UNKNOWN_DAC
#define S3_OPTIONS_DAC_RGB_WIDTH_DEFAULT\
	S3_OPTIONS_DAC_RGB_WIDTH_6
#define S3_OPTIONS_DECODE_WAIT_CONTROL_DEFAULT\
	3
#define S3_OPTIONS_DISPLAY_SKEW_DEFAULT\
	0
#define S3_OPTIONS_ENABLE_ALTERNATE_IOPORT_ADDRESS_DEFAULT\
	S3_OPTIONS_ENABLE_ALTERNATE_IOPORT_ADDRESS_NO
#define S3_OPTIONS_ENABLE_EPROM_WRITE_DEFAULT\
	S3_OPTIONS_ENABLE_EPROM_WRITE_NO
#define S3_OPTIONS_ENABLE_NIBBLE_SWAP_DEFAULT\
	S3_OPTIONS_ENABLE_NIBBLE_SWAP_NO
#define S3_OPTIONS_ENABLE_NIBBLE_WRITE_CONTROL_DEFAULT\
	S3_OPTIONS_ENABLE_NIBBLE_WRITE_CONTROL_NO
#define S3_OPTIONS_ENABLE_READ_AHEAD_CACHE_DEFAULT\
	S3_OPTIONS_ENABLE_READ_AHEAD_CACHE_NO
#define S3_OPTIONS_ENABLE_SPLIT_TRANSFERS_DEFAULT\
	S3_OPTIONS_ENABLE_SPLIT_TRANSFERS_YES
#define S3_OPTIONS_ENABLE_WRITE_POSTING_DEFAULT\
	S3_OPTIONS_ENABLE_WRITE_POSTING_NO
#define S3_OPTIONS_FAST_RMW_DEFAULT\
	S3_OPTIONS_FAST_RMW_NO
#define S3_OPTIONS_FONTDRAW_OPTIONS_DEFAULT\
	( S3_OPTIONS_FONTDRAW_OPTIONS_DRAW_NON_TERMINAL_FONTS |S3_OPTIONS_FONTDRAW_OPTIONS_DRAW_TERMINAL_FONTS |S3_OPTIONS_FONTDRAW_OPTIONS_USE_OFFSCREEN_MEMORY |S3_OPTIONS_FONTDRAW_OPTIONS_ASSEMBLE_GLYPHS )
#define S3_OPTIONS_GLYPH_CACHE_SIZE_DEFAULT\
	"64X64"
#define S3_OPTIONS_GRAPHICS_ENGINE_FIFO_BLOCKING_FACTOR_DEFAULT\
	8
#define S3_OPTIONS_GRAPHICS_ENGINE_LOOP_TIMEOUT_COUNT_DEFAULT\
	200000
#define S3_OPTIONS_GRAPHICS_ENGINE_MICRO_DELAY_COUNT_DEFAULT\
	40
#define S3_OPTIONS_HORIZONTAL_SKEW_DEFAULT\
	0
#define S3_OPTIONS_HSYNC_POLARITY_DEFAULT\
	S3_OPTIONS_HSYNC_POLARITY_DEFAULT
#define S3_OPTIONS_LATCH_ISA_ADDR_DEFAULT\
	S3_OPTIONS_LATCH_ISA_ADDR_NO
#define S3_OPTIONS_LIMIT_WRITE_POSTING_DEFAULT\
	S3_OPTIONS_LIMIT_WRITE_POSTING_NO
#define S3_OPTIONS_LINEAR_FRAME_BUFFER_SIZE_DEFAULT\
	0
#define S3_OPTIONS_LINEDRAW_OPTIONS_DEFAULT\
	( S3_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW |S3_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW |S3_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES |S3_OPTIONS_LINEDRAW_OPTIONS_USE_DASHED_LINE )
#define S3_OPTIONS_MAX_FULLY_CACHEABLE_FONT_SIZE_DEFAULT\
	1
#define S3_OPTIONS_MAX_NUMBER_OF_GLYPHS_IN_DOWNLOADABLE_FONT_DEFAULT\
	256
#define S3_OPTIONS_MAXIMUM_OFFSCREEN_DOWNLOADABLE_BITMAP_HEIGHT_DEFAULT\
	256
#define S3_OPTIONS_MAXIMUM_OFFSCREEN_DOWNLOADABLE_BITMAP_WIDTH_DEFAULT\
	256
#define S3_OPTIONS_MEMORY_AND_REGISTER_ACCESS_MODE_DEFAULT\
	( S3_OPTIONS_MEMORY_AND_REGISTER_ACCESS_MODE_IO_ACCESS )
#define S3_OPTIONS_MODEDB_STRING_DEFAULT 0
#define S3_OPTIONS_MONITOR_SYNC_TYPE_DEFAULT\
	S3_OPTIONS_MONITOR_SYNC_TYPE_COMPOSITE_SYNC
#define S3_OPTIONS_NUMBER_OF_DOWNLOADABLE_CURSORS_DEFAULT\
	1
#define S3_OPTIONS_NUMBER_OF_DOWNLOADABLE_FONTS_DEFAULT\
	17
#define S3_OPTIONS_NUMBER_OF_GRAPHICS_STATES_DEFAULT\
	4
#define S3_OPTIONS_OFFSCREEN_STIPPLE_PADDED_HEIGHT_DEFAULT\
	32
#define S3_OPTIONS_OFFSCREEN_STIPPLE_PADDED_WIDTH_DEFAULT\
	128
#define S3_OPTIONS_OFFSCREEN_TILE_PADDED_HEIGHT_DEFAULT\
	32
#define S3_OPTIONS_OFFSCREEN_TILE_PADDED_WIDTH_DEFAULT\
	128
#define S3_OPTIONS_OMM_FULL_COALESCE_WATERMARK_DEFAULT\
	2
#define S3_OPTIONS_OMM_HASH_LIST_SIZE_DEFAULT\
	0
#define S3_OPTIONS_OMM_HORIZONTAL_CONSTRAINT_DEFAULT\
	0
#define S3_OPTIONS_OMM_NAMED_ALLOCATION_LIST_DEFAULT\
	""
#define S3_OPTIONS_OMM_NEIGHBOUR_LIST_INCREMENT_DEFAULT\
	0
#define S3_OPTIONS_OMM_VERTICAL_CONSTRAINT_DEFAULT\
	0
#define S3_OPTIONS_OVERRIDE_SS_STIPPLING_DEFAULT\
	S3_OPTIONS_OVERRIDE_SS_STIPPLING_YES
#define S3_OPTIONS_POINTDRAW_OPTIONS_DEFAULT\
	( S3_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINT )
#define S3_OPTIONS_RAC_EXTRA_PREFETCH_DEFAULT\
	0
#define S3_OPTIONS_RAS_M_CLK_DEFAULT\
	S3_OPTIONS_RAS_M_CLK_7
#define S3_OPTIONS_READ_WAIT_CONTROL_DEFAULT\
	3
#define S3_OPTIONS_RECTFILL_OPTIONS_DEFAULT\
	( S3_OPTIONS_RECTFILL_OPTIONS_USE_SOLID_FILL_RECT |S3_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT |S3_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT |S3_OPTIONS_RECTFILL_OPTIONS_USE_GE_PATFILL |S3_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY |S3_OPTIONS_RECTFILL_OPTIONS_REDUCE_STIPPLES |S3_OPTIONS_RECTFILL_OPTIONS_REDUCE_TILES )
#define S3_OPTIONS_REGISTER_VALUES_STRING_DEFAULT\
	""
#define S3_OPTIONS_S3_BUS_WIDTH_DEFAULT\
	S3_OPTIONS_S3_BUS_WIDTH_16_BIT
#define S3_OPTIONS_SCREEN_8_4_BIT_VISUAL_LIST_DEFAULT 0
#define S3_OPTIONS_SERIAL_ACCESS_MODE_CONTROL_DEFAULT\
	S3_OPTIONS_SERIAL_ACCESS_MODE_CONTROL_512
#define S3_OPTIONS_SI_INTERFACE_VERSION_DEFAULT\
	S3_OPTIONS_SI_INTERFACE_VERSION_AUTO_CONFIGURE
#define S3_OPTIONS_SPANSFILL_OPTIONS_DEFAULT\
	( S3_OPTIONS_SPANSFILL_OPTIONS_USE_SOLID_FILL |S3_OPTIONS_SPANSFILL_OPTIONS_USE_TILE_FILL |S3_OPTIONS_SPANSFILL_OPTIONS_USE_STIPPLE_FILL )
#define S3_OPTIONS_STATIC_COLORMAP_DESCRIPTION_FILE_DEFAULT 0
#define S3_OPTIONS_STEPPING_NUMBER_DEFAULT\
	S3_OPTIONS_STEPPING_NUMBER_AUTO_DETECT
#define S3_OPTIONS_STIPPLE_BEST_SIZE_DEFAULT\
	"64X64"
#define S3_OPTIONS_TILE_BEST_SIZE_DEFAULT\
	"64X64"
#define S3_OPTIONS_USE_CLOCK_DOUBLER_DEFAULT\
	S3_OPTIONS_USE_CLOCK_DOUBLER_AUTO_CONFIGURE
#define S3_OPTIONS_USE_DAC_EXTERNAL_SID_MODE_DEFAULT\
	S3_OPTIONS_USE_DAC_EXTERNAL_SID_MODE_AUTO_CONFIGURE
#define S3_OPTIONS_USE_SAVE_UNDERS_DEFAULT\
	S3_OPTIONS_USE_SAVE_UNDERS_NO
#define S3_OPTIONS_VERBOSE_STARTUP_DEFAULT\
	0
#define S3_OPTIONS_VRAM_ADDRESSING_MODE_DEFAULT\
	S3_OPTIONS_VRAM_ADDRESSING_MODE_SERIAL
#define S3_OPTIONS_VSYNC_POLARITY_DEFAULT\
	S3_OPTIONS_VSYNC_POLARITY_DEFAULT
#define S3_OPTIONS_VT_SWITCH_SAVE_LINES_DEFAULT\
	4096
#define S3_OPTIONS_WAIT_STATE_CONTROL_DEFAULT\
	S3_OPTIONS_WAIT_STATE_CONTROL_YES


#if (defined(__DEBUG__))
extern boolean	s3_options_debug ;
#endif


extern struct s3_options_structure *
s3_options_parse (struct s3_options_structure *option_struct_p,
			   const char *option_string_p)
;


#endif
