/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ld:common/main.c	1.39"
/*
 * ld -- link/editor main program
 */

/****************************************
** Imports
****************************************/

#include	<unistd.h>
#include	"globals.h"
#include	"macros.h"
#include	"locale.h"
#include	"sgs.h"

/****************************************
** Main Function Declaration
****************************************/

/*
 * The main program
 */
main(argc, argv)
	int argc;
	char** argv;
{
	char   label[256];

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxcds");
	(void)sprintf(label,"UX:%sld",SGS);
	(void)setlabel(label);
	init_signals();
	libver = EV_CURRENT;
	if (elf_version(EV_CURRENT) == EV_NONE)
		lderror(MSG_ELF,gettxt(":1076","libelf is out of date"));
	 
	process_flags(argc, argv);
	check_flags(argc);
	lib_setup();
	process_files(argc, argv);

	process_second_level_needed_so();

#ifdef	DEBUG
	if (debug_bits[DBG_MAIN])
		mapprint("");
#endif	/* DEBUG */
	if (dmode || aflag ) {
		build_specsym(ETEXT_SYM,ETEXT_USYM);
		build_specsym(EDATA_SYM,EDATA_USYM);
		build_specsym(END_SYM,END_USYM);
		build_specsym(DYN_SYM,DYN_USYM);
	}
	if ( outfile_name != NULL)
		if ( find_infile(outfile_name) != NULL){
			char *temp = outfile_name;
			outfile_name=NULL;
			lderror(MSG_FATAL, gettxt(":22","-o would overwrite %s\n"), temp);
		}

       	if (!rflag && sym_find(GOT_USYM, NOHASH) != 0)
		build_specsym(GOT_SYM,GOT_USYM);

	if (Qflag == SET_TRUE)
		make_comment();
	if( ((!dmode && aflag) && (interp_path != NULL)) || (dmode && !Gflag))
		make_interp();
	if (Gflag && bsym_list)
		mark_bsymbolic();
	if (Gflag)

		if( Bflag_hide == LIST && bhide_list)
			mark_hidden_symbols();
		else if (Bflag_hide == ALL || 
		        (Bflag_hide == NONE && Bflag_export == LIST && bexport_list))
			mark_def_globals_as_hidden();	
	

	count_relentries();

	if( dmode )
		make_dyn();
	if(dmode && !Gflag){
		if (znflag)
			add_undefs_to_dynsymtab();

		if (Bflag_export == ALL || 
		   (Bflag_export == NONE && Bflag_hide == LIST && bhide_list))
			add_exports_to_dynsymtab();
		else if (Bflag_export == LIST && bexport_list)
			add_export_list_to_dynsymtab();

		lookup_profiling_symbol();

		make_dynstrtab();
		make_dynsymtab();
	}

	if( Gflag || rflag || !sflag){
		make_strtab();
		make_symtab();
	}
	if (dmode)
	    	make_hash();
/* always after all the make_*'s so that all section names are accounted for */
	make_shstrtab();
	open_out();
	set_off_addr();
	if (mflag)
		ldmap_out();
	update_syms();
	update_syn_value();
	relocate();
	finish_out();
	exit(EXIT_SUCCESS);
	/* NOTREACHED */
}
