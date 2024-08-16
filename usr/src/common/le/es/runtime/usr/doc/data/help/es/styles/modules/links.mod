<!-- /lfs/doc/DMG/Projects/sgml/sccs/styles/modules/s.links.mod 1.8 94/10/16 -->
<!-- ******************* NOVELL HYPERTEXT ELEMENTS ****************** -->

<group name="xlink">
   <foreground>         &xclr; </>
   <line>               &xline; </>
</group>

<!-- STANDARD ID-BASED LINK AND ITS TARGET -->

<style name="LINK" group="xlink">
   <script>	if(eq(env(DHELPSTYLE),dhelp),
		if(eq("bookname()","attr(book)"),
			ebt-link window="current" view="fulltext"
 				root="idmatch(ID, attr(LINKEND))"
				showtoc=false title="content(me())",
	 		ebt-link window=specified book="attr(book)"
				collection="refman" title="content(me())" 
				root="idmatch(ID, attr(LINKEND))"
				view="fulltext" showtoc=false),
		if(eq("bookname()","attr(book)"),
			ebt-link window="specified" view="fulltext" root=1
				title="content(me())"
 				target="idmatch(ID, attr(LINKEND))",
	 		ebt-link window=specified book="attr(book)"
				collection="refman" title="content(me())" 
				target="idmatch(ID, attr(LINKEND))"
				view="fulltext" showtoc=true))
				</>
</style>

<style name="XMARK">
   <hide>		all </>
</style>

<!-- All other links below are local (nonstandard) extensions to
the DocBook DTD -->

<!-- SIMPLE ID-BASED INTERNAL LINK (NONSTANDARD) -->

<style name="IXREF" group="xlink">
   <script>             ebt-link window=specified view=fulltext root=1
			showtoc=true target="idmatch(ID, attr(IDREF))" </>
</style>

<!-- RESTRICTED ID-BASED INTERNAL LINK -->

<style name="IXREFR" group="xlink">
   <script>     ebt-link window=specified view=fulltext 
		root="idmatch(ID, attr(IDREF))"
                title="content(me())" showtoc=false </>
</style>

<!-- LINK THAT OPENS AN ENTIRE BOOK -->

<style name="BXREF" group="xlink">
   <font-slant>         &bodslan; </>
   <script>	if(eq(env(DHELPSTYLE),dhelp),
		ebt-link window=specified view=fulltext showtoc=false
		title="content(me())"
		root=1 book="attr(book)" collection="attr(collection).es",
   		ebt-link window=specified view=fulltext showtoc=true
		title="content(me())"
		root=1 book="attr(book)" collection="attr(collection).es") </>
</style>

<!-- LINKS BASED ON A STRING MATCH WITH THE CONTENT OF A TITLE ELEMENT -->
<!-- NOTE: The destination of the link will be the immediate parent
	   of the TITLE element whose content exactly matches the content
	   of the XREF element -->

<!-- Internal link to a container whose TITLE matches link content -->
<style name="TIXREF" group="xlink">
   <script>	if(eq(env(DHELPSTYLE),dhelp),
		ebt-link window=current view=fulltext showtoc=false
		title="content(me())"
		root=queryexact(TITLE,"content(me())",1),
		ebt-link window=specified view=fulltext root=1
		target=queryexact(TITLE,"content(me())",1)) </>
</style>

<!-- Same as TIXREF, but view is restricted to target container -->
<style name="TIXREFR" group="xlink">
   <script>	ebt-link window=specified view=fulltext showtoc=false
		root=queryexact(TITLE,"content(me())",1)
                title="content(me())" </>
</style>

<!-- External link to a container whose TITLE matches link content -->
<style name="TEXREF" group="xlink">
   <script>	if(eq(env(DHELPSTYLE),dhelp),
		ebt-link book="attr(book)" collection="attr(collection).es"
		window=specified view=fulltext showtoc=false
		title="content(me())"
		root=queryexact(TITLE,"content(me())",1),
		ebt-link book="attr(book)" collection="attr(collection).es"
		window=specified view=fulltext showtoc=true root=1
		title="content(me())"
		target=queryexact(TITLE,"content(me())",1)) </>
</style>

<!-- Same as TEXREF, but view is restricted to target container -->
<style name="TEXREFR" group="xlink">
   <script>	ebt-link book="attr(book)" collection="attr(collection)"
		window=specified view=fulltext showtoc=false
		root=queryexact(TITLE,"content(me())",1)
                title="content(me())" </>
</style>

<!-- LINKS BASED ON A STRING MATCH WITH CONTENT OF A REFENTRYTITLE ELEMENT -->
<!-- NOTE: The destination of the link will be the *grandparent*
	   of the REFENTRYTITLE element whose content
	   exactly matches the content of the XREF element.
           The grandparent is appropriate because the immediate parent
	   is the REFMETA element. -->

<!-- Internal link to a REFENTRY container
     whose REFENTRYTITLE matches link content -->
<style name="RIXREF" group="xlink">
   <script>	ebt-link window=specified view=fulltext root=1 showtoc=true
		target=queryexact(REFENTRYTITLE,"content(me())",2) </>
</style>

<!-- Same as RIXREF, but view is restricted to target container -->
<style name="RIXREFR" group="xlink">
   <script>	ebt-link window=specified view=fulltext showtoc=false
		root=queryexact(REFENTRYTITLE,"content(me())",2)
                title="content(me())" </>
</style>

<!-- External link to a REFENTRY container whose 
     REFENTRYTITLE matches link content -->
<style name="REXREF" group="xlink">
   <script>	ebt-link book="attr(book)" collection="attr(collection)"
		window=specified view=fulltext root=1 showtoc=true
		target=queryexact(REFENTRYTITLE,"content(me())",2) </>
</style>

<!-- Same as REXREF, but view is restricted to target container -->
<style name="REXREFR" group="xlink">
   <script>	ebt-link book="attr(book)" collection="attr(collection)"
		window=specified view=fulltext showtoc=false
		root=queryexact(REFENTRYTITLE,"content(me())",2)
                title="content(me())" </>
</style>

<!-- LINKS USED IN A GENERATED HYPERINDEX -->

<!-- Raw index marker (disappears when master index is made) -->
<style name="INDEXTERM">
   <hide>               all </>
</style>

<!-- External link to an index marker from an index -->
<style name="IDXREF">
   <text-before>&idx2sym;</>
   <script>     ebt-link book="attr(book)" collection="attr(collection)"
		window=specified view=fulltext showtoc=true root=1
		target="idmatch(IDXID, attr(IDXMKID))" </>
</style>

<style name="IDXREF,#text-before">
   <foreground>         &idxclr; </>
   <font-family>        &idx2fam; </>
   <font-size>          &idx2siz; </>
   <vertical-offset>    &idx2off; </>
</style>

<!-- internal index cross-reference (for ISEE and ISEEALSO) -->
<style name="ISEEREF">
   <select>		iseeref_hasnotarget_isnull(attr(IDREF, me())) </>
</style>

<style name="iseeref_hasnotarget_False" group="xlink">
   <script>             ebt-link target="idmatch(ID, attr(IDREF))" </>
</style>

<style name="iseeref_hasnotarget_True"></style>
