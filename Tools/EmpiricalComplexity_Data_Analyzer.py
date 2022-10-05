#!/bin/python3

import sys
import subprocess
import csv
import numpy
import matplotlib
import matplotlib.pyplot


# Run the configuration generation then NonoMain to make
# the data. Generator is not perfect in One-black Colourable
# One-Pixel SSC generation so there has to be check from NonoMain
# to retry if perfection problems.
datacollection=True;
SamplesPerConf=10;
MaxNumberOfSizes=10;
if len(sys.argv)<2:
	maxconfi=10;
else:
	for arg in sys.argv[1:]:
		if(arg[0]=='-'):
			datacollection=False;
		else:
			maxconfi=int(arg);
			break;

if datacollection:
	# Open the configuration and create
	# the configuration file.
	nonogenpath="/tmp/nonogen.conf"
	with open(nonogenpath,'w') as filestdout:
		for confi in range(0,maxconfi):
			
			# F***ing lack of goto statement. 
			retry=True
			while retry:
				retry=False
				
				filestdout.seek(0);
				proc=subprocess.run(["./NonoConfGen.exe","-g","One-Black-Colourable-One-Pixel-SSC-Gen","-n",str(2+confi)],stdout=filestdout,stderr=subprocess.PIPE);
				# Generate a Nextto SSC onfiguration generator
				if proc.returncode!=0:
					print("Error: NonoConfGen! "+str(proc.returncode));
					print(proc.stderr.decode());
					exit(1);
				filestdout.truncate();
				
				for resample in range(0,SamplesPerConf):
					print("Sample Info: "+str(confi)+","+str(resample));
					proc=subprocess.run(["./NonoMain.exe","-I","-p","-e",nonogenpath]);
					if proc.returncode!=0:
						if proc.returncode==100:
							# More just in case.
							print("Retry configuration generation.");
							retry=True
							break;
							if proc.returncode==-11:
								print("NonoMain segfaulted for some reason.")
								retry=True
								break;
						print("Error: NonoMain!");
						exit(1);
			
			# Generate configuration for with multiple switching components.
			# compnum loop is ment for generating different amounts of switching
			# components per size.
			for compnum in range(0,MaxNumberOfSizes):
				retry=True
				while retry:
					retry=False;
					filestdout.seek(0);
					proc=subprocess.run(["./NonoConfGen.exe","-g","VLConcatenate","-n",str(2+confi),"-m",str(2+compnum)],stdout=filestdout,stderr=subprocess.PIPE);
					# Generate a Nextto SSC onfiguration generator
					if proc.returncode!=0:
						print("Error: NonoConfGen! "+str(proc.returncode));
						print(proc.stderr.decode());
						exit(1);
					filestdout.truncate();
					
					for resample in range(0,SamplesPerConf):
						print("Sample Info: "+str(confi)+","+str(compnum)+","+str(resample));
						proc=subprocess.run(["./NonoMain.exe","-I","-p","-e",nonogenpath]);
						if proc.returncode!=0:
							if proc.returncode==100:
								# More just in case.
								print("Retry configuration generation.");
								retry=True
								break;
							if proc.returncode==-11:
								print("NonoMain segfaulted for some reason.")
								retry=True
								break;
							print("Error: NonoMain!");
							exit(1);

		filestdout.close();

# Load data 0 -> the nm complexity timings
data0=[];
with open("/tmp/data.0.txt",newline='') as csvfile:
	reader=csv.reader(csvfile,delimiter=' ',quotechar='|')
	for row in reader:
		data0.append([int(col) for col in row]);

# Take avarage of the SamplesPerConf sample sets
data0avg=[[],[],[],[]]
for confi in range(0,len(data0),SamplesPerConf):
	absc=data0[confi][0];
	col1=0;
	col2=0;
	col3=0;
	for s in data0[(confi):(confi+SamplesPerConf)]:
		col1+=s[1];
		col2+=s[2];
		col3+=s[3];
	data0avg[0].append(absc);
	data0avg[1].append(col1/SamplesPerConf);
	data0avg[2].append(col2/SamplesPerConf);
	data0avg[3].append(col3/SamplesPerConf);

# Load data 1 -> the number of switching components complexity timings
data1=[[],[]];
with open("/tmp/data.1.txt",newline='') as csvfile:
	reader=csv.reader(csvfile,delimiter=' ',quotechar='|')
	for row in reader:
		data1[0].append(int(row[0]));
		data1[1].append(int(row[1]));

# Generate some comparasion curves.
#curve2powanscmax=33;
#curve2powansc=list(range(1,curve2powanscmax+1));
#curves2pow=[2];
#i=1;
#while i<curve2powanscmax:
#	curves2pow.append(2*curves2pow[i-1]);
#	i+=1;

#curve4degreeanscmax=350;
#curve4degreeans=list(range(1,curve4degreeanscmax+1,2));
#curve4degree=[];
#for absc in curve4degreeans:
#	curve4degree.append(absc**4);

curve3degreeabscmax=3200;
curve3degreeabsc=list(range(1,curve3degreeabscmax));
curve3degree=[];
for absc in curve3degreeabsc:
	curve3degree.append(absc**3)

curve2degreeabscmax=180000;
curve2degreeabsc=list(range(1,curve2degreeabscmax+1,2));
curve2degree=[];
for absc in curve2degreeabsc:
	curve2degree.append(absc**2);

curve2degreelessabscmax=840000;
curve2degreelessabsc=curve2degreeabsc+list(range(curve2degreeabscmax+1,curve2degreelessabscmax));
curve2degreeless=[];
for absc in curve2degreelessabsc:
	curve2degreeless.append(0.045*(absc**2));

curvelinearabscmax=1000000;
curvelinearabsc=list(range(1,curvelinearabscmax+1,10));

curvelinearadjs=[];
i=0;
for x in curvelinearabsc:
	curvelinearadjs.append(7*x);

# data0 first column
fig0,ax0=matplotlib.pyplot.subplots();
ax0.xaxis.set_major_locator(matplotlib.ticker.MaxNLocator(integer=True))
ax0.xaxis.set_label_text("size of the nonogram (nm)");
ax0.yaxis.set_label_text("Nano seconds (ns)");
#ax0.yaxis.set_major_locator(matplotlib.ticker.MaxNLocator(integer=True))
#ax0.plot(curve2powansc,curves2pow,'-',color="#0f0f0fff");
#ax0.plot(curve4degreeans,curve4degree,'r-');
ax0.plot(curve3degreeabsc,curve3degree,'-',color="#FF000090",label="n³m³");
ax0.plot(curve2degreeabsc,curve2degree,'-',color="#00FF0090",label="n²m²");
ax0.plot(curvelinearabsc,curvelinearabsc,'-',color="#0000FF90",label="nm");
ax0.plot(curve2degreelessabsc,curve2degreeless,'y-',label="0.045n²m²");
ax0.legend(loc=9);
ax0.plot(data0avg[0],data0avg[1],'b*');

fig1,ax1=matplotlib.pyplot.subplots();
ax1.xaxis.set_major_locator(matplotlib.ticker.MaxNLocator(integer=True))
ax1.xaxis.set_label_text("size of the nonogram (nm)");
ax1.yaxis.set_label_text("Nano seconds (ns)");
ax1.plot(curve3degreeabsc,curve3degree,'-',color="#FF000090",label="n³m³");
ax1.plot(curve2degreeabsc,curve2degree,'-',color="#00FF0090",label="n²m²");
ax1.plot(curvelinearabsc,curvelinearabsc,'-',color="#0000FF90",label="nm");
ax1.plot(curve2degreelessabsc,curve2degreeless,'y-',label="0.045n²m²");
ax1.legend(loc=9);
ax1.plot(data0avg[0],data0avg[2],'b*');

fig2,ax2=matplotlib.pyplot.subplots();
ax2.xaxis.set_major_locator(matplotlib.ticker.MaxNLocator(integer=True))
ax2.xaxis.set_label_text("size of the nonogram (nm)");
ax2.yaxis.set_label_text("Nano seconds (ns)");
ax2.plot(curvelinearabsc,curvelinearabsc,'-',color="#0000FF90",label="nm");
ax2.plot(curvelinearabsc,curvelinearadjs,'y-',label="7nm");
ax2.legend(loc=2);
ax2.plot(data0avg[0],data0avg[3],'*',color="blue");

fig3,ax3=matplotlib.pyplot.subplots();
ax3.xaxis.set_major_locator(matplotlib.ticker.MaxNLocator(integer=True));
ax3.yaxis.set_major_locator(matplotlib.ticker.MaxNLocator(integer=True));
ax3.xaxis.set_label_text("Number of pixels in a switching component.");
ax3.yaxis.set_label_text("Nano seconds (ns)");
#ax3.plot(curve2powansc[1:14],curves2pow[1:14],'r-');
#ax3.plot(curve3degreeabsc[1:39],curve3degree[1:39],'-',color="#FF000090");
ax3.plot(curvelinearabsc[1:250],curvelinearabsc[1:250],'-',color="#0000FF90",label="x");
ax3.plot(curve2degreeabsc[1:155],curve2degree[1:155],'-',color="#00FF0090",label="x²");
ax3.legend(loc=1)
ax3.plot(data1[0],data1[1],'b*');

matplotlib.pyplot.show();

