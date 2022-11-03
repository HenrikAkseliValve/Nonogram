
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

To apply intialization for solving or solver use `-I` option. For example, will show same nonogram as before with inital block ranges and some black pixels:
```bash
./NonoMain.exe -s -I /NonogramConfs/Logical_Solver_Test2.cfg > /tmp/nono.html
$BROWSER /tmp/nono.html
```
![](Doc/resources/readme-image2.svg)

To manually apply a rule first apply initialization with `-I` option then apply rule via `-r`. For example,
to add white pixel to row 3 and column 5, apply rule 3.1 first to refine block ranges and then colour
white pixels by rule 1.2:
```bash
./NonoMain.exe -s -I -r3.1r3 -r3.1c4 -r1.2r3 -r1.2c4 NonogramConfs/Logical_Solver_Test2.cfg > /tmp/nono.html
$BROWSER /tmp/nono.html
```
![](Doc/resources/readme-image3.svg)

To partially solve the image use `-p` option. For example a full solution of "Logical_Solver_Test2.cfg" can be gotten:
```bash
./NonoMain.exe -s -I -p NonogramConfs/Logical_Solver_Test2.cfg > /tmp/nono.html
$BROWSER /tmp/nono.html
```
![](Doc/resources/readme-image4.svg)

Other example which shows a partial solution rather than full one is "Logical_Solver":
```bash
./NonoMain.exe -s -I -p NonogramConfs/ElementarySwitch2.cfg > /tmp/nono.html
$BROWSER /tmp/nono.html
```
![](Doc/resources/readme-image5.svg)
