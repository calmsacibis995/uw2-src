/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)nfs.cmds:lockd/sm_inter.h	1.3"
#ident  "$Header: $"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _SM_INTER_H_RPCGEN
#define _SM_INTER_H_RPCGEN

#include <rpc/rpc.h>

#define SM_MAXSTRLEN 1024

struct sm_name {
	char *mon_name;
};
typedef struct sm_name sm_name;
#ifdef __cplusplus 
extern "C" bool_t xdr_sm_name(XDR *, sm_name*);
#elif __STDC__ 
extern  bool_t xdr_sm_name(XDR *, sm_name*);
#else /* Old Style C */ 
bool_t xdr_sm_name();
#endif /* Old Style C */ 


struct my_id {
	char *my_name;
	int my_prog;
	int my_vers;
	int my_proc;
};
typedef struct my_id my_id;
#ifdef __cplusplus 
extern "C" bool_t xdr_my_id(XDR *, my_id*);
#elif __STDC__ 
extern  bool_t xdr_my_id(XDR *, my_id*);
#else /* Old Style C */ 
bool_t xdr_my_id();
#endif /* Old Style C */ 


struct mon_id {
	char *mon_name;
	struct my_id my_idno;
};
typedef struct mon_id mon_id;
#ifdef __cplusplus 
extern "C" bool_t xdr_mon_id(XDR *, mon_id*);
#elif __STDC__ 
extern  bool_t xdr_mon_id(XDR *, mon_id*);
#else /* Old Style C */ 
bool_t xdr_mon_id();
#endif /* Old Style C */ 


struct mon {
	struct mon_id mon_idno;
	char priv[16];
};
typedef struct mon mon;
#ifdef __cplusplus 
extern "C" bool_t xdr_mon(XDR *, mon*);
#elif __STDC__ 
extern  bool_t xdr_mon(XDR *, mon*);
#else /* Old Style C */ 
bool_t xdr_mon();
#endif /* Old Style C */ 


struct sm_stat {
	int state;
};
typedef struct sm_stat sm_stat;
#ifdef __cplusplus 
extern "C" bool_t xdr_sm_stat(XDR *, sm_stat*);
#elif __STDC__ 
extern  bool_t xdr_sm_stat(XDR *, sm_stat*);
#else /* Old Style C */ 
bool_t xdr_sm_stat();
#endif /* Old Style C */ 


enum res {
	stat_succ = 0,
	stat_fail = 1,
};
typedef enum res res;
#ifdef __cplusplus 
extern "C" bool_t xdr_res(XDR *, res*);
#elif __STDC__ 
extern  bool_t xdr_res(XDR *, res*);
#else /* Old Style C */ 
bool_t xdr_res();
#endif /* Old Style C */ 


struct sm_stat_res {
	res res_stat;
	int state;
};
typedef struct sm_stat_res sm_stat_res;
#ifdef __cplusplus 
extern "C" bool_t xdr_sm_stat_res(XDR *, sm_stat_res*);
#elif __STDC__ 
extern  bool_t xdr_sm_stat_res(XDR *, sm_stat_res*);
#else /* Old Style C */ 
bool_t xdr_sm_stat_res();
#endif /* Old Style C */ 


struct status {
	char *mon_name;
	int state;
	char priv[16];
};
typedef struct status status;
#ifdef __cplusplus 
extern "C" bool_t xdr_status(XDR *, status*);
#elif __STDC__ 
extern  bool_t xdr_status(XDR *, status*);
#else /* Old Style C */ 
bool_t xdr_status();
#endif /* Old Style C */ 


#define SM_PROG ((u_long)100024)
#define SM_VERS ((u_long)1)

#ifdef __cplusplus
#define SM_STAT ((u_long)1)
extern "C" struct sm_stat_res * sm_stat_1(struct sm_name *, CLIENT *);
extern "C" struct sm_stat_res * sm_stat_1_svc(struct sm_name *, struct svc_req *);
#define SM_MON ((u_long)2)
extern "C" struct sm_stat_res * sm_mon_1(struct mon *, CLIENT *);
extern "C" struct sm_stat_res * sm_mon_1_svc(struct mon *, struct svc_req *);
#define SM_UNMON ((u_long)3)
extern "C" struct sm_stat * sm_unmon_1(struct mon_id *, CLIENT *);
extern "C" struct sm_stat * sm_unmon_1_svc(struct mon_id *, struct svc_req *);
#define SM_UNMON_ALL ((u_long)4)
extern "C" struct sm_stat * sm_unmon_all_1(struct my_id *, CLIENT *);
extern "C" struct sm_stat * sm_unmon_all_1_svc(struct my_id *, struct svc_req *);
#define SM_SIMU_CRASH ((u_long)5)
extern "C" void * sm_simu_crash_1(void *, CLIENT *);
extern "C" void * sm_simu_crash_1_svc(void *, struct svc_req *);

#elif __STDC__
#define SM_STAT ((u_long)1)
extern  struct sm_stat_res * sm_stat_1(struct sm_name *, CLIENT *);
extern  struct sm_stat_res * sm_stat_1_svc(struct sm_name *, struct svc_req *);
#define SM_MON ((u_long)2)
extern  struct sm_stat_res * sm_mon_1(struct mon *, CLIENT *);
extern  struct sm_stat_res * sm_mon_1_svc(struct mon *, struct svc_req *);
#define SM_UNMON ((u_long)3)
extern  struct sm_stat * sm_unmon_1(struct mon_id *, CLIENT *);
extern  struct sm_stat * sm_unmon_1_svc(struct mon_id *, struct svc_req *);
#define SM_UNMON_ALL ((u_long)4)
extern  struct sm_stat * sm_unmon_all_1(struct my_id *, CLIENT *);
extern  struct sm_stat * sm_unmon_all_1_svc(struct my_id *, struct svc_req *);
#define SM_SIMU_CRASH ((u_long)5)
extern  void * sm_simu_crash_1(void *, CLIENT *);
extern  void * sm_simu_crash_1_svc(void *, struct svc_req *);

#else /* Old Style C */ 
#define SM_STAT ((u_long)1)
extern  struct sm_stat_res * sm_stat_1();
extern  struct sm_stat_res * sm_stat_1_svc();
#define SM_MON ((u_long)2)
extern  struct sm_stat_res * sm_mon_1();
extern  struct sm_stat_res * sm_mon_1_svc();
#define SM_UNMON ((u_long)3)
extern  struct sm_stat * sm_unmon_1();
extern  struct sm_stat * sm_unmon_1_svc();
#define SM_UNMON_ALL ((u_long)4)
extern  struct sm_stat * sm_unmon_all_1();
extern  struct sm_stat * sm_unmon_all_1_svc();
#define SM_SIMU_CRASH ((u_long)5)
extern  void * sm_simu_crash_1();
extern  void * sm_simu_crash_1_svc();
#endif /* Old Style C */ 

#endif /* !_SM_INTER_H_RPCGEN */
