/**
 * SNES ROM HEADER
 */

.ifndef _HEADER_INC
.ifndef _LIB

.ifdef _MAKESMC
.snesheader
  ;ID "SPC "
  NAME "SUPERC SPC PLAYER"
  ;    "123456789012345678901"

  ;FASTROM
  ;HIROM
  LOROM

  CARTRIDGETYPE 0	; ROM ONLY
  ROMSIZE 9		; 4Mbits(512KB)
  ;ROMSIZE 10		; 8Mbits(1024KB)
  ;ROMSIZE 12		; 32Mbits(4096KB)
  SRAMSIZE 0		; No-SRAM
  COUNTRY 0		; Jp
  LICENSEECODE 0	; 0 = Nintendo
  VERSION 0		;
.endsnes

.else

.snesheader
  ID "SPC "
  NAME "SUPERC SPC PLAYER"
  ;    "123456789012345678901"

  ;FASTROM
  ;HIROM
  LOROM

  CARTRIDGETYPE 0	; ROM ONLY
  ROMSIZE 7		; 1Mbits(128KB)
  SRAMSIZE 0		; No-SRAM
  COUNTRY 0		; Jp
  LICENSEECODE 0	; 0 = Nintendo
  VERSION 0		;
.endsnes

.endif

.snesnativevector
  COP		EmptyHandler
  BRK		EmptyHandler
  ABORT		EmptyHandler
  NMI		EmptyHandler
  IRQ		EmptyHandler
.endnativevector

.snesemuvector
  COP		EmptyHandler
  ABORT		EmptyHandler
  NMI		EmptyHandler
  RESET		Main
  IRQBRK	EmptyHandler
.endemuvector

.endif

.endif

