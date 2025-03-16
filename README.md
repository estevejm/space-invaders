# Space Invaders

Intel 8080 Space Invaders arcade emulator written in C

## Usage
```sh
cmake --build . 
./bin/spaceinvaders
```
Sample output
```
00 c3 d4                                                   // peek next 3 bytes after PC 
~ r 0002 00                                                // memory access: read / write + address + data
NOP                                                        // decoded instruction
····················                                       // CPU state after executing decoded instruction,
A|00 F|00  S Z A P C                                       // including registers, flags, SP and PC
B|00 C|00  0 0 0 0 0
D|00 E|00  SP | 0000
H|00 L|00  PC | 0003
····················
fff0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00     // 16 bytes of stack space, '|' marks SP position
~~~~~~~~~~~~~~~~~~~~                                       // next instruction separator
c3 d4 18
~ r 0003 c3
~ r 0004 d4
~ r 0005 18
JMP 18d4
····················
A|00 F|00  S Z A P C
B|00 C|00  0 0 0 0 0
D|00 E|00  SP | 0000
H|00 L|00  PC | 18d4
····················
fff0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
~~~~~~~~~~~~~~~~~~~~
31 00 24
~ r 18d4 31
~ r 18d5 00
~ r 18d6 24
LXI SP,2400
····················
A|00 F|00  S Z A P C
B|00 C|00  0 0 0 0 0
D|00 E|00  SP | 2400
H|00 L|00  PC | 18d7
····················
23f0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
~~~~~~~~~~~~~~~~~~~~
06 00 cd
~ r 18d7 06
~ r 18d8 00
MVI B,00
····················
A|00 F|00  S Z A P C
B|00 C|00  0 0 0 0 0
D|00 E|00  SP | 2400
H|00 L|00  PC | 18d9
····················
23f0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
~~~~~~~~~~~~~~~~~~~~
cd e6 01
~ r 18d9 cd
~ r 18da e6
~ r 18db 01
CALL 01e6
····················
~ w 23ff 18
~ w 23fe dc
A|00 F|00  S Z A P C
B|00 C|00  0 0 0 0 0
D|00 E|00  SP | 23fe
H|00 L|00  PC | 01e6
····················
23f0  00 00 00 00 00 00 00 00  00 00 00 00 00 00|dc 18
~~~~~~~~~~~~~~~~~~~~
```

## Resources
Extracted from `/r/EmuDev/` discord

### 8080
* [Assembly programming manual](https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf)
* [Datasheet](https://altairclone.com/downloads/manuals/8080%20Data%20Sheet.pdf)
* [Instruction set](https://tobiasvl.github.io/optable/intel-8080/classic) [(alternative)](https://pastraiser.com/cpu/i8080/i8080_opcodes.html)

### Space Invaders
* [SN76477N technical data](https://web.archive.org/web/20150425030455/http://www.emutalk.net/attachment.php?attachmentid=34143&d=1160668005)
* [General info on Space Invaders](http://www.brentradio.com/SpaceInvaders.htm)
* [Space Invaders disassembly and info](https://computerarcheology.com/Arcade/SpaceInvaders)
* [CPU tests](https://altairclone.com/downloads/cpu_tests/) (needs a CP/M implementation or to fake it to some extent, see CP/M section)

### CP/M
* [General CP/M info](https://en.m.wikipedia.org/wiki/CP/M)
* [Zero page breakdown](https://en.m.wikipedia.org/wiki/Zero_page_(CP/M))
* [CP/M BDOS system calls](https://www.seasip.info/Cpm/bdos.html)
* [Discord message](https://discord.com/channels/465585922579103744/482208284032499713/1340790514252779650)
