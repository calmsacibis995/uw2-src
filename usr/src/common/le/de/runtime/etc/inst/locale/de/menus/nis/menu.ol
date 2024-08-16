.ul
Installation und Konfiguration des Pakets Network Information Service(NIS)
.lr
F1=Hilfe
.top
`echo  "Folgende NIS-Konfiguration ist auf diesem Computer bereits vorhanden:"` 
`echo "\n \n \n"`
`[ "$TYPE" = "1" ] && echo "    Dieser Computer ist ein NIS-Hauptserver.\n \n"`
`[ "$TYPE" = "2" ] && echo "    Dieser Computer ist ein NIS-Nebenserver.\n \n"`
`[ "$TYPE" = "3" ] && echo "    Dieser Computer ist ein NIS-Client.\n \n"`
`echo "    NIS-Dom�ne: $def_dom"`  
`echo "\n \n    NIS-Server f�r Domain $def_dom: \c" && cat /tmp/nis.overlay | xargs echo`
.button
Anwenden
Zur�cksetzen
.bottom
Dr�cken Sie 'TAB', um den Cursor zwischen den Feldern zu bewegen. 
Nach Beendigung der Eingaben bewegen Sie den Cursor auf "Anwenden" 
und dr�cken anschlie�end 'ENTER', um fortzufahren.
.form
2 2//Yes::Ja//No::Nein//Aktuelle NIS-Konfiguration verwenden?://USE_CURRENT//
//Pfeiltaste-links/rechts, um die aktuelle NIS-Konfigur. zu verwenden.//
.help
Dieser Computer wurde bereits f�r NIS konfiguriert. Die Installation 
kann unter Verwendung der aktuellen NIS-Konfiguration fortgesetzt
werden (Ja eingeben). Sie k�nnen mit dem NIS-Installationsskript aber 
auch eine neue NIS-Konfiguration erstellen (Nein eingeben).

Hinweis: Dieses Men� zeigt nur die ersten drei f�r diese Domain 
         konfigurierten NIS-Server an.
.helpinst
Entf=Abbrechen  F1=Hilfe  ESC=Hilfe verlassen  1=Vor  2=Zur�ck
