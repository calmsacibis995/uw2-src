/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)wksh:xmwksym.c	1.1"
/*#include <sys/types.h>*/
/*#include <sys/stropts.h>*/
#define SYMS_ONLY
#include "xksh.h"
extern unsigned long All_libs;
extern unsigned long Deflist;
extern unsigned long Ndeflist;
extern unsigned long Nstructlist;
extern unsigned long Nsymlist;
extern unsigned long Pr_tmpnonames;
extern unsigned long Prfailpoint;
extern unsigned long Promptfunc;
extern unsigned long STR_string_t;
extern unsigned long STR_uint;
extern unsigned long STR_ulong;
extern unsigned long Structlist;
extern unsigned long Symlist;
extern unsigned long T_char;
extern unsigned long T_dint;
extern unsigned long T_dlong;
extern unsigned long T_dshort;
extern unsigned long T_int;
extern unsigned long T_intp;
extern unsigned long T_long;
extern unsigned long T_longp;
extern unsigned long T_short;
extern unsigned long T_string_t;
extern unsigned long T_uint;
extern unsigned long T_ulong;
extern unsigned long T_unchar;
extern unsigned long T_ushort;
extern unsigned long Toplevel;
extern unsigned long Xk_errno;
extern unsigned long Xkdebug;
extern unsigned long _Prdebug;
extern unsigned long _ctype1;
extern unsigned long _ctype2;
extern unsigned long _stak_cur;
extern unsigned long _stakgrow;
extern unsigned long add_structlist;
extern unsigned long add_symbolic;
extern unsigned long alias_names;
extern unsigned long aliasload;
extern unsigned long all_tbl_find;
extern unsigned long all_tbl_search;
extern unsigned long altflush;
extern unsigned long altfprintf;
extern unsigned long altfputs;
extern unsigned long altgets;
extern unsigned long altperror;
extern unsigned long altprintf;
extern unsigned long altprompt;
extern unsigned long altputs;
extern unsigned long arg_build;
extern unsigned long arg_clear;
extern unsigned long arg_dolminus;
extern unsigned long arg_free;
extern unsigned long arg_new;
extern unsigned long arg_opts;
extern unsigned long arg_reset;
extern unsigned long arg_set;
extern unsigned long arg_use;
extern unsigned long array_dotset;
extern unsigned long array_find;
extern unsigned long array_grow;
extern unsigned long array_next;
extern unsigned long array_subscript;
extern unsigned long asl_find;
extern unsigned long asl_set;
extern unsigned long b_alias;
extern unsigned long b_bgfg;
extern unsigned long b_break;
extern unsigned long b_chdir;
extern unsigned long b_continue;
extern unsigned long b_dot;
extern unsigned long b_echo;
extern unsigned long b_eval;
extern unsigned long b_exec;
extern unsigned long b_export;
extern unsigned long b_fc;
extern unsigned long b_getopts;
extern unsigned long b_jobs;
extern unsigned long b_kill;
extern unsigned long b_let;
extern unsigned long b_login;
extern unsigned long b_null;
extern unsigned long b_print;
extern unsigned long b_pwd;
extern unsigned long b_read;
extern unsigned long b_readonly;
extern unsigned long b_ret_exit;
extern unsigned long b_set;
extern unsigned long b_shift;
extern unsigned long b_test;
extern unsigned long b_times;
extern unsigned long b_trap;
extern unsigned long b_typeset;
extern unsigned long b_ulimit;
extern unsigned long b_umask;
extern unsigned long b_unalias;
extern unsigned long b_unset;
extern unsigned long b_wait;
extern unsigned long b_whence;
extern unsigned long basedefs;
extern unsigned long basemems;
extern unsigned long built_ins;
extern unsigned long call_init;
extern unsigned long close_pipes;
extern unsigned long do_XBell;
extern unsigned long do_XDraw;
extern unsigned long do_XFlush;
extern unsigned long do_XmCreateArrowButton;
extern unsigned long do_XmCreateArrowButtonGadget;
extern unsigned long do_XmCreateBulletinBoard;
extern unsigned long do_XmCreateBulletinBoardDialog;
extern unsigned long do_XmCreateCascadeButton;
extern unsigned long do_XmCreateCascadeButtonGadget;
extern unsigned long do_XmCreateCommand;
extern unsigned long do_XmCreateDialogShell;
extern unsigned long do_XmCreateDrawingArea;
extern unsigned long do_XmCreateDrawnButton;
extern unsigned long do_XmCreateErrorDialog;
extern unsigned long do_XmCreateFileSelectionBox;
extern unsigned long do_XmCreateFileSelectionDialog;
extern unsigned long do_XmCreateForm;
extern unsigned long do_XmCreateFormDialog;
extern unsigned long do_XmCreateFrame;
extern unsigned long do_XmCreateInformationDialog;
extern unsigned long do_XmCreateLabel;
extern unsigned long do_XmCreateLabelGadget;
extern unsigned long do_XmCreateList;
extern unsigned long do_XmCreateMainWindow;
extern unsigned long do_XmCreateMenuBar;
extern unsigned long do_XmCreateMenuShell;
extern unsigned long do_XmCreateMessageBox;
extern unsigned long do_XmCreateMessageDialog;
extern unsigned long do_XmCreateOptionMenu;
extern unsigned long do_XmCreatePanedWindow;
extern unsigned long do_XmCreatePopupMenu;
extern unsigned long do_XmCreatePromptDialog;
extern unsigned long do_XmCreatePulldownMenu;
extern unsigned long do_XmCreatePushButton;
extern unsigned long do_XmCreatePushButtonGadget;
extern unsigned long do_XmCreateQuestionDialog;
extern unsigned long do_XmCreateRadioBox;
extern unsigned long do_XmCreateRowColumn;
extern unsigned long do_XmCreateScale;
extern unsigned long do_XmCreateScrollBar;
extern unsigned long do_XmCreateScrolledList;
extern unsigned long do_XmCreateScrolledText;
extern unsigned long do_XmCreateScrolledWindow;
extern unsigned long do_XmCreateSelectionBox;
extern unsigned long do_XmCreateSelectionDialog;
extern unsigned long do_XmCreateSeparator;
extern unsigned long do_XmCreateSeparatorGadget;
extern unsigned long do_XmCreateText;
extern unsigned long do_XmCreateToggleButton;
extern unsigned long do_XmCreateToggleButtonGadget;
extern unsigned long do_XmCreateWarningDialog;
extern unsigned long do_XmCreateWorkingDialog;
extern unsigned long do_XtAddCallback;
extern unsigned long do_XtAddInput;
extern unsigned long do_XtAddTimeOut;
extern unsigned long do_XtAppCreateShell;
extern unsigned long do_XtAppInitialize;
extern unsigned long do_XtCallCallbacks;
extern unsigned long do_XtCreateManagedWidget;
extern unsigned long do_XtCreatePopupShell;
extern unsigned long do_XtCreateWidget;
extern unsigned long do_XtDestroyWidget;
extern unsigned long do_XtGetValues;
extern unsigned long do_XtIsManaged;
extern unsigned long do_XtIsRealized;
extern unsigned long do_XtIsSensitive;
extern unsigned long do_XtMainLoop;
extern unsigned long do_XtManageChildren;
extern unsigned long do_XtMapWidget;
extern unsigned long do_XtPopdown;
extern unsigned long do_XtPopup;
extern unsigned long do_XtRealizeWidget;
extern unsigned long do_XtRemoveAllCallbacks;
extern unsigned long do_XtSetSensitive;
extern unsigned long do_XtSetValues;
extern unsigned long do_XtUnmanageChildren;
extern unsigned long do_XtUnmapWidget;
extern unsigned long do_XtUnrealizeWidget;
extern unsigned long do_call;
extern unsigned long do_cmdload;
extern unsigned long do_define;
extern unsigned long do_deflist;
extern unsigned long do_deref;
extern unsigned long do_field_comp;
extern unsigned long do_field_get;
extern unsigned long do_finddef;
extern unsigned long do_findsym;
extern unsigned long do_init;
extern unsigned long do_libinit;
extern unsigned long do_libload;
extern unsigned long do_managelist_func;
extern unsigned long do_sizeof;
extern unsigned long do_struct;
extern unsigned long do_structlist;
extern unsigned long do_symbolic;
extern unsigned long do_typedef;
extern unsigned long do_widlist;
extern unsigned long do_widload;
extern unsigned long e_Done;
extern unsigned long e_Running;
extern unsigned long e_access;
extern unsigned long e_alias;
extern unsigned long e_aliname;
extern unsigned long e_ambiguous;
extern unsigned long e_argexp;
extern unsigned long e_arglist;
extern unsigned long e_atline;
extern unsigned long e_badcolon;
extern unsigned long e_bltfn;
extern unsigned long e_bracket;
extern unsigned long e_colon;
extern unsigned long e_coredump;
extern unsigned long e_create;
extern unsigned long e_crondir;
extern unsigned long e_defedit;
extern unsigned long e_defpath;
extern unsigned long e_devfd;
extern unsigned long e_devfdNN;
extern unsigned long e_devnull;
extern unsigned long e_direct;
extern unsigned long e_divzero;
extern unsigned long e_dot;
extern unsigned long e_echobin;
extern unsigned long e_echoflag;
extern unsigned long e_endoffile;
extern unsigned long e_envmarker;
extern unsigned long e_exec;
extern unsigned long e_fexists;
extern unsigned long e_file;
extern unsigned long e_flimit;
extern unsigned long e_fnhdr;
extern unsigned long e_fork;
extern unsigned long e_format;
extern unsigned long e_found;
extern unsigned long e_function;
extern unsigned long e_hdigits;
extern unsigned long e_heading;
extern unsigned long e_history;
extern unsigned long e_ident;
extern unsigned long e_intbase;
extern unsigned long e_jobusage;
extern unsigned long e_kill;
extern unsigned long e_killcolon;
extern unsigned long e_libacc;
extern unsigned long e_libbad;
extern unsigned long e_libmax;
extern unsigned long e_libscn;
extern unsigned long e_link;
extern unsigned long e_logout;
extern unsigned long e_longname;
extern unsigned long e_mailmsg;
extern unsigned long e_minus;
extern unsigned long e_moretokens;
extern unsigned long e_multihop;
extern unsigned long e_nargs;
extern unsigned long e_newtty;
extern unsigned long e_nlspace;
extern unsigned long e_no_jctl;
extern unsigned long e_no_job;
extern unsigned long e_no_proc;
extern unsigned long e_no_start;
extern unsigned long e_notdir;
extern unsigned long e_notlvalue;
extern unsigned long e_notset;
extern unsigned long e_nullset;
extern unsigned long e_nullstr;
extern unsigned long e_number;
extern unsigned long e_off;
extern unsigned long e_oldtty;
extern unsigned long e_on;
extern unsigned long e_open;
extern unsigned long e_option;
extern unsigned long e_paren;
extern unsigned long e_pexists;
extern unsigned long e_pipe;
extern unsigned long e_profile;
extern unsigned long e_prohibited;
extern unsigned long e_pwd;
extern unsigned long e_query;
extern unsigned long e_readonly;
extern unsigned long e_real;
extern unsigned long e_recursive;
extern unsigned long e_restricted;
extern unsigned long e_running;
extern unsigned long e_runvi;
extern unsigned long e_setpwd;
extern unsigned long e_space;
extern unsigned long e_sptbnl;
extern unsigned long e_stdprompt;
extern unsigned long e_subscript;
extern unsigned long e_subst;
extern unsigned long e_suidprofile;
extern unsigned long e_supprompt;
extern unsigned long e_swap;
extern unsigned long e_synbad;
extern unsigned long e_sys;
extern unsigned long e_sysprofile;
extern unsigned long e_terminate;
extern unsigned long e_test;
extern unsigned long e_testop;
extern unsigned long e_timeout;
extern unsigned long e_timewarn;
extern unsigned long e_toobig;
extern unsigned long e_traceprompt;
extern unsigned long e_trap;
extern unsigned long e_txtbsy;
extern unsigned long e_ulimit;
extern unsigned long e_unexpected;
extern unsigned long e_unknown;
extern unsigned long e_unlimited;
extern unsigned long e_unmatched;
extern unsigned long e_user;
extern unsigned long e_version;
extern unsigned long echo_list;
extern unsigned long echoctl;
extern unsigned long ed_crlf;
extern unsigned long ed_expand;
extern unsigned long ed_external;
extern unsigned long ed_flush;
extern unsigned long ed_fulledit;
extern unsigned long ed_gencpy;
extern unsigned long ed_genlen;
extern unsigned long ed_genncpy;
extern unsigned long ed_getchar;
extern unsigned long ed_internal;
extern unsigned long ed_macro;
extern unsigned long ed_putchar;
extern unsigned long ed_ringbell;
extern unsigned long ed_setup;
extern unsigned long ed_setwidth;
extern unsigned long ed_ungetchar;
extern unsigned long ed_virt_to_phys;
extern unsigned long ed_window;
extern unsigned long editb;
extern unsigned long emacs_read;
extern unsigned long env_arrayset;
extern unsigned long env_blank;
extern unsigned long env_gen;
extern unsigned long env_get;
extern unsigned long env_init;
extern unsigned long env_namset;
extern unsigned long env_nolocal;
extern unsigned long env_prattr;
extern unsigned long env_prnamval;
extern unsigned long env_readline;
extern unsigned long env_scan;
extern unsigned long env_set;
extern unsigned long env_set_gbl;
extern unsigned long env_set_var;
extern unsigned long env_setlist;
extern unsigned long err_no;
extern unsigned long erwrite;
extern unsigned long f_complete;
extern unsigned long fdef;
extern unsigned long find_special;
extern unsigned long fsym;
extern unsigned long fsymbolic;
extern unsigned long funcload;
extern unsigned long get_shell;
extern unsigned long getaddr;
extern unsigned long getlineno;
extern unsigned long gettree;
extern unsigned long getwidth;
extern unsigned long gscan_all;
extern unsigned long gscan_some;
extern unsigned long gsort;
extern unsigned long handle_to_widget;
extern unsigned long hist_cancel;
extern unsigned long hist_close;
extern unsigned long hist_copy;
extern unsigned long hist_eof;
extern unsigned long hist_find;
extern unsigned long hist_flush;
extern unsigned long hist_fname;
extern unsigned long hist_list;
extern unsigned long hist_locate;
extern unsigned long hist_match;
extern unsigned long hist_open;
extern unsigned long hist_position;
extern unsigned long hist_ptr;
extern unsigned long hist_subst;
extern unsigned long hist_word;
extern unsigned long int_charsize;
extern unsigned long io_access;
extern unsigned long io_clear;
extern unsigned long io_fclose;
extern unsigned long io_fopen;
extern unsigned long io_ftable;
extern unsigned long io_getc;
extern unsigned long io_init;
extern unsigned long io_intr;
extern unsigned long io_linkdoc;
extern unsigned long io_mktmp;
extern unsigned long io_movefd;
extern unsigned long io_nextc;
extern unsigned long io_open;
extern unsigned long io_pclose;
extern unsigned long io_pop;
extern unsigned long io_popen;
extern unsigned long io_push;
extern unsigned long io_readbuff;
extern unsigned long io_readc;
extern unsigned long io_redirect;
extern unsigned long io_renumber;
extern unsigned long io_restore;
extern unsigned long io_save;
extern unsigned long io_seek;
extern unsigned long io_settemp;
extern unsigned long io_sopen;
extern unsigned long io_stdin;
extern unsigned long io_stdout;
extern unsigned long io_swapdoc;
extern unsigned long io_sync;
extern unsigned long io_tmpname;
extern unsigned long is_;
extern unsigned long is_alias;
extern unsigned long is_builtin;
extern unsigned long is_function;
extern unsigned long is_reserved;
extern unsigned long is_talias;
extern unsigned long is_ufunction;
extern unsigned long is_xalias;
extern unsigned long is_xfunction;
extern unsigned long isalph;
extern unsigned long isblank;
extern unsigned long ismetach;
extern unsigned long ispipe;
extern unsigned long ja_restore;
extern unsigned long job;
extern unsigned long job_bwait;
extern unsigned long job_clear;
extern unsigned long job_close;
extern unsigned long job_init;
extern unsigned long job_kill;
extern unsigned long job_list;
extern unsigned long job_post;
extern unsigned long job_switch;
extern unsigned long job_wait;
extern unsigned long job_walk;
extern unsigned long ksh_eval;
extern unsigned long limit_names;
extern unsigned long line_numbers;
extern unsigned long logdir;
extern unsigned long lsprintf;
extern unsigned long ltos;
extern unsigned long ltou;
extern unsigned long mac_check;
extern unsigned long mac_expand;
extern unsigned long mac_here;
extern unsigned long mac_trim;
extern unsigned long mac_try;
extern unsigned long main;
extern unsigned long match_paren;
extern unsigned long nam_alloc;
extern unsigned long nam_fputval;
extern unsigned long nam_free;
extern unsigned long nam_fstrval;
extern unsigned long nam_hash;
extern unsigned long nam_init;
extern unsigned long nam_link;
extern unsigned long nam_longput;
extern unsigned long nam_newtype;
extern unsigned long nam_putval;
extern unsigned long nam_rjust;
extern unsigned long nam_scope;
extern unsigned long nam_search;
extern unsigned long nam_strval;
extern unsigned long nam_unscope;
extern unsigned long name_unscope;
extern unsigned long node_names;
extern unsigned long nop;
extern unsigned long opt_arg;
extern unsigned long opt_char;
extern unsigned long opt_index;
extern unsigned long opt_option;
extern unsigned long opt_pchar;
extern unsigned long opt_pindex;
extern unsigned long optable;
extern unsigned long optget;
extern unsigned long p_char;
extern unsigned long p_flush;
extern unsigned long p_list;
extern unsigned long p_nchr;
extern unsigned long p_num;
extern unsigned long p_prp;
extern unsigned long p_setout;
extern unsigned long p_str;
extern unsigned long p_sub;
extern unsigned long p_time;
extern unsigned long path_absolute;
extern unsigned long path_alias;
extern unsigned long path_basename;
extern unsigned long path_exec;
extern unsigned long path_expand;
extern unsigned long path_get;
extern unsigned long path_join;
extern unsigned long path_open;
extern unsigned long path_physical;
extern unsigned long path_pwd;
extern unsigned long path_relative;
extern unsigned long path_search;
extern unsigned long pathcanon;
extern unsigned long printerr;
extern unsigned long printerrf;
extern unsigned long rm_files;
extern unsigned long save_alloc;
extern unsigned long scan_all;
extern unsigned long scrlen;
extern unsigned long set_flag;
extern unsigned long set_special;
extern unsigned long sh;
extern unsigned long sh_access;
extern unsigned long sh_arith;
extern unsigned long sh_cfail;
extern unsigned long sh_chktrap;
extern unsigned long sh_copy;
extern unsigned long sh_done;
extern unsigned long sh_errno;
extern unsigned long sh_eval;
extern unsigned long sh_exec;
extern unsigned long sh_exit;
extern unsigned long sh_fail;
extern unsigned long sh_fault;
extern unsigned long sh_freeup;
extern unsigned long sh_funct;
extern unsigned long sh_funstaks;
extern unsigned long sh_gettxt;
extern unsigned long sh_getwidth;
extern unsigned long sh_heap;
extern unsigned long sh_itos;
extern unsigned long sh_lastbase;
extern unsigned long sh_lex;
extern unsigned long sh_lookup;
extern unsigned long sh_mailchk;
extern unsigned long sh_mkfork;
extern unsigned long sh_parse;
extern unsigned long sh_prompt;
extern unsigned long sh_randnum;
extern unsigned long sh_seconds;
extern unsigned long sh_substitute;
extern unsigned long sh_syntax;
extern unsigned long sh_tilde;
extern unsigned long sh_timeout;
extern unsigned long sh_trace;
extern unsigned long sh_trim;
extern unsigned long sh_whence;
extern unsigned long sibuf;
extern unsigned long sig_clear;
extern unsigned long sig_funset;
extern unsigned long sig_ignore;
extern unsigned long sig_init;
extern unsigned long sig_messages;
extern unsigned long sig_names;
extern unsigned long sig_ontrap;
extern unsigned long sig_reset;
extern unsigned long sobuf;
extern unsigned long st;
extern unsigned long stakalloc;
extern unsigned long stakcopy;
extern unsigned long stakcreate;
extern unsigned long stakdelete;
extern unsigned long stakfreeze;
extern unsigned long stakinstall;
extern unsigned long stakputs;
extern unsigned long stakseek;
extern unsigned long stakset;
extern unsigned long streval;
extern unsigned long strfree;
extern unsigned long strmatch;
extern unsigned long strparse;
extern unsigned long strperm;
extern unsigned long strprint;
extern unsigned long strtoul;
extern unsigned long submatch;
extern unsigned long symcomp;
extern unsigned long tab_attributes;
extern unsigned long tab_options;
extern unsigned long tab_reserved;
extern unsigned long tcgetattr;
extern unsigned long tcsetattr;
extern unsigned long test_binop;
extern unsigned long test_inode;
extern unsigned long test_mode;
extern unsigned long test_optable;
extern unsigned long test_unops;
extern unsigned long tracked_names;
extern unsigned long tty_alt;
extern unsigned long tty_check;
extern unsigned long tty_cooked;
extern unsigned long tty_get;
extern unsigned long tty_raw;
extern unsigned long tty_set;
extern unsigned long unop_test;
extern unsigned long utol;
extern unsigned long utos;
extern unsigned long varload;
extern unsigned long vi_read;
extern unsigned long wk_libinit;
extern unsigned long xk_free;
extern unsigned long xk_get_delim;
extern unsigned long xk_get_pardelim;
extern unsigned long xk_parse;
extern unsigned long xk_prdebug;
extern unsigned long xk_print;
extern unsigned long xk_ret_buf;
extern unsigned long xk_ret_buffer;
extern unsigned long xk_usage;
extern unsigned long xkhash_add;
extern unsigned long xkhash_find;
extern unsigned long xkhash_init;
extern unsigned long xkhash_override;

struct symarray Symarray[] = {
	{ "All_libs", (unsigned long) &All_libs },
	{ "Deflist", (unsigned long) &Deflist },
	{ "Ndeflist", (unsigned long) &Ndeflist },
	{ "Nstructlist", (unsigned long) &Nstructlist },
	{ "Nsymlist", (unsigned long) &Nsymlist },
	{ "Pr_tmpnonames", (unsigned long) &Pr_tmpnonames },
	{ "Prfailpoint", (unsigned long) &Prfailpoint },
	{ "Promptfunc", (unsigned long) &Promptfunc },
	{ "STR_string_t", (unsigned long) &STR_string_t },
	{ "STR_uint", (unsigned long) &STR_uint },
	{ "STR_ulong", (unsigned long) &STR_ulong },
	{ "Structlist", (unsigned long) &Structlist },
	{ "Symlist", (unsigned long) &Symlist },
	{ "T_char", (unsigned long) &T_char },
	{ "T_dint", (unsigned long) &T_dint },
	{ "T_dlong", (unsigned long) &T_dlong },
	{ "T_dshort", (unsigned long) &T_dshort },
	{ "T_int", (unsigned long) &T_int },
	{ "T_intp", (unsigned long) &T_intp },
	{ "T_long", (unsigned long) &T_long },
	{ "T_longp", (unsigned long) &T_longp },
	{ "T_short", (unsigned long) &T_short },
	{ "T_string_t", (unsigned long) &T_string_t },
	{ "T_uint", (unsigned long) &T_uint },
	{ "T_ulong", (unsigned long) &T_ulong },
	{ "T_unchar", (unsigned long) &T_unchar },
	{ "T_ushort", (unsigned long) &T_ushort },
	{ "Toplevel", (unsigned long) &Toplevel },
	{ "Xk_errno", (unsigned long) &Xk_errno },
	{ "Xkdebug", (unsigned long) &Xkdebug },
	{ "_Prdebug", (unsigned long) &_Prdebug },
	{ "_ctype1", (unsigned long) &_ctype1 },
	{ "_ctype2", (unsigned long) &_ctype2 },
	{ "_stak_cur", (unsigned long) &_stak_cur },
	{ "_stakgrow", (unsigned long) &_stakgrow },
	{ "add_structlist", (unsigned long) &add_structlist },
	{ "add_symbolic", (unsigned long) &add_symbolic },
	{ "alias_names", (unsigned long) &alias_names },
	{ "aliasload", (unsigned long) &aliasload },
	{ "all_tbl_find", (unsigned long) &all_tbl_find },
	{ "all_tbl_search", (unsigned long) &all_tbl_search },
	{ "altflush", (unsigned long) &altflush },
	{ "altfprintf", (unsigned long) &altfprintf },
	{ "altfputs", (unsigned long) &altfputs },
	{ "altgets", (unsigned long) &altgets },
	{ "altperror", (unsigned long) &altperror },
	{ "altprintf", (unsigned long) &altprintf },
	{ "altprompt", (unsigned long) &altprompt },
	{ "altputs", (unsigned long) &altputs },
	{ "arg_build", (unsigned long) &arg_build },
	{ "arg_clear", (unsigned long) &arg_clear },
	{ "arg_dolminus", (unsigned long) &arg_dolminus },
	{ "arg_free", (unsigned long) &arg_free },
	{ "arg_new", (unsigned long) &arg_new },
	{ "arg_opts", (unsigned long) &arg_opts },
	{ "arg_reset", (unsigned long) &arg_reset },
	{ "arg_set", (unsigned long) &arg_set },
	{ "arg_use", (unsigned long) &arg_use },
	{ "array_dotset", (unsigned long) &array_dotset },
	{ "array_find", (unsigned long) &array_find },
	{ "array_grow", (unsigned long) &array_grow },
	{ "array_next", (unsigned long) &array_next },
	{ "array_subscript", (unsigned long) &array_subscript },
	{ "asl_find", (unsigned long) &asl_find },
	{ "asl_set", (unsigned long) &asl_set },
	{ "b_alias", (unsigned long) &b_alias },
	{ "b_bgfg", (unsigned long) &b_bgfg },
	{ "b_break", (unsigned long) &b_break },
	{ "b_chdir", (unsigned long) &b_chdir },
	{ "b_continue", (unsigned long) &b_continue },
	{ "b_dot", (unsigned long) &b_dot },
	{ "b_echo", (unsigned long) &b_echo },
	{ "b_eval", (unsigned long) &b_eval },
	{ "b_exec", (unsigned long) &b_exec },
	{ "b_export", (unsigned long) &b_export },
	{ "b_fc", (unsigned long) &b_fc },
	{ "b_getopts", (unsigned long) &b_getopts },
	{ "b_jobs", (unsigned long) &b_jobs },
	{ "b_kill", (unsigned long) &b_kill },
	{ "b_let", (unsigned long) &b_let },
	{ "b_login", (unsigned long) &b_login },
	{ "b_null", (unsigned long) &b_null },
	{ "b_print", (unsigned long) &b_print },
	{ "b_pwd", (unsigned long) &b_pwd },
	{ "b_read", (unsigned long) &b_read },
	{ "b_readonly", (unsigned long) &b_readonly },
	{ "b_ret_exit", (unsigned long) &b_ret_exit },
	{ "b_set", (unsigned long) &b_set },
	{ "b_shift", (unsigned long) &b_shift },
	{ "b_test", (unsigned long) &b_test },
	{ "b_times", (unsigned long) &b_times },
	{ "b_trap", (unsigned long) &b_trap },
	{ "b_typeset", (unsigned long) &b_typeset },
	{ "b_ulimit", (unsigned long) &b_ulimit },
	{ "b_umask", (unsigned long) &b_umask },
	{ "b_unalias", (unsigned long) &b_unalias },
	{ "b_unset", (unsigned long) &b_unset },
	{ "b_wait", (unsigned long) &b_wait },
	{ "b_whence", (unsigned long) &b_whence },
	{ "basedefs", (unsigned long) &basedefs },
	{ "basemems", (unsigned long) &basemems },
	{ "built_ins", (unsigned long) &built_ins },
	{ "call_init", (unsigned long) &call_init },
	{ "close_pipes", (unsigned long) &close_pipes },
	{ "do_XBell", (unsigned long) &do_XBell },
	{ "do_XDraw", (unsigned long) &do_XDraw },
	{ "do_XFlush", (unsigned long) &do_XFlush },
	{ "do_XmCreateArrowButton", (unsigned long) &do_XmCreateArrowButton },
	{ "do_XmCreateArrowButtonGadget", (unsigned long) &do_XmCreateArrowButtonGadget },
	{ "do_XmCreateBulletinBoard", (unsigned long) &do_XmCreateBulletinBoard },
	{ "do_XmCreateBulletinBoardDialog", (unsigned long) &do_XmCreateBulletinBoardDialog },
	{ "do_XmCreateCascadeButton", (unsigned long) &do_XmCreateCascadeButton },
	{ "do_XmCreateCascadeButtonGadget", (unsigned long) &do_XmCreateCascadeButtonGadget },
	{ "do_XmCreateCommand", (unsigned long) &do_XmCreateCommand },
	{ "do_XmCreateDialogShell", (unsigned long) &do_XmCreateDialogShell },
	{ "do_XmCreateDrawingArea", (unsigned long) &do_XmCreateDrawingArea },
	{ "do_XmCreateDrawnButton", (unsigned long) &do_XmCreateDrawnButton },
	{ "do_XmCreateErrorDialog", (unsigned long) &do_XmCreateErrorDialog },
	{ "do_XmCreateFileSelectionBox", (unsigned long) &do_XmCreateFileSelectionBox },
	{ "do_XmCreateFileSelectionDialog", (unsigned long) &do_XmCreateFileSelectionDialog },
	{ "do_XmCreateForm", (unsigned long) &do_XmCreateForm },
	{ "do_XmCreateFormDialog", (unsigned long) &do_XmCreateFormDialog },
	{ "do_XmCreateFrame", (unsigned long) &do_XmCreateFrame },
	{ "do_XmCreateInformationDialog", (unsigned long) &do_XmCreateInformationDialog },
	{ "do_XmCreateLabel", (unsigned long) &do_XmCreateLabel },
	{ "do_XmCreateLabelGadget", (unsigned long) &do_XmCreateLabelGadget },
	{ "do_XmCreateList", (unsigned long) &do_XmCreateList },
	{ "do_XmCreateMainWindow", (unsigned long) &do_XmCreateMainWindow },
	{ "do_XmCreateMenuBar", (unsigned long) &do_XmCreateMenuBar },
	{ "do_XmCreateMenuShell", (unsigned long) &do_XmCreateMenuShell },
	{ "do_XmCreateMessageBox", (unsigned long) &do_XmCreateMessageBox },
	{ "do_XmCreateMessageDialog", (unsigned long) &do_XmCreateMessageDialog },
	{ "do_XmCreateOptionMenu", (unsigned long) &do_XmCreateOptionMenu },
	{ "do_XmCreatePanedWindow", (unsigned long) &do_XmCreatePanedWindow },
	{ "do_XmCreatePopupMenu", (unsigned long) &do_XmCreatePopupMenu },
	{ "do_XmCreatePromptDialog", (unsigned long) &do_XmCreatePromptDialog },
	{ "do_XmCreatePulldownMenu", (unsigned long) &do_XmCreatePulldownMenu },
	{ "do_XmCreatePushButton", (unsigned long) &do_XmCreatePushButton },
	{ "do_XmCreatePushButtonGadget", (unsigned long) &do_XmCreatePushButtonGadget },
	{ "do_XmCreateQuestionDialog", (unsigned long) &do_XmCreateQuestionDialog },
	{ "do_XmCreateRadioBox", (unsigned long) &do_XmCreateRadioBox },
	{ "do_XmCreateRowColumn", (unsigned long) &do_XmCreateRowColumn },
	{ "do_XmCreateScale", (unsigned long) &do_XmCreateScale },
	{ "do_XmCreateScrollBar", (unsigned long) &do_XmCreateScrollBar },
	{ "do_XmCreateScrolledList", (unsigned long) &do_XmCreateScrolledList },
	{ "do_XmCreateScrolledText", (unsigned long) &do_XmCreateScrolledText },
	{ "do_XmCreateScrolledWindow", (unsigned long) &do_XmCreateScrolledWindow },
	{ "do_XmCreateSelectionBox", (unsigned long) &do_XmCreateSelectionBox },
	{ "do_XmCreateSelectionDialog", (unsigned long) &do_XmCreateSelectionDialog },
	{ "do_XmCreateSeparator", (unsigned long) &do_XmCreateSeparator },
	{ "do_XmCreateSeparatorGadget", (unsigned long) &do_XmCreateSeparatorGadget },
	{ "do_XmCreateText", (unsigned long) &do_XmCreateText },
	{ "do_XmCreateToggleButton", (unsigned long) &do_XmCreateToggleButton },
	{ "do_XmCreateToggleButtonGadget", (unsigned long) &do_XmCreateToggleButtonGadget },
	{ "do_XmCreateWarningDialog", (unsigned long) &do_XmCreateWarningDialog },
	{ "do_XmCreateWorkingDialog", (unsigned long) &do_XmCreateWorkingDialog },
	{ "do_XtAddCallback", (unsigned long) &do_XtAddCallback },
	{ "do_XtAddInput", (unsigned long) &do_XtAddInput },
	{ "do_XtAddTimeOut", (unsigned long) &do_XtAddTimeOut },
	{ "do_XtAppCreateShell", (unsigned long) &do_XtAppCreateShell },
	{ "do_XtAppInitialize", (unsigned long) &do_XtAppInitialize },
	{ "do_XtCallCallbacks", (unsigned long) &do_XtCallCallbacks },
	{ "do_XtCreateManagedWidget", (unsigned long) &do_XtCreateManagedWidget },
	{ "do_XtCreatePopupShell", (unsigned long) &do_XtCreatePopupShell },
	{ "do_XtCreateWidget", (unsigned long) &do_XtCreateWidget },
	{ "do_XtDestroyWidget", (unsigned long) &do_XtDestroyWidget },
	{ "do_XtGetValues", (unsigned long) &do_XtGetValues },
	{ "do_XtIsManaged", (unsigned long) &do_XtIsManaged },
	{ "do_XtIsRealized", (unsigned long) &do_XtIsRealized },
	{ "do_XtIsSensitive", (unsigned long) &do_XtIsSensitive },
	{ "do_XtMainLoop", (unsigned long) &do_XtMainLoop },
	{ "do_XtManageChildren", (unsigned long) &do_XtManageChildren },
	{ "do_XtMapWidget", (unsigned long) &do_XtMapWidget },
	{ "do_XtPopdown", (unsigned long) &do_XtPopdown },
	{ "do_XtPopup", (unsigned long) &do_XtPopup },
	{ "do_XtRealizeWidget", (unsigned long) &do_XtRealizeWidget },
	{ "do_XtRemoveAllCallbacks", (unsigned long) &do_XtRemoveAllCallbacks },
	{ "do_XtSetSensitive", (unsigned long) &do_XtSetSensitive },
	{ "do_XtSetValues", (unsigned long) &do_XtSetValues },
	{ "do_XtUnmanageChildren", (unsigned long) &do_XtUnmanageChildren },
	{ "do_XtUnmapWidget", (unsigned long) &do_XtUnmapWidget },
	{ "do_XtUnrealizeWidget", (unsigned long) &do_XtUnrealizeWidget },
	{ "do_call", (unsigned long) &do_call },
	{ "do_cmdload", (unsigned long) &do_cmdload },
	{ "do_define", (unsigned long) &do_define },
	{ "do_deflist", (unsigned long) &do_deflist },
	{ "do_deref", (unsigned long) &do_deref },
	{ "do_field_comp", (unsigned long) &do_field_comp },
	{ "do_field_get", (unsigned long) &do_field_get },
	{ "do_finddef", (unsigned long) &do_finddef },
	{ "do_findsym", (unsigned long) &do_findsym },
	{ "do_init", (unsigned long) &do_init },
	{ "do_libinit", (unsigned long) &do_libinit },
	{ "do_libload", (unsigned long) &do_libload },
	{ "do_managelist_func", (unsigned long) &do_managelist_func },
	{ "do_sizeof", (unsigned long) &do_sizeof },
	{ "do_struct", (unsigned long) &do_struct },
	{ "do_structlist", (unsigned long) &do_structlist },
	{ "do_symbolic", (unsigned long) &do_symbolic },
	{ "do_typedef", (unsigned long) &do_typedef },
	{ "do_widlist", (unsigned long) &do_widlist },
	{ "do_widload", (unsigned long) &do_widload },
	{ "e_Done", (unsigned long) &e_Done },
	{ "e_Running", (unsigned long) &e_Running },
	{ "e_access", (unsigned long) &e_access },
	{ "e_alias", (unsigned long) &e_alias },
	{ "e_aliname", (unsigned long) &e_aliname },
	{ "e_ambiguous", (unsigned long) &e_ambiguous },
	{ "e_argexp", (unsigned long) &e_argexp },
	{ "e_arglist", (unsigned long) &e_arglist },
	{ "e_atline", (unsigned long) &e_atline },
	{ "e_badcolon", (unsigned long) &e_badcolon },
	{ "e_bltfn", (unsigned long) &e_bltfn },
	{ "e_bracket", (unsigned long) &e_bracket },
	{ "e_colon", (unsigned long) &e_colon },
	{ "e_coredump", (unsigned long) &e_coredump },
	{ "e_create", (unsigned long) &e_create },
	{ "e_crondir", (unsigned long) &e_crondir },
	{ "e_defedit", (unsigned long) &e_defedit },
	{ "e_defpath", (unsigned long) &e_defpath },
	{ "e_devfd", (unsigned long) &e_devfd },
	{ "e_devfdNN", (unsigned long) &e_devfdNN },
	{ "e_devnull", (unsigned long) &e_devnull },
	{ "e_direct", (unsigned long) &e_direct },
	{ "e_divzero", (unsigned long) &e_divzero },
	{ "e_dot", (unsigned long) &e_dot },
	{ "e_echobin", (unsigned long) &e_echobin },
	{ "e_echoflag", (unsigned long) &e_echoflag },
	{ "e_endoffile", (unsigned long) &e_endoffile },
	{ "e_envmarker", (unsigned long) &e_envmarker },
	{ "e_exec", (unsigned long) &e_exec },
	{ "e_fexists", (unsigned long) &e_fexists },
	{ "e_file", (unsigned long) &e_file },
	{ "e_flimit", (unsigned long) &e_flimit },
	{ "e_fnhdr", (unsigned long) &e_fnhdr },
	{ "e_fork", (unsigned long) &e_fork },
	{ "e_format", (unsigned long) &e_format },
	{ "e_found", (unsigned long) &e_found },
	{ "e_function", (unsigned long) &e_function },
	{ "e_hdigits", (unsigned long) &e_hdigits },
	{ "e_heading", (unsigned long) &e_heading },
	{ "e_history", (unsigned long) &e_history },
	{ "e_ident", (unsigned long) &e_ident },
	{ "e_intbase", (unsigned long) &e_intbase },
	{ "e_jobusage", (unsigned long) &e_jobusage },
	{ "e_kill", (unsigned long) &e_kill },
	{ "e_killcolon", (unsigned long) &e_killcolon },
	{ "e_libacc", (unsigned long) &e_libacc },
	{ "e_libbad", (unsigned long) &e_libbad },
	{ "e_libmax", (unsigned long) &e_libmax },
	{ "e_libscn", (unsigned long) &e_libscn },
	{ "e_link", (unsigned long) &e_link },
	{ "e_logout", (unsigned long) &e_logout },
	{ "e_longname", (unsigned long) &e_longname },
	{ "e_mailmsg", (unsigned long) &e_mailmsg },
	{ "e_minus", (unsigned long) &e_minus },
	{ "e_moretokens", (unsigned long) &e_moretokens },
	{ "e_multihop", (unsigned long) &e_multihop },
	{ "e_nargs", (unsigned long) &e_nargs },
	{ "e_newtty", (unsigned long) &e_newtty },
	{ "e_nlspace", (unsigned long) &e_nlspace },
	{ "e_no_jctl", (unsigned long) &e_no_jctl },
	{ "e_no_job", (unsigned long) &e_no_job },
	{ "e_no_proc", (unsigned long) &e_no_proc },
	{ "e_no_start", (unsigned long) &e_no_start },
	{ "e_notdir", (unsigned long) &e_notdir },
	{ "e_notlvalue", (unsigned long) &e_notlvalue },
	{ "e_notset", (unsigned long) &e_notset },
	{ "e_nullset", (unsigned long) &e_nullset },
	{ "e_nullstr", (unsigned long) &e_nullstr },
	{ "e_number", (unsigned long) &e_number },
	{ "e_off", (unsigned long) &e_off },
	{ "e_oldtty", (unsigned long) &e_oldtty },
	{ "e_on", (unsigned long) &e_on },
	{ "e_open", (unsigned long) &e_open },
	{ "e_option", (unsigned long) &e_option },
	{ "e_paren", (unsigned long) &e_paren },
	{ "e_pexists", (unsigned long) &e_pexists },
	{ "e_pipe", (unsigned long) &e_pipe },
	{ "e_profile", (unsigned long) &e_profile },
	{ "e_prohibited", (unsigned long) &e_prohibited },
	{ "e_pwd", (unsigned long) &e_pwd },
	{ "e_query", (unsigned long) &e_query },
	{ "e_readonly", (unsigned long) &e_readonly },
	{ "e_real", (unsigned long) &e_real },
	{ "e_recursive", (unsigned long) &e_recursive },
	{ "e_restricted", (unsigned long) &e_restricted },
	{ "e_running", (unsigned long) &e_running },
	{ "e_runvi", (unsigned long) &e_runvi },
	{ "e_setpwd", (unsigned long) &e_setpwd },
	{ "e_space", (unsigned long) &e_space },
	{ "e_sptbnl", (unsigned long) &e_sptbnl },
	{ "e_stdprompt", (unsigned long) &e_stdprompt },
	{ "e_subscript", (unsigned long) &e_subscript },
	{ "e_subst", (unsigned long) &e_subst },
	{ "e_suidprofile", (unsigned long) &e_suidprofile },
	{ "e_supprompt", (unsigned long) &e_supprompt },
	{ "e_swap", (unsigned long) &e_swap },
	{ "e_synbad", (unsigned long) &e_synbad },
	{ "e_sys", (unsigned long) &e_sys },
	{ "e_sysprofile", (unsigned long) &e_sysprofile },
	{ "e_terminate", (unsigned long) &e_terminate },
	{ "e_test", (unsigned long) &e_test },
	{ "e_testop", (unsigned long) &e_testop },
	{ "e_timeout", (unsigned long) &e_timeout },
	{ "e_timewarn", (unsigned long) &e_timewarn },
	{ "e_toobig", (unsigned long) &e_toobig },
	{ "e_traceprompt", (unsigned long) &e_traceprompt },
	{ "e_trap", (unsigned long) &e_trap },
	{ "e_txtbsy", (unsigned long) &e_txtbsy },
	{ "e_ulimit", (unsigned long) &e_ulimit },
	{ "e_unexpected", (unsigned long) &e_unexpected },
	{ "e_unknown", (unsigned long) &e_unknown },
	{ "e_unlimited", (unsigned long) &e_unlimited },
	{ "e_unmatched", (unsigned long) &e_unmatched },
	{ "e_user", (unsigned long) &e_user },
	{ "e_version", (unsigned long) &e_version },
	{ "echo_list", (unsigned long) &echo_list },
	{ "echoctl", (unsigned long) &echoctl },
	{ "ed_crlf", (unsigned long) &ed_crlf },
	{ "ed_expand", (unsigned long) &ed_expand },
	{ "ed_external", (unsigned long) &ed_external },
	{ "ed_flush", (unsigned long) &ed_flush },
	{ "ed_fulledit", (unsigned long) &ed_fulledit },
	{ "ed_gencpy", (unsigned long) &ed_gencpy },
	{ "ed_genlen", (unsigned long) &ed_genlen },
	{ "ed_genncpy", (unsigned long) &ed_genncpy },
	{ "ed_getchar", (unsigned long) &ed_getchar },
	{ "ed_internal", (unsigned long) &ed_internal },
	{ "ed_macro", (unsigned long) &ed_macro },
	{ "ed_putchar", (unsigned long) &ed_putchar },
	{ "ed_ringbell", (unsigned long) &ed_ringbell },
	{ "ed_setup", (unsigned long) &ed_setup },
	{ "ed_setwidth", (unsigned long) &ed_setwidth },
	{ "ed_ungetchar", (unsigned long) &ed_ungetchar },
	{ "ed_virt_to_phys", (unsigned long) &ed_virt_to_phys },
	{ "ed_window", (unsigned long) &ed_window },
	{ "editb", (unsigned long) &editb },
	{ "emacs_read", (unsigned long) &emacs_read },
	{ "env_arrayset", (unsigned long) &env_arrayset },
	{ "env_blank", (unsigned long) &env_blank },
	{ "env_gen", (unsigned long) &env_gen },
	{ "env_get", (unsigned long) &env_get },
	{ "env_init", (unsigned long) &env_init },
	{ "env_namset", (unsigned long) &env_namset },
	{ "env_nolocal", (unsigned long) &env_nolocal },
	{ "env_prattr", (unsigned long) &env_prattr },
	{ "env_prnamval", (unsigned long) &env_prnamval },
	{ "env_readline", (unsigned long) &env_readline },
	{ "env_scan", (unsigned long) &env_scan },
	{ "env_set", (unsigned long) &env_set },
	{ "env_set_gbl", (unsigned long) &env_set_gbl },
	{ "env_set_var", (unsigned long) &env_set_var },
	{ "env_setlist", (unsigned long) &env_setlist },
	{ "err_no", (unsigned long) &err_no },
	{ "erwrite", (unsigned long) &erwrite },
	{ "f_complete", (unsigned long) &f_complete },
	{ "fdef", (unsigned long) &fdef },
	{ "find_special", (unsigned long) &find_special },
	{ "fsym", (unsigned long) &fsym },
	{ "fsymbolic", (unsigned long) &fsymbolic },
	{ "funcload", (unsigned long) &funcload },
	{ "get_shell", (unsigned long) &get_shell },
	{ "getaddr", (unsigned long) &getaddr },
	{ "getlineno", (unsigned long) &getlineno },
	{ "gettree", (unsigned long) &gettree },
	{ "getwidth", (unsigned long) &getwidth },
	{ "gscan_all", (unsigned long) &gscan_all },
	{ "gscan_some", (unsigned long) &gscan_some },
	{ "gsort", (unsigned long) &gsort },
	{ "handle_to_widget", (unsigned long) &handle_to_widget },
	{ "hist_cancel", (unsigned long) &hist_cancel },
	{ "hist_close", (unsigned long) &hist_close },
	{ "hist_copy", (unsigned long) &hist_copy },
	{ "hist_eof", (unsigned long) &hist_eof },
	{ "hist_find", (unsigned long) &hist_find },
	{ "hist_flush", (unsigned long) &hist_flush },
	{ "hist_fname", (unsigned long) &hist_fname },
	{ "hist_list", (unsigned long) &hist_list },
	{ "hist_locate", (unsigned long) &hist_locate },
	{ "hist_match", (unsigned long) &hist_match },
	{ "hist_open", (unsigned long) &hist_open },
	{ "hist_position", (unsigned long) &hist_position },
	{ "hist_ptr", (unsigned long) &hist_ptr },
	{ "hist_subst", (unsigned long) &hist_subst },
	{ "hist_word", (unsigned long) &hist_word },
	{ "int_charsize", (unsigned long) &int_charsize },
	{ "io_access", (unsigned long) &io_access },
	{ "io_clear", (unsigned long) &io_clear },
	{ "io_fclose", (unsigned long) &io_fclose },
	{ "io_fopen", (unsigned long) &io_fopen },
	{ "io_ftable", (unsigned long) &io_ftable },
	{ "io_getc", (unsigned long) &io_getc },
	{ "io_init", (unsigned long) &io_init },
	{ "io_intr", (unsigned long) &io_intr },
	{ "io_linkdoc", (unsigned long) &io_linkdoc },
	{ "io_mktmp", (unsigned long) &io_mktmp },
	{ "io_movefd", (unsigned long) &io_movefd },
	{ "io_nextc", (unsigned long) &io_nextc },
	{ "io_open", (unsigned long) &io_open },
	{ "io_pclose", (unsigned long) &io_pclose },
	{ "io_pop", (unsigned long) &io_pop },
	{ "io_popen", (unsigned long) &io_popen },
	{ "io_push", (unsigned long) &io_push },
	{ "io_readbuff", (unsigned long) &io_readbuff },
	{ "io_readc", (unsigned long) &io_readc },
	{ "io_redirect", (unsigned long) &io_redirect },
	{ "io_renumber", (unsigned long) &io_renumber },
	{ "io_restore", (unsigned long) &io_restore },
	{ "io_save", (unsigned long) &io_save },
	{ "io_seek", (unsigned long) &io_seek },
	{ "io_settemp", (unsigned long) &io_settemp },
	{ "io_sopen", (unsigned long) &io_sopen },
	{ "io_stdin", (unsigned long) &io_stdin },
	{ "io_stdout", (unsigned long) &io_stdout },
	{ "io_swapdoc", (unsigned long) &io_swapdoc },
	{ "io_sync", (unsigned long) &io_sync },
	{ "io_tmpname", (unsigned long) &io_tmpname },
	{ "is_", (unsigned long) &is_ },
	{ "is_alias", (unsigned long) &is_alias },
	{ "is_builtin", (unsigned long) &is_builtin },
	{ "is_function", (unsigned long) &is_function },
	{ "is_reserved", (unsigned long) &is_reserved },
	{ "is_talias", (unsigned long) &is_talias },
	{ "is_ufunction", (unsigned long) &is_ufunction },
	{ "is_xalias", (unsigned long) &is_xalias },
	{ "is_xfunction", (unsigned long) &is_xfunction },
	{ "isalph", (unsigned long) &isalph },
	{ "isblank", (unsigned long) &isblank },
	{ "ismetach", (unsigned long) &ismetach },
	{ "ispipe", (unsigned long) &ispipe },
	{ "ja_restore", (unsigned long) &ja_restore },
	{ "job", (unsigned long) &job },
	{ "job_bwait", (unsigned long) &job_bwait },
	{ "job_clear", (unsigned long) &job_clear },
	{ "job_close", (unsigned long) &job_close },
	{ "job_init", (unsigned long) &job_init },
	{ "job_kill", (unsigned long) &job_kill },
	{ "job_list", (unsigned long) &job_list },
	{ "job_post", (unsigned long) &job_post },
	{ "job_switch", (unsigned long) &job_switch },
	{ "job_wait", (unsigned long) &job_wait },
	{ "job_walk", (unsigned long) &job_walk },
	{ "ksh_eval", (unsigned long) &ksh_eval },
	{ "limit_names", (unsigned long) &limit_names },
	{ "line_numbers", (unsigned long) &line_numbers },
	{ "logdir", (unsigned long) &logdir },
	{ "lsprintf", (unsigned long) &lsprintf },
	{ "ltos", (unsigned long) &ltos },
	{ "ltou", (unsigned long) &ltou },
	{ "mac_check", (unsigned long) &mac_check },
	{ "mac_expand", (unsigned long) &mac_expand },
	{ "mac_here", (unsigned long) &mac_here },
	{ "mac_trim", (unsigned long) &mac_trim },
	{ "mac_try", (unsigned long) &mac_try },
	{ "main", (unsigned long) &main },
	{ "match_paren", (unsigned long) &match_paren },
	{ "nam_alloc", (unsigned long) &nam_alloc },
	{ "nam_fputval", (unsigned long) &nam_fputval },
	{ "nam_free", (unsigned long) &nam_free },
	{ "nam_fstrval", (unsigned long) &nam_fstrval },
	{ "nam_hash", (unsigned long) &nam_hash },
	{ "nam_init", (unsigned long) &nam_init },
	{ "nam_link", (unsigned long) &nam_link },
	{ "nam_longput", (unsigned long) &nam_longput },
	{ "nam_newtype", (unsigned long) &nam_newtype },
	{ "nam_putval", (unsigned long) &nam_putval },
	{ "nam_rjust", (unsigned long) &nam_rjust },
	{ "nam_scope", (unsigned long) &nam_scope },
	{ "nam_search", (unsigned long) &nam_search },
	{ "nam_strval", (unsigned long) &nam_strval },
	{ "nam_unscope", (unsigned long) &nam_unscope },
	{ "name_unscope", (unsigned long) &name_unscope },
	{ "node_names", (unsigned long) &node_names },
	{ "nop", (unsigned long) &nop },
	{ "opt_arg", (unsigned long) &opt_arg },
	{ "opt_char", (unsigned long) &opt_char },
	{ "opt_index", (unsigned long) &opt_index },
	{ "opt_option", (unsigned long) &opt_option },
	{ "opt_pchar", (unsigned long) &opt_pchar },
	{ "opt_pindex", (unsigned long) &opt_pindex },
	{ "optable", (unsigned long) &optable },
	{ "optget", (unsigned long) &optget },
	{ "p_char", (unsigned long) &p_char },
	{ "p_flush", (unsigned long) &p_flush },
	{ "p_list", (unsigned long) &p_list },
	{ "p_nchr", (unsigned long) &p_nchr },
	{ "p_num", (unsigned long) &p_num },
	{ "p_prp", (unsigned long) &p_prp },
	{ "p_setout", (unsigned long) &p_setout },
	{ "p_str", (unsigned long) &p_str },
	{ "p_sub", (unsigned long) &p_sub },
	{ "p_time", (unsigned long) &p_time },
	{ "path_absolute", (unsigned long) &path_absolute },
	{ "path_alias", (unsigned long) &path_alias },
	{ "path_basename", (unsigned long) &path_basename },
	{ "path_exec", (unsigned long) &path_exec },
	{ "path_expand", (unsigned long) &path_expand },
	{ "path_get", (unsigned long) &path_get },
	{ "path_join", (unsigned long) &path_join },
	{ "path_open", (unsigned long) &path_open },
	{ "path_physical", (unsigned long) &path_physical },
	{ "path_pwd", (unsigned long) &path_pwd },
	{ "path_relative", (unsigned long) &path_relative },
	{ "path_search", (unsigned long) &path_search },
	{ "pathcanon", (unsigned long) &pathcanon },
	{ "printerr", (unsigned long) &printerr },
	{ "printerrf", (unsigned long) &printerrf },
	{ "rm_files", (unsigned long) &rm_files },
	{ "save_alloc", (unsigned long) &save_alloc },
	{ "scan_all", (unsigned long) &scan_all },
	{ "scrlen", (unsigned long) &scrlen },
	{ "set_flag", (unsigned long) &set_flag },
	{ "set_special", (unsigned long) &set_special },
	{ "sh", (unsigned long) &sh },
	{ "sh_access", (unsigned long) &sh_access },
	{ "sh_arith", (unsigned long) &sh_arith },
	{ "sh_cfail", (unsigned long) &sh_cfail },
	{ "sh_chktrap", (unsigned long) &sh_chktrap },
	{ "sh_copy", (unsigned long) &sh_copy },
	{ "sh_done", (unsigned long) &sh_done },
	{ "sh_errno", (unsigned long) &sh_errno },
	{ "sh_eval", (unsigned long) &sh_eval },
	{ "sh_exec", (unsigned long) &sh_exec },
	{ "sh_exit", (unsigned long) &sh_exit },
	{ "sh_fail", (unsigned long) &sh_fail },
	{ "sh_fault", (unsigned long) &sh_fault },
	{ "sh_freeup", (unsigned long) &sh_freeup },
	{ "sh_funct", (unsigned long) &sh_funct },
	{ "sh_funstaks", (unsigned long) &sh_funstaks },
	{ "sh_gettxt", (unsigned long) &sh_gettxt },
	{ "sh_getwidth", (unsigned long) &sh_getwidth },
	{ "sh_heap", (unsigned long) &sh_heap },
	{ "sh_itos", (unsigned long) &sh_itos },
	{ "sh_lastbase", (unsigned long) &sh_lastbase },
	{ "sh_lex", (unsigned long) &sh_lex },
	{ "sh_lookup", (unsigned long) &sh_lookup },
	{ "sh_mailchk", (unsigned long) &sh_mailchk },
	{ "sh_mkfork", (unsigned long) &sh_mkfork },
	{ "sh_parse", (unsigned long) &sh_parse },
	{ "sh_prompt", (unsigned long) &sh_prompt },
	{ "sh_randnum", (unsigned long) &sh_randnum },
	{ "sh_seconds", (unsigned long) &sh_seconds },
	{ "sh_substitute", (unsigned long) &sh_substitute },
	{ "sh_syntax", (unsigned long) &sh_syntax },
	{ "sh_tilde", (unsigned long) &sh_tilde },
	{ "sh_timeout", (unsigned long) &sh_timeout },
	{ "sh_trace", (unsigned long) &sh_trace },
	{ "sh_trim", (unsigned long) &sh_trim },
	{ "sh_whence", (unsigned long) &sh_whence },
	{ "sibuf", (unsigned long) &sibuf },
	{ "sig_clear", (unsigned long) &sig_clear },
	{ "sig_funset", (unsigned long) &sig_funset },
	{ "sig_ignore", (unsigned long) &sig_ignore },
	{ "sig_init", (unsigned long) &sig_init },
	{ "sig_messages", (unsigned long) &sig_messages },
	{ "sig_names", (unsigned long) &sig_names },
	{ "sig_ontrap", (unsigned long) &sig_ontrap },
	{ "sig_reset", (unsigned long) &sig_reset },
	{ "sobuf", (unsigned long) &sobuf },
	{ "st", (unsigned long) &st },
	{ "stakalloc", (unsigned long) &stakalloc },
	{ "stakcopy", (unsigned long) &stakcopy },
	{ "stakcreate", (unsigned long) &stakcreate },
	{ "stakdelete", (unsigned long) &stakdelete },
	{ "stakfreeze", (unsigned long) &stakfreeze },
	{ "stakinstall", (unsigned long) &stakinstall },
	{ "stakputs", (unsigned long) &stakputs },
	{ "stakseek", (unsigned long) &stakseek },
	{ "stakset", (unsigned long) &stakset },
	{ "streval", (unsigned long) &streval },
	{ "strfree", (unsigned long) &strfree },
	{ "strmatch", (unsigned long) &strmatch },
	{ "strparse", (unsigned long) &strparse },
	{ "strperm", (unsigned long) &strperm },
	{ "strprint", (unsigned long) &strprint },
	{ "strtoul", (unsigned long) &strtoul },
	{ "submatch", (unsigned long) &submatch },
	{ "symcomp", (unsigned long) &symcomp },
	{ "tab_attributes", (unsigned long) &tab_attributes },
	{ "tab_options", (unsigned long) &tab_options },
	{ "tab_reserved", (unsigned long) &tab_reserved },
	{ "tcgetattr", (unsigned long) &tcgetattr },
	{ "tcsetattr", (unsigned long) &tcsetattr },
	{ "test_binop", (unsigned long) &test_binop },
	{ "test_inode", (unsigned long) &test_inode },
	{ "test_mode", (unsigned long) &test_mode },
	{ "test_optable", (unsigned long) &test_optable },
	{ "test_unops", (unsigned long) &test_unops },
	{ "tracked_names", (unsigned long) &tracked_names },
	{ "tty_alt", (unsigned long) &tty_alt },
	{ "tty_check", (unsigned long) &tty_check },
	{ "tty_cooked", (unsigned long) &tty_cooked },
	{ "tty_get", (unsigned long) &tty_get },
	{ "tty_raw", (unsigned long) &tty_raw },
	{ "tty_set", (unsigned long) &tty_set },
	{ "unop_test", (unsigned long) &unop_test },
	{ "utol", (unsigned long) &utol },
	{ "utos", (unsigned long) &utos },
	{ "varload", (unsigned long) &varload },
	{ "vi_read", (unsigned long) &vi_read },
	{ "wk_libinit", (unsigned long) &wk_libinit },
	{ "xk_free", (unsigned long) &xk_free },
	{ "xk_get_delim", (unsigned long) &xk_get_delim },
	{ "xk_get_pardelim", (unsigned long) &xk_get_pardelim },
	{ "xk_parse", (unsigned long) &xk_parse },
	{ "xk_prdebug", (unsigned long) &xk_prdebug },
	{ "xk_print", (unsigned long) &xk_print },
	{ "xk_ret_buf", (unsigned long) &xk_ret_buf },
	{ "xk_ret_buffer", (unsigned long) &xk_ret_buffer },
	{ "xk_usage", (unsigned long) &xk_usage },
	{ "xkhash_add", (unsigned long) &xkhash_add },
	{ "xkhash_find", (unsigned long) &xkhash_find },
	{ "xkhash_init", (unsigned long) &xkhash_init },
	{ "xkhash_override", (unsigned long) &xkhash_override },
	{ 0, 0 }
};

int Symsize = sizeof(Symarray) / sizeof(struct symarray);
