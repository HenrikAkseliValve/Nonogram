/*
* Solves given nonogram. Options include include SVG
* generation, every solution, and partial solution.
*/
#include<getopt.h>
#include<stdint.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<assert.h>
#include<sys/uio.h>
#include<ctype.h>
#include"Nonograms.h"
#include"LogicalRules.h"
#include"ValidityCheck.h"
#include"SwitchingComponent.h"
#include"Etc.h"
#ifdef _PROFILE_
	#include<time.h>
	#include"Profiling.h"
#endif


/*
* Structures for storing rule operations from the command line.
*/
enum OP{
	LOGICAL_RULE11=0,
	LOGICAL_RULE12,
	LOGICAL_RULE13,
	LOGICAL_RULE14,
	LOGICAL_RULE15,
	LOGICAL_RULE21,
	LOGICAL_RULE22,
	LOGICAL_RULE23,
	LOGICAL_RULE24,
	LOGICAL_RULE25,
	LOGICAL_RULE31,
	LOGICAL_RULE32,
	LOGICAL_RULE33,
	LOGICAL_RULE0,
	FULL_RUN_RULE,
	FORCE_COLOR,
};
struct Operation{
	struct{
		enum OP id:31;
		uint32_t line:1;
	};
	int32_t linenumber;
};
struct SingleRule{
	struct SingleRule *next;
	struct Operation op;
};
/*
* Structure to store forcefull coloring operations fromt he command line.
* Thrown with SingleRule operation's linked list.
*/
struct ForcefullyColour{
	struct SingleRule *next;
	struct{
		enum OP id:31;
		uint32_t check:1;
	};
	int32_t row;
	int32_t column;
	enum Pixel colour;
};
/*
* Select logical rule by id.
*/
int32_t (*const Rules[])(enum Pixel *line,int32_t length,uint32_t stride,Description *desc,BlocksRanges *ranges)={nonogramLogicalRule11,
                                                                                                                 nonogramLogicalRule12,
                                                                                                                 nonogramLogicalRule13,
                                                                                                                 nonogramLogicalRule14,
                                                                                                                 nonogramLogicalRule15,
                                                                                                                 nonogramLogicalRule21,
                                                                                                                 nonogramLogicalRule22,
                                                                                                                 nonogramLogicalRule23,
                                                                                                                 nonogramLogicalRule24,
                                                                                                                 nonogramLogicalRule25,
                                                                                                                 nonogramLogicalRule31,
                                                                                                                 nonogramLogicalRule32,
                                                                                                                 nonogramLogicalRule33,
                                                                                                                 nonogramLogicalRule0
};

/*
* Start of the solver program.
* Non-option argument is the configuration file to load.
* Option arguments:
*   -s output SVG.
*   -r#.#{c|r}# do one logical rule to ether columns (c) or rows (r).
*               Example rule 1.2 to column 4 -r1.2c4.
*   -f#,#{w|b} forcefully color pixel to white or black.
*              Checks is the pixel unknown before setting.
*              Example -f2,3w would mark column 2 and row 3 to white.
*   -F#,#{w|b} same as -f but no doesn't check pixel is unknown.
*   -I Run rule 0 and rule 1.1 every column and row.
*   -p solve partial solution.
*   -V{pre|range} do validity check.
*                 Pre means before any operation.
*                 Range means do sanity check that block's ranges haven't reduced passed each other.
*   -e estimate number of solutions from current state of nonogram's solution.
*   -h display this information.
*/
int main(int argc,char *argv[]){
	// Mode flags
	struct{
		uint8_t onerule:1;
		uint8_t solve:1;
		uint8_t dosvg:1;
		uint8_t dopartialsolution:1;
		uint8_t doprevaliditycheck:1;
		uint8_t doblockswapguard:1;
		uint8_t dosolutioncountingestimate:1;
		uint8_t writepartialsolution:1;
	}flags={0,0,0,0,0,0,0,0};
	// Write flags when writting SVG table.
	NonogramWriteSvgFlags writeflags=NONOGRAM_WRITE_SVG_FLAG_EMPTY;

	// Single rule operation list.
	struct SingleRule *rulelist=NULL;
	// Structure to store information about the nonogram.
	Nonogram nono;
	
	// Setup profiling if asked.
	#if 1//def _PROFILE_
		time_t rawtime;
		time(&rawtime);
		struct tm *processedtime;
		processedtime=localtime(&rawtime);
		char profbuffpath[]="/tmp/data_xx:xx:xx.0.txt";
		strftime(profbuffpath+10,8,"%d:%m:%y",processedtime);
		initProfiling(0,1,profbuffpath);
		profbuffpath[19]='1';
	#endif


	// Handle the options.
	{
		// Pointer to last member of list is to speed up
		// addition. Rulelistlast is initialized to address
		// of rulelist so to give fake root member to the list.
		// This works only if next pointer of SingleRule Structure
		// is first member of the structure. First member has
		// same memory address as the whole structure.
		struct SingleRule *rulelistlast=(struct SingleRule*)&rulelist;
		// Command line option storage.
		int opt;
		while((opt=getopt(argc,argv,"r:Ipsf:hF:V:e"))!=-1){
			switch(opt){
				case 's':
					flags.dosvg=1;
					break;
				case 'r':
					flags.onerule=1;

					// Parse what rule is used.
					if(optarg){
						// Allocate and initialize new one line rule.
						rulelistlast->next=malloc(sizeof(struct SingleRule));
						if(!rulelistlast->next) goto GETOPT_PARSING_ERROR;
						rulelistlast=rulelistlast->next;
						rulelistlast->next=NULL;
						// Handle major number of the rule.
						switch(optarg[0]){
							case '0':
								rulelistlast->op.id=LOGICAL_RULE0;
								break;
							case '1':
								rulelistlast->op.id=LOGICAL_RULE11;
								break;
							case '2':
								rulelistlast->op.id=LOGICAL_RULE21;
								break;
							case '3':
								rulelistlast->op.id=LOGICAL_RULE31;
								break;
							default:
								goto ONE_LINE_RULE_SYNTAX_ERROR;
						}
						// Handle minor number of the rule.
						int8_t offset=0;
						if(rulelistlast->op.id==LOGICAL_RULE0){
							if(optarg[1]=='.'){
								if(optarg[0]=='0'){
									offset+=2;
								}
								else goto ONE_LINE_RULE_SYNTAX_ERROR;
							}
						}
						else if(optarg[1]=='.'){
							int16_t minor=optarg[2]-'0'-1;
							// For set 1 1.5 is highest.
							// For set 2 and 3 x.3 is highest.
							if((rulelistlast->op.id==LOGICAL_RULE11 && minor<5) || (rulelistlast->op.id==LOGICAL_RULE21 && minor<5) || minor<3){
								rulelistlast->op.id+=minor;
							}
							else{
								ONE_LINE_RULE_SYNTAX_ERROR:
								(void)write(STDERR_FILENO,"ERROR: Rule doesn't exits!\n",27);
								goto GETOPT_PARSING_ERROR;
							}
							offset+=2;
						}
						else goto ONE_LINE_RULE_SYNTAX_ERROR;
						// Get the line direction and the line number.
						if(optarg[1+offset]=='r') rulelistlast->op.line=0;
						else if(optarg[1+offset]=='c') rulelistlast->op.line=1;
						else goto ONE_LINE_RULE_SYNTAX_ERROR;
						rulelistlast->op.linenumber=atoi(optarg+2+offset);
					}
					else{
						(void)write(STDERR_FILENO,"Argument not given\n",19);
					}
					break;
				case 'F':
				case 'f':{
					// User wants to forcefully color a pixel.
					int32_t pixelcolumn=atoi(optarg);
					// Read coordinates of the pixel.
					char *next;
					for(next=optarg;*next && isdigit(*next);next++);
					if(*next!=',') goto ERROR_FORCE_A_PIXEL;
					int32_t pixelrow=atoi(++next);
					while(*next && isdigit(*next)) next++;
					// Check that everything is fine uptill now.
					if(!(*next=='w' || *next=='b') || pixelcolumn<0 || pixelrow<0){
						ERROR_FORCE_A_PIXEL:
						(void)write(STDERR_FILENO,"ERROR: forcefull colouring of a pixel wasn't formated correctly!\n",65);
						goto GETOPT_PARSING_ERROR;
					}
					// Allocate link to rulelist.
					rulelistlast->next=malloc(sizeof(struct ForcefullyColour));
					if(!rulelistlast->next) goto GETOPT_PARSING_ERROR;
					rulelistlast=rulelistlast->next;
					rulelistlast->next=NULL;
					((struct ForcefullyColour*)rulelistlast)->id=FORCE_COLOR;
					((struct ForcefullyColour*)rulelistlast)->row=pixelrow;
					((struct ForcefullyColour*)rulelistlast)->column=pixelcolumn;
					((struct ForcefullyColour*)rulelistlast)->colour=(*next=='b'?BLACK_PIXEL:WHITE_PIXEL);
					((struct ForcefullyColour*)rulelistlast)->check=(opt=='f'?1:0);
					flags.onerule=1;
					break;
				}
				case 'I':{
					// Add the operations.
					const enum OP opstoadd[]={LOGICAL_RULE0,LOGICAL_RULE0,LOGICAL_RULE11,LOGICAL_RULE11};
					const uint32_t lines[]={0,1,0,1};
					for(int i=0;i<sizeof(opstoadd)/sizeof(opstoadd[0]);i++){
						rulelistlast->next=malloc(sizeof(struct SingleRule));
						if(!rulelistlast->next) goto GETOPT_PARSING_ERROR;
						rulelistlast=rulelistlast->next;
						rulelistlast->next=NULL;

						rulelistlast->op.id=FULL_RUN_RULE;
						rulelistlast->op.line=lines[i];
						rulelistlast->op.linenumber=(int32_t)opstoadd[i];
					}
					flags.onerule=1;
					break;
				}
				case 'p':
					flags.dopartialsolution=1;
					break;
				case 'V':
					if(strcmp(optarg,"pre")==0){
						flags.doprevaliditycheck=1;
					}
					else if(strcmp(optarg,"range")==0){
						flags.doblockswapguard=1;
					}
					else{
						(void)write(STDOUT_FILENO,"ERROR: not proper argument for -V option!\n",42);
						goto GETOPT_PARSING_ERROR;
					}
					break;
				case 'e':
					flags.dosolutioncountingestimate=1;
					break;
				case 'h':{
					const char usage[]="Usage: NonoSolver.exe [options] cfgfile"
					                    "Options:\n"
					                    "\t-s output SVG.\n"
					                    "\t-r#.#{c|r}# do one logical rule to ether columns (c) or rows (r).\n"
					                    "\t            Example rule 1.2 to column 4 -r1.2c4.\n"
					                    "\t-f#,#{w|b} forcefully color pixel to white or black. Gives error if pixel is already coloured.\n"
					                    "\t           Example -f2,3w would mark column 2 and row 3 to white.\n"
					                    "\t-F#,#{w|b} is same as -f but does not give error if pixel already has colour.\n"
					                    "\t-I Run rule 0 and rule 1.1 every column and row.\n"
					                    "\t-p solve partial solution.\n"
					                    "\t-e estimate number of solutions from current state of nonogram's solution.\n"
					                    "\t-V{v} do validity check at point argument asks.\n"
					                    "\t      With pre means before any operation.\n"
					                    "\t      With range means do sanity check that block's ranges haven't reduced passed each other after a (partial) solver.\n"
					                    "\t-h display this information.\n";
					(void)write(STDOUT_FILENO,usage,sizeof(usage)-1);
					break;
				}
				case ':':
				case '?':
					goto GETOPT_PARSING_ERROR;
			}
		}
	}

	// Get the config location.
	if(optind<argc){
		int file=open(argv[optind],O_RDONLY);
		if(file>=0){
			struct NonogramLoadConfResult result=nonogramLoadConf(file,&nono);
			if(result.id==0){
				
				NonoHTML htmlpage;

				TableHead tables;
				if(nonogramInitTableHead(&nono,&tables)){

					if(flags.dosvg){
						// Make the Nonogram SVG template.
						if(nonogramGenSvgStart(STDOUT_FILENO,&nono,&htmlpage)){
							(void)write(STDERR_FILENO,"ERROR: SVG template!\n",21);
							goto ERROR_AT_NONOGRAMGENSVG;
						}
					}

					if(flags.doprevaliditycheck){
						// Do check of validity to nonogram before solving.
						// Validity check hence is only total description
						// count check and description length check.
						if(nonogramIsValid(&nono)!=0){
							(void)write(STDERR_FILENO,"Warning: Precheck detected non-valid nonogram!\n",47);
						}
						else{
							(void)write(STDERR_FILENO,"Note: Nonogram is valid!\n",25);
						}
					}

					if(flags.onerule || flags.dopartialsolution){
						// Initilize block's ranges stored in first table.
						for(int32_t col=nono.width-1;col>=0;col--){
							initBlocksRanges(tables.firsttable.col+col,nono.colsdesc+col,nono.height);
						}
						for(int32_t row=nono.height-1;row>=0;row--){
							initBlocksRanges(tables.firsttable.row+row,nono.rowsdesc+row,nono.width);
						}
						writeflags|=NONOGRAM_WRITE_SVG_FLAG_WRITE_BLOCKS_RANGES;
					}

					// Solve the nonogram until no line can be solved futher
					// or apply one rule.
					if(flags.onerule){

						// Go through the list of rules to perform.
						while(rulelist){

							// Grap Operation structure from rulelist.
							struct Operation ruleop=rulelist->op;

							// Calculate stride and line.
							uint32_t stride=ruleop.line*nono.width+(!ruleop.line);
							int32_t length=ruleop.line*nono.height+(!ruleop.line)*nono.width;
							static_assert((&nono.rowsdesc)+1==&nono.colsdesc,"Alignment doesn't allow calculate addressing!\n");

							if(ruleop.id<FULL_RUN_RULE){

								// Since this comes from user safety check that line even exists!
								if(ruleop.linenumber<ruleop.line*nono.width+(!ruleop.line)*nono.height){

									enum Pixel *line=getLine(&nono,ruleop.line,ruleop.linenumber,&tables.firsttable);
									Description *desc=getDesc(&nono,ruleop.line,ruleop.linenumber);
									BlocksRanges *ranges=getBlockRanges(&nono,ruleop.line,ruleop.linenumber,&tables.firsttable);

									if(Rules[ruleop.id](line,length,stride,desc,ranges)<0){
										(void)write(STDERR_FILENO,"ERROR: line rule retuned a error!\n",35);
										goto GETOPT_PARSING_ERROR;
									}
								}
								else (void)write(STDERR_FILENO,"WARNING: single line rule ignored as line didn't exists!\n",57);
							}
							else if(ruleop.id==FULL_RUN_RULE){
								// Run rule to every line in a dimension
								int32_t lineamount=(!ruleop.line)*nono.height+ruleop.line*nono.width;
								for(int32_t linenumber=lineamount-1;linenumber>=0;linenumber--){

									enum Pixel *line=getLine(&nono,ruleop.line,linenumber,&tables.firsttable);
									Description *desc=getDesc(&nono,ruleop.line,linenumber);
									BlocksRanges *ranges=getBlockRanges(&nono,ruleop.line,linenumber,&tables.firsttable);

									if(Rules[ruleop.linenumber](line,length,stride,desc,ranges)<0){
										(void)write(STDERR_FILENO,"ERROR: line rule retuned a error!\n",35);
										goto GETOPT_PARSING_ERROR;
									}
								}
							}
							else if(ruleop.id==FORCE_COLOR){
								// Forcefully colour a pixel.
								struct ForcefullyColour *cmd=(struct ForcefullyColour*)rulelist;
								if(cmd->check && getTablePixel (&nono,&tables.firsttable,cmd->row,cmd->column)!=UNKNOWN_PIXEL){
									(void)write(STDERR_FILENO,"ERROR: Trying colour pixel which is already coloured!\n",54);
									goto ERROR_AT_NONOGRAMGENSVG;
								}
								// Sanity check cmd->row and cmd->column
								if(0<=cmd->row && cmd->row<nono.height && 0<=cmd->column && cmd->column<nono.width){
									                           colourTablePixel (&nono,&tables.firsttable,cmd->row,cmd->column,cmd->colour);
								}
								else (void)write(STDERR_FILENO,"WARNING: single line rule ignored as colouring would be off!\n",61);
							}

							// Move to next rule and free memory.
							{
								struct SingleRule *notneeded=rulelist;
								rulelist=rulelist->next;
								free(notneeded);
							}
						}
					}

					#ifdef _PROFILE_
						newSample(0,nono.width*nono.height);
						// Profiling for the whole algorithm.
						tickProfiling(0,0);
					#endif
					if(flags.dopartialsolution){
						#ifdef _PROFILE_
							// Profile partial solver.
							tickProfiling(0,1);
						#endif
						// Simple loop every logical rule until no
						// updates where made.
						int update;
						do{
							update=0;
							{
								// Handle rows.
								uint32_t stride=1;
								int32_t length=nono.width;
								for(int32_t row=nono.height-1;row>=0;row--){
									enum Pixel *line=getLine(&nono,0,row,&tables.firsttable);
									Description *desc=getDesc(&nono,0,row);
									BlocksRanges *ranges=getBlockRanges(&nono,0,row,&tables.firsttable);
									for(int8_t rule=(&Rules[12]-&Rules[0])-1;rule>=0;rule--){
										int32_t temp=Rules[rule](line,length,stride,desc,ranges);
										if(temp<0) goto ERROR_AT_SOLVER;
										update+=temp;
									}
								}
							}
							// Handle column.
							{
								uint32_t stride=nono.width;
								int32_t length=nono.height;
								for(int32_t col=nono.width-1;col>=0;col--){
									enum Pixel *line=getLine(&nono,1,col,&tables.firsttable);
									Description *desc=getDesc(&nono,1,col);
									BlocksRanges *ranges=getBlockRanges(&nono,1,col,&tables.firsttable);
									for(int8_t rule=(&Rules[12]-&Rules[0])-1;rule>=0;rule--){
										int32_t temp=Rules[rule](line,length,stride,desc,ranges);
										if(temp<0) goto ERROR_AT_SOLVER;
										update+=temp;
									}
								}
							}

						}while(update);
						#ifdef _PROFILE_
							// End profiling of partial solver.
							tockProfiling(0,1);
						#endif
					}

					// Did user ask for partial solution?
					if(flags.dosvg && flags.writepartialsolution){
						int result=nonogramWriteSvg(STDOUT_FILENO,&htmlpage,&nono,&tables.firsttable,writeflags);
						if(!result) (void)write(STDERR_FILENO,"ERROR: Partial solution SVG couldn't be written!\n",49);
					}

					// Did user ask check for block's range swapping?
					if(flags.doblockswapguard){
						// Check every row block has swapped
						for(int32_t row=0;row<nono.height;row++){
							for(int32_t block=0;block<nono.rowsdesc[row].length;block++){
								if(tables.firsttable.row[row].blocksrangelefts[block]>tables.firsttable.row[row].blocksrangerights[block]){
									// SANITY GONE!
									(void)write(STDERR_FILENO,"SANITY: Block's range swap in one the rows!\n",44);
									goto ERROR_AT_NONOGRAMGENSVG;
								}
							}
						}
						// Check every column block has swapped
						for(int32_t col=0;col<nono.width;col++){
							for(int32_t block=0;block<nono.colsdesc[col].length;block++){
								if(tables.firsttable.col[col].blocksrangelefts[block]>tables.firsttable.col[col].blocksrangerights[block]){
									// SANITY GONE!
									(void)write(STDERR_FILENO,"SANITY: Block's range swap in one the columns!\n",47);
									goto ERROR_AT_NONOGRAMGENSVG;
								}
							}
						}
					}

					// Solution counting estimate algorithm.
					if(flags.dosolutioncountingestimate){
						(void)write(STDOUT_FILENO,"Estimating number of solutions.\n",32);

						// Get switching components.
						uint32_t solutioncount=0;
						uint32_t undetected=0;
						SwitchingComponent *scomp;
						int32_t numberofswitchingcomps;
						#ifdef _PROFILE_
							tickProfiling(0,2);
						#endif
						if((numberofswitchingcomps=detectSwitchingComponents(&nono,&tables,&scomp))>0){

							#ifdef _PROFILE_
								initProfiling(1,numberofswitchingcomps,profbuffpath);
							#endif
							// Detect every switching component.
							for(int32_t comp=0;comp<numberofswitchingcomps;comp++){
								#ifdef _PROFILE_
									newSample(1,scomp[comp].sizeofunknownpixelgraph);
									tickProfiling(1,0);
								#endif
								uint32_t temp=NonoDetectOneBlackColourableOnePixelSquareSwitchingComponent(&nono,&tables,scomp+comp);
								#ifdef _PROFILE_
									tockProfiling(1,0);
								#endif
								if(temp){
									solutioncount+=temp;
								}
								else undetected++;
							}
							#ifdef _PROFILE_
								tockProfiling(1,0);
								tockProfiling(0,2);
								tockProfiling(0,0);
								finalizePlot(1,0);
							#endif

							// Print out number of solution counted.
							PRINT_SOLUTION_COUNT:
							{
								struct iovec iov[]={{"Estimated number of solutions: ",31},{0,0},{"\nNumber of unknown proposed switching components: ",51},{0,0},{"\n",1}};
								iov[1].iov_len=i32toalen(solutioncount);
								uint8_t buffer[iov[1].iov_len];
								iov[1].iov_base=buffer;
								i32toa(solutioncount,buffer,iov[1].iov_len);
								iov[3].iov_len=i32toalen(undetected);
								uint8_t undetectedbuffer[iov[3].iov_len];
								iov[3].iov_base=undetectedbuffer;
								i32toa(undetected,undetectedbuffer,iov[3].iov_len);
								(void)writev(STDOUT_FILENO,iov,sizeof(iov)/sizeof(struct iovec));
							}

							freeSwitchingComponentArray(scomp,numberofswitchingcomps);
						}
						else if(numberofswitchingcomps==0){
							// One solution.
							(void)write(STDOUT_FILENO,"Only one solution.\n",19);
						}
					}

					if(flags.dosvg){
						int result=nonogramWriteSvg(STDOUT_FILENO,&htmlpage,&nono,&tables.firsttable,writeflags);
						if(!result) (void)write(STDERR_FILENO,"ERROR: SVG couldn't be written!\n",32);
						nonogramFreeSvg(&htmlpage);
					}

					ERROR_AT_NONOGRAMGENSVG:
					// Start unwinding the resource allocation process.
					nonogramEmptyTables(&nono,&tables);
				}
				else (void)write(STDERR_FILENO,"ERROR: Table's pixels couldn't be allocated!\n",45);
				nonogramEmpty(&nono);
			}
			else{
				(void)write(STDERR_FILENO,"ERROR: Couldn't read!\n",22);
				if(result.id>2){

					// Write out line number error happened at.
					uint32_t linelength=i32toalen(result.line);
					uint8_t linebuff[linelength];
					i32toa(result.line,linebuff,linelength);

					struct iovec io[]={{"Error happened at: ",19},{linebuff,linelength},{"\n",1}};
					(void)writev(STDERR_FILENO,io,sizeof(io)/sizeof(struct iovec));
				}
			}
			close(file);
		}
		else (void)write(STDERR_FILENO,"ERROR: File couldn't be opened!\n",32);
	}
	else (void)write(STDERR_FILENO,"ERROR: No file to open!\n",24);
	
	// If error happens then free happens as well.
	while(rulelist){
		struct SingleRule *temp=rulelist->next;
		free(rulelist);
		rulelist=temp;
	}

	// Exit
	return 0;

	// Parsing error during options handling.
	GETOPT_PARSING_ERROR:
	while(rulelist){
		struct SingleRule *temp=rulelist->next;
		free(rulelist);
		rulelist=temp;
	}
	return 1;

	ERROR_AT_SOLVER:
	(void)write(STDERR_FILENO,"Warning: Solver reported an error!\n",35);
	goto ERROR_AT_NONOGRAMGENSVG;
}
