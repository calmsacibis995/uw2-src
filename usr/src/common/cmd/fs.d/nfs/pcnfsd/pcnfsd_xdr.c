/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:pcnfsd/pcnfsd_xdr.c	1.1"
#ident	"$Header: $"

/*
**=====================================================================
** Copyright (c) 1986-1993 by Sun Microsystems, Inc.
**=====================================================================
*/

/*
**=====================================================================
** Any and all changes made herein to the original code obtained from
** Sun Microsystems may not be supported by Sun Microsystems, Inc.
**=====================================================================
*/

/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <rpc/rpc.h>
#include "pcnfsd.h"

bool_t
xdr_ident(xdrs, objp)
	XDR *xdrs;
	ident *objp;
{
	if (!xdr_string(xdrs, objp, IDENTLEN)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_message(xdrs, objp)
	XDR *xdrs;
	message *objp;
{
	if (!xdr_string(xdrs, objp, MESSAGELEN)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_password(xdrs, objp)
	XDR *xdrs;
	password *objp;
{
	if (!xdr_string(xdrs, objp, PASSWORDLEN)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_client(xdrs, objp)
	XDR *xdrs;
	client *objp;
{
	if (!xdr_string(xdrs, objp, CLIENTLEN)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_printername(xdrs, objp)
	XDR *xdrs;
	printername *objp;
{
	if (!xdr_string(xdrs, objp, PRINTERNAMELEN)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_username(xdrs, objp)
	XDR *xdrs;
	username *objp;
{
	if (!xdr_string(xdrs, objp, USERNAMELEN)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_comment(xdrs, objp)
	XDR *xdrs;
	comment *objp;
{
	if (!xdr_string(xdrs, objp, COMMENTLEN)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_spoolname(xdrs, objp)
	XDR *xdrs;
	spoolname *objp;
{
	if (!xdr_string(xdrs, objp, SPOOLNAMELEN)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_printjobid(xdrs, objp)
	XDR *xdrs;
	printjobid *objp;
{
	if (!xdr_string(xdrs, objp, PRINTJOBIDLEN)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_homedir(xdrs, objp)
	XDR *xdrs;
	homedir *objp;
{
	if (!xdr_string(xdrs, objp, OPTIONSLEN)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_options(xdrs, objp)
	XDR *xdrs;
	options *objp;
{
	if (!xdr_string(xdrs, objp, OPTIONSLEN)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_arstat(xdrs, objp)
	XDR *xdrs;
	arstat *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_alrstat(xdrs, objp)
	XDR *xdrs;
	alrstat *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_pirstat(xdrs, objp)
	XDR *xdrs;
	pirstat *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_pcrstat(xdrs, objp)
	XDR *xdrs;
	pcrstat *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_psrstat(xdrs, objp)
	XDR *xdrs;
	psrstat *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_mapreq(xdrs, objp)
	XDR *xdrs;
	mapreq *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_maprstat(xdrs, objp)
	XDR *xdrs;
	maprstat *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_auth_args(xdrs, objp)
	XDR *xdrs;
	auth_args *objp;
{
	if (!xdr_ident(xdrs, &objp->id)) {
		return (FALSE);
	}
	if (!xdr_password(xdrs, &objp->pw)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_auth_results(xdrs, objp)
	XDR *xdrs;
	auth_results *objp;
{
	if (!xdr_arstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->uid)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->gid)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_pr_init_args(xdrs, objp)
	XDR *xdrs;
	pr_init_args *objp;
{
	if (!xdr_client(xdrs, &objp->system)) {
		return (FALSE);
	}
	if (!xdr_printername(xdrs, &objp->pn)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_pr_init_results(xdrs, objp)
	XDR *xdrs;
	pr_init_results *objp;
{
	if (!xdr_pirstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_spoolname(xdrs, &objp->dir)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_pr_start_args(xdrs, objp)
	XDR *xdrs;
	pr_start_args *objp;
{
	if (!xdr_client(xdrs, &objp->system)) {
		return (FALSE);
	}
	if (!xdr_printername(xdrs, &objp->pn)) {
		return (FALSE);
	}
	if (!xdr_username(xdrs, &objp->user)) {
		return (FALSE);
	}
	if (!xdr_spoolname(xdrs, &objp->file)) {
		return (FALSE);
	}
	if (!xdr_options(xdrs, &objp->opts)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_pr_start_results(xdrs, objp)
	XDR *xdrs;
	pr_start_results *objp;
{
	if (!xdr_psrstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_info_args(xdrs, objp)
	XDR *xdrs;
	v2_info_args *objp;
{
	if (!xdr_comment(xdrs, &objp->vers)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_info_results(xdrs, objp)
	XDR *xdrs;
	v2_info_results *objp;
{
	if (!xdr_comment(xdrs, &objp->vers)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	if (!xdr_array(xdrs, (char **)&objp->facilities.facilities_val, (u_int *)&objp->facilities.facilities_len, FACILITIESMAX, sizeof(int), xdr_int)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_init_args(xdrs, objp)
	XDR *xdrs;
	v2_pr_init_args *objp;
{
	if (!xdr_client(xdrs, &objp->system)) {
		return (FALSE);
	}
	if (!xdr_printername(xdrs, &objp->pn)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_init_results(xdrs, objp)
	XDR *xdrs;
	v2_pr_init_results *objp;
{
	if (!xdr_pirstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_spoolname(xdrs, &objp->dir)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_start_args(xdrs, objp)
	XDR *xdrs;
	v2_pr_start_args *objp;
{
	if (!xdr_client(xdrs, &objp->system)) {
		return (FALSE);
	}
	if (!xdr_printername(xdrs, &objp->pn)) {
		return (FALSE);
	}
	if (!xdr_username(xdrs, &objp->user)) {
		return (FALSE);
	}
	if (!xdr_spoolname(xdrs, &objp->file)) {
		return (FALSE);
	}
	if (!xdr_options(xdrs, &objp->opts)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->copies)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_start_results(xdrs, objp)
	XDR *xdrs;
	v2_pr_start_results *objp;
{
	if (!xdr_psrstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_printjobid(xdrs, &objp->id)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_pr_list(xdrs, objp)
	XDR *xdrs;
	pr_list *objp;
{
	if (!xdr_pointer(xdrs, (char **)objp, sizeof(struct pr_list_item), xdr_pr_list_item)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_pr_list_item(xdrs, objp)
	XDR *xdrs;
	pr_list_item *objp;
{
	if (!xdr_printername(xdrs, &objp->pn)) {
		return (FALSE);
	}
	if (!xdr_printername(xdrs, &objp->device)) {
		return (FALSE);
	}
	if (!xdr_client(xdrs, &objp->remhost)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	if (!xdr_pr_list(xdrs, &objp->pr_next)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_list_results(xdrs, objp)
	XDR *xdrs;
	v2_pr_list_results *objp;
{
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	if (!xdr_pr_list(xdrs, &objp->printers)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_queue_args(xdrs, objp)
	XDR *xdrs;
	v2_pr_queue_args *objp;
{
	if (!xdr_printername(xdrs, &objp->pn)) {
		return (FALSE);
	}
	if (!xdr_client(xdrs, &objp->system)) {
		return (FALSE);
	}
	if (!xdr_username(xdrs, &objp->user)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->just_mine)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_pr_queue(xdrs, objp)
	XDR *xdrs;
	pr_queue *objp;
{
	if (!xdr_pointer(xdrs, (char **)objp, sizeof(struct pr_queue_item), xdr_pr_queue_item)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_pr_queue_item(xdrs, objp)
	XDR *xdrs;
	pr_queue_item *objp;
{
	if (!xdr_int(xdrs, &objp->position)) {
		return (FALSE);
	}
	if (!xdr_printjobid(xdrs, &objp->id)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->size)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->status)) {
		return (FALSE);
	}
	if (!xdr_client(xdrs, &objp->system)) {
		return (FALSE);
	}
	if (!xdr_username(xdrs, &objp->user)) {
		return (FALSE);
	}
	if (!xdr_spoolname(xdrs, &objp->file)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	if (!xdr_pr_queue(xdrs, &objp->pr_next)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_queue_results(xdrs, objp)
	XDR *xdrs;
	v2_pr_queue_results *objp;
{
	if (!xdr_pirstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->just_yours)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->qlen)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->qshown)) {
		return (FALSE);
	}
	if (!xdr_pr_queue(xdrs, &objp->jobs)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_cancel_args(xdrs, objp)
	XDR *xdrs;
	v2_pr_cancel_args *objp;
{
	if (!xdr_printername(xdrs, &objp->pn)) {
		return (FALSE);
	}
	if (!xdr_client(xdrs, &objp->system)) {
		return (FALSE);
	}
	if (!xdr_username(xdrs, &objp->user)) {
		return (FALSE);
	}
	if (!xdr_printjobid(xdrs, &objp->id)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_cancel_results(xdrs, objp)
	XDR *xdrs;
	v2_pr_cancel_results *objp;
{
	if (!xdr_pcrstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_status_args(xdrs, objp)
	XDR *xdrs;
	v2_pr_status_args *objp;
{
	if (!xdr_printername(xdrs, &objp->pn)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_status_results(xdrs, objp)
	XDR *xdrs;
	v2_pr_status_results *objp;
{
	if (!xdr_pirstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->avail)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->printing)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->qlen)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->needs_operator)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->status)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_admin_args(xdrs, objp)
	XDR *xdrs;
	v2_pr_admin_args *objp;
{
	if (!xdr_client(xdrs, &objp->system)) {
		return (FALSE);
	}
	if (!xdr_username(xdrs, &objp->user)) {
		return (FALSE);
	}
	if (!xdr_printername(xdrs, &objp->pn)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_admin_results(xdrs, objp)
	XDR *xdrs;
	v2_pr_admin_results *objp;
{
	if (!xdr_pirstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_requeue_args(xdrs, objp)
	XDR *xdrs;
	v2_pr_requeue_args *objp;
{
	if (!xdr_printername(xdrs, &objp->pn)) {
		return (FALSE);
	}
	if (!xdr_client(xdrs, &objp->system)) {
		return (FALSE);
	}
	if (!xdr_username(xdrs, &objp->user)) {
		return (FALSE);
	}
	if (!xdr_printjobid(xdrs, &objp->id)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->qpos)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_requeue_results(xdrs, objp)
	XDR *xdrs;
	v2_pr_requeue_results *objp;
{
	if (!xdr_pcrstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_hold_args(xdrs, objp)
	XDR *xdrs;
	v2_pr_hold_args *objp;
{
	if (!xdr_printername(xdrs, &objp->pn)) {
		return (FALSE);
	}
	if (!xdr_client(xdrs, &objp->system)) {
		return (FALSE);
	}
	if (!xdr_username(xdrs, &objp->user)) {
		return (FALSE);
	}
	if (!xdr_printjobid(xdrs, &objp->id)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_hold_results(xdrs, objp)
	XDR *xdrs;
	v2_pr_hold_results *objp;
{
	if (!xdr_pcrstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_release_args(xdrs, objp)
	XDR *xdrs;
	v2_pr_release_args *objp;
{
	if (!xdr_printername(xdrs, &objp->pn)) {
		return (FALSE);
	}
	if (!xdr_client(xdrs, &objp->system)) {
		return (FALSE);
	}
	if (!xdr_username(xdrs, &objp->user)) {
		return (FALSE);
	}
	if (!xdr_printjobid(xdrs, &objp->id)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_pr_release_results(xdrs, objp)
	XDR *xdrs;
	v2_pr_release_results *objp;
{
	if (!xdr_pcrstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_mapreq_arg(xdrs, objp)
	XDR *xdrs;
	mapreq_arg *objp;
{
	if (!xdr_pointer(xdrs, (char **)objp, sizeof(struct mapreq_arg_item), xdr_mapreq_arg_item)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_mapreq_arg_item(xdrs, objp)
	XDR *xdrs;
	mapreq_arg_item *objp;
{
	if (!xdr_mapreq(xdrs, &objp->req)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->id)) {
		return (FALSE);
	}
	if (!xdr_username(xdrs, &objp->name)) {
		return (FALSE);
	}
	if (!xdr_mapreq_arg(xdrs, &objp->mapreq_next)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_mapreq_res(xdrs, objp)
	XDR *xdrs;
	mapreq_res *objp;
{
	if (!xdr_pointer(xdrs, (char **)objp, sizeof(struct mapreq_res_item), xdr_mapreq_res_item)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_mapreq_res_item(xdrs, objp)
	XDR *xdrs;
	mapreq_res_item *objp;
{
	if (!xdr_mapreq(xdrs, &objp->req)) {
		return (FALSE);
	}
	if (!xdr_maprstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->id)) {
		return (FALSE);
	}
	if (!xdr_username(xdrs, &objp->name)) {
		return (FALSE);
	}
	if (!xdr_mapreq_res(xdrs, &objp->mapreq_next)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_mapid_args(xdrs, objp)
	XDR *xdrs;
	v2_mapid_args *objp;
{
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	if (!xdr_mapreq_arg(xdrs, &objp->req_list)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_mapid_results(xdrs, objp)
	XDR *xdrs;
	v2_mapid_results *objp;
{
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	if (!xdr_mapreq_res(xdrs, &objp->res_list)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_auth_args(xdrs, objp)
	XDR *xdrs;
	v2_auth_args *objp;
{
	if (!xdr_client(xdrs, &objp->system)) {
		return (FALSE);
	}
	if (!xdr_ident(xdrs, &objp->id)) {
		return (FALSE);
	}
	if (!xdr_password(xdrs, &objp->pw)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_auth_results(xdrs, objp)
	XDR *xdrs;
	v2_auth_results *objp;
{
	if (!xdr_arstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->uid)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->gid)) {
		return (FALSE);
	}
	if (!xdr_array(xdrs, (char **)&objp->gids.gids_val, (u_int *)&objp->gids.gids_len, EXTRAGIDLEN, sizeof(u_int), xdr_u_int)) {
		return (FALSE);
	}
	if (!xdr_homedir(xdrs, &objp->home)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->def_umask)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_alert_args(xdrs, objp)
	XDR *xdrs;
	v2_alert_args *objp;
{
	if (!xdr_client(xdrs, &objp->system)) {
		return (FALSE);
	}
	if (!xdr_printername(xdrs, &objp->pn)) {
		return (FALSE);
	}
	if (!xdr_username(xdrs, &objp->user)) {
		return (FALSE);
	}
	if (!xdr_message(xdrs, &objp->msg)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_v2_alert_results(xdrs, objp)
	XDR *xdrs;
	v2_alert_results *objp;
{
	if (!xdr_alrstat(xdrs, &objp->stat)) {
		return (FALSE);
	}
	if (!xdr_comment(xdrs, &objp->cm)) {
		return (FALSE);
	}
	return (TRUE);
}
#if RPC_SVC
 void msg_out(msg) char *msg; {_msgout(msg);}
#endif
#if RPC_HDR
 extern void msg_out();
#endif
