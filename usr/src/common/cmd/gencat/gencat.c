/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)gencat:gencat.c	1.2.8.3"
#ident  "$Header: gencat.c 1.3 91/08/12 $"
#include <dirent.h>
#include <stdio.h>
#include <ctype.h>
#include <nl_types.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "gencat.h"

/*
 * gencat : take a message source file and produce a message catalog.
 * usage : gencat [-lmXf <format>] catfile [msgfile]
 * if msgfile is not specified, read from standard input.
 * Several message files can be specified
 */
char msg_buf[NL_TEXTMAX];
struct cat_set *sets = 0;
int curset = 1;
int is_set_1 = 1;
int no_output = 0;
int coff = 0;
int list = 0;
int cat_format = 0;
int show_flag = 0;
char catname[MAXNAMLEN];

FILE *tempfile;
FILE *fd_temp;

const char
  nomem[] = ":12:Out of memory: %s\n",
  badread[] = ":13:Cannot read %s: %s\n",
  badreadtmp[] = ":14:Cannot read temporary file: %s\n",
  badwritetmp[] = ":15:Cannot write in temporary file: %s\n",
  badwrite[] = ":16:Cannot write %s: %s\n",
  badcreate[] = ":17:Cannot create %s: %s\n",
  badopen[] = ":18:Cannot open %s: %s\n",
  badseektmp[] = ":19:seek() failed in temporary file: %s\n",
  badsetnr[] = ":20:Invalid set number %d -- Ignored\n",
  badmsgnr[] = ":21:Invalid message number %d -- Ignored\n";

main(argc, argv)
  int argc;
  char **argv;
{
  FILE *fd;
  register int i;
  int cat_exists;
  int opt;
  extern int optind;

  (void)setlocale(LC_ALL, "");
  (void)setcat("uxmesg");
  (void)setlabel("UX:gencat");
  /*
   * No arguments at all : give usage
   */
  if (argc == 1)
    usage(1);
    
  /*
   * Create tempfile
   */
  if ((tempfile = tmpfile()) == NULL){
    pfmt(stderr, MM_ERROR, ":22:tempfile() failed: %s\n",
      strerror(errno));
    exit(1);
  }
  
  /*
   * test for mkmsgs format
   */
  while ((opt = getopt(argc, argv, "lmXf:")) != EOF){
    switch (opt) {
      case 'l' :
	list = 1;
	break;
      case 'm' :
      case 'X' :
	if (cat_format != 0)
	  usage(0);
	cat_format = MALLOC_FORMAT;
	break;
      case 'f' :
	if (cat_format != 0)
	  usage(0);

	if (strcmp(optarg, "m") == 0)
	  cat_format = MALLOC_FORMAT;
	else if (strcmp(optarg, "SVR4") == 0)
	  cat_format = SVR_FORMAT;
	else if (strcmp(optarg, "sco") == 0)
	  cat_format = SCO_FORMAT;
	else if (strcmp(optarg, "ISC") == 0)
	  cat_format = ISC_FORMAT;
	else
	  usage(0);
	break;

      default :
	usage(0);
	exit(1);

    }
  }
      
  if (optind == argc)
    usage(1);

  /*
   *  Check for environment variable to set default output mode if
   *  no mode specified earlier
   */
  if (cat_format == 0 && getenv("SCOMPAT") != NULL)
    cat_format = SCO_FORMAT;

  /*
   * Check catfile name : if file exists, read it.
   */
  strcpy(catname, argv[optind]);
  
  /*
   * use an existing catalog
   */
  if (cat_build(catname))
    cat_exists = 1;
  else
    cat_exists = 0;

  /*
   * Open msgfile(s) or use stdin and call handling proc
   */
  if (optind == argc - 1)
    cat_msg_build("stdin", stdin);
  else {
    for (i = optind + 1; i < argc ; i++){
      if ((fd = fopen(argv[i], "r")) == 0){
        /*
         * Cannot open file for reading
         */
        pfmt(stderr, MM_ERROR, badopen, argv[i], strerror(errno));
        continue;
      }
      if (argc - optind > 2)
        pfmt(stdout, MM_INFO, ":23:%s:\n", argv[i]);
      cat_msg_build(argv[i], fd);
      fclose(fd);
    }
  }

  if (!no_output){
    /*
     * Work is done. Time now to open catfile (for writing) and to dump
     * the result
     */

    if (cat_format == MALLOC_FORMAT)
      cat_dump(catname);
    else if (cat_format == SCO_FORMAT)
      cat_sco_dump(catname);
    else if (cat_format == ISC_FORMAT)
      cat_isc_dump(catname);
    else
      cat_mmp_dump(catname);
  } else
    if (cat_exists)
      pfmt(stderr, MM_ERROR, ":24:%s not updated\n", catname);
    else
      pfmt(stderr, MM_ERROR, ":25:%s not created\n", catname);

  exit(0);
}

usage(complain)
int complain;
{
  if (complain)
    pfmt(stderr, MM_ERROR, ":26:Incorrect usage\n");
  pfmt(stderr, MM_ACTION, ":193:Usage: gencat [-mXlf <format> ] catfile [msgfile ...]\n");
  exit(0);
}

char linebuf[BUFSIZ];

/*
 * Scan message file and complete the tables
 */
cat_msg_build(filename, fd)
  char *filename;
  FILE *fd;
{
  register int i, j;
  char quotechar = 0;
  char c;
  int msg_nr = 0;
  int msg_len;

  /*
   * always a set NL_SETD
   */
  curset=NL_SETD;
  add_set(curset);
  
  while (fgets(linebuf, BUFSIZ, fd) != 0){


    if ((i = skip_blanks(linebuf, 0)) == -1)
      continue;

    if (linebuf[i] == '$'){
      i += 1;
      /*
       * Handle commands or comments
       */
      if (strncmp(&linebuf[i], CMD_SET, CMD_SET_LEN) == 0){
        i += CMD_SET_LEN;

        /*
         * Change current set
         */
        if ((i = skip_blanks(linebuf, i)) == -1){
          pfmt(stderr, MM_WARNING, ":28:Incomplete set command -- Ignored\n");
          continue;
        }
        add_set(atoi(&linebuf[i]));

	/*  This issues a warning message if there is text after
	 *  the set number.  This is so SCO message source files that
	 *  use the undocumented extensions warn the user.
	 */
	while (isdigit(linebuf[i]) || isspace(linebuf[i]))
	  ++i;

	if (linebuf[i] != NULL)
	  pfmt(stderr, MM_WARNING, ":194:Unexpected text after set number - Ignored\n");
        continue;
      }
      if (strncmp(&linebuf[i], CMD_DELSET, CMD_DELSET_LEN) == 0){
  i += CMD_DELSET_LEN;

        /*
         * Delete named set
         */
        if ((i = skip_blanks(linebuf, i)) == -1){
          pfmt(stderr, MM_WARNING, ":29:Incomplete delset command -- Ignored\n")
          ;
          continue;
        }
        del_set(atoi(&linebuf[i]));
        continue;
      }
      if (strncmp(&linebuf[i], CMD_QUOTE, CMD_QUOTE_LEN) == 0){
        i += CMD_QUOTE_LEN;

        /*
         * Change quote character
         */
        if ((i = skip_blanks(linebuf, i)) == -1)
          quotechar = 0;
        else
          quotechar = linebuf[i];
        continue;
      }
      /*
       * Everything else is a comment
       */
      continue;
    }
    if (isdigit(linebuf[i])){
      msg_nr = 0;
      /*
       * A message line
       */
      while(isdigit(c = linebuf[i])){
        msg_nr *= 10;
        msg_nr += c - '0';
        i++;
      }
      j = i;
      if (linebuf[i] == '\n')
        del_msg(curset, msg_nr);
      else
      if ((i = skip_blanks(linebuf, i)) == -1)
	add_msg(curset,msg_nr,1,"");
      else {
        if ((msg_len = msg_conv(filename, curset, msg_nr, fd, linebuf + j,
        BUFSIZ, msg_buf, NL_TEXTMAX, quotechar)) == -1){
    no_output = 1;
          continue;
  }
        add_msg(curset, msg_nr, msg_len + 1, msg_buf);
      }
      continue;
    }
    
    /*
     * Everything else is unexpected
     */
    pfmt(stderr, MM_WARNING, ":30:Unexpected line -- Skipped\n");
  }
}

/*
 * Skip blanks in a line buffer
 */
skip_blanks(linebuf, i)
  char *linebuf;
  int i;
{
  while (linebuf[i] && isspace(linebuf[i]) && !iscntrl(linebuf[i]))
    i++;
  if (!linebuf[i] || linebuf[i] == '\n')
    return -1;
  return i;
}

/*
 * Dump the internal structure into the catfile.
 */
cat_dump(catalog)
char *catalog;
{
  FILE *fd;
  FILE *f_shdr, *f_mhdr, *f_msg;
  long o_shdr, o_mhdr, o_msg;
  struct cat_set *p_sets;
  struct cat_msg *p_msg;
  struct cat_hdr hdr;
  struct cat_set_hdr shdr;
  struct cat_msg_hdr mhdr;
  int msg_len;
  int nmsg = 0;
  
  if ((fd = fopen(catalog,"w")) == NULL) {
    pfmt(stderr, MM_ERROR, badcreate, catalog, strerror(errno));
    exit(1);
  }
    
  if ((f_shdr = tmpfile()) == NULL || (f_mhdr = tmpfile()) == NULL ||
      (f_msg = tmpfile()) == NULL){
    pfmt(stderr, MM_ERROR, ":31:Cannot create temporary file: %s\n",
      strerror(errno));
    exit(1);
  }
  o_shdr = 0;
  o_mhdr = 0;
  o_msg = 0;
  
  p_sets = sets;
  hdr.hdr_set_nr = 0;
  hdr.hdr_magic = CAT_MAGIC;
  while (p_sets != 0){
    hdr.hdr_set_nr++;
    p_msg = p_sets->set_msg;
    
    /*
     * Keep offset in shdr temp file to mark the set's begin
     */
    shdr.shdr_msg = nmsg;
    shdr.shdr_msg_nr = 0;
    shdr.shdr_set_nr = p_sets->set_nr;
    while (p_msg != 0){
      shdr.shdr_msg_nr++;
      nmsg++;
      msg_len = p_msg->msg_len;
      /*
       * Get message from main temp file
       */
      if (fseek(tempfile, p_msg->msg_off, 0) != 0){
        pfmt(stderr, MM_ERROR, badseektmp, strerror(errno));
        exit(1);
      }
      if (fread(msg_buf, 1, msg_len, tempfile) != msg_len){
        pfmt(stderr, MM_ERROR, badreadtmp, strerror(errno));
        exit(1);
      }
      
      /*
       * Put it in the messages temp file and keep offset
       */
      mhdr.msg_ptr = (int)ftell(f_msg);
      mhdr.msg_len = msg_len;
      mhdr.msg_nr  = p_msg->msg_nr;
      if (fwrite(msg_buf, 1, msg_len, f_msg) != msg_len){
        pfmt(stderr, MM_ERROR, badwritetmp, strerror(errno));
        exit(1);
      }

      /*
       * Put message header
       */
      if (fwrite((char *)&mhdr, sizeof(mhdr), 1, f_mhdr) != 1){
        pfmt(stderr, MM_ERROR, badwritetmp, strerror(errno));
        ;
        exit(1);
      }
      p_msg = p_msg->msg_next;
    }
    
    /*
     * Put set hdr
     */
    if (fwrite((char *)&shdr, sizeof(shdr), 1, f_shdr) != 1){
      pfmt(stderr, MM_ERROR, badwritetmp, strerror(errno));
      exit(1);
    }
    p_sets = p_sets->set_next;
  }
  
  /*
   * Fill file header
   */
  hdr.hdr_mem = ftell(f_shdr) + ftell(f_mhdr) + ftell(f_msg);
  hdr.hdr_off_msg_hdr = ftell(f_shdr);
  hdr.hdr_off_msg = hdr.hdr_off_msg_hdr + ftell(f_mhdr);

  /*
   * Generate catfile
   */
  if (fwrite((char *)&hdr, sizeof (hdr), 1, fd) != 1){
    pfmt(stderr, MM_ERROR, badwrite, catname, strerror(errno));
    exit(1);
  }
  
  copy(fd, f_shdr);
  copy(fd, f_mhdr);
  copy(fd, f_msg);
  fclose(f_shdr);
  fclose(f_mhdr);
  fclose(f_msg);
}

static char copybuf[BUFSIZ];
static
copy(dst, src)
  FILE *dst, *src;
{
  int n;
  
  rewind(src);
  
  while ((n = fread(copybuf, 1, BUFSIZ, src)) > 0){
    if (fwrite(copybuf, 1, n, dst) != n){
      pfmt(stderr, MM_ERROR, badwrite, catname, strerror(errno));
      exit(1);
    }
  }
}

void
fatal(err_str)
char *err_str;
{
  pfmt(stderr, MM_ERROR, err_str);
  exit(1);
}
