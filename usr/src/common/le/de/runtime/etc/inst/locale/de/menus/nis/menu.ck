.ul
Installation und Konfiguration des Pakets Network Information Service (NIS)
.lr
F1=Hilfe
.top
`echo  "\n \nSie haben folgende NIS-Konfiguration gewählt:"` 
`echo  "\n \n  $host ist ein NIS $host_type"`  
`echo  "\n \n  NIS-Domäne: $def_dom"`
`[ "$slavep" = "F" ] && echo  "\n  NIS-Server für Domain $def_dom: $SERV1"`
`[ "$slavep" = "F" ] && echo  "                                       $SERV2"`
`[ "$slavep" = "F" ] && echo  "                                       $SERV3"`
`[ "$slavep" = "T" ] && echo  "\n  NIS-Hauptserver für\nDomain $def_dom: $master"`
`echo   "\n \nUm die NIS-Konfiguration zu wiederholen oder abzubrechen,"`
 `echo   " geben Sie \"Nein\" ein."`
.button
Anwenden
Zurücksetzen
.bottom
Drücken Sie 'TAB', um den Cursor zwischen den Feldern zu bewegen. Nach
Beendigung der Eingaben bewegen Sie den Cursor auf "Anwenden" und 
drücken anschließend 'ENTER', um fortzufahren.
.form
2 2//No::Nein//Yes::Ja//NIS-Konfiguration akzeptieren?://ACCEPT//
//Wählen Sie Ja mit Hilfe der Tasten <Pfeil-nach-links/rechts>, 
um die Konfiguration zu akzeptieren.//
.help
Wenn Sie diese NIS-Konfiguration nicht akzeptieren möchten, geben Sie 
"Nein" ein. Damit erhalten Sie die Möglichkeit, im nächsten Menü NIS 
erneut zu konfigurieren oder die Konfiguration abzubrechen. Auch wenn 
Sie die Konfiguration abbrechen, wird NIS auf jeden Fall installiert.

Hinweis: Dieses Menü zeigt nur die ersten drei für diese Domain 
         konfigurierten NIS-Server an.
.helpinst
Entf=Abbrechen  F1=Hilfe  ESC=Hilfe verlassen  1=Vor  2=Zurück
