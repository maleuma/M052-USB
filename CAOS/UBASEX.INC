;
;	USB-Treiber fuer BASIC - analog BASEX vom D004
;	(nur fuer CAOS vor Version 4.7 benoetigt)
;
; Speicherbelegung:
;
;	000h..00Ah	Dateiname+Typ
;	0D8h..14Fh	Zusatzroutinen wie DIENST.KCC
;	150h..160h	Umschaltung Tape / USB-Disk
;	200h..2FFh	BASEX und modifizierte SUTAB
;
; UBASEX.INC	(c) M. Leubner, 24.11.2008-05.03.2021
;----------------------------------------------------

	DW	7F7Fh
	db	'basic',1	; fuer alle CAOS-Versionen geeignet
	LD	HL,0C00DH	; BASIC-Kaltstart
	JR	UBASIC
 
	DW	7F7Fh
	db	'rebasic',1	; fuer alle CAOS-Versionen geeignet
	LD	HL,0C08CH	; BASIC-Warmstart
 
; BASEX-Programmteile umladen, BASIC starten:

UBASIC:	PUSH	HL		; BASIC-Start-Adresse merken
	call	minit		; Modul initialisieren
	JR	C,NOTUSB	; nicht gefunden, Start ohne USB-Treiber!
	SET	0,(IX+7)	; CAOS-Modus (LOAD)

	LD	DE,00D8H-D8+SERV
	LD	HL,SERV
	LD	BC,SERVEND-SERV
	LDIR			; SERVICE-Routinen

	LD	DE,150H
	LD	HL,UMS
	LD	BC,UMSE-UMS
	LDIR			; Umschaltroutine

	LD	DE,200H
	LD	HL,BASEX
	LD	BC,BASEND-BASEX
	LDIR			; BASEX-Treiber selbst

	LD	HL,(SUTAB)
	LD	(SUSP),HL	; Original SUTAB hier merken
	LD	DE,SUBAS
	LD	(SUTAB),DE	; BASEX-SUTAB aktivieren
	LD	BC,94H
	LDIR			; SUTAB kopieren
	LD	HL,MBINC
	LD	(SUBAS+6EH),HL	; MBIN ersetzen
	LD	HL,MBOUTC
	LD	(SUBAS+70H),HL	; MBOUT ersetzen

NOTUSB:	LD	HL,BASUM
	LD	DE,0
	LD	BC,BASUM2-BASUM
	LDIR		; Aktivierroutine ab 0 kopieren
	LD	D,0C1H
	LD	A,2
	LD	L,A
	JP	0	; und dort hinspringen
	;
BASUM:	CALL	PV1
	DB	MODU		; BASIC-Ebene einblenden
	IN	A,(88H)
	RES	2,A
	OUT	(88H),A		; IRM aus
	RET			; -> Sprung zu BASIC
BASUM2:
;-------------------------------------------------------------------------------
; BASIC Service-Schnittstelle		-> Adresse 00BEh..00FFh
;-------------------------------------------------------------------------------
SERV:	LD	C,88H
	IN	B,(C)
	RES	7,B		; BASIC aus
	OUT	(C),B
	CALL	IRMON
	CALL	BSERV		; Service-Verteiler im ROM aufrufen
	CALL	IRMOF
	LD	C,88H
	IN	B,(C)
	SET	7,B		; BASIC ein
	OUT	(C),B
	RET
	;
D8:	LD	A,0	; CALL*D8	FLOAD	Name ab Adr. 0
	DB	1
	LD	A,1	; CALL*DB	FSAVE	Name ab Adr. 0
	DB	1
	LD	A,2	; CALL*DE	DIR	Verzeichnis anzeigen
	DB	1
	LD	A,3	; CALL*E1	STAT	Anzeige was frei ist
	DB	1
	LD	A,4	; CALL*E4	REN	Namen eingeben
	DB	1
	LD	A,5	; CALL*E7	SETRO	Name eingeben
	DB	1
	LD	A,6	; CALL*EA	SETWR	Name eingeben
	DB	1
	LD	A,7	; CALL*ED	ERA	Name eingeben
;neu (ab DEP 2.0):
	DB	1
	LD	A,8	; CALL*F0	DRIVE	LW eingeben
	DB	1
	LD	A,9	; CALL*F3	USER	User eingeben
	DB	1
	LD	A,10	; CALL*F6	TYPTX	Name ab Adr. 0
	DB	1
	LD	A,11	; CALL*F9	TYPTP	Name ab Adr. 0
	DB	1
	LD	A,12	; CALL*FC	DUMP	Name ab Adr. 0
	JR	SERV	; Sprung zurueck zur Adresse 00BEh
			; damit der Adressbereich ab 0100h frei bleibt
;
SERVEND:		; Ende der Service-Routinen im RAM-0

;-------------------------------------------------------------------------------
; Umschaltung Tape/USB-Disk		-> Adresse 0150h..0160h
;-------------------------------------------------------------------------------

UMS:	CALL	IRMON		; IRM on
	LD	HL,(SUSP)
	LD	DE,(SUTAB)
	LD	(SUTAB),HL
	LD	(SUSP),DE
	CALL	IRMOF		; IRM off
	RET
UMSE:	;Ende der Umschaltroutine

;-------------------------------------------------------------------------------
; BASEX-Programmteil			-> Adresse 0200h..02FFh
;-------------------------------------------------------------------------------
 
BASEX	EQU	$		; Adresse im ROMC
RUN	EQU	$-BASEX+200H
	DB	0		; 1 ->RUN-Switch (Dateiende 1Dh statt 03h)
SUSP	EQU	$-BASEX+200H
	DW	0		; Merkzelle fuer Original-SUTAB
;
; Byteweise MB-Eingabe (CAOS-UP Nr. 37H)
;
; PE:	D	Steuerbyte	Bit3=1 -> INIT (1. Block)
;				Bit6=1 -> CLOSE (letzter Block)
;	HL	Zeiger auf Name (11 Byte) bei INIT
;
; PA:	A	Datenbyte
;
; VR:	AF, BC, DE, HL
;
MBINC	EQU	$-BASEX+200H
	LD	C,88H
	IN	B,(C)
	RES	7,B		; BASIC aus
	OUT	(C),B
	call	UMBIN		; Byte-Eingabe im ROM aufrufen
MBOK:	LD	C,88H
	IN	B,(C)
	SET	7,B		; BASIC ein
	OUT	(C),B
	RET	NC		; kein Error
	LD	HL,(IOERR)
	PUSH	HL		; Error-Routine anspringen
	JP	IRMOF
;
; Byteweise MB-Ausgabe (CAOS-UP Nr. 38H)
;
; PE:	A	Datenbyte
;	D	Steuerbyte	Bit3=1 -> INIT (1. Block)
;				Bit6=1 -> CLOSE (letzter Block)
;	HL	Zeiger auf Name (11 Byte) bei INIT
;
; PA:	-
;
; VR:	AF, DE, HL
;
MBOUTC	EQU	$-BASEX+200H
	LD	C,88H
	IN	B,(C)
	RES	7,B		; BASIC aus
	OUT	(C),B
	call	UMBOUT		; Byte-Ausgabe im ROM aufrufen
	BIT	6,D		; Close?
	JR	Z,MBOK		; nein
	SET	0,(IX+8)	; Taste gedrueckt, fuer Verify
	JR	MBOK		; weiter wie bei MBIN

BASEND:	;Ende BASEX-Programmteil

SUBAS	EQU	$-BASEX+200H	;hier wird die SUTAB hin kopiert

;-------------------------------------------------------------------------------
; Sprungverteiler und Serviceroutinen im ROM:
;-------------------------------------------------------------------------------
;
; PE:	A	Nummer der Routine (0..12)
;		IRM ist ein, BASIC ist aus
;
BSERV:	LD	DE,NULL			; DIR-Maske fuer "alles"
	CP	2
	JP	Z,UDIR	; CALL*DE	DIR anzeigen (unsortiert)
	CP	8
	JR	NZ,BSERR
;
; CALL*F0	DIR wechseln fuer BASIC (und EDAS)
;
BASCD:	call	pv1
	db	ostr
	db	'CD ',0		; Kommando CD vorgeben
	CALL	PV1
	DB	INLIN		; Kommando vervollstaendigen
	INC	DE
	INC	DE		; DE vor bis auf Verzeichnisnamen
	INC	DE
	JP	UCD		; Verzeichnis wechseln
;
; Fuer alle anderen CALL's anzeigen was geht:
;
BSERR:	call	pv1
	db	ostr	; fuer die anderen Kommandos Hilfe anzeigen:
	DB	'CALL*DE  = DIR',cr,lf
	DB	'CALL*F0  = CD',cr,lf
	DB	'CALL*150 = USB ein/aus',cr,lf
NULL:	DB	0
	ret

;******************   PROGRAMM BSAVE	**************************************
;									     *
; Abspeichern von BASIC-Programmen auf MC-Ebene mit AUTO-Start		     *
; Es wird automatisch der Dateityp .KCB vergeben, um die Files von reinen    *
; MC-Programmen unterscheiden zu koennen.				     *
;									     *
; Test, ob BASIC initialisiert wurde.					     *
;									     *
; BASIC-Programmspeicher kann auf 7FFFH begrenzt werden, um Programme kompa- *
; tibel zum KC85/3 zu machen.						     *
;									     *
; Menuewort %bsave	(hier speziell fuer USB)			     *
;									     *
;*****************************************************************************

; BASIC-Vereinbarungen

WSP	EQU	0300h		; BASIC-Arbeitsspeicher
STDPTR	EQU	0356h		; Ende Stringspeicher (256 Byte vor Speicherende)
PSTBEG	EQU	035Fh		; Programmstart-Adresse
INPBUF	EQU	0362h		; Eingabepuffer+1
MEMSIZ	EQU	03B0h		; Speiucherende (MEMORY SIZE)
SWAPTR	EQU	03C4h		; Zeiger fuer naechsten String
SVARPT	EQU	03D7h		; Ende des BASIC-Programms
DVARPT	EQU	03D9h		; Adresse der Liste der Feldvariablen
FSLPTR	EQU	03DBh		; Erste freie Adresse hinter den Listen
NEW2	EQU	0C64Fh		; BASIC-Programm initialisieren
RUNMOD	EQU	0C854h		; RUN ausfuehren

; CAOS-Vereinbarungen

BASMEN	EQU	0E011H		; hier steht beim KC85/4+ das BASIC-Menuewort
BASON	EQU	0E02FH		; BASIC aktivieren
;
; BASIC-Autostart:
;
BSTART:	LD	A,(BASMEN)
	CP	7FH		; KC85/4+ ?
	CALL	Z,BASON		; ja, dann Basic einschalten
	IN	A,(PIOAD)
	AND	0FBH		; fuer CAOS 3.1..4.22 und 5.0 IRM aus
	RRCA			; Bit 0 (ROM E) nach Bit 7 (BASIC) kopieren:
	RLA			;  fuer CAOS 3.4 BASIC-ROM an
	RLA			;  und fuer M006 CAOS-ROMF(=BASIC-ROM) aus
	RRCA			;   je nach Schaltzustand vom ROM E
	OUT	(PIOAD),A
	CALL	NEW2		; Basic-Programm initialisieren
	JP	RUNMOD		; RUN ausfuehren
STARTE:
;
;-----------------------------------------------------------------------------
;
KCB:	DB	'KCB'		; Dateiname *.KCB
;
	DW	7F7FH
	DB	'bsave',1	; verstecktes Menuewort fuer alle CAOS-Versionen
	SET	0,(IX+7)	; CAOS-Modus (LOAD)
	call	minit		; Modul initialisieren
	ret	c		; nicht gefunden
	call	sync		; mit Vinculum synchronisieren
	ret	c		; TimeOut
	LD	HL,0EE00H	; CAOS-Zeichensatz 1
	LD	(CCTL0),HL
	LD	HL,0FE00H	; CAOS-Zeichensatz 2
	LD	(CCTL1),HL
;
; Test, ob BASIC initialisiert wurde:
;
	LD	A,(WSP)
	CP	0C3H		; JP RSTLOC vorhanden?
	JR	NZ,NBAS		; nein, BASIC nicht initialisiert
	LD	A,(WSP+3)
	CP	0C3H		; JP USRLOC vorhanden?
	JR	NZ,NBAS		; nein, BASIC nicht initialisiert
	LD	HL,PSTBEG
	LD	E,(HL)
	INC	HL
	LD	D,(HL)		; DE = Beginn BASIC-Programm
	LD	HL,0400H
	SBC	HL,DE		; muss 0401h oder hoeher sein!
	JR	C,AANZ		; OK.
NBAS:	CALL	PV1
	DB	OSTR		; Programm zu gross oder nicht initialisiert
	DB	'BASIC Programmfehler'
	DB	7,CR,LF,0
	RET
	;
AANZ:	LD	HL,(SVARPT)	; Programmende
	LD	A,H
	AND	80H		; darf maximal bis 7FFFh gehen...
	JR	NZ,NBAS
;
; Adressen anzeigen:
;
EOK:	CALL	PV1
	DB	OSTR
	DB	0Bh		; Cursor nach oben
	DB	'%bsave ',0
	LD	A,3
	LD	(ARGN),A		; 3 Argumente fuer USAVE
	LD	HL,(03D7H)		; Programmende+1
	LD	(ARG2),HL
	EX	DE,HL
	LD	HL,300H			; Anfangsadresse = WORKRAM
	LD	(ARG1),HL
	CALL	PV1
	DB	HLDE			; AADR EADR anzeigen
	LD	HL,0370H		; Startadresse
	LD	(ARG3),HL
	CALL	PV1
	DB	HLHX			; SADR anzeigen
	CALL	PV1
	DB	CRLF
	EX	DE,HL			; DE=0370h
	LD	HL,BSTART		; Auto-Startprogramm
	LD	BC,STARTE-BSTART
	LDIR				; in den BASIC-Eingabepuffer kopieren
	LD	HL,KCB			; Dateityp *.KCB
	CALL	GETNAM			; Dateiname eingeben
	JR	NC,BSA1
	LD	A,(VERSION)
	CP	43h		; CY=1 ist ab CAOS 4.3 BRK!
	ret	nc
BSA1:	CALL	BSAVE			; Datei speichern (wenn kein TimeOut)
	JP	EXIT2			; Modul aus und Interrupt: CAOS-Mode

;-----	end of bsave
