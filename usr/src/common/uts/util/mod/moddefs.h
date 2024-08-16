/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MODDEFS_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MODDEFS_H	/* subject to change without notice */

#ident	"@(#)kern:util/mod/moddefs.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL

extern void mod_drvattach(void *);
extern void mod_drvdetach(void *);

#define MODREV	11

struct mod_operations {
	int	(*modm_install)();
	int	(*modm_remove)();
	int	(*modm_info)();
	int	(*modm_bind)();
};

extern struct mod_operations mod_drv_ops;
extern struct mod_operations mod_str_ops;
extern struct mod_operations mod_fs_ops;


struct modlink {
	struct	mod_operations	*ml_ops;
	void	*ml_type_data;
};

/*
 * Module type specific linkage structure.
 */

struct mod_type_data	{
	char 	*mtd_info;
	void	*mtd_pdata;
};

struct modwrapper_rev10 {
	int	mw_rev;
	int	(*mw_load)();
	int	(*mw_unload)();
	void	(*mw_halt)();
	void	*mw_conf_data;
	struct	modlink	*mw_modlink;
};

struct modwrapper {
	int	mw_rev;
	int	(*mw_load)();
	int	(*mw_unload)();
	void	(*mw_halt)();
	int	(*mw_verify)();
	void	*mw_conf_data;
	struct	modlink	*mw_modlink;
};

#define	_STR(a)		#a
#define	_XSTR(a)	_STR(a)
#define	_WEAK(p, s)	asm(_XSTR(.weak p##s));

#define	MOD_DRV_WRAPPER(p, l, u, h, n)	\
		extern	void	*p##_conf_data;	\
		extern	void	p##_mod_drvdata; \
		extern	void	p##_attach_info; \
		extern	struct	mod_operations	mod_drv_ops; \
		_WEAK(, mod_drv_ops) \
		_WEAK(, mod_drvattach) \
		_WEAK(, mod_drvdetach) \
		_WEAK(p, _mod_drvdata) \
		_WEAK(p, _conf_data) \
		_WEAK(p, _attach_info) \
		static	struct	mod_type_data	p##_drv_link = { \
			n, &p##_mod_drvdata \
		}; \
		static	struct	modlink	p##_mod_link[] = { \
			{ &mod_drv_ops, &p##_drv_link }, \
			{ NULL, NULL } \
		}; \
		struct	modwrapper	p##_wrapper = { \
			MODREV, l, u, h, \
			(int (*)())0, \
			&p##_conf_data, \
			p##_mod_link \
		}

#define MOD_HDRV_WRAPPER(p, l, u, h, n)	\
		extern	void	*p##_conf_data;	\
		extern	void	p##_attach_info; \
		extern	struct	mod_operations	mod_misc_ops; \
		_WEAK(, mod_misc_ops) \
		_WEAK(, mod_drvattach) \
		_WEAK(, mod_drvdetach) \
		_WEAK(p, _conf_data) \
		_WEAK(p, _attach_info) \
		static	struct	mod_type_data	p##_misc_link = { \
			n, NULL \
		}; \
		static 	struct	modlink p##_mod_link[] = { \
			{ &mod_misc_ops, &p##_misc_link }, \
			{ NULL, NULL } \
		}; \
		struct	modwrapper	p##_wrapper = { \
			MODREV, l, u, h, \
			(int (*)())0, \
			&p##_conf_data, \
			p##_mod_link \
		}

#define	MOD_ACDRV_WRAPPER(p, l, u, h, v, n)	\
		extern	void	*p##_conf_data;	\
		extern	void	p##_mod_drvdata; \
		extern	void	p##_attach_info; \
		extern	struct	mod_operations	mod_drv_ops; \
		_WEAK(, mod_drv_ops) \
		_WEAK(, mod_drvattach) \
		_WEAK(, mod_drvdetach) \
		_WEAK(p, _mod_drvdata) \
		_WEAK(p, _conf_data) \
		_WEAK(p, _attach_info) \
		static	struct	mod_type_data	p##_drv_link = { \
			n, &p##_mod_drvdata \
		}; \
		static	struct	modlink	p##_mod_link[] = { \
			{ &mod_drv_ops, &p##_drv_link }, \
			{ NULL, NULL } \
		}; \
		struct	modwrapper	p##_wrapper = { \
			MODREV, l, u, h, v, \
			&p##_conf_data, \
			p##_mod_link \
		}

#define MOD_ACHDRV_WRAPPER(p, l, u, h, v, n)	\
		extern	void	*p##_conf_data;	\
		extern	void	p##_attach_info; \
		extern	struct	mod_operations	mod_misc_ops; \
		_WEAK(, mod_misc_ops) \
		_WEAK(, mod_drvattach) \
		_WEAK(, mod_drvdetach) \
		_WEAK(p, _conf_data) \
		_WEAK(p, _attach_info) \
		static	struct	mod_type_data	p##_misc_link = { \
			n, NULL \
		}; \
		static 	struct	modlink p##_mod_link[] = { \
			{ &mod_misc_ops, &p##_misc_link }, \
			{ NULL, NULL } \
		}; \
		struct	modwrapper	p##_wrapper = { \
			MODREV, l, u, h, v, \
			&p##_conf_data, \
			p##_mod_link \
		}

#define	MOD_STR_WRAPPER(p, l, u, n)	\
		extern	void	*p##_conf_data;	\
		extern	void	p##_mod_strdata; \
		extern	struct	mod_operations	mod_str_ops; \
		_WEAK(, mod_str_ops) \
		_WEAK(p, _mod_strdata) \
		_WEAK(p, _conf_data) \
		static	struct	mod_type_data	p##_str_link = { \
			n, &p##_mod_strdata \
		}; \
		static	struct	modlink	p##_mod_link[] = { \
			{ &mod_str_ops, &p##_str_link }, \
			{ NULL, NULL } \
		}; \
		struct	modwrapper	p##_wrapper = { \
			MODREV, l, u, \
			(void (*)())0, \
			(int (*)())0, \
			&p##_conf_data, \
			p##_mod_link \
		}

#define	MOD_FS_WRAPPER(p, l, u, n)	\
		extern	void	*p##_conf_data;	\
		extern	void	p##_mod_fsdata; \
		extern	struct	mod_operations	mod_fs_ops; \
		_WEAK(, mod_fs_ops) \
		_WEAK(p, _mod_fsdata) \
		_WEAK(p, _conf_data) \
		static	struct	mod_type_data	p##_fs_link = { \
			n, &p##_mod_fsdata \
		}; \
		static	struct	modlink	p##_mod_link[] = { \
			{ &mod_fs_ops, &p##_fs_link }, \
			{ NULL, NULL } \
		}; \
		struct	modwrapper	p##_wrapper = { \
			MODREV, l, u, \
			(void (*)())0, \
			(int (*)())0, \
			&p##_conf_data, \
			p##_mod_link \
		}

#define MOD_EXEC_WRAPPER(p, l, u, n)	\
		extern	void	*p##_conf_data;	\
		extern	void	p##_mod_execdata; \
		extern	struct	mod_operations	mod_exec_ops; \
		_WEAK(p, _conf_data) \
		_WEAK(p, _mod_execdata) \
		_WEAK(, mod_exec_ops) \
		static	struct	mod_type_data	p##_exec_link = { \
			n, &p##_mod_execdata \
		}; \
		static 	struct	modlink p##_mod_link[] = { \
			{ &mod_exec_ops, &p##_exec_link }, \
			{ NULL, NULL } \
		}; \
		struct	modwrapper	p##_wrapper = { \
			MODREV, l, u, \
			(void (*)())0, \
			(int (*)())0, \
			&p##_conf_data, \
			p##_mod_link \
		}

#define MOD_MISC_WRAPPER(p, l, u, n)	\
		extern	void	*p##_conf_data;	\
		extern	struct	mod_operations	mod_misc_ops; \
		_WEAK(p, _conf_data) \
		_WEAK(, mod_misc_ops) \
		static	struct	mod_type_data	p##_misc_link = { \
			n, NULL \
		}; \
		static 	struct	modlink p##_mod_link[] = { \
			{ &mod_misc_ops, &p##_misc_link }, \
			{ NULL, NULL } \
		}; \
		struct	modwrapper	p##_wrapper = { \
			MODREV, l, u, \
			(void (*)())0, \
			(int (*)())0, \
			&p##_conf_data, \
			p##_mod_link \
		}

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MOD_MODDEFS_H */
