/**
 * SuperC N-Format DSP Update Process
 *
 * DSPレジスタの更新処理
 *
 */

.incdir  "./include/"
.include "spc.inc"
.include "var.inc"

.section "__dsp__" free
;--------------------------------------------------
; DspPreProcess - DSP REGISTER CONTROLL PROCESS
;   Level       : 1 (Main Level)
;   Input       : none
;   Output      : none
;   Description : DSP制御処理1(前処理)
;                 前回周期時に更新したメモリの
;                 データにより、KEYON等を行う
;--------------------------------------------------
DspPreProcess:
; チャンネル毎のデータを更新する
; (Vol, Pitch, Srcn, ADSR, Gain)

	mov	x, #0			; x ... ch更新情報メモリ読出しカウンタ
	mov	y, #0			; y ... DSP メモリアドレス

	; --- DSPメモリ更新
__	mov	SPC_REGADDR, y
	mov	a, buf_chData+x
	mov	SPC_REGDATA, a
	inc	x
	inc	y
	mov	a, y
	xcn	a
	bpl	_b			; DSP $X0 ~ $X7 までループ

	inc	x			; AllocTrack分インクリメント

	; --- 次CHに移動
	and	a, #$0f
	inc	a
	xcn	a
	mov	y, a
	bpl	_b			; 8CH分ループ

; 全チャンネル共通のデータを更新する
	; エコー
	mov	a, buf_echo
	mov	SPC_REGADDR, #DSP_EON
	mov	SPC_REGDATA, a
	mov	a, buf_eVol
	mov	SPC_REGADDR, #$2c
	mov	SPC_REGDATA, a
	mov	SPC_REGADDR, #$3c
	mov	SPC_REGDATA, a
	; ノイズ
	mov	a, buf_noise
	mov	SPC_REGADDR, #DSP_NON
	mov	SPC_REGDATA, a
	; キーオン
	mov	a, buf_keyon
	mov	SPC_REGADDR, #DSP_KON
	mov	SPC_REGDATA, a
	; キーオフ(キーオンする場所は対象外)
	eor	a, #$ff
	mov	SPC_REGADDR, #DSP_KOF
	and	a, buf_keyoff
	mov	SPC_REGDATA, a

	; RAMの始末をする
	mov	a, #0
	mov	buf_keyon, a
	mov	buf_keyoff, a

	ret


;--------------------------------------------------
; DspPostProcess - DSP REGISTER CONTROLL PROCESS
;   Level       : 1 (Main Level)
;   Input       : none
;   Output      : none
;   Description : DSP制御処理2(後処理)
;                 以下を処理します
;                 - Volume (+ Panpot)
;                 - Pitch
;--------------------------------------------------
;----------------------------------------
; local values
;----------------------------------------
.enum	$08
	lTemp		dw
	lTrackInx	db
	lPitch		dw
	lPitchDif	dw
	lVolumeH	db
	lVolumeL	db
	lpan		db
	llvol		db
	lrvol		db
.ende

DspPostProcess:
	mov	x, #(_sizeof_stChannel * 7)

_ChannelLoop:
	mov	a, buf_chData.allocTrack+x
	bpl	+
	jmp	_BeforeChannelLoop

+	mov	lTrackInx, a
	push	x
	mov	x, a

	; --- Pitch計算
	push	x
	mov	a, track.curKey+x
	push	a
	call	CalcScale
	movw	lPitch, ya
	pop	a
	cmp	a, #$48
	bmi	+
	mov	y, #$21
	mov	a, #$7c
	bra	++
+	inc	a
	call	CalcScale
++	subw	ya, lPitch
	movw	lPitchDif, ya
	pop	x

	mov	a, track.PitchCounter+x
	bne	+
	mov	a, track.PitchPhase

+	mov	y, lPitch
	mov	a, track.pitchRatio+x
	mul	ya
	movw	lTemp, ya
	mov	y, lPitch+1
	mov	a, track.pitchRatio+x
	mul	ya
	clrc
	adc	a, lTemp+1
	mov	y, a
	mov	a, lTemp
	movw	lPitch, ya

	; --- Volume計算
	mov	a, track.velocity+x
	mov	lTemp, a
	mov	a, track.panH+x
	mov	lpan, a
	mov1	c, musicSysFlags.2	; pampot off?
	bcc	_Panpot
	mov	a, lTemp
	lsr	a
	mov	llvol, a
	mov	lrvol, a
	bra	_VolLevel

_Panpot:
	cmp	a, #$3f
	bne	+
	mov	a, lTemp
	bra	++
+	and	a, #$3f
	mov	y, a
	mov	a, #0
	push	x
	mov	x, #$3f
	div	ya, x
	pop	x
	mov	y, lTemp
	mul	ya
	mov	a, y
++	mov	lrvol, a
	mov	a, lTemp
	setc
	sbc	a, lrvol
	mov	llvol, a

_VolLevel:
	; --- left
	mov	y, a
	inc	y
	mov	a, track.volumeH+x
	mul	ya
	inc	y
	mov	a, musicGlobalVolume
	mul	ya
	mov	llvol, y

	; --- right
	mov	a, lrvol
	mov	y, a
	inc	y
	mov	a, track.volumeH+x
	mul	ya
	inc	y
	mov	a, musicGlobalVolume
	mul	ya
	mov	lrvol, y

	pop	x

	mov	a, lPitch
	mov	buf_chData.pitch+x, a
	mov	a, lPitch+1
	mov	buf_chData.pitch+1+x, a

	mov	a, llvol
	mov1	c, lpan.7
	bcc	+
	eor	a, #$ff
	inc	a
+	mov	buf_chData.lvol+x, a
	mov	a, lrvol
	mov1	c, lpan.6
	bcc	+
	eor	a, #$ff
	inc	a
+	mov	buf_chData.rvol+x, a

_BeforeChannelLoop:
	mov	a, x
	setc
	sbc	a, #_sizeof_stChannel
	mov	x, a
	bcc	+
	jmp	_ChannelLoop

+	mov	SPC_REGADDR, #$0c
	mov	y, SPC_REGDATA
	mov	a, eVolRatio
	and	a, #$7f
	asl	a
	mul	ya
	mov	a, y
	bbc	eVolRatio.7, +
	eor	a, #$ff
	inc	a
+	mov	buf_eVol, a

	mov	SPC_REGADDR, #DSP_FLG
	mov	a, SPC_REGDATA
	and	a, #(FLG_ECEN ~ $ff)
	mov	y, buf_echo
	bne	+
	or	a, #FLG_ECEN
+	mov	SPC_REGDATA, a
	ret

;----------------------------------------
; local values undefine
;----------------------------------------
.undefine		lPitch
.undefine		lPitchDif
.undefine		lVolumeH
.undefine		lVolumeL

;--------------------------------------------------
; DspReleaseMuteChannel - DSP REGISTER CONTROLL PROCESS
;   Level       : 1 (Main Level)
;   Input       : none
;   Output      : none
;   Description : DSP制御処理3
;                 音の鳴っていないチャンネルを開放します
;--------------------------------------------------
;------------------------------
; local values
;------------------------------

DspReleaseMuteChannel:
	mov	y, #0
	mov	a, #DSP_ENVX
-	mov	SPC_REGADDR, a
	mov	x, SPC_REGDATA
	bne	+

	; OUTX = 0、つまり音は鳴っていない
	mov	x, #$ff
	mov	buf_chData.allocTrack+y, x
	mov	buf_chData.srcn+y, x			; debug

	; ループ前処理
+	mov	x, a
	mov	a, y
	clrc
	adc	a, #_sizeof_stChannel
	mov	y, a
	mov	a, x
	xcn	a
	inc	a
	xcn	a
	bpl	-					; 8CH分ループ
	ret
;------------------------------
; local values undefine
;------------------------------

.ends

