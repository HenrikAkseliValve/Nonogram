
# NONOGRAM TOOL

Two programs:
* _NonoMain.exe_ program for operating on a nonogram.
* _NonoConfGen.exe_ program for creating a nonogram configuration files. 

## HOW TO COMPILE
Operating system being Linux and compiler being GCC are assumed.

To build both programs with debug information:
```bash
make all
```

To build only _NonoMain.exe_ with debug information use:
```bash
make NonoMain.exe
```

To build only _NonoConfGen.exe_ with debug information use:
```bash
make NonoConfGen.exe
```

Oprimized binary is created setting DEBUG=0. For example, compile both programs optimized:
```bash
make all DEBUG=0
```

Makefile supports `clean` target.

Makefile has **NOT** implemented `install` target.

## HOW TO USE EXAMPLES
Both programs have help text which can be viewed with `-h` option:
```bash
./NonoMain.exe -h
./NonoConfGen.exe -h
```

To visualize a nonogram use `-s` option with _Nonomain.exe_. Program outputs stdout HTML with SVG image of Nonogram which can be opened by a web browser: 
```bash
./NonoMain.exe -s /NonogramConfs/Logical_Solver_Test2.cfg > /tmp/nono.html
$BROWSER /tmp/nono.html
```
![](Doc/resources/readme-image1.svg)

To manually apply a rule first apply initialization with `-I` option then apply rule via `-r`. For example, to add some pixels at bottom : 
```bash
./NonoMain.exe -s -I -r3.1r3 -r3.1c4 -r1.2r3 -r1.2c4 NonogramConfs/Logical_Solver_Test2.cfg > /tmp/nono.html
$BROWSER /tmp/nono.html
```

