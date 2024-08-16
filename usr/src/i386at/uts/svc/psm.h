/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_PSM_H	/* wrapper symbol for kernel use */
#define _SVC_PSM_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:svc/psm.h	1.12"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/emask.h>
#include <util/types.h>		/* REQUIRED */
#include <util/dl.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/emask.h>
#include <sys/types.h>		/* REQUIRED */
#include <sys/dl.h>		/* REQUIRED */

#endif  /* _KERNEL_HEADERS */


#ifdef _KERNEL

typedef enum led_request {
	LED_OFF, LED_ON
} led_request_t;

typedef struct {
	unsigned long pt_lo;
	unsigned long pt_hi;
} psmtime_t;

#define ONLINE_LED      (1 << 0)
#define ACTIVE_LED      (1 << 1)

struct engine;

extern int xclock_pending;
extern int prf_pending;
extern uchar_t kvpage0[];

extern void	psm_pstart(void);
extern int	psm_doarg(char *);
extern void	psm_reboot(int);
extern struct idt_init *psm_idt(int);
extern void	psm_intr_init(void);
extern void	psm_intr_start(void);
extern int	psm_numeng(void);
extern void	psm_configure(void);
extern void	psm_online_engine(int, paddr_t, int);
extern void	psm_selfinit(void);
extern void	psm_misc_init(void);
extern void	psm_offline_self(void);
extern void	psm_clear_xintr(int);
extern void	psm_send_xintr(int);
extern void	psm_sendsoft(int, uint_t);
extern void	psm_intron(int, pl_t, int, int, int);
extern void	psm_introff(int, pl_t, int, int);
extern void	psm_timer_init(void);
extern void	psm_time_get(psmtime_t *);
extern void	psm_time_add(psmtime_t *, psmtime_t *);
extern void	psm_time_sub(psmtime_t *, psmtime_t *);
extern void	psm_time_cvt(dl_t *, psmtime_t *);
extern void	psm_ledctl(led_request_t, uint_t);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_PSM_H */
