<!-- /lfs/doc/DMG/Projects/sgml/sccs/styles/modules/s.onlinfig.mod 1.5 94/08/10 -->
<style name="FIGURE,TITLE">
   <text-before>&fignum;</>
</style>
<style name="FIGURE,TITLE,#text-before">
    <break-after>	none </>
</style>

<group name="figs">
   <font-family>        &codfam; </>
   <break-before>	line </>
   <space-before>	&lead3; </>
   <space-after>	&lead3; </>
   <left-indent>	+=&in2; </>
   <right-indent>       -600 </>
   <justification>      verbatim </>
   <icon-position>	inline </>
   <icon-type>		raster </>
   <foreground>		&figclr; </>
   <script>		ebt-reveal stylesheet="figtext.w"
			root="1" title="&figtitl;"</>
   <hide>		children </>
</group>

<style name="FIGURE,LITERALLAYOUT" group="figs"></style>
<style name="FIGURE,PROGRAMLISTING" group="figs"></style>
<style name="FIGURE,SCREEN" group="figs"></style>

<style name="FIGURE,GRAPHIC" group="figs">
   <script>	ebt-raster filename="attr(FILEREF)"
		title="&figtitl;"</>
</style>
<style name="FIGURE,SCREENSHOT,GRAPHIC" group="figs">
   <script>	ebt-raster filename="attr(FILEREF)"
		title="&figtitl;"</>
</style>
