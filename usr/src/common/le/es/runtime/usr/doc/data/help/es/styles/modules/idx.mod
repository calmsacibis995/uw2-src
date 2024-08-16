<!-- /lfs/doc/DMG/Projects/sgml/sccs/styles/modules/s.idx.mod 1.1 94/06/19 -->
<!-- Hypertext elements that appear only when the INDEXED view
    is selected -->

<!-- External link to an index entry from a source book -->
<style name="ITERMREF">
   <text-before>&idx1sym;</>
   <script>	ebt-link book="attr(book)" collection="attr(collection)"
		window=new view=fulltext showtoc=true root=1
		target="idmatch(ID, attr(TIDREF))" </>
</style>

<style name="ITERMREF,#text-before">
   <foreground>         &idxclr; </>
   <font-family>        &idx1fam; </>
   <font-size>          &idx1siz; </>
   <vertical-offset>    &idx1off; </>
</style>

<style name="BEGINPAGE">
   <text-before>[&pgwd; attr(PAGENUM)]</>
</style>

<style name="BEGINPAGE,#text-before" group="bodytype">
   <foreground>		&bpclr; </>
   <left-indent>	&ctin; </>
   <break-before>	line </>
   <break-after>	line </>
</style>
