/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

%{
#ident	"@(#)mail:common/cmd/mail/rewrite.y	1.1.3.11"
/*
    NAME
	have_rewrite_function - check if a mailR function has been defined

    SYNOPSIS
	int have_rewrite_function(const char *function, const char *filename)

    DESCRIPTION
	Determine if the function with the given name exists.
	This is used by mail to see if a mailR call should be made.

    NAME
	invoke_rewrite - invoke a mailR function

    SYNOPSIS
	void invoke_rewrite(const char *function, Msg *pmsg, Hdrinfo *pin_hdrinfo;
	    Hdrinfo *pout_hdrinfo, const char *filename, Tmpfile *ptmpfile)

    DESCRIPTION
	Invoke the given mailR function on the given message, reading the header
	from "pin_hdrinfo" and writing the new header to "pout_hdrinfo". Use
	the mailR functions found in "filename".

    NAME
	set_recipients - initialize recipient list

    SYNOPSIS
	void mailR_set_recipients(char **argv)

    DESCRIPTION
	mailR_set_recipients() primes the array which will be used when
	the mailR "recipients()" function is invoked.
*/
#include "mail.h"
#include "r822.h"

/* VOID* will be void* or char* */
#if defined(__STDC__) || defined(c_plusplus) || defined(__cplusplus)
# define VOID void
#else
# define VOID char
#endif

/* Try to reduce yacc namespace problems */
#define yy_yys yy_yys_mailr
#define yy_yyv yy_yyv_mailr
#define yyact yyact_mailr
#define yychar yychar_mailr
#define yychk yychk_mailr
#define yydebug yydebug_mailr
#define yydef yydef_mailr
#define yyerrflag yyerrflag_mailr
#define yyerror yyerror_mailr
#define yyexca yyexca_mailr
#define yylex yylex_mailr
#define yylval yylval_mailr
#define yynerrs yynerrs_mailr
#define yypact yypact_mailr
#define yyparse yyparse_mailr
#define yypgo yypgo_mailr
#define yyps yyps_mailr
#define yypv yypv_mailr
#define yyr1 yyr1_mailr
#define yyr2 yyr2_mailr
#define yyreds yyreds_mailr
#define yys yys_mailr
#define yystate yystate_mailr
#define yytmp yytmp_mailr
#define yytoks yytoks_mailr
#define yyv yyv_mailr
#define yyval yyval_mailr
/* end yacc namespace redefinitions */

#define YYDEBUG 1
#undef CTRL
#define CTRL(x) ((x) ^ 0100)

#define TABSIZE 8

typedef struct TreeNode TreeNode;
typedef struct Value Value;
typedef struct ArrayElement ArrayElement;
typedef struct VarReference VarReference;
typedef struct Symbol Symbol;
typedef struct SymbolTable SymbolTable;
typedef enum MS_Gotos MS_Gotos;
typedef enum UpToDate UpToDate;
typedef enum VarRefType VarRefType;

enum MS_Gotos { MS_return, MS_continue, MS_break, MS_next };
static const char *MS_tokenlist[] = { "MS_return", "MS_continue", "MS_break", "MS_next" };

/*
    All statements and expressions are kept in a tree.
    The type is one of the yacc token types. The other
    members are only used by some of the tree types.
*/
struct TreeNode
{
    TreeNode *left, *right, *middle, *middle2, *next;
    int type;
    Value *val;
    string *name;
    SymbolTable *table;
    MS_Gotos (*function0) ARGS((VarReference*));
    MS_Gotos (*functionn) ARGS((VarReference*, VarReference*));
    int argcount;
};
static TreeNode *TreeNode_new ARGS((TreeNode *left, TreeNode *right, int type));
static TreeNode *TreeNode_new__table ARGS((TreeNode *left, TreeNode *right, int type));
static TreeNode *TreeNode_new__middle ARGS((TreeNode *left, TreeNode *right, TreeNode *middle, int type));
static TreeNode *TreeNode_new__middle_middle2 ARGS((TreeNode *left, TreeNode *right, TreeNode *middle, TreeNode *middle2, int type));
static TreeNode *TreeNode_new__name ARGS((TreeNode *left, TreeNode *right, string *name, int type));
static void TreeNode_delete ARGS((TreeNode *top));
static MS_Gotos TreeNode_eval_function ARGS((TreeNode *top, VarReference *parameters, VarReference *ret));
static MS_Gotos TreeNode_eval ARGS((TreeNode *top, VarReference *v, SymbolTable *st, int type, int scopelevel, int continue_break_flag));
static TreeNode *find_function_TreeNode ARGS((const char *function, int complain));
static void TreeNode_dump ARGS((TreeNode *top, int level));

/*
    A string/double dynamic value is kept in a Value.
    At any given time, it will have no value, a string value,
    a double value, or both may be current. When evaluating
    for a given type, and that type is out-of-date, its value
    will be updated. A variable with no value is equivalent
    to one with a string value of "" and a double value of 0.
*/
struct Value
{
    string *s;
    double d;
    enum UpToDate { UTD_Neither, UTD_String, UTD_Double, UTD_Both } utd_type;
};
static Value *Value_new__string_double ARGS((string *s, double d));
static Value *Value_new ARGS((void));
static void Value_delete ARGS((Value *v));
static void Value_print ARGS((FILE *out, Value *ae));
static int Value_true ARGS((Value *v));
static Value *Value_copy ARGS((Value *v));
static double Value_get__double ARGS((Value *v));
static string *Value_get__string ARGS((Value *v));
static void Value_update__double ARGS((Value *v));
static void Value_update__string ARGS((Value *v));
static void Value_assign__double ARGS((Value *v, double d));
static void Value_assign__string ARGS((Value *v, string *s));

/*
    Arrays are kept in a vector of ArrayElements. The index is kept
    in "name", and its value in "vr". Because "name" is a string, this
    means that this is an associative array, as in awk.
*/
struct ArrayElement
{
    string *name;
    VarReference *vr;
};
static ArrayElement *ArrayElement_new ARGS((string *name, VarReference *vr));
static void ArrayElement_delete ARGS((ArrayElement *ae));
static void ArrayElement_print ARGS((FILE *out, ArrayElement *ae));
static int ArrayElement_true ARGS((ArrayElement *ae));
static ArrayElement *ArrayElement_copy ARGS((ArrayElement *ae));
static double ArrayElement_get__double ARGS((ArrayElement *ae));
static string *ArrayElement_get__hdrtag ARGS((ArrayElement *ae));
static string *ArrayElement_get__string ARGS((ArrayElement *ae));
static UpToDate ArrayElement_get__Value_type ARGS((ArrayElement *ae));

/*
    MailR variables are of type VarReference. These are dynamically
    switched between being a Value, a header (a Value with a name),
    a reference to another variable, or an array of ArrayElements (which
    in turn point to a VarReference).
*/
struct VarReference
{
    enum VarRefType { VR_None, VR_Value, VR_Reference, VR_Array, VR_Header } vr_type;
    Value *v;
    VarReference *vr;
    ArrayElement **array;
    int arraycount;
    string *hdrtag;
    int hdrtype;
};
static VarReference *VarReference_new ARGS((void));
static VarReference *VarReference_Array_new ARGS((void));
static VarReference *VarReference_new__Value ARGS((Value *v));
static VarReference *VarReference_new__Header ARGS((Value *v, const char *hdrtag, int hdrtype));
static void VarReference_delete ARGS((VarReference *vr));
static void VarReference_print ARGS((FILE *out, VarReference *vr));
static int VarReference_true ARGS((VarReference *vr));
static void VarReference_assign__double ARGS((VarReference *vr, double d));
static void VarReference_assign__charstring ARGS((VarReference *vr, const char *s));
static void VarReference_assign__string ARGS((VarReference *vr, string *s));
static void VarReference_assign__reference ARGS((VarReference *vr, VarReference *v));
static void VarReference_assign__VarReference ARGS((VarReference *vl, VarReference *vr));
static void VarReference_assign__Value ARGS((VarReference *vr, Value *vfrom));
static void VarReference_clear_value_and_reference ARGS((VarReference *vr));
static void VarReference_clear_value ARGS((VarReference *vr));
static VarReference *VarReference_copy ARGS((VarReference *vr));
static double VarReference_get__double ARGS((VarReference *vr));
static string *VarReference_get__hdrtag ARGS((VarReference *vr));
static string *VarReference_get__string ARGS((VarReference *vr));
static UpToDate VarReference_get__Value_type ARGS((VarReference *vr));
static VarReference *VarReference_Array_set ARGS((VarReference *vr));
static void VarReference_Array_grow ARGS((VarReference *vr, int additional_count));
static VarReference *VarReference_Array_lookup__VarReference ARGS((VarReference *l, VarReference *r));
static void VarReference_Array_add__double_string ARGS((VarReference *l, double index, string *s));
static void VarReference_Array_add__string_string ARGS((VarReference *l, string *index, string *s));
static void VarReference_Array_add__charstring_string ARGS((VarReference *l, const char *index, string *s));
static void VarReference_Array_add__double_header ARGS((VarReference *l, double index, string *value, const char *hdrtag, int hdrtype));
static void VarReference_Array_add__double_Value ARGS((VarReference *l, double index, Value *v));
static void VarReference_Array_add__double_VarReference ARGS((VarReference *l, double index, VarReference *vr));

/*
    mailR variables are kept in a symbol table. Each symbol
    in the table has a name and a VarReference.
*/
struct Symbol
{
    string *name;
    VarReference *v;
    Symbol *next;
};
static Symbol *Symbol_new ARGS((string *name, VarReference *v, Symbol *next));
static void Symbol_delete ARGS((Symbol *s));

/*
    All mailR variables are kept in a symbol table.
    Each compound statement has its own symbol table whose contents
    come and go as the compound statement is entered and exited.
    Each symbol table contains a list of Symbol's, and a pointer to
    the symbol table at the next scope out from the current compound
    statement scope.
*/
struct SymbolTable
{
    Symbol *table;
    SymbolTable *parent;
};
static SymbolTable *SymbolTable_new ARGS((void));
static void SymbolTable_delete ARGS((SymbolTable *st));
static void SymbolTable_clear ARGS((SymbolTable *st));
static void SymbolTable_set__parent ARGS((SymbolTable *st, SymbolTable *parent));
static VarReference *SymbolTable_lookup__VarReference ARGS((SymbolTable *st, string *name));
static VarReference *SymbolTable_add__VarReference ARGS((SymbolTable *st, string *name));

/*
    All built-in functions are kept in an array (sorted by name).
    Depending on the number of arguments permitted for the function,
    there will be a pointer to either a C function that takes a parameter
    list, or a C function that doesn't. When the function is called
    for the first time, a TreeNode representing the function will be created.
*/
struct builtin_functions
{
    const char *name;
    MS_Gotos (*function0) ARGS((VarReference *ret));
    MS_Gotos (*functionn) ARGS((VarReference *parameters, VarReference *ret));
    int argcount;
    TreeNode *treeptr;
};

extern int yyparse ARGS((void));
static int string_match ARGS((string *str, string *pat, int maptolowercase));
static string *double_to_string ARGS((double d));
static string *set_continuation_lines ARGS((string *value));
static void invoke ARGS((const char *function));

extern int yydebug;

/* Variables used within yyparse and while invoking a mailr function. */
/* These must be global due to yacc constraints. */
static VarReference *mailr_allheaders = 0;	/* where headers are kept during processing */
static Msg *mailr_pmsg = 0;			/* the current message being worked on */
static FILE *msg_tmpfile = 0;			/* the current message body */
static TreeNode *mailr_treetop = 0;		/* the list of all mailR functions */
static const char *mailr_errormessage = 0;	/* a message about something that went wrong */
static int mailr_debug = 0;			/* can be turned on with $MAILR_DEBUG */
static FILE *mailr_fp = 0;			/* the mailR input stream */

/* variables used to maintain the file information in a stack */
typedef struct file_info
    {
    string *filename;			/* the file containing the mailR functions */
    int lastcolumn;			/* where the previous token started */
    int column;				/* where the current character is */
    int linenumber;			/* where the current character is */
    int token_column;			/* where the current token started */
    int token_linenumber;		/* where the current token started */
    long curoffset;			/* where we left off when pushing the stack */
    } file_info;

#define MAXSTACK 15
static file_info input_stack[MAXSTACK];	/* the input file and include files */
static int input_stacklevel = -1;	/* which file are we on right now? */

static void pushfile ARGS((string *filename,int complain));
static int popfile ARGS((void));

%}

/* this declares the yacc variable yylval */
%union
{
    string *yString;
    double yNumber;
    TreeNode *yTreeNode;
}

%token TError TVarName TStringConstant TNumberConstant TPlus TMinus TStar
%token TSlash TComma TAt TGT TGE TLT TLE TEq TEqEq TNot TNE TMatch TNMatch
%token TAND TOR TSemi TOpCurly TClCurly TOpBracket TClBracket TOpParen TClParen
%token Tbreak Tcontinue Telse Tfor Tfunction TCfunction Tgenerator Tgeneratorcall
%token Tif Tin Tfrom Treference Treturn Tstart Tuntil Tvar Twhile TDot Tname
%token Tcount TvarDecl TUMinus Tinclude

%type <yString>	TVarName
%type <yTreeNode> and_expr assignment at_expr break_stmt compound_statement continue_stmt
%type <yTreeNode> declaration equal_expr for_stmt func_or_gen function_list if_stmt muldivexpr
%type <yTreeNode> or_expr parm_list plusminus_expr primary program rel_expr
%type <yTreeNode> return_stmt simple_statement start_statement statement
%type <yTreeNode> statement_list until_stmt vardecl vardecl_list var_reference while_stmt
%type <yTreeNode> Vparmdecl_list VRparmdecl_list Vparmdecl VRparmdecl assignment_or_EMPTY
%%
program:	function_list
	{ mailr_treetop = $1; }
    |	EMPTY
	{ mailr_treetop = TreeNode_new((TreeNode*)0, (TreeNode*)0, Tinclude); }
	;

function_list:
	func_or_gen function_list
	{ $1->next = $2; $$ = $1; }
    |	func_or_gen
	{ $$ = $1; }
	;

func_or_gen:
	Tfunction TVarName TOpParen VRparmdecl_list TClParen compound_statement
	{   /* it would be nice to be able to check if $2 redefines a function name */
	    $$ = TreeNode_new($4, $6, Tfunction); $$->name = $2;
	}
    |	Tgenerator TVarName TOpParen Vparmdecl_list TClParen compound_statement
	{   /* it would be nice to be able to check if $2 redefines a function name */
	    $$ = TreeNode_new($4, $6, Tgenerator); $$->name = $2;
	}
    |	Tinclude TStringConstant
	{
	    $$ = TreeNode_new((TreeNode*)0, (TreeNode*)0, Tinclude);
	    pushfile($<yString>2, 1);
	}
	;

VRparmdecl_list:
	VRparmdecl TComma VRparmdecl_list
	{ $1->next = $3; $$ = $1; }
    |	VRparmdecl
	{ $$ = $1; }
    |	EMPTY
	{ $$ = 0; }
	;

Vparmdecl_list:
	Vparmdecl TComma Vparmdecl_list
	{ $1->next = $3; $$ = $1; }
    |	Vparmdecl
	{ $$ = $1; }
    |	EMPTY
	{ $$ = 0; }
	;

EMPTY: ;

VRparmdecl:
	Treference TVarName
	{ $$ = TreeNode_new((TreeNode*)0, (TreeNode*)0, Treference); $$->name = $2; }
    |	Vparmdecl
	{ $$ = $1; }
	;

Vparmdecl:
	Tvar TVarName
	{ $$ = TreeNode_new((TreeNode*)0, (TreeNode*)0, Tvar); $$->name = $2; }
	;

statement_list:
	statement statement_list
	{ $1->next = $2; $$ = $1; }
    |	statement
	{ $$ = $1; }
	;

statement:
	if_stmt
	{ $$ = $1; }
    |	while_stmt
	{ $$ = $1; }
    |	until_stmt
	{ $$ = $1; }
    |	for_stmt
	{ $$ = $1; }
    |	start_statement
	{ $$ = $1; }
    |	return_stmt
	{ $$ = $1; }
    |	break_stmt
	{ $$ = $1; }
    |	continue_stmt
	{ $$ = $1; }
    |	declaration
	{ $$ = $1; }
    |	simple_statement
	{ $$ = $1; }
    |	compound_statement
	{ $$ = $1; }
	;

compound_statement:
	TOpCurly statement_list TClCurly
	{ $$ = TreeNode_new__table($2, (TreeNode*)0, TOpCurly); }
    |	TOpCurly TClCurly
	{ $$ = TreeNode_new__table((TreeNode*)0, (TreeNode*)0, TOpCurly); }
	;

if_stmt:
	Tif TOpParen assignment TClParen statement
	{ $$ = TreeNode_new($3, $5, Tif); }
    |	Tif TOpParen assignment TClParen statement Telse statement
	{ $$ = TreeNode_new__middle($3, $5, $7, Tif); }
	;

while_stmt:
	Twhile TOpParen assignment TClParen statement
	{ $$ = TreeNode_new($3, $5, Twhile); }
	;

until_stmt:
	Tuntil TOpParen assignment TClParen statement
	{ $$ = TreeNode_new($3, $5, Tuntil); }
	;

for_stmt:
	Tfor TOpParen assignment_or_EMPTY TSemi assignment_or_EMPTY TSemi assignment_or_EMPTY TClParen statement
	{ $$ = TreeNode_new__middle_middle2($3, $5, $7, $9, Tfor); }
    |	Tfor TOpParen TVarName Tin var_reference TClParen statement
	{ $$ = TreeNode_new__name($5, $7, $3, Tin); }
    |	Tfor TOpParen TVarName Tfrom var_reference TClParen statement
	{ $$ = TreeNode_new__name($5, $7, $3, Tfrom); }
	;

start_statement:
	Tstart compound_statement
	{ $$ = TreeNode_new($2, (TreeNode*)0, Tstart); }
    |	Tstart simple_statement
	{ $$ = TreeNode_new($2, (TreeNode*)0, Tstart); }
	;

return_stmt:
	Treturn assignment_or_EMPTY TSemi
	{ $$ = TreeNode_new($2, (TreeNode*)0, Treturn); }
	;

break_stmt:
	Tbreak TSemi
	{ $$ = TreeNode_new((TreeNode*)0, (TreeNode*)0, Tbreak); }
	;

continue_stmt:
	Tcontinue TSemi
	{ $$ = TreeNode_new((TreeNode*)0, (TreeNode*)0, Tcontinue); }
	;

declaration:
	Tvar vardecl_list TSemi
	{ $$ = $2; }

vardecl_list:
	vardecl TComma vardecl_list
	{ $1->right = $3; $$ = $1; }
    |	vardecl
	{ $$ = $1; }
	;

vardecl:
	TVarName TEq at_expr
	{ $$ = TreeNode_new__name($3, (TreeNode*)0, $1, TvarDecl); }
    |	TVarName
	{ $$ = TreeNode_new__name((TreeNode*)0, (TreeNode*)0, $1, TvarDecl); }
	;

simple_statement:
	assignment_or_EMPTY TSemi
	{ $$ = $1; }
	;

assignment:
	var_reference TEq assignment
	{ $$ = TreeNode_new($1, $3, TEq); }
    |	at_expr
	{ $$ = $1; }
	;

assignment_or_EMPTY:
	assignment
	{ $$ = $1; }
    |	EMPTY
	{ $$ = TreeNode_new((TreeNode*)0, (TreeNode*)0, TEq); }
	;

at_expr:
	at_expr TAt or_expr
	{ $$ = TreeNode_new($1, $3, TAt); }
    |	or_expr
	{ $$ = $1; }
	;

or_expr:
	or_expr TOR and_expr
	{ $$ = TreeNode_new($1, $3, TOR); }
    |	and_expr
	{ $$ = $1; }
	;

and_expr:
	and_expr TAND equal_expr
	{ $$ = TreeNode_new($1, $3, TAND); }
    |	equal_expr
	{ $$ = $1; }
	;

equal_expr:
	equal_expr TEqEq rel_expr
	{ $$ = TreeNode_new($1, $3, TEqEq); }
    |	equal_expr TNE rel_expr
	{ $$ = TreeNode_new($1, $3, TNE); }
    |	equal_expr TMatch rel_expr
	{ $$ = TreeNode_new($1, $3, TMatch); }
    |	equal_expr TNMatch rel_expr
	{ $$ = TreeNode_new($1, $3, TNMatch); }
    |	rel_expr
	{ $$ = $1; }
	;

rel_expr:
	rel_expr TLT plusminus_expr
	{ $$ = TreeNode_new($1, $3, TLT); }
    |	rel_expr TLE plusminus_expr
	{ $$ = TreeNode_new($1, $3, TLE); }
    |	rel_expr TGT plusminus_expr
	{ $$ = TreeNode_new($1, $3, TGT); }
    |	rel_expr TGE plusminus_expr
	{ $$ = TreeNode_new($1, $3, TGE); }
    |	plusminus_expr
	{ $$ = $1; }
	;

plusminus_expr:
	plusminus_expr TPlus muldivexpr
	{ $$ = TreeNode_new($1, $3, TPlus); }
    |	plusminus_expr TMinus muldivexpr
	{ $$ = TreeNode_new($1, $3, TMinus); }
    |	muldivexpr
	{ $$ = $1; }
	;

muldivexpr:
	muldivexpr TStar primary
	{ $$ = TreeNode_new($1, $3, TStar); }
    |	muldivexpr TSlash primary
	{ $$ = TreeNode_new($1, $3, TSlash); }
    |	primary
	{ $$ = $1; }
	;

primary:
	TNumberConstant
	{
	    TreeNode *t = TreeNode_new((TreeNode*)0, (TreeNode*)0, TNumberConstant);
	    t->val = Value_new__string_double((string*)0, yylval.yNumber);
	    $$ = t;
	}
    |	TStringConstant
	{
	    TreeNode *t = TreeNode_new((TreeNode*)0, (TreeNode*)0, TStringConstant);
	    t->val = Value_new__string_double(yylval.yString, 0.0);
	    $$ = t;
	}
    |	var_reference TDot Tname
	{ $$ = TreeNode_new($1, (TreeNode*)0, Tname); }
    |	var_reference TDot Tcount
	{ $$ = TreeNode_new($1, (TreeNode*)0, Tcount); }
    |	var_reference
	{ $$ = $1; }
    |	TMinus primary
	{ $$ = TreeNode_new($2, (TreeNode*)0, TUMinus); }
    |	TNot primary
	{ $$ = TreeNode_new($2, (TreeNode*)0, TNot); }
    |	TOpParen assignment TClParen
	{ $$ = $2; }

var_reference:
	TVarName TOpBracket assignment TClBracket
	{ $$ = TreeNode_new__name($3, (TreeNode*)0, $1, TOpBracket); }
    |	TVarName TOpParen parm_list TClParen
	{ $$ = TreeNode_new__name($3, (TreeNode*)0, $1, TOpParen); }
    |	TVarName
	{ $$ = TreeNode_new__name((TreeNode*)0, (TreeNode*)0, $1, TVarName); }
	;

parm_list:
	assignment TComma parm_list
	{ $1->next = $3; }
    |	assignment
	{ $$ = $1; }
    |	EMPTY
	{ $$ = 0; }
	;

%%

/* structure to hold a language keyword and its corresponding yacc token */
struct keywordmap
{
    const char *name;
    int token;
};

/* the mapping from a language keyword into its corresponding yacc token */
static const struct keywordmap keywordlist[] = {
    { "break", Tbreak },
    { "continue", Tcontinue },
    { "count", Tcount },
    { "else", Telse },
    { "for", Tfor },
    { "from", Tfrom },
    { "function", Tfunction },
    { "generator", Tgenerator },
    { "if", Tif },
    { "in", Tin },
    { "include", Tinclude },
    { "name", Tname },
    { "reference", Treference },
    { "return", Treturn },
    { "start", Tstart },
    { "until", Tuntil },
    { "var", Tvar },
    { "while", Twhile }
};

/* a function for use by bsearch to search for a keyword */
static int keywordmap_cmp(v1, v2)
const VOID *v1;
const VOID *v2;
{
    return strcmp(((struct keywordmap*)v1)->name, ((struct keywordmap*)v2)->name);
}

/*
    lookup the given string in the keyword list. If found, return the
    corresponding yacc token. Otherwise, return the token saying that it's
    a variable name.
*/
static int lookup_keyword(s)
string *s;
{
#define nelements(x) (sizeof(x) / sizeof(x[0]))

    struct keywordmap srchstring;
    struct keywordmap *ret;

    srchstring.name = s_to_c(s);
    ret = (struct keywordmap *) bsearch((char*)&srchstring, (char*)keywordlist, nelements(keywordlist), sizeof(keywordlist[0]), keywordmap_cmp);
    return ret ? ret->token : TVarName;
}

/* get a character, updating the mailr_linenumber and mailr_column along the way */
static int getch()
{
    int c = getc(mailr_fp);
    if (c == EOF)
	return EOF;

    input_stack[input_stacklevel].lastcolumn = input_stack[input_stacklevel].column;
    if (c == '\n')	/* newline */
	{
	input_stack[input_stacklevel].column = 1;
	input_stack[input_stacklevel].linenumber++;
	}

    else if (c == '\t')	/* tab goes to next TABSIZE boundary */
	input_stack[input_stacklevel].column = (input_stack[input_stacklevel].column - 1) / TABSIZE * TABSIZE + TABSIZE + 1;

    else
	input_stack[input_stacklevel].column++;
    if (mailr_debug > 1)
	(void) fprintf (stderr, "getch: %d, %d: '%c'\n",
	    input_stack[input_stacklevel].token_linenumber,
	    input_stack[input_stacklevel].token_column, c);
    return c;
}

/* return a character to the stream, updating the mailr_linenumber and mailr_column along the way */
static void ungetch(c)
int c;
{
    if (input_stack[input_stacklevel].column <= 1)
	input_stack[input_stacklevel].linenumber--;
    input_stack[input_stacklevel].column = input_stack[input_stacklevel].lastcolumn;
    (void) ungetc(c, mailr_fp);
    if (mailr_debug > 1) (void) fprintf (stderr, "ungetch: %d, %d: '%c'\n",
	input_stack[input_stacklevel].token_linenumber,
	input_stack[input_stacklevel].token_column, c);
}

/* map yacc tokens into a string. this is used for debugging */
static const char *yytokenlist[] = {
    "TError", "TVarName", "TStringConstant", "TNumberConstant", "TPlus",
    "TMinus", "TStar", "TSlash", "TComma", "TAt", "TGT", "TGE", "TLT", "TLE",
    "TEq", "TEqEq", "TNot", "TNE", "TMatch", "TNMatch", "TAND", "TOR", "TSemi",
    "TOpCurly", "TClCurly", "TOpBracket", "TClBracket", "TOpParen", "TClParen",
    "Tbreak", "Tcontinue", "Telse", "Tfor", "Tfunction", "TCfunction",
    "Tgenerator", "Tgeneratorcall", "Tif", "Tin", "Tfrom", "Treference",
    "Treturn", "Tstart", "Tuntil", "Tvar", "Twhile", "TDot", "Tname", "Tcount",
    "TvarDecl", "TUMinus", "Tinclude"
};

/* If debugging is turned on, report on the token being returned. */
/* Otherwise, just return the token. */
static int yylex_report(ret)
int ret;
{
    if (mailr_debug)
	{
	(void) fprintf (stderr, "yylex returns: %s\n", yytokenlist[ret - TError]);
	switch (ret)
	    {
	    case TVarName:
	    case TStringConstant: (void) fprintf (stderr, "\t'%s'\n", s_to_c(yylval.yString)); break;
	    case TNumberConstant: (void) fprintf (stderr, "\t'%g'\n", yylval.yNumber); break;
	    }
	}
    return ret;
}

/* check a character to see if it is a digit appropriate for an octal number */
static int isodigit(c)
int c;
{
    return isdigit(c) && (c < '8');
}

/* Read from mailr_fp and return a token for what's next on the input stream. */
/* Yacc requires to return 0 on EOF. Otherwise, return the yacc tokens such as TDot. */
static int yylex()
{
    int c;

    if (!mailr_fp)
	return 0;

    /* skip over comments and white space */
    for (;;)
	{
	/* remember where the token starts */
	input_stack[input_stacklevel].token_column = input_stack[input_stacklevel].column;
	input_stack[input_stacklevel].token_linenumber = input_stack[input_stacklevel].linenumber;

	/* if we hit the end of the file, there may be another file to continue */
	for (c = getch(); c == EOF; c = getch())
	    if (!popfile())
		return 0;

	if (c == '#')	/* skip comments, which stop at the end of the line */
	    {
	    while ((c = getch()) != EOF)
		if (c == '\n')
		    break;
	    }
	else if (!isspace(c))
	    break;	/* skip white space */
	}

    if (isalpha(c) || (c == '_'))
	{		/* start of keyword or variable name */
	string *s = s_new();
	(void) s_putc(s, c);
	while ((c = getch()) != EOF)
	    if (isalnum(c) || (c == '_'))
		(void) s_putc(s, c);
	    else
		{	/* finish off the string and return it in yylval */
		int ret;
		ungetch(c);
		s_terminate(s);
		ret = lookup_keyword(s);
		if (ret == TVarName)
		    yylval.yString = s;
		else
		    s_free(s);
		return yylex_report(ret);
		}
	mailr_errormessage = gettxt(":546","EOF found in name");
	return yylex_report(TError);
	}

    if (isdigit(c) || (c == '.'))		/* a number  000.000e+00  or '.' */
	{
	string *s;
	int svc = c;

	/* Differentiate between .[0-9] and .[A-Z], which is needed */
	/* for variable.name and variable.count. */
	if (c == '.')
	    {
	    c = getch();
	    ungetch(c);
	    if (!isdigit(c))
		return yylex_report(TDot);
	    }

	/* start saving the number */
	s = s_new();
	(void) s_putc(s, svc);
	c = svc;

	/* grab the digits before the "." */
	if (isdigit(c))
	    while ((c = getch()) != EOF)
		if (isdigit(c))
		    (void) s_putc(s, c);
		else
		    break;

	if ((c == '.') || (c == 'e') || (c == 'E'))
	    {
	    /* grab the digits after the "." */
	    if (c == '.')
		{
		(void) s_putc(s, c);
		while ((c = getch()) != EOF)
		    if (isdigit(c))
			(void) s_putc(s, c);
		    else
			break;
		}

	    /* grab the exponent */
	    if ((c == 'e') || (c == 'E'))
		{
		(void) s_putc(s, c);
		c = getch();
		if ((c == '+') || (c == '-'))
		    {
		    s_putc(s, c);
		    c = getch();
		    }
		while (isdigit(c))
		    {
		    s_putc(s, c);
		    c = getch();
		    }
		}
	    }

	/* the above reads 1 character too far, so put it back */
	if (!feof(mailr_fp))
	    ungetch(c);

	s_terminate(s);
	yylval.yNumber = atof(s_to_c(s));
	s_free(s);
	return yylex_report(TNumberConstant);
	}

    if (c == '"')		/* string constant */
	{
	string *s = s_new();
	while ((c = getch()) != EOF)
	    {
	    /* end of the string? */
	    if (c == '"')
		{
		s_terminate(s);
		yylval.yString = s;
		return yylex_report(TStringConstant);
		}

	    else if (c == '\\')		/* have to translate \X, \000, \x00 */
		{
		if ((c = getch()) == EOF)
		    {
		    mailr_errormessage = gettxt(":547","EOF found in string/character constant");
		    return yylex_report(TError);
		    }

		switch (c)
		    {
		    case 'a': s_putc(s, CTRL('G')); break;	/* \a == ^G == BEL */
		    case 'f': s_putc(s, CTRL('L')); break;	/* \f == ^L == FF */
		    case 'n': s_putc(s, CTRL('J')); break;	/* \n == ^J == LF */
		    case 'r': s_putc(s, CTRL('M')); break;	/* \r == ^M == CR */
		    case 't': s_putc(s, CTRL('I')); break;	/* \t == ^I == HT */
		    case 'b': s_putc(s, CTRL('H')); break;	/* \b == ^H == BS */
		    case 'e': s_putc(s, CTRL('[')); break;	/* \e == ^[ == ESC */

		    case '0': case '1': case '2': case '3': case '4':
		    case '5': case '6': case '7':		/* \nnn == octal 0nnn */
			{
			char num[4];
			int i = 0;
			while ((i < 3) && isodigit(c))
			    {
			    num[i++] = (char)c;
			    c = getch();
			    }
			num[i] = '\0';
			if (i < 3) ungetch(c);
			s_putc(s, (char)strtol(num, 0, 8));
			break;
			}

		    case 'x':					/* \xnn = hex 0xnn */
			{
			char num[3];
			int i = 0;
			c = getch();	/* skip the x */
			while ((i < 2) && isxdigit(c))
			    {
			    num[i++] = (char)c;
			    c = getch();
			    }
			num[i] = '\0';
			if (i < 3) ungetch(c);
			s_putc(s, (char)strtol(num, 0, 16));
			break;
			}

		    default: s_putc(s, c); break;		/* \X == X */
		    }
		}

	    else if (c == '\n')		/* bad newline */
		{
		mailr_errormessage = gettxt(":548","newline found in string/character constant");
		return yylex_report(TError);
		}

	    else			/* good character */
		(void) s_putc(s, c);
	    }

	/* bad EOF */
	mailr_errormessage = gettxt(":549","EOF found in string/character constant");
	return yylex_report(TError);
	}

    /* 1 and 2-character symbols */
    switch (c)
	{
	case '@':	return yylex_report(TAt);
	case ']':	return yylex_report(TClBracket);
	case '}':	return yylex_report(TClCurly);
	case ')':	return yylex_report(TClParen);
	case '~':	return yylex_report(TMatch);
	case '-':	return yylex_report(TMinus);
	case '[':	return yylex_report(TOpBracket);
	case '{':	return yylex_report(TOpCurly);
	case '(':	return yylex_report(TOpParen);
	case '+':	return yylex_report(TPlus);
	case ';':	return yylex_report(TSemi);
	case '/':	return yylex_report(TSlash);
	case '*':	return yylex_report(TStar);
	case '.':	return yylex_report(TDot);
	case ',':	return yylex_report(TComma);

	case '=':	/* = or == */
	    if ((c = getch()) != EOF)
		{
		if (c == '=') return yylex_report(TEqEq);
		ungetch(c);
		}
	    return yylex_report(TEq);

	case '>':	/* > or >= */
	    if ((c = getch()) != EOF)
		{
		if (c == '=') return yylex_report(TGE);
		ungetch(c);
		}
	    return yylex_report(TGT);

	case '<':	/* < or <= */
	    if ((c = getch()) != EOF)
		{
		if (c == '=') return yylex_report(TLE);
		ungetch(c);
		}
	    return yylex_report(TLT);

	case '!':	/* ! or != or !~ */
	    if ((c = getch()) != EOF)
		{
		if (c == '~') return yylex_report(TNMatch);
		if (c == '=') return yylex_report(TNE);
		ungetch(c);
		}
	    return yylex_report(TNot);

	case '|':	/* || */
	    if ((c = getch()) != EOF)
		{
		if (c == '|') return yylex_report(TOR);
		ungetch(c);
		}
	    break;

	case '&':	/* && */
	    if ((c = getch()) != EOF)
		{
		if (c == '&') return yylex_report(TAND);
		ungetch(c);
		}
	    break;
	}

    mailr_errormessage = gettxt(":550","unexpected character found");
    return yylex_report(TError);
}

/* Report an error message. This function is called by yacc on bad mailR input. */
static void yyerror(s)
char *s;
{
    if (yydebug) (void) putc('\n', stderr);
    pfmt(stderr, MM_ERROR, ":544:\"%s\", line %d: column %d: Error: %s",
	s_to_c(input_stack[input_stacklevel].filename),
	input_stack[input_stacklevel].token_linenumber,
	input_stack[input_stacklevel].token_column, s);
    if (mailr_errormessage)
	(void) fprintf (stderr, ": %s\n", mailr_errormessage);
    /* A common mistake is to be missing a semi-colon. */
    pfmt(stderr, MM_NOSTD, ":545:(Are you missing a semi-colon?)\n");
}

/*
  evaluate a given function with the given parameters
	top->left == list of parameters declarations
	top->right == compound_statement
	top->type == Tfunction, TCfunction, Tgenerator or Tgeneratorcall
	parameters == an array of references to values
	ret == where to return the result
*/
static MS_Gotos TreeNode_eval_function(top, parameters, ret)
TreeNode *top;
VarReference *parameters;
VarReference *ret;
{
    SymbolTable *st = (top->type == TCfunction) ? 0 : top->right->table;
    TreeNode *parlist = top->left;
    int functype;

    if (mailr_debug) (void) fprintf (stderr, "Evaluating function '%s', type=%d\n", s_to_c(top->name), top->type);
    /* it's a built in function; invoke it directly */
    if (top->type == TCfunction)
	{
	/* are there the right number of arguments? */
	if (top->argcount != parameters->arraycount)
	    {
	    pfmt(stderr, MM_ERROR, ":551:Unmatched number of arguments in call to '%s'\n",
		s_to_c(top->name));
	    return MS_break;
	    }
	/* invoke either a function that takes no arguments, or one that does. */
	return (top->argcount > 0) ? (*top->functionn)(parameters, ret) :
				     (*top->function0)(ret);
	}

    /* Check for arguments to the generator. */
    /* If it's a generator and there are no arguments, then we are in a */
    /* subsequent invocation that doesn't reinitialize the argument list. */
    if ((top->type == Tgenerator) && (parameters->arraycount == 0))
	functype = Tgeneratorcall;
    else
	functype = top->type;

    /* assign the arguments */
    if ((functype == Tfunction) || (functype == Tgenerator))
	{
	int i;
	SymbolTable_clear(st);
	/* set up a copy or reference in the function's symbol table */
	for (i = 0; parlist && i < parameters->arraycount; parlist = parlist->next, i++)
	    {
	    VarReference *vr = SymbolTable_add__VarReference(st, parlist->name);
	    if (parlist->type == Tvar)
		VarReference_assign__VarReference(vr, parameters->array[i]->vr);
	    else
		VarReference_assign__reference(vr, parameters->array[i]->vr);
	    }

	/* are there the right number of arguments? */
	if (parlist || i < parameters->arraycount)
	    {
	    pfmt(stderr, MM_ERROR, ":551:Unmatched number of arguments in call to '%s'\n",
		s_to_c(top->name));
	    return MS_break;
	    }
	}

    return TreeNode_eval(top->right, ret, (SymbolTable*)0, functype, 0, 0);
}

/*
    Return true if this statement is being run at the outermost scope of a function.
    The scope levels are marked for the following example:
	  function foo()
	0 {
	1     var x = 5;
	1     if (x)
	2         {
	3	  print("hello\n");
	2	  }
	1     else
	2         print("world\n");
	1     print("");
	0 }

    The outermost {} are at scopelevel 0 and its statements are at scopelevel 1.
    Lower scope statements are all at scopelevel 2 or greater.
*/

#define at_outerscope() (scopelevel < 2)

/*
    evaluate a statement
	top->type == type of statement (uses the yacc tokens)
	top->left,right,middle,... == parts of statement (see the yacc grammar
			to see what parts are used by which statements)
	vr_ret == where to return the result of the mailR function
			(some statement types also use vr_ret as a temporary
			variable during processing before assignment the result)
	st == symbol table to use for looking up values
	functype == are we in a function or generator?
	scopelevel == how many compound statement levels inside a function are we?
    returns
	MS_return - a return statement was executed
	MS_continue - a continue statement was executed
	MS_break - a break statement was executed
	MS_next - normal flow of execution
*/
static MS_Gotos TreeNode_eval(top, vr_ret, st, functype, scopelevel, continue_break_flag)
TreeNode *top;
VarReference *vr_ret;
SymbolTable *st;
int functype;
int scopelevel;
int continue_break_flag;
{
    MS_Gotos ret = MS_next;
    if (mailr_debug) (void) fprintf (stderr, "Evaluating %s\n", yytokenlist[top->type - TError]);
    VarReference_clear_value_and_reference(vr_ret);

    if (!top)
	return ret;

    switch (top->type)
	{
	case Tfunction:	/* these are all evaluated using eval_function_tree() */
	case TCfunction:
	case Tgenerator:
	case Tgeneratorcall:
	case Tvar:
	case Treference:
	    (void) fprintf (stderr, "program error! found Tfunction/Tgenerator/Tvar/Treference\n");
	    break;

	case TOpCurly:	/* compound statement */
	    {		/* { "top->left" } */
	    TreeNode *statment;

	    /* Clear all sub-scope symbol tables. The function-scope */
	    /* symbol table is cleared in TreeNode_eval_function(). */
	    if (!at_outerscope())
		SymbolTable_clear(top->table);
	    SymbolTable_set__parent(top->table, st);

	    /* Loop through statements until one says */
	    /* to quit, then pass the return code up. */
	    for (statment = top->left; statment; statment = statment->next)
		{
		ret = TreeNode_eval(statment, vr_ret, top->table, functype, scopelevel+1, continue_break_flag);
		if (ret == MS_next)
		    continue;
		else if (ret == MS_return)
		    break;
		else if (!continue_break_flag)	/* MS_continue or MS_break */
		    pfmt(stderr, MM_ERROR, ":552:Illegal %s statement found and ignored\n",
			(ret == MS_continue) ? "continue" : "break");
		else
		    break;
		}
	    break;
	    }

	case Tif:	/* if (expression) statement1 else statement2 */
	    {		/* if ("top->left") "top->right" else "top->middle" */
	    (void) TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* expression */
	    if (VarReference_true(vr_ret))
		ret = TreeNode_eval(top->right, vr_ret, st, functype, scopelevel, continue_break_flag);	/* statement1 */
	    else if (top->middle)
		ret = TreeNode_eval(top->middle, vr_ret, st, functype, scopelevel, continue_break_flag); /* statement2 */
	    break;
	    }

	case Twhile:	/* while (expression) statement */
	    {		/* while ("top->left") "top->right" */
	    for (;;)
		{
		(void) TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* expression */
		if (!VarReference_true(vr_ret))
		    break;
		ret = TreeNode_eval(top->right, vr_ret, st, functype, scopelevel+1, 1); /* statement */
		if ((ret == MS_break) || (ret == MS_return))
		    break;
		}
	    if (ret != MS_return)
		ret = MS_next;
	    break;
	    }

	case Tuntil:	/* until (expression) statement */
	    {		/* until ("top->left") "top->right" */
	    for (;;)
		{
		(void) TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* expression */
		if (VarReference_true(vr_ret))
		    break;
		ret = TreeNode_eval(top->right, vr_ret, st, functype, scopelevel+1, 1); /* statement */
		if ((ret == MS_break) || (ret == MS_return))
		    break;
		}
	    if (ret != MS_return)
		ret = MS_next;
	    break;
	    }

	case Tfor:		/* for (expr1; expr2; expr3) statement */
	    {			/* for ("top->left"; "top->right"; "top->middle") "top->middle2" */
	    (void) TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* expr1 */
	    for (;;)
		{
		if (top->right)			/* expr2 */
		    {
		    (void) TreeNode_eval(top->right, vr_ret, st, functype, scopelevel, continue_break_flag);
		    if (!VarReference_true(vr_ret))
			break;
		    }
		ret = TreeNode_eval(top->middle2, vr_ret, st, functype, scopelevel+1, 1); /* statement */
		if ((ret == MS_break) || (ret == MS_return))
		    break;
		(void) TreeNode_eval(top->middle, vr_ret, st, functype, scopelevel, continue_break_flag); /* expr3 */
		}
	    if (ret != MS_return)
		ret = MS_next;
	    break;
	    }

	case Tin:		/* for (var in expr) statement */
				/* for ("top->name" in "top->left") "top->right" */
	case Tfrom:		/* for (var from expr) statement */
	    {			/* for ("top->name" from "top->left") "top->right" */
	    VarReference *var = SymbolTable_lookup__VarReference(st, top->name);
	    int i;
	    if (var)
		{
		VarReference *r = VarReference_new();
		VarReference *arrayvar;
		(void) TreeNode_eval(top->left, r, st, functype, scopelevel, continue_break_flag);		/* expr */
		arrayvar = VarReference_Array_set(r);

		/* loop through the values of the array returned from the expression */
		/* and set var to point to the index of each array element. */
		for (i = 0; i < arrayvar->arraycount; i++)
		    {
		    VarReference_clear_value_and_reference(var);
		    if (top->type == Tin)
			VarReference_assign__string(var, arrayvar->array[i]->name);
		    else
			VarReference_assign__reference(var, arrayvar->array[i]->vr);
		    ret = TreeNode_eval(top->right, vr_ret, st, functype, scopelevel+1, 1);	/* statement */
		    if ((ret == MS_break) || (ret == MS_return))
			break;
		    }

		/* clean up and make sure var isn't a dangling reference to anything. */
		VarReference_delete(r);
		VarReference_clear_value_and_reference(var);
		}
	    if (ret != MS_return)
		ret = MS_next;
	    break;
	    }

	case Tstart:		/* start { statements } */
	    {			/* start { "top->left" } */
	    if (functype == Tgenerator)
		ret = TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* statement */
	    else if (functype != Tgeneratorcall)
		pfmt(stderr, MM_ERROR, ":664:mailR start statement found in function instead of a generator\n");
	    break;
	    }

	case Treturn:		/* return expression; */
	    {			/* return "top->left" */
	    (void) TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* expression */
	    ret = MS_return;
	    break;
	    }

	case Tbreak:		/* break; */
	    ret = MS_break;
	    break;

	case Tcontinue:		/* continue; */
	    ret = MS_continue;
	    break;

	case TvarDecl:		/* var X = expr, Y = expr, Z; */
	    {			/* var "top->name" = "top->left" */
	    TreeNode *var = top;
	    /* If calling a generator, only do this if we're not at the outer scope. */
	    /* Always do it for other function types. */
	    if (!((functype == Tgeneratorcall) && at_outerscope()))
		for ( ; var; var = var->right)
		    {
		    VarReference *vr = SymbolTable_add__VarReference(st, var->name);
		    if (var->left)
			{
			(void) TreeNode_eval(var->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* expr */
			/* make copy of value */
			VarReference_assign__VarReference(vr, vr_ret);
			}
		    }
	    break;
	    }

	case TEq:		/* var = expr */
	    {			/* "top->left" = "top->right" */
	    VarReference *r = VarReference_new();
	    /* we evaluate var to look up variable names, */
	    /* array references, and to allow a builtin */
	    /* return a reference. */
	    (void) TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* var */
	    (void) TreeNode_eval(top->right, r, st, functype, scopelevel, continue_break_flag);		/* expr */
	    VarReference_assign__VarReference(vr_ret, r);
	    VarReference_delete(r);
	    break;
	    }

	case TEqEq:		/* either == same */
	case TNE:		/* either != same */
	case TLT:		/* either < same */
	case TLE:		/* either <= same */
	case TGT:		/* either > same */
	case TGE:		/* either >= same */
				/* left side @@ right side */
	    /* If both are numeric, compare as numbers, */
	    /* else convert to strings and then compare. */
	    {			/* "top->left" @@ "top->right" */
	    VarReference *r = VarReference_new();
	    double dret;
	    UpToDate ltype;
	    UpToDate rtype;

	    /* evaluate the left and right */
	    (void) TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* left side */
	    (void) TreeNode_eval(top->right, r, st, functype, scopelevel, continue_break_flag);		/* right side */


	    /* are both left and right doubles? */
	    ltype = VarReference_get__Value_type(vr_ret);
	    rtype = VarReference_get__Value_type(r);
	    if (((ltype == UTD_Double) || (ltype == UTD_Both)) &&
		((rtype == UTD_Double) || (rtype == UTD_Both)))
		{
		double ld = VarReference_get__double(vr_ret);
		double rd = VarReference_get__double(r);
		switch (top->type)
		    {
		    case TEqEq:	dret = ld == rd; break;
		    case TNE:	dret = ld != rd; break;
		    case TLT:	dret = ld <  rd; break;
		    case TLE:	dret = ld <= rd; break;
		    case TGT:	dret = ld >  rd; break;
		    case TGE:	dret = ld >= rd; break;
		    }
		}

	    /* no? then compare as strings */
	    else
		{
		string *ls = VarReference_get__string(vr_ret);
		string *rs = VarReference_get__string(r);
		int cmp = strcmp(s_to_c(ls), s_to_c(rs));
		switch (top->type)
		    {
		    case TEqEq:	dret = cmp == 0; break;
		    case TNE:	dret = cmp != 0; break;
		    case TLT:	dret = cmp <  0; break;
		    case TLE:	dret = cmp <= 0; break;
		    case TGT:	dret = cmp >  0; break;
		    case TGE:	dret = cmp >= 0; break;
		    }
		}

	    VarReference_clear_value_and_reference(vr_ret);
	    VarReference_assign__double(vr_ret, dret);
	    VarReference_delete(r);
	    break;
	    }

	case TAND:		/* either1 && either2 */
	    {			/* "top->left" && "top->right" */
	    VarReference *r = VarReference_new();
	    (void) TreeNode_eval(top->left, r, st, functype, scopelevel, continue_break_flag);	/* either1 */
	    if (VarReference_true(r))
		(void) TreeNode_eval(top->right, r, st, functype, scopelevel, continue_break_flag);	/* either
		*/
	    VarReference_assign__double(vr_ret, (double)VarReference_true(r));
	    VarReference_delete(r);
	    break;
	    }

	case TOR:		/* either1 || either2 */
	    {			/* "top->left" || "top->right" */
	    VarReference *r = VarReference_new();
	    (void) TreeNode_eval(top->left, r, st, functype, scopelevel, continue_break_flag);	/* either1 */
	    if (!VarReference_true(r))
		(void) TreeNode_eval(top->right, r, st, functype, scopelevel, continue_break_flag);	/* either2 */
	    VarReference_assign__double(vr_ret, (double)VarReference_true(r));
	    VarReference_delete(r);
	    break;
	    }


	case TPlus:		/* number1 + number2 */
	case TMinus:		/* number1 - number2 */
	case TStar:		/* number1 * number2 */
	case TSlash:		/* number1 / number2 */
	    {			/* "top->left" @ "top->right" */
	    VarReference *r = VarReference_new();
	    double ld, rd, dret;
	    (void) TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* number1 */
	    (void) TreeNode_eval(top->right, r, st, functype, scopelevel, continue_break_flag);		/* number2 */
	    ld = VarReference_get__double(vr_ret);
	    rd = VarReference_get__double(r);
	    switch (top->type)
		{
		case TPlus:	dret = ld + rd; break;
		case TMinus:	dret = ld - rd; break;
		case TStar:	dret = ld * rd; break;
		case TSlash:	if (rd != 0) dret = ld / rd; else dret = 0; break;
		}
	    VarReference_clear_value_and_reference(vr_ret);
	    VarReference_assign__double(vr_ret, dret);
	    VarReference_delete(r);
	    break;
	    }

	case TAt:		/* string1 @ string2 */
	case TMatch:		/* string1 ~ string2 */
	case TNMatch:		/* string1 !~ string2 */
	    {			/* "top->left" @ "top->right" */
	    VarReference *r = VarReference_new();
	    string *ls, *rs;
	    (void) TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* string1 */
	    (void) TreeNode_eval(top->right, r, st, functype, scopelevel, continue_break_flag);		/* string2 */
	    ls = VarReference_get__string(vr_ret);
	    rs = VarReference_get__string(r);

	    if (top->type == TAt)
		{
		string *s = s_xappend((string*)0, s_to_c(ls), s_to_c(rs), (char*)0);
		VarReference_clear_value_and_reference(vr_ret);
		VarReference_assign__string(vr_ret, s);
		s_free(s);
		}

	    else
		{
		double dret = string_match(ls, rs, 0);
		VarReference_clear_value_and_reference(vr_ret);
		VarReference_assign__double(vr_ret, (top->type == TMatch) ? dret : !dret);
		}

	    VarReference_delete(r);
	    break;
	    }

	case TNumberConstant:	/* 99.99 */
	    VarReference_assign__double(vr_ret, top->val->d);
	    break;

	case TStringConstant:	/* "abc" */
	    VarReference_assign__string(vr_ret, top->val->s);
	    break;

	case Tname:		/* header.name */
	    {			/* "top->left"."hdrtag" */
	    string *name;
	    (void) TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* header */
	    name = VarReference_get__hdrtag(vr_ret);
	    VarReference_clear_value_and_reference(vr_ret);
	    VarReference_assign__charstring(vr_ret, name ? s_to_c(name) : "");
	    break;
	    }

	case Tcount:		/* array.count */
	    {			/* "top->left"."arraycount" */
	    double dret;
	    (void) TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* array */
	    dret = (vr_ret->vr_type == VR_Array) ? vr_ret->arraycount : 1;
	    VarReference_clear_value_and_reference(vr_ret);
	    VarReference_assign__double(vr_ret, dret);
	    break;
	    }

	case TUMinus:		/* - number */
	    {			/* - "top->left" */
	    double dret;
	    (void) TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* number */
	    dret = -VarReference_get__double(vr_ret);
	    VarReference_clear_value_and_reference(vr_ret);
	    VarReference_assign__double(vr_ret, dret);
	    break;
	    }

	case TNot:		/* ! either */
	    {			/* ! "top->left" */
	    double dret;
	    (void) TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* either */
	    dret = !VarReference_true(vr_ret);
	    VarReference_clear_value_and_reference(vr_ret);
	    VarReference_assign__double(vr_ret, dret);
	    break;
	    }

	case TOpBracket:	/* array[expr] */
	    {			/* "top->name"["top->left"] */
	    VarReference *arr = SymbolTable_lookup__VarReference(st, top->name);	/* array */
	    if (arr)
		{
		VarReference *r;
		(void) TreeNode_eval(top->left, vr_ret, st, functype, scopelevel, continue_break_flag);	/* expr */
		r = VarReference_Array_lookup__VarReference(arr, vr_ret);
		VarReference_clear_value_and_reference(vr_ret);
		VarReference_assign__reference(vr_ret, r);
		}
	    break;
	    }

	case TOpParen:		/* name(parms) */
	    {			/* "top->name"("top->left", ...) */
	    VarReference *args = VarReference_Array_new();
	    TreeNode *parm, *func;

	    for (parm = top->left; parm; parm = parm->next)
		{
		VarReference *arg = VarReference_new();
		(void) TreeNode_eval(parm, arg, st, functype, scopelevel, continue_break_flag);	/* parms */
		VarReference_Array_grow(args, 1);
		args->array[args->arraycount-1] = ArrayElement_new(0, arg);
		}

	    func = find_function_TreeNode(s_to_c(top->name), 1);
	    if (func)
		(void) TreeNode_eval_function(func, args, vr_ret);
	    VarReference_delete(args);
	    break;
	    }

	case TVarName:		/* name */
	    {			/* "top->name" */
	    VarReference *var = SymbolTable_lookup__VarReference(st, top->name);	/* name */
	    if (var)
		VarReference_assign__reference(vr_ret, var);
	    break;
	    }
	}

    if (mailr_debug)
	{
	(void) fprintf (stderr, "Returning %s, ", MS_tokenlist[ret]);
	VarReference_print(stderr, vr_ret);
	}

    return ret;
}

/*
    Push a file onto the input stack. Include files are handled by
    pushing another file onto the stack. If complain is set, then an
    error message will be printed if the file cannot be opened.

    input_stacklevel is initially at -1.
*/
static void pushfile(filename, complain)
string *filename;
int complain;
{
    FILE *newfp;

    /* any room left on the stack? */
    if (input_stacklevel == MAXSTACK-1)
	{
	pfmt(stderr, MM_ERROR, ":562:/etc/mail/rewrite include files stacked too deep\n");
	return;
	}

    /* try opening the new file */
    newfp = fopen(s_to_c(filename), "r");
    if (!newfp)
	{
	if (complain)
	    lfmt(stderr, MM_ERROR, ":2:Cannot open %s: %s\n", s_to_c(filename), Strerror(errno));
	return;
	}

    /* remember where we were in the old file */
    if (mailr_fp && input_stacklevel >= 0)
	{
	input_stack[input_stacklevel].curoffset = ftell(mailr_fp);
	fclose(mailr_fp);
	}

    /* start up the new file */
    input_stacklevel++;
    mailr_fp = newfp;
    input_stack[input_stacklevel].filename = filename;
    input_stack[input_stacklevel].lastcolumn = 0;
    input_stack[input_stacklevel].column = 1;
    input_stack[input_stacklevel].linenumber = 1;
    input_stack[input_stacklevel].token_column = 1;
    input_stack[input_stacklevel].token_linenumber = 1;
}

/*
    Pop a file from the input stack. If this is the last file, return 0.
    If an error occurs restoring the position, report an error and return 0.
    Otherwise, restore our state and return 1.
*/
static int popfile()
{
    FILE *newfp;

    /* close the current file */
    (void) fclose(mailr_fp);
    mailr_fp = 0;
    s_free(input_stack[input_stacklevel].filename);

    /* Was this the original file? If so, we're done. */
    if (input_stacklevel <= 0)
	{
	input_stacklevel = -1;
	return 0;
	}

    /* move down to the previous file */
    input_stacklevel--;
    newfp = fopen(s_to_c(input_stack[input_stacklevel].filename), "r");

    /* try to restore our location */
    if (!newfp || fseek(newfp, input_stack[input_stacklevel].curoffset, SEEK_SET))
	{
	lfmt(stderr, MM_ERROR, ":561:Cannot restore rewrite file location: %s: %s\n",
	    s_to_c(input_stack[input_stacklevel].filename), Strerror(errno));
	if (newfp) fclose(newfp);
	/* clean up the entire stack */
	for ( ; input_stacklevel >= 0; input_stacklevel--)
	    s_free(input_stack[input_stacklevel].filename);
	return 0;
	}

    /* we have a good file again! */
    mailr_fp = newfp;
    return 1;
}

/*
    Open up the given filename and parse the file.
    The results are left in the mailr_treetop variable.
*/
static void set_treetop(file)
const char *file;
{
    pushfile(s_copy(file), 0);
    if (mailr_fp)
	yyparse();
}

/*
    Convert the headers in the message into the list kept in
    the mailR variable mailr_allheaders.
*/
static void save_all_headers(pin_hdrinfo)
Hdrinfo *pin_hdrinfo;
{
    register Hdrs *hdr;

    if (mailr_allheaders) VarReference_delete(mailr_allheaders);
    mailr_allheaders = VarReference_Array_new();

    /* copy pin_hdrinfo into mailr_allheaders */
    if((hdr = pin_hdrinfo->hdrs[H_FROM]) != NULL)
	{
	VarReference_Array_add__double_header(mailr_allheaders, (double)(mailr_allheaders->arraycount),
	    s_dup(hdr->value), hdr->name, hdr->hdrtype);
	}

    if((hdr = pin_hdrinfo->hdrs[H_RETURN_PATH]) != NULL)
	{
	VarReference_Array_add__double_header(mailr_allheaders, (double)(mailr_allheaders->arraycount),
	    s_dup(hdr->value), hdr->name, hdr->hdrtype);
	}

    for (hdr = pin_hdrinfo->hdrhead; hdr; hdr = hdr->next)
	VarReference_Array_add__double_header(mailr_allheaders, (double)(mailr_allheaders->arraycount),
	    s_dup(hdr->value), hdr->name, hdr->hdrtype);
}

/* determine the type (using the H_ values) of a header */
static int lookup_headertype(s)
string *s;
{
    register int i;
    if (strcmp(s_to_c(s), header[H_FROM].tag) == 0)
	return H_FROM;
    if (strcmp(s_to_c(s), header[H_FROM1].tag) == 0)
	return H_FROM1;
    /* sort! bsearch! ???? */
    for (i = H_MVERS; i < H_CONT; i++)
	if (cascmp(s_to_c(s), header[i].tag) == 0)
	    return i;
    return H_NAMEVALUE;
}

/*
    convert the headers kept in mailr_allheaders back into the form used by the message.
*/
static void store_new_headers(pout_hdrinfo)
Hdrinfo *pout_hdrinfo;
{
    register int i;
    clr_hdrinfo(pout_hdrinfo);
    /* loop through the headers */
    for (i = 0; i < mailr_allheaders->arraycount; i++)
	{
	VarReference *vr = mailr_allheaders->array[i]->vr;
	/* skip over values that were created but never assigned a value */
	if (vr->hdrtag && (vr->v->utd_type != UTD_Neither))
	    {
	    int headertype = vr->hdrtype <= 0 ? lookup_headertype(vr->hdrtag) : vr->hdrtype;
	    string *value = Value_get__string(vr->v);
	    if (headertype == H_NAMEVALUE)
		{
		/* Zap the trailing `:'. save_a_hdr() will put it back. */
		s_skipback(vr->hdrtag);
		s_terminate(vr->hdrtag);
		}
	    save_a_hdr(pout_hdrinfo, s_to_c(value), headertype, s_to_c(vr->hdrtag));
	    /* create a "remote from" version of the "From " header */
	    if (headertype == H_FROM)
		{
		/* 25 extra bytes is big enough for most system names */
		string *rf = s_copy_reserve(s_to_c(value), s_curlen(value)+1, s_curlen(value) + 25);
		rf = s_xappend(rf, " remote from ", remotefrom, (char*)0);
		save_a_hdr(pout_hdrinfo, s_to_c(rf), H_RFROM, s_to_c(vr->hdrtag));
		s_free(rf);
		}
	    }
	}

    /* clean up our data space */
    VarReference_delete(mailr_allheaders);
    mailr_allheaders = 0;
}

/*
    Determine if the function with the given name exists.
    This is used by mail to see if a mailR call should be made.
*/
int have_rewrite_function(function, filename)
const char *function;
const char *filename;
{
    TreeNode *top;
    if (!mailr_treetop)
	set_treetop(filename);

    top = find_function_TreeNode(function, 0);
    return top != 0;
}

/*
    Invoke the given mailR function on the given message, reading the header
    from "pin_hdrinfo" and writing the new header to "pout_hdrinfo". Use
    the mailR functions found in "filename".
*/
void invoke_rewrite(function, pmsg, pin_hdrinfo, pout_hdrinfo, filename, ptmpfile)
const char *function;
Msg *pmsg;
Hdrinfo *pin_hdrinfo;
Hdrinfo *pout_hdrinfo;
const char *filename;
Tmpfile *ptmpfile;
{
    if (!mailr_treetop)
	set_treetop(filename);

    { const char *debugvar = getenv("MAILR_DEBUG"); mailr_debug = atoi(debugvar ? debugvar : "0"); }

    if (mailr_debug)
	{
	(void) printf
	    (
	    "invoke_rewrite(%s, 0x%x, 0x%x, 0x%x, %s, 0x%x) Entered.\n",
	    function,
	    (int)pmsg,
	    (int)pin_hdrinfo,
	    (int)pout_hdrinfo,
	    filename,
	    (int)ptmpfile
	    );
	}

    /* if we have something to rewrite with, then do so */
    if (mailr_treetop)
	{
	mailr_pmsg = pmsg;			/* set our global message pointer */
	msg_tmpfile = pmsg->tmpfile->tmpf;	/* current message body */
	if (!msg_tmpfile)
	    msg_tmpfile = doopen(pmsg->tmpfile->lettmp, "r", E_TMP);

	save_all_headers(pin_hdrinfo);		/* store the old headers */
	if (mailr_debug)
	    {
	    (void) printf("Allheaders at start:\n");
	    VarReference_print(stdout, mailr_allheaders);
	    }
	invoke(function);			/* invoke the mailR function */
	if (mailr_debug)
	    {
	    (void) printf("Allheaders at end:\n");
	    VarReference_print(stdout, mailr_allheaders);
	    }

	store_new_headers(pout_hdrinfo);	/* copy the new headers back out */
	if (msg_tmpfile != pmsg->tmpfile->tmpf)	/* and any new body */
	    {
	    if (!ptmpfile->tmpf)
		mktmp(ptmpfile);
	    rewind(msg_tmpfile);
	    copystream(msg_tmpfile, ptmpfile->tmpf);
	    rewind(ptmpfile->tmpf);
	    }
	}
}

/*
    Set up a mailR function environment and invoke the given mailR function.
*/
static void invoke(function)
const char *function;
{
    TreeNode *top = find_function_TreeNode(function, 1);
    if (mailr_debug) (void) fprintf(stderr, "Invoking %s\n", function);
    if (top)
	{
	VarReference *v = VarReference_new();
	VarReference *ret = VarReference_Array_new();
	MS_Gotos mret = TreeNode_eval_function(top, v, ret);
	if (mailr_debug)
	    {
	    (void) fprintf (stderr, "Returning %s\n", MS_tokenlist[mret]);
	    VarReference_print(stdout, ret);
	    }
	VarReference_delete(v);
	VarReference_delete(ret);
	}

    else
	pfmt(stderr, MM_ERROR, ":553:Cannot find mailR function '%s'\n", function);
}

/* -------------------------------- TreeNode -------------------------------- */
/* create a TreeNode element */
static TreeNode *TreeNode_new(left, right, type)
TreeNode *left;
TreeNode *right;
int type;
{
    TreeNode *ret = New(TreeNode);
    ret->left = left;
    ret->right = right;
    ret->middle = 0;
    ret->middle2 = 0;
    ret->next = 0;
    ret->type = type;
    ret->val = 0;
    ret->name = 0;
    ret->table = 0;
    ret->function0 = 0;
    ret->functionn = 0;
    ret->argcount = 0;
    return ret;
}

/* create a tree element with a symbol table associated with it */
static TreeNode *TreeNode_new__table(left, right, type)
TreeNode *left;
TreeNode *right;
int type;
{
    TreeNode *ret = TreeNode_new(left, right, type);
    ret->table = SymbolTable_new();
    return ret;
}

/* create a tree element, setting middle to a value */
static TreeNode *TreeNode_new__middle(left, right, middle, type)
TreeNode *left;
TreeNode *right;
TreeNode *middle;
int type;
{
    TreeNode *ret = TreeNode_new(left, right, type);
    ret->middle = middle;
    return ret;
}

/* create a tree element, setting middle and middle2 to a value */
static TreeNode *TreeNode_new__middle_middle2(left, right, middle, middle2, type)
TreeNode *left;
TreeNode *right;
TreeNode *middle;
TreeNode *middle2;
int type;
{
    TreeNode *ret = TreeNode_new(left, right, type);
    ret->middle = middle;
    ret->middle2 = middle2;
    return ret;
}

/* create a tree element, setting name to a value */
static TreeNode *TreeNode_new__name(left, right, name, type)
TreeNode *left;
TreeNode *right;
string *name;
int type;
{
    TreeNode *ret = TreeNode_new(left, right, type);
    ret->name = name;
    return ret;
}

#if 0		/* not needed in this code */
static void TreeNode_delete(top)
TreeNode *top;
{
    /* recursively descend down left, right, middle, and next, deleting and freeing the tree */
    if (!top) return;
    if (top->left) TreeNode_delete(top->left);
    if (top->right) TreeNode_delete(top->right);
    if (top->middle) TreeNode_delete(top->middle);
    if (top->middle2) TreeNode_delete(top->middle2);
    if (top->next) TreeNode_delete(top->next);
    if (top->val) Value_delete(top->val);
    if (top->name) s_free(top->name);
    if (top->table) SymbolTable_delete(top->table);
}
#endif		/* not needed */

/* -------------------------------- Value -------------------------------- */
/* create an empty variable */
static Value *Value_new()
{
    Value *ret = New(Value);
    ret->s = 0;
    ret->d = 0;
    ret->utd_type = UTD_Neither;
    return ret;
}

/*
    Create a variable with a given value.
    If there is a string, use that pointer and
    set the type to be a string.
    Otherwise, set the type to be a double.
    This function is never called with both
    s and d being non-zero.
*/
static Value *Value_new__string_double(s, d)
string *s;
double d;
{
    Value *ret = New(Value);
    ret->s = s;
    ret->d = d;
    if (s) ret->utd_type = UTD_String;
    else ret->utd_type = UTD_Double;
    return ret;
}

/* free the space used by a variable */
static void Value_delete(v)
Value *v;
{
    if (v)
	{
	s_free(v->s);
	free(v);
	}
}

/* print a representation of the variable */
/* The %g must be consistent with that used in double_to_string. */
static void Value_print(out, v)
FILE *out;
Value *v;
{
    switch (v->utd_type)
	{
	case UTD_Neither:
	    pfmt(out, MM_NOSTD, ":554:UNASSIGNED\n");
	    break;
	case UTD_Double:
	    (void) fprintf (out, "%g\n", v->d);
	    break;
	case UTD_String:
	case UTD_Both:
	    putc('"', out);
	    s_write(v->s, out);
	    putc('"', out);
	    putc('\n', out);
	    break;
	}
}

/* determine if a variable can be considered "true" */
static int Value_true(v)
Value *v;
{
    switch (v->utd_type)
	{
	case UTD_Neither:
	    return 0;
	case UTD_Double:
	case UTD_Both:
	    return v->d != 0;
	case UTD_String:
	    return s_to_c(v->s)[0] != '\0';
	default:	/* impossible case */
	    return 0;
	}
}

/* copy a variable */
static Value *Value_copy(v)
Value *v;
{
    Value *ret = New(Value);
    if (v->s) ret->s = s_dup(v->s);		/* copy the string */
    else ret->s = 0;
    ret->d = v->d;				/* copy the double */
    ret->utd_type = v->utd_type;		/* copy the type */
    return ret;
}

/* retrieve the value of the variable expressed as a double */
static double Value_get__double(v)
Value *v;
{
    Value_update__double(v);
    return v->d;
}

/* retrieve the value of the variable expressed as a string */
static string *Value_get__string(v)
Value *v;
{
    Value_update__string(v);
    return v->s;
}

/* if the variable isn't a double already, make it one */
static void Value_update__double(v)
Value *v;
{
    switch (v->utd_type)
	{
	case UTD_Neither:
	    Value_assign__double(v, 0.0);
	    break;
	case UTD_String:
	    v->d = atof(s_to_c(v->s));
	    v->utd_type = UTD_Both;
	    break;
	case UTD_Double:
	case UTD_Both:
	    break;
	}
}

/* if the variable isn't a string already, make it one */
static void Value_update__string(v)
Value *v;
{
    switch (v->utd_type)
	{
	case UTD_Neither:
	    Value_assign__string(v, s_copy(""));
	    break;
	case UTD_Double:
	    if (v->s) s_free(v->s);
	    v->s = double_to_string(v->d);
	    v->utd_type = UTD_Both;
	    break;
	case UTD_String:
	case UTD_Both:
	    break;
	}
}

/* change the value of the double in the variable */
static void Value_assign__double(v, d)
Value *v;
double d;
{
    if (v->s) s_free(v->s);
    v->s = 0;
    v->d = d;
    v->utd_type = UTD_Double;
}

/*
    Change the value of the string in the variable.
    As always, assume we can keep the pointer.
*/
static void Value_assign__string(v, s)
Value *v;
string *s;
{
    if (v->s) s_free(v->s);
    v->s = s;
    v->utd_type = UTD_String;
}

/* -------------------------------- ArrayElement -------------------------------- */
/* create a new array element with the given name and reference */
static ArrayElement *ArrayElement_new(name, vr)
string *name;
VarReference *vr;
{
    ArrayElement *ret = New(ArrayElement);
    ret->name = name;
    ret->vr = vr;
    return ret;
}

/* delete the resources used by the array element */
static void ArrayElement_delete(ae)
ArrayElement *ae;
{
    if (ae)
	{
	s_free(ae->name);
	VarReference_delete(ae->vr);
	free(ae);
	}
}

/* print a representation of the array element */
static void ArrayElement_print(out, ae)
FILE *out;
ArrayElement *ae;
{
    (void) fprintf (out, "[%s]:", s_to_c(ae->name));
    VarReference_print(out, ae->vr);
}

/* determine whether this element can be considered "true" */
static int ArrayElement_true(ae)
ArrayElement *ae;
{
    return VarReference_true(ae->vr);
}

/* make a copy of the array element */
static ArrayElement *ArrayElement_copy(ae)
ArrayElement *ae;
{
    ArrayElement *ret = New(ArrayElement);
    ret->name = s_dup(ae->name);
    ret->vr = VarReference_copy(ae->vr);
    return ret;
}

/* retrieve the value of the element as a double */
static double ArrayElement_get__double(ae)
ArrayElement *ae;
{
    return VarReference_get__double(ae->vr);
}

/* retrieve the header tag of the element */
static string *ArrayElement_get__hdrtag(ae)
ArrayElement *ae;
{
    return VarReference_get__hdrtag(ae->vr);
}

/* retrieve the value of the element as a string */
static string *ArrayElement_get__string(ae)
ArrayElement *ae;
{
    return VarReference_get__string(ae->vr);
}

/* retrieve the type of the element's VarReference */
static UpToDate ArrayElement_get__Value_type(ae)
ArrayElement *ae;
{
    return VarReference_get__Value_type(ae->vr);
}

/* -------------------------------- VarReference -------------------------------- */
/* create a new, empty, VarReference */
static VarReference *VarReference_new()
{
    VarReference *ret = New(VarReference);
    ret->vr_type = VR_None;
    ret->v = 0;
    ret->vr = 0;
    ret->array = 0;
    ret->arraycount = 0;
    ret->hdrtag = 0;
    ret->hdrtype = -1;
    return ret;
}

/* create a new VarReference, making it an array from the beginning */
static VarReference *VarReference_Array_new()
{
    VarReference *ret = VarReference_new();
    (void) VarReference_Array_set(ret);
    return ret;
}

/* create a new VarReference, initializing it to be a Value with the given Value pointer. */
static VarReference *VarReference_new__Value(v)
Value *v;
{
    VarReference *ret = VarReference_new();
    ret->vr_type = VR_Value;
    ret->v = v;
    return ret;
}

/*
    create a new VarReference, initializing it to be a header with the
    given Value pointer, header tag and header type.
*/
static VarReference *VarReference_new__Header(v, hdrtag, hdrtype)
Value *v;
const char *hdrtag;
int hdrtype;
{
    VarReference *ret = VarReference_new();
    ret->vr_type = VR_Header;
    ret->v = v;
    ret->hdrtag = s_copy(hdrtag);
    ret->hdrtype = hdrtype;

    /* Make certain name-value: headers have a trailing colon. */
    /* Such headers coming in from rmail won't. */
    if ((hdrtype == H_NAMEVALUE) && (s_to_c(ret->hdrtag)[s_curlen(ret->hdrtag)-1] != ':'))
	{
	s_putc(ret->hdrtag, ':');
	s_terminate(ret->hdrtag);
	}
    return ret;
}

/* free the resources used by the VarReference */
static void VarReference_delete(vr)
VarReference *vr;
{
    if (vr)
	{
	VarReference_clear_value_and_reference(vr);
	if (vr->hdrtag) s_free(vr->hdrtag);
	free(vr);
	}
}

/* print a representation of the VarReference */
static void VarReference_print(out, vr)
FILE *out;
VarReference *vr;
{
    int i;
    switch (vr->vr_type)
	{
	case VR_None:
	    pfmt(out, MM_NOSTD, ":555:NOTHING\n");
	    break;
	case VR_Header:
	    if (vr->hdrtag)
		{
		putc('{', out);
		fwrite(s_to_c(vr->hdrtag), 1, s_curlen(vr->hdrtag), out);
		(void) fprintf (out, ",%d} ", vr->hdrtype);
		}
	    /* FALLTHROUGH */
	case VR_Value:
	    Value_print(out, vr->v);
	    break;
	case VR_Reference:
	    pfmt(out, MM_NOSTD, ":556:Reference: ");
	    VarReference_print(out, vr->vr);
	    break;
	case VR_Array:
	    pfmt(out, MM_NOSTD, ":557:Array: ");
	    for (i = 0; i < vr->arraycount; i++)
		{
		(void) fprintf (out, "[%d]:", i);
		ArrayElement_print(out, vr->array[i]);
		}
	    break;
	}
}

/*
    determine if a VarReference can be considered "true".
    For arrays, look at the truth of the first element in the list.
*/
static int VarReference_true(vr)
VarReference *vr;
{
    switch (vr->vr_type)
	{
	case VR_None:
	    return 0;
	case VR_Header:
	case VR_Value:
	    return Value_true(vr->v);
	case VR_Reference:
	    return VarReference_true(vr->vr);
	case VR_Array:
	    if (vr->arraycount == 0)
		return 0;
	    else
		return ArrayElement_true(vr->array[0]);
	default:	/* impossible case */
	    return 0;
	}
}

/*
    change the value of a VarReference
    to be a double, string or charstring.
    Since the code is so similar, this
    routine is used by ..._double,
    ..._charstring and ..._string.
    Recurse through any references.
*/
static void VarReference_assign__multiple(vr, dfrom, csfrom, sfrom)
VarReference *vr;
double dfrom;
const char *csfrom;
string *sfrom;
{
    if (vr->vr_type == VR_Reference)
	VarReference_assign__multiple(vr->vr, dfrom, csfrom, sfrom);
    else if (vr->hdrtag)	/* when assigning to a header, verify string structure */
	{
	string *ohdrtag = vr->hdrtag;	/* preserve any header tag and type */
	int ohdrtype = vr->hdrtype;

	vr->hdrtag = 0;
	VarReference_clear_value(vr);
	if (csfrom)
	    vr->v = Value_new__string_double(set_continuation_lines(s_copy(csfrom)), 0.0);
	else if (sfrom)
	    vr->v = Value_new__string_double(set_continuation_lines(s_dup(sfrom)), 0.0);
	else
	    vr->v = Value_new__string_double((string*)0, dfrom);
	vr->vr_type = VR_Header;
	vr->hdrtag = ohdrtag;
	vr->hdrtype = ohdrtype;
	}

    else
	{
	VarReference_clear_value(vr);
	if (csfrom)
	    vr->v = Value_new__string_double(s_copy(csfrom), 0.0);
	else if (sfrom)
	    vr->v = Value_new__string_double(s_dup(sfrom), 0.0);
	else
	    vr->v = Value_new__string_double((string*)0, dfrom);
	vr->vr_type = VR_Value;
	}
}

/*
    change the value of a VarReference to be a double.
    Recurse through any references.
*/
static void VarReference_assign__double(vr, dfrom)
VarReference *vr;
double dfrom;
{
    VarReference_assign__multiple(vr, dfrom, (char*)0, (string*)0);
}

/*
    change the value of a VarReference to be a character string value.
    Recurse through any references.
*/
static void VarReference_assign__charstring(vr, sfrom)
VarReference *vr;
const char *sfrom;
{
    VarReference_assign__multiple(vr, 0.0, sfrom, (string*)0);
}

/*
    change the value of a VarReference to be a string value.
    Recurse through any references.
*/
static void VarReference_assign__string(vr, sfrom)
VarReference *vr;
string *sfrom;
{
    VarReference_assign__multiple(vr, 0.0, (char*)0, sfrom);
}

/*
    change the value of a VarReference to be the value of the Value.
    Recurse through any references.
*/
static void VarReference_assign__Value(vr, vfrom)
VarReference *vr;
Value *vfrom;
{
    if (vr->vr_type == VR_Reference)
	VarReference_assign__Value(vr->vr, vfrom);
    else
	{
	string *ohdrtag = 0;	/* preserve any header tag and type */
	int ohdrtype = -1;
	if (vr->hdrtag)
	    { ohdrtag = vr->hdrtag; vr->hdrtag = 0; ohdrtype = vr->hdrtype; }
	VarReference_clear_value(vr);
	vr->v = Value_copy(vfrom);
	if (ohdrtag)
	    vr->vr_type = VR_Header;
	else
	    vr->vr_type = VR_Value;
	vr->hdrtag = ohdrtag;
	vr->hdrtype = ohdrtype;
	}
}

/*
    Change the value of a VarReference to be a reference to another variable.
    Recurse through any references on the left side of the
    assignment. This handles the case of pass-by-reference
    in a function call.
*/
static void VarReference_assign__reference(vr, vrfrom)
VarReference *vr;
VarReference *vrfrom;
{
    if (vr->vr_type == VR_Reference)
	VarReference_assign__reference(vr->vr, vrfrom);
    else
	{
	VarReference_clear_value(vr);
	vr->vr = vrfrom;
	vr->vr_type = VR_Reference;
	}
}

/*
    Change the value of a VarReference to the value of another variable.
    Recurse through any references.
    This handles the two cases of assignment statements and
    pass-by-value to a function.
*/
static void VarReference_assign__VarReference(vr, vrfrom)
VarReference *vr;
VarReference *vrfrom;
{
    if (vr->vr_type == VR_Reference)
	VarReference_assign__VarReference(vr->vr, vrfrom);
    else if (vrfrom->vr_type == VR_Reference)
	VarReference_assign__VarReference(vr, vrfrom->vr);
    else
	{
	string *ohdrtag = 0;	/* preserve any header tag and type */
	int ohdrtype = -1;
	if (vr->hdrtag)
	    { ohdrtag = vr->hdrtag; vr->hdrtag = 0; ohdrtype = vr->hdrtype; }

	/* get rid of the old */
	VarReference_clear_value(vr);
	vr->vr_type = vrfrom->vr_type;
	/* and bring in the new */
	switch (vrfrom->vr_type)
	    {
	    case VR_None:
		break;
	    case VR_Header:
	    case VR_Value:
		vr->v = Value_copy(vrfrom->v);
		if (ohdrtag)	/* keep an existing header tag and type */
		    {
		    vr->hdrtag = ohdrtag;
		    ohdrtag = 0;
		    vr->hdrtype = ohdrtype;
		    vr->vr_type = VR_Header;
		    }

		else if (vrfrom->hdrtag)	/* copy over other variables's header tag/type */
		    {
		    vr->hdrtag = s_dup(vrfrom->hdrtag);
		    vr->hdrtype = vrfrom->hdrtype;
		    vr->vr_type = VR_Header;
		    }

		else				/* not a header */
		    vr->vr_type = VR_Value;
		break;
	    case VR_Reference:	/* should never happen */
		vr->vr = vrfrom->vr;
		break;
	    case VR_Array:
		{
		int i;
		vr->array = (ArrayElement**)malloc(sizeof(ArrayElement*) *
		    (vrfrom->arraycount ? vrfrom->arraycount : 1));
		vr->arraycount = vrfrom->arraycount;
		if (vr->arraycount)
		    for (i = 0; i < vr->arraycount; i++)
			vr->array[i] = ArrayElement_copy(vrfrom->array[i]);
		else
		    vr->array[0] = 0;
		}
	    }
	if (ohdrtag) s_free(ohdrtag);	/* free unused header */
	}
}

/* totally zap any value that this variable has, including any references. */
static void VarReference_clear_value_and_reference(vr)
VarReference *vr;
{
    if (vr->vr_type != VR_None)
	{
	/* zap the value */
	if (vr->v)
	    Value_delete(vr->v);
	/* zap the array */
	if (vr->array)
	    {
	    int i;
	    for (i = 0; i < vr->arraycount; i++)
		ArrayElement_delete(vr->array[i]);
	    free(vr->array);
	    }
	/* reset the pointers */
	vr->vr_type = VR_None;
	vr->v = 0;
	vr->vr = 0;
	vr->array = 0;
	vr->arraycount = 0;
	}
}

/*
    zap any value that this variable has.
    Recurse through any references.
*/
static void VarReference_clear_value(vr)
VarReference *vr;
{
    if (vr->vr_type == VR_Reference)
	VarReference_clear_value(vr->vr);
    else if (vr->vr_type != VR_None)
	{
	/* zap the value */
	if (vr->v)
	    Value_delete(vr->v);
	/* zap the array */
	if (vr->array)
	    {
	    int i;
	    for (i = 0; i < vr->arraycount; i++)
		ArrayElement_delete(vr->array[i]);
	    free(vr->array);
	    }
	/* reset the pointers */
	vr->vr_type = VR_None;
	vr->v = 0;
	vr->vr = 0;
	vr->array = 0;
	vr->arraycount = 0;
	}
}

/* make a copy of the variable */
static VarReference *VarReference_copy(vr)
VarReference *vr;
{
    VarReference *ret = VarReference_new();
    VarReference_assign__VarReference(ret, vr);
    return ret;
}

/*
    Retrieve the value of a variable, expressed as a double.
    If the variable is an array, use the first element's value.
*/
static double VarReference_get__double(vr)
VarReference *vr;
{
    switch (vr->vr_type)
	{
	case VR_None: VarReference_assign__double(vr, 0.0);
	    return 0;
	case VR_Header:
	case VR_Value:
	    return Value_get__double(vr->v);
	case VR_Reference:
	    return VarReference_get__double(vr->vr);
	case VR_Array:
	    if (vr->arraycount)
		return ArrayElement_get__double(vr->array[0]);
	    else
		return 0;
	default:	/* impossible case */
	    return 0;
	}
}

/*
    Retrieve the value of a variable, expressed as a string.
    If the variable is an array, use the first element's value.
*/
static string *VarReference_get__string(vr)
VarReference *vr;
{
    switch (vr->vr_type)
	{
	case VR_None:
	    VarReference_assign__charstring(vr, "");
	    return VarReference_get__string(vr);
	case VR_Header:
	case VR_Value:
	    return Value_get__string(vr->v);
	case VR_Reference:
	    return VarReference_get__string(vr->vr);
	case VR_Array:
	    if (!vr->arraycount)
		{
		VarReference *nvr = VarReference_new();
		VarReference_Array_grow(vr, 1);
		vr->array[0] = ArrayElement_new(s_copy_reserve("0", 2, 2), nvr);
		}
	    return ArrayElement_get__string(vr->array[0]);
	default:	/* impossible case */
	    return 0;
	}
}

/*
    Retrieve the type of a variable.
    If the variable is an array, use the first element's type.
*/
static UpToDate VarReference_get__Value_type(vr)
VarReference *vr;
{
    switch (vr->vr_type)
	{
	case VR_None:
	    return UTD_Neither;
	case VR_Header:
	case VR_Value:
	    return vr->v->utd_type;
	case VR_Reference:
	    return VarReference_get__Value_type(vr->vr);
	case VR_Array:
	    if (!vr->arraycount)
		return UTD_Neither;
	    return ArrayElement_get__Value_type(vr->array[0]);
	default:	/* impossible case */
	    return UTD_Neither;
	}
}

/*
    Retrieve the header tag of a header variable.
    If the variable is an array, use the first element's type.
*/
static string *VarReference_get__hdrtag(vr)
VarReference *vr;
{
    switch (vr->vr_type)
	{
	case VR_None:
	case VR_Header:
	case VR_Value:
	    return vr->hdrtag;
	case VR_Reference:
	    return VarReference_get__hdrtag(vr->vr);
	case VR_Array:
	    if (!vr->arraycount)
		return 0;
	    return ArrayElement_get__hdrtag(vr->array[0]);
	default:	/* impossible case */
	    return 0;
	}
}

/* Change the size of the vector associated with the array */
static void VarReference_Array_grow(vr, additional_count)
VarReference *vr;
int additional_count;
{
    int oldcount = vr->arraycount;
    vr->arraycount += additional_count;
    if (vr->array)
	vr->array = (ArrayElement**)realloc(vr->array, sizeof(ArrayElement*) * vr->arraycount);
    else
	vr->array = (ArrayElement**)malloc(sizeof(ArrayElement*) * vr->arraycount);
    while (additional_count-- > 0)
	vr->array[oldcount++] = 0;
}

/* Convert a VarReference into an array if it is not already one. */
/* Return the pointer passed in. */
static VarReference *VarReference_Array_set(vr)
VarReference *vr;
{
    switch (vr->vr_type)
	{
	case VR_None:
	    vr->arraycount = 0;
	    vr->array = (ArrayElement**)malloc(sizeof(ArrayElement*) * 1);
	    vr->array[0] = 0;
	    break;

	case VR_Header:
	case VR_Value:
	    {
	    VarReference *nvr = VarReference_new();
	    nvr->v = vr->v;
	    VarReference_Array_grow(vr, 1);
	    vr->array[0] = ArrayElement_new(s_copy_reserve("0", 2, 2), nvr);
	    vr->v = 0;
	    break;
	    }

	case VR_Reference:
	    return VarReference_Array_set(vr->vr);

	case VR_Array:
	    break;
	}
    vr->vr_type = VR_Array;
    return vr;
}

/* Look up the array element whose index is the string value of "ind" */
static VarReference *VarReference_Array_lookup__VarReference(arr, ind)
VarReference *arr;
VarReference *ind;
{
    int i;
    VarReference *v;
    VarReference *arrayvar;

    /* find the string name to look for */
    string *s = VarReference_get__string(ind);

    /* search our array for the string name */
    arrayvar = VarReference_Array_set(arr);
    for (i = 0; i < arrayvar->arraycount; i++)
	if (strcmp(s_to_c(s), s_to_c(arrayvar->array[i]->name)) == 0)
	    return arrayvar->array[i]->vr;

    /* add a new element of that name */
    VarReference_Array_grow(arrayvar, 1);
    v = VarReference_new();
    arrayvar->array[arrayvar->arraycount-1] = ArrayElement_new(s_dup(s), v);
    return arrayvar->array[arrayvar->arraycount-1]->vr;
}

/* add an array element with name index and value s */
/* precondition: arr is an array and the element does not exist */
static void VarReference_Array_add__string_string(arr, index, s)
VarReference *arr;
string *index;
string *s;
{
    VarReference *v = VarReference_new();
    VarReference_assign__string(v, s);
    VarReference_Array_grow(arr, 1);
    arr->array[arr->arraycount-1] = ArrayElement_new(s_dup(index), v);
}

/* add an array element with name index and value s */
/* precondition: arr is an array and the element does not exist */
static void VarReference_Array_add__charstring_string(arr, index, s)
VarReference *arr;
const char *index;
string *s;
{
    VarReference *v = VarReference_new();
    VarReference_assign__string(v, s);
    VarReference_Array_grow(arr, 1);
    arr->array[arr->arraycount-1] = ArrayElement_new(s_copy(index), v);
}

/* add an array element with name index and value s */
/* precondition: arr is an array and the element does not exist */
static void VarReference_Array_add__double_string(arr, index, s)
VarReference *arr;
double index;
string *s;
{
    VarReference *v = VarReference_new();
    VarReference_assign__string(v, s);
    VarReference_Array_grow(arr, 1);
    arr->array[arr->arraycount-1] = ArrayElement_new(double_to_string(index), v);
}

/* add an array element with name index and value s and the element does not exist */
/* precondition: arr is an array and the element does not exist */
static void add_VarReference_array_double_reference(arr, index, s)
VarReference *arr;
double index;
VarReference *s;
{
    VarReference *v = VarReference_new();
    VarReference_assign__reference(v, s);
    VarReference_Array_grow(arr, 1);
    arr->array[arr->arraycount-1] = ArrayElement_new(double_to_string(index), v);
}

/* add an array element with name "index" and value "v" */
/* precondition: arr is an array and the element does not exist */
static void VarReference_Array_add__double_Value(arr, index, v)
VarReference *arr;
double index;
Value *v;
{
    VarReference *vr = VarReference_new();
    VarReference_assign__Value(vr, v);
    VarReference_Array_add__double_VarReference(arr, index, vr);
}

/* add an array element with name "index" and value "vr" */
/* precondition: arr is an array and the element does not exist */
static void VarReference_Array_add__double_VarReference(arr, index, vr)
VarReference *arr;
double index;
VarReference *vr;
{
    VarReference_Array_grow(arr, 1);
    arr->array[arr->arraycount-1] = ArrayElement_new(double_to_string(index), vr);
}

/* check a string to be used for a header to make certain */
/* that any continaution lines start with a blank */
static string *set_continuation_lines(value)
string *value;
{
    string *nvalue;
    int len = s_curlen(value);
    int i;

    /* check for newlines first */
    if (!memchr(s_to_c(value), '\n', len))
	return value;

    nvalue = s_new();
    for (i = 0; i < len; i++)
	{
	s_putc(nvalue, s_to_c(value)[i]);
	/* make sure a space is at start of each line */
	if ((s_to_c(value)[i] == '\n') && (s_to_c(value)[i+1] != ' ') && (s_to_c(value)[i+1] != '\t'))
	    s_putc(nvalue, ' ');
	}
    s_terminate(nvalue);
    s_free(value);
    return nvalue;
}

/* add an array element with name "index" and value "value" */
/* precondition: arr is an array and the element doesn't exist*/
static void VarReference_Array_add__double_header(arr, index, value, hdrtag, hdrtype)
VarReference *arr;
double index;
string *value;
const char *hdrtag;
int hdrtype;
{
    Value *v = Value_new__string_double(set_continuation_lines(value), 0.0);
    VarReference *vrnew = VarReference_new__Header(v, hdrtag, hdrtype);
    VarReference_Array_grow(arr, 1);
    arr->array[arr->arraycount-1] = ArrayElement_new(double_to_string(index), vrnew);
}

/* -------------------------------- Symbol -------------------------------- */
/* create a new symbol for a symbol table. Give it the name and VarReference. */
static Symbol *Symbol_new(name, v, next)
string *name;
VarReference *v;
Symbol *next;
{
    Symbol *ret = New(Symbol);
    ret->name = s_dup(name);
    ret->v = v;
    ret->next = next;
    return ret;
}

/* Free the resources used by the symbol. */
static void Symbol_delete(s)
Symbol *s;
{
    if (s)
	{
	s_free(s->name);
	VarReference_delete(s->v);
	free(s);
	}
}

/* -------------------------------- SymbolTable -------------------------------- */
/* create a new symbol table */
static SymbolTable *SymbolTable_new()
{
    SymbolTable *ret = New(SymbolTable);
    ret->table = 0;
    ret->parent = 0;
    return ret;
}

#if 0 /* not used */
/* free the resources used by the symbol table */
static void SymbolTable_delete(st)
SymbolTable *st;
{
    if (st)
	{
	SymbolTable_clear(st);
	free(st);
	}
}
#endif /* not used */

/* Clean out the symbols in this symbol table. */
/* This allows the symbol table to be cleaned */
/* out without being deallocated. */
static void SymbolTable_clear(st)
SymbolTable *st;
{
    Symbol *s = st->table;
    while (s)
	{
	Symbol *ns = s->next;
	Symbol_delete(s);
	s = ns;
	}
    st->table = 0;
}

/* establish a link from this symbol table to another outer-scope symbol table */
static void SymbolTable_set__parent(st, parent)
SymbolTable *st;
SymbolTable *parent;
{
    st->parent = parent;
}

/*
    Retrieve the variable with the given name.
    If the variable isn't found, look in the next
    scope's symbol table.
*/
static VarReference *SymbolTable_lookup__VarReference(st, name)
SymbolTable *st;
string *name;
{
    for (; st; st = st->parent)
	{
	Symbol *s = st->table;
	for ( ; s; s = s->next)
	    if (strcmp(s_to_c(name), s_to_c(s->name)) == 0)
		return s->v;
	}
    pfmt(stderr, MM_ERROR, ":558:Cannot find mailR variable '%s'\n", s_to_c(name));
    return 0;
}

/* Add a new variable to the symbol table, linking it to the front of the list. */
/* Note, this does not notice multiple decarations of the same variable */
/* statements in the same scope. */
static VarReference *SymbolTable_add__VarReference(st, name)
SymbolTable *st;
string *name;
{
    VarReference *v = VarReference_new();
    /* Pass in the front of the list (which is saved) */
    /* and then change the front of the list. */
    st->table = Symbol_new(name, v, st->table);
    return v;
}

/* -------------------------------- Misc Functions -------------------------------- */

/* Check a header name for a trailing ':', and append one if necessary. */
/* "From " and ">From " headers get a trailing space instead. */
static void set_header_colon(x)
string *x;
{
    if (strcmp(s_to_c(x), ">From") == 0)
	x = s_append(x, " ");
    else if ((strcmp(s_to_c(x), "From ") == 0) || (strcmp(s_to_c(x), ">From ") == 0))
	;
    else if (!strchr(s_to_c(x), ':'))
	x = s_append(x, ":");
}

/*
    Compile a string pattern and return the compiled regular expression.
    If maptolowercase, then set up the mapping appropriately so that
    all letters are considered equivalent to their lowercase values.
*/
static re_re *compile_string(pat, maptolowercase)
string *pat;
int maptolowercase;
{
    unsigned char re_map[256];
    register int i;
    re_re *regex;

    /* set up regular expression mapping */
    for (i = 0; i < 256; i++)
	re_map[i] = (char)i;
    if (maptolowercase)
	/* Map upper case letters to lower case. */
	/* This works even on EBCDIC systems. */
	/* Note: the RFC's say that non-ASCII */
	/* chars cannot be included, so we don't */
	/* have to worry about non-ASCII capitals. */
	for (i = 'A'; i <= 'Z'; i++)
	    re_map[i] = tolower(i);

    regex = re_recomp(s_to_c(pat), s_to_c(pat) + s_curlen(pat), re_map);
    if (!regex)
	pfmt(stderr, MM_ERROR, ":559:Cannot compile regular expression '%s'!\n", s_to_c(pat));
    return regex;
}

/* check whether the given string is matched by the given pattern */
static int string_match(str, pat, maptolowercase)
string *str;
string *pat;
int maptolowercase;
{
    re_re *regex = compile_string(pat, maptolowercase);
    char *match[10][2];
    int i;

    if (!regex)
	return 0;

    i = re_reexec(regex, s_to_c(str), s_to_c(str) + s_curlen(str), match);
    re_refree(regex);
    return i;
}

/* turn a double into its string representation */
/* The %g must be consistent with that used in Value_print. */
static string *double_to_string(d)
double d;
{
    char buf[20];
    (void) sprintf (buf, "%g", d);
    return s_copy(buf);
}

/* -------------------------------- mailR Built In functions -------------------------------- */

/*
    For mailR functions which are not passed arguments, the
    corresponding built-in function will have a single argument
    indicating (always named "ret") where to put the return value.

    For mailR functions which are passed arguments, the
    corresponding built-in function will have two arguments,
    one indicating where to put the return value (named "ret"), and the
    other the parameter list (named "parms"). "parms" is a VarReference array.
    The number of arguments is checked before calling the function, so there
    is no need to check here.

    In each function, the first step is always to retrieve the parameters from
    the array. The values of "var" parameters are then retrieved
*/

/* addrparse(var x)
    Treat x as a series of rfc 822 addresses separated by commas and optional
    newlines. Return an array created from x, split on successive addresses.
*/
static MS_Gotos builtin_addrparse(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_x = parameters->array[0]->vr;
    string *x = VarReference_get__string(var_x);
    r822_address *alist = 0, *ap;
    int flags = r822_LOOSE_CRLF | r822_LOOSE_LWSP | r822_LOOSE_DOMLIT | r822_LOOSE_FNAME;
    VarReference *arrayvar;
    int i;

    VarReference_clear_value(ret);
    arrayvar = VarReference_Array_set(ret);

    r822_addr_parse(x, flags, &alist);

    /* now loop through the addresses, creating the arrays */
    for (i = 0, ap = alist; ap; ap = ap->next, i++)
	{
	VarReference *hdr = VarReference_Array_new();
	string *tmpstr = s_new();
	r822_domain *dp;

	VarReference_Array_add__charstring_string(hdr, "group_phrase", ap->group_phrase);
	VarReference_Array_add__charstring_string(hdr, "local_part", ap->local_part);
	VarReference_Array_add__charstring_string(hdr, "domain_part", ap->domain_part->dotted);

	/* make sure an error string exists if there is an error */
	if ((ap->flags & r822_IS_ERROR) && (s_curlen(ap->error) == 0))
	    ap->error = s_append(ap->error, "unknown addressing error");
	VarReference_Array_add__charstring_string(hdr, "error", ap->error);

	/* combine the comment strings together into a single comment string */
	if (ap->comment || (s_curlen(ap->name_phrase) != 0))
	    {
	    r822_subthing *sp;
	    int add_parens = (ap->comment->next != 0 ||
			     (s_curlen(ap->name_phrase) != 0));
	    /* recreate ()'s around the comment parts */
	    if (add_parens)
		tmpstr = s_append(tmpstr, "(");
	    if (s_curlen(ap->name_phrase) != 0)
		if (ap->comment)
		    tmpstr = s_xappend(tmpstr, "(", s_to_c(ap->name_phrase), ")", (char*)0);
		else
		    tmpstr = s_xappend(tmpstr, s_to_c(ap->name_phrase), (char*)0);
	    for (sp=ap->comment; sp; sp=sp->next)
		tmpstr = s_xappend(tmpstr, "(", s_to_c(sp->element), ")", (char*)0);
	    if (add_parens)
		tmpstr = s_append(tmpstr, ")");
	    }
	VarReference_Array_add__charstring_string(hdr, "comment", tmpstr);

	/* combine the route fields together into a single route string */
	s_reset(tmpstr);
	for (dp = ap->route; dp; dp = dp->next)
	    {
	    s_putc(tmpstr, '@');
	    tmpstr = s_append(tmpstr, s_to_c(ap->route->dotted));
	    if (dp->next)
		s_putc(tmpstr, ',');
	    else
		s_putc(tmpstr, ':');
	    }
	VarReference_Array_add__charstring_string(hdr, "route", tmpstr);

	/* Recreate the address using the route local_part and domain_part. */
	/* Keep using tmpstr which already has the route in it. */
	tmpstr = s_append(tmpstr, s_to_c(ap->local_part));
	if (ap->domain_part)
	    tmpstr = s_xappend(tmpstr, "@", s_to_c(ap->domain_part->dotted), (char*)0);
	VarReference_Array_add__charstring_string(hdr, "address", tmpstr);

	VarReference_Array_add__double_VarReference(arrayvar, i, hdr);
	s_free(tmpstr);
	}

    delete_r822_address(alist);

    return MS_next;
}

/* append_header(var hdrtag, var afterhdr, var val)
    Create a new header of type hdrtag after the last header of type afterhdr with value 'val'.
    If no header of type afterhdr exists, the header will be created at the end of all
    other headers. For example, append_header("Received", "Received", "by tom")
    will create a new Received: header after all other Received: headers with
    the value "by tom". The case used by hdrtag is used for creating the header.

    A header also gets created by saying:
	header("Xyz") = string
    If the header Xyz: does not already exist, it will be created. Otherwise,
    all existing headers of that type will be replaced.
*/
static MS_Gotos builtin_append_header(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_hdrtag =	 parameters->array[0]->vr;
    VarReference *var_afterhdr = parameters->array[1]->vr;
    VarReference *var_val =	 parameters->array[2]->vr;

    string *hdrtag = VarReference_get__string(var_hdrtag);
    string *afterhdr = VarReference_get__string(var_afterhdr);
    string *val = VarReference_get__string(var_val);
    int i, lasti = -1;

    set_header_colon(hdrtag);
    set_header_colon(afterhdr);

    /* look for headers of that name */
    for (i = 0; i < mailr_allheaders->arraycount; i++)
	if (cascmp(s_to_c(hdrtag), s_to_c(mailr_allheaders->array[i]->vr->hdrtag)) == 0)
	    lasti = i;

    /* we found the last header */
    if (lasti != -1)
	{
	VarReference *vr;
	Value *v;
	int j;

	/* put the header after the last header of that type */
	lasti++;

	/* shift headers to the right */
	VarReference_Array_grow(mailr_allheaders, 1);
	for (j = mailr_allheaders->arraycount - 1; j > lasti; j--)
	    mailr_allheaders->array[j] = mailr_allheaders->array[j-1];

	/* create a new element to plug in */
	v = Value_new__string_double(s_dup(val), 0.0);
	vr = VarReference_new__Header(v, s_to_c(hdrtag), -1);
	mailr_allheaders->array[lasti] = ArrayElement_new(double_to_string(i), vr);
	}

    /* not found, create a new one at the end */
    else
	VarReference_Array_add__double_header(mailr_allheaders, (double)(mailr_allheaders->arraycount),
	    val, s_to_c(hdrtag), -1);

    return MS_next;
}

/* characterset()
    Returns the current character set, such as US-ASCII or ISO-2022-JP.
*/
static MS_Gotos builtin_characterset(ret)
VarReference *ret;
{
    VarReference_assign__charstring(ret, mail_get_charset());
    return MS_next;
}

/* delete_header(var x)
    Delete all headers of the given name.
    (The first ``From'' header cannot be deleted.)
    Return 1 if any header was deleted, 0 otherwise.
*/
static MS_Gotos builtin_delete_header(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_x = parameters->array[0]->vr;
    string *x = VarReference_get__string(var_x);
    int i;
    double dret = 0.0;

    set_header_colon(x);

    /* Look for headers of that name and delete it. */
    /* Cannot delete the "From" header, so start after it. */
    for (i = 1; i < mailr_allheaders->arraycount; i++)
	if (cascmp(s_to_c(x), s_to_c(mailr_allheaders->array[i]->vr->hdrtag)) == 0)
	    {
	    int j;
	    ArrayElement_delete(mailr_allheaders->array[i]);
	    mailr_allheaders->arraycount--;
	    for (j = i; j < mailr_allheaders->arraycount; j++)
		mailr_allheaders->array[j] = mailr_allheaders->array[j+1];
	    mailr_allheaders->array[j] = 0;
	    i--;
	    dret = 1.0;
	    }

    VarReference_assign__double(ret, dret);
    return MS_next;
}

/* domain()
    Return the domain name of the system.
*/
static MS_Gotos builtin_domain(ret)
VarReference *ret;
{
    VarReference_assign__charstring(ret, maildomain());
    return MS_next;
}

/* exists(var hdr)
    Return true if a header of the given type exists.
*/
static MS_Gotos builtin_exists(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_hdr = parameters->array[0]->vr;
    string *hdr = VarReference_get__string(var_hdr);
    int i;
    double d = 0;

    set_header_colon(hdr);

    /* look for headers of that name and delete it */
    for (i = 0; i < mailr_allheaders->arraycount; i++)
	if (cascmp(s_to_c(hdr), s_to_c(mailr_allheaders->array[i]->vr->hdrtag)) == 0)
	    {
	    d = 1;
	    break;
	    }

    VarReference_assign__double(ret, d);
    return MS_next;
}

/* run a command, rerouting stdin and stdout to our temp files */
static int system_io(cmd, tmpin, tmpout)
const char *cmd;
FILE *tmpin;
FILE *tmpout;
{
    pid_t pid;
    int ret = 0;

    /* fork and execute the command */
    switch (pid = loopfork())
	{
	case 0:		/* child */
	    dup2(fileno(tmpin), 0);	/* redirect stdin from tmpin */
	    dup2(fileno(tmpout), 1);	/* redirect stdout to tmpout */
	    ret = systm(cmd);
	    _exit(ret);
	    /* NOTREACHED */

	case -1:	/* fork failed */
	    ret = -1;
	    break;

	default:	/* parent */
	    ret = dowait(pid);
	}

    return ret;
}

/*
    Return an indication of if the line is the beginning of a header block.
    RFC 822 says a header is a series of non-control and
    non-space characters followed by a :. Return 0 or the index
    of the ':'.
*/
static int is_start_of_a_header(line)
string *line;
{
    register char *p, *endp;

    if (!isgraph(s_to_c(line)[0]))
	return 0;

    endp = s_to_c(line) + s_curlen(line);
    for (p = s_to_c(line); p < endp; p++)
	{
	if (Iscntrl(*p) || Isspace(*p))
	    return 0;

	if (*p == ':')
	    return p - s_to_c(line);
	}

    return 0;
}

/* append any continuation lines of the header */
static void add_continuation_lines(infp, line)
FILE *infp;
string *line;
{
    int c;

    /* RFC822 says a continuation line starts with a blank or tab */
    for (c = peekc(infp); (c == ' ') || (c == '\t'); c = peekc(infp))
	if (!s_read_line(infp, line))
	    break;
}

/* remove any trailing newline from the string */
static void s_trimnl(line)
string *line;
{
    if (s_to_c(line)[s_curlen(line)-1] == '\n')
	{
	s_skipback(line);
	s_terminate(line);
	}
}

/* return the size of the file open with the FILE* */
static long fpsize(fp)
FILE *fp;
{
    struct stat statbuf;
    if (fstat(fileno(fp), &statbuf) == -1)
	return -1;
    return statbuf.st_size;
}

/* Write the current set of headers to the given file for later input to a command. */
/* Skip past the first From_ header. */
static void write_headers(cmd_infp)
FILE *cmd_infp;
{
    register VarReference *vr = mailr_allheaders;
    int i;

    for (i = 1; i < vr->arraycount; i++)
	{
	VarReference *nvr = vr->array[i]->vr;
	if (nvr->vr_type == VR_Header)
	    {
	    (void) fprintf (cmd_infp, "%s", s_to_c(nvr->hdrtag));
	    switch (nvr->v->utd_type)
		{
		case UTD_Double:
		    (void) fprintf (cmd_infp, " %g\n", nvr->v->d);
		    break;
		case UTD_String:
		case UTD_Both:
		    if ((s_curlen(nvr->v->s) > 0) && !Isspace(s_to_c(nvr->v->s)[0]))
			(void) putc(' ', cmd_infp);
		    (void) fputs(s_to_c(nvr->v->s), cmd_infp);
		    (void) putc('\n', cmd_infp);
		    break;
		}
	    }
	}
}

/* re-read the headers from the output file into a new array */
static const char *reread_headers(pnmailr_allheaders, line, cmd_outfp, pclen)
VarReference **pnmailr_allheaders;
string *line;
FILE *cmd_outfp;
int *pclen;
{
    const char *still_reading;
    int colon;
    VarReference *nvr = mailr_allheaders->array[0]->vr;
    string *fromval = (nvr->v->utd_type == UTD_Double) ?
		      double_to_string(nvr->v->d) :
		      s_dup(nvr->v->s);

    *pnmailr_allheaders = VarReference_Array_new();
    if(fromval != NULL)
	{
	VarReference_Array_add__double_header(*pnmailr_allheaders,
	    (double)((*pnmailr_allheaders)->arraycount),
	    fromval, "From ", H_FROM);
	}

    rewind(cmd_outfp);

    /* read From headers */
    while (((still_reading = s_read_line(cmd_outfp, line)) != 0) &&
	   ((strncmp(s_to_c(line), "From ", 5) == 0) ||
	    (strncmp(s_to_c(line), ">From ", 6) == 0)))
	{
	const char *ln = s_to_c(line);
	s_trimnl(line);
	if (ln[0] == '>') ln++;
	VarReference_Array_add__double_header(*pnmailr_allheaders,
	    (double)((*pnmailr_allheaders)->arraycount),
	    s_copy(skipspace(ln+5)), ">From ", H_FROM1);
	s_reset(line);
	}

    /* read remaining headers */
    while (still_reading && (colon = is_start_of_a_header(line)) > 0)
	{
	string *cp;
	int hdrtype;

	add_continuation_lines(cmd_outfp, line);
	s_trimnl(line);
	cp = s_copy(skipspace(s_to_c(line) + colon + 1));
	s_to_c(line)[colon+1] = '\0';
	hdrtype = lookup_headertype(line);
	if ((hdrtype == H_CLEN) && pclen && (*pclen == -1))
	    *pclen = (*pnmailr_allheaders)->arraycount;
	VarReference_Array_add__double_header(*pnmailr_allheaders,
	    (double)((*pnmailr_allheaders)->arraycount),
	    cp, s_to_c(line), hdrtype);
	s_reset(line);
	still_reading = s_read_line(cmd_outfp, line);
	}

    return still_reading;
}

/* filter(var "command")
    Replace the headers and message body with the output from the command.
    Return the exit code.
*/
static MS_Gotos builtin_filter(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_command = parameters->array[0]->vr;
    string *cmd = VarReference_get__string(var_command);

    FILE *cmd_infp = xtmpfile();
    FILE *cmd_outfp;
    FILE *nmsg_tmpfile;
    VarReference *vr;
    double dret;

    if (!cmd_infp)
	{
	VarReference_assign__double(ret, -2);
	return MS_next;
	}

    /* create the input to our command */
    write_headers(cmd_infp);
    rewind(msg_tmpfile);
    putc('\n', cmd_infp);
    copystream(msg_tmpfile, cmd_infp);

    fflush(cmd_infp);
    if (ferror(cmd_infp))
	{
	fclose(cmd_infp);
	VarReference_assign__double(ret, -1);
	return MS_next;
	}

    /* create the output file for the command */
    cmd_outfp = xtmpfile();
    if (!cmd_outfp)
	{
	VarReference_assign__double(ret, -2);
	fclose(cmd_infp);
	return MS_next;
	}

    /* create the new copy of our message body */
    nmsg_tmpfile = xtmpfile();
    if (!nmsg_tmpfile)
	{
	VarReference_assign__double(ret, -2);
	fclose(cmd_infp);
	fclose(cmd_outfp);
	return MS_next;
	}

    rewind(cmd_infp);
    dret = system_io(s_to_c(cmd), cmd_infp, cmd_outfp);

    if (dret == 0)
	{
	string *line = s_new();
	int clen = -1;
	VarReference *nmailr_allheaders;
	const char *still_reading = reread_headers(&nmailr_allheaders, line, cmd_outfp, &clen);

	/* skip any leading blank line */
	if (still_reading &&
	    (((s_curlen(line) == 1) && (s_to_c(line)[0] == '\n')) ||
	     ((s_curlen(line) == 2) && (s_to_c(line)[0] == '\r') && (s_to_c(line)[1] == '\n'))))
	    {
	    s_reset(line);
	    still_reading = s_read_line(cmd_outfp, line);
	    }

	/* read message body */
	while (still_reading)
	    {
	    s_write(line, nmsg_tmpfile);
	    s_reset(line);
	    still_reading = s_read_line(cmd_outfp, line);
	    }

	fflush(nmsg_tmpfile);
	if (ferror(nmsg_tmpfile))
	    dret = -3;

	else
	    {
	    /* commit everything */
	    VarReference_delete(mailr_allheaders);
	    mailr_allheaders = nmailr_allheaders;
	    fclose(msg_tmpfile);
	    msg_tmpfile = nmsg_tmpfile;
	    nmsg_tmpfile = 0;
	    /* update the content-length header */
	    if (clen != -1)
		{
		long newlen = fpsize(msg_tmpfile);
		if (newlen >= 0)
		    Value_assign__double(mailr_allheaders->array[clen]->vr->v, (double)newlen);
		}
	    }

	s_free(line);
	}

    if (nmsg_tmpfile)
	fclose(nmsg_tmpfile);
    fclose(cmd_infp);
    fclose(cmd_outfp);

    VarReference_assign__double(ret, dret);
    return MS_next;
}

/* filterbody(var "command")
    Replace the message body with the output from the command.
    Return the exit code.
*/
static MS_Gotos builtin_filterbody(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_command = parameters->array[0]->vr;
    string *cmd = VarReference_get__string(var_command);

    FILE *nmsg_tmpfile = xtmpfile();
    double dret;

    /* create the output file for the command */
    if (!nmsg_tmpfile)
	{
	VarReference_assign__double(ret, -2);
	return MS_next;
	}

    rewind(msg_tmpfile);
    dret = system_io(s_to_c(cmd), msg_tmpfile, nmsg_tmpfile);

    if (dret == 0)
	{
	int i;
	register VarReference *vr = mailr_allheaders;

	fclose(msg_tmpfile);
	msg_tmpfile = nmsg_tmpfile;
	/* update any content length header */
	for (i = 1; i < vr->arraycount; i++)
	    {
	    VarReference *nvr = vr->array[i]->vr;
	    if (nvr->vr_type == VR_Header)
		{
		if (nvr->hdrtype == -1)
		    nvr->hdrtype = lookup_headertype(nvr->hdrtag);
		if (nvr->hdrtype == H_CLEN)
		    {
		    long newlen = fpsize(msg_tmpfile);
		    if (newlen >= 0)
			Value_assign__double(nvr->v, (double)newlen);
		    break;
		    }
		}
	    }
	}

    else
	fclose(nmsg_tmpfile);

    VarReference_assign__double(ret, dret);
    return MS_next;
}

/* filterheaders(var "command")
    Replace headers with the output from the command.
    Return the exit code.
*/
static MS_Gotos builtin_filterheaders(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_command = parameters->array[0]->vr;
    string *cmd = VarReference_get__string(var_command);

    FILE *cmd_infp = xtmpfile();
    FILE *cmd_outfp;
    double dret;

    if (!cmd_infp)
	{
	VarReference_assign__double(ret, -2);
	return MS_next;
	}

    /* create the input to our command */
    write_headers(cmd_infp);
    fflush(cmd_infp);
    if (ferror(cmd_infp))
	{
	fclose(cmd_infp);
	VarReference_assign__double(ret, -1);
	return MS_next;
	}

    /* create the output file for the command */
    cmd_outfp = xtmpfile();
    if (!cmd_outfp)
	{
	VarReference_assign__double(ret, -2);
	fclose(cmd_infp);
	return MS_next;
	}

    rewind(cmd_infp);
    dret = system_io(s_to_c(cmd), cmd_infp, cmd_outfp);

    if (dret == 0)
	{
	string *line = s_new();
	int clen = -1;
	VarReference *nmailr_allheaders;

	(void) reread_headers(&nmailr_allheaders, line, cmd_outfp, (int*)0);

	/* commit everything */
	VarReference_delete(mailr_allheaders);
	mailr_allheaders = nmailr_allheaders;

	s_free(line);
	}

    fclose(cmd_infp);
    fclose(cmd_outfp);

    VarReference_assign__double(ret, dret);
    return MS_next;
}

/* filterstring(var string, var "command", reference outstring)
    Send the given string to the given command and return the output in
    outstring. Return the exit code.
*/
static MS_Gotos builtin_filterstring(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_string =	  parameters->array[0]->vr;
    VarReference *var_command =	  parameters->array[1]->vr;
    VarReference *ref_outstring = parameters->array[2]->vr;

    string *s = VarReference_get__string(var_string);
    string *cmd = VarReference_get__string(var_command);
    double dret;
    FILE *cmd_infp, *cmd_outfp;

    VarReference_clear_value(ret);

    /* create the input to the command */
    cmd_infp = xtmpfile();
    if (!cmd_infp)
	{
	VarReference_assign__double(ret, -2);
	return MS_next;
	}

    (void) fprintf(cmd_infp, "%s\n", s_to_c(s));
    (void) fflush(cmd_infp);
    if (ferror(cmd_infp))
	{
	VarReference_assign__double(ret, -2);
	fclose(cmd_infp);
	return MS_next;
	}
    rewind(cmd_infp);

    /* create the output file for the command */
    cmd_outfp = xtmpfile();
    if (!cmd_outfp)
	{
	VarReference_assign__double(ret, -2);
	fclose(cmd_infp);
	return MS_next;
	}

    dret = system_io(s_to_c(cmd), cmd_infp, cmd_outfp);

    fclose(cmd_infp);
    rewind(cmd_outfp);

    /* place output into the output string */
    if (dret == 0)
	{
	string *to = s_new();
	(void) s_read_to_eof(cmd_outfp, to);
	VarReference_assign__string(ref_outstring, to);
	s_free(to);
	}
    fclose(cmd_outfp);

    VarReference_assign__double(ret, dret);
    return MS_next;
}

/* fromcrack(var x, reference user, reference date, reference system, reference forward)
    Treat x as a UNIX Postmark header and return the parts of the header in the
    reference variables. The three forms of UNIX postmarks are
	>From user date-string
	>From user date-string remote from system
	>From user date-string forwarded by user
    The system reference will be filled in if there is a "remote from" system
    present. The forward reference will be filled in if there is a "forwarded by"
    user present.
*/
static MS_Gotos builtin_fromcrack(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_x =	parameters->array[0]->vr;
    VarReference *ref_user =	parameters->array[1]->vr;
    VarReference *ref_date =	parameters->array[2]->vr;
    VarReference *ref_system =	parameters->array[3]->vr;
    VarReference *ref_forward =	parameters->array[4]->vr;

    string *s = VarReference_get__string(var_x);
    string *user = s_etok(s_restart(s), " \t");
    string *date = s_clone(s);
    int stringloc;
    static char c_remote_from[] = "remote from";
    static char c_forwarded_by[] = "forwarded by";

    VarReference_assign__string(ref_user, user);

    /* look for "remote from" in the string */
    if ((stringloc = substr(s_to_c(date), c_remote_from)) >= 0)
	{			/* "remote from" is present */
	string *from = s_copy(skipspace(s_to_c(date) + stringloc + sizeof(c_remote_from) - 1));
	s_truncate(date, stringloc);
	while ((stringloc > 0) && isspace(s_to_c(date)[stringloc-1]))
	    s_truncate(date, --stringloc);
	VarReference_assign__string(ref_date, date);
	VarReference_assign__string(ref_system, from);
	VarReference_clear_value(ref_forward);
	s_free(from);
	}

    /* look for "forwarded by" in the string */
    else if ((stringloc = substr(s_to_c(s), c_forwarded_by)) >= 0)
	{			/* "forwarded by" is present */
	string *frwrd = s_copy(skipspace(s_to_c(date) + stringloc + sizeof(c_forwarded_by) - 1));
	s_truncate(date, stringloc);
	while ((stringloc > 0) && isspace(s_to_c(date)[stringloc-1]))
	    s_truncate(date, --stringloc);
	VarReference_assign__string(ref_date, date);
	VarReference_clear_value(ref_system);
	VarReference_assign__string(ref_forward, frwrd);
	s_free(frwrd);
	}

    /* it's a local user reference */
    else
	{
	VarReference_assign__string(ref_date, date);
	VarReference_clear_value(ref_system);
	VarReference_clear_value(ref_forward);
	}

    s_free(user);
    s_free(date);
    return MS_next;
}

/* fromdate(var seconds)
    Return the date and time formatted as appropriate for a From UNIX Postmark.
    The seconds is the time since January 1, 1970. If 0 is specified, the
    current time is used.
*/
static MS_Gotos builtin_fromdate(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    char datestring[60];
    long ltmp = VarReference_get__double(parameters->array[0]->vr);
    if (!ltmp) ltmp = time((long*)0);
    fromdate(datestring, ltmp);
    VarReference_assign__charstring(ret, datestring);
    return MS_next;
}

/* getdate(var string)
    The string is treated as a date and converted into a canonical internal format usable
    by the date() and rfc822date() functions.
*/
static MS_Gotos builtin_getdate(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_string = parameters->array[0]->vr;
    string *s = VarReference_get__string(var_string);
    static char *datemsk = 0;
    struct tm *tm_ptr;

    /* The DATEMSK environment variable must be set for getdate(3) to work. */
    /* If it isn't, default it to point to our datemask file. */
    if (!datemsk)
	{
	datemsk = getenv("DATEMSK");
	if (!datemsk)
	    putenv(datemsk = "DATEMSK=/etc/mail/datemask");
	}

    VarReference_clear_value(ret);

    /* parse the date */
    tm_ptr = getdate(s_to_c(s));
    if (tm_ptr)
	{
	/* now convert to seconds since 1/1/1970 */
	time_t t = mktime(tm_ptr);
	if (t >= 0)
	    VarReference_assign__double(ret, (double)t);
	else
	    VarReference_assign__double(ret, 0.0);
	}
    else
	VarReference_assign__double(ret, (double)-getdate_err);
    return MS_next;
}

/* getenv(var x)
    Return the value of the environment variable with the name x.
*/
static MS_Gotos builtin_getenv(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_x = parameters->array[0]->vr;
    string *x = VarReference_get__string(var_x);
    const char *env = getenv(s_to_c(x));
    VarReference_assign__charstring(ret, env ? env : "");
    return MS_next;
}

/*
    This function is used by builtin_gsubstitute() and builtin_substitute().
    See those functions for the description.
*/
static MS_Gotos do_substitution(parameters, ret, global)
VarReference *parameters;
VarReference *ret;
int global;
{
    VarReference *var_s1 =   parameters->array[0]->vr;
    VarReference *var_pat =  parameters->array[1]->vr;
    VarReference *var_repl = parameters->array[2]->vr;
    string *s1 = VarReference_get__string(var_s1);
    string *pat = VarReference_get__string(var_pat);
    string *repl = VarReference_get__string(var_repl);
    string *sret;
    re_re *regex;
    char *match[10][2];
    char *beg, *end;

    regex = compile_string(pat, 0);
    if (!regex)
	{
	VarReference_clear_value(ret);
	return MS_next;
	}

    /*
	Consider the substitution

	    substitute("abcdef", "[bd]", "x")
	    gsubstitute("abcdef", "[bd]", "x")

	At the beginning of this loop, beg and end are set to the beginning
	and end of the string:

	    abcdef
	    ^     ^
	    beg   end

	After the first call to re_reexec(), match[0] will point at the "b":

	    abcdef
	     ^^
	     match[0]

	 Everything before match[0] ("a") is copied to the output string. The
	 substitution string is then copied, changing any \n strings along the
	 way to their match[n] values.

	 If this is a substitute(), we are done and the remainder of the string
	 "cdef" is copied to the output string. The result will be "axcdef".

	 If this is a gsubstitute(), we reset beg to point after the match[0],
	 and the call to re_reexec() is looped back to. After there are no more
	 matches (at "d" in this example), the remainder of the string "ef" is
	 copied to the output string. The result will be "axcxef".

    */

    /* set the boundaries */
    sret = s_new();
    beg = s_to_c(s1);
    end = s_to_c(s1) + s_curlen(s1);

    /* look for a match */
    while (re_reexec(regex, beg, end, match))
	{
	const char *c1 = beg;
	const char *c2 = s_to_c(repl);

	/* first copy leading characters not matched */
	for ( ; c1 < match[0][0]; c1++)
	    s_putc(sret, *c1);

	/* now copy characters in the range of the replacement string */
	/* from repl to sret, looking for \n sequences which are */
	/* expanded according to the regex match. */
	for ( ; *c2; c2++)
	    {
	    if (*c2 == '\\')
		{
		if (isdigit(*++c2))	/* found a \n reference */
		    {
		    int which = *c2 - '0';
		    if (match[which][0] && match[which][1])
			{
			const char *c3 = match[which][0];
			for ( ; c3 < match[which][1]; c3++)
			    s_putc(sret, *c3);
			}
		    }
		else			/* \x => x */
		    s_putc(sret, *c2);
		}
	    else			/* copy other characters across */
		s_putc(sret, *c2);
	    }

	/* set up the next match, or the remaining copy */
	beg = match[0][1];

	/* if not gsub, stop here and copy the rest */
	if (!global)
	    break;
	}

    /* No match at all or this time through. Copy the original string, or what's left. */
    if (beg < end)
	sret = s_append(sret, beg);

    VarReference_assign__string(ret, sret);
    re_refree(regex);
    s_free(sret);
    return MS_next;
}

/* gsubstitute(var s1, var pat, var repl)
    Return the string created by using the pattern pat on string s1, making
    replacements as controlled by repl. For example, gsubstitute("system!user",
    "(.*)!(.*)", "\\2@\\1") would return the string "user@system".
*/
static MS_Gotos builtin_gsubstitute(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    return do_substitution(parameters, ret, 1);
}

/* headers(var string)
    Returns array of references to all headers named by the string,
    indexed by numbers (starting at 0) and in the order found in the mail message.
    For example, header("To") would return all To: headers.
    Multiple line headers are returned as a single string with newlines embedded.
    (Leading white space on continuation lines are preserved.)
    The reference is live: if you change a value, it changes for real.
    If the given header does not exist, one will be created without a value.
*/
static MS_Gotos builtin_headers(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_x = parameters->array[0]->vr;
    VarReference *arrayvar;
    string *x = VarReference_get__string(var_x);
    int i;

    VarReference_clear_value(ret);
    arrayvar = VarReference_Array_set(ret);
    set_header_colon(x);

    /* look for headers of that name and create a reference to it */
    for (i = 0; i < mailr_allheaders->arraycount; i++)
	if (cascmp(s_to_c(x), s_to_c(mailr_allheaders->array[i]->vr->hdrtag)) == 0)
	    add_VarReference_array_double_reference(arrayvar, (double)(arrayvar->arraycount),
		mailr_allheaders->array[i]->vr);

    /* this header does not exist, so create an empty Value with that name */
    if (arrayvar->arraycount == 0)
	{
	string *s = s_copy_reserve("", 1, 1);
	VarReference_Array_add__double_header(mailr_allheaders, (double)(mailr_allheaders->arraycount),
	    s, s_to_c(x), -1);
	add_VarReference_array_double_reference(arrayvar, (double)(arrayvar->arraycount),
	    mailr_allheaders->array[mailr_allheaders->arraycount-1]->vr);
	}

    return MS_next;
}

/* headers_pattern(var pattern)
    Returns array of references to all headers whose names match the regular
    expression pattern, indexed by numbers (starting at 1) and in the order
    found in the mail message. For example, headers_pattern("To") would return all
    To: headers and headers_pattern(".*") would return ALL headers. Multiple line
    headers are returned as a single string with newlines embedded. (Leading
    white space on continuation lines are preserved.) The reference is live:
    if you change a value, it changes for real.
*/
static MS_Gotos builtin_headers_pattern(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_pattern =	parameters->array[0]->vr;
    VarReference *arrayvar;
    string *pattern = VarReference_get__string(var_pattern);
    int i;
    re_re *regex = compile_string(pattern, 1);
    char *match[10][2];

    VarReference_clear_value(ret);
    if (!regex)
	return MS_next;

    arrayvar = VarReference_Array_set(ret);

    /* look for headers of that name and create a reference to it */
    for (i = 0; i < mailr_allheaders->arraycount; i++)
	{
	string *hdrtag = mailr_allheaders->array[i]->vr->hdrtag;
	if (re_reexec(regex, s_to_c(hdrtag), s_to_c(hdrtag) + s_curlen(hdrtag), match))
	    add_VarReference_array_double_reference(arrayvar, (double)(arrayvar->arraycount),
		mailr_allheaders->array[i]->vr);
	}

    re_refree(regex);
    return MS_next;
}

/* int(var x)
    return value of x truncated to an integer
*/
static MS_Gotos builtin_int(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    double d = VarReference_get__double(parameters->array[0]->vr);
    VarReference_assign__double(ret, (double)(int)d);
    return MS_next;
}

/* isbinary()
    Returns 0, 1 or 2 if the message body is text, generic-text or binary, respectively.
*/
static MS_Gotos builtin_isbinary(ret)
VarReference *ret;
{
    VarReference_assign__double(ret, (double)(mailr_pmsg->binflag));
    return MS_next;
}

/* length(var x)
    Return the number of characters in the string x.
*/
static MS_Gotos builtin_length(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_x = parameters->array[0]->vr;
    string *s = VarReference_get__string(var_x);
    VarReference_assign__double(ret, (double)s_curlen(s));
    return MS_next;
}

/* lines(var x)
    Equivalent to split(x, "\n").
*/
static MS_Gotos builtin_lines(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_x = parameters->array[0]->vr;
    VarReference *arrayvar;
    string *s = s_dup(VarReference_get__string(var_x));
    string *line;
    double index = 0;

    VarReference_clear_value(ret);
    arrayvar = VarReference_Array_set(ret);

    /* retrieve successive lines of the array */
    s_restart(s);
    while ((line = s_etokc(s, '\n')) != 0)
	{
	VarReference_Array_add__double_string(arrayvar, index, line);
	s_free(line);
	index++;
	}
    s_free(s);
    return MS_next;
}

/* localmessage()
    Returns true if the message was created locally.
*/
static MS_Gotos builtin_localmessage(ret)
VarReference *ret;
{
    VarReference_assign__double(ret, (double)(mailr_pmsg->localmessage));
    return MS_next;
}

/* mailsystem(var x)
    If x == 0, return the name of the system, taken from the cluster name or
    uname if there is no cluster name. If x != 0, return the uname of the system.
*/
static MS_Gotos builtin_mailsystem(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    int i = VarReference_get__double(parameters->array[0]->vr);
    VarReference_assign__charstring(ret, mailsystem(i));
    return MS_next;
}

/* match(var str, var pat, reference length)
    Returns the position in string str where the regular expression pat occurs,
    0 if it does not occur at all, or -1 if the pattern is illegal.
    The length variable is set to the length of the matched string.
*/
static MS_Gotos builtin_match(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_str = parameters->array[0]->vr;
    VarReference *var_pat = parameters->array[1]->vr;
    VarReference *var_len = parameters->array[2]->vr;
    string *str = VarReference_get__string(var_str);
    string *pat = VarReference_get__string(var_pat);
    re_re *regex;
    char *match[10][2];

    VarReference_clear_value(ret);

    /* compile the pattern */
    regex = compile_string(pat, 0);
    if (!regex)
	{
	VarReference_assign__double(ret, -1.0);
	VarReference_clear_value(var_len);
	return MS_next;
	}

    /* find the match */
    if (re_reexec(regex, s_to_c(str), s_to_c(str) + s_curlen(str), match))
	{
	VarReference_assign__double(ret, match[0][0] - s_to_c(str) + 1);
	VarReference_assign__double(var_len, match[0][1] - match[0][0]);
	}

    else
	{
	VarReference_assign__double(ret, 0.0);
	VarReference_clear_value(var_len);
	}
   return MS_next;
}

/* messageid()
    Return a string suitable for use in a Message-ID: or Content-ID:
    header. Each invocation returns a unique string.
*/
static MS_Gotos builtin_messageid(ret)
VarReference *ret;
{
    string *msgid = getmessageid(0);
    VarReference_assign__string(ret, msgid);
    s_free(msgid);
    return MS_next;
}

/* mgetenv(var x)
    Return the value of the mail configuration variable with the name x.
*/
static MS_Gotos builtin_mgetenv(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    string *s = VarReference_get__string(parameters->array[0]->vr);
    const char *env = mgetenv(s_to_c(s));
    VarReference_assign__charstring(ret, env ? env : "");
    return MS_next;
}

/*
    Insert a given header at the location "i" in mailr_allheaders.
    This function is used by prepend_header()
*/
static void inserthdr_here(i, hdrtag, hdrtype, val)
int i;
string *hdrtag;
int hdrtype;
string *val;
{
    VarReference *vr;
    Value *v;
    int j;

    /* shift headers to the right */
    VarReference_Array_grow(mailr_allheaders, 1);
    for (j = mailr_allheaders->arraycount - 1; j > i; j--)
	mailr_allheaders->array[j] = mailr_allheaders->array[j-1];

    /* create a new element to plug in */
    v = Value_new__string_double(val, 0.0);
    vr = VarReference_new__Header(v, s_to_c(hdrtag), hdrtype);
    mailr_allheaders->array[i] = ArrayElement_new(double_to_string(i), vr);
}

/* prepend_header(var x, var beforehdr, var val)
    Create a new header of type x before the first header of type beforehdr with value
    val. If no header of type beforehdr exists, the header will be created before all other
    headers, but after the >From UNIX Postmark headers. For example,
    prepend_header("Date", "To", rfc822date(0)) will create a new header named Date:
    before the first To: header with the value returned from the rfc822date()
    function.
*/
static MS_Gotos builtin_prepend_header(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_x =	  parameters->array[0]->vr;
    VarReference *var_beforehdr = parameters->array[1]->vr;
    VarReference *var_val =	  parameters->array[2]->vr;
    string *x = VarReference_get__string(var_x);
    string *beforehdr = VarReference_get__string(var_beforehdr);
    string *val = VarReference_get__string(var_val);
    int i;

    set_header_colon(x);
    set_header_colon(beforehdr);

    /* look for headers of that name */
    for (i = 0; i < mailr_allheaders->arraycount; i++)
	if (cascmp(s_to_c(x), s_to_c(mailr_allheaders->array[i]->vr->hdrtag)) == 0)
	    {
	    inserthdr_here(i, x, -1, val);
	    break;
	    }

    /* not found, create a new one */
    if (i == mailr_allheaders->arraycount)
	{
	/* look for end of From/>From headers. Leave i pointing there. */
	for (i = 0; i < mailr_allheaders->arraycount; i++)
	    if ((strcmp("From ", s_to_c(mailr_allheaders->array[i]->vr->hdrtag)) != 0) &&
		(strcmp(">From ", s_to_c(mailr_allheaders->array[i]->vr->hdrtag)) != 0))
		break;
	inserthdr_here(i, x, -1, val);
	}

    VarReference_assign__double(ret, 1.0);
    return MS_next;
}

/* print(reference expr)
    Print the string on standard output.
*/
static MS_Gotos builtin_print(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_expr = parameters->array[0]->vr;
    string *s = VarReference_get__string(var_expr);
    (void) fputs(s_to_c(s), stdout);
    VarReference_assign__double(ret, 1.0);
    return MS_next;
}

/* print_headers()
    Print the current headers on standard output.
*/
static MS_Gotos builtin_print_headers(ret)
VarReference *ret;
{
    pfmt(stdout, MM_NOSTD, ":560:Headers:\n");
    VarReference_print(stdout, mailr_allheaders);
    VarReference_assign__double(ret, 1.0);
    return MS_next;
}

/* rand()
    Return a random number between 0 and 1.
*/
static MS_Gotos builtin_rand(ret)
VarReference *ret;
{
    VarReference_clear_value(ret);
    VarReference_assign__double(ret, rand() / (double)RAND_MAX);
    return MS_next;
}

/* recipients()
    Return an array of the recipients of this message.
*/
static char **recipients_list;

void mailR_set_recipients(argv)
char **argv;
{
    recipients_list = argv;
}

static MS_Gotos builtin_recipients(ret)
VarReference *ret;
{
    VarReference *arrayvar;
    int i;

    VarReference_clear_value(ret);
    arrayvar = VarReference_Array_set(ret);
    if (recipients_list)
	for (i = 1; recipients_list[i]; i++)
	    VarReference_Array_add__double_string(arrayvar, (double)(i-1), s_copy(recipients_list[i]));

    return MS_next;
}

/* returnpath()
    Return the return path of the person who sent the mail message.
*/
static MS_Gotos builtin_returnpath(ret)
VarReference *ret;
{
    VarReference_clear_value(ret);
    VarReference_assign__charstring(ret, s_to_c(mailr_pmsg->Rpath));
    return MS_next;
}


/* rfc822date(var seconds)
    Return the date and time formatted according to the RFC 822 standard.
    The seconds is the time since January 1, 1970. If 0 is specified, the
    current time is used.
*/
static MS_Gotos builtin_rfc822date(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_seconds = parameters->array[0]->vr;
    char datestring[60];
    char RFC822datestring[60];
    long ltmp = VarReference_get__double(var_seconds);

    if (!ltmp) ltmp = time((long*)0);
    fromdate(datestring, ltmp);
    rfc822date(datestring, RFC822datestring);
    VarReference_assign__charstring(ret, RFC822datestring);
    return MS_next;
}

/* split(var x, var y)
    Return an array created from x, split on each occurrence of the string y.
*/
static MS_Gotos builtin_split(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_x = parameters->array[0]->vr;
    VarReference *var_y = parameters->array[1]->vr;
    VarReference *arrayvar;
    string *x = s_dup(VarReference_get__string(var_x));
    string *y = VarReference_get__string(var_y);
    string *line;
    double index = 0;

    VarReference_clear_value(ret);
    arrayvar = VarReference_Array_set(ret);

    s_restart(x);
    /* loop through the tokens */
    while ((line = s_etok(x, s_to_c(y))) != 0)
	{
	VarReference_Array_add__double_string(arrayvar, index, line);
	s_free(line);
	index++;
	}
    s_free(x);
    return MS_next;
}

/* split_pattern(var x, var y)
    Return an array created from x, split on each occurrence of the regular
    expression pattern y.
*/
static MS_Gotos builtin_split_pattern(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_x =   parameters->array[0]->vr;
    VarReference *var_pat = parameters->array[1]->vr;
    VarReference *arrayvar;
    string *x = VarReference_get__string(var_x);
    string *pat = VarReference_get__string(var_pat);
    double index = 0;
    re_re *regex = compile_string(pat, 0);
    char *match[10][2];
    char *beg, *end;

    VarReference_clear_value(ret);
    if (!regex)
	return MS_next;

    arrayvar = VarReference_Array_set(ret);

    beg = s_to_c(x);
    end = s_to_c(x) + s_curlen(x);
    /* loop through the matches */
    while (re_reexec(regex, beg, end, match))
	{
	int substrlen = match[0][0] - beg;
	string *line = s_copy_reserve(beg, substrlen + 1, substrlen + 1);
	s_terminate(line);
	VarReference_Array_add__double_string(arrayvar, index, line);
	s_free(line);
	beg = match[0][1];
	index++;
	}

    /* also return any trailing piece of the string */
    if (beg < end)
	{
	int substrlen = end - beg;
	string *line = s_copy_reserve(beg, substrlen + 1, substrlen + 1);
	s_terminate(line);
	VarReference_Array_add__double_string(arrayvar, index, line);
	s_free(line);
	index++;
	}

    re_refree(regex);
    return MS_next;
}

/* srand(var seed)
    Initialize the random number generator used by rand() to the given seed.
    If the seed is zero, the current time of day is used.
    The seed used is returned.
*/
static MS_Gotos builtin_srand(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_seed = parameters->array[0]->vr;
    unsigned seed = VarReference_get__double(var_seed);

    if (seed == 0)
	seed = time((long*)0);
    srand(seed);
    VarReference_assign__double(ret, (double)seed);
    return MS_next;
}

/* strrstr(var s1, var s2)
    Finds s2 in s1 starting from end and returns its position (1-n),
    0 if s2 not present.
*/
static MS_Gotos builtin_strrstr(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_s1 = parameters->array[0]->vr;
    VarReference *var_s2 = parameters->array[1]->vr;
    string *s1 = VarReference_get__string(var_s1);
    string *s2 = VarReference_get__string(var_s2);
    int len1 = s_curlen(s1);
    int len2 = s_curlen(s2);
    int i;
    for (i = len1 - len2; i >= 0; i--)
	{
	int j;
	for (j = 0; j < len2 && s_to_c(s1)[i+j] == s_to_c(s2)[j]; j++)
	    ;
	if (j == len2)
	    {
	    VarReference_assign__double(ret, (double)(i+1));
	    return MS_next;
	    }
	}
    VarReference_assign__double(ret, 0.0);
    return MS_next;
}

/* strstr(var s1, var s2)
    Finds s2 in s1 and returns its position (1-n), 0 if s2 not present.
*/
static MS_Gotos builtin_strstr(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    string *s1 = VarReference_get__string(parameters->array[0]->vr);
    string *s2 = VarReference_get__string(parameters->array[1]->vr);
    double d = substr(s_to_c(s1), s_to_c(s2)) + 1;
    VarReference_assign__double(ret, d);
    return MS_next;
}

/* substr(var s, var m, var n)
    Return the n-character substring of s that begins at position m (1-based).
*/
static MS_Gotos builtin_substr(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    VarReference *var_s = parameters->array[0]->vr;
    VarReference *var_m = parameters->array[1]->vr;
    VarReference *var_n = parameters->array[2]->vr;
    string *s = VarReference_get__string(var_s);
    int m = (int)VarReference_get__double(var_m) - 1;	/* convert to 0-base */
    int n = (int)VarReference_get__double(var_n);
    string *sret;

    if (m < 0) m = 0;					/* have to start at left */
    if (n < 0) n = 0;					/* 0 bytes minimum */
    if (m > s_curlen(s)) m = s_curlen(s);		/* can't go past right edge */
    if (m + n > s_curlen(s)) n = s_curlen(s) - m;	/* can't go past right edge */

    sret = s_copy_reserve(s_to_c(s) + m, n+1, n+1);
    s_terminate(sret);
    VarReference_assign__string(ret, sret);
    s_free(sret);
    return MS_next;
}

/* substitute(var s1, var pat, var repl)
    Return the string created by using the pattern pat on string s1, making
    the first replacement as controlled by repl. For example, substitute("system!user",
    "(.*)!(.*)", "\\2@\\1") would return the string "user@system".
*/
static MS_Gotos builtin_substitute(parameters, ret)
VarReference *parameters;
VarReference *ret;
{
    return do_substitution(parameters, ret, 0);
}

/* sysname()
    If there is a domain name, then sysname() is equivalent to mailsystem(0).domain().
    Otherwise, it is equivalent to mailsystem(0).
*/
static MS_Gotos builtin_sysname(ret)
VarReference *ret;
{
    const char *dom = maildomain();
    const char *sys = mailsystem(0);
    if (dom[0])
	{
	string *s = s_xappend((string*)0, sys, dom, (char*)0);
	VarReference_assign__string(ret, s);
	s_free(s);
	}
    else
	VarReference_assign__charstring(ret, sys);
    return MS_next;
}

/* texttype()
    Returns the strings "text", "generic-text" or "binary" depending on if the
    message contains strictly 7-bit text, 8-bit text or binary contents.
*/
static MS_Gotos builtin_texttype(ret)
VarReference *ret;
{
    VarReference_assign__charstring(ret,
	(mailr_pmsg->binflag == C_Text)  ? "text" :
	(mailr_pmsg->binflag == C_GText) ? "generic-text" :
					   "binary");
    return MS_next;
}

/* time()
    Return the current time as a number of seconds since Jan. 1, 1970.
*/
static MS_Gotos builtin_time(ret)
VarReference *ret;
{
    VarReference_assign__double(ret, (double)time((long*)0));
    return MS_next;
}

/* username()
    Return the name of the user executing this command.
*/
static MS_Gotos builtin_username(ret)
VarReference *ret;
{
    VarReference_assign__charstring(ret, my_name);
    return MS_next;
}

/* -------------------------------- mailR Built In functions -------------------------------- */

/* a list of all built in functions */
static struct builtin_functions builtinlist[] =
    {	/* NOTE: this list must be sorted so that bsearch() can work */
	{ "addrparse", 0, builtin_addrparse, 1, 0 },
	{ "append_header", 0, builtin_append_header, 3, 0 },
	{ "characterset", builtin_characterset, 0, 0, 0 },
	{ "delete_header", 0, builtin_delete_header, 1, 0 },
	{ "domain", builtin_domain, 0, 0, 0 },
	{ "exists", 0, builtin_exists, 1, 0 },
	{ "filter", 0, builtin_filter, 1, 0 },
	{ "filterbody", 0, builtin_filterbody, 1, 0 },
	{ "filterheaders", 0, builtin_filterheaders, 1, 0 },
	{ "filterstring", 0, builtin_filterstring, 3, 0 },
	{ "fromcrack", 0, builtin_fromcrack, 5, 0 },
	{ "fromdate", 0, builtin_fromdate, 1, 0 },
	{ "getdate", 0, builtin_getdate, 1, 0 },
	{ "getenv", 0, builtin_getenv, 1, 0 },
	{ "gsubstitute", 0, builtin_gsubstitute, 3, 0 },
	{ "headers", 0, builtin_headers, 1, 0 },
	{ "headers_pattern", 0, builtin_headers_pattern, 1, 0 },
	{ "int", 0, builtin_int, 1, 0 },
	{ "isbinary", builtin_isbinary, 0, 0, 0 },
	{ "length", 0, builtin_length, 1, 0 },
	{ "lines", 0, builtin_lines, 1, 0 },
	{ "localmessage", builtin_localmessage, 0, 0, 0 },
	{ "mailsystem", 0, builtin_mailsystem, 1, 0 },
	{ "match", 0, builtin_match, 3, 0 },
	{ "messageid", builtin_messageid, 0, 0, 0 },
	{ "mgetenv", 0, builtin_mgetenv, 1, 0 },
	{ "prepend_header", 0, builtin_prepend_header, 3, 0 },
	{ "print", 0, builtin_print, 1, 0 },
	{ "print_headers", builtin_print_headers, 0, 0, 0 },
	{ "rand", builtin_rand, 0, 0, 0 },
	{ "recipients", builtin_recipients, 0, 0, 0 },
	{ "returnpath", builtin_returnpath, 0, 0, 0 },
	{ "rfc822date", 0, builtin_rfc822date, 1, 0 },
	{ "split", 0, builtin_split, 2, 0 },
	{ "split_pattern", 0, builtin_split_pattern, 2, 0 },
	{ "srand", 0, builtin_srand, 1, 0 },
	{ "strrstr", 0, builtin_strrstr, 2, 0 },
	{ "strstr", 0, builtin_strstr, 2, 0 },
	{ "substitute", 0, builtin_substitute, 3, 0 },
	{ "substr", 0, builtin_substr, 3, 0 },
	{ "sysname", builtin_sysname, 0, 0, 0 },
	{ "texttype", builtin_texttype, 0, 0, 0 },
	{ "time", builtin_time, 0, 0, 0 },
	{ "username", builtin_username, 0, 0, 0 },
	{ 0, 0, 0, 0 }
    };

/* a function for use by bsearch to search for a builtin function */
static int builtin_functions_cmp(v1, v2)
const VOID *v1;
const VOID *v2;
{
    return strcmp(((struct builtin_functions*)v1)->name, ((struct builtin_functions*)v2)->name);
}

/*
    Find the given mailR function with the given name. It will either be
    a user-defined function, or a built-in function. If it's a built-in
    and a tree representation hasn't been created yet, create one.
*/
static TreeNode *find_function_TreeNode(function, complain)
const char *function;
int complain;
{
    TreeNode *top;
    char firstchar;
    struct builtin_functions srchstring;
    struct builtin_functions *ret;

    /* first search in the builtin list */
    srchstring.name = function;
    ret = (struct builtin_functions *) bsearch((char*)&srchstring, (char*)builtinlist, nelements(builtinlist), sizeof(builtinlist[0]), builtin_functions_cmp);
    if (ret)
	{
	if (mailr_debug) (void) fprintf (stderr, "found function '%s' on builtin tree\n", function);
	if (!ret->treeptr)	/* Create a tree representation if we don't already have one */
	    {			/* so it can be passed to the tree evaluation function. */
	    TreeNode *t = TreeNode_new((TreeNode*)0, (TreeNode*)0, TCfunction);
	    t->function0 = ret->function0;
	    t->functionn = ret->functionn;
	    t->argcount = ret->argcount;
	    t->next = 0;
	    t->name = s_copy(function);
	    ret->treeptr = t;
	    }
	return ret->treeptr;
	}

    /* now search in the user-defined list */
    firstchar = function[0];
    for (top = mailr_treetop; top; top = top->next)
	if (top->name &&
	    (firstchar == s_to_c(top->name)[0]) &&
	    (strcmp(function+1, s_to_c(top->name)+1) == 0))
	    {
	    if (mailr_debug) (void) fprintf (stderr, "found function '%s' on expr tree\n", function);
	    return top;
	    }

    if (complain)
	pfmt(stderr, MM_ERROR, ":553:Cannot find mailR function '%s'\n", function);
    return 0;
}

/*
????
	bangtodomain(user) convert @%! -> @
*/
