;
; YMZ280B 68000 support code
; 2025 Mike Moffitt
;

;
; ------------------
; STANDARD REGISTERS
; ------------------
;
; Registers are indexed by number, so when writing from the 68000 side double
; the offset as there is no A0 line.
;
; The standard blocks index different channels by adding $4, so each block is
; repeated eight times within its region.
;
; $00-$1F: Play control
; +$0 : frequency
; +$1 : key / frequency high
; +$2 : total level
; +$3 : panpot
;
; $20-$3F: High address byte
; +$0 : Start
; +$1 : Loop start
; +$2 : Loop end
; +$3 : End address
;
; $40-$5F: Middle address byte
; +$0 : Start
; +$1 : Loop start
; +$2 : Loop end
; +$3 : End address
;
; $60-$7F: Low address byte
; +$0 : Start
; +$1 : Loop start
; +$2 : Loop end
; +$3 : End address
;
; -----------------
; UTILITY REGISTERS
; -----------------
;
; These registers do not follow the pattern above.
; $80 : Enablement
;     7654 3210
;     L... R... : LENB/RENB left and right channel enable bits; '1' = disable
;     .lll .rrr : LCH2..0/RCH2..0 left and right channel select bits (address)
;
; $81 : DSP enable     ; '0' = disable
; $82 : DSP data
; $84 ; RAM address H
; $85 ; RAM address M
; $86 ; RAM address L
; $87 : RAM data
; $FE : IRQ mask '1' = disable
; $FF : Various control flags
;     7654 3210
;     k... .... : KENB key on enable  '0' = all channels key off
;     .m.. .... : MENB memory enable  '0' = pin59 becomes an IRQ output. '1' makes it a DSPCDI input.
;     ...i .... : IENB IRQ enable     '0' = memory pins go high impedance.
;     .... ..tt ; TST2..1 LSI test;   '0' = normal operation
;
;
; ------------
; STATUS FLAGS
; ------------
;
; Read from the data port to get the status register.
;
; Each flag bit corresponds to a channel. The bit goes high when a channel has
; its corresponding IRQ enable bit set and the playback has reached the
; end address.
;
; Reading the status register clears all bits that were set high.
;

; You must define YMZ_BASE before including this.

YMZ_CTRL = YMZ_BASE + $00  ; Selecting registers or reading external memory
YMZ_DATA = YMZ_BASE + $02  ; Writing data or reading status register

; Description of binary data emitted by ymztool.
YMZDAT struc
key:			ds.b 1  ; also fn8
fn:			ds.b 1
tl:			ds.b 1
pan:			ds.b 1
start_address:		ds.b 3
loop_start_address:	ds.b 3
loop_end_address:	ds.b 3
end_address:		ds.b 3
YMZDAT ends
