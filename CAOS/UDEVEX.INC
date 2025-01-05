;
;	USB-Treiber fuer EDAS - analog DEVEX vom D004
;	(nur fuer CAOS vor Version 4.7 benoetigt)
;
;	fuer EDAS 1.2 (?) / 1.4 / 1.5 / 1.6 / 1.7 (RAM-Version)
;	(EDAS 1.2 findet keine Menueworte im IRM?)
;
; Funktionsprinzip:
; - Erweiterungsprogramm und modifizierte SUTAB stehen im IRM ab BA00h
; - LOAD+SAVE-Routinen werden in der SUTAB modifiziert zur Umlenkung auf
;   das USB-Geraet
; - Grundfunktionen springen in die ROM-Ebene des M052
; - EDAS-Speicherbereich wird mit SWITCH ein und aus
;   geschalten, um Zugriff auf M052-Ebene freizugeben
; 21.03.2022: EDAS-Suche auf RAM-EDAS ausgedehnt
;
; Systemvoraussetzung:
;	KC85/3 mit CAOS 3.1 mit EDAS im Modul (M027) oder EDAS im RAM
;	KC85/4 mit CAOS 4.2 mit EDAS im Modul (M027) oder EDAS im RAM
;	KC85/5 mit CAOS 4.3-4.8 und EDAS intern im USER-ROM
;
; UDEVEX.INC	(c) M. Leubner, 29.11.2008-21.03.2022
;------------------------------------------------------------------------

	DW	7F7Fh
	db	'edas',1	; fuer alle CAOS-Versionen geeignet
	LD	HL,EDAS-DEVEX+0BA06h
	JR	UEDAS
 
	DW	7F7Fh
	db	'reedas',1	; fuer alle CAOS-Versionen geeignet
	LD	HL,REEDAS-DEVEX+0BA06h
UEDAS:	LD	(0BA00h),HL	; Vergleichskette fuer Menuewort

; FLOAD auf Adresse 0 loeschen wegen EDAS-Konflikt:
	LD	HL,SLOT		; Suche nach Dateiname und M052-Steckplatz
	LD	B,80h
DEV01:	LD	A,(HL)
	CP	7FH		; Prolog?
	JR	Z,DEV02		; Prolog gefunden
	INC	HL
	DJNZ	DEV01
	JR	DEV03
	;
DEV02:	LD	(HL),0		; FLOAD loeschen
;
; System-SUTAB in den IRM kopieren:
;
DEV03:	LD	HL,(SUTAB)	; System-SUTAB
	LD	DE,SUDEV
	AND	A
	SBC	HL,DE		; SUDEV bereits aktiv?
	JR	Z,SEARCH	; ja!
	LD	HL,(SUTAB)
	LD	(SUBALT),HL	; Original SUTAB hier merken
	LD	BC,94H		; max. Laenge bei CAOS 4.7
	LDIR			; SUTAB kopieren
;
; kopierte SUTAB im IRM modifizieren:
;
	LD	HL,EMBO
	LD	(SUDEV+2),HL	; MBO ersetzen
	LD	HL,EMBI
	LD	(SUDEV+0AH),HL	; MBI ersetzen
	LD	HL,EISRO
	LD	(SUDEV+10H),HL	; ISRO ersetzen
	LD	HL,ECSRO
	LD	(SUDEV+12H),HL	; CSRO ersetzen
	LD	HL,EISRI
	LD	(SUDEV+14H),HL	; ISRI ersetzen
	LD	HL,ESCRI
	LD	(SUDEV+16H),HL	; CSRI ersetzen
;
; Suche nach Steckplatz von EDAS -> Abarbeitung im IRM ab BA00h
;	- Steckplatz 2 (USER-ROM)
;	- dann von Steckplatz 8 bis FFh
; PE:	-
; PA:	?	Steckplatz des EDAS-Moduls
;
SEARCH:	LD	DE,0BA06h
	LD	HL,DEVEX	; Suchprogramm und DEVEX selbst
	LD	BC,DEVEND-DEVEX
	LDIR			; nach BA06h kopieren
	JP	0BA06h		; und im IRM ausfuehren...
;
; Ab hier im IRM ab Adresse BA00h
;
;BA00h	DW	Adresse		; Vergleichskette bzw. Einsprungadresse
;BA02h	DB	2		; Steckplatz EDAS-Modul
;BA03h	DB	MODUL		; Steckplatz zu schaltendes Modul
;BA04h	DB	ON		; Sicht auf USB:  M052 ON, EDAS OFF
;BA05h	DB	OFF		; Sicht auf EDAS: M052 OFF, EDAS ON
;
DEVEX:	LD	A,2		; Modul schalten
	LD	L,A		; Steckplatz USER-ROM
	LD	D,0E1h		; EDAS-Ebene im USER-ROM (bei KC85/5)
	CALL	PV1
	DB	MODU		; Modul einschalten
	CALL	SU-DEVEX+0BA06h	; EDAS-Menuewort suchen
	LD	B,2		; Steckplatz
	JR	C,EDASOK	; gefunden im USER-ROM!
	LD	A,B		; A=2: Modul schalten
	LD	L,A		; Steckplatz 2=USER-ROM
M0	EQU	$+1-DEVEX+0BA06h; NULL-Maske fuer DIR
	LD	D,0		; AUS
	CALL	PV1		; USER-ROM bzw. BASIC aus
	DB	MODU		; damit die Sicht auf andere Module frei ist
;
; im USER-Rom nicht gefunden, deshalb Suche in allen Modulsteckplaetzen
;
	LD	B,8		; ab Steckplatz 8
	LD	C,80h		; Modulschaltung
SRCH2:	IN	A,(C)		; Strukturbyte lesen
	INC	A
	JR	Z,SRCH3		; kein Modul
	LD	H,High(MODST)
	LD	L,C
	LD	A,(HL)		; aktuellen Schaltzustand lesen
	INC	A		; FF? (Modul ist mit JUMP aktiviertes System)
	JR	Z,SRCH3		; dann Modul nicht verwenden!
	LD	A,0C1h		; Steuerbyte fuer Adresse C000h
	OUT	(C),A		; Modul testweise auf C000h EIN schalten
	CALL	SU-DEVEX+0BA06h	; EDAS-Menuewort suchen
	JR	C,EDASOK	; gefunden im Modul
	XOR	A
	OUT	(C),A		; Modul AUS schalten	
SRCH3:	INC	B		; naechster Steckplatz
	JR	NZ,SRCH2	; bis FFh weiter suchen
;
; in keinem Modulsteckplatz gefunden, deshalb noch Suche EDAS im RAM
;
	DEC	B		; Pseudo-Steckplatz FF falls RAM-EDAS
	CALL	SUR-DEVEX+0BA06h; EDAS-Menuewort im RAM suchen

EDASOK:	INC	HL		; Epilog des Menuewortes uebergehen
	LD	(0BA00h),HL	; Startadresse des Menuewortes
	LD	A,B		; Steckplatz wo EDAS gefunden wurde
	LD	(0BA02h),A	; hier eintragen

; USER ROM aus und alle anderen Modulzustaende regenerieren:

	PUSH	AF		; CY=1 wenn EDAS gefunden
	LD	A,2		; Modul schalten
	LD	L,A		; Steckplatz USER-ROM
	LD	D,0		; AUS
	CALL	PV1
	DB	MODU		; USER-ROM ausschalten

	ld	hl,modst+8	; beginnend ab Steckplatz 8
	ld	c,80h		; Modulschaltung
SRCH4:	ld	a,(hl)		; gespeicherter Schaltzustand
	ld	b,l
	out	(c),a		; Schaltzustand regenerieren
	inc	l		; naechster Steckplatz
	jr	nz,SRCH4	; bis FFh
	POP	AF
	JR	C,SRCH5		; EDAS gefunden
	CALL	PV1
	DB	OSTR
	DB	'Kein EDAS gefunden!'
	DB	7,CR,LF,0
	RET

SRCH5:	CALL	MINIT		; M052 suchen und einschalten
	RET	C		; M052 nicht gefunden
;
; Modulschaltung vorbereiten, EDAS einschalten und starten
;
	LD	HL,0BA02h	; Steckplatz EDAS
	LD	A,(HL)
	CP	2		; EDAS im USER-ROM?
	LD	D,0E1h		; Steuerbyte fuer EDAs im USER-ROM
	JR	Z,SRCH6
	LD	D,0C1h		; Steuerbyte fuer EDAS im Modul
SRCH6:	LD	A,(SLOT)	; Steckplatz M052
	CP	(HL)
	JR	C,SRCH7		; M052 auf kleinerem Steckplatz

; M052 bleibt immer an, EDAS-Modul schalten
	LD	A,(HL)		; Steckplatz EDAS
	INC	HL
	LD	(HL),A		; EDAS ist zu schalten
	INC	HL
	LD	(HL),0		; Sicht auf M052: EDAS aus
	INC	HL
	LD	(HL),D		; Sicht auf EDAS: EDAS ein
	JR	SRCH8

; EDAS immer an, M052 schalten
SRCH7:	LD	E,(HL)		; Steckplatz EDAS
	INC	HL
	LD	(HL),A		; M052 ist zu schalten
	LD	HL,0C5h		; L=M052 ein
	LD	(0BA04h),HL	; H=M052 aus
	LD	L,E		; Steckplatz EDAS
	LD	A,2		; Modul schalten
	CALL	PV1
	DB	MODU		; EDAS einschalten

SRCH8:	CALL	OFF		; Sicht auf EDAS
	LD	HL,(0BA00h)	; Adresse EDAS / REEDAS
	JP	(HL)		; und los geht's
;
; Menuewort "EDAS" suchen im Bereich C000h bis DFFFh
; PA:	CY=1	gefunden
;	HL	Ende+1 gefundene Kette
; VR:	AF, HL, DE
;
SU:	PUSH	BC
	LD	HL,0C000h	; Beginn Suchbereich ROM-EDAS
SU1:	LD	BC,2000h	; Laenge Suchbereich
	LD	DE,(0BA00h)	; Menuewort "EDAS" bzw. "REEDAS"
	LD	A,(IX+9)	; Prologbyte 7Fh
	CALL	PV1
	DB	ZSUCH		; Menuewort suchen
	POP	BC
	RET
	;
SUR:	PUSH	BC
	LD	HL,200H		; Beginn Suchbereich RAM-EDAS
	JR	SU1
	;
REEDAS:	DB	'RE'
EDAS:	DB	'EDAS',0	; Vergleichskette fuer Suche
;
; Umschaltung zwischen EDAS und USB-Modul:
;
ON	EQU	$-DEVEX+0BA06H
	PUSH	AF
	LD	A,(0BA04h)	; Steuerbyte fuer Sicht auf USB
	JR	ONOF
	;
OFF	EQU	$-DEVEX+0BA06H
	PUSH	AF
	LD	A,(0BA05h)	; Steuerbyte fuer Sicht auf EDAS
ONOF:	PUSH	HL
	PUSH	DE
	PUSH	BC
	LD	D,A		; Steuerbyte
	LD	HL,(0BA03h)	; L=Steckplatz zu schaltendes Modul
	LD	A,2		; Modul schalten
	CALL	PV1
	DB	MODU
	POP	BC
	POP	DE
	POP	HL
	POP	AF
	RET
;
; neues Menuewort im EDAS-Menu:
;
;	USB	=	Anzeige Zustand + Hilfe
;	USB 0	=	USB aus
;	USB 1	=	USB ein
;	USB 2	=	UDIR
;	USB 3	=	CD .. (Pfad wird abgefragt)

	DW	0DDDDH
	DB	'USB',1
	ld	a,(argn)
	and	a		; Parameter angegeben?
	jr	nz,U1		; ja, dann auswerten
U5:	CALL	UOO		; Status anzeigen
	CALL	PV1
	DB	OSTR		; Hilfetext anzeigen
	DB	' (0=aus, 1=ein, 2=DIR, 3=CD)',cr,lf,0
	ret
	;
U1:	LD	HL,(SUBALT)	; urspruengliche SUTAB
	ld	a,(arg1)	; Parameter
	and	a		; 0?
	jr	z,U2
	dec	a		; 1?
	jr	nz,U3
	ld	hl,6ch		; Arbeitszelle "STAT" von EDAS 1.6
	res	1,(hl)		; Tape-Modus erzwingen
	LD	HL,SUDEV	; modifizierte SUTAB
U2:	LD	(SUTAB),HL
	CALL	UOO		; und anzeigen was aktiv ist
	CALL	PV1
	DB	CRLF
	ret
	;
U3:	dec	a		; 2?
	jr	nz,U4
	call	on
	LD	DE,M0		; DIR-Maske fuer "alles"
	call	udir		; DIR anzeigen
	jp	off
	;
U4:	dec	a		; 3?
	jr	nz,U5
	call	on
	call	BASCD		; Verzeichnisname anfordern, DIR wechseln
	jp	off
;
; USB-Status anzeigen
;
UOO	EQU	$-DEVEX+0BA06H
	CALL	PV1
	DB	OSTR
	DB	'USB=',0
	LD	HL,(SUTAB)
	LD	DE,SUDEV
	AND	A
	SBC	HL,DE		; SUDEV aktiv?
	JR	Z,U0		; ja!
	CALL	PV1
	DB	OSTR
	DB	'aus',0
	RET
	;
U0:	CALL	PV1
	DB	OSTR
	DB	'ein',0
	RET
;
; Initialisierung Eingabe, einlesen 1. Block		ISRI	UP-Nr. 0AH
;
; PE:	(IX+5/6)	Pufferanfang
;	HL		Dateiname
; PA:	CY=1		Fehler
;	(IX+2)		Blocknummer
; VR:	AF,BC,DE
;
EISRI	EQU	$-DEVEX+0BA06H
	POP	DE		; Return-Adresse
	POP	HL		; Name nach HL holen (wie in DEVEX vom D004)
	PUSH	HL		; 8 Zeichen Dateiname, 3 Zeichen Typ
	LD	B,8
EISRI1:	LD	A,(HL)
	OR	A
	JR	NZ,EISRI2
	LD	(HL),' '	; 00h in 20h umwandeln fuer Dateiname
EISRI2:	INC	HL
	DJNZ	EISRI1
	POP	HL
	PUSH	HL
	PUSH	DE
	call	on
	call	UISRI		; Datei oeffnen, 1. Block einlesen
	jr	EOFF
;
; Datenblock einlesen					MBI	UP-Nr. 05H
;
; PE:	(IX+5/6)	Pufferanfang
; PA:	CY=1		Fehler
;	(IX+2)		Blocknummer
; VR:	AF,BC
;
EMBI	EQU	$-DEVEX+0BA06H
	call	on
	call	UMBI		; Datenblock einlesen
EOFF:	call	off
	ret	nc		; kein Fehler
	LD	A,3
	LD	(IX+13),A	; BRK als Tastencode eintragen
	SET	0,(IX+8)	; und gueltig machen
	ret
;
; Eingabe abschliessen, Autostart ausfuehren		CSRI	UP-Nr. 0BH
;
; PE:	-
; PA:	CY=1		Fehler
; VR:	AF,HL
;
ESCRI	EQU	$-DEVEX+0BA06H
	call	on
	call	UCSRI		; Datei schliessen
	push	af
	CALL	PV1
	DB	CRLF
	pop	af
	jr	EOFF
;
; Initialisierung Ausgabe, ausgeben 1. Block		ISRO	UP-Nr. 08H
;
; PE:	BC		Laenge Vorton
;	(IX+5/6)	Pufferadresse
;			Dateiname im Puffer (11 Zeichen)
; PA:	DE=HL		Pufferende+1
;	CY=1		Fehler
;	(IX+2)		Blocknummer
; VR:	AF,BC,DE,HL
;
EISRO	EQU	$-DEVEX+0BA06H
	LD	L,(IX+5)
	LD	H,(IX+6)	; Name aus Kassettenpuffer holen
	PUSH	HL		; 8 Zeichen Dateiname, 3 Zeichen Typ
	LD	B,8
EISRO1:	LD	A,(HL)
	OR	A
	JR	NZ,EISRO2
	LD	(HL),' '	; 00h in 20h umwandeln fuer Dateiname
EISRO2:	INC	HL
	DJNZ	EISRO1
	POP	HL
	call	on
	call	UISRO		; Ausgabedatei oeffnen und ersten Block ausgeben
	jr	EOFF
;
; Datenblock ausgeben					MBO	UP-Nr. 01H
;
; PE:	BC		Laenge Vorton
;	(IX+2)		Blocknummer-1
;	(IX+5/6)	Pufferadresse
; PA:	DE=HL		Pufferende+1
;	CY=1		Fehler
;	(IX+2)		Blocknummer
; VR:	AF,BC,DE,HL
;
EMBO	EQU	$-DEVEX+0BA06H
	call	on
	call	UMBO		; Datenblock schreiben
	jr	EOFF
;
; Ausgabe abschliessen, Ausgabe letzter Block (FF)	CSRO	UP-Nr. 09H
;
; PE:	BC		Laenge Vorton
;	(IX+2)		Blocknummer-1
;	(IX+5/6)	Pufferadresse
; PA:	DE=HL		Pufferende+1
;	CY=1		Fehler
;	(IX+2)		Blocknummer 0FFH
; VR:	AF,BC,DE,HL
;
ECSRO	EQU	$-DEVEX+0BA06H
	call	on
	call	UCSRO		; Datei schliessen
	jr	EOFF

DEVEND:	;bis hier geht DEVEX (Teil zum Umladen)

SUBALT	EQU	$-DEVEX+0BA06H		; Merkzelle fuer alternative SUTAB
SUDEV	EQU	$-DEVEX+0BA06H+2	; hier wird die SUTAB hin kopiert

;-----	end of include
