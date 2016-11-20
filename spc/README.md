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
Specify the drum note settings.  
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

