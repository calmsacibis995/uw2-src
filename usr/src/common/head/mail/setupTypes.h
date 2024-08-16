/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:mail/setupTypes.h	1.2"
#if	!defined(SETUPTYPE_H)
#define	SETUPTYPE_H

#if	!defined(SETUPSPECIFICFILE_OBJ)
typedef void setupVariableHandle_t;
typedef void setupFileHandle_t;
#endif

#if	!defined(SETUPFILE_OBJ)
typedef void fileHandle_t;
#endif

#if	!defined(SETUPWEB_OBJ)
typedef void setupWeb_t;
#endif

#if	!defined(SETUPMOVE_OBJ)
typedef void setupMove_t;
#endif

#if	!defined(SETUPVAR_OBJ)
typedef void setupVar_t;
#endif

typedef unsigned setupFlagType_t;

typedef enum setupVariableType_e
    {
    svt_string,
    svt_integer,
    svt_flag,
    svt_menu,
    svt_password,
    svt_none
    }	setupVariableType_t;

typedef struct setupVarOps_s
    {
    void
	(*svop_localDataSet)(setupVariableHandle_t *handle, char **localData),
	(*svop_free)(setupVariableHandle_t *handle);

    char
	*(*svop_setValue)(setupVariableHandle_t *handle, void *variable),
	**svop_choices;

    int
	(*svop_getValue)(setupVariableHandle_t *handle, void *variable),
	(*svop_setDefaultValue)(setupVariableHandle_t *handle, char *variable),
	(*svop_defaultValue)(setupVariableHandle_t *handle),
	(*svop_applyValue)(setupVariableHandle_t *handle),
	(*svop_resetValue)(setupVariableHandle_t *handle);
    
    setupVariableHandle_t
	*svop_handle;

    setupVariableType_t
	svop_type;
    }	setupVarOps_t;

typedef enum moveDirectrion_e
    {
    SM_DOWN,
    SM_LEFT,
    SM_NEXT,
    SM_RIGHT,
    SM_TAB,
    SM_UP
    }	moveDirection_t;

#endif
