/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/newparse.c	1.4"

#include <stdio.h>
#include "sh_config.h" /* which includes sys/types.h */
#include <ctype.h>
#include <string.h>
#include "xksh.h"
#include "strparse.h"

static const char Close_curly = '}';
static const char Open_curly = '{';
static const char *Str_close_curly = "}";
static const char *Str_open_curly = "{";

static char **Dont = NULL;
static int Ndont, Sdont;

char *(*Mall_func)() = malloc;
char *(*Grow_func)() = realloc;
void (*Free_func)() = free;

unsigned int Pr_format = PRSYMBOLIC|PRDECIMAL;
int Pr_tmpnonames = 0;
int Cmode = 0;

struct symlist Val_list = { NULL, 0, 0, NULL };

#define ISPTR(PPINP) (((UPPER((*PPINP)[0]) == 'P') && isdigit((*PPINP)[1])) || ((*PPINP)[0] == '&'))

new_parse(strp, ppinp, pval, assocp)
struct strhead *strp;
char **ppinp;
VAL *pval;
struct assoc_field *assocp;
{
	/* What about C variables */
	if (strp->flags & F_SPECIAL) {
		;
		/* find_special, call special */
		/* if found, return; */
	}
	if (Cmode && isalpha(**ppinp) && (strp->size <= sizeof(VAL))) {
		struct valtype tmpvalt;

		if (cvar_get(*ppinp, &tmpvalt) != SH_FAIL) {
			set_int(strp->size, pval, tmpvalt.val);
			return(SH_SUCC);
		}
		/* Otherwise fall through */
	}

	memset(pval, '\0', strp->size);
	XK_SKIPWHITE(ppinp);
	switch (strp->type) {
	case TYPE_INT:
		{
			ulong tmp = 0;

			if (new_par_int(ppinp, &tmp) == SH_FAIL)
				return(SH_FAIL);
			set_int(strp->size, pval, tmp);
			break;
		};
	case TYPE_TYPEDEF:
		return(new_parse(((struct typedefstr *) strp)->typeptr, ppinp, pval, assocp));
	case TYPE_STRUCT:
		{
			int i;
			struct fieldstr *fldp = ((struct structstr *) strp)->fields;
			struct assoc_field tmp_assoc, *pass_assoc;
			char fld_is_set[80];

			if (**ppinp != Open_curly)
				return(SH_FAIL);
			(*ppinp)++;
			for (i = 0; fldp[i].name; i++) {
				XK_SKIPWHITE(ppinp);
				if (**ppinp == Close_curly)
					break;
				if ((fldp[i].typeptr->flags & (F_CHOICE_FIELD|F_LENGTH_FIELD)) && (**ppinp == ',')) {
					(*ppinp)++;
					fld_is_set[i] = 0;
					continue;
				}
				else
					fld_is_set[i] = 1;
				if ((fldp[i].assoc_field_num >= 0) && ((i < fldp[i].assoc_field_num) || !fld_is_set[fldp[i].assoc_field_num])) {
					pass_assoc = &tmp_assoc;
					tmp_assoc.type = fldp[fldp[i].assoc_field_num].typeptr;
					tmp_assoc.pval = (VAL *) ((unchar *) pval) + fldp[fldp[i].assoc_field_num].offset;
				}
				else
					pass_assoc = NULL;
				if (new_parse(fldp[i].typeptr, ppinp, ((unchar *) pval) + fldp[i].offset, pass_assoc) == SH_FAIL)
					return(SH_FAIL);
				if (**ppinp == ',')
					(*ppinp)++;
			}
			XK_SKIPWHITE(ppinp);
			if (**ppinp == Close_curly)
				(*ppinp)++;
			break;
		}
	case TYPE_UNION:
		{
			int i;
			struct union_fieldstr *fldp = ((struct unionstr *) strp)->fields;
			if (**ppinp != Open_curly)
				return(SH_FAIL);
			(*ppinp)++;
			XK_SKIPWHITE(ppinp);
			if (**ppinp != Close_curly) {
				XK_SKIPWHITE(ppinp);
				for (i = 0; fldp[i].name; i++) {
					if (strncmp(fldp[i].name, *ppinp, strlen(fldp[i].name)) == 0) {
						*ppinp += strlen(fldp[i].name);
						XK_SKIPWHITE(ppinp);
						if (**ppinp == Close_curly)
							break;
						if (new_parse(fldp[i].typeptr, ppinp, pval, NULL) == SH_FAIL)
							return(SH_FAIL);
						if (!(strp->flags & F_EXTERNAL_TAG))
							set_int(((struct unionstr *) strp)->choice_size, ((unchar *) pval) + ((struct unionstr *) strp)->choice_offset, fldp[i].tag);
						else
							set_int(assocp->type->size, assocp->pval, fldp[i].tag);
						break;
					}
				}
			}
			XK_SKIPWHITE(ppinp);
			if (**ppinp == Close_curly)
				(*ppinp)++;
			break;
		};
	case TYPE_POINTER:
		{
			struct strhead *ptype = ((struct ptrstr *) strp)->typeptr;

			if (ptype->size == 1)
				return(call_charparse(strp, ppinp, pval, assocp));
			XK_SKIPWHITE(ppinp);
			if (strncmp(*ppinp, (const char *) "NULL", 4) == 0) {
				*ppinp += 4;
				break;
			}
			if (ISPTR(ppinp)) {
				if (par_pointer(ppinp, pval) == SH_FAIL)
					return(SH_FAIL);
				break;
			}
			if ((*pval = (VAL) (*Mall_func)(ptype->size)) == NULL)
				return(SH_FAIL);
			if (new_parse(ptype, ppinp, *pval, assocp) == SH_FAIL) {
				
				(*Free_func)(*pval);
				return(SH_FAIL);
			}
			break;
		};
	case TYPE_DYNARRAY:
		{
			struct strhead *ptype = ((struct arraystr *) strp)->typeptr;
			register uint i;
			int malloc_num;
			char *p;
			int incr = ((Mall_func == malloc) ? 5 : 10);

			if (ptype->size == 1)
				return(call_charparse(strp, ppinp, pval, assocp));
			XK_SKIPWHITE(ppinp);
			if (strncmp(*ppinp, (const char *) "NULL", 4) == 0) {
				*ppinp += 4;
				if (assocp)
					set_int(assocp->type->size, assocp->pval, 0);
				break;
			}
			if (ISPTR(ppinp)) {
				if (par_pointer(ppinp, pval) == SH_FAIL)
					return(SH_FAIL);
				break;
			}
			if (**ppinp != Open_curly)
				return(SH_FAIL);
			malloc_num = incr;
			if (!(p = malloc(malloc_num * ptype->size)))
				return(SH_FAIL);
			memset(p, '\0', malloc_num * ptype->size);
			(*ppinp)++;
			for (i = 0; ; i++) {
				XK_SKIPWHITE(ppinp);
				if (i == malloc_num) {
					if (!(p = (*Grow_func)(p, malloc_num * ptype->size, incr * ptype->size)))
						return(SH_FAIL);
					malloc_num += incr;
					memset(p + (i * ptype->size), '\0', incr * ptype->size);
				}
				/*
				** Notice that we test for the close curly, after
				** growing the array.  This will put in "NULL"
				** termination for those structures that need it.
				*/

				if (**ppinp == Close_curly) {
					i--;
					break;
				}

				if (new_parse(ptype, ppinp, p + (i * ptype->size), NULL) == SH_FAIL)
					return(SH_FAIL);
				XK_SKIPWHITE(ppinp);
				if (**ppinp == ',')
					(*ppinp)++;
			}
			*pval = (VAL) p;
			if (assocp)
				set_int(assocp->type->size, assocp->pval, i + 1);
			if (**ppinp == Close_curly)
				(*ppinp)++;
			break;
		};
	case TYPE_ARRAY:
		{
			struct strhead *ptype = ((struct arraystr *) strp)->typeptr;
			register uint i;

			if (ptype->size == 1)
				return(call_charparse(strp, ppinp, pval, assocp));
			if (**ppinp != Open_curly)
				return(SH_FAIL);
			(*ppinp)++;
			for (i = 0; (ptype->size * i) < strp->size; i++) {
				XK_SKIPWHITE(ppinp);
				if (**ppinp == Close_curly)
					break;
				if (new_parse(ptype, ppinp, ((unchar *) pval) + (i * ptype->size), NULL) == SH_FAIL)
					return(SH_FAIL);
				if (**ppinp == ',')
					(*ppinp)++;
			}
			if (**ppinp == Close_curly)
				(*ppinp)++;
			if (assocp)
				set_int(assocp->type->size, assocp->pval, i + 1);
			break;
		};
	}
	return(SH_SUCC);
}

new_print(strp, pval, pout, assocp)
struct strhead *strp;
VAL *pval;
struct easyio *pout;
struct assoc_field *assocp;
{
	if (strp->flags & F_SPECIAL) {
		;
		/* find_special, call special */
		/* if found, return; */
	}
	if (strp->flags & F_SYMBOLIC) {
		ulong tmp = 0;

		get_int(strp->size, pval, &tmp);
		sub_prin_int(pout, tmp, fsymbolic(strp));
		return(SH_SUCC);
	}
	switch (strp->type) {
	case TYPE_INT:
		{
			ulong tmp = 0;

			get_int(strp->size, pval, &tmp);
			if (Val_list.syms) {
				sub_prin_int(pout, tmp, &Val_list);
				return(SH_SUCC);
			}
			if (new_prin_int(pout, tmp) == SH_FAIL)
				return(SH_FAIL);
			break;
		};
	case TYPE_TYPEDEF:
		return(new_print(((struct typedefstr *) strp)->typeptr, pval, pout, assocp));
	case TYPE_STRUCT:
		{
			int i;
			struct fieldstr *fldp = ((struct structstr *) strp)->fields;
			struct assoc_field tmp_assoc, *pass_assoc;

			outprintf(pout, "{ ");
			for (i = 0; fldp[i].name; i++) {
				/*if (fldp[i].flags & (F_CHOICE_FIELD|F_LENGTH_FIELD))*/
					/*continue;*/
				if (i != 0)
					outprintf(pout, ", ");
				if (fldp[i].flags & F_SYMBOLIC) {
					ulong tmp = 0;

					get_int(fldp[i].typeptr->size, ((unchar *) pval) + fldp[i].offset, &tmp);
					sub_prin_int(pout, tmp, fsymbolic(fldp + i));
				}
				else {
					if (fldp[i].assoc_field_num >= 0) {
						pass_assoc = &tmp_assoc;
						tmp_assoc.type = fldp[fldp[i].assoc_field_num].typeptr;
						tmp_assoc.pval = (VAL *) ((unchar *) pval) + fldp[fldp[i].assoc_field_num].offset;
					}
					else
						pass_assoc = NULL;
					if (new_print(fldp[i].typeptr, ((unchar *) pval) + fldp[i].offset, pout, pass_assoc) == SH_FAIL)
						return(SH_FAIL);
				}
			}
			outprintf(pout, " }");
			break;
		}
	case TYPE_UNION:
		{
			int i;
			struct union_fieldstr *fldp = ((struct unionstr *) strp)->fields;
			ulong choice;

			if (strp->flags & F_EXTERNAL_TAG) {
				if (!assocp)
					return(SH_FAIL);
				get_int(assocp->type->size, assocp->pval, &choice);
			}
			else
				get_int(((struct unionstr *) strp)->choice_size, ((unchar *) pval) + ((struct unionstr *) strp)->choice_offset, &choice);
			for (i = 0; fldp[i].name; i++)
				if (!(fldp[i].flags & F_NO_TAG) && (fldp[i].tag == choice))
					break;
			if (fldp[i].name) {
				outprintf(pout, "{");
				outprintf(pout, fldp[i].name);
				outprintf(pout, " ");
				if (fldp[i].flags & F_SYMBOLIC) {
					ulong tmp = 0;

					get_int(fldp[i].typeptr->size, pval, &tmp);
					sub_prin_int(pout, tmp, fsymbolic(fldp + i));
				}
				else if (new_print(fldp[i].typeptr, pval, pout, NULL) == SH_FAIL)
					return(SH_FAIL);
				outprintf(pout, " }");
			}
			else
				return(SH_FAIL);
			break;
		};
	case TYPE_POINTER:
		{
			struct strhead *ptype = ((struct ptrstr *) strp)->typeptr;

			if (!*pval)
				outprintf(pout, "NULL");
			else if (ptype->size == 1)
				return(call_charprint(strp, pval, pout, assocp));
			else if (new_print(ptype, *pval, pout, assocp) == SH_FAIL)
				return(SH_FAIL);
			break;
		};
	case TYPE_ARRAY:
		{
			struct strhead *ptype = ((struct arraystr *) strp)->typeptr;
			unchar *tmpptr = (unchar *) pval;
			ulong length;
			register uint i;

			if (ptype->size == 1)
				return(call_charprint(strp, pval, pout, assocp));
			if (assocp)
				get_int(assocp->type->size, assocp->pval, &length);
			else
				length = strp->size / ptype->size;
			outprintf(pout, "{ ");
			for (i = 0; i < length; i++) {
				if (i != 0)
					outprintf(pout, ", ");
				if (new_print(ptype, tmpptr, pout, NULL) == SH_FAIL)
					return(SH_FAIL);
				tmpptr += ptype->size;
			}
			outprintf(pout, " }");
			break;
		}
	case TYPE_DYNARRAY:
		{
			struct strhead *ptype = ((struct arraystr *) strp)->typeptr;
			unchar *tmpptr = (unchar *) *pval;
			register uint i;
			ulong dynlength;
			ulong max;

			if (!tmpptr) {
				outprintf(pout, "NULL");
				break;
			}
			if (ptype->size == 1)
				return(call_charprint(strp, pval, pout, assocp));
			if (strp->flags & F_EXTERNAL_LENGTH) {
				if (!assocp)
					return(SH_FAIL);
				get_int(assocp->type->size, assocp->pval, &dynlength);
			}
			outprintf(pout, "{ ");
			for (i = 0; ; i++) {
				if (strp->flags & F_EXTERNAL_LENGTH) {
					if (dynlength == i)
						break;
				}
				else {
					if (ptype->size == sizeof(char)) {
						if (!tmpptr[0])
							break;
					}
					else if (ptype->size == sizeof(short)) {
						if (*((ushort *) tmpptr) == 0)
							break;
					}
					else {
						if (*((ulong *) tmpptr) == 0)
							break;
					}
				}
				if (i != 0)
					outprintf(pout, ", ");
				if (new_print(ptype, tmpptr, pout, NULL) == SH_FAIL)
					return(SH_FAIL);
				tmpptr += ptype->size;
			}
			outprintf(pout, " }");
			break;
		};
	}
	return(SH_SUCC);
}

new_free(strp, pval, assocp)
struct strhead *strp;
VAL *pval;
struct assoc_field *assocp;
{
	if (strp->flags & F_SPECIAL) {
		;
		/* find_special, call special */
		/* if found, return; */
	}
	switch (strp->type) {
	case TYPE_INT:
		return;
	case TYPE_TYPEDEF:
		return(new_free(((struct typedefstr *) strp)->typeptr, pval,
			(struct assoc_field *)NULL));
	case TYPE_STRUCT:
		{
			int i;
			struct fieldstr *fldp = ((struct structstr *) strp)->fields;
			struct assoc_field tmp_assoc, *pass_assoc;

			for (i = 0; fldp[i].name; i++) {
				if (((fldp[i].typeptr->type == TYPE_UNION) && (fldp[i].typeptr->flags & F_EXTERNAL_TAG)) ||
						((fldp[i].typeptr->type == TYPE_DYNARRAY) && (fldp[i].typeptr->flags & F_EXTERNAL_LENGTH))) {
					pass_assoc = &tmp_assoc;
					tmp_assoc.type = fldp[fldp[i].assoc_field_num].typeptr;
					tmp_assoc.pval = (VAL *) ((unchar *) pval) + fldp[fldp[i].assoc_field_num].offset;
				}
				else
					pass_assoc = NULL;
				new_free(fldp[i].typeptr, ((unchar *) pval) + fldp[i].offset, pass_assoc);
			}
			break;
		}
	case TYPE_UNION:
		{
			int i;
			struct union_fieldstr *fldp = ((struct unionstr *) strp)->fields;
			ulong choice;

			if (strp->flags & F_EXTERNAL_TAG) {
				if (!assocp)
					return(SH_FAIL);
				get_int(assocp->type->size, assocp->pval, &choice);
			}
			else
				get_int(((struct unionstr *) strp)->choice_size, ((unchar *) pval) + ((struct unionstr *) strp)->choice_offset, &choice);
			for (i = 0; fldp[i].name; i++)
				if (!(fldp[i].flags & F_NO_TAG) && (fldp[i].tag == choice))
					break;
			if (fldp[i].name)
				new_free(fldp[i].typeptr, pval, NULL);
			break;
		};
	case TYPE_POINTER:
		{
			struct strhead *ptype = ((struct ptrstr *) strp)->typeptr;

			if (!*pval)
				break;
			new_free(ptype, *pval, assocp);
			free_pointer(*pval);
			break;
		};
	case TYPE_ARRAY:
		{
			struct strhead *ptype = ((struct arraystr *) strp)->typeptr;
			unchar *tmpptr = (unchar *) pval;
			register uint i;

			for (i = 0; (ptype->size * i) < strp->size; i++) {
				new_free(ptype, tmpptr, NULL);
				tmpptr += ptype->size;
			}
			break;
		}
	case TYPE_DYNARRAY:
		{
			struct strhead *ptype = ((struct arraystr *) strp)->typeptr;
			unchar *tmpptr = (unchar *) *pval;
			register uint i;
			ulong dynlength;
			ulong max;

			if (!tmpptr)
				break;
			if (strp->flags & F_EXTERNAL_LENGTH) {
				if (!assocp)
					return(SH_FAIL);
				get_int(assocp->type->size, assocp->pval, &dynlength);
			}
			for (i = 0; ; i++) {
				if (strp->flags & F_EXTERNAL_LENGTH) {
					if (dynlength == i)
						break;
				}
				else {
					if (ptype->size == sizeof(char)) {
						if (!tmpptr[0])
							break;
					}
					else if (ptype->size == sizeof(short)) {
						if (*((ushort *) tmpptr) == 0)
							break;
					}
					else {
						if (*((ulong *) tmpptr) == 0)
							break;
					}
				}
				new_free(ptype, tmpptr, NULL);
				tmpptr += ptype->size;
			}
			free_pointer(*pval);
			break;
		};
	}
	return(SH_SUCC);
}

#define PREC_TOP 6

static char *binops = "*/%+-&^|";
static char precedence[] = { PREC_TOP-1, PREC_TOP-1, PREC_TOP-1, PREC_TOP-2, PREC_TOP-2, PREC_TOP-3, PREC_TOP-4, PREC_TOP-5 };
static char *unops = "!~-";

#define VALNEEDED 1
#define OPNEEDED 2

new_par_int(ppinp, dst)
unchar **ppinp;
ulong *dst;
{
	ulong tmp;

	if (!par_int(ppinp, 0, &tmp))
		return(SH_FAIL);
	*dst = tmp;
	return(SH_SUCC);
}

static
par_int(ppinp, prec, pret)
unchar **ppinp;
int prec;
ulong *pret;
{
	ulong local = 0;
	int flag = VALNEEDED;
	const char open_paren = '(', close_paren = ')';
	char c;

	XK_SKIPWHITE(ppinp);
	while (1) {
		c = **ppinp;
		if (flag & VALNEEDED) {
			if (!c)
				return(0);
			if (c == open_paren) {
				(*ppinp)++;
				if (!par_int(ppinp, 0, pret) || (**ppinp != close_paren))
					return(0);
				(*ppinp)++;
			}
			else if (isalpha(**ppinp)) {
				if (!getsymval(ppinp, pret))
					return(0);
			}
			else if (isdigit(**ppinp)) {
				if (!getintval(ppinp, pret))
					return(0);
			}
			else if (strchr(unops, c)) {
				(*ppinp)++;
				if (!par_int(ppinp, PREC_TOP, pret))
					return(0);
				switch(c) {
				case '!':
					*pret= !(*pret);
					break;
				case '-':
					*pret = (ulong) (-((long) *pret));
					break;
				case '~':
					*pret = ~(*pret);
				}
			}
			else
				return(0);
			if (prec == PREC_TOP)
				return(1);
			flag = OPNEEDED;
		}
		else {
			char *p;
			ulong nextval;

			/*
			** In the old parser, an open-parentheses in this context
			** was a comment (most likely an alternative statement of
			** the value - like the hex version).  We will not put this
			** back in, unless requested to.
			*/
			if (!c || (c == close_paren) || !(p = strchr(binops, c)) ||  (precedence[p - binops] <= prec))
				return(1);
			(*ppinp)++;
			if (!par_int(ppinp, precedence[p - binops], &nextval))
				return(0);
			switch(c) {
			case '+':
				*pret += nextval;
				break;
			case '-':
				*pret -= nextval;
				break;
			case '*':
				*pret *= nextval;
				break;
			case '/':
				*pret /= nextval;
				break;
			case '%':
				*pret %= nextval;
				break;
			case '|':
				*pret |= nextval;
				break;
			case '&':
				*pret &= nextval;
				break;
			case '^':
				*pret ^= nextval;
				break;
			}
		}
		XK_SKIPWHITE(ppinp);
	}
}

getintval(ppinp, pret)
char **ppinp;
ulong *pret;
{
	ulong base;

	base = strtoul(*ppinp, ppinp, 0);
	if (**ppinp == '#') {
		if (((base == 16) && strchr("abcdef", **ppinp)) || isdigit(**ppinp)) {
			*pret = strtoul(*ppinp, ppinp, base);
			return(1);
		}
		else
			return(0);
	}
	*pret = base;
	return(1);
}

getsymval(ppinp, pret)
char **ppinp;
ulong *pret;
{
	char defname[80], *p = defname;

	while (isvarchar(**ppinp))
		*p++ = (*ppinp)++[0];
	*p = '\0';
	if (!fdef(defname, pret))
		return(0);
	return(1);
}

outprintf(pout, fmt, arg1, arg2, arg3, arg4, arg5)
struct easyio *pout;
char *fmt;
ulong arg1, arg2, arg3, arg4, arg5;
{
	if (!pout)
		altprintf(fmt, arg1, arg2, arg3, arg4, arg5);
	else if (pout->type == EZ_FD)
		altfprintf(pout->fd_or_buf.fd, fmt, arg1, arg2, arg3, arg4, arg5);
	else {
		char buf[80];

		lsprintf(buf, fmt, arg1, arg2, arg3, arg4, arg5);
		outputs(pout, buf);
		pout->fd_or_buf.buf->curspot--;
		pout->fd_or_buf.buf->buf[pout->fd_or_buf.buf->curspot] = '\0';
	}
}

outputs(pout, str)
struct easyio *pout;
char *str;
{
	if (!pout)
		ALTPUTS(str);
	else if (pout->type == EZ_FD)
		altfputs(pout->fd_or_buf.fd, str);
	else {
		struct buf *pbuf = pout->fd_or_buf.buf;
		int len = strlen(str);

		if (!pbuf->buf || ((len + 2) > (pbuf->buflen - pbuf->curspot))) {
			if (pbuf->buf) {
				pbuf->buf = stakbgrow(pbuf->buf, pbuf->buflen, 1000);
				pbuf->buflen += 1000;
			}
			else {
				pbuf->buf = stakalloc(100);
				pbuf->curspot = 0;
				pbuf->buflen = 100;
			}
		}
		memcpy(pbuf->buf + pbuf->curspot, str, len); /* includes '\0' */
		pbuf->curspot += len;
		pbuf->buf[pbuf->curspot++] = '\n';
		pbuf->buf[pbuf->curspot] = '\0';
	}
}

outputc(pout, c)
struct easyio *pout;
char c;
{
	if (!pout)
		altfputchar(1, c);
	else if (pout->type == EZ_FD)
		altfputchar(pout->fd_or_buf.fd, c);
	else {
		struct buf *pbuf = pout->fd_or_buf.buf;

		if (!pbuf->buf || (pbuf->curspot == pbuf->buflen)) {
			if (pbuf->buf) {
				pbuf->buf = stakbgrow(pbuf->buf, pbuf->buflen, 1000);
				pbuf->buflen += 1000;
			}
			else {
				pbuf->buf = stakalloc(100);
				pbuf->buflen = 100;
				pbuf->curspot = 0;
			}
		}
		pbuf->buf[pbuf->curspot++] = c;
	}
}

new_prin_int(pout, val)
struct easyio *pout;
ulong val;
{
	sub_prin_int(pout, val, NULL);
	return(SH_SUCC);
}

static int
sub_prin_int(pout, val, sym)
struct easyio *pout;
ulong val;
struct symlist *sym;
{
	register int i, printed = 0;

	if (sym && (Pr_format & PRSYMBOLIC)) {
		if (sym->isflag) {
			if (val == 0) {
				outprintf(pout, "0");
				return;
			}
			for (i = 0; i < sym->nsyms; i++) {
				if (sym->syms[i].addr & val) {
					if (Pr_format & PRMIXED_SYMBOLIC) {
						if (Pr_format & PRDECIMAL)
							outprintf(pout, "%s%s(%d)", printed ? "|" : "", sym->syms[i].str, sym->syms[i].addr);
						else
							outprintf(pout, "%s%s(0x%x)", printed ? "|" : "", sym->syms[i].str, sym->syms[i].addr);
					}
					else
						outprintf(pout, "%s%s", printed ? "|" : "", sym->syms[i].str);
					val &= ~(sym->syms[i].addr);
					printed++;
				}
			}
			if (val) {
				if (Pr_format & PRMIXED_SYMBOLIC) {
					if (Pr_format & PRDECIMAL)
						outprintf(pout, "%sNOSYMBOLIC(%d)", printed ? "|" : "", val);
					else
						outprintf(pout, "%sNOSYMBOLIC(0x%x)", printed ? "|" : "", val);
				}
				else {
					if (Pr_format & PRDECIMAL)
						outprintf(pout, "%s%d", printed ? "|" : "", val);
					else
						outprintf(pout, "%s0x%x", printed ? "|" : "", val);
				}
			}
			return;
		}
		else {
			for (i = 0; i < sym->nsyms; i++) {
				if (sym->syms[i].addr == val) {
					if (Pr_format & PRMIXED_SYMBOLIC) {
						if (Pr_format & PRDECIMAL)
							outprintf(pout, "%s(%d)", sym->syms[i].str, val);
						else
							outprintf(pout, "%s(0x%x)", sym->syms[i].str, val);
					}
					else
						outprintf(pout, "%s", sym->syms[i].str);
					return;
				}
			}
		}
	}
	if (Pr_format & PRHEX)
		outprintf(pout, "0x%x", val);
	else if (Pr_format & PRDECIMAL)
		outprintf(pout, "%d", val);
	else
		outprintf(pout, "%d(0x%x)", val, val);
}

set_int(size, pval, lval)
ulong size;
VAL *pval;
ulong lval;
{
	switch (size) {
	case sizeof(short):
		*((ushort *) pval) = lval;
		break;
	case sizeof(long):
		*((ulong *) pval) = lval;
		break;
	case sizeof(char):
		*((unchar *) pval) = lval;
		break;
	}
}

get_int(size, pval, lvalp)
ulong size;
VAL *pval;
ulong *lvalp;
{
	switch (size) {
	case sizeof(short):
		*lvalp = *((ushort *) pval);
		break;
	case sizeof(long):
		*lvalp = *((ulong *) pval);
		break;
	case sizeof(char):
		*lvalp = *((unchar *) pval);
		break;
	}
}

xk_Strncmp(s1, s2, len)
char *s1, *s2;
int len;
{
	int diff, i;

	for (i=0; i < len && s1[i] != '\0' && s2[i] != '\0'; i++)
		if ((diff = tolower(s1[i]) - tolower(s2[i])) != 0)
			return (diff);
	return(i == len ? 0 : s1[i] - s2[i]);
}

static
par_pointer(ppinp, pval)
char **ppinp;
VAL *pval;
{
	if ((*ppinp)[0] == '&') {
		char *start;

		(*ppinp)++;
		for (start = *ppinp; isvarchar(*ppinp[0]); (*ppinp)++)
			;
		if ((((ulong *) pval)[0] = fsym(start, -1)) == NULL)
			return(SH_FAIL);
	}
	else {
		(*ppinp)++;
		if (new_par_int(ppinp, pval) == SH_FAIL)
			return(SH_FAIL);
	}
	if (Ndont == Sdont) {
	if (Mall_func == stakalloc)
		return(SH_SUCC);
		if (Dont)
			Dont = (char **) realloc(Dont, (Sdont + 20) * sizeof(char *));
		else
			Dont = (char **) malloc((Sdont + 20) * sizeof(char *));
		if (!Dont) {
			ALTPUTS("Out of space, exiting");
			exit(1);
		}
		Sdont += 20;
	}
	Dont[Ndont++] = (char *) (pval[0]);
	return(SH_SUCC);
}

free_pointer(p)
char *p;
{
	int i;

	for (i = Ndont - 1; i >= 0; i--) {
		if (Dont[i] == p) {
			for ( ; i < Ndont - 1; i++)
				Dont[i] = Dont[i + 1];
			Ndont--;
			return;
		}
	}
	(*Free_func)(p);
}

call_charparse(strp, ppinp, pval, assocp)
struct strhead *strp;
char **ppinp;
VAL *pval;
struct assoc_field *assocp;
{
	int len;

	switch(strp->type) {
	case TYPE_ARRAY:
		if (new_par_chararr(ppinp, pval, strp->size, &len) == SH_FAIL)
			return(SH_FAIL);
		break;
	case TYPE_DYNARRAY:
	case TYPE_POINTER:
		if (new_par_charptr(ppinp, pval, &len) == SH_FAIL)
			return(SH_FAIL);
	}
	if (assocp)
		set_int(assocp->type->size, assocp->pval, len);
	return(SH_SUCC);
}

call_charprint(strp, pval, pout, assocp)
struct strhead *strp;
VAL *pval;
struct easyio *pout;
struct assoc_field *assocp;
{
	uint len;

	switch(strp->type) {
	case TYPE_ARRAY:
		if (assocp)
			get_int(assocp->type->size, assocp->pval, &len);
		else
			for (len = 0; (len < strp->size) && (((char *) pval)[len]); len++)
				;
		if (new_prin_charstr(pout, pval, len) == SH_FAIL)
			return(SH_FAIL);
		break;
	case TYPE_DYNARRAY:
	case TYPE_POINTER:
		if (assocp)
			get_int(assocp->type->size, assocp->pval, &len);
		else
			len = *pval ? strlen((char *) *pval) : 0;
		if (new_prin_charstr(pout, *pval, len) == SH_FAIL)
			return(SH_FAIL);
	}
	return(SH_SUCC);
}
