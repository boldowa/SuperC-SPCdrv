/**
 * SNES MEMORY MAP
 */

.ifndef _MAP_INC
.define _MAP_INC

.include "ram.inc"

.ifndef _LIB
.define EMPTY $ff
.emptyfill EMPTY

.memorymap
  ;slotsize $10000
  slotsize $8000
  defaultslot 0
  ;slot 0 $0000
  slot 0 $8000
.endme

;.rombanksize $10000
.rombanksize $8000
;.rombanks 8
.ifdef _MAKESMC
.rombanks 16
.else
.rombanks 4
.endif
;.rombanks 64

.endif

;.base $c0

.define NULL 0

/* SNES RAM USAGE */
.define Scratch 0

/* SNES I/O PORTS */
.define SPC_PORT0		$2140
.define SPC_PORT1		$2141
.define SPC_PORT2		$2142
.define SPC_PORT3		$2143

.endif

