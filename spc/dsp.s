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

	; キーオフ(キーオンする場所は対象外)
	mov	a, buf_keyon
	eor	a, #$ff
	and	a, buf_keyoff
	mov	SPC_REGADDR, #DSP_KOF
	mov	SPC_REGDATA, a

	; 音の鳴っていない、且つKEYONしないCHを開放する
	; (※ 重くなる、もしくは割り当て不安定なら、要コメントアウト)
	call	DspReleaseMuteChannel

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
	; --- チャンネルループ ここまで
	; --- 以下はループを抜けた後の処理

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
;                 - Pitch
;                 - Volume (+ Panpot)
;--------------------------------------------------
;----------------------------------------
; local values (l***)
;----------------------------------------
.enum	$08
	lTemp		dw
	lTrackInx	db
	lPitch		dw
	lPitchDif	dw
	lVolumeH	db
	lVolumeL	db
	lpan		db
	lrvol		db
	llvol		db
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

	/******************************/
	/* Pitchを計算する            */
	/******************************/
_CalcPitch
	push	x
	mov	a, track.curKey+x
	clrc
	adc	a, track.transpose+x
	push	a
	call	CalcScale
	movw	lPitch, ya
	pop	a
	call	CalcScaleDiff
	movw	lPitchDif, ya
	pop	x

	/****************************************/
	/* ピッチベンド処理                     */
	/****************************************/
	mov	a, track.pitchBendSpan+x
	beq	_Detune
	mov	a, track.pitchBendDelay+x
	bne	_Detune
	mov	$04, lPitchDif+1
	mov	a, lPitchDif
	mov	y, a
	mov	a, track.pitchBendPhase+x
	call	mul16_8
	addw	ya, lPitch
	movw	lPitch, ya

	/****************************************/
	/* デチューン値の適用                   */
	/****************************************/
_Detune:
	mov	$04, lPitchDif+1
	mov	a, lPitchDif
	mov	y, a
	mov	a, track.detune+x
	mov	lTemp, a
	bpl	+
	eor	a, #$ff
	inc	a
+	call	mul16_8
	bbc	lTemp.7, +
	movw	lTemp, ya
	movw	ya, ZERO
	subw	ya, lTemp
+	addw	ya, lPitch
	movw	lPitch, ya

	/****************************************/
	/* Vol LFO                              */
	/****************************************/
	mov	a, track.tremoloDepth+x
	beq	_CalcVol1
	mov	a, track.tremoloWaits+x
	bne	_CalcVol1
_Tremolo:
	mov	a, track.tremoloPhase+x
	asl	a
	bcc	+
	eor	a, #$ff
+	mov	y, a
	mov	a, track.tremoloDepth+x
	mul	ya
	mov	a, y
	eor	a, #$ff
	mov	y, a
	mov	a, track.velocity+x
	mul	ya
	mov	a, y
	bra	_CalcVol

	/******************************/
	/* Volumeを計算する           */
	/******************************/
_CalcVol1:
	mov	a, track.velocity+x
_CalcVol:
	mov	lTemp, a
	mov	a, track.panH+x
	mov	lpan, a
	; システムフラグのモノラルビットをチェック
	; このbitがONの場合、モノラル化する
	mov1	c, musicSysFlags.2	; pampot off?
	bcc	_Panpot
	mov	a, lTemp
	lsr	a
	mov	lrvol, a
	mov	llvol, a
	bra	_VolLevel

_Panpot:
	and	a, #$3f
	cmp	a, #$3f
	bne	+
	mov	a, lTemp
	bra	++
+	mov	y, a
	mov	a, #0
	push	x
	mov	x, #$3f
	div	ya, x
	pop	x
	mov	y, lTemp
	mul	ya
	mov	a, y
++	mov	llvol, a
	mov	a, lTemp
	setc
	sbc	a, llvol
	mov	lrvol, a

_VolLevel:
	; --- right
	mov	y, a
	inc	y
	mov	a, track.volumeH+x
	mul	ya
	inc	y
	cmp     x, #MUSICTRACKS
	bmi	+
	mov	a, seGlobalVolume
	bra	++
+	mov	a, musicGlobalVolume
++	mul	ya
	mov	lrvol, y

	; --- left
	mov	a, llvol
	mov	y, a
	inc	y
	mov	a, track.volumeH+x
	mul	ya
	inc	y
	cmp     x, #MUSICTRACKS
	bmi	+
	mov	a, seGlobalVolume
	bra	++
+	mov	a, musicGlobalVolume
++	mul	ya
	mov	llvol, y

	/******************************/
	/* Pitchモジュレーション      */
	/******************************/
	mov	a, track.modulationDepth+x
	beq	_NoModuration
	mov	a, track.modulationWaits+x
	bne	_NoModuration

_Modulation:
	mov	a, track.modulationPhase+x
	mov	lTemp, a
	mov	lTemp+1, a
	and	a, #$c0
	beq	+
	cmp	a, #$80
	beq	+
	eor	lTemp+1, #$3f
+	and	lTemp+1, #$3f
	push	x
	; --- 振動幅を取得する
	mov	a, track.curKey+x
	clrc
	adc	a, track.transpose+x
	;call	CalcScaleDiff
	call	CalcScale
	pop	x
	mov	$04, a
	mov	a, y
	mov	$05, a
	mov	a, track.modulationDepth+x	; moduration
	mov	y, a
	mov	a, lTemp+1			; Phase
	mul	ya
	call	mul16_16

	bbc	lTemp.7, +
	movw	lTemp, ya
	movw	ya, ZERO
	subw	ya, lTemp
+	addw	ya, lPitch
	movw	lPitch, ya

_NoModuration:
	/******************************/
	/* ピッチ倍率補正             */
	/******************************/
	mov	y, lPitch
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

	pop	x
	/******************************/
	/* Pitch値をチャンネルに反映  */
	/******************************/
	mov	a, lPitch
	mov	buf_chData.pitch+x, a
	mov	a, lPitch+1
	mov	buf_chData.pitch+1+x, a

	/******************************/
	/* 逆位相設定、および         */
	/* ポーズbitがON時の音量半減  */
	/* の処理を行った後に、       */
	/* Volume値をチャンネルに反映 */
	/******************************/
	; 左音量
	mov	a, llvol
	mov1    c, musicSysFlags.0	; ポーズbit
	bcc     +
	lsr     a
+	mov1	c, lpan.7
	bcc	+
	eor	a, #$ff
	inc	a
+	mov	buf_chData.lvol+x, a
	; 右音量
	mov	a, lrvol
	mov1    c, musicSysFlags.0	; ポーズbit
	bcc     +
	lsr     a
+	mov1	c, lpan.6
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

	/****************************************/
	/* エコー音量の算出                     */
	/****************************************/
+	mov	SPC_REGADDR, #DSP_MVOLL
	mov	y, SPC_REGDATA
	mov	a, eVolRatio
	asl	a
	bcc	+
	eor	a, #$ff
	inc	a
+	mul	ya
	mov	a, y
	bcc	+
	eor	a, #$ff
	inc	a
+	mov	buf_eVol, a

	/****************************************/
	/* エコー音量が0の場合、エコー無効にする*/
	/****************************************/
	mov	SPC_REGADDR, #DSP_FLG
	mov	a, SPC_REGDATA
	and	a, #(FLG_ECEN ~ $ff)
	mov	y, buf_eVol
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
.undefine		lpan
.undefine		lrvol
.undefine		llvol

.ends

.section "CHRELEASE" free
;--------------------------------------------------
; DspReleaseMuteChannel - DSP REGISTER CONTROLL PROCESS
;   Level       : 2 (Main sub)
;   Input       : none
;   Output      : none
;   Description : DSP制御処理3
;                 音の鳴っていないチャンネルを開放します
;--------------------------------------------------
;------------------------------
; local values
;------------------------------

DspReleaseMuteChannel:
	mov     x, #0
	mov	y, #0
	mov	a, #DSP_ENVX
_DspReleaseLoop:
-	mov	SPC_REGADDR, a			; ChのENVXアドレスをセット

	; --- キーオン対象のチャンネルはチェック対象から除外
	mov	a, buf_keyon
	and	a, !ChBitTable+x
	bne	_SkipCh

	; --- チャンネルで使用しているトラックが、
	;     Releaseモードになっているかチェック
	mov	a, !buf_chData.allocTrack+y
	bmi	_SkipCh
	push	x
	mov	x, a
	mov	a, track.releaseTiming+x
	pop	x
	bne	_SkipCh

	; ENVX が 0かどうかをチェック
	; ENVXが0ではないなら音が鳴っているので、
	; チャンネル開放の対象から外す
	mov	a, SPC_REGDATA
	bne	_SkipCh

	; チャンネルを開放する
	mov     a, !ChBitTable+x
	or      a, buf_keyoff
	mov     buf_keyoff, a			; 音が鳴っていないのであまり意味はないが、KEYOFFする
	mov	a, #$ff				; 0xff = 割り当てTrackなし
	mov	buf_chData.allocTrack+y, a

	; ループ前処理
_SkipCh:
	mov	a, y
	clrc
	adc	a, #_sizeof_stChannel
	mov	y, a
	inc     x
	mov     a, x
	or      a, #(DSP_ENVX << 4)
	xcn	a
	bpl	_DspReleaseLoop			; 8CH分ループ
	ret
;------------------------------
; local values undefine
;------------------------------

.ends

