/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/debug.c	1.8"
#ident	"$Header: $"

#include <mem/vmparam.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * Tables for the debug_help() function.
 *
 * These provide information on the various debugging tools in the kernel.
 * (These are functions which can be called from a kernel debugger to
 * assist in debugging or statistics gathering.)
 *
 * The debug_help() function itself is called from a kernel debugger to
 * print out some or all of this information.
 *
 * The table does not need to be initialized in any order; it will be sorted
 * at run time.
 */

static struct dbgtool_info dbgtools[] = {
	{ "debug_help", "keyword",
	  "Print all debug tool function descriptions which contain keyword",
	  "If the string, keyword, is found anywhere in the name, arguments,\n"
	  "or description for a function, information on that function will\n"
	  "be printed.  The null string (\"\") will match all items." },
	{ "debug_details", "function_name",
	  "Print details about a function, if available" },
	{ "conv_phystopp", "paddr",
	  "Convert a physical address to a page pointer (vm)" },
	{ "conv_pptonum", "pp",
	  "Convert a page pointer to a physical page frame number (vm)" },
	{ "find_dnlc_by_dp", "dp",
	  "Find and print dnlc entries with directory vnode, dp (fs)" },
	{ "find_dnlc_by_name", "name",
	  "Find and print dnlc entries with filename, name (fs)" },
	{ "find_dnlc_by_vp", "vp",
	  "Find and print dnlc entries with target vnode, vp (fs)" },
	{ "find_page_by_id", "vp offset",
	  "Find and print the page with <vp, offset> identity (vm)" },
	{ "find_smap_by_addr", "vaddr",
	  "Find and print the segmap smap struct for segmap address, vaddr" },
	{ "find_smap_by_id", "vp offset",
	  "Find and print the segmap smap struct with <vp, offset> identity" },
	{ "print_active_pages", NULL,
	  "Print all active (read or write locked) pages (vm)" },
	{ "print_adt_bufctl", NULL,
	  "Print the adt_bufctl structure (audit)" },
	{ "print_adt_logctl", NULL,
	  "Print the adt_logctl structure (audit)" },
	{ "print_adt_lvlctl", NULL,
	  "Print the adt_lvlctl structure (audit)" },
	{ "print_anon_hist", "anonp",
	  "Print the history information for an anon_t (anonfs)",
	  "Available only when compiled with ANON_DEBUG." },
	{ "print_anon_info", NULL,
	  "Print miscellaneous anonfs info" },
	{ "print_arecbuf", "arecp",
	  "Print an arecbuf_t structure (audit)" },
	{ "print_as", "as",
	  "Print an AS (address space) structure (vm)" },
	{ "print_bfreelist_id", NULL,
	  "Print identity-related fields for buffers on the freelist (bio)" },
	{ "print_bfreelist_links", NULL,
	  "Print linkage fields for buffers on the freelist (bio)" },
	{ "print_buf", "buf",
	  "Print a buffer header structure (bio)" },
	{ "print_bufhash_id", NULL,
	  "Print identity-related fields for buffers in the hash table (bio)" },
	{ "print_bufhash_links", NULL,
	  "Print linkage fields for buffers in the hash table (bio)" },
	{ "print_bufpool", NULL,
	  "Print miscellaneous buffer pool info (bio)" },
	{ "print_cdfs_freelist", NULL,
	  "Print cdfs inodes on the freelist" },
	{ "print_cdfs_hash", NULL,
	  "Print cdfs inodes in the hash table" },
	{ "print_cdfs_inode", NULL,
	  "Print a cdfs inode information" },
	{ "print_cred", "credp",
	  "Print a cred_t structure (credentials)" },
	{ "print_dblk", "dblkp",
	  "Print a dblk_t structure (streams)" },
	{ "print_dnlc_hash", NULL,
	  "Print all dnlc entries in hash table (fs)" },
	{ "print_dnlc_lru", NULL,
	  "Print all dnlc entries on LRU list (fs)" },
	{ "print_file", "fp",
	  "Print a file structure" },
	{ "print_intr_stats", "cpu",
	  "Print interrupt statistics for given CPU, or all CPUs if -1" },
	{ "print_kernel_addrs", NULL,
	  "Print special kernel virtual addresses" },
	{ "print_kma_hist", "control_string ...",
	  "Print history of KMA activity (vm)",
	  "control_string includes zero or more cmd chars (either case);\n"
	  "for each cmd char, there is an additional argument.\n\n"
	  "Available cmd chars are:\n"
	  "    L: LWP == arg\n"
	  "    A: addr == arg\n"
	  "    S: size == arg (rounded to KMA buffer size)\n"
	  "    2: size == arg (rounded to next power of 2)\n"
	  "    C: addr is in same chunk as arg\n"
	  "    F: filename == arg\n"
	  "    N: consider only youngest arg history entries\n\n"
	  "Any cmd char except N may be preceded by '!' to reverse the sense\n"
	  "of the comparison.\n\n"
	  "Available only when compiled with _KMEM_HIST." },
	{ "print_kma_stats", NULL,
	  "Print KMA statistics (vm)",
	  "Available only when compiled with _KMEM_STATS." },
	{ "print_kmp", "kmp",
	  "Print a segkvn kmp struct (vm)" },
	{ "print_kpg_stats", NULL,
	  "Print KPG statistics (vm)",
	  "Available only when compiled with DEBUG" },
	{ "print_kpg_zbm", NULL,
	  "Print ZBM info for KPG (vm)",
	  "Available only when compiled with DEBUG" },
	{ "print_lkstat", "lkstatp",
	  "Print a lkstat_t struct, for lock statistics (ksynch)" },
	{ "print_lock_stats", NULL,
	  "Print lock-related metrics (ksynch)" },
	{ "print_locks", "cpu",
	  "Print locks held on the given CPU, or all CPUs if -1 (ksynch)" },
	{ "print_lwp", "lwpp",
	  "Print an LWP structure (proc)" },
	{ "print_lwp_alwp", "lwpp",
	  "Print the alwp_t struct for an LWP (audit)" },
	{ "print_lwp_cred", "lwpp",
	  "Print the cred_t struct for an LWP (credentials)" },
	{ "print_mapped_pages", NULL,
	  "Print all mapped pages (vm)" },
	{ "print_mblk", "mblkp",
	  "Print an mblk_t structure (streams)" },
	{ "print_mem_resv_stats", NULL,
	  "Print mem_resv statistics (vm)",
	  "Available only when compiled with DEBUG or with _MEM_RESV_STATS.\n"
	  "When compiled with DEBUG prints waiting statistics.\n"
	  "When compiled with _MEM_RESV_STATS prints call counts.\n"
	  "When compiled with both DEBUG and _MEM_RESV_STATS prints\n"
	  "waiting statistics and call counts."},
	{ "print_metrics", NULL,
	  "Print all collected metrics" },
	{ "print_mnode", "mnodep",
	  "Print a memfs mnode structure" },
	{ "print_mnode_bs", "bsnp size",
	  "Print backing-store information for a memfs mnode",
	  "bsnp is a memfs_bs_t pointer from an mnode_t;\n"
	  "size is the mnode file size" },
	{ "print_mnodes", NULL,
	  "Print all memfs mnodes" },
	{ "print_nfs_freelist", NULL,
	  "Print nfs inodes on the freelist" },
	{ "print_nfs_hash", NULL,
	  "Print nfs inodes in the hash table" },
	{ "print_nfs_mntlist", NULL,
	  "Print nfs mntlist info" },
	{ "print_nfs_page_info", NULL,
	  "Print summary info for nfs pages" },
	{ "print_page", "pp",
	  "Print a page structure (vm)" },
	{ "print_page_info", NULL,
	  "Print summary info on pages (vm)" },
	{ "print_page_plocal_stats", NULL,
	  "Print per-CPU page metrics (vm)" },
	{ "print_page_stats", NULL,
	  "Print extra page statistics (vm)",
	  "Available only when compiled with EXTRA_PAGE_STATS" },
	{ "print_pageio_log", "bp pp lwp n",
	  "print log of pageio activity (fs)",
	  "If all parameters are 0, then print all entries.\n"
	  "\n"
	  "bp      If non-NULL, then print only entries matching the\n"
	  "        specified buffer.\n"
	  "pp      If non-NULL, then print only entries matching the\n"
	  "        specified page.\n"
	  "lwp     If non-NULL, then print only entries matching the\n"
	  "        specified LWP.\n"
	  "n       If non-zero, then limit the number of entries printed\n"
	  "        to n.\n"
	  "Available only when compiled with _PAGEIO_HIST" },
	{ "print_pagepool", NULL,
	  "Print the pagepool chunks and sizes (vm)" },
	{ "print_prnode", "prnode",
	  "Print a prnode" },
	{ "print_proc", "procp",
	  "Print a proc structure (proc)" },
	{ "print_proc_aproc", "procp",
	  "Print the aproc_t structure for a process (audit)" },
	{ "print_proc_as", "procp",
	  "Print the AS (address-space) structure for a process (vm)" },
	{ "print_proc_cred", "procp",
	  "Print the cred_t structure for a process (credentials)" },
	{ "print_putbuf", NULL,
	  "Print putbuf as text (console output gets copied to putbuf)" },
	{ "print_pvn_memresv_hash", NULL,
	  "Print the contents of pvn_memresv_hash (vm)" },
	{ "print_queue", "qp",
	  "Print a queue_t structure (streams)" },
	{ "print_rnode", "rnodep",
	  "Print an nfs inode (also called an rnode)" },
	{ "print_s5_freelist", NULL,
	  "Print s5fs inodes on the freelist" },
	{ "print_s5_hash", NULL,
	  "Print s5fs inodes in the hash table" },
	{ "print_segkvn_stats", NULL,
	  "Print segkvn statistics (vm)",
	  "Available only when compiled with DEBUG" },
	{ "print_segkvn_zbm", NULL,
	  "Print ZBM info for segkvn (vm)",
	  "Available only when compiled with DEBUG" },
	{ "print_segmap_stats", NULL,
	  "Print debug statistics for segmap (vm)",
	  "Available only when compiled with DEBUG" },
	{ "print_sfs_cg", "cgp",
	  "Print an sfs cylinder-group structure" },
	{ "print_sfs_freelist", NULL,
	  "Print sfs inodes on the freelist" },
	{ "print_sfs_hash", NULL,
	  "Print sfs inodes in the hash table" },
	{ "print_sfs_inode", "ip",
	  "Print an sfs inode" },
	{ "print_smap", "smapp",
	  "Print a segmap smap_t structure (vm)" },
	{ "print_snode", "snode",
	  "Print an snode" },
	{ "print_static_modules", NULL,
	  "Print the list of modules statically linked into the kernel (dlm)" },
	{ "print_ufs_inode", "ip",
	  "Print an ufs inode" },
	{ "print_vfs", "vfs",
	  "Print a vfs structure" },
	{ "print_vnode", "vp",
	  "Print a vnode" },
	{ "print_vnode_log", "vp lwp",
	  "Print contents of the log for vnode vp (fs)",
	  "If lwp is non-NULL, then restrict printout to entries associated\n"
	  "with the specified lwp.\n"
	  "Available only when compiled with _VNODE_HIST" },
	{ "print_lwp_vnlog", "lwp vp",
	  "Print contents of the vnode log for the specified lwp (fs)",
	  "If vp is non-NULL then restrict printout to entries associated\n"
	  "with the specified vp\n."
	  "Available only when compiled with _VNODE_HIST" },
	{ "print_vnode_mnode", "vp",
	  "Print the memfs mnode for a (memfs) vnode" },
	{ "print_vnode_pages", "vp",
	  "Print the list of pages for a vnode (vm/fs)" },
	{ "print_vxfs_fs", "fsp",
	  "Print a vx_fs (super-block) structure" },
	{ "print_vxfs_fsdisk", "fsdskp",
	  "Print a vx_fsdisk (super-block disk based portion) structure" },
	{ "print_vxfs_fscommon", "fscomnp",
	  "Print a vx_fscommon (ro portion of super-block) structure" },
	{ "print_vxfs_fswrite", "fswrtp",
	  "Print a vx_fswrite (rw portion of super-block) structure" },
	{ "print_vxfs_fsmem", "fsmemp",
	  "Print a vx_fsmem (memory only super-block flds) structure" },
	{ "print_vxfs_fscommon2", "fscmnp",
	  "Print a vx_fscommon2 (v2 layout super-block flds) structure" },
	{ "print_vxfs_fsnocopy", "fsncpyp",
	  "Print a vx_fsnocopy (stable portion fld of super-block) structure" },
	{ "print_vxfs_fsstruct", "fsstrp",
	  "Print a vx_fsstruct (fs structure super-block flds) structure" },
	{ "print_vxfs_fset", "fsp",
	  "Print the vx_fset (incore file set) structure" },
	{ "print_vxfs_inode", "ip",
	  "Print a vxfs inode" },
	{ "print_vxfs_tran", "tranp",
	  "Print a vx_tran (transaction control) structure" },
	{ "print_vxfs_ctran", "ctranp",
	  "Print a vx_ctran (transaction common area) structure" },
	{ "print_vxfs_imtran", "imtranp",
	  "Print a vx_imtran (inode modification transaction) structure" },
	{ "print_vxfs_datran", "datranp",
	  "Print a vx_datran (dir entry addition transaction) structure" },
	{ "print_vxfs_drtran", "drtranp",
	  "Print a vx_drtran (dir entry removal transaction) structure" },
	{ "print_vxfs_emtran", "emtranp",
	  "Print a vx_emtran (extent map manipulation) structure" },
	{ "print_vxfs_trtran", "trtranp",
	  "Print a vx_trtran (indirect extent truncation) structure" },
	{ "print_vxfs_tatran", "tatranp",
	  "Print a vx_tatran (indirect extent addition) structure" },
	{ "print_vxfs_lwrtran", "lwrtranp",
	  "Print a vx_lwrtran (logged write to a file) structure" },
	{ "print_vxfs_wltran", "wltranp",
	  "Print a vx_wltran (pathname log) structure" },
	{ "print_vxfs_iltran", "iltranp",
	  "Print a vx_iltran structure" },
	{ "print_vxfs_ietran", "ietranp",
	  "Print a vx_ietran (inode map update subfunction) structure" },
	{ "print_vxfs_cutran", "cutranp",
	  "Print a vx_cutran (CUT modification subfunction) structure" },
	{ "print_vxfs_fshdtran", "fshdtp",
	  "Print a vx_fshdtran (fileset hdr mod. subfunction) structure" },
	{ "print_vxfs_lctran", "lctranp",
	  "Print a vx_lctran (link cnt tbl update subfunction) structure" },
	{ "print_vxfs_lbtran", "lbtranp",
	  "Print a vx_lbtran (logged buf changes subfunction) structure" },
	{ "print_vxfs_duclog", "duclogp",
	  "Print a vx_duclog (done and undo subfunction info) structure" },
	{ "print_vxfs_daclog", "daclogp",
	  "Print a vx_daclog (log area if VX_DACTRAN) structure" },
	{ "print_vxfs_map", "mapp",
	  "Print a vx_map (map control) structure" },
	{ "print_vxfs_vxinfo", "ip",
	  "Print a full vx_info (vxfs statistics) structure" },
	{ "print_vxfs_buffstat", "ip",
	  "Print buffer statistics from vx_info structure" },
	{ "print_vxfs_logstat", "ip",
	  "Print log statistics from vx_info structure" },
	{ "print_vxfs_dirstat", "ip",
	  "Print directory statistics from vx_info structure" },
	{ "print_vxfs_miscstat", "ip",
	  "Print miscellaneous statistics from vx_info structure" },
	{ "print_vxfs_inodestat", "ip",
	  "Print inode statistics from vx_info structure" },
	{ "print_vxfs_rwstat", "ip",
	  "Print sync/async read/write statistics from vx_info structure" },
	{ "print_vxfs_transtat", "ip",
	  "Print sync/async tranflush/tranlogflush stats from vx_info struct" },
	{ "print_zbm", "zbmp",
	  "Print a zbm_t structure (zbm)" },
	{ "print_zbm_cell", "cellp",
	  "Print a ZBM zbm_cell structure (zbm)" },
	{ "print_zbm_stats", "zbmp",
	  "Print ZBM statistics for a zbm_t structure (zbm)" },
};

#define DBGTOOLS_SIZE	(sizeof dbgtools / sizeof dbgtools[0])

/* Internal data structures used by debug_help(), et al. */
static struct dbgtool_info *dbgtools_head;

void debug_help(const char *keyword);

/*
 * static boolean_t
 * debug_valid_string(const char *str, const char *funcname)
 *	Check if a string argument looks like a valid string.
 *
 * Calling/Exit State:
 *	Returns B_TRUE if str appears to be a valid string;
 *	otherwise prints an error message and returns B_FALSE.
 */
static boolean_t
debug_valid_string(const char *str, const char *funcname)
{
	size_t n;

	if (str != NULL && KADDR(str)) {
		for (n = 10;; ++str) {
			if (*str == '\0' || --n == 0)
				return B_TRUE;
			if (*str <= ' ' || *str >= '\177')
				break;
		}
	}
	debug_printf("Invalid string argument\n\n");
	debug_help(funcname);
	return B_FALSE;
}

/*
 * static boolean_t
 * debug_kwd_match(const char *keyword, const char *str)
 *	Check if keyword occurs anywhere in the string, str.
 *	Ignores case.
 *
 * Calling/Exit State:
 *	No locking needed.
 */
static boolean_t
debug_kwd_match(const char *keyword, const char *str)
{
	size_t klen, slen;
	const char *kp, *sp;
	char kc, sc;

	if (str == NULL)
		return B_FALSE;
	klen = strlen(keyword);
	slen = strlen(str);
	while (slen >= klen) {
		kp = keyword;  sp = str;
		do {
			if ((kc = *kp++) == '\0')
				return B_TRUE;
			if (kc >= 'A' && kc <= 'Z')
				kc += 'a' - 'A';
			if ((sc = *sp++) >= 'A' && sc <= 'Z')
				sc += 'a' - 'A';
		} while (kc == sc);
		++str;  --slen;
	}
	return B_FALSE;
}

/*
 * void
 * debug_help(const char *keyword)
 *	Print info about all debug tool functions matching keyword.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger.
 */
void
debug_help(const char *keyword)
{
	struct dbgtool_info *dtip;
	boolean_t match_found = B_FALSE;
	boolean_t details_available = B_FALSE;

	if (!debug_valid_string(keyword, "debug_help"))
		return;

	if (dbgtools_head == NULL) {
		/* Sort the info in alphabetical order by function name. */
		struct dbgtool_info **dtipp;

		for (dtip = dbgtools + DBGTOOLS_SIZE; dtip-- != dbgtools;) {
			if (dtip->dti_funcname == NULL)
				continue;  /* just in case */
			for (dtipp = &dbgtools_head; *dtipp != NULL;) {
				if (strcmp(dtip->dti_funcname,
					   (*dtipp)->dti_funcname) < 0)
					break;
				dtipp = &(*dtipp)->dti_next;
			}
			dtip->dti_next = *dtipp;
			*dtipp = dtip;
		}
	}

	/* Search for entries matching keyword */
	for (dtip = dbgtools_head; dtip != NULL; dtip = dtip->dti_next) {
		if (!debug_kwd_match(keyword, dtip->dti_funcname) &&
		    !debug_kwd_match(keyword, dtip->dti_args) &&
		    !debug_kwd_match(keyword, dtip->dti_desc))
			continue;
		/* Found a match; print it. */
		if (dtip->dti_details) {
			debug_printf("* ");
			details_available = B_TRUE;
		} else
			debug_printf("  ");
		debug_printf("%-25s", dtip->dti_funcname);
		if (dtip->dti_args)
			debug_printf(" Arguments: %s", dtip->dti_args);
		debug_printf("\n    %s\n", dtip->dti_desc);
		
		if (debug_output_aborted())
			return;
		match_found = B_TRUE;
	}

	if (!match_found)
		debug_printf("No match for \"%s\"\n", keyword);
	else if (details_available) {
		debug_printf(
    "\n* More details are available for functions marked with an asterisk;\n"
      "  call debug_details() with the function name string to see them.\n");
	}
}

/*
 * void
 * debug_details(const char *funcname)
 *	Print details for the debug tool function, funcname.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger.
 */
void
debug_details(const char *funcname)
{
	struct dbgtool_info *dtip;

	if (!debug_valid_string(funcname, "debug_details"))
		return;

	/* Search for an entry with the given name */
	for (dtip = dbgtools_head; dtip != NULL; dtip = dtip->dti_next) {
		if (strcmp(funcname, dtip->dti_funcname) != 0)
			continue;
		if (dtip->dti_details) {
			debug_printf("%s details:\n\n%s\n",
				     funcname, dtip->dti_details);
		} else {
			debug_printf("No details available for \"%s\"\n",
				     funcname);
		}
		return;
	}

	debug_printf("No such function: \"%s\"\n", funcname);
}

#endif /* DEBUG || DEBUG_TOOLS */


#ifdef DEBUG

/*
 * Set to have assfail() print a message and then call demon
 */
boolean_t aask;
/*
 * Set to have assfail() to return to caller, instead of panicking.
 */
boolean_t aok;

#endif /* DEBUG */

/*
 * int
 * assfail(const char *, const char *, int)
 *	Routine called from ASSERT macro to print panic message and
 *	panic the system.
 *
 * Calling/Exit State:
 *	Must return a value (that is not used) because of
 *	ASSERT() macro.
 */
int
assfail(const char *a, const char *f, int l)
{
	/*
	 * force messages to go to console
	 */
	(void) conslog_set(CONSLOG_DIS);
#ifdef	DEBUG
	if (aask) {
		/*
		 *+ DEBUG ONLY: an assertion failed; calling demon
		 */
		cmn_err(CE_NOTE,
		    "ASSERTION CAUGHT: %s, file: %s, line: %d", a, f, l);
		call_demon();
	}
	if (aok)
		return 0;
#endif /* DEBUG */
	/*
	 *+ A DEBUG assertion failed.
	 */
	cmn_err(CE_PANIC,
	    "assertion failed: %s, file: %s, line: %d", a, f, l);
	/* NOTREACHED */
	return 1;
}

/*
 * void
 * s_assfail(const char *a, const char *f, int id, S_ASSFAIL_ARGDECL)
 *	Print out an assembler assertion failure
 *
 * Calling/Exit State:
 *	Called from assembly code when an assertion failure occurs.
 *	Arguments are assertion, file name, assertion id,
 *	and additional machine dependent arguments.
 *
 * Remarks:
 *	Follows same conventions as assfail with respect to aask and aok.
 */
void
s_assfail(const char *a, const char *f, int id, S_ASSFAIL_ARGDECL)
{
	/*
	 * force messages to go to console
	 */
	(void) conslog_set(CONSLOG_DIS);
#ifdef	DEBUG
	if (aask) {
		/*
		 *+ DEBUG ONLY: an assertion failed; calling demon
		 */
		cmn_err(CE_NOTE,
			"ASSERTION CAUGHT: \"%s\", file: %s, assertion #: %d\n"
			 S_ASSFAIL_FMT,
			a, f, id, S_ASSFAIL_ARGVAL);
		call_demon();
	}
	if (aok)
		return;
#endif /* DEBUG */
	/*
	 *+ A DEBUG assembly-language assertion failed.
	 */
	cmn_err(CE_PANIC,
		"assertion failed: \"%s\", file: %s, assertion #: %d\n"
		 S_ASSFAIL_FMT,
		a, f, id, S_ASSFAIL_ARGVAL);
	/* NOTREACHED */
}
