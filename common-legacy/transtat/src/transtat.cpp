/*
 * TRANSTAT.C
 * written by   :  Tim Heier
 * last updated :   04/28/96
 */

#define VER               "1.0"
#define REL          "04/28/96"

#include "stdutil.h"
#include "ctlutil.h"

#define INIFILE  "transtat.ini"     /* file with user settings */
#define MAXLENGTH           150     /* length for character strings */
#define MAXLINES            500     /* transit lines */
#define MAXFILES              6     /* transit summary files */
#define MAXGROUPS            25     /* number of groups */
#define MAXSEGMENTS         200     /* segments in a transit line */

FILE  *dptr, *iptr, *optr, *lptr, *aptr[6+1];

void  page_header(void);
void  run_header(void);
void  banner(void);
void  main_menu(void);
void  module1(void);
void  module2(void);
void  module3(void);
void  module4(void);
void  read_control(void);

/* global variables */
int    i, ret;                /* scratch variables */
int    run_option;            /* which run mode are we in */
int    bDefFile;              /* was a default file found */
int    bNodeLabels;           /* were node labels in control file */
int    ngrplst;               /* number of items in grplst[] */
int    ngroups;               /* number of groups not including group 0 */
int    nlabels;               /* number of node labels */
float  tot[MAXFILES+1];       /* temporary variable for column totals */

float  TimePeriod;            /* time period for analysis */
float  RoundingThreshold;

char  input_file[MAXLENGTH];  /* input file name */
char  output_file[MAXLENGTH]; /* output file */
char  iniFile[MAXLENGTH];     /* ini file name */

typedef struct {              /* hold vehicle data */
  int   type;
  float seats;
  float targetload;
  float layfactor;
} ini_data;

typedef struct {              /* hold line input list */
  char  n[6+1];               /* line name */
  int   group;                /* group the line belongs to */
  float val[MAXFILES+1];      /* value in each file */
  float tot;                  /* total for all files */
} line_data;

typedef struct {             /* hold record from the 6.21 report */
  char  n[6+1];
  char  mode[2];
  long  type;
  float cveh;                /* coded values */
  float chdwy;
  float length;
  float time;
  float board;
  float cmiles;
  float chours;
  float avgload;
  float maxload;
  float maxvol;
  float opcost;
  float energy;
  float cvehtot;
  float cmilestot;
  float chourstot;
  float c1way;
  float c1waytot;

  float dhdwy;               /* demand values */
  float dveh;
  float dmiles;
  float dhours;
  char  dflag;
  float dvehtot;
  float dmilestot;
  float dhourstot;
  float d1way;
  float d1waytot;

  float phdwy;               /* policy values */
  float pveh;
  float pmiles;
  float phours;
  char  pflag;
  float pvehtot;
  float pmilestot;
  float phourstot;
  float p1way;
  float p1waytot;
} vehicle_data;

typedef struct {              /* a single record from the busstat report */
  char  n[6+1];
  float col[7+1];
} busstat_data;

typedef struct {              /* input list */
  float val[MAXFILES+1];      /* value in each file */
  float tot;                  /* total for all files */
} group_data;

typedef struct {              /* transit line groups */
  char  n[6+1];               /* line descriptor */
  int   group;                /* group line belongs to */
} group_list;

typedef struct {              /* structure to hold master line input list */
  long  num;                  /* node number */
  char  label[25+1];          /* node label */
} node_data;

ini_data     *ini;
line_data    *mlist;
vehicle_data *veh;
busstat_data *bs;
group_data   *grpdata;
group_list   grplst[MAXLINES+1];
node_data    nldata[MAXSEGMENTS+1];

int main(int argc, char *argv[])
{
long option;

if(argc > 1)
	strcpy(iniFile,argv[1]);
else
	strcpy(iniFile,INIFILE);

read_control();

banner();

while ( (option != 9) && (option != 113) ){
	main_menu();
	option = get_long("Option ?= ");
	switch (option) {
      case  1  : run_option = option;
		 module1();
		 break;
      case  2  : run_option = option;
		 module2();
		 break;
      case  3  : run_option = option;
		 module3();
		 break;
      case  4  : run_option = option;
		 module4();
		 break;
		}
   }

printf("\n\nTRANSTAT (TM)\n");
printf("Ver %s  Rel %s\n", VER, REL);

return(0);
}

/*************************************************************************/
void banner(void)
{
printf("\n\n\n");
printf("     TRANSIT ANALYZER FOR TRANSIT LINES\n");
printf("        (Module 6.21 Post-Processor)\n\n\n");
printf("            Ver %s  Rel %s\n", VER, REL);
printf("                TRANSTAT (TM)\n");

printf("\n\n\n[ENTER] to continue...\n");
getc(stdin);
printf("\n\n");
}

/*************************************************************************/
void main_menu(void)
{
run_header();
printf("\nOptions:\n");
printf("             1= Perform transit route equilibration\n");
printf("             2= Add transit line summary files together\n");
printf("             3= Work with TRANSTAT output (from Module 2)\n");
printf("             4= Add transit link segment volume files\n");
printf("             9= Exit program\n");
}

/*************************************************************************/
void module1(void)
{
  int   j;                    /* loop variables */
  int   end;                  /* EOF flag */
  int   width, dec;           /* formating */
  int   nlines, nselect;      /* lines with selected vehicle type */
  long  down,up;              /* rounded values */
  char  answer[2];            /* answer to single character questions */
  char  inbuf[MAXLENGTH+1];   /* line from input file */
  char  format1[80];          /* line format string */
  char  format2[80];          /* line format string */

  nlines = nselect = 0;

  printf("\n\n\nMODULE 1 - Perform transit route equilibration\n");

  if( (veh = malloc(sizeof(vehicle_data))) == NULL ) {
	  printf("error: could not allocate \"vehicles data\" buffer\n");
	  pause_exit();
	  }
  memset(veh, 0, sizeof(vehicle_data));

  printf("\n");
  do {
	  printf("File name for line summary file: ");
	  gets(input_file);
	  if ( (iptr = fopen(input_file,"r")) == NULL )
	printf("error: opening file \"%s\" \n\n",input_file);
	} while (iptr == NULL);

  printf("\n");
  do {
	  printf("Output file: ");  gets(output_file);
	  } while (strlen(output_file) == 0);

  if (! file_exists(output_file))
	  optr = fopen(output_file,"w");
  else {
  answer[0]='N';
  while ( (file_exists(output_file)) && (answer[0] == 'N') ) {
	  printf("*File \"%s\" exists - overwrite (Y or N) or append (A): ",
			 output_file);

	  gets(answer);
	  answer[0] = toupper(answer[0]);
	  printf("\n");
	  switch (answer[0]) {
	case 'Y':
		 optr = fopen(output_file,"w");
		 break;
	case 'N':
		  printf("\n");
		  do {
			  printf("Output file: ");
			  gets(output_file);
			  } while (strlen(output_file) == 0);
		 break;
	case 'A':
		 optr = fopen(output_file,"a");
		 break;
	default:
		 printf("*Valid answers are \"Y\", \"N\", or \"A\"\n\n");
		 answer[0] = 'N';
		 break;
	}
	  }
  }

/* process the 6.21 file - start by writing header to output file and
  then find the first occurance of "line"  in input file*/
page_header();
fprintf(optr,  "Input File: %s\n", input_file);
fprintf(optr,"\nTime Period        : %.0f min.\n",TimePeriod);
fprintf(optr,  "Rounding Threshold : %.2f\n",RoundingThreshold);
fprintf(optr,"\nVehicle Type       : %d\n",  ini->type);
fprintf(optr,  "Seats on Vehicle   : %.0f\n",ini->seats);
fprintf(optr,  "Target Load Factor : %.2f\n",ini->targetload);
fprintf(optr,  "Layover Factor     : %.2f\n",ini->layfactor);
fprintf(optr,"\n\n");
fprintf(optr,"T R A N S I T   E Q U I L I B R A T I O N   R E S U L T S\n");
fprintf(optr,"*********************************************************\n");
fprintf(optr,"\n");
fprintf(optr,"                              ------------coded------------      ------------demand------------ \n");
fprintf(optr,"        max.   line   line          no. 1-way    veh    veh       	no. 1-way    veh    veh \n");
fprintf(optr,"  line   vol length   time    hdwy  veh  trip   dist   time       hdwy  veh  trip   dist   time \n");
fprintf(optr,"                     (min)                            (hrs)                               (hrs) \n");

printf("\nReading transit lines...\n");

end = fget_line(inbuf,MAXLENGTH,iptr);
while (end != EOF) {
	if(strncmp(&inbuf[1],"line",4) == 0) {
		for(i=0; i < 3; i++)
			fget_line(inbuf,MAXLENGTH,iptr);
		while ( ((end = fget_line(inbuf,MAXLENGTH,iptr)) != EOF) &&
				(strlen(inbuf) > 5) ) {

			sscanf(inbuf," %s %s", veh->n, veh->mode);
			printf(" %6s\r", veh->n);

			veh->type    = getlong(inbuf,11,4);
			veh->cveh    = getfloat(inbuf,16,4);
			veh->chdwy   = getfloat(inbuf,21,6);
			veh->length  = getfloat(inbuf,28,7);
			veh->time    = getfloat(inbuf,36,6);
			veh->board   = getfloat(inbuf,44,6);
			veh->cmiles  = getfloat(inbuf,51,6);
			veh->chours  = getfloat(inbuf,58,6);
			veh->avgload = getfloat(inbuf,65,5);
			veh->maxload = getfloat(inbuf,71,4);
			veh->maxvol  = getfloat(inbuf,77,6);
			veh->opcost  = getfloat(inbuf,85,6);
			veh->energy  = getfloat(inbuf,93,6);

			nlines++;

			/* screen for transit line type */
			if(veh->type != (long)ini->type)
				goto NEXT_LINE;

			nselect++;

			/* do some error checking */
			if(veh->cveh == 0)
				printf("\nwarning: number of vehicles = 0, line \"%s\"\n",veh->n);

			if(veh->chdwy == 0) {
				printf("\nwarning: headway = 0, line \"%s\"\n",veh->n);
				veh->chdwy = 30;
				}

/*
Start transit line equilibration...

Methodology
------------

Step 1 - Equilibrate based on demand
1. Calculate number of vehicles to meet capacity and load requirement
2. Calculate simple headway for buses during time period
3. Calculate new number of buses to meet capacity after accounting for
	line time over the bus route.
4. Compare number of vehicles with the original number of vehicles -
	flag with a '+' if the new demand is greater than the original
	number of buses and '-' if the new demand is less than the original

*/
/* ------------------------ coded calculations -------------------------- */

			/* calculate number of buses - coded */
			down = veh->cveh;  up = down + 1;
			veh->cveh = (veh->time * ini->layfactor)/veh->chdwy;
			if( (veh->cveh - (float)down) >= RoundingThreshold )
				veh->cveh = (float)up;
			else
				veh->cveh = (float)down;

			/* calculate 1 trips - coded */
			veh->c1way = TimePeriod/veh->chdwy;

			/* calculate vehicle miles/hours - coded */
			veh->cmiles = veh->c1way * veh->length;
			veh->chours = (veh->c1way * veh->time * ini->layfactor)/60;

			/* sum total values - coded */
			veh->cvehtot   += veh->cveh;
			veh->c1waytot  += veh->c1way;
			veh->cmilestot += veh->cmiles;
			veh->chourstot += veh->chours;

/* ----------------------- demand calculations -------------------------- */

		/* calculate headway based on maximum volume - demand */
			if(veh->maxvol == 0)
				veh->dhdwy = 0;
			else
				veh->dhdwy = (TimePeriod * ini->seats * ini->targetload) / veh->maxvol;

			/* calculate number of buses - demand */
			if(veh->dhdwy == 0)
				veh->dveh = 0;
			else
				veh->dveh = (veh->time * ini->layfactor)/veh->dhdwy;

			down = veh->dveh;  up = down + 1;
			if( (veh->dveh - (float)(down)) >= RoundingThreshold )
				veh->dveh = (float)up;
			else
				veh->dveh = (float)down;

			/* calculate 1 trips - demand */
			if(veh->dhdwy == 0)
				veh->d1way = 0;
			else
				veh->d1way = TimePeriod/veh->dhdwy;

			/* calculate vehicle miles/hours - demand */
			veh->dmiles = veh->d1way * veh->length;
			veh->dhours = (veh->d1way * veh->time * ini->layfactor)/60;

			/* sum total values - demand */
			veh->dvehtot   += veh->dveh;
			veh->d1waytot  += veh->d1way;
			veh->dmilestot += veh->dmiles;
			veh->dhourstot += veh->dhours;

			/* check buses based on demand vs.coded headway */
			if (veh->dhdwy < veh->chdwy)
				veh->dflag = '+';
			else
				veh->dflag = '-';

			if(veh->maxvol == 0)  /* special case - no volume on line */
				veh->dflag = ' ';

/* ----------------------- policy calculations -------------------------- */

			/* hard code policy values for now */
			veh->pveh   = 0;
			veh->p1way  = 0;
			veh->phdwy  = 0;
			veh->pmiles = 0;
			veh->phours = 0;
			veh->pflag  = '*';

			/* calculate policy miles/hours based on number of vehicles */
			veh->pmiles = veh->pveh * veh->length;
			veh->phours = veh->pveh * (veh->time/60);

			/* sum total values */
			veh->pvehtot   += veh->pveh;
			veh->p1waytot  += veh->p1way;
			veh->pmilestot += veh->pmiles;
			veh->phourstot += veh->phours;

/* ------------------------ write out record --------------------------- */

			/* set decimal place based on size of number */
			if(veh->cmiles > 99999) {
				width = 6;
				dec   = 0;
				}
			else {
				width = 7;
				dec   = 1;
				}

			/* prepare string format specifier */
			sprintf(format1," %%7.1f %%4.0f %%4.0f %%%d.%df %%6.1f|", width, dec);
			sprintf(format2," %%c %%7.1f %%4.0f %%4.0f %%%d.%df %%6.1f|", width, dec);

			fprintf(optr,"%6s %5.0f %6.2f %6.2f",
				veh->n, veh->maxvol, veh->length, veh->time);

			/* operating statistics based on coding (no constraints) */
			fprintf(optr,format1,
				veh->chdwy,veh->cveh,veh->c1way,veh->cmiles,veh->chours);

			/* operating statistics based on demand */
			fprintf(optr,format2,veh->dflag,
				veh->dhdwy,veh->dveh,veh->d1way,veh->dmiles,veh->dhours);

			/* adance to the next line */
			fprintf(optr,"\n");

			NEXT_LINE: ;
			} /* while */

		} /* if */
	else
		end = fget_line(inbuf,MAXLENGTH,iptr);

	} /* while */

/* need this because of the transit line screen counter */
printf("\n");

/* print totals */
fprintf(optr,"\n\n");
fprintf(optr,  "TRANSIT SUMMARY STATISTICS\n");
fprintf(optr,  "---------------------------\n");
fprintf(optr,  "\n");
fprintf(optr,  " %3d transit lines read.\n",nlines);
fprintf(optr,  " %3d transit lines have selected vehicle type.\n",nselect);
fprintf(optr,  "\n");
fprintf(optr,  "                                Coded    Demand \n");
fprintf(optr,  "                              -------------------\n");
fprintf(optr,  "Number of Vehicles            %7.0f   %7.0f\n", veh->cvehtot,veh->dvehtot);
fprintf(optr,  "Number of One-Way Trips       %7.0f   %7.0f\n", veh->c1waytot,veh->d1waytot);
fprintf(optr,  "Number of Vehicle Miles(Km)   %7.0f   %7.0f\n", veh->cmilestot,veh->dmilestot);
fprintf(optr,  "Number of Vehicle Hours       %7.0f   %7.0f\n", veh->chourstot,veh->dhourstot);
fprintf(optr,"\n");

fclose(optr);
free(veh);

printf("\nProcessing complete...\n");
printf("Press any key to continue...");
getc(stdin);
printf("\n\n");
return;
} /* end module1() */

/*************************************************************************/
void module2(void)
{
int   i, j, k, end, valid;
int   location;              /* place saver for arrays */
int   nfiles;                /* number of files to add together */
int   nlines = 0;            /* number of lines in master list */
int   onlist;                /* indicator for list comparison */
int   show_groups;           /* output values by group */
int   show_definitions = 0;  /* report group definitions to output file */
char  answer[2];             /* answer to single character questions */
char  line[MAXLENGTH+1];     /* scratch character string */

struct {                     /* structure to hold input file data */
	char  n[12+1];            /* file name */
} ifile[MAXFILES+1];

printf("\n\n\nMODULE 2 - Add transit line summary files together\n\n");

if( (mlist = calloc( (MAXLINES+1), sizeof(line_data))) == NULL ) {
	printf("error: could not allocate \"line data\" buffers\n");
	pause_exit();
	}
if( (veh = malloc(sizeof(vehicle_data))) == NULL ) {
	printf("error: could not allocate \"vehicle data\" buffer\n");
	pause_exit();
	}
if( (grpdata = calloc( (MAXGROUPS+1), sizeof(group_data))) == NULL ) {
	printf("error: could not allocate \"group data\" buffers\n");
	pause_exit();
	}

for(i=1; i <= MAXLINES; i++) {
	mlist[i].n[0] = '\0';
	mlist[i].tot = 0;
	for(j=1; j <= MAXFILES; j++)
		mlist[i].val[j] = 0;
	}
for(i=0; i <= MAXFILES; i++)
	tot[i] = 0;

printf("\nNumber of transit line files to add (max. 6):\n");
do {
	printf("?=");
	gets(line);
	sscanf(line,"%d", &nfiles);
} while ((nfiles <= 0 ) || (nfiles > MAXFILES));

printf("\n");
for(i=1; i <= nfiles; i++) {
	do {
		printf("File name for line summary file #%d: ", i);
		gets(ifile[i].n);
		if( (aptr[i] = fopen(ifile[i].n,"r")) == NULL )
			printf("error: opening file \"%s\" \n\n",ifile[i].n);
		} while (aptr[i] == NULL);
	}

	do {
		printf("\nOutput file: ");
		gets(output_file);
		} while (strlen(output_file) == 0);

if (! file_exists(output_file))
	optr = fopen(output_file,"w");
else {
	answer[0]='N';
	while ( (file_exists(output_file)) && (answer[0] == 'N') ) {
		printf("*file \"%s\" exists - overwrite (Y or N) or append (A): ",
				output_file);

		gets(answer);
		answer[0] = toupper(answer[0]);
		printf("\n");
		switch (answer[0]) {
			case 'Y': optr = fopen(output_file,"w"); break;
			case 'N':
				printf("\n");
				do {
					printf("Output file: ");
					gets(output_file);
					} while (strlen(output_file) == 0);
				break;
			case 'A': optr = fopen(output_file,"a"); break;
			default:
				printf("*valid answers are \"Y\", \"N\", or \"A\"\n\n");
				answer[0] = 'N';
				break;
			}
		}
	}

printf("\n");
do {
	printf("Output values by group as well? (y or n):");
	gets(answer);
	answer[0] = toupper(answer[0]);
	if (answer[0] == 'N') {
		valid = 1;
		show_groups = 0;
		}
	else
	if (answer[0] == 'Y') {
		valid = 1;
		show_groups = 1;
		}
	else
		valid = 0;
	} while (! valid);

if (show_groups) {
	printf("\n\n");
	do {
		printf("Print group definitions? (y or n):");
		gets(answer);
		answer[0] = toupper(answer[0]);
		if (answer[0] == 'N') {
			valid = 1;
			show_definitions = 0;
			}
		else
		if (answer[0] == 'Y') {
			valid = 1;
			show_definitions = 1;
			}
		else
			valid = 0;
		} while (! valid);
	}

/* build a master list of transit routes and add up values */

printf("\n\nBuilding Master Line Summary File...\n");
for(j=1; j <= nfiles; j++) {
	rewind(aptr[j]);
	end = fget_line(line,MAXLENGTH,aptr[j]);
	while (end != EOF) {
		if(strncmp(&line[1],"line",4) == 0) {
			for(i=0; i < 3; i++)
				fget_line(line,MAXLENGTH,aptr[j]);
			while ( ((end = fget_line(line,MAXLENGTH,aptr[j])) != EOF) &&
						(strlen(line) > 5) ) {

				sscanf(line," %s %s",veh->n, veh->mode);
				veh->type    = getlong(line,11,4);
				veh->cveh    = getfloat(line,16,4);
				veh->chdwy   = getfloat(line,21,6);
				veh->length  = getfloat(line,28,7);
				veh->time    = getfloat(line,36,6);
				veh->board   = getfloat(line,44,6);
				veh->cmiles  = getfloat(line,51,6);
				veh->chours  = getfloat(line,58,6);
				veh->avgload = getfloat(line,65,5);
				veh->maxload = getfloat(line,71,4);
				veh->maxvol  = getfloat(line,77,6);
				veh->opcost  = getfloat(line,85,6);
				veh->energy  = getfloat(line,93,6);

				onlist = 0;
				for(k=1; k <= nlines; k++)
					if(strcmp(mlist[k].n, veh->n) == 0) {
						onlist = 1;
						location = k;
						break;
						}

				if(! onlist) {
					nlines++;
					location = nlines;
					strcpy(mlist[nlines].n,veh->n);
					mlist[nlines].n[strlen(veh->n)] = '\0';
					}

				mlist[location].val[j]  = veh->board;
				mlist[location].tot    += veh->board;
				tot[j] += veh->board;

				}
			}
		else
			end = fget_line(line,MAXLENGTH,aptr[j]);
		}
	}

printf("-> %d distinct transit lines found\n", nlines);
printf("\nBuilding Output Line Summary File...\n");

/* add group indicator to master list - if a line ia on the master list
	is not in the grplst then it is assigned to the default group[0] */

for(k=1; k <= nlines; k++)
	for(i=0; i <= ngrplst; i++)
		if(strcmp(mlist[k].n, grplst[i].n) == 0) {
			if (mlist[k].group > 0) {
				printf("\nerror: a transit line is assigned to more than one group\n");
				printf("line -> %s\n", mlist[k].n);
				pause_exit();
				}
			else
				mlist[k].group = grplst[i].group;
			}

page_header();

fprintf(optr,"Transit Boarding Summaries/Totals:\n\n" );

fprintf(optr,"  Line:");
for(j=1; j <= nfiles; j++)
	fprintf(optr," %12s", ifile[j].n);
fprintf(optr,"       Total:\n");

for(i=1; i <= nlines; i++) {
	fprintf(optr," %6s", mlist[i].n);
	for(j=1; j <= nfiles; j++)
		fprintf(optr," %12.0f", mlist[i].val[j]);
	fprintf(optr," %12.0f\n", mlist[i].tot);
	}

fprintf(optr,"\n Total:");
for(j=1; j <= nfiles; j++) {
	fprintf(optr," %12.0f", tot[j]);
	tot[0] += tot[j];
	}
fprintf(optr," %12.0f\n", tot[0]);

/* loop through the groups, lines and files to accumulate group totals
	lines with no match are assigned to group[0] */
if (show_groups) {
	for(i=0; i <= ngroups; i++) {
		for(j=1; j <= nlines; j++) {
			for (k=1; k <= nfiles; k++) {
				if (mlist[j].group == i) {
					grpdata[i].val[k] += mlist[j].val[k];
					grpdata[i].tot    += mlist[j].val[k];
					}
				}
			}
		}
	}

/* now print out groups to output file */
fprintf(optr,"\n\nTransit Group Totals:\n\n" );
fprintf(optr," Group:");
for(j=1; j <= nfiles; j++)
	fprintf(optr," %12s", ifile[j].n);
fprintf(optr,"       Total:\n");

for(i=0; i <= ngroups; i++) {
	fprintf(optr," grp %2d", i);
	for(j=1; j <= nfiles; j++)
		fprintf(optr," %12.0f", grpdata[i].val[j]);
	fprintf(optr," %12.0f\n", grpdata[i].tot);
	}

if (show_definitions) {
	fprintf(optr,"\n\nTransit Line Group Assignments:\n\n" );
	for(i=0; i <= ngroups; i++) {
		fprintf(optr,"Group %2d:", i);
		k = 0;
		for(j=1; j <= nlines; j++) {

			if (mlist[j].group == i) {
				k++;
				fprintf(optr," %6s", mlist[j].n);
				if ((k % 10) == 0) {
					fprintf(optr,"\n");
					fprintf(optr,"Group %2d:", i);
					}
				}
			}

		fprintf(optr,"\n");
		}
	}

fclose(optr);
free(mlist);
free(veh);
free(grpdata);

printf("\nProcessing complete...\n");
printf("Press any key to continue...");
getc(stdin);
printf("\n\n");

return;
}  /* end module2() */

/************************************************************************/
void module3(void)
{
int   i, j, k, end, valid;
int   location;              /* place saver for arrays */
int   col[2+1];              /* summary column from summary files */
int   operation;             /* what kind of comparison are we doing */
int   nfiles;                /* number of files to add together */
int   nlines = 0;            /* number of lines in master list */
int   onlist;                /* indicator for list comparison */
int   show_groups;           /* output values by group */
int   show_definitions = 0;  /* report group definitions to output file */
char  answer[2];             /* answer to single character questions */
char  line[MAXLENGTH+1];     /* scratch character string */

struct {                     /* structure to hold input file data */
	char  n[12+1];            /* file name */
} ifile[MAXFILES+1];

printf("\n\n\nMODULE 3 - Compare TRANSTAT summary files (from Module 2)\n\n");

if( (mlist = calloc( (MAXLINES+1), sizeof(line_data))) == NULL ) {
	printf("error: could not allocate \"line data\" buffers\n");
	pause_exit();
	}
if( (bs = calloc( 1, sizeof(busstat_data))) == NULL ) {
	printf("error: could not allocate \"busstat data\" buffers\n");
	pause_exit();
	}
if( (grpdata = calloc( (MAXGROUPS+1), sizeof(group_data))) == NULL ) {
	printf("error: could not allocate \"group data\" buffers\n");
	pause_exit();
	}
for(i=1; i <= MAXLINES; i++) {
	mlist[i].n[0] = '\0';
	mlist[i].tot = 0;
	for(j=1; j <= MAXFILES; j++)
		mlist[i].val[j] = 0;
	}
for(i=0; i <= MAXFILES; i++)
	tot[i] = 0;

nfiles = 2;

for(i=1; i <= nfiles; i++) {
	do {
		printf("\nFile name for line summary file #%d: ", i);
		gets(ifile[i].n);
		if ( (aptr[i] = fopen(ifile[i].n,"r")) == NULL )
			printf("error: opening file \"%s\" \n\n",ifile[i].n);
		} while (aptr[i] == NULL);

	do {
		printf("\nEnter the number of the column to operate on:\n");
		printf(  "(Note: Do not count the line description column)\n");
		printf("?=");
		get_line(line, MAXLENGTH);
		sscanf(line,"%d", &col[i]);
		} while ((col[i] <= 0 ) || (col[i] > MAXFILES+1));
	}

do {
	printf("\nOperation to perform on the selected columns:\n");
	printf(  "(Note: For subtraction, the second file will be subtracted from the first)\n");
	printf("\n 1. Add Values\n");
	printf(  " 2. Subtract Values\n");
	printf("?=");
	get_line(line, MAXLENGTH);
	sscanf(line,"%d", &operation);
	} while ((operation <= 0 ) || (operation > 2));

printf("\n");
do {
	printf("Output file: ");
	gets(output_file);
	} while (strlen(output_file) == 0);

if (! file_exists(output_file))
	optr = fopen(output_file,"w");
else {
	answer[0]='N';
	 while ( (file_exists(output_file)) && (answer[0] == 'N') ) {
		printf("*file \"%s\" exists - overwrite (Y or N) or append (A): ",
				output_file);

		gets(answer);
		answer[0] = toupper(answer[0]);
		printf("\n");
		switch (answer[0]) {
		case 'Y': optr = fopen(output_file,"w"); break;
		case 'N':
			printf("\n");
			do {
				printf("Output file: ");
				gets(output_file);
				} while (strlen(output_file) == 0);
			break;
		case 'A': optr = fopen(output_file,"a"); break;
		default:
			printf("*valid answers are \"Y\", \"N\", or \"A\"\n\n");
			answer[0] = 'N';
			break;
			}
		}
	}

printf("\n");
do {
	printf("Output values by group as well? (y or n):");
	gets(answer);
	answer[0] = toupper(answer[0]);
	if (answer[0] == 'N') {
		valid = 1;
		show_groups = 0;
		}
	else
	if (answer[0] == 'Y') {
		valid = 1;
		show_groups = 1;
		}
	else
		valid = 0;
	} while (! valid);

if (show_groups) {
	printf("\n\n");
	do {
		printf("Print group definitions? (y or n):");
		gets(answer);
		answer[0] = toupper(answer[0]);
		if (answer[0] == 'N') {
			valid = 1;
			show_definitions = 0;
			}
		else
		if (answer[0] == 'Y') {
			valid = 1;
			show_definitions = 1;
			}
		else
			valid = 0;
		} while (! valid);
	}

/* build a master list of transit routes */

for(j=1; j <= nfiles; j++) {
	rewind(aptr[j]);
	end = fget_line(line,MAXLENGTH,aptr[j]);
	while (end != EOF) {
		if(strncmp(&line[2],"Line:",4) == 0) {
			while ( ((end = fget_line(line,MAXLENGTH,aptr[j])) != EOF) &&
						(strlen(line) > 5) ) {
				sscanf(line," %s %f %f %f %f %f %f %f",
					bs[0].n, &bs[0].col[1], &bs[0].col[2], &bs[0].col[3],
					&bs[0].col[4], &bs[0].col[5], &bs[0].col[6], &bs[0].col[7]);

				onlist = 0;
				for(k=1; k <= nlines; k++)
					if(strcmp(mlist[k].n, bs[0].n) == 0) {
						onlist = 1;
						location = k;
						break;
						}

				if(! onlist) {
					nlines++;
					location = nlines;
					strcpy(mlist[nlines].n,bs[0].n);
					mlist[nlines].n[strlen(bs[0].n)] = '\0';
					}

				mlist[location].val[j]  = bs[0].col[col[j]];
				tot[j] += bs[0].col[col[j]];

				}
			}
		else
			end = fget_line(line,MAXLENGTH,aptr[j]);
		}
	}

printf("-> %d distinct transit lines found\n", nlines);
printf("\nBuilding Output Line Summary File...\n");

/* add group indicator to master list - if a line on the master list
	is not in the grplst then it is assigned to the default group[0] */
for(k=1; k <= nlines; k++)
	for(i=0; i <= ngrplst; i++)
		if(strcmp(mlist[k].n, grplst[i].n) == 0) {
			if (mlist[k].group > 0) {
				printf("\nerror: a transit line is assigned to more than one group\n");
				printf("Line -> %s\n", mlist[k].n);
				pause_exit();
				}
			else
				mlist[k].group = grplst[i].group;
			}

for(k=1; k <= nlines; k++) {
	switch (operation) {
		case 1: mlist[k].tot = mlist[k].val[1] + mlist[k].val[2]; break;
		case 2:	mlist[k].tot = mlist[k].val[1] - mlist[k].val[2]; break;
		}
	}

page_header();

fprintf(optr,"\nTransit Line Summaries:\n\n");

fprintf(optr,"Operation: ");
switch (operation) {
	case 1: fprintf(optr,"Addition"); break;
	case 2: fprintf(optr,"Subtraction"); break;
	}
fprintf(optr," (column %d from file 1 and column %d from file 2)\n",
		col[1], col[2]);

fprintf(optr,"\n  Line:");
for(j=1; j <= nfiles; j++)
	fprintf(optr," %12s", ifile[j].n);
fprintf(optr,"      Result:\n");

for(i=1; i <= nlines; i++) {
	fprintf(optr," %6s", mlist[i].n);
	for(j=1; j <= nfiles; j++)
		fprintf(optr," %12.0f", mlist[i].val[j]);
	fprintf(optr," %12.0f\n", mlist[i].tot);
	}

fprintf(optr,"\n Total:");
for(j=1; j <= nfiles; j++)
	fprintf(optr," %12.0f", tot[j]);

switch (operation) {
	case 1: tot[0] = tot[1] + tot[2]; break;
	case 2: tot[0] = tot[1] - tot[2]; break;
	}
fprintf(optr," %12.0f\n", tot[0]);

/* loop through the groups, lines and files to accumulate group totals
	lines with no match are assigned to group[0] */
if (show_groups) {
	for(i=0; i <= ngroups; i++) {
		for(j=1; j <= nlines; j++) {
			for (k=1; k <= nfiles; k++) {
				if (mlist[j].group == i) {
					grpdata[i].val[k] += mlist[j].val[k];
					tot[k] += mlist[j].val[k];
					}
				}
			}
		}
	}

/* perform selected operation - this could be done above loops in but it
	is done here to help keep things simple */
for(i=0; i <= ngroups; i++) {
	switch (operation) {
		case 1:
			grpdata[i].tot = grpdata[i].val[1] + grpdata[i].val[2];
			break;
		case 2:
			grpdata[i].tot = grpdata[i].val[1] - grpdata[i].val[2];
			break;
		}
	}

/* print out groups to output file */
fprintf(optr,"\n\nTransit Group Totals:\n\n" );
fprintf(optr," Group:");
for(j=1; j <= nfiles; j++)
	fprintf(optr," %12s", ifile[j].n);
fprintf(optr,"      Result:\n");

for(i=0; i <= ngroups; i++) {
	fprintf(optr," grp %2d", i);
	for(j=1; j <= nfiles; j++)
		fprintf(optr," %12.0f", grpdata[i].val[j]);
	fprintf(optr," %12.0f\n", grpdata[i].tot);
	}

fprintf(optr,"\n Total:");
for(j=1; j <= nfiles; j++)
	fprintf(optr," %12.0f", tot[j]);

switch (operation) {
	case 1: tot[0] = tot[1] + tot[2]; break;
	case 2: tot[0] = tot[1] - tot[2]; break;
	}
  fprintf(optr," %12.0f\n", tot[0]);

if (show_definitions) {
  fprintf(optr,"\n\nTransit Line Group Assignments:\n\n" );
	for(i=0; i <= ngroups; i++) {
		fprintf(optr,"Group %2d:", i);
		k = 0;
		for(j=1; j <= nlines; j++) {

			if (mlist[j].group == i) {
				k++;
				fprintf(optr," %6s", mlist[j].n);
				if ((k % 10) == 0) {
					fprintf(optr,"\n");
					fprintf(optr,"Group %2d:", i);
					}
				}
			}

		fprintf(optr,"\n");
		}
	}

fclose(optr);
free(mlist);
free(bs);
free(grpdata);

printf("\nProcessing complete...\n");
printf("Press any key to continue...");
getc(stdin);
printf("\n\n");
return;
}  /* end module3() */

/*************************************************************************/
void module4(void)
{
int   i, j, k, end, valid;
int   location;              /* place saver for arrays */
int   col[2+1];              /* summary column from summary files */
int   nfiles;                /* number of files to add together */
int   nsegs;                 /* number of lines in master list */
int   onlist;                /* indicator for list comparison */
int   show_graph;            /* show an ascii graph of line segment volumes */
int   graph_val;             /* which value to graph if we are graphing */
int   use_labels;            /* use node labels if they are available */
char  answer[2];             /* answer to single character questions */
char  line[MAXLENGTH+1];     /* scratch character string */
long  down, up;              /* rounding values */
float *barsize;              /* array of scaled bar sizes */
float barmax;                /* maximum bar size in ascii units */
float fval;

typedef struct {             /* input file data */
	char  name[80];            /* file name */
	float totent;              /* total entries */
	float totext;              /* total exits */
} ifile_data;

ifile_data *ifile;

typedef struct {             /* single record from 6.21 segment report */
	long  from;
	long  to;
	float length;
	float time;
	float speed;
	float load;
	float volume;
	long  stop;
	float exits;
	float entries;
} scratch_data;

scratch_data *sc;

typedef struct {              /* line segment data */
	long  from;                 /* from node */
	long  to;                   /* to node */
	float entries[MAXFILES+1];  /* value in each file */
	float exits[MAXFILES+1];    /* value in each file */
	float totvolume;            /* total for all files */
	float totentries;           /* total for all files */
	float totexits;             /* total for all files */
} segment_data;

segment_data *sd;

nsegs      = 0;
graph_val  = 0;
use_labels = 0;

printf("\n\n\nMODULE 4 - Add transit link volume summaries together\n\n");

if( (ifile = calloc( (MAXFILES+1), sizeof(ifile_data))) == NULL ) {
	printf("error: could not allocate \"file data\" buffers\n");
	pause_exit();
	}

if( (sd = calloc( (MAXSEGMENTS+1), sizeof(segment_data))) == NULL ) {
	printf("error: could not allocate \"segment data\" buffers\n");
	pause_exit();
	}

if( (sc = calloc( 1, sizeof(scratch_data))) == NULL ) {
	printf("error: could not allocate \"scratch data\" buffers\n");
	pause_exit();
	}

if( (barsize = calloc( MAXSEGMENTS+1, sizeof(float))) == NULL ) {
	printf("error: could not allocate \"scaled bar data\" buffers\n");
	pause_exit();
	}

printf("\nEnter number of transit line files to add together (max. 6):\n");
do {
	printf("?=");
	get_line(line, MAXLENGTH);
	sscanf(line,"%d", &nfiles);
	} while ((nfiles <= 0 ) || (nfiles > MAXFILES));

printf("\n");
for(i=1; i <= nfiles; i++) {
	do {
		printf("File name for line summary file #%d: ", i);
		gets(ifile[i].name);
		if( (aptr[i] = fopen(ifile[i].name,"r")) == NULL )
			printf("error: opening file \"%s\"\n\n",ifile[i].name);
		} while (aptr[i] == NULL);
	}

do {
	printf("\nOutput file: ");
	gets(output_file);
	} while (strlen(output_file) == 0);

if (! file_exists(output_file))
	optr = fopen(output_file,"w");
else {
	answer[0]='N';
	while ( (file_exists(output_file)) && (answer[0] == 'N') ) {
		printf("*File \"%s\" exists - overwrite (Y or N) or append (A): ",
			output_file);

		gets(answer);
		answer[0] = toupper(answer[0]);
		printf("\n");
		switch (answer[0]) {
			case 'Y': optr = fopen(output_file,"w"); break;
			case 'N':
				printf("\n");
				do {
					printf("Output file: ");
					gets(output_file);
					} while (strlen(output_file) == 0);
				break;
			case 'A': optr = fopen(output_file,"a"); break;
			default:
				printf("*Valid answers are \"Y\", \"N\", or \"A\"\n\n");
				answer[0] = 'N';
				break;
			}
		}
	}

printf("\n");
do {
	printf("Show graph of the total segment values? (y or n):");
	gets(answer);
	answer[0] = toupper(answer[0]);
	if (answer[0] == 'N') {
		valid = 1;
		show_graph = 0;
		}
	else if (answer[0] == 'Y') {
		valid = 1;
		show_graph = 1;
		}
	else
		valid = 0;
	} while (! valid);

NEW_GRAPH:
if (show_graph) {
	do {
		printf("\nEnter the segment value to graph:\n");
		printf(" 1. Total Volumes\n");
		printf(" 2. Total Entries\n");
		printf(" 3. Total Exits\n");
		printf("?=");
		gets(answer);
		answer[0] = toupper(answer[0]);
		line[0] = answer[0];  line[1] = '\0';
		graph_val = atoi(line);
		} while ((graph_val <= 0 ) || (graph_val > 3));
	}

if(bNodeLabels) {
	printf("\n\n");
	do {
		printf("Use node labels from the setup file? (y or n):");
		gets(answer);
		answer[0] = toupper(answer[0]);
		if (answer[0] == 'N') {
			valid = 1;
			use_labels = 0;
			}
		else if (answer[0] == 'Y') {
			valid = 1;
			use_labels = 1;
			}
		else
			valid = 0;
		} while (! valid);
	}

printf("\n\nBuilding Master Segment Summary File...\n");

/* build a master list of transit segments to work from */
for(j=1; j <= nfiles; j++) {
	end = fget_line(line,MAXLENGTH,aptr[j]);
	while (end != EOF) {
		end = fget_line(line,MAXLENGTH,aptr[j]);
		if(strncmp(&line[1],"from",4) == 0) {
			fget_line(line,MAXLENGTH,aptr[j]);
			while ( ((end = fget_line(line,MAXLENGTH,aptr[j])) != EOF) &&
					  ((str_index(line,"layover") < 0) &&
				(line[1] != FORMFEED)) ) {

				/* read each segment */
				sc[0].from = getlong(line,1,6);
				sc[0].to   = getlong(line,8,6);
				sc[0].stop = getlong(line,68,6);

				sc[0].length = getfloat(line,15,7);
				sc[0].time   = getfloat(line,23,7);
				sc[0].speed  = getfloat(line,31,7);
				sc[0].load   = getfloat(line,39,7);
				sc[0].volume = getfloat(line,48,7);
				sc[0].stop   = getlong(line,68,6);

				if((line[81] == '-') || (line[81] == ' '))
					sc[0].exits = 0;
				else
					sc[0].exits = getfloat(line,75,7);

				if((line[89] == '-') || (line[89] == ' '))
					sc[0].entries = 0;
				else
					sc[0].entries = getfloat(line,83,7);

				/* end of the line segment */
				if( (sc[0].stop > 0) && ((sc[0].from == 0)&&(sc[0].to == 0)) ) {
					sc[0].from = sc[0].stop;
					sc[0].to   = sc[0].stop;
					}

				/* is current segment is on list */
				onlist = FALSE;
				for(k=1; k <= nsegs; k++)
					if( (sd[k].from == sc[0].from)&&(sd[k].to == sc[0].to) ) {
						onlist = TRUE;
						location = k;
						}

				/* add to list */
				if(! onlist) {
					nsegs++;
					sd[nsegs].from = sc[0].from;
					sd[nsegs].to   = sc[0].to;
					location = nsegs;
					}

				/* update total columns */
				sd[location].entries[j] = sc[0].entries;
				sd[location].exits[j]   = sc[0].exits;
				sd[location].totvolume += sc[0].volume;

				sd[location].totentries += sc[0].entries;
				sd[location].totexits   += sc[0].exits;

				/* file totals */
				ifile[j].totent += sc[0].entries;
				ifile[j].totext += sc[0].exits;

				/* grand totals */
				ifile[0].totent += sc[0].entries;
				ifile[0].totext += sc[0].exits;

				} /* while */
			} /* if */
		} /* while */
	} /* for j */

/* actual segments are nsegs-1 because last entry is a stop */
printf("-> %d distinct transit segments found\n", (nsegs-1));

page_header();

fprintf(optr,"            ");
for(j=1; j <= nfiles; j++)
	fprintf(optr,"%12s", ifile[j].name);
fprintf(optr,"              Totals:\n");

fprintf(optr," from:   to:");
for(j=1; j <= nfiles; j++)
	fprintf(optr,"  Ent. Exits");
fprintf(optr,"    Vol   Ent.  Exits\n");

for(i=1; i <= nsegs; i++) {
	fprintf(optr," %5ld %5ld", sd[i].from, sd[i].to);
	for(j=1; j <= nfiles; j++)
		fprintf(optr," %5.0f %5.0f", sd[i].entries[j], sd[i].exits[j]);
	fprintf(optr," %6.0f %6.0f %6.0f\n", sd[i].totvolume,
		sd[i].totentries,sd[i].totexits);
	}

fprintf(optr,"\nTotals:     ");
for(j=1; j <= nfiles; j++)
	fprintf(optr," %5.0f %5.0f", ifile[j].totent, ifile[j].totext);
fprintf(optr,"        %6.0f %6.0f\n", ifile[0].totent,ifile[0].totext);

  if(show_graph) {
	  barmax = 0;
	  for(i=1; i <= nsegs; i++) {
	switch (graph_val) {
		case 1: fval = sd[i].totvolume;
			break;
		case 2: fval = sd[i].totentries;
			break;
		case 3: fval = sd[i].totexits;
			break;
		}

	if(barmax < fval)
		barmax=fval;
	}

     if (barmax == 0) {
	printf("\nerror: maximum bar size is 0, no a graph cannot be produced\n");
	fprintf(optr,"\nerror: maximum bar size is 0, no a graph cannot be produced\n");
	goto NO_GRAPH;
	}

	  for(i=1; i <= nsegs; i++) {  /* scale bars in array, highest bar = 60 */
	switch (graph_val) {
		case 1: fval = sd[i].totvolume;
		   break;
	   case 2: fval = sd[i].totentries;
		   break;
	   case 3: fval = sd[i].totexits;
		   break;
	   }

	down = (fval * (60/barmax));  up = down + 1;
	barsize[i] = (float)up;
	}

     page_header();
     switch (graph_val) {
	case 1: fprintf(optr,"Graph of Total Volumes:\n");
		break;
	case 2: fprintf(optr,"Graph of Total Entries:\n");
		break;
	case 3: fprintf(optr,"Graph of Total Exits:\n");
		break;
	}

	  fprintf(optr,"Maximum Bar Size = %.0f\n", barmax);
	  fprintf(optr,"Each  *  is %.4f units\n", (barmax/60));

	  fprintf(optr,"\n");
	  if (use_labels) {
	for(i=1; i <= nsegs; i++) {
		onlist = 0;
		for(j=1; j <= nlabels; j++)
			if(nldata[j].num == sd[i].from) {
		 onlist = 1;
		 location = j;
		 }
		if (onlist)
			fprintf(optr," %25s |", nldata[location].label);
		else
			fprintf(optr,"                           |");
		for(j=1; j <= (int) barsize[i]; j++)
			fprintf(optr,"*");
		fprintf(optr,"\n");
		}

	fprintf(optr,"                           ");
	for(i=1; i <= 60; i++)
		if( (i % 15) == 0)
			fprintf(optr,"|");
		else
			fprintf(optr,"-");
	fprintf(optr,"\n");

	fprintf(optr,"                           ");
	for(i=1; i <= 4; i++)
		fprintf(optr,"%15.0f", ((barmax/60)*(i*15)) );
	fprintf(optr,"\n");

	page_header();
	fprintf(optr,"%d Node Labels:\n",nlabels);
	for(i=1; i <= nlabels; i++)
	   fprintf(optr,"%2d. %6ld - %s\n", i, nldata[i].num, nldata[i].label);

	}
     else {
	for(i=1; i <= nsegs; i++) {
	   fprintf(optr," %5ld-%5ld |", sd[i].from, sd[i].to);
	   for(j=1; j <= (int) barsize[i]; j++)
	      fprintf(optr,"*");
	   fprintf(optr,"\n");
	   }

	fprintf(optr,"             -");
	for(i=1; i <= 60; i++)
	   if( (i % 15) == 0)
	      fprintf(optr,"|");
	   else
	      fprintf(optr,"-");
	fprintf(optr,"\n");

	fprintf(optr,"             ");
	for(i=1; i <= 4; i++)
		fprintf(optr,"%15.0f", ((barmax/60)*(i*15)) );

	fprintf(optr,"\n");
	}

	  }

NO_GRAPH:  /* an error was encountered while graphing */

fclose(optr);
free(ifile);
free(sd);
free(sc);
free(barsize);

printf("\nProcessing complete...\n");
printf("Press any key to continue...");
getc(stdin);
printf("\n\n");
return;

} /* end module4() */

/*************************************************************************/
void read_control(void)
{
int  i, c, curr_group, begin, end, retcode;
int  save_group = 0;
int  BufLen;
char buf[MAXLENGTH];
char junk[26], desc[7];
long lval;

FILE *fptr;

/* vehicle data */
if ((ini = (ini_data *) malloc(sizeof(ini_data)) ) == NULL)
	terror("not enough memory to allocate buffer",-1,NULL);

/* assign default values */
TimePeriod=60;
RoundingThreshold=0.3;

ini->type       = 1;
ini->seats      = 50;
ini->targetload = 1.3;
ini->layfactor  = 0.3;


bNodeLabels = 0;
ngrplst = 0;
ngroups = 0;
nlabels = 0;

/* check for *.ini file */
if (file_exists(iniFile))
	bDefFile = 1;
else {
	bDefFile = 0;
	printf("\nFile: %s, not found, default values will be used.\n",iniFile);
	printf("\nPress any key to continue...");
	getc(stdin);
	goto NO_INIFILE;
	}

/* initialize group list structure */
for(i=0; i <= MAXLINES; i++) {
	grplst[i].n[0] = '\0';
	grplst[i].group = 0;
	}

printf("\nReading user data from file: %s...\n", iniFile);

strcpy(buf,"Transit Analysis Data");
TimePeriod =
	GetPrivateProfileInt(buf,"TimePeriod",60,iniFile);
RoundingThreshold =
	GetPrivateProfileFloat(buf,"RoundingThreshold",0.3,iniFile);

strcpy(buf,"Transit Vehicle Data");
ini->type =
	GetPrivateProfileInt(buf,"VehicleType",1,iniFile);
ini->layfactor =
	GetPrivateProfileFloat(buf,"LayoverFactor",0.3,iniFile);
ini->seats =
	GetPrivateProfileInt(buf,"Seats",50,iniFile);
ini->targetload =
	GetPrivateProfileFloat(buf,"TargetLoad",1.3,iniFile);

/* open *.ini file and read to group list */
if( (fptr=fopen(iniFile,"r")) == NULL)
	terror("opening file",-1,NULL);

while( fget_line(buf,99,fptr) != EOF ) {
	BufLen=strlen(buf);
	for(c=0; c < BufLen; c++)
		buf[c] = toupper(buf[c]);

	if( (buf[0] == '[') &&
		 (strncmp("GROUP DEF",&buf[1],9) == 0) ) {

		while( (fget_line(buf,99,fptr) != EOF) &&
		  (toupper(buf[0]) == 'G') ) {

	 retcode = sscanf(buf," %s %d %s ", junk, &curr_group, desc);

	 /* do some checking */
	 if (retcode < 3) {
		 printf("\nerror: group list format error\n");
	    printf( "=> %s\n",buf);
	    pause_exit();
	    }
	 if ( (curr_group < 0) || (curr_group > MAXGROUPS) ) {
	    printf("\nerror: group list number error\n");
	    printf( "=> %s\n",buf);
	    pause_exit();
	    }

	 /* this counts the number of groups - they must be sorted
		 in ascending order.  Group zero is not counted */
	 if(curr_group != save_group) {
		 ngroups++;
	    save_group = curr_group;
	    }

	 ngrplst++;
	 grplst[ngrplst].group = curr_group;

	 strcpy(grplst[ngrplst].n, desc);
	 grplst[ngrplst].n[strlen(desc)] = '\0';
	 } /* while */
      } /* if */
   } /* while */
fclose(fptr);

/* open *.ini file and read to node label list */
if( (fptr=fopen(iniFile,"r")) == NULL)
	terror("opening file",-1,NULL);

while(fget_line(buf,99,fptr) != EOF) {
	BufLen=strlen(buf);
	for(c=0; c < BufLen; c++)
		buf[c] = toupper(buf[c]);

	if( (buf[0] == '[') &&
		 (strncmp("NODE LABE",&buf[1],9) == 0) ) {

		while( (fget_line(buf,99,fptr) != EOF) &&
		  (buf[0] != '[') && (buf[0] != ' ') ) {

	 retcode = sscanf(buf,"%ld  %s", &lval, junk);

	 /* find label and pull it out */
	 c = begin = end = 0;
	 while ( (buf[c] != '"') && (c < strlen(buf)) )
	    c++;

	 c++;      /* move to next column */
	 begin=c;  /* mark first " */

	 while ( (buf[c] != '"') && (c < strlen(buf)) )
	    c++;
	 end=c;    /* mark last " */

	 /* Note: the node labels do not need to be sorted */
	 nlabels++;
	 nldata[nlabels].num = lval;


	 /* do some checking */
	 if (begin != end) {
	    if( (end-begin) > 25) {
	       strncpy(nldata[nlabels].label, &buf[begin], 25);
	       nldata[nlabels].label[25] = '\0';
	       }
	    else {
	       strncpy(nldata[nlabels].label, &buf[begin], (end-begin));
	       nldata[nlabels].label[end-begin] = '\0';
	       }
	    }
	 else
	    nldata[nlabels].label[0] = '\0';

	 } /* while */
      } /* if */

	if (nlabels >= 1 )  bNodeLabels = 1;

   } /* while */
fclose(fptr);

NO_INIFILE:

free(buf);

} /* end read_control() */

/*************************************************************************/
void run_header(void)
{
int i;
char buf[MAXLENGTH];

sprintf(buf,"TRANSTAT (TM) - TRANSIT ROUTE ANAYSIS PROGRAM           Ver %s  Rel %s", VER, REL);
printf("\n%s\n",buf);
for (i=1; i <= strlen(buf); i++)
	putc('-',stdout);
printf("\n");
}

/***************************************************************************/
void page_header(void)
{
time_t t;

time(&t);

fprintf(optr,"\f\nTRANSTAT (TM) - TRANSIT ROUTE ANAYSIS PROGRAM             Ver %s  Rel %s\n", VER, REL);
fprintf(optr,"\nSYSTEM DATE: %s",ctime(&t));
fprintf(optr,"--------------------------------------------------------------------------------\n");
fprintf(optr,"\n");
}
