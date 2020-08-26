//---------------------------------------------------------------------------

//#pragma hdrstop

//---------------------------------------------------------------------------

//#pragma argsused
/*
 * DRVLINKS.C
 * written by   :  Tim Heier
 * last updated :  07/07/96
 */

#define VER          "2.11"
#define REL      " 3/12/08"
#define BIT            "32"


/* 
 *	Revisions started from version:
 *		#define VER          "2.00"
 *		#define REL      " 7/07/96"
 *		#define BIT            "32"

 *	Revisions:
 *		2.01	14feb2007	
 *		Resolved cast warnings regarding int/float, float/double, etc. type mismatches (roughly 6 resolved).
 *		Changed parameters passed in read_ini() function calls from constant char strings ("string") 
 *		to char[] holding the value of the "string", otherwise got runtime errors.
 *		Added read_mplines() function to read Minutp format transit lines file and added it as a Transit Format option.
 *
 *		2.02	20feb2007	
 *		Added a Minutp output formst for the access link records.
 *
 *		2.03	15mar2007	
 *		Corrected bug in original code for MANHATTEN distance - previously summed square of right angle side distances
 *		instead of summing just the distances themselves.
 *		Added a function to read a Minutp format station file.
 *		Added processing station nodes as well as regular highway nodes in drvlinks() function.
 *		Added a nodeIndex[] correspondence array.
 *		Only stations that have dP flag set to one in station file are eligible for drive access links.
 *
 *		2.04	16mar2007
 *		Corrected bugs in original ctlutil.cpp that prevented Exception from being handled correctly
 *		when files were opened and the filename specified could not be found.
 *		Added function to read highway link records and mark anodes for which periods link is available.
 *		Wrote drive access links to separate files by period.
 *
 *		2.05	12sep2007
 *		Corrected bug where drive access links were written as a regular node when within range of a centroid
 *		when node was not specified in node file.
 *
 *		2.06	13sep2007
 *		Corrected bug related to last revision where node is skipped if it's not in node file AND it's
 *		not a station.
 *
 *		2.07	14sep2007
 *		Corrected bug where limita, limitm, limitp values were previously used to make anodes eligible, but should
 *		have been done with bnodes.
 *
 *		2.08	17sep2007
 *		Corrected bug where node coordinates were by default (0,0) when node didn't exist in nodes file, when node
 *		should have really been skipped over.
 *
 *		2.10	11mar2008
 *		Starting from drvlinks.c that was prepared by Joel Freedman based on drvlinks.cpp (2.08).
 *		Revising to write the limited set of access links from a sorted (by distance) set.
 *		Also modified the reporting of connected/unconnected nodes in .rpt file.
 *
 *		2.11	12mar2008
 *		Fixed bug in saving all the output link record info before sorting, sorting, then
 *		only writing out the max records specified.
 *
 */

char *ProgName = "DRVLINKS";

/*
The basic program logic goes like this:

1. Start from a centroid and find all the nodes within the search values.
2. The canidate nodes are then sorted by distance from the centroid.
3. The node closest to the centroid is accepted into the final set.
4. Subsequent nodes are accepted into the final set if the multiple
	links option is on.
*/

#include "stdutil.h"
#include "ctlutil.h"

#define MAXSTR         120
#define CARTESIAN        0
#define MANHATTEN        1

FILE  *iptr, *rptr, *optr, **mptr;

void  read_ini(void);
void  read_cents(void);
void  read_m2cents(void);
void  read_nodes(void);
void  read_m2nodes(void);
void  read_mpstations(void);
void  read_mplinks(void);
void  drvlinks(void);
void  write_link(long cent, long cnode, float dist);
void  usage(void);
void  run_header(void);
void  rpt_header(void);
void  pick_frto(char *tempbuf, int *from, int *to, int *length);

/* global variables */
int   CentCount, NodeCount, LinkCount;
char  *linebuf;
char  *StationFile, *iniFile, *CentroidFile, *NodeFile, *LinkFile, *HwyLinkFile, *ReportFile;

int   NumCentroids, NumNodes, MaxStations, MaxBus;
float NetworkScale,MinDistance,MaxDistance,DriveSpeed;

int   PrintUnconnected,PrintConnections,DryRun;
int   IndividualSearch,MultipleLinks;
int   FirstCentroid,FirstNode;

int   DeleteLinks,Distance;
char  NodeFormat,CentroidFormat,LinkFormat;

typedef struct {
	long   num;
	float  x;
	float  y;
	long   connected;
} cent_data;

typedef struct {
	int	station;
	float x;
	float y;
	int	hwy[2];
	int d[2];
} stn_data;

typedef struct {
	long   num;
	float  x;
	float  y;
	float  mindist;
	float  maxdist;
	int    station;
	long   connected;
	float  padding2;
	float  padding3;
	int    periods[3];
} node_data;

typedef struct {
	int    type;
	int    vdf;
	char   modesin[MAXSTR];
	char   modesout[MAXSTR];
	char   direction;
	float  lanes;
} link_data;

typedef struct {
	int    from;
	int    to;
	int    length;
} col_data;

int		   *nodeIndex;
cent_data  *cent;
node_data  *node;
link_data  newlink;
stn_data   *stn;
col_data   fnode, fnodex, fnodey, fnodeu1, fnodeu2;
col_data   fcent, fcentx, fcenty;
char   *periodLabels[] = { "am", "md", "pm" };

int main(int argc, char* argv[])
{

int i, j, index;
char   temp[MAXSTR];



if(argc < 2) {
	usage();
	exit(1);
	}

/* allocate memory for strings */
CentroidFile = (char *) malloc(MAXSTR);
NodeFile     = (char *) malloc(MAXSTR);
LinkFile     = (char *) malloc(MAXSTR);
HwyLinkFile  = (char *) malloc(MAXSTR);
StationFile  = (char *) malloc(MAXSTR);
ReportFile   = (char *) malloc(MAXSTR);
iniFile      = (char *) malloc(MAXSTR);
linebuf      = (char *) malloc(MAXSTR);

/* allocate memory for character arrays */
if( (CentroidFile == NULL) || (NodeFile == NULL) || (LinkFile == NULL) || (HwyLinkFile == NULL) ||
	 (ReportFile == NULL) || (iniFile == NULL) || (linebuf == NULL)  || (StationFile == NULL) ) {
	printf("\nerror: not enough memory to allocate string buffers\n");
	exit(1);
	}

strcpy(iniFile,argv[1]);

run_header();
read_ini();

if(DryRun)
	exit(1);

/* allocate memory for data */
if( (cent = (cent_data *) calloc( NumCentroids+1, sizeof(cent_data))) == NULL ) {
	printf("\nerror: allocating centroid buffer\n");
	exit(1);
			}
if( (node = (node_data *) calloc( NumNodes+1, sizeof(node_data))) == NULL ) {
	printf("\nerror: allocating node buffer\n");
	exit(1);
	}

if( (stn = (stn_data *) calloc( NumNodes+1, sizeof(stn_data))) == NULL ) {
	printf("\nerror: allocating station buffer\n");
	exit(1);
	}

if( (nodeIndex = (int *) calloc( NumNodes+1, sizeof(int))) == NULL ) {
	printf("\nerror: allocating node to index correspondence buffer\n");
	exit(1);
	}


if(CentroidFormat == 'e')
	read_m2cents();
else
	read_cents();

if(NodeFormat == 'e')
	read_m2nodes();
else
	read_nodes();

fprintf(rptr,"\nRUN RESULTS\n");
fprintf(rptr,  "------------\n");
fprintf(rptr,"\nCENTROIDS READ = %d\n",CentCount);
fprintf(rptr,  "NODES READ     = %d\n",NodeCount);



if ( LinkFormat == 'm' ) {
	/* if there's a station file specified, read stations.  The file, and station processing is optional */
	if (strcmp(StationFile, "__Optional__") != 0)
		read_mpstations();

	/* read the highway network link file and mark each node's time period if that node is a link anode
	and is available in the time period */
	read_mplinks();


	/* allocate FILE* array, name and open output files */
	if( (mptr = (FILE**) calloc( 3, sizeof(FILE *))) == NULL ) {
		printf("\nerror: allocating array of output link file handles\n");
		exit(1);
		}

	for (i=0; i < 3; i++) {
		index = 0;
		if (str_index(LinkFile, ".") >= 0) {
			for (j=strlen(LinkFile); j >= 0; j--)
				if(LinkFile[j] == '.') {
					index = j;
					break;
					}
			}
		else {
			index = strlen(LinkFile);
			}

		for (j=0; j < index; j++)
			temp[j] = LinkFile[j];
		sprintf(&temp[index], "_%s", periodLabels[i]);
		for (j=index; j < (signed)strlen(LinkFile); j++)
			temp[j+strlen(periodLabels[i])+1] = LinkFile[j];
		temp[strlen(LinkFile)+strlen(periodLabels[i])+1] = '\0';
		if( (mptr[i] = fopen(temp,"w")) == NULL ) {
			printf("\nerror: opening file \"%s\" for writing.\n", temp);
			exit(1);
			}
		}
	}

else if( (optr = fopen(LinkFile,"w")) == NULL ) {
	printf("\nerror: opening file \"%s\" for writing.\n",LinkFile);
	exit(1);
	}


drvlinks();

return(0);
}

/*************************************************************************/
void read_ini(void)
{
char c, tempbuf[MAXSTR];
char entryBuf[MAXSTR];
char defaultBuf[MAXSTR];

/* FILES Section */
strcpy(linebuf,"Files");

strcpy(entryBuf,"CentroidFile");
strcpy(defaultBuf,"zonecents.in");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, CentroidFile, iniFile);

strcpy(entryBuf,"NodeFile");
strcpy(defaultBuf,"hwynodes.in");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, NodeFile, iniFile);

strcpy(entryBuf,"HwyLinkFile");
strcpy(defaultBuf,"hwylinks.in");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, HwyLinkFile, iniFile);

strcpy(entryBuf,"LinkFile");
strcpy(defaultBuf,"drvlinks.out");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, LinkFile, iniFile);

strcpy(entryBuf,"ReportFile");
strcpy(defaultBuf,"drvlinks.rpt");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, ReportFile, iniFile);

strcpy(entryBuf,"StationFile");
strcpy(defaultBuf,"__Optional__");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, StationFile, iniFile);


/* open report file */
if( (rptr = fopen(ReportFile,"w")) == NULL ) {
	printf("\nerror: opening file \"%s\" for writing.", ReportFile);
	exit(1);
	}
rpt_header();

/* open control file */
if( (iptr = fopen(iniFile,"r")) == NULL ) {
	printf("\nerror: opening file \"%s\" for reading.", iniFile);
	exit(1);
	}


/* echo control file to report file */
fprintf(rptr,"\nCONTROL FILE\n");
fprintf(rptr,  "-------------\n\n");
while( (c = getc(iptr) ) != EOF)
	putc(c, rptr);
fclose(iptr);



/* PARAMETERS Section */
strcpy(linebuf,"Parameters");

strcpy(entryBuf,"DryRun");
DryRun = GetPrivateProfileBool(linebuf, entryBuf, 0, iniFile);

strcpy(entryBuf,"NumCentroids");
NumCentroids = GetPrivateProfileInt(linebuf, entryBuf, 2000, iniFile);

strcpy(entryBuf,"NumNodes");
NumNodes = GetPrivateProfileInt(linebuf, entryBuf, 500, iniFile);

strcpy(entryBuf,"MaxStations");
MaxStations = GetPrivateProfileInt(linebuf, entryBuf, 4, iniFile);

strcpy(entryBuf,"MaxBus");
MaxBus = GetPrivateProfileInt(linebuf, entryBuf, 4, iniFile);

strcpy(entryBuf,"Distance");
strcpy(defaultBuf,"cartesian");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
if(toupper(tempbuf[0]) == 'C')
	Distance = CARTESIAN;
else
	Distance = MANHATTEN;

strcpy(entryBuf,"IndividualSearch");
IndividualSearch = GetPrivateProfileBool(linebuf, entryBuf, 0, iniFile);

strcpy(entryBuf,"MinDistance");
MinDistance = GetPrivateProfileFloat(linebuf, entryBuf, 0.0, iniFile);

strcpy(entryBuf,"MaxDistance");
MaxDistance = GetPrivateProfileFloat(linebuf, entryBuf, 3.0f, iniFile);

strcpy(entryBuf,"NetworkScale");
NetworkScale = GetPrivateProfileFloat(linebuf, entryBuf, 1.0, iniFile);

strcpy(entryBuf,"MultipleLinks");
MultipleLinks = GetPrivateProfileBool(linebuf, entryBuf, 1, iniFile);

strcpy(entryBuf,"DriveSpeed");
DriveSpeed = GetPrivateProfileFloat(linebuf, entryBuf, 20.0, iniFile);



if(NumCentroids <= 0) {
	printf("\nerror: invalid number of centroids=%d.\n", NumCentroids);
	exit(1);
	}
if(NumNodes <=  0) {
	printf("\nerror: invalid number of nodes=%d.\n", NumNodes);
	exit(1);
	}
if(NetworkScale == 0) {
	printf("\nerror: parameter *NetworkScale* cannot be zero.\n");
	exit(1);
	}

if(MaxBus == 0 && MaxStations==0) {
	printf("\nerror: either parameter *MaxBus* or *MaxStations* cannot be zero.\n");
	exit(1);
	}
/* REPORTS Section */
strcpy(linebuf,"Reports");

strcpy(entryBuf,"PrintUnconnected");
PrintUnconnected = GetPrivateProfileBool(linebuf, entryBuf, 1, iniFile);

strcpy(entryBuf,"PrintConnections");
PrintConnections = GetPrivateProfileBool(linebuf, entryBuf, 1, iniFile);



/* CENTROID FORMAT Section */
strcpy(linebuf,"Centroid Format");

strcpy(entryBuf,"CentroidFormat");
strcpy(defaultBuf,"u");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
CentroidFormat = tolower(tempbuf[0]);
if( (CentroidFormat != 'u') && (CentroidFormat != 'e') ) {
	printf("\nerror: invalid *CentroidFormat* entry = %c\n",CentroidFormat);
	exit(1);
	}

if(CentroidFormat == 'u') {
	strcpy(entryBuf,"Number");
	strcpy(defaultBuf,"\0");
	GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
	pick_frto(tempbuf, &fcent.from, &fcent.to, &fcent.length);

	strcpy(entryBuf,"XCoord");
	strcpy(defaultBuf,"\0");
	GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
	pick_frto(tempbuf, &fcentx.from, &fcentx.to, &fcentx.length);

	strcpy(entryBuf,"YCoord");
	strcpy(defaultBuf,"\0");
	GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
	pick_frto(tempbuf, &fcenty.from, &fcenty.to, &fcenty.length);

	}

strcpy(entryBuf,"FirstCentroid");
FirstCentroid = GetPrivateProfileInt(linebuf, entryBuf, 1, iniFile);



/* NODE FORMAT Section */
strcpy(linebuf,"Node Format");

strcpy(entryBuf,"NodeFormat");
strcpy(defaultBuf,"u");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
NodeFormat = tolower(tempbuf[0]);
if( (NodeFormat != 'u') && (NodeFormat != 'e') ) {
	printf("\nerror: invalid *NodeFormat* entry = %c\n",NodeFormat);
	exit(1);
	}

if(NodeFormat == 'u') {
	strcpy(entryBuf,"Number");
	strcpy(defaultBuf,"\0");
	GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
	pick_frto(tempbuf, &fnode.from, &fnode.to, &fnode.length);

	strcpy(entryBuf,"XCoord");
	strcpy(defaultBuf,"\0");
	GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
	pick_frto(tempbuf, &fnodex.from, &fnodex.to, &fnodex.length);

	strcpy(entryBuf,"YCoord");
	strcpy(defaultBuf,"\0");
	GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
	pick_frto(tempbuf, &fnodey.from, &fnodey.to, &fnodey.length);

    if (IndividualSearch) {
		strcpy(entryBuf,"User1");
		strcpy(defaultBuf,"\0");
		GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
		pick_frto(tempbuf, &fnodeu1.from, &fnodeu1.to, &fnodeu1.length);

		strcpy(entryBuf,"User2");
		strcpy(defaultBuf,"\0");
		GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
		pick_frto(tempbuf, &fnodeu2.from, &fnodeu2.to, &fnodeu2.length);
	}

	}

strcpy(entryBuf,"FirstNode");
FirstNode = GetPrivateProfileInt(linebuf, entryBuf, 1, iniFile);




/* NEW LINK ATTRIBUTES Section */
strcpy(linebuf,"New Link Attributes");

strcpy(entryBuf,"DeleteLinks");
DeleteLinks = GetPrivateProfileBool(linebuf, entryBuf, 0, iniFile);

strcpy(entryBuf,"Lanes");
newlink.lanes = GetPrivateProfileFloat(linebuf, entryBuf, 1.0, iniFile);

strcpy(entryBuf,"LinkType");
newlink.type = GetPrivateProfileInt(linebuf, entryBuf, 99, iniFile);

strcpy(entryBuf,"VolumeDelay");
newlink.vdf = GetPrivateProfileInt(linebuf, entryBuf, 99, iniFile);

/* check for old "modes" entry for one-way links */
strcpy(entryBuf,"Modes");
strcpy(defaultBuf,"p");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, newlink.modesout, iniFile);

if(strlen(newlink.modesout) >= 1)
	newlink.direction = 'o';
else {
	strcpy(entryBuf,"ModesIn");
	strcpy(defaultBuf,"p");
	GetPrivateProfileString(linebuf, entryBuf, defaultBuf, newlink.modesin, iniFile);

	strcpy(entryBuf,"ModesOut");
	strcpy(defaultBuf,"q");
	GetPrivateProfileString(linebuf, entryBuf, defaultBuf, newlink.modesout, iniFile);

	strcpy(entryBuf,"Direction");
	strcpy(defaultBuf,"outbound");
	GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
	}

strcpy(entryBuf,"LinkFormat");
strcpy(defaultBuf,"u");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
LinkFormat = tolower(tempbuf[0]);
if( (LinkFormat != 'e') && (LinkFormat != 't') && (LinkFormat != 'i') && (LinkFormat != 'm') ) {
	printf("\nerror: invalid *LinkFormat* entry = %s\n",tempbuf);
	exit(1);
	}



/* check link entries */

newlink.direction = tolower(newlink.direction);
if( (newlink.direction != 't') && (newlink.direction != 'i') &&
		(newlink.direction != 'o') ) {
	printf("\nerror: invalid *Direction* entry = %s\n",tempbuf);
	exit(1);
	}

if(MinDistance > MaxDistance) {
	printf("\nerror: The Minimum search distance is greater then the Maximum.\n");
	exit(1);
	}

if( (LinkFormat == 't') || (LinkFormat == 'i') || (LinkFormat == 'm') ) {
	DeleteLinks = FALSE;
	}

}

/*************************************************************************/
void read_cents(void)
{
int  lines;

CentCount = lines = 0;

if( (iptr = fopen(CentroidFile,"r")) == NULL ) {
	printf("\nerror: opening file \"%s\" for reading.\n",CentroidFile);
	exit(1);
	}

while( (lines+1) < FirstCentroid) {
	fget_line(&linebuf[1],MAXSTR,iptr);
	lines++;
	}

while( (fget_line(&linebuf[1],MAXSTR,iptr)) != EOF) {
	lines++;
	if(strlen(&linebuf[1]) > 5) {
		CentCount++;
		if(CentCount > NumCentroids) {
			printf("\nerror: more than %d centroids found.\n", NumCentroids);
			exit(1);
			}

		cent[CentCount].num = getlong(&linebuf[1],fcent.from,fcent.length);
		cent[CentCount].x   = getfloat(&linebuf[1],fcentx.from,fcentx.length);
		cent[CentCount].y   = getfloat(&linebuf[1],fcenty.from,fcenty.length);

		cent[CentCount].x = cent[CentCount].x / NetworkScale;
		cent[CentCount].y = cent[CentCount].y / NetworkScale;

		if(cent[CentCount].num == 0) {
			printf("\nerror: reading centroid data at line %d.\n", lines);
			exit(1);
			}
		}
	}

fclose(iptr);
}

/*************************************************************************/
void read_nodes(void)
{
int  lines;

NodeCount = lines=0;

if( (iptr = fopen(NodeFile,"r")) == NULL ) {
	printf("\nerror: opening file \"%s\" for reading.\n",NodeFile);
	exit(1);
	}

while( (lines+1) < FirstNode) {
	fget_line(&linebuf[1],MAXSTR,iptr);
	lines++;
	}

while( (fget_line(&linebuf[1],MAXSTR,iptr)) != EOF) {
	lines++;
	if(strlen(&linebuf[1]) > 5) {
		NodeCount++;
		if(NodeCount > NumNodes) {
			printf("\nerror: more than %d nodes found.\n", NumNodes);
			exit(1);
			}

		node[NodeCount].num = getlong(&linebuf[1],fnode.from,fnode.length);
		node[NodeCount].x   = getfloat(&linebuf[1],fnodex.from,fnodex.length);
		node[NodeCount].y   = getfloat(&linebuf[1],fnodey.from,fnodey.length);
		node[NodeCount].mindist
				= getfloat(&linebuf[1],fnodeu1.from,fnodeu1.length);
		node[NodeCount].maxdist
				= getfloat(&linebuf[1],fnodeu2.from,fnodeu2.length);

		node[NodeCount].x = node[NodeCount].x / NetworkScale;
		node[NodeCount].y = node[NodeCount].y / NetworkScale;

		nodeIndex[node[NodeCount].num] = NodeCount;

		if(node[NodeCount].num == 0) {
			printf("\nerror: reading node data at line %d.\n", lines);
			exit(1);
			}
		}
	}

fclose(iptr);
}

/*************************************************************************/
void drvlinks(void)
{

void isort (int *idata, int *index, int elements);


int l, m, n, numRecords;
int    i, j, k, debug=0, totalStationConnections,totalBusConnections,tempN;
int    BestNode, BestNodeIndex, Printed, index;
double BestDist, X, Y, Xs, Ys, X1, Y1, X2, Y2;
double mindist, maxdist, dist, distS, dist1, dist2;

int* sortIndex;
int* stnSortData;
int* busSortData;
double** stnAccessLinkInfo;
double** busAccessLinkInfo;

sortIndex = (int*)calloc(NumNodes, sizeof(int));
stnSortData = (int*)calloc(NumNodes, sizeof(int));
busSortData = (int*)calloc(NumNodes, sizeof(int));
stnAccessLinkInfo = (double**)calloc(NumNodes, sizeof(double*));
busAccessLinkInfo = (double**)calloc(NumNodes, sizeof(double*));
for (i=0; i < NumNodes; i++) {
	stnAccessLinkInfo[i] = (double*)calloc(2, sizeof(double));
	busAccessLinkInfo[i] = (double*)calloc(2, sizeof(double));
}


/* create output file */
if( (optr = fopen(LinkFile,"w")) == NULL ) {
	printf("\nerror: opening file \"%s\" for writing.\n",LinkFile);
	exit(1);
	}

if(LinkFormat == 'e') {
	fprintf(optr,"c========================================\n");
	fprintf(optr,"c %s (TM)  Ver %s  Rel  %s\n", ProgName, VER, REL);
	fprintf(optr,"c========================================\n");
	fprintf(optr,"t links\n");
	}

/* set global search values */
mindist = MinDistance;
maxdist = MaxDistance;

/* loop through all the centroids and connect them to nodes */
LinkCount = 0;
printf("\nProcessing Zone:\n");

for (i=1; i <= CentCount; i++) {

	printf("%ld\r",cent[i].num);

	BestNode = 0;
	BestDist = 999999999;

    totalStationConnections=0;
    totalBusConnections=0;

	// zero out the arrays used to store candidate link info for sorting
	n = 0;
	m = 0;
	for (k=0; k < NumNodes; k++) {
		stnSortData[k] = 0;
		busSortData[k] = 0;
		for (j=0; j < 2; j++) {
			stnAccessLinkInfo[k][j] = 0.0;
			busAccessLinkInfo[k][j] = 0.0;
		}
	}

	/* loop from 1 to highest possible node number to find those within the search distance */
	for (k=1; k <= NumNodes; k++) {

		/* get the node[] index from the node number */
		j = nodeIndex[k];

		if ( node[j].num == 0 && stn[k].station == 0 )
			continue;

		//if (  cent[i].num == 576 && stn[k].station == 4503 ) {
		//	debug = 1;
		//}
		if (  cent[i].num == 71 && stn[k].station > 0 ) {
			debug = 1;
		}

		/* calculate right horizontal and verticals distances */
		X =  -1.0;
		Y =  -1.0;
		Xs = -1.0;
		Ys = -1.0;
		X1 = -1.0;
		Y1 = -1.0;
		X2 = -1.0;
		Y2 = -1.0;
		if (stn[k].station == 0) {
			/* regular highway node */
			X = fabs((node[j].x - cent[i].x));
			Y = fabs((node[j].y - cent[i].y));

			/* set search values */
			if(IndividualSearch > 0) {
				mindist = node[j].mindist;
				maxdist = node[j].maxdist;
				}

			}
		else {
			/* station node */
			Xs = fabs((stn[k].x - cent[i].x));
			Ys = fabs((stn[k].y - cent[i].y));
			index = nodeIndex[stn[k].hwy[0]];
			if ( node[index].num > 0 ) {
				X1 = fabs((node[index].x - cent[i].x));
				Y1 = fabs((node[index].y - cent[i].y));
				if (stn[k].hwy[1] > 0) {
					index = nodeIndex[stn[k].hwy[1]];
					if ( node[index].num > 0 ) {
						X2 = fabs((node[index].x - cent[i].x));
						Y2 = fabs((node[index].y - cent[i].y));
						}
					}
				}
			}

                dist  = -1.0;
                distS = -1.0;
                dist1 = -1.0;
                dist2 = -1.0;
		switch(Distance) {
			case CARTESIAN :
				/* calculate distance along hypotenuse */
				if ( X >= 0.0 && Y >= 0.0)
					dist = sqrt(X*X + Y*Y);
				if ( Xs >= 0.0 && Ys >= 0.0)
					distS = sqrt(Xs*Xs + Ys*Ys);
				if ( X1 >= 0.0 && Y1 >= 0.0)
					dist1 = sqrt(X1*X1 + Y1*Y1);
				if ( X2 >= 0.0 && Y2 >= 0.0)
					dist2 = sqrt(X2*X2 + Y2*Y2);
				break;
			case MANHATTEN :
				/* calculate right angle distance */
				if ( X >= 0.0 && Y >= 0.0)
					dist = X + Y;
				if ( Xs >= 0.0 && Ys >= 0.0)
					distS = Xs + Ys;
				if ( X1 >= 0.0 && Y1 >= 0.0)
					dist1 = X1 + Y1;
				if ( X2 >= 0.0 && Y2 >= 0.0)
					dist2 = X2 + Y2;
				break;
			default        :
				/* default is distance along hypotenuse */
				if ( X >= 0.0 && Y >= 0.0)
					dist = sqrt(X*X + Y*Y);
				if ( Xs >= 0.0 && Ys >= 0.0)
					distS = sqrt(Xs*Xs + Ys*Ys);
				if ( X1 >= 0.0 && Y1 >= 0.0)
					dist1 = sqrt(X1*X1 + Y1*Y1);
				if ( X2 >= 0.0 && Y2 >= 0.0)
					dist2 = sqrt(X2*X2 + Y2*Y2);
				break;
			}



		/* if the node is a station node, add extra distance from station file records to direct station access link 
		   and write access links to station and to highway nodes associated with stations */
		if (stn[k].station != 0) {

			/* write out link to station if node in search area */
			if( (distS >= mindist) && (distS <= maxdist)) {
				
				if( MultipleLinks ) {

					// get an integer value for the distance.  multiply by 1000 to include at least 3 decimal places in sort criteria.
					stnSortData[n] = (int)(distS*1000 + 0.5);
					stnAccessLinkInfo[n][0] = k;
					stnAccessLinkInfo[n][1] = (distS + stn[k].d[0]/100.0);
		            totalStationConnections +=1;
					n++;

					//write_link(cent[i].num, stn[k].station + 200, (float)(distS + stn[k].d[0]/100.0));
					//cent[i].connected += 1;
					//node[stn[k].station].connected += 1;
                    //totalStationConnections +=1;
					}

				/* keep track of best node and write out later */
				else {
					if(distS < BestDist) {
						BestNodeIndex = k;
						BestNode = stn[k].station;
						BestDist = distS;
						}
					}

				} // if distS


			/** this code probably should have never been here... unclear.

			// write out link to 1st station highway node in search area
			if( (dist1 >= mindist) && (dist1 <= maxdist) ) {
				
				index = nodeIndex[stn[k].hwy[0]];
				if(MultipleLinks) {
					write_link(cent[i].num, stn[k].hwy[0], (float)(dist1));
					cent[i].connected += 1;
					node[index].connected += 1;
					}

				// keep track of best node and write out later
				else {
					if(distS < BestDist) {
						BestNodeIndex = index;
						BestNode = stn[k].hwy[0];
						BestDist = dist1;
						}
					}

				} // if dist1

			// write out link to 2nd station highway node if it exists and is in search area
			if( stn[k].hwy[1] > 0 && (dist2 >= mindist) && (dist2 <= maxdist)  ) {

				index = nodeIndex[stn[k].hwy[1]];
				if(MultipleLinks ) {
					write_link(cent[i].num, stn[k].hwy[1], (float)(dist2));
					cent[i].connected += 1;
					node[index].connected += 1;
					}

				// keep track of best node and write out later
				else {
					if(distS < BestDist) {
						BestNodeIndex = index;
						BestNode = stn[k].hwy[1];
						BestDist = dist2;
						}
					}

				} // if dist2


			**/

			}

		else {	/* regular highway node */

			/* write out link if node in search area */
			if( (dist >= mindist) && (dist <= maxdist) ) {

				if(MultipleLinks ) {

					// get an integer value for the distance.  multiply by 1000 to include at least 3 decimal places in sort criteria.
					busSortData[m] = (int)(dist*1000 + 0.5);
					busAccessLinkInfo[m][0] = j;
					busAccessLinkInfo[m][1] = dist;
		            totalBusConnections +=1;
					m++;

					//write_link(cent[i].num, node[j].num, (float)dist);
					//cent[i].connected += 1;
					//node[j].connected += 1;
                    //totalBusConnections +=1;

					}

				/* keep track of best node and write out later */
				else {
					if(dist < BestDist) {
						BestNodeIndex = j;
						BestNode = node[j].num;
						BestDist = dist;
						}
					}
				} /* if dist */

			} /* if station*/

		} /* for NumNodes */


	/* write out the best link to best node */
	if(! MultipleLinks) {

		if(BestNode != 0) {
			write_link(cent[i].num, BestNode, (float)BestDist);
			cent[i].connected += 1;
			node[BestNodeIndex].connected += 1;
			}
		}

	else {

		isort (stnSortData, sortIndex, totalStationConnections);
		numRecords =  totalStationConnections < MaxStations ? totalStationConnections : MaxStations;
		for (l=0; l < numRecords; l++) {
			k = (int)stnAccessLinkInfo[sortIndex[l]][0];
			dist = stnAccessLinkInfo[sortIndex[l]][1];
			write_link(cent[i].num, stn[k].station + 200, (float)dist);
			cent[i].connected += 1;
			node[stn[k].station].connected += 1;
			}

		isort (busSortData, sortIndex, totalBusConnections);
		numRecords =  totalBusConnections < MaxBus ? totalBusConnections : MaxBus;
		for (l=0; l < numRecords; l++) {
			j = (int)busAccessLinkInfo[sortIndex[l]][0];
			dist = busAccessLinkInfo[sortIndex[l]][1];
			write_link(cent[i].num, node[j].num, (float)dist);
			cent[i].connected += 1;
			node[j].connected += 1;
			}

		}


	} /* for CentCount */

printf("\n");
fprintf(rptr,"LINKS WRITTEN  = %d\n",LinkCount);

if(PrintUnconnected) {
	fprintf(rptr,"\nUNCONNECTED CENTROIDS\n");
	fprintf(rptr,  "----------------------\n\n");
	Printed = 0;
	for (i=1; i <= CentCount; i++) {
		if(cent[i].connected == 0) {
			fprintf(rptr," %6ld",cent[i].num);
			Printed++;
			if( Printed % 10 == 0)
				fprintf(rptr,"\n");
			}
		}
	fprintf(rptr,"\n");

	fprintf(rptr,"\nUNCONNECTED NODES\n");
	fprintf(rptr,  "------------------\n\n");
	Printed = 0;
	for (i=1; i <= NumNodes; i++) {
		if(node[i].connected == 0) {

			j = nodeIndex[i];
			if ( node[j].num == 0 && stn[i].station == 0 )
				continue;

			if ( node[j].num > 0 )
				tempN = node[j].num;
			else
				tempN = stn[i].station;

			fprintf(rptr," %6ld", tempN);
			Printed++;
			if( Printed % 10 == 0)
				fprintf(rptr,"\n");
			}
		}
	fprintf(rptr,"\n");

	} /* if  PrintUnconnected */

if(PrintConnections) {
	fprintf(rptr,"\nNUMBER OF LINKS/CENTROID\n");
	fprintf(rptr,  "-------------------------\n\n");
	Printed = 0;
	for (i=1; i <= CentCount; i++) {
		if(cent[i].connected > 0) {
			fprintf(rptr," %6ld = %6ld |", cent[i].num, cent[i].connected);
			Printed++;
			if( Printed % 4 == 0)
				fprintf(rptr,"\n");
			}
		}
	fprintf(rptr,"\n");

	fprintf(rptr,"\nNUMBER OF LINKS/NODE\n");
	fprintf(rptr,  "---------------------\n\n");
	Printed = 0;
	for (i=1; i <= NumNodes; i++) {
		if(node[i].connected > 0) {

			j = nodeIndex[i];
			if ( node[j].num == 0 && stn[i].station == 0 )
				continue;

			if ( node[j].num > 0 )
				tempN = node[j].num;
			else
				tempN = stn[i].station;

			fprintf(rptr," %6ld = %6ld |", tempN, node[i].connected);
			Printed++;
			if( Printed % 4 == 0)
				fprintf(rptr,"\n");
			}
		}
	fprintf(rptr,"\n");

	} /* if PrintConnections */

fclose(optr);
}

/*************************************************************************/
void usage(void)
{

run_header();

printf("\nSee written documentation for further details.\n");
printf("\nUSAGE:  %s  [control-file]\n", ProgName);
}

/*************************************************************************/
void  run_header()
{
int i;

printf("\n");
printf("%s (TM) TRANSIT DRIVE LINK GENERATION PROGRAM\n",ProgName);
printf("Ver %s  Rel %s  (%s-Bit Version)\n", VER, REL, BIT);
for(i=1; i < 53; i++)
	printf("-");
printf("\n");
}

/*************************************************************************/
void  rpt_header()
{
int i;

fprintf(rptr,"%s (TM) TRANSIT DRIVE LINK GENERATION PROGRAM\n",ProgName);
fprintf(rptr,"Ver %s  Rel %s  (%s-Bit Version)\n", VER, REL, BIT);
for(i=1; i < 53; i++)
	fprintf(rptr,"-");
fprintf(rptr,"\n");
fflush(rptr);
}

/*************************************************************************/
void read_m2cents(void)
{
int  lines, endfile;

CentCount = lines=0;

if( (iptr = fopen(CentroidFile,"r")) == NULL ) {
	printf("\nerror: opening file \"%s\" for reading.\n",CentroidFile);
	exit(1);
	}

/* skip "t" cards and blank lines - position pointer to read centroids */
do {
	endfile = fget_line(&linebuf[1],MAXSTR,iptr);
	lines++;
	} while ( (linebuf[2] != '*') && (endfile != EOF) );

if(endfile == EOF) {
	printf("\nerror: file \"%s\" ended suddenly.\n",CentroidFile);
	exit(1);
	}

/* start reading centroids */
while((linebuf[2] == '*') && (endfile != EOF) ) {
	if(linebuf[1] != 'a')
		break;
	CentCount++;
	if(CentCount > NumCentroids) {
		printf("\nerror: more than %d centroids found.\n", NumCentroids);
		exit(1);
		}

	if(sscanf(&linebuf[3]," %ld %f %f",
					&cent[CentCount].num,
					&cent[CentCount].x,
					&cent[CentCount].y) < 3) {
		printf("\nerror: in \"%s\" invalid file format, line %d\n",
					CentroidFile, lines);
		exit(1);
		}

	cent[CentCount].x = cent[CentCount].x / NetworkScale;
	cent[CentCount].y = cent[CentCount].y / NetworkScale;

	endfile = fget_line(&linebuf[1],MAXSTR,iptr);
	lines++;
	}

fclose(iptr);
}

/*************************************************************************/
void read_m2nodes(void)
{
int  lines, endfile;

NodeCount = lines=0;

if( (iptr = fopen(NodeFile,"r")) == NULL ) {
	printf("\nerror: opening file \"%s\" for reading.\n",NodeFile);
	exit(1);
	}

/* skip "t" cards and blank lines - position pointer to read nodes */
do {
	endfile = fget_line(&linebuf[1],MAXSTR,iptr);
   lines++;
   } while ( (linebuf[1] != 'a') && (endfile != EOF) );

if(endfile == EOF) {
	printf("\nerror: file \"%s\" ended suddenly.\n",NodeFile);
	exit(1);
	}

/* start reading nodes - first one has been read */
while( (endfile != EOF) && (linebuf[1] == 'a') ) {
	NodeCount++;
	if(NodeCount > NumNodes) {
		printf("\nerror: more than %d nodes found.\n", NumNodes);
		exit(1);
		}

	if(sscanf(&linebuf[3]," %ld %f %f %f %f",
		&node[NodeCount].num,
		&node[NodeCount].x,
		&node[NodeCount].y,
		&node[NodeCount].mindist,
		&node[NodeCount].maxdist) < 3) {
		printf("\nerror: in \"%s\" invalid file format, line %d\n",
			NodeFile,lines);
		exit(1);
		}

	node[NodeCount].x = node[NodeCount].x / NetworkScale;
	node[NodeCount].y = node[NodeCount].y / NetworkScale;

	nodeIndex[node[NodeCount].num] = NodeCount;

	endfile = fget_line(&linebuf[1],MAXSTR,iptr);
	lines++;
	}

fclose(iptr);
}

/*************************************************************************/
void pick_frto(char *tempbuf, int *from, int *to, int *length)
{
int err = FALSE;

if( (sscanf(tempbuf," %d - %d", from, to)) < 2)
  err = TRUE;
if(*to < *from)
  err = TRUE;
if( (*from <= 0) || (*to <= 0) )
  err = TRUE;

*length = (*to - *from) + 1;

if(err) {
	printf("\nerror: reading line:\n=>%s", tempbuf);
	exit(1);
	}

}

/*************************************************************************/
void read_mpstations(void)
{
char InputString[MAXSTR];
char temp[MAXSTR];
int stationNode, hwyNode1, hwyNode2, dist1, dist2, dp;
int i, c1, c2, k;
float stationX, stationY;
FILE *fp;

int NumStations = 0;
int lineNum = 0;

/* Open the Station file for reading */
if ((fp = fopen(StationFile, "r")) == NULL) {
	printf("\nerror: opening file \"%s\" for reading.\n",StationFile);
	exit(1);
	}

printf("\nReading station lines...");

/* first 2 lines are for header */
fgets(&InputString[0], MAXSTR, fp);
fgets(&InputString[0], MAXSTR, fp);

while( (fgets(&InputString[0], MAXSTR, fp)) != NULL) {
	lineNum++;

	/* read station number */
	c1 = 0;
	c2 = 4;
	k = 0;
	for (i=c1; i <= c2; i++)
		temp[k++] = InputString[i];
	temp[c2-c1+1] = '\0';
	stationNode = atoi(temp);


	if (stationNode <= 0)
		break;


	/* read station x coord number */
	c1 = 16;
	c2 = 21;
	k = 0;
	for (i=c1; i <= c2; i++)
		temp[k++] = InputString[i];
	temp[c2-c1+1] = '\0';
	stationX = (float)atof(temp);


	/* read station y coord number */
	c1 = 22;
	c2 = 27;
	k = 0;
	for (i=c1; i <= c2; i++)
		temp[k++] = InputString[i];
	temp[c2-c1+1] = '\0';
	stationY = (float)atof(temp);


	/* read first highway node */
	c1 = 28;
	c2 = 32;
	k = 0;
	for (i=c1; i <= c2; i++)
		temp[k++] = InputString[i];
	temp[c2-c1+1] = '\0';

	hwyNode1 = -1;
	if (strcmp(temp, "     ") != 0)
		hwyNode1 = atoi(temp);


	/* read first highway node's extra distance */
	c1 = 33;
	c2 = 35;
	k = 0;
	for (i=c1; i <= c2; i++)
		temp[k++] = InputString[i];
	temp[c2-c1+1] = '\0';

	dist1 = 0;
	if (strcmp(temp, "   ") != 0)
		dist1 = atoi(temp);


	/* read second highway node */
	c1 = 36;
	c2 = 40;
	k = 0;
	for (i=c1; i <= c2; i++)
		temp[k++] = InputString[i];
	temp[c2-c1+1] = '\0';

	hwyNode2 = -1;
	if (strcmp(temp, "     ") != 0)
		hwyNode2 = atoi(temp);


	/* read second highway node's extra distance */
	c1 = 41;
	c2 = 43;
	k = 0;
	for (i=c1; i <= c2; i++)
		temp[k++] = InputString[i];
	temp[c2-c1+1] = '\0';

	dist2 = 0;
	if (strcmp(temp, "   ") != 0)
		dist2 = atoi(temp);


	/* read second highway node's extra distance */
	c1 = 63;
	c2 = 65;
	k = 0;
	for (i=c1; i <= c2; i++)
		temp[k++] = InputString[i];
	temp[c2-c1+1] = '\0';

	dp = 0;
	if (strcmp(temp, "  ") != 0)
		dp = atoi(temp);


	if (hwyNode1 > 0 && dp > 0) {
		nodeIndex[stationNode] = stationNode;
		stn[stationNode].station = stationNode;
		stn[stationNode].x = stationX  / NetworkScale;
		stn[stationNode].y = stationY  / NetworkScale;
		stn[stationNode].hwy[0] = hwyNode1;
		stn[stationNode].d[0] = dist1;
		stn[stationNode].hwy[1] = hwyNode2;
		stn[stationNode].d[1] = dist2;
		NumStations++;
		}

	} /* while ! EOF */

printf("%d found that are PNR stations\n",NumStations);

}


/*************************************************************************/
void read_mplinks(void)
{
int linkCount, lines, endfile, index;
int an, bn, dist, limita, limitm, limitp;

linkCount = lines = 0;

if( (iptr = fopen(HwyLinkFile,"r")) == NULL ) {
	printf("\nerror: opening highway link file \"%s\" for reading.\n",HwyLinkFile);
	exit(1);
	}

endfile = fget_line(linebuf,MAXSTR,iptr);
if(endfile == EOF) {
	printf("\nerror: EOF in file \"%s\" encountered when starting to read it.\n", HwyLinkFile);
	exit(1);
	}


printf("\nReading highway links...");

/* start reading links - first one has been read */
while( endfile != EOF ) {

	lines++;

	sscanf(linebuf,"%d %d %*s %d %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %d %d %d %*s %*s %*s %*s %*s %*s",&an, &bn, &dist, &limita, &limitm, &limitp);

	if(an > NumNodes || bn > NumNodes) {
		printf("\nerror: link (%d,%d) has one of its nodes > specified largest node number %d.\n", an, bn, NumNodes);
		exit(1);
		}

	if (limita <= 1) {
		index = nodeIndex[bn];
		node[index].periods[0] = 1;
	}
	if (limitm <= 1) {
		index = nodeIndex[bn];
		node[index].periods[1] = 1;
	}
	if (limitp <= 1) {
		index = nodeIndex[bn];
		node[index].periods[2] = 1;
	}

	endfile = fget_line(linebuf,MAXSTR,iptr);

	}

printf("%d found\n",lines);

fclose(iptr);
}


/*************************************************************************/
void write_link(long cent, long cnode, float dist)
{
char FormatString[MAXSTR];
long ldist, ltime;
float fdist;
int i, index;

if(LinkFormat == 'e') {

/* sample emme/2 link record */
/*a      1   9020   1.17 p             61 0.0   0     150  91.023   97.65 */

	if(DeleteLinks) {

		sprintf(FormatString,"d %%ld,%%ld\n");
		switch(newlink.direction) {
			case 't':
				fprintf(optr,FormatString,cent,cnode);
				fprintf(optr,FormatString,cnode,cent);
				break;
			case 'i':
				fprintf(optr,FormatString,cnode,cent);
				break;
			case 'o':
				fprintf(optr,FormatString,cent,cnode);
				break;
			}
		} /* DeleteLinks */

	sprintf(FormatString,"a %%ld,%%ld,%%.4f,%%s,%%d,%%.1f,%%d,0,0,0\n");

	switch(newlink.direction) {
		case 't':
			fprintf(optr,FormatString,
				cent,cnode,dist,newlink.modesout,newlink.type,
				newlink.lanes,newlink.vdf);
				LinkCount++;
			fprintf(optr,FormatString,
				cnode,cent,dist,newlink.modesin,newlink.type,
				newlink.lanes,newlink.vdf);
			 LinkCount++;
			 break;
		case 'i':
			fprintf(optr,FormatString,
				cnode,cent,dist,newlink.modesin,newlink.type,
				newlink.lanes,newlink.vdf);
			LinkCount++;
			break;
		case 'o':
			fprintf(optr,FormatString,
				cent,cnode,dist,newlink.modesout,newlink.type,
				newlink.lanes,newlink.vdf);
			LinkCount++;
			break;
		}

	} /* if LinkFormat == e */
else
if(LinkFormat == 't') {
	sprintf(FormatString,"1%%5ld%%5ld %%-2s        %%4ld   %%3ld   %%3ld   %%3ld1\n");

	ldist = (long)dist*10;
	ltime = (long)((dist/DriveSpeed)*60*10);

	switch(newlink.direction) {
		case 't':
			fprintf(optr,FormatString,
				cent,cnode,newlink.modesout,ldist,ltime,ltime,ltime);
				LinkCount++;
			fprintf(optr,FormatString,
				cnode,cent,newlink.modesin,ldist,ltime,ltime,ltime);
				LinkCount++;
			 LinkCount++;
			 break;
		case 'i':
			fprintf(optr,FormatString,
				cnode,cent,newlink.modesin,ldist,ltime,ltime,ltime);
				LinkCount++;
			 LinkCount++;
			break;
		case 'o':
			fprintf(optr,FormatString,
				cent,cnode,newlink.modesout,ldist,ltime,ltime,ltime);
				LinkCount++;
			break;
		}

	} /* if LinkFormat == t */
else
if(LinkFormat == 'i') {

	sprintf(FormatString,"1%%5ld%%5ld %%-2s         %%.2f %%4.1f            1\n");

	fdist = dist;

	switch(newlink.direction) {
		case 't':
			fprintf(optr,FormatString,
				cent,cnode,newlink.modesout,fdist,DriveSpeed);
			LinkCount++;
			fprintf(optr,FormatString,
				cnode,cent,newlink.modesin,fdist,DriveSpeed);
			LinkCount++;
			break;
		case 'i':
			fprintf(optr,FormatString,
				cnode,cent,newlink.modesin,fdist,DriveSpeed);
			LinkCount++;
			break;
		case 'o':
			fprintf(optr,FormatString,
				cent,cnode,newlink.modesout,fdist,DriveSpeed);
			LinkCount++;
			break;
		}

	} /* if LinkFormat == i */
else
if(LinkFormat == 'm') {

	for (i=0; i < 3; i++) {
		index = nodeIndex[cnode];
		if (node[index].periods[i] == 1) {
			optr = mptr[i];
			fprintf (optr, "link %04d-%04d,%03d,%.1f\n", cent, cnode, (int)(dist*100.0), DriveSpeed);
			}
		}

	} /* if LinkFormat == m */

}

