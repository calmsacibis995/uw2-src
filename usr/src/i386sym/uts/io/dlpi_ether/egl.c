/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/dlpi_ether/egl.c	1.20"
#ident	"$Header: $"

/* 
 * egl.c : Ethernet driver for interphase 4207 eagle ethernet controller
 */

#include <util/types.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/ksynch.h>
#include <util/ipl.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <mem/kmem.h>
#include <io/cfg.h>
#include <io/ssm/ssm_vme.h>
#include <io/stream.h>
#include <io/strmdep.h>
#include <io/stropts.h>
#include <io/termio.h>
#include <io/strlog.h>
#include <net/dlpi.h>
#include <net/inet/if.h>
#include <svc/errno.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <io/dlpi_ether/dlpi_egl.h>
#include <io/dlpi_ether/conf_attr.h>
#ifdef DEBUG
#include <sys/xdebug.h>
#endif
#include <io/ddi.h>

/* LAF POLYNOMIAL */
#define EGL_POLY		0xedb88320	/* just a magic number */

/* BOARD spec */
#define G_LIST_OFFSET		0x40000		/* gather list space offset */	
#define G_LIST_SIZE		200		/* gather list size */
#define FCS_SIZE		4		/* size of FCS in ethernet packet */

int	egl_devflag = D_MP;	/* Multi-threaded */

struct g_list_entry {				/* gather list struct on VME */
	ulong	vmeadd ;			/* addr on the VME bus */
	ushort	vmeopt ;			/* vme options */
	ushort	xfrcnt ;			/* transfer cnt */
} ;

/* vmeopt */
#define VME_BLK_XFR	0x400			/* blk transfer */
#define VME_DMA_READ	0x1000			/* dma direction */
#define VME_MEM_32	0x200			/* mem type */
#define VME_AM32	0x0D			/* Addr modifier for A32 space */
#define VME_AM24	0x3D			/* Addr modifier for A24 space */
#define VME_AM24_BLK	0x3F			/* Addr modifier for A24 space with */
						/* blk transfer */

/* IOPB */
struct g_iopb {			/* genetic iopb struct */
	ushort	cmd ;
	ushort	cmdo ;		/* cmd options */
	ushort	stat ;		/* status */
	unchar	n_vec ;		/* normal completion vector */
	unchar	n_lvl ;		/* normal completion level */
	unchar	e_vec ;		/* error completion vector */
	unchar	e_lvl ;		/* error completion level */
} ;

struct init_iopb {		/* iopb for INIT_CMD */
	ushort	cmdo ;
	ushort	cmd ;
	unchar	n_vec ;
	unchar	n_lvl ;
	ushort	stat ;
	ushort  qqq ;
	unchar	e_vec ;
	unchar	e_lvl ;

	ulong	ib_offset ;	/* offset to initialization blk */
	unchar	resd1[20] ;
} ;

struct xmt_iopb {		/* iopb for XMT_CMD */
	ushort	cmdo ;
	ushort	cmd ;
	unchar	n_vec ;
	unchar	n_lvl ;
	ushort	stat ;
	ushort  vme_opt ;
	unchar	e_vec ;
	unchar	e_lvl ;

	ulong	vme_add ;	/* vme address */
	ulong	dma_size ;	/* xfr size */
	unchar	resd[8] ;
	ushort	sg_cnt ;	/* gather list cnt */
	unchar	resd1[2] ;
	ushort	lance3 ;	/* word 3 of the lance chip */
	ushort	lance1 ;	/* word 1 of the lance chip */
} ;

struct rcv_iopb {		/* iopb for RCV_CMD */
	ushort	cmdo ;
	ushort	cmd ;
	unchar	n_vec ;
	unchar	n_lvl ;
	ushort	stat ;
	ushort  vme_opt ;
	unchar	e_vec ;
	unchar	e_lvl ;

	ulong	vme_add ;	/* vme address for receive buffer */
	ulong	tfr_size ;	/* transfer size */
	ushort	ptype ;		/* returned packet type */
	unchar	resd[2] ;
	unchar	s_add[4] ;	/* source address of the packet */
	unchar	resd1[2] ;
	unchar	s_add1[2] ;
	ushort	lance3 ;	/* word 3 of the lance chip */
	ushort	lance1 ;	/* word 1 of the lance chip */
} ;

struct cadd_iopb {			/* iopb for CADD_CMD */
	ushort	cmdo ;
	ushort	cmd ;
	unchar	n_vec ;
	unchar	n_lvl ;
	ushort	stat ;
	ushort  qqq ;
	unchar	e_vec ;
	unchar	e_lvl ;

	unchar	resd[8] ;
	unchar	p_addr[4] ;		/* new physical address */
	unchar	resd2[2] ;
	unchar	p_addr1[2] ;
	unchar	resd1[8] ;
} ;

struct laf_iopb {			/* iopb for LAF_CMD */
	ushort	cmdo ;
	ushort	cmd ;
	unchar	n_vec ;
	unchar	n_lvl ;
	ushort	stat ;
	ushort  qqq ;
	unchar	e_vec ;
	unchar	e_lvl ;

	unchar	resd[8] ;
	unchar	laf[8] ;		/* new filter */
	unchar	resd1[8] ;
} ;

union iopb {				/* IOPB */
	struct g_iopb		g ;
	struct init_iopb	i ;
	struct xmt_iopb 	x ;
	struct rcv_iopb 	r ;
	struct cadd_iopb 	c ;
	struct laf_iopb 	l ;
} ;

/* cmd */
#define RCV_CMD		0x60		/* receive packet */
#define XMT_CMD		0x50		/* transmit packet */
#define INIT_CMD	0x41		/* initialize the controller */
#define CADD_CMD	0x45		/* change physical node address */
#define LAF_CMD		0x46		/* change logical filter for multicast */

/* cmd options */
#define INT_ENABLE		0x1	/* interrupt enable */

/* ADDR_CMD */
#define REINIT_LANCE		0x4	/* re-init lance */

/* XMT_CMD */
#define GATHER_ENABLE		0x2	/* enable gather operation on transmit */
#define DMA_ENABLE		0x4	/* enable DMA for receive/transmit */

struct init_blk {			/* controller init blk */
	ushort	nw_opt ;		/* network options */
	unchar	cmd_q_size ;		/* cmd q size */
	unchar	resd ;

	unchar	eaddr[2] ;		/* physical node address */
	unchar	resd1[2] ;
	unchar	eaddr1[4] ;

	unchar	laf[8] ;		/* logical addresses filter */
	unchar	resd2[8] ;

	unchar	n_vec ;			/* normal completion vector */
	unchar	n_lvl ;			/* normal completion level */
	unchar	resd4[2] ;

	ushort	dma_burst_cnt ;		/* dma burst cnt in words */
	unchar	e_vec ;			/* error completion vector */
	unchar	e_lvl ;			/* error completion level */

	unchar	resd3[8] ;
} ;

/* nw_opt */
#define NW_ETHER		0x1	/* network type == ethernet */
#define PROMIS_MODE		0x8000	/* promiscuous mode bit */

#define DMA_BURST		128	/* words */

struct	cmd_q_entry {			/* command q entry on VME */
	ushort	ctl ;
	ushort	iopb_addr ;		/* offset of the iopb from beginning of SIO
					   space */
	struct cmd_buf *cmd_tag ;	/* host usable space */
	unchar	q_num ;			/* egl internal q number */
	unchar	resd ;
	unchar	resd1[2] ;
} ;

/* ctl */
#define CMD_DONE	0x1		/* cmd done */
#define CMD_COMP	0x2		/* cmd complete */
#define CMD_ERROR	0x4		/* done with error */
#define CMD_EXCEP	0x8		/* done with exception */
#define Q_MOD_ST	0x20		/* q mode started */

/* q_num */
#define Q_XMT		0x3		/* xmt q number */
#define Q_RCV		0x2		/* rcv q number */

struct	cmd_resp_blk {			/* cmd response blk */
	ushort	status ;		/* cmd returned status */
	unchar	resd[2] ;
	struct 	cmd_buf *cmd_tag ;	/* host usable space */
	unchar	iopb_len ;		/* len of the returned iopb */
	unchar	q_num ;			/* cmd q number */
	unchar	resd1[2] ;
} ;
	
struct	cf_stat { 			/* configuration status block */
	unchar	resd ;
	unchar	p_code[3] ;		/* product code */
	unchar	resd1 ;
	unchar	p_var ;			/* product variation */
	unchar	resd2 ;
	unchar	fw_lvl[3] ;		/* FW level */
	unchar	fw_date[8] ;		/* FW date */
	unchar	resd3[12] ;
	ushort	buf_size ;		/* buffer size */
	ulong	host_offset ;		/* offset to host usable space */
	ulong 	rcv_b_offset ;		/* offset to rcv buf */
	ulong	xmt_b_offset ;		/* offset to xmt buf */
	ulong	mem_offset ;		/* host usable space offset */
	unchar	p_addr[6] ;		/* physical node address */
	unchar	laf[8] ;		/* logical address filter */
	unchar	resd4[10] ;
} ;
	
struct statis { 			/* controller statistics block */
	ulong	xmt_cmd ;		/* # of frame transmission attempted */
	ulong	xmt_dma ;		/* # of DMAs completed as transmission */
	ulong	xmt_intr ;		/* # of frames transmitted by lance */
	ulong 	xmt_ok ;		/* # of frames transmitted successfully */
	ulong	xmt_failed ;		/* # of frames transmitted failed */
	ulong	xmt_host ;		/* # of transmit completions posted to host */
	
	ulong	rcv_cmd ;		/* # of receive cmds submitted */
	ulong	rcv_drop ;		/* # of packets dropped caused by overrun */
	ulong	rcv_intr ;		/* # of receive completion interrupts */
	ulong 	rcv_ok ;		/* # of successful receive */
	ulong	rcv_failed ;		/* # of failed receive */
	ulong	rcv_dma ;		/* # of receive dma completions */
	ulong	rcv_host ;		/* # of receive completions posted to host */

	ulong 	resd ;

	ulong	lance_intr ;		/* # of lance intr */
	ulong 	lance_csr0 ;		/* # of lance err */
	ulong	lance_babble ;		/* # of lance babble err */
	ulong	lance_coll ;		/* # of lance collision err */
	ulong	lance_miss ;		/* # of lance miss err */
	ulong	lance_mem ;		/* # of lance memory err */
} ;
	
struct	r_iopb { 			/* returned iopb */
	struct cmd_buf	*cmd_tag ;	/* host usable tag field */
	ushort	q_num ;			/* egl internal work q number */
	ushort	xfr_cnt ;		/* data transfer cnt */
} ;

union	ret_iopb { 			/* general return iopb */
	union	iopb	i ;
	struct	r_iopb	r_iopb[1] ;	/* for multiple completions per interrupt */
} ;
	
#define CMD_Q_SZ	36		/* cmd q size for receive/transmit */

struct	macis_blk {			/* controller short IO area */
	ushort	stat ;			/* stat register */
	unchar	resd[2] ;
	ushort	ctl ;			/* ctl register */
	unchar	resd1[2] ;
	ulong	q_ptr ;			/* not used */
	unchar	resd2[4] ;

	/* master command entry */
	ushort	cmd_ctl ;		/* per cmd status */
	ushort	iopb_add ;		/* cmd iopb offset */
	struct  cmd_buf *cmd_tag ;	/* host tag field */
	unchar	resd3[4] ;

	struct	cmd_q_entry	cmd[ CMD_Q_SZ ] ;	/* cmd q entry */
	union	iopb	iopb[ CMD_Q_SZ ] ;		/* cmd iopb */

	unchar	resd4[40] ;
	struct	init_blk	ib ;	/* initialization blk */

	struct	cmd_resp_blk	resp ;	/* cmd response blk */
	unchar	resd5[4] ;
	union	ret_iopb	r ;	/* returned iopb */
	struct	cf_stat cf_stat ;	/* board statistics */
	struct	statis	statis ;	/* configuration status */
	unchar	resd6[4] ;
} ;

/* status */
#define CTLR_NOT_AVAIL		0x1	/* controller not available */
#define BOARD_OK		0x2	/* board ok */

/* control */
#define ST_Q_MOD		0x1	/* start q mode */
#define BD_RESET		0x1000	/* board reset */
#define SET_SYSFAIL		0x2000	/* drive the SYSFAIL signal on VME */

/* cmd_ctl */
#define CMD_GO			0x1	/* issue command */

/* timeout */
#define INIT_TO			5000000	/* burn some cycles */
#define XMT_TO			5 * HZ	/* transmit timeout : 5 seconds */

#define RCV_BUF_SIZE	1530		/* give it extra 12 bytes */

struct	ssm_e {
	struct ssm_vme_mdesc	*ssp ;	/* map ram handle */
	struct ssm_e	*nsp ;
} ;

struct	cmd_buf {
	int	cmd ;		/* cmd */
	int stat ;		/* status of cmd_buf */
	struct	cmd_buf	*np ;	/* next */
	struct	cmd_buf	*pp ;	/* prev */
	mblk_t	*mp ;		/* mblk_t */
	struct	ssm_e	*sp ;	/* chain of mdesc */
	int	g_start ;	/* gather list start */
	int	g_size ;	/* gather list size */
	int	d_size ;	/* for xmt dma */
} ;

/* stat */
#define CF_USED			1	/* cmd_buf entry is used */

#define XMT_BUF_CNT		16	/* cmd_buf for transmit */
#define RCV_BUF_CNT		20	/* cmd_buf for receive */
#define SSME_PER_CTL		180	/* 16 * 10 + 20 */
#define GL_PER_CTL		160 	/* 16 * 10 */

typedef struct mcat {		/* multicast addresses */
        unchar status ;
        unchar entry[DL_MAC_ADDR_LEN] ;
} mcat_t ;

struct	egl_ctl {
	struct	map	*mp ;		/* for xmt gather list on 4207 */
	int toid ;			/* timeout id */
	struct	ssm_e	*sp ;		/* list of free ssm_e */
	int	rcv_cnt ;		/* <= 20 */
	int	xmt_cnt ;		/* <= 16 */
	struct	cmd_buf	*fip ;		/* free io_buf max 36 */
	struct	cmd_buf	*ip ;		/* used io_buf */
	ulong	g_l_vme ;		/* host usable space for g_list on 4207 */
	struct	macis_blk	*mcp ;	/* short i/o area */
	mcat_t	*egl_multiaddr ;	/* multicast addresses */
	unchar	vec ;			/* vector */
	unchar lvl ;			/* level */
	int ssmid ;			/* ssm id */
	union	iopb	iop ;		/* buffer space */
	struct	init_blk	icb ;	/* initialization blk */
	int	next_cmd ;		/* next slot in cmd q to use */
	int 	tocnt ;			/* number of time out */
	int	nbcnt ;			/* number of controller/no-buf err */
} ;

/* ETC */
#define MAC_HDR_LEN 		14	/* mac header len */
#define MULTI_ADDR_CNT		64	/* support 64 multicast addresses */
#define LAF_LEN 		8	/* logical address filter length */
#define	DL_MAC_ADDR_LEN		6	/* mac add len */
#define N_EGL_SAPS		8	/* number of SAPs */

#define EGL_SIO_AM	SSMVME_MODIFIER( 1, 0x2D )	/* for control registers */
#define EGL_GL_AM	SSMVME_MODIFIER( 1, 0x3D )	/* for gather list entries */
#define EGL_DMA_AM	SSMVME_MODIFIER( 0, 0x3F )	/* for DMA */

/* flush DMA buffer */
#define VME_FLUSH(bp)	ssm_vme_dma_flush(((struct egl_ctl *)(bp))->ssmid)	

#define POLLTAG		0xffffff00		/* tag for the polling cmds */

#define MEMALO_1_SZ		( sizeof( struct egl_ctl ) +\
			sizeof( struct ssm_e ) * SSME_PER_CTL + \
			sizeof( struct cmd_buf ) * CMD_Q_SZ +  \
			sizeof( mcat_t ) * MULTI_ADDR_CNT + 20 )

#define MEMALO_2_SZ 		( sizeof ( DL_sap_t ) *\
			N_EGL_SAPS + sizeof( struct ifstats ) + 8 )

#define EGL_HIER	2	/* lock */

STATIC LKINFO_DECL( egl_lockinfo, "ID:egl:egl_lock", 0);
int	egl_base_vec;

STATIC int issue_cmd( DL_bdconfig_t *, int , mblk_t *),
	egl_cinit( DL_bdconfig_t *, int ),
	egl_probe( volatile struct macis_blk *);

STATIC void cmd_done( DL_bdconfig_t *, struct cmd_buf *, union iopb *),
	xmt_qd_pkg( DL_bdconfig_t *),
	egl_intr( int ), 
	egl_lcopy( ulong *, ulong *, int ),
	p_dot( char *, unchar *, int ),
	egl_ck( DL_bdconfig_t *),
	egl_to( DL_bdconfig_t *);

char egl_id_string[] = "egl V1.0";

int egl_boards ;			/* for dlpi_ether layer */
extern int egl_nconf, egl_bin;
extern int ivec_alloc_group( uint , uint ),
	egl_recv( mblk_t *, DL_sap_t *);

extern void ivec_free(uint , uint ),
	ivec_init( uint, uint, void (*)());

extern struct egl_conf	*egl_conf[];
extern struct  ifstats *ifstats;

DL_bdconfig_t	egl_config[2];	/* XXX symbolic */
/*
 * int 
 * egl_init()
 *  controller initialization routine
 *
 * Calling/Exit State: 
 *  Caller must not hold any locks or sleep locks
 *
 * Description :
 *  For each possible target controller locations in the system, invoke
 *  egl_probe() to determine if an ethernet controller is present.  If
 *  it is, allocates ssm_vme_mdesc for short IO and Gather List mappings.
 *  Set up control structure, board configuration structure, multicast
 *  addresses structure, command buffer, map structure and ssm_vme_mdesc's
 *  for VME/Sequent address mappings.  Also, setup the mapping for the
 *  interrupt vectors between VME/Sequent bus.
 *  
 *  invoke egl_cinit() to configure the controller and set up receive 
 *  buffers for normal transmit/receive packet operations.
*/

int
egl_init()
{
	DL_bdconfig_t	*bp;
	DL_sap_t *sapp;
	struct macis_blk *mp;
	void *glp, *siop;
	struct egl_conf *cp;
	struct egl_ctl	*ecp;
	unchar	*ucp;
	struct	ssm_e	*sp, *sp1;
	struct	ifstats	*ifp;
	struct cmd_buf *cmdp;
	int cont, i,j;

	if( !( i = egl_boards = egl_nconf ))
		return( 0 );
	
	/* get slic vectors */
	if(( egl_base_vec = ivec_alloc_group ( egl_bin, egl_nconf )) < 0 )
	{
		/*
		 *+ de-configure the ehternet driver
		 */
		cmn_err( CE_CONT, "egl : can't get ivec \n");
		egl_boards = 0;
		return( 1 );
	}

	for (i = 0, bp = &egl_config[0]; i < egl_nconf; i++, bp++) {
		cp = egl_conf[i];
		ecp = NULL;
		glp = NULL;
		cont = 0 ;

		if ( !( SSM_EXISTS ( cp->ssmid ) && SSM_desc[cp->ssmid].ssm_vme_alive))
		{
			/*
		 	*+ either ssm or ssm/vme is missing
		 	*/
			cmn_err ( CE_CONT, "SSM/vme %d doesn't exist \n", cp->ssmid ) ;
			continue ;
		}

		/* map short io area */
		if( !( siop = ssm_vme_mdesc_alloc ( cp->ssmid, EGL_SIO_AM, 
			KM_NOSLEEP )))
		{
			/* 
			 *+ can't get mdesc, stop configuring the driver
			 */
			cmn_err( CE_CONT, "egl : can't alloc mdesc for %d \n",
				cp->ssmid );
			break ;
		}

		if( !( mp = ( struct macis_blk *)ssm_s2v_map ( siop, 
		/* LINTED pointer alignment */
			cp->vme_sio_addr, sizeof( struct macis_blk ), KM_NOSLEEP )))
		{
			/* 
			 *+ can't do mapping, stop configuring the driver
			 */
			cmn_err( CE_CONT, "egl : can't map sio for %d \n", cp->ssmid );
			goto cleanup;
		}

		/* probe 4207 */
		if( !ssm_vme_probe( cp->ssmid, egl_probe, mp ))
		{
			cont++ ;
			goto cleanup;
		}

		/* alloc control sturct, cmd buffer, multicast list, and RCV/XMT
			map ram */
		if(!( ecp = ( struct egl_ctl *)kmem_zalloc ( MEMALO_1_SZ, KM_NOSLEEP )))
		{
			/*
			 *+ out of mem, stop configuring the driver
			 */
			cmn_err( CE_CONT, "egl : can't get egl_ctl %d \n",
				cp->ssmid );
			goto cleanup;
		}

		bp->bd_dependent1 =( caddr_t )ecp;
		ecp->ssmid = cp->ssmid;

		/* map interrupt vector between VME/Sequent */
		if(( ecp->vec = ssm_vme_assign_vec ( cp->ssmid, cp->lvl,
			egl_bin, egl_base_vec + i )) < 0 )
		{
			/* 
			 *+ can't get vector, stop configuring the driver
			 */
			cmn_err( CE_CONT, "egl : can't assign vec %d \n",
				cp->ssmid );

			goto cleanup;
		}

		/* initialize vector talbe */
		ivec_init( egl_bin, egl_base_vec + i, egl_intr );

		ecp->lvl = cp->lvl;

		/* alloc ssm_e pool */
		ucp =( unchar *)ecp;
		ucp += sizeof( struct egl_ctl );
		ucp =( unchar *)(((( uint )ucp + 3 ) / 4 ) * 4 );
		/* LINTED pointer alignment */
		ecp->sp =( struct ssm_e * )ucp;

		/* alloc free command buf pool */
		ucp += sizeof( struct ssm_e ) * SSME_PER_CTL;
		ucp =( unchar *)(((( uint )ucp + 3 ) / 4 ) * 4 );
		/* LINTED pointer alignment */
		ecp->fip =( struct cmd_buf *)ucp;

		/* LINTED pointer alignment */
		cmdp =( struct cmd_buf *)ucp;
		j = CMD_Q_SZ;
		while( j-- )
		{
			if( j )
				cmdp->np = cmdp + 1;
			cmdp++;
		}

		/* alloc multicast list */
		ucp += sizeof( struct cmd_buf ) * CMD_Q_SZ;
		ucp =( unchar *)(((( uint )ucp + 3 ) / 4 ) * 4 );
		ecp->egl_multiaddr =( mcat_t * )ucp;

		/* macis_blk seq addr */
		ecp->mcp = mp;
		
		/* alloc map ram for glist */
		if( !( glp = ssm_vme_mdesc_alloc ( cp->ssmid, EGL_GL_AM, 
			KM_NOSLEEP )))
		{
			/* 
			 *+ can't get gl mdesc, stop configuring the driver
			 */
			cmn_err( CE_CONT, "egl : can't alloc mdesc for %d gl \n",
				cp->ssmid );
			goto cleanup;
		}

		if( !( ecp->g_l_vme = ( ulong )ssm_s2v_map ( glp, cp->vme_gl_addr, 
			sizeof( struct g_list_entry ) * GL_PER_CTL, KM_NOSLEEP )))
		{
			/* 
			 *+ can't map gl, stop configuring the driver
			 */
			cmn_err( CE_CONT, "egl : can't map gl for %d \n", cp->ssmid );
			goto cleanup;
		}

		/* set up map ram for XMT/RCV */
		sp = ecp->sp;
		j = SSME_PER_CTL;
		while( j-- )
		{
			/* allocate map ram handle */
			if( !( sp->ssp = ssm_vme_mdesc_alloc ( cp->ssmid, EGL_DMA_AM, 
				KM_NOSLEEP )))
			{
				/* 
			 	*+ can't get io mdesc, stop configuring the driver
			 	*/
				cmn_err( CE_CONT, "egl : can't alloc mdesc for %d ssp \n",
					cp->ssmid );
				goto cleanup;
			}

			/* reserve space */
			if( !ssm_v2s_reserve ( sp->ssp, RCV_BUF_SIZE, 
				sizeof( ulong ), KM_NOSLEEP ))
			{
				/* 
			 	*+ can't reserve mem, stop configuring the driver
			 	*/
				cmn_err( CE_CONT, "egl : can't reserve %d \n",
					cp->ssmid );
				goto cleanup;
			}

			if( j )
				sp->nsp = sp + 1;
			sp++;
		}

		/* alloc map for glist */
		if( !( ecp->mp = rmallocmap ( GL_PER_CTL )))
		{
			/* 
			 *+ can't get map, stop configuring the driver
			 */
			cmn_err( CE_CONT, "egl : can't allocmap \n",
				cp->ssmid );
			goto cleanup;
		}

		/* init map */
		rmfree( ecp->mp, GL_PER_CTL, 1 );

		/* alloc sap pool and ifstats */
		if(!( ucp = ( unchar * )kmem_zalloc ( MEMALO_2_SZ, KM_NOSLEEP )))
		{
			/* 
			 *+ can't get mem, stop configuring the driver
			 */
			cmn_err( CE_CONT, "egl : can't get sap \n",
				cp->ssmid );
			goto cleanup;
		}
		/* LINTED pointer alignment */
		bp->sap_ptr =( DL_sap_t * )ucp;

		ucp += sizeof( DL_sap_t ) * N_EGL_SAPS;
		ucp =( unchar *)(((( uint )ucp + 3 ) / 4 ) * 4 );
	
		/* LINTED pointer alignment */
		bp->ifstats =( struct ifstats * )ucp;

		/* allocate lock */
 		if(!( bp->bd_lock = LOCK_ALLOC( EGL_HIER, plstr, &egl_lockinfo, 
			KM_NOSLEEP)))
		{
			/* 
			 *+ can't get lock, stop configuring the driver
			 */
			cmn_err(CE_PANIC, "egl_init: no memory for egl_lock");
		}

		/* init saps */
		j = N_EGL_SAPS;
		sapp = bp->sap_ptr;
		while( j-- )
		{                    
			sapp->state = DL_UNBOUND;
			sapp->sap_addr = 0;
			SV_INIT( &sapp->sap_sv ) ;
			sapp->read_q = NULL;
			sapp->write_q = NULL;
			sapp->flags = 0;
			sapp->max_spdu = USER_MAX_SIZE;
			sapp->min_spdu = USER_MIN_SIZE;
			sapp->mac_type = DL_ETHER;
			sapp->service_mode = DL_CLDLS;
			sapp->provider_style = DL_STYLE1;;
			sapp->bd = bp;
			sapp++;
		}

		/* init ifstats */
		ifp = bp->ifstats;
		ifp->ifs_name = "egl";
		ifp->ifs_unit =(short)i;
		ifp->ifs_mtu = USER_MAX_SIZE;
		ifp->ifs_active = 1;
		ifp->ifs_next = ifstats;
		ifp->ifs_ipackets = ifp->ifs_opackets = 0;
		ifp->ifs_ierrors = ifp->ifs_oerrors  = 0;
		ifp->ifs_collisions = 0;

		ifstats = ifp;

		/* init DL_config */
		bp->major = cp->egl_major; 
		bp->max_saps = N_EGL_SAPS;
		bp->bd_number = i;
		bp->flags = BOARD_PRESENT;
		bp->timer_val = -1;

		/* config board : still in splhi, no lock needed */
		if( egl_cinit( bp, 0 ))
			continue ;
		
		bp->flags = 0;

cleanup :
		if( siop )
			ssm_vme_mdesc_free( siop );

		if( glp )
			ssm_vme_mdesc_free( glp );

		if( ecp )
		{
			sp = ecp->sp;
			while( sp )
			{
				sp1 = sp->nsp;
				if ( sp->ssp )
					ssm_vme_mdesc_free( sp->ssp );
				sp = sp1;
			}

			if( ecp->mp )
				rmfreemap( ecp->mp );

			kmem_free( bp->bd_dependent1, MEMALO_1_SZ ) ;
		}

		if( bp->sap_ptr )
			kmem_free( bp->sap_ptr, MEMALO_2_SZ ) ;

		ivec_free( egl_bin, egl_base_vec + i );

 		if( bp->bd_lock )
			LOCK_DEALLOC( bp->bd_lock );

		if ( !cont )
			break ;
	}
	return( 0 );
}

/* 
 * void
 * egl_bdspecclose( queue_t *q )
 *  q is pointing to the write queue of the SAP which is doing the close.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description :
 *  There is no hardware specific close procedure for this controller
 *
 */
/* ARGSUSED */
void
egl_bdspecclose( queue_t *q )
{
	return;
}

/* 
 * void
 * egl_bdspecioctl( queue_t *q, mblk_t *mp )
 *  q is pointing to the write queue of the SAP which is doing the ioctl.
 *  mp is pointing to the ioctl message.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description :
 *  There is no hardware specific ioctl procedure for this controller.
 *  Just return M_IOCNAK.
 *
 */
/* ARGSUSED */
void
egl_bdspecioctl( queue_t *q, mblk_t *mp )
{
	/* LINTED pointer alignment */
	struct iocblk	*ioctl_req =(struct iocblk *)mp->b_rptr;

	ioctl_req->ioc_error = EINVAL;
	ioctl_req->ioc_count = 0;
	mp->b_datap->db_type = M_IOCNAK;
}

/*
 * int
 * egl_xmit_packet( DL_bdconfig_t *bd, mblk_t	*mp )
 *  bd is pointing to the controller which is supposed to transmitting the 
 *  packet pointed to by mp.
 *
 * Calling/Exit State:
 *  caller should hold lock bd->lock.
 */
int
egl_xmit_packet( DL_bdconfig_t *bd, mblk_t	*mp )
{
	struct egl_ctl *ep;
	int i;

	/* LINTED pointer alignment */
	ep =( struct egl_ctl * )bd->bd_dependent1;
	i = issue_cmd( bd, XMT_CMD, mp );
	if( i && !ep->toid )
		ep->toid = itimeout( egl_to, ( caddr_t )bd, 
			XMT_TO, plstr );

	if( !i )
		freemsg(mp);

	return( --i );
}

/*
 * void 
 * egl_intr()
 *  controller interrupt service routine
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description :
 *  flush vme dma buffer.
 *  call cmd_done() to service the command finished.
 *  call issue_cmd(), if necessary, to prepare for more incoming packets.
 *  if there is queued outgoing packets, transmit as many as possible.
 *  call itimeout(), if there is any pending transmit command.
*/
STATIC void
egl_intr( int	vector )
{
	DL_bdconfig_t	*bd;
	struct	macis_blk	*mcp;
	ushort	status;
	struct egl_ctl *ep;
	pl_t opri;
	toid_t	toid;

	vector -= egl_base_vec;

	bd = &egl_config[ vector ];
	/* LINTED pointer alignment */
	ep =( struct egl_ctl * )bd->bd_dependent1;

	/* flush dma buffer */
	VME_FLUSH( ep );

	mcp = ep->mcp;

	opri = LOCK( bd->bd_lock, plstr );
	/*
	 * Need to be concerned with the watchdog timeout being rescheduled
	 * during the sequence that we: determine that it should be canceled,
	 * unlock bd_lock, call untimeout() and re-lock bd_lock.  It's possible
	 * for the timeout to fire, reset the board and schedule itself again.
	 */
	while ((toid = ep->toid) > 0) {
		ep->toid = 0;
		UNLOCK(bd->bd_lock, plstr);
		untimeout(toid);
		(void)LOCK(bd->bd_lock, plstr);
	}

	status = mcp->resp.status;
	if(!( status & CMD_DONE ))
	{
		UNLOCK( bd->bd_lock, opri );
		return;
	}

	cmd_done( bd, mcp->resp.cmd_tag, &mcp->r.i );

	/* tell 4207 that we are done */
	mcp->resp.cmd_tag = 0 ;

	/* send more rcv buf to 4207 */
	while( ep->rcv_cnt < RCV_BUF_CNT )
	{
		if( !issue_cmd ( bd, RCV_CMD, NULL ))
			break;
	}
 
	/* xmt whatever left on the queue */
	if( bd->flags & ( TX_QUEUED ))
		xmt_qd_pkg( bd );
	else if( bd->flags & TX_BUSY )
		bd->flags &= ~TX_BUSY;

	if( ep->xmt_cnt )
		ep->toid = itimeout( egl_to, ( caddr_t )bd, XMT_TO, plstr );

	mcp->resp.status = 0;
	UNLOCK( bd->bd_lock, opri );
}

/*
 * int 
 * xmt_qd_pkg( DL_bdconfig_t *bd )
 *  transmit queued outgoing packets
 *
 * Calling/Exit State:
 *  bd->bd_lock should have been held.
 *
 * Description :
 *  this routine will go thru all the SAPs of this controller to
 *  issue XMT_CMD to all the queued outgoing packets.
*/
STATIC void
xmt_qd_pkg( DL_bdconfig_t *bd )
{
	int i;
	struct egl_ctl *ep;
	DL_sap_t *sap, *osap;
	mblk_t *mp;

	/* LINTED pointer alignment */
	ep =( struct egl_ctl *)bd->bd_dependent1;

	if( bd->flags & TX_QUEUED )
	{
		osap = sap = &bd->sap_ptr [ bd->tx_next ];
		i = XMT_BUF_CNT - ep->xmt_cnt;

		while( i )
		{
			if( sap->state == DL_IDLE ) 
			{
				while( i && ( mp = getq( sap->write_q )))
				{
					if( !issue_cmd ( bd, XMT_CMD, mp ))
					{
						if( !putbq ( sap->write_q, mp ))
						{
							freemsg( mp );
							bd->mib.ifOutDiscards++;
							bd->mib.ifOutQlen--;
						}
						goto out;
					}
					i--;
					bd->mib.ifOutQlen--;
				}
				if( !i )
					goto out;
			}

			if( ++bd->tx_next == N_EGL_SAPS )
				bd->tx_next = 0;
	
			sap = &bd->sap_ptr [ bd->tx_next ];
			if( osap == sap )
				break;
		}
	}

out :

	if( ep->xmt_cnt == XMT_BUF_CNT )
		bd->flags |= TX_BUSY;
	else
		bd->flags &= ~TX_BUSY;

	if( bd->mib.ifOutQlen )
		bd->flags |= TX_QUEUED;
	else
		bd->flags &= ~TX_QUEUED;
}

/*
 * int 
 * cmd_done( DL_bdconfig_t *bp, struct cmd_buf *cmdp, union iopb *iop )
 *  Routine to clean up the command finished and update the statistics.
 *
 * Calling/Exit State:
 *  bp->bd_lock should have been held.
 *
 * Description :
 *  The bp points to the board specific structure.  cmdp points to the command
 *  which is just finished.  iop points to the iopb area for status checking.
 *  The basic function of this routine is to update the statistics and return
 *  all resources back to the driver for later use.
*/
STATIC void
cmd_done( DL_bdconfig_t *bp, struct cmd_buf *cmdp, union iopb *iop )
{
	struct	egl_ctl *ecp;
	struct	ssm_e *sp;
	struct	cmd_buf *cp1 ;

	/* LINTED pointer alignment */
	ecp =( struct egl_ctl *)bp->bd_dependent1;

	if(!( cmdp->stat & CF_USED ))
	{
		ecp->nbcnt++ ;

		/* this hardware will, sometimes, post the interrupt while there is no 
	         * completed command.  In such case, the cmd_tag will be the same
		 * as the last completed one.  To get around it, we just call
		 * egl_ck() to make sure it is the case and returns quietly.
		 */
		 
		egl_ck( bp ) ;
		return ;
	}
	switch( cmdp->cmd ) {
	case RCV_CMD :
		bp->ifstats->ifs_ipackets++;

		if( iop->g.stat )
		{
			bp->ifstats->ifs_ierrors++;
			bp->mib.ifInErrors = bp->ifstats->ifs_ierrors;
			bp->mib.ifInDiscards++;
			freemsg( cmdp->mp );
		} else
		{
			bp->mib.ifInOctets += iop->r.tfr_size - FCS_SIZE ;
			cmdp->mp->b_wptr +=( iop->r.tfr_size - FCS_SIZE );
			if( !egl_recv ( cmdp->mp, bp->sap_ptr )) /* receive done */
				bp->mib.ifInOctets += MAC_HDR_LEN + iop->r.tfr_size;
		}
		ecp->rcv_cnt--;
		break;

	case XMT_CMD :
		bp->ifstats->ifs_opackets++;
		if( iop->g.stat )
		{
			bp->ifstats->ifs_oerrors++;
			bp->mib.ifOutErrors = bp->ifstats->ifs_oerrors;
		} else 
		{
			bp->mib.ifOutOctets += cmdp->d_size ;
		}
		ecp->xmt_cnt--;
		freemsg( cmdp->mp );
		break;

	default :
		/*
		 *+ should be only RCV/XMT
		 */
		cmn_err( CE_PANIC, "egl : intr %d cmd %x \n", bp->bd_number,
			cmdp->cmd );
	}
	/* return ssm_e */
	sp = cmdp->sp;
	while( sp->nsp )
		sp = sp->nsp;

	sp->nsp = ecp->sp;
	ecp->sp = cmdp->sp;

	/* free g_list_entry */
	if( cmdp->cmd == XMT_CMD )
		rmfree( ecp->mp, cmdp->g_size, cmdp->g_start );

	/* free cmd_buf */
	if(( cp1 = cmdp->np ) != NULL )
		cp1->pp = cmdp->pp;
	else
		cmdp->pp->np = NULL;

	if(( cp1 = cmdp->pp ) != NULL )
		cp1->np = cmdp->np;
	else
		ecp->ip = cmdp->np;

	cmdp->stat = 0;
	cmdp->np = ecp->fip;
	ecp->fip = cmdp;
}

/*
 * void 
 * egl_to( DL_bdconfig_t *bd )
 *  controller time out routine for transmit.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description :
 *  bd points to the board which just timed out.
 *  egl_cinit() will be called to re-initialize the board.
*/
STATIC void
egl_to( DL_bdconfig_t *bd )
{
	pl_t opri;

	opri = LOCK( bd->bd_lock, plstr );

	/* LINTED pointer alignment */
	(( struct egl_ctl * )( bd->bd_dependent1 ))->toid = 0;
	/* LINTED pointer alignment */
	(( struct egl_ctl * )( bd->bd_dependent1 ))->tocnt++ ;
	
	if( !egl_cinit ( bd, 0 ))
	{
		bd->flags |= BOARD_DISABLED;
		/* 
		 *+ can't reset hardware, mark it as DISABLED.
		 */
		cmn_err( CE_CONT, "egl : board %d will not reset.",
			bd->bd_number );
	}
	UNLOCK( bd->bd_lock, opri );
}

/*
 * int 
 * egl_cinit( DL_bdconfig_t *bp, int resetonly )
 *  controller configuration routine
 *
 * Calling/Exit State:
 *  caller should have lock bp->bd_lock.
 *
 * Description :
 *  bp points to the board that has to be re-configured.  resetonly is the flag
 *  to indicate the caller only wants to reset the board.
 *
*/
STATIC int
egl_cinit( DL_bdconfig_t *bp, int resetonly )
{
	volatile struct	macis_blk	*mcp;
	volatile int i;
	struct	egl_ctl	*ep;
	struct	cmd_buf	*ocmdp, *cmdp;
	struct	ssm_e	*sp;
	struct	init_iopb	*ip;
	struct	init_blk	*ibp;
	unchar *cp, *ccp;
	toid_t	toid;

	/* reset */
	/* LINTED pointer alignment */
	ep =( struct egl_ctl *)bp->bd_dependent1;
	/*
	 * Need to be concerned with the watchdog timeout being rescheduled
	 * during the sequence that we: determine that it should be canceled,
	 * unlock bd_lock, call untimeout() and re-lock bd_lock.  It's possible
	 * for the timeout to fire, reset the board and schedule itself again.
	 */
	while ((toid = ep->toid) > 0) {
		ep->toid = 0;
		UNLOCK(bp->bd_lock, plstr);
		untimeout(toid);
		(void)LOCK(bp->bd_lock, plstr);
	}

	mcp = ep->mcp;

	mcp->stat = 0;
	mcp->ctl = BD_RESET | SET_SYSFAIL; 

	drv_usecwait( 200 ) ;

	mcp->ctl = 0;

	i = 10000 ;
	while ( i-- )
	{
		drv_usecwait( 1000 ) ;
		if(( mcp->stat & ( BOARD_OK | CTLR_NOT_AVAIL )) == BOARD_OK )
			break ;
	}

	if(( mcp->stat & ( BOARD_OK | CTLR_NOT_AVAIL )) != BOARD_OK )
	{
		cmn_err ( CE_CONT, "egl : reset failed %x \n", mcp->stat ) ;
		return( 0 );
	}

	if(( mcp->stat & ( BOARD_OK | CTLR_NOT_AVAIL )) != BOARD_OK )
	{
		return( 0 );
	}

	/* clean up */
	cmdp = ep->ip;

	while( cmdp )
	{
		switch( cmdp->cmd ) {
		case XMT_CMD :
			/* free g-list */
			rmfree( ep->mp, cmdp->g_size, cmdp->g_start );
			/* FALLTHRU */

		case RCV_CMD :
			/* free msg */
			freemsg( cmdp->mp );

			/* free ssm_e */
			sp = cmdp->sp;
			while( sp )
			{
				if( !sp->nsp )
					break;
				sp = sp->nsp;
			}
			sp->nsp = ep->sp;
			ep->sp = cmdp->sp;
			break;

		default :
			/*
			 *+ software error
			 */
			cmn_err( CE_PANIC, "egl : wrong cmd in config %x \n",
				cmdp->cmd );
		}

		/* free cmd_buf */
		ocmdp = cmdp;
		ocmdp->stat = 0;
		cmdp = cmdp->np;
		ocmdp->np = ep->fip;
		ep->fip = ocmdp;
	}
	ep->next_cmd = ep->rcv_cnt = ep->xmt_cnt = 0;
	ep->ip = NULL;

	if( resetonly )
		return( 1 );

	ip =( struct init_iopb * )&ep->iop;
	ibp = &ep->icb;
	mcp = ep->mcp;

	bzero( ip, sizeof ( *ip ));

	ip->cmd = INIT_CMD;
	ip->ib_offset =(ulong)( &mcp->ib ) - (ulong)( mcp );

	if( !ibp->cmd_q_size )
	{
		/* system boot up time */
		ibp->cmd_q_size = CMD_Q_SZ;
		ibp->nw_opt = NW_ETHER;

		cp =( unchar * )&mcp->cf_stat.p_addr;
		ccp =( unchar * )&bp->eaddr;
		ccp[1] = ibp->eaddr[0] = *cp++;		/* bytes are reversed */
		ccp[0] = ibp->eaddr[1] = *cp++;
		ccp[3] = ibp->eaddr1[2] = *cp++;
		ccp[2] = ibp->eaddr1[3] = *cp++;
		ccp[5] = ibp->eaddr1[0] = *cp++;
		ccp[4] = ibp->eaddr1[1] = *cp++;

		p_dot( "Ethernet Address ", ( unchar *)&bp->eaddr, 6 );
 
		ibp->dma_burst_cnt = DMA_BURST;
	}

	/* LINTED pointer alignment */
	egl_lcopy(( ulong *)ibp, ( ulong *)&mcp->ib, sizeof ( *ibp ));
	egl_lcopy(( ulong *)ip, ( ulong *)mcp->iopb, sizeof ( *ip ));

	mcp->iopb_add =( ulong )mcp->iopb - ( ulong )mcp;

	mcp->cmd_ctl = CMD_GO;

	i = INIT_TO;
	while( i-- && !( mcp->resp.status & CMD_DONE ))
		;

	if(!( mcp->resp.status & CMD_DONE ))
	{
		/*
		 *+ board has no response
		 */
		cmn_err( CE_WARN, "egl : config init timeout \n");
		return( 0 );
	}

	if( mcp->r.i.g.stat )
	{
		/*
		 *+ config failed
		 */
		cmn_err( CE_WARN, "egl : config init failed %x \n",
			mcp->r.i.g.stat );
		return( 0 );
	}

	mcp->resp.cmd_tag = 0 ;
	mcp->resp.status = 0;
		
	/* start Q-MODE */
	mcp->ctl |= ST_Q_MOD;

	i = INIT_TO;
	while( i-- && !( mcp->resp.status & CMD_DONE ))
		;

	if(!( mcp->resp.status & CMD_DONE ))
	{
		/*
		 *+ can't start q mode 
		 */
		cmn_err( CE_WARN, "egl : config qmode timeout \n");
		return( 0 );
	}

	if(!( mcp->resp.status & Q_MOD_ST ))
	{
		/*
		 *+ qmode start failed
		 */
		cmn_err( CE_WARN, "egl : cinit can't start qmode %d \n", 
			bp->bd_number );
		return( 0 );
	}

	mcp->resp.cmd_tag = 0 ;
	mcp->resp.status = 0;

	i = RCV_BUF_CNT;
	while( i-- )
	{
		if( !issue_cmd ( bp, RCV_CMD, NULL ))
			break;
	}
  
	/* xmt whatever left on the queue */
	if( bp->flags & ( TX_QUEUED ))
		xmt_qd_pkg( bp );
	else if( bp->flags & TX_BUSY )
		bp->flags &= ~TX_BUSY;

	if( ep->xmt_cnt )
		ep->toid = itimeout( egl_to, ( caddr_t )bp, XMT_TO, plstr );

	return( 1 );
}

/*
 * int 
 * issue_cmd( DL_bdconfig_t *bp, int cmd, mblk_t *mp )
 *  issue RCV/XMT commands to the hardware.
 *
 * Calling/Exit State:
 *  caller should have lock bp->lock.
 *
 * Description :
 *  bp points to the board to which the command will be issued.  cmd is the
 *  command, it is either XMT_CMD or RCV_CMD.  mp points to the mblk_t that
 *  needs to be transmit or the area for receiving packet.
*/
STATIC int
issue_cmd( DL_bdconfig_t *bp, int cmd, mblk_t *mp )
{
	struct egl_ctl *ep;
	union iopb	*ip;
	struct	cmd_buf	*iop;
	struct	ssm_e	*osp, *sp;
	mblk_t	*mpp;
	struct g_list_entry	*gp;
	paddr_t	paddr;
	struct	cmd_q_entry	*cqp;
	int i,j ;

	/* LINTED pointer alignment */
	ep =( struct egl_ctl * )bp->bd_dependent1;

	/* flush dma buffer */
	VME_FLUSH( ep );
		
	if( ep->mcp->cmd[ ep->next_cmd ].ctl & CMD_GO )
	{
		return( 0 );	/* hardware malfunction ??? */
	}

	/* in case reset in between */
	if(( bp->flags & ( BOARD_DISABLED | BOARD_PRESENT )) != BOARD_PRESENT )
	{
		return( 0 );
	}

	if( !ep->fip || !ep->sp)
	{
		return( 0 );
	}

	iop = ep->fip;
	ep->fip = iop->np;

	ip = &ep->iop;
	bzero( ip, sizeof ( *ip ));

	iop->cmd = ip->r.cmd =( ushort )cmd;
	ip->r.stat = 0;
	ip->r.n_vec = ip->r.e_vec = ep->vec;
	ip->r.n_lvl = ip->r.e_lvl = ep->lvl;

	switch( cmd ) {
	case RCV_CMD :
		
		/* get buffer for RCV */
		if(!( mp = allocb( RCV_BUF_SIZE, 0 )))
		{
			iop->np = ep->fip;
			ep->fip = iop;
			return( 0 );
		}

		iop->sp = sp = ep->sp;
		ep->sp = sp->nsp;
		sp->nsp = NULL;

		/* map buffer address to vme address */
		if(( paddr = ( paddr_t )ssm_v2s_remap ( sp->ssp, 
			( caddr_t )mp->b_datap->db_base, RCV_BUF_SIZE)) < 0 )
		{
			freemsg( mp );
			sp->nsp = ep->sp;
			ep->sp = sp;
			iop->np = ep->fip;
			ep->fip = iop; 
			return( 0 );
		}

		iop->mp = mp;
	
		ip->r.cmdo = INT_ENABLE | DMA_ENABLE;
		ip->r.vme_opt = VME_BLK_XFR | VME_MEM_32 | VME_AM24_BLK | VME_DMA_READ;
		ip->r.tfr_size = RCV_BUF_SIZE;
		ip->r.vme_add = paddr;

		ep->rcv_cnt++;

		break;

	case XMT_CMD :
		
		/* calculate g_list */
		mpp = mp;
		i = 0;
		while( mpp )
		{
			if ((mpp->b_wptr - mpp->b_rptr) <= 32) {
				if ((mpp = msgpullup(mp, -1)) != NULL) {
					freemsg(mp);
					mp = mpp;
					i = 1;
					break;
				} else
				{
					iop->np = ep->fip ;
					ep->fip = iop;
					return (0);
				}
			}
			i++;
			mpp = mpp->b_cont;
		}
		iop->g_size = i;

		/* check # of ssm_e's */
		sp = ep->sp;
		while( i-- && sp )
		{
			osp = sp;
			sp = sp->nsp;
		}

		if( i >= 0 )
		{
			iop->np = ep->fip;
			ep->fip = iop; 
			return( 0 );
		}
		
		/* alloc ssm_e */
		iop->mp = mp;
		iop->sp = ep->sp;
		ep->sp = sp;
		osp->nsp = NULL;

		/* alloc gather list area */
		if(!( iop->g_start = rmalloc ( ep->mp, iop->g_size )))
		{	
			iop->np = ep->fip;
			ep->fip = iop; 
			osp->nsp = sp;
			ep->sp = iop->sp;
			return( 0 );
		}

		/* map map-ram */
		gp =( struct g_list_entry *)( ep->g_l_vme + 
			(( ulong )iop->g_start - 1 )* sizeof( struct g_list_entry ));

		i = iop->g_size;
		iop->d_size = 0;
		mpp = mp;
		while( mpp )
		{
			j = mpp->b_wptr - mpp->b_rptr;
			if(( gp->vmeadd = ( ulong )ssm_v2s_remap ( sp->ssp, 
				( caddr_t )mpp->b_rptr, j )) < 0 )
			{
				iop->np = ep->fip;
				ep->fip = iop; 
				osp->nsp = ep->sp;
				ep->sp = iop->sp;
				rmfree( ep->mp, iop->g_size, iop->g_start );
				return( 0 );
			}
			gp->vmeopt = VME_BLK_XFR | VME_MEM_32 | VME_AM24_BLK;
			gp->xfrcnt =( ushort )j;
			iop->d_size += j;

			mpp = mpp->b_cont;
			gp++;
			sp = sp->nsp;
		}

		ip->x.cmdo = INT_ENABLE | GATHER_ENABLE | DMA_ENABLE;
		ip->x.vme_opt = VME_BLK_XFR | VME_MEM_32 | VME_AM24_BLK;

		ip->x.vme_add = G_LIST_OFFSET +(( ulong )iop->g_start - 1 ) * 
			sizeof( struct g_list_entry );
		ip->x.dma_size = iop->d_size;
		ip->x.sg_cnt = iop->g_size;

		if( ++ep->xmt_cnt == XMT_BUF_CNT )
			bp->flags |= TX_BUSY;

		bp->mib.ifOutOctets += iop->d_size + MAC_HDR_LEN;	/* SNMP */

		break;
	}

	/* Q cmd_buf to head of Q */
	if(( iop->np = ep->ip ) != NULL )
		ep->ip->pp = iop;
	iop->pp = NULL;
	ep->ip = iop;

	/* setup iopb */
	egl_lcopy(( ulong *)ip, ( ulong *)&ep->mcp->iopb[ ep->next_cmd ], 
		sizeof ( *ip ));

	/* setup cmd Q entry */
	cqp = &ep->mcp->cmd[ ep->next_cmd ];
	cqp->q_num =( cmd == RCV_CMD ) ? Q_RCV : Q_XMT;
	cqp->cmd_tag = iop;
	cqp->iopb_addr =( ulong )&ep->mcp->iopb[ ep->next_cmd ] - 
		( ulong )ep->mcp;

	cqp->ctl = CMD_GO;

	if( ++(ep->next_cmd) == CMD_Q_SZ )
		ep->next_cmd = 0 ;

	iop->stat = CF_USED;
	return( 1 );
}

/*
 * int 
 * egl_promisc_on( DL_bdconfig_t	*bd )
 *  routine to turn on the promiscuous mode.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description :
 *  bd points to the board which the caller wants to have the promiscuous 
 *  mode turned on.  For this hardware, it will call egl_cinit(), if 
 * necessary, to re-configure the board.
 */
int
egl_promisc_on( DL_bdconfig_t	*bd )
{
	struct egl_ctl *ep;
	pl_t opri;

	/* LINTED pointer alignment */
	ep =( struct egl_ctl * )bd->bd_dependent1;
	opri = LOCK( bd->bd_lock, plstr );
	if(bd->promisc_cnt++)
	{
		UNLOCK( bd->bd_lock, opri );
		return(0);
	}

	ep->icb.nw_opt |= PROMIS_MODE;
	if( !egl_cinit ( bd, 0 ))
	{
		bd->flags |= BOARD_DISABLED;
		UNLOCK( bd->bd_lock, opri );
		return(1);
	}
	UNLOCK( bd->bd_lock, opri );
	return( 0 );	/* ok */
}

/*
 * int 
 * egl_promisc_off( DL_bdconfig_t	*bd )
 *  routine to turn off the promiscuous mode.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description :
 *  bd points to the board which the caller wants to have the 
 *  promiscuous mode turned off.  For this hardware, it will call 
 *  egl_cinit(), if necessary, to re-configure the board.
 */
int
egl_promisc_off( DL_bdconfig_t	*bd )
{
	struct egl_ctl *ep;
	int		opri;

	/* LINTED pointer alignment */
	ep =( struct egl_ctl * )bd->bd_dependent1;
	opri = LOCK( bd->bd_lock, plstr );
	
	if( !bd->promisc_cnt || --bd->promisc_cnt )
	{
		UNLOCK( bd->bd_lock, opri );
		return(0);
	}

	ep->icb.nw_opt &= ~PROMIS_MODE;

	if( !egl_cinit ( bd, 0 ))
	{
		bd->flags |= BOARD_DISABLED;
		UNLOCK( bd->bd_lock, opri );
		return(1);
	}

	UNLOCK( bd->bd_lock, opri );
	return(0);
}

/*
 * int 
 * issue_poll_cmd( DL_bdconfig_t *bd, int cmd, unchar *ccp )
 *  routine to do polling on a command.
 *
 * Calling/Exit State:
 *  caller should have lock bd->bd_lock.
 *
 * Description :
 *  bd points to the board which the caller wants to perform polling on the
 *  command indicated by the argument cmd.  ccp points to the command specific
 *  data. 
 */
STATIC int
issue_poll_cmd( DL_bdconfig_t *bd, int cmd, unchar *ccp )
{
	struct egl_ctl *ep;
	struct cadd_iopb *cp;
	struct laf_iopb *lap;
	struct macis_blk *mcp;
	short once, s;
	long i;
	toid_t	toid;

	/* LINTED pointer alignment */
	ep =( struct egl_ctl * )bd->bd_dependent1;
	/*
	 * Need to be concerned with the watchdog timeout being rescheduled
	 * during the sequence that we: determine that it should be canceled,
	 * unlock bd_lock, call untimeout() and re-lock bd_lock.  It's possible
	 * for the timeout to fire, reset the board and schedule itself again.
	 */
	while ((toid = ep->toid) > 0) {
		ep->toid = 0;
		UNLOCK(bd->bd_lock, plstr);
		untimeout(toid);
		(void)LOCK(bd->bd_lock, plstr);
	}

	mcp = ep->mcp;
	bzero( &ep->iop, sizeof ( struct iopb ));

	if( cmd == CADD_CMD )
	{
		/* change physical address cmd */
		cp =( struct cadd_iopb * )&ep->iop;
		cp->cmd = CADD_CMD;
		cp->cmdo = REINIT_LANCE;

		cp->p_addr[3] = *ccp++;
		cp->p_addr[2] = *ccp++;
		cp->p_addr[1] = *ccp++;
		cp->p_addr[0] = *ccp++;
		cp->p_addr1[1] = *ccp++;
		cp->p_addr1[0] = *ccp++;
	} else 
	{
		/* change logical address filter */
		lap =( struct laf_iopb * )&ep->iop;
		lap->cmd = LAF_CMD;
		lap->cmdo = REINIT_LANCE;

		bcopy( ccp, lap->laf, LAF_LEN );
	}

	once = 0;
	/* issue cmd */
	egl_lcopy(( ulong *)&ep->iop, ( ulong *)&mcp->iopb[ ep->next_cmd ], 
		sizeof ( struct iopb ));

	mcp->cmd_tag =( struct cmd_buf * )( POLLTAG | cmd );
	mcp->iopb_add =( ulong )&mcp->iopb[ ep->next_cmd ] - ( ulong )mcp;
	mcp->cmd_ctl = CMD_GO;

tryonemore :

	i = INIT_TO;
	while( i-- && !( mcp->resp.status & CMD_DONE ))
		;

	if(!( mcp->resp.status & CMD_DONE ))
	{
		return( egl_cinit( bd, 0 ));
	}

	if( mcp->resp.cmd_tag != ( struct cmd_buf * )( POLLTAG | cmd ))
	{
		if( once )
		{
			/*
			 *+ hardware error
			 */
			cmn_err( CE_PANIC, " egl : too many intr in polling mode\n");
		}

		once++;
		if( mcp->resp.cmd_tag )
			cmd_done( bd, mcp->resp.cmd_tag, &mcp->r.i );

		mcp->resp.cmd_tag = 0 ;
		mcp->resp.status = 0;
		
		goto tryonemore;
	}

	s = mcp->r.i.g.stat ;

	mcp->resp.cmd_tag = 0 ;
	mcp->resp.status = 0;

	if( ep->xmt_cnt )
		ep->toid = itimeout( egl_to, ( caddr_t )bd, XMT_TO, plstr );

	return( s ? 0 : 1 );
}

/*
 * int 
 * egl_set_eaddr( DL_bdconfig_t	*bd, DL_eaddr_t	*eaddr )
 *  routine to set physical address for the controller.
 *
 * Calling/Exit State:
 *  caller should have lock bd->bd_lock.
 *
 * Description :
 *  bd points to the board to which the caller wants to change the physical
 *  address to the address pointed to by argument eaddr.
 *  This command will be issued in polling mode.
 */
int
egl_set_eaddr( DL_bdconfig_t	*bd, DL_eaddr_t	*eaddr )
{
	struct egl_ctl *ep;
	int		i, opri;
	unchar *cp;

	/* LINTED pointer alignment */
	ep =( struct egl_ctl * )bd->bd_dependent1;

	cp =( unchar * )eaddr;
	ep->icb.eaddr[1] = *cp++;
	ep->icb.eaddr[0] = *cp++;
	ep->icb.eaddr1[3] = *cp++;
	ep->icb.eaddr1[2] = *cp++;
	ep->icb.eaddr1[1] = *cp++;
	ep->icb.eaddr1[0] = *cp++;

	bcopy( eaddr, ( caddr_t )&bd->eaddr, DL_MAC_ADDR_LEN );

	opri = LOCK( bd->bd_lock, plstr );

	if(( bd->flags & ( BOARD_DISABLED | BOARD_PRESENT )) == BOARD_PRESENT )
		i = issue_poll_cmd( bd, CADD_CMD, ( unchar *)eaddr );
	else
		i = 1;

	UNLOCK( bd->bd_lock, opri );
	return( i ? 0 : 1 );
}

/*
 * int 
 * egl_get_multicast( DL_bdconfig_t *bd, mblk_t *mp )
 *  routine to return multicast addresses back to user.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description :
 *  bd points to the board from which the caller wants to get the multicast 
 *  addresses.
 */
int
egl_get_multicast( DL_bdconfig_t *bd, mblk_t *mp )
{
	mcat_t  *mcp;
	unsigned char *dp;
	int i, found = 0;
	struct egl_ctl *ep;
	pl_t opri;

	/* LINTED pointer alignment */
	ep =( struct egl_ctl * )bd->bd_dependent1;
	opri = LOCK( bd->bd_lock, plstr );
	mcp = ep->egl_multiaddr;

	if(!( mp->b_wptr - mp->b_rptr ))
		found = bd->multicast_cnt;
	else 
	{
		dp = mp->b_rptr;
		i = MULTI_ADDR_CNT;
		while( i-- && ( dp < mp->b_wptr))
		{
			if( mcp->status ) 
			{
				bcopy( mcp->entry, dp, DL_MAC_ADDR_LEN );
				dp += DL_MAC_ADDR_LEN;
				found++;
			}
			mcp++;
		}
		mp->b_wptr = dp;
	}
	UNLOCK( bd->bd_lock, opri );
	return( found );
}

/*
 * int 
 * egl_set_multicast( DL_bdconfig_t *bd )
 *  routine to set the multicast addresses to the controller.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description :
 *  bd points to the board to which the caller wants to set the multicast 
 *  addresses.
 *  This command will be issued in polling mode.
 */
STATIC int
egl_set_multicast( DL_bdconfig_t *bd )
{
	ulong c, crc;
	unchar	claf[8], *cp;
	ushort i,j,len;
	ushort  *sp, laf[4];
	mcat_t *mcp;
	struct egl_ctl *ep;

	/* LINTED pointer alignment */
	ep =( struct egl_ctl * )bd->bd_dependent1;
	mcp = ep->egl_multiaddr;

	laf[0] = laf[1] = laf[2] = laf[3] = 0;
	i = MULTI_ADDR_CNT;
	while( i-- )
	{
		/* LINTED pointer alignment */
		crc = 0xffffffff;
		if( mcp->status ) 
		{
			cp = mcp->entry;
			len = 6;
			while( len-- )
			{
				c = *cp++;
				j = 8;
				while( j-- )
				{
					if(( c & 1 ) ^ ( crc & 1 ))
					{
						crc >>= 1;
						crc = crc ^ EGL_POLY;
					} else
						crc >>= 1;
					c >>= 1;
				}
			}
			crc >>= 26;
			laf[ crc >> 4 ] |= 1 <<( crc & 0xf );
		}
		mcp++;
	}

	i = 4;
	sp = laf;
	while( i-- )
	{
		*sp =((*sp) >> 8 ) & 0xff | ((*sp) & 0xff ) << 8;
		sp++;
	}

	cp =( unchar * )laf;

	claf[3] = *cp++;
	claf[2] = *cp++;
	claf[1] = *cp++;
	claf[0] = *cp++;
	claf[7] = *cp++;
	claf[6] = *cp++;
	claf[5] = *cp++;
	claf[4] = *cp++;

	if( !bcmp (( char * )claf, ( char * )ep->icb.laf, LAF_LEN ))
		return( 1 );

	bcopy( claf, ep->icb.laf, LAF_LEN );

	if(( bd->flags & ( BOARD_DISABLED | BOARD_PRESENT )) == BOARD_PRESENT )
		return( issue_poll_cmd ( bd, LAF_CMD, claf ));
	return( 1 );
}

/*
 * int 
 * egl_add_multicast( DL_bdconfig_t	*bd, DL_eaddr_t	*maddr )
 *  routine to add a multicast address to the controller.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description :
 *  bd points to the board to which the caller wants to add the multicast 
 *  address pointed to by the argument maddr.
 */
int
egl_add_multicast( DL_bdconfig_t	*bd, DL_eaddr_t	*maddr )
{
	mcat_t  *mcp;
	int i, found;
	struct egl_ctl *ep;
	pl_t opri;

	/* LINTED pointer alignment */
	ep =( struct egl_ctl * )bd->bd_dependent1;
	opri = LOCK( bd->bd_lock, plstr );

	/* make sure we have space and address is a multicast address */
	if(( bd->multicast_cnt >= MULTI_ADDR_CNT ) || !( maddr->bytes[0] & 0x1 ))
	{
		UNLOCK( bd->bd_lock, opri );
		return( 1 );
	}

	/* ck for dup */
	found = 0;
	i = MULTI_ADDR_CNT;
	mcp = ep->egl_multiaddr;
	while( i-- )
	{
		if(( mcp->status ) && ( !bcmp(( char * )maddr->bytes, 
			( char * )mcp->entry, DL_MAC_ADDR_LEN )))
		{
			found++;
			break;
		}
		mcp++;
	}

	if( !found )
	{
		/* find empty slot */
		mcp = ep->egl_multiaddr;
		i = MULTI_ADDR_CNT;
		while( i-- )
		{
			if( !mcp->status )
			{
				found++;
				break;
			}
			mcp++;
		}

		/* update the empty slot */
		if( found )
		{
			mcp->status++;
			bd->multicast_cnt++;
			bcopy((caddr_t)maddr->bytes, (caddr_t)mcp->entry, DL_MAC_ADDR_LEN );
			i = egl_set_multicast( bd );
		} else
			i = 0;
	} else
		i = 1;
	UNLOCK( bd->bd_lock, opri );
	return( !i );
}

/*
 * int 
 * egl_del_multicast( DL_bdconfig_t	*bd, DL_eaddr_t	*maddr ) 
 *  routine to delete a multicast address to the controller.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description :
 *  bd points to the board to which the caller wants to delete the 
 *  multicast address pointed to by the argument maddr.
 */
int
egl_del_multicast( DL_bdconfig_t	*bd, DL_eaddr_t	*maddr ) 
{
	int found, i;
	mcat_t  *mcp;
	struct egl_ctl *ep;
	pl_t opri;

	found = 0;
	/* LINTED pointer alignment */
	ep =( struct egl_ctl * )bd->bd_dependent1;
	mcp = ep->egl_multiaddr;
	opri = LOCK( bd->bd_lock, plstr );

	/* find the one to be deleted */
	i = MULTI_ADDR_CNT;
	while( i-- )
	{
		if(( mcp->status ) && ( !bcmp(( char *)maddr->bytes, 
			( char *)mcp->entry, ( size_t )DL_MAC_ADDR_LEN )))
		{
			found++;
			break;
		}
		mcp++;
	}

	if( found )
	{
		/* delete it */
		mcp->status = 0;
		bd->multicast_cnt--;
		i = egl_set_multicast( bd );
	} else
		i = 1;

	UNLOCK( bd->bd_lock, opri );
	return( !i );
}

/*
 * int 
 * egl_disable( DL_bdconfig_t	*bd )
 *  routine to disable the controller.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description :
 *  bd points to the board which the caller wants to disable.
 *  It will call egl_cinit() with the resetonly flag set to 1.
 */
int
egl_disable( DL_bdconfig_t	*bd )
{
	pl_t opri;
	int i;

	opri = LOCK( bd->bd_lock, plstr );

	/* LINTED pointer alignment */
	i = egl_cinit( bd, 1 );
	bd->flags |= BOARD_DISABLED;
	UNLOCK( bd->bd_lock, opri );
	return( !i );
}

/*
 * int 
 * egl_enable( DL_bdconfig_t	*bd )
 *  routine to enable the controller.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description :
 *  bd points to the board which the caller wants to enable.
 *  It will call egl_cinit() with the resetonly flag set to 0.
 */
int
egl_enable( DL_bdconfig_t	*bd )
{
	pl_t opri;
	int i;

	opri = LOCK( bd->bd_lock, plstr );

	bd->flags &= ~BOARD_DISABLED;

	/* LINTED pointer alignment */
	i = egl_cinit( bd, 0 );
	if( i )
		bd->flags &= ~BOARD_DISABLED;

	UNLOCK( bd->bd_lock, opri );
	return( !i );
}

/*
 * int 
 * egl_reset( DL_bdconfig_t	*bd )
 *  routine to reset the controller.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description :
 *  bd points to the board which the caller wants to reset.
 *  It will call egl_cinit() with the resetonly flag set to 0.
 */
int
egl_reset( DL_bdconfig_t	*bd )
{
	pl_t opri;
	int i;

	opri = LOCK( bd->bd_lock, plstr );
	bd->flags &= ~BOARD_DISABLED;
	i = egl_cinit( bd, 0 );
	if( !i )
		bd->flags |= BOARD_DISABLED;

	UNLOCK( bd->bd_lock, opri );
	return( !i );
}

/*
 * int 
 * egl_is_multicast( DL_bdconfig_t	*bd, DL_eaddr_t	*eaddr )
 *  routine to check if a multicast address has been set to the controller.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description :
 *  bd points to the board against which the caller wants to check the multicast
 *  address pointed to by argument eaddr. 
*/
int
egl_is_multicast( DL_bdconfig_t	*bd, DL_eaddr_t	*eaddr )
{
	mcat_t *mcp;
	int rval, i;
	struct egl_ctl *ep;

	/* LINTED pointer alignment */
	ep =( struct egl_ctl * )bd->bd_dependent1;
	mcp = ep->egl_multiaddr;
	rval = 0;

	i = MULTI_ADDR_CNT;
	if( bd->multicast_cnt ) 
	{
		while( i-- )
		{
			if( mcp->status && !bcmp(( char *)eaddr->bytes, 
				( char *)mcp->entry, ( size_t )DL_MAC_ADDR_LEN ))
			{
				rval++;
				break;
			}
			mcp++;
		}
	}
	return(rval);
}

/*
 * int
 * egl_probe( struct macis_blk *mp )
 *  to check if the board is present. 
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 */
STATIC int
egl_probe( volatile struct macis_blk *mp )
{
	/* LINTED hardware needs this */
	volatile int i ;

	i = mp->stat ;	/* Don't change this line */
 
	return( 1 );
}

/*
 * void
 * egl_lcopy( ulong *fp, ulong *tp, int s )
 *  copy in long from seq to eagle board for better performance. 
 *
 * Calling/Exit State:
 *      No locking assumptions.
 */
STATIC void
egl_lcopy( ulong *fp, ulong *tp, int s )
{
	s /= 4;
	while( s-- )
		*tp++ = *fp++;
}

/*
 * void
 * p_dot( char *s, unchar *cp, int i )
 *  print out buffer in hex format. 
 *
 * Calling/Exit State:
 *      No locking assumptions.
 */
STATIC void
p_dot( char *s, unchar *cp, int i )
{
	char q[30];
	unchar	c, *tp;
	int j;

	tp =( unchar * )q;

	while( i-- )
	{
		j = 2;
		while( j-- )
		{
			if( j )
				c =( *cp >> 4 ) & 0xf;
			else
				c = *cp & 0xf;

			if( c < 10 )
				*tp++ = c + '0';
			else
				*tp++ = c + 'a' - 10;
		}
		cp++;
		if( i )
			*tp++ = '.';
	}
	*tp++ = 0;
	/*
	 *+ print data out
	 */
	cmn_err( CE_CONT, "%s %s\n", s, q );
}

unchar	glused[ GL_PER_CTL ];
/*
 * void
 * egl_ck( DL_bdconfig_t *bp )
 *  go thru the egl_ctl to see if everything is ok. 
 *
 * Calling/Exit State:
 *  caller should have lock.
 *
 */
STATIC void
egl_ck( DL_bdconfig_t *bp )
{
	struct egl_ctl *ep;
	int scnt, rcnt, xcnt, i, k;
	struct cmd_buf	*cbp;
	struct ssm_e	*sp;
	struct map *mp;
	unchar	*ucp;

 	ucp = glused;
	i = GL_PER_CTL;
	while( i-- )
		*ucp++ = 0;
	
	/* LINTED pointer alignment */
	ep = ( struct egl_ctl *)bp->bd_dependent1;
	scnt = 0;

	mp = ep->mp + 1;
	i = GL_PER_CTL;
	while( i-- )
	{
		if(( k = mp->m_size ) != 0 )
		{
			ucp = &glused [ mp->m_addr - 1 ];
			while( k-- )
			{
				if( *ucp )
				{
					/*
					 *+ map overlap
					 */
					cmn_err( CE_PANIC, "egl map s %d start %d i %d \n",  
						mp->m_size, mp->m_addr, i );
				}
				*ucp++ = 1;
			}
		}
		mp++;
	}

	i = 0;
	cbp = ep->fip;
	while( cbp )
	{
		ASSERT( !cbp->stat );
		i++;
		if( i > CMD_Q_SZ )
		{
			/* 
			 *+ too many cmd_buf
			 */
			cmn_err( CE_PANIC, "egl : cmd cnt %d \n", i );
		}
		cbp = cbp->np;
	}
 
	rcnt = ep->rcv_cnt;
	xcnt = ep->xmt_cnt;
	
	if( rcnt + xcnt + i != CMD_Q_SZ )
	{
		/*
		 *+ cmd_buf number should be CMD_Q_SZ
		 */
		cmn_err( CE_PANIC, "egl : xmt %d rcv %d free %d \n",
			xcnt, rcnt, i );
	}

	sp = ep->sp;
	while( sp )
	{
		scnt++;
		if( scnt > SSME_PER_CTL )
		{
			/*
			 *+ too many mdesc
			 */
			cmn_err( CE_PANIC, "egl : ssme cnt %d \n", scnt );
		}
		sp = sp->nsp;
	}

	cbp = ep->ip;
	while( cbp )
	{
		ASSERT( cbp->stat & CF_USED );
		ASSERT( cbp->mp->b_datap->db_ref == 1 );
		if( cbp->cmd == RCV_CMD )
		{
			rcnt--;
		}
		else
		{
			xcnt--;
			ucp = &glused[ cbp->g_start - 1 ];
			i = cbp->g_size;
			while( i-- )
			{
				if( *ucp )
				{
					/* 
					 *+ map overlap
					 */
					cmn_err( CE_PANIC, "egl g_list dup %d i %d \n",  
						cbp->g_start, i );
				}
				*ucp++ = 1;
			}
		}

		sp = cbp->sp;
		while( sp ) 
		{
			sp = sp->nsp;
			scnt++;
			if( scnt > SSME_PER_CTL )
			{
				/*
				 *+ too many mdesc
				 */
				cmn_err( CE_PANIC, "egl : ssme cntt %d \n", scnt );
			}
		}
		cbp = cbp->np;
	}

	if( xcnt || rcnt || ( scnt != SSME_PER_CTL ))
	{
		/*
		 *+ number of mdesc should be SSME_PER_CTL
		 */
		cmn_err( CE_PANIC, "egl R %d X %d S %d \n", rcnt, xcnt, scnt );
	}

	ucp = glused;
	i = GL_PER_CTL;
	while( i-- )
	{
		if( !*ucp )
		{
			/* 
			 *+ map has holes
			 */
			cmn_err( CE_PANIC, "egl, lost glist %d \n", i );
		}
		ucp++;
	}
}
