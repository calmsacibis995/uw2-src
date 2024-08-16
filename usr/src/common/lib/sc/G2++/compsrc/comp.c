/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident       "@(#)sc:G2++/compsrc/comp.c	3.6" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

//  g2++comp - Compile g2++ record definitions
//
//  g2++comp compiles the g2++ record definitions in one or 
//  more files.  The files must have a ".g" suffix in their 
//  name to be considered.  Each ".g" file is compiled into 
//  corresponding ".h" and ".c" files.
//
//  g2++comp is an extension of g2comp, written by
//  Jim Weythman.  

#include <g2comp.h>
#include <g2ctype.h>
#include <g2debug.h>
#include <g2io.h>
#include <g2mach.h>

#include <ctype.h>
#ifdef IOSTREAMH
#include <fstream.h>
#else
#include <stream.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stream.h>
#include <String.h>

//  Global data

static String	progname;
static String	filename;
static int	err;
static G2NODE*	stack[G2MAXDEPTH_ATTLC];   // should use a block
static long 	seed;  // for ranstr()

static G2BUF*  gbuf; // global so transform can grow gbuf->buf
static G2NODE* head;
static G2NODE* tail;

static G2NODE nulldef;
static G2NODE chardef;
static G2NODE shortdef;
static G2NODE longdef;
static G2NODE stringdef;

static G2BUF char_def; 
static G2BUF short_def;
static G2BUF long_def;
static G2BUF string_def;

struct mtbl{
    G2BUF*	buf;
    mtbl* 	next;
};

static mtbl*	mtblhead=0;
static mtbl*	mtbltail=0;

Map<String,udt_info_ATTLC> udt_map_ATTLC;

//  Local functions

static mtbl* addtbl(G2BUF*);
static void bdef(G2NODE* t, FILE* hf, FILE* cf);
static void blockdef(G2NODE* t, FILE* hf, FILE* cf);
static void bolt(const String& stem, FILE* hf, FILE* cf);
static int dotg(char*);
static void errf(int level, const char* msg);
static void errf(int level, const char* msg, const char* arg);
static void errf(int level, const char* msg, const char* arg1,
    const char* arg2);
static void errf(int level, const char* msg, const char* arg1,
    const char* arg2, const char* arg3);
static void error(const char* msg);
static void error(const char* msg, const char* a);
static void error(const char* msg, const char* a, const char* b);
static void error(const char* msg, const char* a, long b);
static void extract_udt_info(G2NODE* gp);
static void generate(G2NODE* t, FILE* hf, FILE* cf);
static void gen_io(G2NODE* t, FILE* hf, FILE* cf);
static void include_udt(FILE* f);
static String lower(const String&);
static void nullify(FILE*);
static FILE* openfile(const String& filename);
static void parse(const String& path, String& dir, String& stem);
static String pathname(int level);
#ifdef IOSTREAMH
static int perfile(ifstream&, const String& stem, const String& hfilename,
    const String& cfilename);
#else
static int perfile(istream&, const String& stem, const String& hfilename,
    const String& cfilename);
#endif
static void prologue(FILE* hf, FILE* cf, const String& stem,
    const String& hfilename);
static void prtbls(const String& stem, FILE* hf, FILE* cf);
static String ranstr(int);
static void remtbl();
static void resettbl();
static void show_udt_info(udt_info_ATTLC x);
static void show_udt_map();
static void sizealign(FILE* cf);
static void transform(G2NODE* p);
static int valid(int level, G2NODE* gp);
static void valid1(int, G2NODE*);
static void wrap(FILE*);

main(int argc, char* argv[]){
    String	stem;
    String 	dir;
    int		i; 
#ifdef IOSTREAMH
    ifstream 	ins;	      // input fstream
#else
    FILE*	f_file;
#endif

    progname = argv[0];

    seed = time((time_t*)0);
    srand((unsigned)seed);
	    
//  Initialize table of G2NODE 

    resettbl();	  

//  Loop over .g arguments in command line

    for(i = 1; i < argc; i++){
	if(!dotg(argv[i])){
	    continue;
	}
#ifdef IOSTREAMH
	ifstream ins(argv[i],ios::in);

	if(!ins){
	    error("cannot open '%s': ", argv[i]);
	    perror("");
	    err++;
	    continue;
	}
#else
	if ((f_file = fopen(argv[i],"r")) == NULL) {
	    error("cannot open '%s': ", argv[i]);
	    perror("");
	    err++;
	    continue;
	}
	istream ins(f_file);
#endif
	filename = String(argv[i]);  //  might be "a/b/c/d.g"
	DEBUG_G2(cerr << "in main, filename=" << filename << "\n" ;)

//  Display the filename on stderr

	fprintf(stderr, "%s:\n", (const char*)filename);

//  Extract directory part of the filename into 'dir' 
//  and file part (less .g) into 'stem'

	parse(
	    filename,         // might be a/b/c/d.g
	    dir,              // would be a/b/c
	    stem              // would be d
	);
	DEBUG_G2(cerr << "on return from parse:\n" << "    dir=" << dir << "\n"
	   << "    stem=" << stem << "\n" ;)

//  We will put the .h and .c files in the current directory

	String hfilename = /* dir + "/" + */ stem + ".h";
	String cfilename = /* dir + "/" + */ stem + ".c";
	DEBUG_G2(cerr << "    hfilename=" << hfilename << "\n" ;)
	DEBUG_G2(cerr << "    cfilename=" << cfilename << "\n" ;)

//  Do it

	if(perfile(ins,stem,hfilename,cfilename)){

//  Success!  Display the rest of the message to user

	    fprintf(stderr, " => %s.[hc]\n", (const char*)stem);
	}

//  Prepare to process the next .g file

	ins.close();
	resettbl();
    }
    return err;
}

static int 
valid(int level, G2NODE* gp){
    DEBUG_G2(cerr << "enter valid with level=" << level << "\n" ;)
    int	saverr = err;
    valid1(level, gp);
    return err - saverr;
}

static void 
valid1(int level, G2NODE* gp){
    stack[level] = gp;
    DEBUG_G2(cerr << "enter valid1 with level = " << level << ", gp = " << "\n" ;)
    DEBUG_G2( showtree_ATTLC(gp,0); )
    String temp=g2name_ATTLC(gp); // elim "sorry not implemented"
    DEBUG_G2( cerr << "temp = " << temp << endl; ) 
    DEBUG_G2(
	cerr << "stack contains:" << endl;
	for(int k = 0;k<=level;k++){
	    cerr << "    " << g2name_ATTLC(stack[k]) << endl;
	}
    )
    if(level<=0 && !isname_ATTLC(temp) ){
	errf(level, " root is not a name\n");
	err++;
	return;
    }
    if(isname_ATTLC(temp)){
	G2NODE* next=g2next_ATTLC(gp);
	if(next){
	    temp=g2name_ATTLC(next);

	    if(!temp.is_empty() && isint_ATTLC(temp)){
		stack[level] = next;
		errf(level, " mixed names and indexes\n");
		err++;
	    }

	}else{

//  Check for variable size String declared at level zero
//  and issue initial allocation warning message

	    temp = g2val_ATTLC(gp);
	    const char* pn = (const char*)temp;
	    int n = atoi(pn);
	    DEBUG_G2( cerr << "max size = " << n << endl; )

	    if(n == 0 && temp.length() >= 4 && temp.char_at(1) == '('){
		n = atoi(pn+2);
	    }
	    DEBUG_G2( cerr << "initial capacity = " << n << endl; )
	    if(n > 0 && level==0){
		String nn = int_to_str(n);
		String type = upper(
		    g2name_ATTLC(stack[0])
		);
		errf(
		    level, 
		    " warning: %s will not be used as the initial string size; for proper preallocation, use a constructor argument, e.g. %s x(Stringsize(%s));\n",
		    (const char*)nn,
		    (const char*)type,
		    (const char*)nn
		); 
	    }
	}

    }else if(isdigit_ATTLC(temp.char_at(0))){

//  Array 
//
//  Remember: all arrays should be at level 0,
//  so we should now be at level 1 looking at the
//  size info.

	DEBUG_G2(cerr << "array" << endl; )
	G2NODE* next=g2next_ATTLC(gp);

	if(next){
	    stack[level] = next;

	    if(isint_ATTLC(g2name_ATTLC(next))){
		errf(level, " only one dimension allowed per level\n");
	    }else{
		errf(level, " mixed names and indexes\n");
	    }
	    err++;

	}else{

//  Decode size and initial allocation information

	    const char* pn = (const char*)temp;
	    int n = atoi(pn);
	    DEBUG_G2( cerr << "max size = " << n << endl; )

	    if(n == 0 && temp.length() >= 4 && temp.char_at(1) == '('){
		n = atoi(pn+2);
	    }
	    DEBUG_G2( cerr << "initial capacity = " << n << endl; )
	    if(n > 0){
		String pval = g2val_ATTLC(stack[level-1]);
		DEBUG_G2( cerr << "level, pval = " << level << ", " << pval << endl; )
		if(level==1 && pval != "HOISTED"){
		    DEBUG_G2(cerr << "level==1 && pval!=HOISTED" << endl;)

//  Corrective action: use a constructor argument 

		    String nn = int_to_str(n);
		    String type = upper(g2name_ATTLC(stack[0]));
		    errf(
			level, 
			" warning: %s will not be used as the initial array capacity; for proper preallocation, use a constructor argument, e.g. %s x(%s);\n",
			(const char*)nn,
			(const char*)type,
			(const char*)nn
		    ); 

		}else{

//  level>1 || (level==1 && pval == HOISTED)
//  Corrective action: modify record structure

		    DEBUG_G2( cerr << "level>1 || (level==1 && pval == HOISTED)" << endl; )
		    String nn = int_to_str(n);

		    errf(
			level,
			" warning: %s will not be used as the initial array capacity; consider making this array the member of a structure\n",
			(const char*)nn
		    ); 

		}
	    }

//  Issue warning for array elements declared as n or 0(n)

	    pn = (const char*)gp->val;
	    n = atoi(pn);
	    DEBUG_G2( cerr << "max size = " << n << endl; )

	    if(n == 0 && gp->val.length() >= 4 &&
		gp->val.char_at(1) == '('){
		n = atoi(pn+2);
	    }
	    DEBUG_G2( cerr << "initial capacity = " << n << endl; )
	    if(n > 0){
		DEBUG_G2(cerr << "n>0: issue array element warning" << endl;)
		char buf[10];
		sprintf(buf,"%d",n);
		errf(level, " warning: %s will not be used as the initial string capacity; consider redefining the element type as a structure\n",buf);
	    }
        }

    }else{

	errf(level, " name is neither a name nor an index\n");
	err++;
    }
    temp=g2val_ATTLC(gp);

    if(temp.is_empty() && !g2child_ATTLC(gp)){
	errf(level, " missing value\n");
	err++;
    }
    if(!g2child_ATTLC(gp)){
	temp=g2val_ATTLC(gp);
	DEBUG_G2(cerr << "gp has no children; set temp=" << temp << "\n" ;)
	if(!isdigit_ATTLC(temp.char_at(0))){
	    
//  Cludge -- prevent user from saying "STRING"

	    if(temp=="STRING"){
		errf(level, " type '%s' not defined\n", (const char*)temp);
		err++;
	    }else{
		if(!lookup(temp) ){
		    if(!udt_map_ATTLC.element(temp)){
			errf(level, " type '%s' not defined\n",
			    (const char*)temp);
			err++;
		    }
		}
	    }
	}
    }
    DEBUG_G2( cerr << "ready to call valid1 recursively for children" << endl; )
    for(G2NODE* cp=g2child_ATTLC(gp); cp; cp=g2next_ATTLC(cp)){
	valid1(level+1,cp);
    }
}

static void 
errf(int level, const char* msg){
    fprintf(stderr, "%s: ", (const char*)progname);
    if(filename.length()>0){
	fprintf(stderr, "file '%s': ", (const char*)filename);
    }
    fprintf(stderr, "path '%s'", (const char*)pathname(level));
    fprintf(stderr, msg);
}

static void 
errf(int level, const char* msg, const char* arg){
    fprintf(stderr, "%s: ", (const char*)progname);
    if(filename.length()>0){
	fprintf(stderr, "file '%s': ", (const char*)filename);
    }
    fprintf(stderr, "path '%s'", (const char*)pathname(level));
    fprintf(stderr, msg, arg);
}

static void 
errf(int level, const char*  msg, const char* arg1, const char* arg2){
    fprintf(stderr, "%s: ", (const char*)progname);
    if(filename.length()>0){
	fprintf(stderr, "file '%s': ", (const char*)filename);
    }
    fprintf(stderr, "path '%s'", (const char*)pathname(level));
    fprintf(stderr, msg, arg1, arg2);
}

static void 
errf(int level, const char* msg, const char* arg1, const char* arg2,
 const char* arg3){
    fprintf(stderr, "%s: ", (const char*)progname);
    if(filename.length()>0){
	fprintf(stderr, "file '%s': ", (const char*)filename);
    }
    fprintf(stderr, "path '%s'", (const char*)pathname(level));
    fprintf(stderr, msg, arg1, arg2, arg3);
}

static String 
pathname(int level){
    String result="";

    for(int i = 0; i <= level; i++){
	String name;

        if(g2val_ATTLC(stack[i]) == "HOISTED"){
	    name = "?";
	}else{
	    name=g2name_ATTLC(stack[i]);
	}
	result+=name;

	if(i<level){
	    result += ".";
	}
    }
    return result;
}

String 
upper(const String& s){
    String result(Stringsize(s.length()));;

    for( int i=0;i<s.length();i++ ){

	if( islower(s.char_at(i)) ){
	    result += toupper(s.char_at(i));
	}else{
	    result += s.char_at(i);
	}
    }
    return result;
}

G2NODE* 
lookup(const String& name){
    if(isdigit_ATTLC(name.char_at(0))){  // optimization
	return NULL;
    }
    DEBUG_G2(cerr << "enter lookup with name=" << name << "\n" ;)
    for(mtbl* mp = mtblhead; mp; mp = mp->next){
	DEBUG_G2(cerr << "consider buffer at address " 
	    << long(mp) << "\n" ;)
	for(G2NODE* t=g2root_ATTLC(mp->buf); t; t=g2next_ATTLC(t)){
	    DEBUG_G2(cerr << "consider G2NODE t:\n";)
	    DEBUG_G2(shownode_ATTLC(t);)
	    DEBUG_G2(cerr << "g2name_ATTLC(t)=" 
		<< g2name_ATTLC(t) << "\n" ;)
	    if(name == g2name_ATTLC(t)){
		DEBUG_G2(cerr << "hit!\n";)
		return t;
	    }
	}
	DEBUG_G2(cerr << "exit from inner forloop\n";)
    }
    DEBUG_G2(cerr << "lookup failed!\n";)
    return NULL;
}

static void 
error(const char* msg){
    fprintf(stderr, "%s: ", (const char*)progname);
    if( filename ){
	fprintf(stderr, "file '%s': ", (const char*)filename);
    }
    fprintf(stderr, msg);
}
static void 
error(const char* msg, const char* a){
    fprintf(stderr, "%s: ", (const char*)progname);
    if( filename ){
	fprintf(stderr, "file '%s': ", (const char*)filename);
    }
    fprintf(stderr, msg, a);
}
static void 
error(const char* msg, const char* a, const char* b){
    fprintf(stderr, "%s: ", (const char*)progname);
    if( filename ){
	fprintf(stderr, "file '%s': ", (const char*)filename);
    }
    fprintf(stderr, msg, a, b);
}
static void 
error(const char* msg, const char* a, long b){
    fprintf(stderr, "%s: ", (const char*)progname);
    if( filename ){
	fprintf(stderr, "file '%s': ", (const char*)filename);
    }
    fprintf(stderr, msg, a, b);
}

static FILE* 
openfile(const String& filename){
    DEBUG_G2(cerr << "enter openfile with filename=" << filename << "\n" ;)
    FILE* result=fopen(filename, "w");

    if(result == NULL){
	error("cannot open '%s': ", (const char*)filename);
	perror("");
	err++;
    }
    return result;
}

static int 
dotg(char name[]){
// screen out files w/o '.g' suffix 

    if(strlen(name) <= 2 || strcmp(name+strlen(name)-2,".g")){
	error("skipping '%s'; doesn't have a '.g' suffix\n", name);
	err++;
	return 0;
    }
    return 1;
}

static void 
parse(

//  Parse directory part of path into 'dir' 
//  and file part (less '.g') into 'stem'

    const String&	path,     // could be a/b/c/d.g
    String& 		dir,      // would be a/b/c
    String& 		stem      // would be d
){
    DEBUG_G2(cerr << "enter parse with path=" << path << "\n" ;)
    int index = path.length()-1;

    while(index>=0){
	if(path[index]=='/'){
	    break;
	}
	index--;
    }
    DEBUG_G2(cerr << "break from loop with index=" << index << "\n" ;)
    if(index<0){
	dir="";
    }else{
	dir=path.chunk(0,index);
    }
    DEBUG_G2(cerr << "dir=" << dir << "\n" ;)
    stem = path.chunk(
	index+1,               // starting character
	path.length()-index-3  // number of characters
    );
    DEBUG_G2(cerr << "stem=" << stem << "\n" ;)
}

static void 
prologue(FILE* hf, FILE* cf, const String& stem, const String& hfilename){
    DEBUG_G2(cerr << "enter prologue with stem=" << stem << "\n" << "hfilename=" 
	<< hfilename << "\n" ;)
    fprintf(hf, "#include <String.h>\n");
    fprintf(hf, "#include \"Vblock.h\"\n\n");
    fprintf(hf, "#include \"g2++.h\"\n\n");
    fprintf(hf, "class istream;\nclass ostream;\n\n"); 
    fprintf(hf, "typedef int %sTYPE;\n", (const char*)upper(stem));
    fprintf(hf, "typedef long LONG;\n");
    fprintf(hf, "typedef char CHAR;\n");
    fprintf(hf, "typedef short SHORT;\n");
    fprintf(hf, "typedef String STRING;\n");
    fprintf(hf, "\n");
    fflush(hf);  // Debug

//  Write necessary #include's to cf

    fprintf(cf, "#include \"%s\"\n", (const char*)hfilename);
    fprintf(cf, "#include \"g2desc.h\"\n");
    fprintf(cf, "#include \"g2inline.h\"\n");
    fprintf(cf, "#include \"g2io.h\"\n");
    fprintf(cf, "#include \"g2mach.h\"\n");
    fprintf(cf, "#include <stream.h>\n");
    fprintf(cf, "\n");
    fflush(cf);  // Debug

//  Generate declaration for array used to hold intermediate 
//  alignment expressions in constructors

    fprintf(cf,"static int align_val[%d];\n",G2MAXDEPTH_ATTLC);
}

static int
#ifdef IOSTREAMH
perfile(ifstream& ins, const String& stem, const String& hfilename,
 const String& cfilename){
#else
perfile(istream& ins, const String& stem, const String& hfilename,
 const String& cfilename){
#endif
    FILE* hf;
    FILE* cf;

//  Use untyped I/O to extract information from 
//  g2 record definitions in ifstream 'ins'

    G2NODE* t;
    mtbl* mp;
    mtbl* first = 0;
    int OK = 1;

    for(;;){

//  Get a new buffer

	if(!(gbuf=new G2BUF)){
	    error("out of memory\n");
	    exit(1);
	}
	if(!getbuf_ATTLC(gbuf,ins)){
	    DEBUG_G2(cerr << "getbuf_ATTLC returns EOF -- break\n";)
	    break;
	}
	DEBUG_G2(cerr << "in perfile, getbuf_ATTLC returns *gbuf=\n";)
	DEBUG_G2(showbuf_ATTLC(gbuf);)

	if( gbuf->root->val == "USER" ){
	    DEBUG_G2(cerr << "USER type!\n";)

//  This is a USER type: store info in udt_map_ATTLC.
//  
//  Note: I tried to do this in a more uniform
//  fashion (treating user-defined type as an
//  ordinary type definition (linking it into
//  mtbl, etc.), and ran into lots of problems.
//  Now, all information is stored in a separate
//  table, udt_map_ATTLC.

	    extract_udt_info(gbuf->root);

	}else{
	    DEBUG_G2(cerr << "ordinary record type\n";)

//  Transform the type tree rooted at gbuf->root:
//
//      o  Hoist array types to level 0 
//      o  Change string and block definitions
//      o  Generate structure tags for anonymous structures

//  Begin debug:

	    DEBUG_G2(cerr << "before calling transform, ";)
	    DEBUG_G2(cerr << "list of type trees=\n";)
	    DEBUG_G2(
		for(t=gbuf->root; t; t=t->next){
		    showtree_ATTLC(t,0);
		}
	    )

//  End debug

	    head=tail=0;
	    transform(gbuf->root);

//  Debug: display the list of hoisted types

	    G2NODE* ptr=tail;
	    DEBUG_G2(cerr << "tail:\n";)
	    while( ptr ){
		DEBUG_G2(showtree_ATTLC(ptr,1);)
		ptr=ptr->next;
		DEBUG_G2(cerr << "tail->next:\n";)
	    }
	
//  If any array types were hoisted, append the original 
//  type definition to the end of the chain

	    if( tail ){
		DEBUG_G2(cerr << "append the original typedef to the end\n";)
		tail->next = gbuf->root;
		gbuf->root = head;
	    }

//  Begin debug

	    DEBUG_G2(cerr << "after calling transform, ";)
	    DEBUG_G2(cerr << "list of type trees=\n";)
	    DEBUG_G2(for(t=gbuf->root; t; t=t->next){
		DEBUG_G2(showtree_ATTLC(t,0);)
	    })

//  End debug

//  Link this G2BUF into the mtblhead list
//  Note: this was done previously AFTER checking the
//  types.  Now we must remove the buffer from the mtbl
//  list before deleting it whenever errors are found.

	    DEBUG_G2(cerr << "ready to call addtbl\n";)
	    if(first==0){
		DEBUG_G2(cerr << "first call to addtbl\n";)
		first = addtbl(gbuf);
		DEBUG_G2(cerr << "addtbl returns:\n";)
		DEBUG_G2(showbuf_ATTLC(first->buf);)
	    }else{
		DEBUG_G2(cerr << "subsequent call to addtbl\n";)
		addtbl(gbuf);
	    }
	    DEBUG_G2(cerr << "on return from addtbl, mtblhead-> =\n";)

//  Begin debug

	    DEBUG_G2(for(mp=mtblhead; mp; mp=mp->next){
		DEBUG_G2(cerr << "->\n";)
		DEBUG_G2(showbuf_ATTLC(mp->buf);)
	    })

//  End debug

//  Check the types...

	    DEBUG_G2(cerr << "Check the types\n";)
	    int ok=1;

	    for(t=gbuf->root; t; t=t->next){
		t->name=lower(t->name);

//  ...for typename clashes...

		DEBUG_G2(cerr << "...for typename clashes...\n";)
		DEBUG_G2(cerr 
		    << "ready to call lookup with t->name = " 
		    << t->name 
		    << "\n"
		;)
		if(lookup(t->name)!=t){
		    error("duplicate record: '%s'\n", (const char*)t->name);
		    err++;
		    ok=0;
		}

//  ...and for well-formedness of the definition

		DEBUG_G2(cerr << "and for well-formedness of the definition\n";)
		DEBUG_G2(cerr << "ready to call valid\n";)
		int temp=valid(0,t);
		DEBUG_G2(cerr << "valid returns " << temp << "\n" ;)
		ok &= (temp==0);
	    }
	    DEBUG_G2(cerr << "exit from check-ok loop with ok=" << ok << "\n" ;)
	    if(!ok){
		DEBUG_G2(cerr << "ready to call remtbl\n";)
		remtbl();

//  Begin debug

		DEBUG_G2(cerr << "after remtbl(), mtblhead-> =\n";)
		DEBUG_G2(for(mp=mtblhead; mp; mp=mp->next){
		    DEBUG_G2(cerr << "->\n";)
		    DEBUG_G2(showbuf_ATTLC(mp->buf);)
		})

//  End debug

		delete gbuf;

	    }
	    OK &= ok;
	}
    }
    DEBUG_G2(cerr << "exit from getbuf_ATTLC-loop with OK = " << OK << "\n";)
    if(!OK){

//  Do not generate code if errors were found

	return 0;

    }else{

//  ***********************************************
//  *                                             *
//  *            BEGIN CODE GENERATION            *
//  *                                             *
//  ***********************************************

//  Debug: display the list of G2BUF's

	DEBUG_G2(cerr << "mtbl list=\n";)
	DEBUG_G2(for(mp = mtblhead; mp; mp = mp->next){
	    cerr << "->\n";
	    showbuf_ATTLC(mp->buf);
	})

//  Open hfilename for output, setting file pointer hf

	if((hf=openfile(hfilename))==NULL){
	    return 0;
	}

//  Open cfilename for output, setting file pointer cf

	if((cf=openfile(cfilename))==NULL){
	    return 0;
	}

//  Write header file guard to protect against multiple 
//  inclusion

	fprintf(hf, "#ifndef %sH\n#define %sH\n\n",
	    (const char*)upper(stem),
	    (const char*)upper(stem));

//  Generate the prologue at beginning of .h and .c files

	prologue(hf,cf,stem,hfilename);

//  For each user-defined type in udt_map_ATTLC, generate
//  the appropriate #include directives.  
//
//  Note: include_udt(), sizealign(), wrap(), and
//  nullify() should be bundled into one routine. 

	DEBUG_G2(cerr << "ready to call include_udt\n";)
	include_udt(hf);

//  For each user-defined type T in udt_map_ATTLC, generate 
//  size and alignment constants (these are used in 
//  size and offset computations --  see desc2)

	DEBUG_G2(cerr << "ready to call sizealign\n";)
	sizealign(cf);

//  For each user-defined type T in udt_map_ATTLC, generate
//  x_put_T and x_get_T wrappers for the client-provided
//  I/O routines.

	DEBUG_G2(cerr << "ready to call wrap\n";)
	wrap(cf);

//  For each user-defined type T in udt_map_ATTLC, generate
//  nullify() and isnull() routines that can be called 
//  through a pointer by g2clear().

	DEBUG_G2(cerr << "ready to call nullify\n";)
	nullify(cf);

//  Generate everything else on a type-by-type basis.

	DEBUG_G2(cerr << "Generate everything else on a type-by-type basis\n";)
	for(mp = first; mp; mp = mp->next){
	    gbuf = mp->buf;

	    for(t=gbuf->root; t; t=t->next){
		DEBUG_G2(cerr << "consider tree:\n";)
		DEBUG_G2(showtree_ATTLC(t,0);)
		DEBUG_G2(cerr << "ready to call generate\n";)
		generate(t,hf,cf);

		if(t->next){
		    DEBUG_G2(cerr << "this is a hoisted type\n";)

//  This is a hoisted type; generate a
//  Vblockdeclare/Vblockimplement pair.

//		    blockdef(t,hf,cf);
		}
	    }   
	}

//  Bolt the .c file and the .h file together

	bolt(stem,hf,cf);

//  Closing endif for file guard

	fprintf(hf, "\n#endif\n");

//  Close the files

	fclose(hf);
	fclose(cf);

//  Return success

	return 1;
    }
}

static void 
extract_udt_info(G2NODE* t){
    DEBUG_G2(cerr << "enter extract_udt_info with *t=\n";)
    DEBUG_G2(showtree_ATTLC(t,0);)
    udt_info_ATTLC x;

//  Assume most user-defined types will name five
//  or fewer header files

    x.headers.size(5);

    if(udt_map_ATTLC.element(g2name_ATTLC(t))){

//  This type has been defined earlier!

	error("duplicate user-defined type %s ignored\n",
	 (const char*)g2name_ATTLC(t));
	
    }else{

//  Set defaults (may be overridden by attributes)

	x.headers[0] = g2name_ATTLC(t) + ".h";
	x.null = g2name_ATTLC(t) + "()";
	x.isnull="";
	x.put="operator<<";
	x.get="operator>>";

//  Get attributes

	int header_count=0;
	int put_count=0;
	int get_count=0;
	int null_count=0;
	int isnull_count=0;

	for(G2NODE* c=g2achild_ATTLC(t); c; c=g2anext_ATTLC(c)){
	    DEBUG_G2(cerr << "consider attribute child:\n";)
	    DEBUG_G2(shownode_ATTLC(c);)
	    if(g2name_ATTLC(c) == ".header"){
		DEBUG_G2(cerr << "header\n";)

		if(header_count==x.headers.size()){
		    x.headers.size(2*x.headers.size());
		}
		x.headers[header_count++]=g2val_ATTLC(c);
	    }else if(g2name_ATTLC(c) == ".get"){
		DEBUG_G2(cerr << "get\n";)
		if(++get_count>1){
		    fprintf(stderr,
			"file %s: user-defined type %s: duplicate get attribute\n",
			(const char*)filename,
			(const char*)t->name);
		}else{
		    x.get=g2val_ATTLC(c);
		}
	    }else if(g2name_ATTLC(c) == ".put"){
		DEBUG_G2(cerr << "put\n";)
		if(++put_count>1){
		    fprintf(stderr,
			"file %s: user-defined type %s: duplicate put attribute\n",
			(const char*)filename,
			(const char*)t->name);
		}else{
		    x.put=g2val_ATTLC(c);
		}
	    }else if(g2name_ATTLC(c) == ".null"){
		DEBUG_G2(cerr << "null\n";)
		if(++null_count>1){
		    fprintf(stderr,
			"file %s: user-defined type %s: duplicate null attribute\n",
			(const char*)filename,
			(const char*)t->name);
		}else{
		    x.null=g2val_ATTLC(c);
		}
	    }else if(g2name_ATTLC(c) == ".isnull"){
		DEBUG_G2(cerr << "isnull\n";)
		if(++isnull_count>1){
		    fprintf(stderr,
			"file %s: user-defined type %s: duplicate isnull attribute\n",
			(const char*)filename,
			(const char*)t->name);
		}else{
		    x.isnull=g2val_ATTLC(c);
		}
	    }
	}
	if(header_count==0){
	    header_count=1; // the default;
	}
	x.header_count=header_count;
	DEBUG_G2(cerr << "after for loop, x=\n";)
	DEBUG_G2(show_udt_info(x);)

//  Store x in udt_map_ATTLC

	DEBUG_G2(cerr << "ready to store x in udt_map_ATTLC\n";)
	udt_map_ATTLC[g2name_ATTLC(t)]=x;
	DEBUG_G2(cerr << "ready to return\n";)
    }
}

static void 
show_udt_info(udt_info_ATTLC x){
    int nheaders=x.header_count;
    DEBUG_G2(cerr << "nheaders=" << nheaders << "\n" ;)
    for(int k=0; k < nheaders; k++){
	DEBUG_G2(cerr << x.headers[k] << "\n";)
    }
    DEBUG_G2(cerr << ".put=" << x.put << "\n" << ".get=" << x.get
	<< "\n" << ".null=" << x.null << "\n" << ".isnull=" << x.isnull
	<< "\n" ;)
}

static void 
show_udt_map(){
    Mapiter<String,udt_info_ATTLC> i(udt_map_ATTLC);

    DEBUG_G2(cerr << "udt_map_ATTLC:\n";)
    while(++i){
	DEBUG_G2(cerr << "    " << i.key() << "\t" << "(" ;)
	udt_info_ATTLC x = i.value();
	DEBUG_G2(cerr << ",";)
	DEBUG_G2(show_udt_info(x);)
	DEBUG_G2(cerr << ")";)
    }
}

static void 
sizealign(FILE* cf){

//  Generate size and alignment constants for
//  each user-defined type in udt_map_ATTLC

    Mapiter<String,udt_info_ATTLC> i(udt_map_ATTLC);

    while(++i){
	fprintf(cf, 
	    "static struct x%s{\n    char base[1];\n    %s x;\n}x%s;\n\n",
	    (const char*)i.key(), (const char*)i.key(), (const char*)i.key());
	fprintf(cf,
	    "const int %s_SIZE=sizeof(%s);\n",
	    (const char*)i.key(), (const char*)i.key());
	fprintf(cf,
	    "const char* %s_ALIGN_SIMPLIFY_EXPRESSION=(char*)(&x%s.x);\n",
	    (const char*)i.key(), (const char*)i.key());
	fprintf(cf,
	    "const int %s_ALIGN=%s_ALIGN_SIMPLIFY_EXPRESSION-x%s.base;\n\n",
	    (const char*)i.key(), (const char*)i.key(), (const char*)i.key());
    }
}

static void 
include_udt(FILE* hf){
    Mapiter<String,udt_info_ATTLC> i(udt_map_ATTLC);
    DEBUG_G2(cerr << "enter include_udt\n";)

    while(++i){
	udt_info_ATTLC x=i.value();
	DEBUG_G2(cerr << "in while loop, x=\n";)
	DEBUG_G2(show_udt_info(x);)

	for(int j=0;j<x.header_count;j++){
	    DEBUG_G2(cerr << "in j loop, consider header " << x.headers[j]
		<< "\n" ;)
	    fprintf(hf, "#include \"%s\"\n", (const char*)x.headers[j]);
	}
    }
    DEBUG_G2(cerr << "ready to return from include_udt\n";)
}

static void
wrap(FILE* cf){
    Mapiter<String,udt_info_ATTLC> i(udt_map_ATTLC);
    DEBUG_G2(cerr << "enter wrap\n";)

    while(++i){
	String name=i.key();
	DEBUG_G2(cerr << "in while loop, i.key()=" << name << "\n" ;)
	udt_info_ATTLC x=i.value();
	DEBUG_G2(cerr << "...and i.value()=\n";)
	DEBUG_G2(show_udt_info(x);)

	if(x.put == "operator<<"){
	    fprintf(cf,
		"static ostream& x_put_%s(ostream& os,const void* vp){\n",
		(const char*)name);
	    fprintf(cf, "    return os << *(%s*)vp;\n}\n",
		(const char*)name);
	}else{
	    fprintf(cf,
		"static ostream& x_put_%s(ostream& os,const void* vp){\n",
		(const char*)name);
	    fprintf(cf, "    return %s(os,*(%s*)vp);\n}\n",
		(const char*)x.put,
		(const char*)name);
	}
	if(x.get == "operator>>"){
	    fprintf(cf,
		"static istream& x_get_%s(istream& is,void* vp){\n",
		(const char*)name);
	    fprintf(cf, "    return is >> *(%s*)vp;\n}\n",
		(const char*)name);
	}else{
	    fprintf(cf,
		"static istream& x_get_%s(istream& is,void* vp){\n",
		(const char*)name);
	    fprintf(cf, "    return %s(is,*(%s*)vp);\n}\n",
		(const char*)x.get,
		(const char*)name);
	}
    }

}

static void
nullify(FILE* cf){
    Mapiter<String,udt_info_ATTLC> i(udt_map_ATTLC);
    DEBUG_G2(cerr << "enter nullify\n";)

    while(++i){
	String name=i.key();
	udt_info_ATTLC x=i.value();
	fprintf(cf, "static %s null_%s=%s;\n",
	    (const char*)name,
	    (const char*)name,
	    (const char*)x.null);
	fprintf(cf,
	    "static void %s_nullify(void* vp){\n    *(%s*)vp=null_%s;\n}\n",
	    (const char*)name,
	    (const char*)name,
	    (const char*)name);
	String result;

	if( x.isnull.is_empty() ){
	    result = "*(" + name + "*)vp==null_" + name;
	}else{
	    result = x.isnull + "(*(" + name + "*)vp)";
	}
	fprintf(cf,
	    "static int %s_is_null(void* vp){\n    return %s;\n}\n",
	    (const char*)name,
	    (const char*)result);
    }
}

static void 
bdef(G2NODE* t, FILE* hf, FILE* cf){ 
    DEBUG_G2(cerr << "enter bdef with t=\n";)
    DEBUG_G2(showtree_ATTLC(t,1);)

//  Visit t's children

    for(G2NODE* cp=t->child; cp; cp=cp->next){ 
	DEBUG_G2(cerr << "ready to visit t's next child\n";)
	bdef(cp,hf,cf); 
    } 

//  Then visit t

    DEBUG_G2(cerr << "exit from children loop; now visit t itself\n";)
    DEBUG_G2(showtree_ATTLC(t,1);)
    if(isdigit_ATTLC(t->name.char_at(0))){ 

//  t is an array (note: all arrays have
//  been transformed into terminal arrays)

	DEBUG_G2( cerr << "t->name is a digit: t is an array\n"; )
	String typename;

	if(isdigit_ATTLC(t->val.char_at(0))){
	    typename = "STRING";
	}else{
	    typename = upper(t->val);
	}
	DEBUG_G2(cerr << "t's typename is " << typename << "\n" ;)

//  Invoke vblockdeclare macro

//	DEBUG_G2(cerr << "put to .h file:\n    ***Vblockdeclare(" 
//	    << typename << ");\n" ;)
//	fprintf(hf, "Vblockdeclare(%s);\n", (const char*)typename);
//	fflush(hf);  // Debug

//  Invoke blockimplement macro

//	DEBUG_G2(cerr << "put to .c file:\n    ***Vblockimplement("
//	    << typename << ");\n" ;)
//	fprintf(cf, "Vblockimplement(%s);\n", (const char*)typename);
//	fflush(cf);  // Debug
    }
}

static void 
blockdef(G2NODE* t, FILE* hf, FILE* cf){
    DEBUG_G2(cerr << "put to .h file:\n    ***Vblockdeclare(" 
	<< upper(t->name) << ");\n" ;)
    fprintf(hf, "Vblockdeclare(%s);\n", (const char*)upper(t->name));
    fflush(hf);  // Debug

    DEBUG_G2(cerr << "put to .c file:\n    ***Vblockimplement("
	<< upper(t->name) << ");\n" ;)
    fprintf(cf, "Vblockimplement(%s);\n", (const char*)upper(t->name));
    fflush(cf);  // Debug
}

static void 
generate(G2NODE* t, FILE* hf, FILE* cf){

//  Generate typedefs in .h file
//  (.c file is needed for constructor definitions, which
//  used to be inline in the .h file, but which gave "outline
//  inline" warning messages from CC +w 

    DEBUG_G2(cerr << "ready to call tdef\n";)
    bdef(t,hf,cf);
    tdef(t,hf,cf);

//  Generate descriptor tables in .c file

    DEBUG_G2(cerr << "ready to call desc\n";)
    desc(t,t->name,cf);

//  Generate stream insertion and extraction operators

    DEBUG_G2(cerr << "ready to call gen_io\n";)
    gen_io(t,hf,cf);
}

static void 
gen_io(G2NODE* t, FILE* hf, FILE* cf){

//  Generate stream insertion and extraction
//  function declarations in .h file

    fprintf(hf, "\nostream& operator<<(ostream& os, const %s& buf);\n",
	(const char*)(upper(t->name)));
    fprintf(hf, "\nistream& operator>>(istream& is, %s& buf);\n",
	(const char*)(upper(t->name)));
    fflush(hf);  // Debug

//  Generate function definitions in .c file

    fprintf(cf, "\nostream& operator<<(ostream& os, const %s& buf){\n",
        (const char*)(upper(t->name)));
    fprintf(cf,
        "    if(!os)return os;\n    putrec_ATTLC((void*)&buf,%s,os);\n",
        (const char*)t->name);
    fprintf(cf, "    return os;\n}\n");

    fprintf(cf, "\nistream& operator>>(istream& is, %s& buf){\n",
	(const char*)(upper(t->name)));
    fprintf(cf, "    if(!is)return is;\n    getrec_ATTLC(&buf,%s,is);\n",
	(const char*)t->name);
    fprintf(cf, "    return is;\n}\n");
    fflush(cf);  // Debug
}

static void 
transform(G2NODE* p){
    DEBUG_G2(cerr << "enter transform with *p=\n";)
    DEBUG_G2(showtree_ATTLC(p,0);)
    G2NODE* c=p->child;

    if(c){

//  Generate a type tag
//
//  Note: we just ignore the case where p->val is nonempty
//
//      assert(p->val=="");

	String rand=ranstr(G2TAGLEN_ATTLC);
	p->val=rand;
	DEBUG_G2(cerr << "after generating tag, *p=\n";)
	DEBUG_G2(showtree_ATTLC(p,0);)

	char temp = c->name.char_at(0);
  	int isarray=(
	    temp=='*' ||                // flexible
	    isdigit_ATTLC(temp)         // fixed
	);
	if(isarray){
	    DEBUG_G2(cerr << "p is an array\n";)
	}else{
	    DEBUG_G2(cerr << "p is a structure\n";)
	}

//  Visit p's children

	DEBUG_G2(cerr << "ready to visit p's children\n";)
	for(G2NODE* cp=c; cp; cp=cp->next){
	    DEBUG_G2(cerr << "visit next child\n";)
	    transform(cp);
	}
	DEBUG_G2(cerr << "after transforming, p=\n";)
	DEBUG_G2(showtree_ATTLC(p,0);)

	if(isarray){

//  Change arrays dimensioned using "*" to "0"
//
//  Note: only modify the first character, preserving
//  any initial-reserve specification that may follow the 
//  asterisk, e.g.: 
//
//      usr    
//              *(100)    LONG  # initially reserve 100

	    if(c->name.char_at(0) == '*'){
		c->name[0]='0';
	    }

//  Hoist a type definition for non-terminal arrays

	    if(c->child!=0){
		DEBUG_G2(cerr << "non-terminal array\n";)
		DEBUG_G2(cerr << "after hoisting,\n";)

//  Set up a new G2NODE for the hoisted type

		if(gbuf->ptr >= gbuf->end){
		    error("out of memory\n");
		    exit(1);
		}
		G2NODE* n = gbuf->ptr++;
		n->name = c->val; 
		n->val = "HOISTED";          // jfi
		n->next = 0;
		n->child = c->child;

//  Detach the subtree rooted at p's child 

		c->child = 0;
		DEBUG_G2(cerr << "p=\n";)
		DEBUG_G2(showtree_ATTLC(p,0);)
		DEBUG_G2(cerr << "n=\n";)
		DEBUG_G2(showtree_ATTLC(n,0);)

//  Link the subtree into the list of generated type trees

		if(tail){
		    tail->next = n;
		    tail = n;
		}else{
		    head = tail = n;
		}
	    }
	}

//  Fix up Strings declared with "*"

    }else if(p->val.char_at(0)=='*'){
	DEBUG_G2(cerr << "p->val.char_at(0)=='*' means p is a flexible string\n";)

//  Again, we only modify the first character, preserving
//  any initial-reserve specification that may follow the 
//  asterisk, e.g.: 
//
//      usr    *(100)    # initially reserve 100   

	p->val[0] = '0';
	DEBUG_G2(showtree_ATTLC(p,0);)

//  Fix up arrays declared with "*"

    }else if(p->name.char_at(0)=='*'){
	DEBUG_G2(cerr << "p is a flexible array\n";)

//  Again, we only modify the first character, preserving
//  any initial-reserve specification that may follow the 
//  asterisk, e.g.: 
//
//      usr    
//              *(100)    LONG  # initially reserve 100

	p->name[0] = '0';
	DEBUG_G2(showtree_ATTLC(p,0);)
    }
}

static void 
bolt(const String& stem, FILE* hf, FILE* cf){

//  Bolt .h and .c files together with strange symbol 
    
    String fastener = "g2" + stem + ranstr(4) + "_ATTLC";
    fprintf(hf, "extern int %s;\n", (const char*)fastener);
    fflush(hf);  // Debug
    fprintf(hf, "static int *_%s = &%s;\n\n", 
	(const char*)fastener, 
	(const char*)fastener);
    fflush(hf);  // Debug
    fprintf(cf, "int %s;\n", (const char*)fastener);
    fflush(hf);  // Debug

//  The following trick eliminates the warning about the
//  unused static int* given by cfront (the inline should 
//  never be "laid down"

    fprintf(hf, "inline void __%s(){ (*_%s)++; }\n",
	(const char*)fastener, 
	(const char*)fastener);
    fflush(hf);  // Debug
}

static void 
resettbl(){
    DEBUG_G2(cerr << "enter resettbl\n";)
    nulldef.name="";
    nulldef.val="";
    nulldef.next = nulldef.child = 0;

//  set up definition for char type
//
//    char_def:
//
//        tattoo   G2MOTHER_ATTLC
//        root           o--------------> chardef
//        base           o--------------> chardef
//        ptr            o--------------> nulldef
//        end            o-----------|
//        buf      chardef           |
//                 nulldef           |
//                 xxxxxxx <----------
//                 xxxxxxx
//
//    chardef:
//   
//        name    "CHAR"
//        val     "-100"
//        next    o----->
//        child   o----->
//

    DEBUG_G2(cerr << "ready to initialize chardef\n";)
    chardef.name="CHAR";
    chardef.val=CHAR_ASC_ATTLC;
    chardef.next = chardef.child = 0;
    DEBUG_G2(shownode_ATTLC(&chardef);)

    DEBUG_G2(cerr << "ready to initialize char_def\n";)
    char_def.tattoo=G2MOTHER_ATTLC;
    char_def.buf.size(2);
    char_def.buf[0]=chardef;
    char_def.buf[1]=nulldef;
    char_def.base=char_def.root=char_def.buf;
    char_def.ptr=char_def.buf+1;
    char_def.end=char_def.buf+2;
    DEBUG_G2(showbuf_ATTLC(&char_def);)

//  do same for short...

    DEBUG_G2(cerr << "ready to initialize shortdef\n";)
    shortdef.name="SHORT";
    shortdef.val=SHORT_ASC_ATTLC;
    shortdef.next = shortdef.child = 0;
    DEBUG_G2(shownode_ATTLC(&shortdef);)

    DEBUG_G2(cerr << "ready to initialize short_def\n";)
    short_def.buf.size(2);
    short_def.buf[0]=shortdef;
    short_def.buf[1]=nulldef;
    short_def.base=short_def.root=short_def.buf;
    short_def.ptr=short_def.buf+1;
    short_def.end=short_def.buf+2;
    DEBUG_G2(showbuf_ATTLC(&short_def);)

//  ...and long

    DEBUG_G2(cerr << "ready to initialize longdef\n";)
    longdef.name="LONG";
    longdef.val=LONG_ASC_ATTLC;
    longdef.next = longdef.child = 0;
    DEBUG_G2(shownode_ATTLC(&longdef);)

    DEBUG_G2(cerr << "ready to initialize long_def\n";)
    long_def.buf.size(2);
    long_def.buf[0]=longdef;
    long_def.buf[1]=nulldef;
    long_def.base=long_def.root=long_def.buf;
    long_def.ptr=long_def.buf+1;
    long_def.end=long_def.buf+2;
    DEBUG_G2(showbuf_ATTLC(&long_def);)

//  ...and string

    DEBUG_G2(cerr << "ready to initialize stringdef\n";)
    stringdef.name="STRING";
    stringdef.val=STRING_ASC_ATTLC;
    stringdef.next = stringdef.child = 0;
    DEBUG_G2(shownode_ATTLC(&stringdef);)

    DEBUG_G2(cerr << "ready to initialize string_def\n";)
    string_def.buf.size(2);
    string_def.buf[0]=stringdef;
    string_def.buf[1]=nulldef;
    string_def.base=string_def.root=string_def.buf;
    string_def.ptr=string_def.buf+1;
    string_def.end=string_def.buf+2;
    DEBUG_G2(showbuf_ATTLC(&string_def);)

//  reinitialize mtblhead-> list to empty

    DEBUG_G2(cerr << "ready to initialize mtblhead-> list to empty\n";)
    mtbl* mp = mtblhead;
    DEBUG_G2(cerr << "in resettbl, mp = " << long(mp) << "\n";)

    while( mp ){
	mtbl* t = mp;
	mp = mp->next;
	delete t;
    }
    mtblhead = NULL;

//  Pre-populate list with definitions for string, long, short, 
//  and char types 
//
//                                                     mtbltail
//                                                        |
//                                                        V
//           mtblhead------>o  o-------->o  o------------>o  o
//                          |            |                |
//                          |            |                |
//                          V            V                V
//                       long_def    short_def         char_def
//
    addtbl(&string_def);
    addtbl(&long_def);
    addtbl(&short_def);
    addtbl(&char_def);
}

mtbl*
addtbl(G2BUF* buf){

    // mtbl* mp = (mtbl*)calloc(1,sizeof(mtbl));
    mtbl* mp = new mtbl;
    mp->buf = 0;
    mp->next=0;

    if( !mp ){
	error("out of memory\n");
	exit(1);
    }
    mp->buf = buf;
    
    if( !mtblhead ){  // first time 
	mtblhead = mtbltail = mp;
    }else{
	mtbltail->next = mp;
	mtbltail = mp;
    }
    return mp;
}

static void 
remtbl(){
    mtbl* mp=mtblhead;
    mtbl* temp;
    mtbltail=0;

//  Find the end of the list

    while(mp && (temp=mp->next)){
	mtbltail=mp;
	mp=temp;
    }

//  Remove the last element and fix up pointers

    if( mtbltail ){
	mtbltail->next=0;
    }else{
	mtblhead=0;
    }
}

static void 
prtbls(const String& stem, FILE* hf, FILE* cf){
    int	i;
    String ustem=upper(stem);
    mtbl* mp;
    
    fprintf(hf, "extern G2DESC	*%s[];\n", (const char*)stem);
    fflush(hf);  // Debug
    fprintf(hf, "typedef union %s {\n", (const char*)ustem);
    fflush(hf);  // Debug
    for( mp = mtblhead; mp; mp = mp->next ){
	String name = mp->buf->root->name;
	String uname=upper(name);

	if( !isupper(name.char_at(0)) ){
	    fprintf(hf, "	%s	%s;\n", 
		(const char*)uname, 
		(const char*)name);
	    fflush(hf);  // Debug
	}
    }
    fprintf(hf, "} %s;\n\n", (const char*)ustem);
    fflush(hf);  // Debug
    i = 0;

    for( mp = mtblhead; mp; mp = mp->next ){
	String name = mp->buf->root->name;

	if( !isupper(name.char_at(0)) ){
	    String uname=upper(name);
	    fprintf(hf, "#define %s_%s	%d\n",
		(const char*)ustem,
		(const char*)uname,
		i++);
	    fflush(hf);  // Debug
	}
    }
    fprintf(cf, "G2DESC	*%s[] = {\n", (const char*)stem);
    fflush(cf);  // Debug
    for( mp = mtblhead; mp; mp = mp->next ){
	String name = mp->buf->root->name;

	if( !isupper(name.char_at(0)) ){
	    fprintf(cf, "	%s,\n", (const char*)name);
	    fflush(cf);  // Debug
	}
    }
    fprintf(cf, "	0\n");
    fflush(cf);  // Debug
    fprintf(cf, "};\n");
    fflush(cf);  // Debug
    fprintf(cf, "int	%s_nrec_ = %d;\n\n", (const char*)stem, i);
    fflush(cf);  // Debug
}

static String 
lower(const String& s){
    String result(Stringsize(s.length()));

    for( int i=0;i<s.length();i++ ){
	
	if( isupper(s.char_at(i)) ){
	    result += tolower(s.char_at(i));
	}else{
	    result += s.char_at(i);
	}
    }
    return result;
}

static String 
ranstr(int len){
    String result((Stringsize)len);
    String range = "abcdefghijklmnopqrstuvwxyz"; // used to include caps and digits

    int j = range.length();
    while( --len >= 0 ){
	int i=rand();
	int k = i % j;
	result+=range.char_at(k);
    }
    DEBUG_G2(cerr << "ranstr returns result = " << result << "\n";)
    return result;
}

