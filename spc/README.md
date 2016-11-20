# SPC Sources

These source files require [This WLA DX](https://github.com/boldowa/wla-dx)\(spc-sfx\_dev branch version\).  
If you want to make spc code, please setup it.

And type "make" to generate the binary(SuperC.bin).  
Or, type "make spc" to generate the spc for test.

## Description of sources

### main.s

This is the entry point of the SPC driver.

### io.s

This program controls I/O communication with SNES-CPU.

### dsp.s

This program writes data to the DSP register.  
And calculate the value for write to DSP register. (Pitch, Volume, Key-On etc...)

### music.s

This program analysis the music sequence data.

### se.s

This program analysis the music data for sound effects.  
This program uses the routine in **music.s**.

### seqcmd.s

This program calls music command routines.

### libSuperC.s

This is the libraly codes for SuperC SPC driver.  
This libraly has 16bits multiple routine, 16bits division routine etc...

### brr.s

This defines the table of BRR data.  
And generates special wave.

### seaq\_data.s

This includes sequence data for test spc.


## Music data specification

### Music data location

ARAM:*seqBaseAddress(0x0028)* holds the music data location.

### Structure of music data

|structure|
|:-:|
|**Track data location table**<br>2 \* 12 bytes|
|**Drum tone table**<br>9 \* 7 bytes|
|**Music tone table**<br>6 \* n bytes|
|**Track 1 data**<br>n bytes|
|**Track 2 data**<br>n bytes|
|**Track 3 data**<br>n bytes|
|** : **|
|**Track 12 data**<br>n bytes|
|**Sub-routine data**<br>n bytes|

#### Track data location table
Specify the position of the track data in relative position from seqBaseAddress.  
For example, **Track1 location** is *0x007B*, and **seqBaseAddress** is *0xB0B3*, Track1 sequence data is allocated to *0x007B + 0xB0B3 = 0xB12E*.

#### Drum tone table
Specify the drum note settings.  
This data has 9bytes length per 1 entry.

|byte|description|
|:--|:--|
|1|BRR index|
|2|Pitch ratio|
|3|Tuning|
|4|ADSR(1)|
|5|ADSR(2) or GAIN|
|6|ADSR(2) or GAIN for Release|
|7|Gatetime and Velocity|
|8|Panpot|
|9|Height of sound|

#### Music tone table
Specify the music tone settings for the **CMD_SET_INST** command.  
This data has 6bytes length per 1 entry.

|byte|description|
|:--|:--|
|1|BRR index|
|2|Pitch ratio|
|3|Tuning|
|4|ADSR(1)|
|5|ADSR(2) or GAIN|
|6|ADSR(2) or GAIN for Release|

#### Track and Sub-routine data
Music sequence data is stored.

### Music sequence data specification

|Range|Decription|
|:--|:--|
|0x00|End of track data|
|0x01 - 0x7F|Note length / Gatetime and Velocity|
|0x80 - 0xD3|Note|
|0xD4|Tie|
|0xD5|Rest|
|0xD6 - 0xDC|Drum note|
|0xDD - 0xFF|Commands|

#### End of track data
This is the end of music data when not looping.

#### Note length / Gatetime and Velocity
Refer [Note](#Note)

#### Note
This is the music note.

|\|c|c+|d|d+|e|f|f+|g|g+|a|a+|b|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|o0|0x80|0x81|0x82|0x83|0x84|0x85|0x86|0x87|0x88|0x89|0x8A|0x8B|
|o1|0x8C|0x8D|0x8E|0x8F|0x90|0x91|0x92|0x93|0x94|0x95|0x96|0x97|
|o2|0x98|0x99|0x9A|0x9B|0x9C|0x9D|0x9E|0x9F|0xA0|0xA1|0xA2|0xA3|
|o3|0xA4|0xA5|0xA6|0xA7|0xA8|0xA9|0xAA|0xAB|0xAC|0xAD|0xAE|0xAF|
|o4|0xB0|0xB1|0xB2|0xB3|0xB4|0xB5|0xB6|0xB7|0xB8|0xB9|0xBA|0xBB|
|o5|0xBC|0xBD|0xBE|0xBF|0xC0|0xC1|0xC2|0xC3|0xC4|0xC5|0xC6|0xC7|
|o6|0xC8|0xC9|0xCA|0xCB|0xCC|0xCD|0xCE|0xCF|0xD0|0xD1|0xD2|0xD3|

###### Note input format

	[Note Length(Optional)] [Gatetime and Velocity(Optional)] <Note>

For example, if you want to be `q8k16o1g4g4g4g4`, do this.
	
	0x30			; length = 48ticks
	0x7F			; Gatetime = 0x7(q8), Velocity = 0xF(k16)
	0x93 0x93 0x93 0x93	; g g g g

If you want to specify Gatetime and Velocity, you must enter the note length.

#### Tie
Continue playing the previous note.

Input format is the same as Note.

#### Rest
It will key off.

Input format is the same as Note.

#### Drum note
It plays drum tone.

This note overwrites the following values with the value of the drum table.

- Gatetime
- Velocity
- Panpot

#### Commands

- CMD\_SET\_INST
- CMD\_VOLUME
- CMD\_PAN
- CMD\_JUMP
- CMD\_TEMPO
- CMD\_GVOLUME
- CMD\_ECHO\_ON
- CMD\_ECHO\_OFF
- CMD\_ECHO\_PARAM
- CMD\_ECHO\_FIR
- CMD\_PORTAM\_ON
- CMD\_PORTAM\_OFF
- CMD\_MODURATION
- CMD\_MODURATION\_OFF
- CMD\_TREMOLO
- CMD\_TREMOLO\_OFF
- CMD\_SUBROUTINE
- CMD\_SUBROUTINE\_RETURN
- CMD\_SUBROUTINE\_BREAK
- CMD\_PITCHBEND
- CMD\_TEMPO\_FADE
- CMD\_VOLUME\_FADE
- CMD\_GVOLUME\_FADE
- CMD\_TRANSPOSE
- CMD\_REL\_TRANSPOSE
- CMD\_PAN\_FADE
- CMD\_PAN\_VIBRATION
- CMD\_PAN\_VIBRATION\_OFF
- CMD\_HARDPM\_ON
- CMD\_HARDPM\_OFF
- CMD\_PITCH\_ENVELOPE
- CMD\_PITCH\_ENVELOPE\_OFF

