
# NONOGRAM TOOL

Two programs:
* _NonoMain.exe_ program for operating on a nonogram.
* _NonoConfGen.exe_ program for creating a nonogram configuration files. 

## HOW TO COMPILE
Environment requirements:
* Linux operating system
* GCC (preferred) or CLANG compiler

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

Optimized binary is created setting `DEBUG=0`.

To get timing information out of `NonoMain.exe` profiling must be turned on by `PROFILE=1`.
Timing information goes into two files:
* "/tmp/data.0.txt" has size of the nonogram, total time spend, time spend for the
partial solver, time spend on proposed switching component finder, and time spend on detecting
switching component type on same line.
* "/tmp/data.1.txt" has size of the switching component and time taken to detect one switching component
in multiple lines.
Rerunning the program appends to files. So reading "/tmp/data.1.txt" may become difficult if file is not
occasionally removed.
Profiling is meant to be used with "Tools/EmpiricalComplexity_Data_Analyzer.py" to get graphs on timing
as size of nonograms grow.

So optimized binary with profiling is:
```bash
make all DEBUG=0 PROFILE=1
```

Makefile supports `clean` target.

Makefile does **NOT** implemented `install` target.

## USE EXAMPLES
Both programs have help text which can be viewed with `-h` option:
```bash
./NonoMain.exe -h
./NonoConfGen.exe -h
```

To visualize a nonogram use `-s` option with _Nonomain.exe_. Program outputs HTML file with SVG image of
Nonogram to stdout. File can be opened by any web browser:
```bash
./NonoMain.exe -s NonogramConfs/Logical_Solver_Test2.cfg > /tmp/nono.html
$BROWSER /tmp/nono.html
```
![](Doc/resources/readme-image1.svg)

**NOTE**: If browser does not open the HTML files in `/tmp` then it is most like sandboxing issue. Easiest fix
is to save "nono.html" in the same folder as the program.

To apply intialization for solving use `-I` option. For example, following will show same
nonogram as before with inital block ranges and some black pixels:
```bash
./NonoMain.exe -s -I NonogramConfs/Logical_Solver_Test2.cfg > /tmp/nono.html
$BROWSER /tmp/nono.html
```
![](Doc/resources/readme-image2.svg)

To manually apply a rule, first apply initialization with `-I` option then apply rule via `-r` option. For
example, by applying rule 3.1 first to refine block ranges and then rule 1.2 to white pixels will add white
pixel to row 3 and column 4:
```bash
./NonoMain.exe -s -I -r3.1r3 -r3.1c4 -r1.2r3 -r1.2c4 NonogramConfs/Logical_Solver_Test2.cfg > /tmp/nono.html
$BROWSER /tmp/nono.html
```
![](Doc/resources/readme-image3.svg)

To partially solve the image use `-p` option. For example, the full solution of "Logical_Solver_Test2.cfg" can
be gotten:
```bash
./NonoMain.exe -s -I -p NonogramConfs/Logical_Solver_Test2.cfg > /tmp/nono.html
$BROWSER /tmp/nono.html
```
![](Doc/resources/readme-image4.svg)

Other example which shows a partial solution rather than a full one is "ElementarySwitch2.cfg":
```bash
./NonoMain.exe -s -I -p NonogramConfs/ElementarySwitch2.cfg > /tmp/nono.html
$BROWSER /tmp/nono.html
```
![](Doc/resources/readme-image5.svg)

To count number of solution use -e (meaning _estimate_) option. This option estimates number
of solution from switching components it recognized. Program counts number of proposed switching
components which are not recognized.  
For example "ElementarySwitch2.cfg":
```bash
./NonoMain.exe -e -I -p NonogramConfs/ElementarySwitch2.cfg
```
Output is:
```
Estimating number of solutions.
Estimated number of solutions: 4
Number of unknown proposed switching components: 0
```

Larger example with switching components which are not recognized
"one_black_colourable_one_pixel_SSC_and_non_SCC_test.cfg":
```bash
./NonoMain.exe -e -I -p  NonogramConfs/one_black_colourable_one_pixel_SSC_and_non_SCC_test.cfg
```
Output is:
```
Estimating number of solutions.
Estimated number of solutions: 2880
Number of unknown proposed switching components: 4
```

To generate a column of one-black colourable one-pixel square switching components use `-g VLConcatenate`
on _NonoConfGen.exe_. Options `-n` controls the most like size of the switching component generated and
`-m` control number of switching components in the column. Output from _NonoConfGen.exe_ is written to standard
output. For example, to generate column of four one-black colourable one-pixel square
switching components with most like size of 6 and counting number of switching components:
```
./NonoConfGen.exe -n6 -m4 -g VLConcatenate > /tmp/nono.conf
./NonoMain.exe -e -I -p /tmp/nono.conf
```
Output from this may include switching component that aren't recognized as generator is not perfect.
Number of solution counted also may differ as generator does random select the size of the switching
components in the column.

**NOTE**: _NonoMain.exe_ does not read UNIX pipes at the moment. 

## NONOGRAM CONFIGURATION FILE FORMAT
Nonogram configuraiton file does have four sections in it:
* **Comment heading**. Comment section which processing ignores. Comment line starts with '#' symbol.
* **Size section**. Telling the size of the nonogram in two lines ending to empty line. First of the
lines is the width of the nonogram, second is the height.
* **Column descriptions**. Width amount of lines defining column descriptions ending with empty line.
Format for the descriptions is &lt;number of blocks&gt;&lt;space&gt;&lt;block 0 length&gt;&lt;space&gt;...&lt;space&gt;&lt;block k length&gt;&lt;newline&gt;.
For example in C format: description of two blocks of sizes 1 and 7 is: "2 1 7\n".
* **Row descriptions** Height amount of lines defining rows descriptions. Uses same description format as columns.
Note that last line of the row descriptions needs the description formats newline.  

```
# I am a heading comment!
# My rows must start with '#' symbol but there can be any number of us.
# I am only comment that can exist in the file!
# AFTER ME must be only digits and newlines
5       <--- width of the nonogram
5       <--- height of the nonogram
        <--- Empty line
1 5     ⎫ 
2 2 2   ⎪ 
1 0     ⎬ Column descriptions
2 1 1   ⎪
2 1 2   ⎭
        <--- Empty line
1 2     ⎫
2 2 2   ⎪
1 1     ⎬ Row descriptions
2 2 2   ⎪
2 2 1   ⎭   Last row in the file also ends to newline!

```

