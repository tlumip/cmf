/*
 * WLKLINKS.C
 * written by   :  Tim Heier
 * revised by   :  Jim Hicks
 * 2.2x revisions began :  02/14/07
 */

#define VER          "2.24.5"
#define REL      " 3/21/07"
#define BIT            "32"

/* 
 *	Revisions started from version:
 *		#define VER          "2.20"
 *		#define REL      " 7/20/97"
 *		#define BIT            "32"

 *	Revisions:
 *		2.21	14feb2007	
 *		Resolved cast warnings regarding int/float, float/double, etc. type mismatches (roughly 20 resolved).
 *		Changed parameters passed in read_ini() function calls from constant char strings ("string") 
 *		to char[] holding the value of the "string", otherwise got runtime errors.
 *		Added read_mplines() function to read Minutp format transit lines file and added it as a Transit Format option.
 *
 *		2.22	20feb2007
 *		Added a Minutp output format for the access link records.
 *
 *		2.23	09mar2007
 *		Added reading a Minutp format station file for node information about nodes
 *		associated with stations.
 *		If a transit line node was defined in the station file, the highway node
 *		associated with this station was used to determine walk access, and a link
 *		was created if there was access from the zone to the station number + 100.
 *		Fixed a bug related to reading Minutp transit routes that was introduced
 *		in version 2.21.
 *
 *		2.24	14mar2007
 *		If a link from a centroid to a station was created, then also create a link
 *		from the centroid to the highway node associated with the station.
 *		Read highway node D1 and D2 fields from the station file.
 *		Add D1 or D2 distance values to access link distances for centroid to station links.
 *		If output format is minutp, write a access link file for each period.  Write an
 *		access link for each period where the transit route the stop is on has a headway > 0. 
 *
 *		2.24.1	15mar2007
 *		Fixed bug with extra distance not added correctly to rail station access links.
 *		Fixed logic used to determine for which period files to write an access link.
 *
 *		2.24.2	16mar2007
 *		Corrected bugs in original ctlutil.cpp that prevented Exception from being handled correctly
 *		when files were opened and the filename specified could not be found.
 *		Fixed problem where D1 was not being added to station access link correctly.
 *
 *		2.24.3	16mar2007
 *		Fixed apparent problem with unitialized fields in centroid link structures.
 *
 *		2.24.4	20mar2007
 *		Allow the end of node string processing to be flagged by '\r' or ' ', not just '-'.
 *
 *		2.24.5	21mar2007
 *		period flags in candidate node list were not being set properly
 *
 */

char *ProgName = "WLKLINKS";

/*
The basic program logic goes like this:

1. Start from a centroid and find all the nodes within the search values.
2. The candidate transit nodes are then sorted by distance from centroid.
3. The candidate node closest to centroid is accepted into final node set.
4. Subsequent nodes are accepted into the final node set if one or more
	transit lines passing through the node did not pass through the
	previous nodes in the final set.
5. The previous logic can be turned off so nodes are accepted based only
	on distance.
*/

#include "stdutil.h"
#include "ctlutil.h"

#define MAXSTR         120
#define MAXROUTES     1500
#define MAXDIR           2
#define MAXcandidateS    99
#define CARTESIAN        0
#define MANHATTEN        1

typedef struct {
	float  mindist;
	float  maxdist;
	float  avgdist;
	long   connect;
	long   cNodes;
} stat_data;

struct cent_link {
	int	hwyNode;
	int	onRoute;
	int	isCandidate;
	float dist;
	struct cent_link *next;
};

typedef struct {
	long   num;
	float  x;
	float  y;
	long   connected;
	float  mindist;
	float  maxdist;
	float  maxlinks;
	struct cent_link *links;
} cent_data;

typedef struct {
	int	station;
	int	hwy[2];
	int d[2];
} stn_data;

typedef struct {
	long   num;
	float  x;
	float  y;
	long   connected;
	float  candidate;
	float  distance;
	int	   centroidConnector;
	int    station;
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

struct cdata {
	long	node;
	float	dist;
	int		station;
	int		periods[3];
};

struct RouteList {
	int   Dir;
	long	Node;
	float	Dwell;
	struct RouteList *next;
};

struct LineDesc {
	char	Line[7];
	char  Desc[MAXSTR];
	char  cMode;
	int	iMode, VehTyp, RG, Dir[MAXDIR+1];
	float	TF, D, S, T, H1, H2, H3;
	float	td1,td2,td3;
	struct RouteList *Route;
};

/* user defined data */
struct LineDesc *TrLine[MAXROUTES];
stat_data  *stat;
cent_data  *cent;
node_data  *node;
stn_data   *stn;
link_data  newlink;

col_data   fnode, fnodex, fnodey;
col_data   fcent, fcentx, fcenty, fcentu1, fcentu2, fcentu3;

/* global variables */
FILE  *iptr, *rptr, *optr, *aptr, **mptr;
int   CentCount, NodeCount, NumRoutes, LinkCount;
char  *linebuf;
char  *iniFile, *CentroidFile, *NodeFile, *TransitFile;
char  *StationFile, *LinkFile, *HwyLinkFile, *ReportFile, *AccessFile;

int   NumCentroids,MaxLinks,SelectModes;
long  HiNodeNumber;
float NetworkScale,MinDistance,MaxDistance,WalkSpeed;
char  SearchModes[MAXSTR];

int   PrintUnconnected,PrintConnections,PrintStats,DryRun,Debug;
int   Distance,AverageDistance,IndividualSearch,IndividualMax;
int   FirstCentroid,FirstNode,EchoTransitLines,LinesAccessed;

int   DeleteLinks,MultipleStops,SplitLines,PrintLines;
char  NodeFormat,CentroidFormat,TransitFormat,LinkFormat;
char  PrintLine[7];

void  read_ini(void);
void  read_cents(void);
void  read_m2cents(void);
void  read_nodes(void);
void  read_m2nodes(void);
void  read_m2lines(void);
void  read_tplines(void);
void  read_mplines(void);
void  read_mpstations(void);
void  read_mplinks(void);
void  echo_transit(void);
void  wlklinks(void);
void  write_link(long cent, long cnode, float dist);
void  usage(void);
void  run_header(void);
void  rpt_header(FILE *ptr);
void  pick_frto(char *tempbuf, int *from, int *to, int *length);
void  add_to_route (struct RouteList *, struct RouteList *);
void  sorts(float *,float *, int, int);
int   sort_function( const void *a, const void *b);

int main(int argc, char* argv[])
{

if(argc < 2) {
	usage();
	exit(1);
	}

/* allocate memory for strings */
CentroidFile = (char *) malloc(MAXSTR);
NodeFile     = (char *) malloc(MAXSTR);
TransitFile  = (char *) malloc(MAXSTR);
StationFile  = (char *) malloc(MAXSTR);
LinkFile     = (char *) malloc(MAXSTR);
HwyLinkFile  = (char *) malloc(MAXSTR);
ReportFile   = (char *) malloc(MAXSTR);
AccessFile   = (char *) malloc(MAXSTR);
iniFile      = (char *) malloc(MAXSTR);
linebuf      = (char *) malloc(MAXSTR);

/* allocate memory for character arrays */
if( (CentroidFile == NULL) || (NodeFile == NULL) || (LinkFile == NULL) || (HwyLinkFile == NULL) ||
	 (ReportFile == NULL)  || (iniFile == NULL)  || (linebuf == NULL)  ||
	 (TransitFile == NULL) || (StationFile == NULL)  || (AccessFile == NULL) ) {
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
if( (stat = (stat_data *) calloc( NumCentroids+1, sizeof(stat_data))) == NULL ) {
	printf("\nerror: allocating stats buffer\n");
	exit(1);
	}
if( (node = (node_data *) calloc( HiNodeNumber+1, sizeof(node_data))) == NULL ) {
	printf("\nerror: allocating node buffer\n");
	exit(1);
	}
if( (stn = (stn_data *) calloc( HiNodeNumber+1, sizeof(stn_data))) == NULL ) {
	printf("\nerror: allocating station buffer\n");
	exit(1);
	}

switch(CentroidFormat) {
	case 'e': read_m2cents(); break;
	default : read_cents();   break;
	}

switch(NodeFormat) {
	case 'e': read_m2nodes(); break;
	default : read_nodes();   break;
	}

switch(TransitFormat) {
	case 'e': read_m2lines(); break;
	case 't': read_tplines(); break;
	case 'm':
		read_mplinks();
		read_mplines();
		/* if there's a station file specified, read stations.  The file, and station processing is optional */
		if (strcmp(StationFile, "__Optional__") != 0)
			read_mpstations();
		break;
	}

if(EchoTransitLines)
	echo_transit();

fprintf(rptr,"\nRUN RESULTS\n");
fprintf(rptr,  "------------\n\n");
fprintf(rptr,"CENTROIDS READ = %d\n",CentCount);
fprintf(rptr,"NODES READ     = %d\n",NodeCount);

wlklinks();

return(0);
}

/*************************************************************************/
void read_ini(void)
{
char i,j,k,c, tempbuf[MAXSTR];
char entryBuf[MAXSTR];
char defaultBuf[MAXSTR];

/* FILES Section */
strcpy(linebuf,"Files");

strcpy(entryBuf,"CentroidFile");
strcpy(defaultBuf,"cents.in");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, CentroidFile, iniFile);

strcpy(entryBuf,"NodeFile");
strcpy(defaultBuf,"nodes.in");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, NodeFile, iniFile);

strcpy(entryBuf,"TransitFile");
strcpy(defaultBuf,"lines.in");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, TransitFile, iniFile);

strcpy(entryBuf,"StationFile");
strcpy(defaultBuf,"__Optional__");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, StationFile, iniFile);

strcpy(entryBuf,"LinkFile");
strcpy(defaultBuf,"wlklinks.out");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, LinkFile, iniFile);

strcpy(entryBuf,"HwyLinkFile");
strcpy(defaultBuf,"hwylinks.in");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, HwyLinkFile, iniFile);

strcpy(entryBuf,"ReportFile");
strcpy(defaultBuf,"wlklinks.rpt");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, ReportFile, iniFile);

strcpy(entryBuf,"AccessFile");
strcpy(defaultBuf,"access.rpt");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, AccessFile, iniFile);


/* open report file */
if( (rptr = fopen(ReportFile,"w")) == NULL ) {
	printf("\nerror: opening file \"%s\" for writing.", ReportFile);
	exit(1);
	}
rpt_header(rptr);

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

strcpy(entryBuf,"HiNodeNumber");
strcpy(defaultBuf,"10000");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
HiNodeNumber = atol(tempbuf);

strcpy(entryBuf,"MaxLinks");
MaxLinks = GetPrivateProfileInt(linebuf, entryBuf, 4, iniFile);

strcpy(entryBuf,"MinDistance");
MinDistance = GetPrivateProfileFloat(linebuf, entryBuf, 0.0, iniFile);

strcpy(entryBuf,"MaxDistance");
MaxDistance = GetPrivateProfileFloat(linebuf, entryBuf, 0.333f, iniFile);

strcpy(entryBuf,"NetworkScale");
NetworkScale = GetPrivateProfileFloat(linebuf, entryBuf, 1.0, iniFile);

strcpy(entryBuf,"SelectModes");
SelectModes = GetPrivateProfileBool(linebuf, entryBuf, 0, iniFile);

strcpy(entryBuf,"SearchModes");
strcpy(defaultBuf,"b");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, SearchModes, iniFile);

strcpy(entryBuf,"WalkSpeed");
WalkSpeed = GetPrivateProfileFloat(linebuf, entryBuf, 3.0, iniFile);

strcpy(entryBuf,"Distance");
strcpy(defaultBuf,"cartesian");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
if(toupper(tempbuf[0]) == 'C')
	Distance = CARTESIAN;
else
	Distance = MANHATTEN;

strcpy(entryBuf,"AverageDistance");
AverageDistance = GetPrivateProfileBool(linebuf, entryBuf, 1, iniFile);

strcpy(entryBuf,"IndividualSearch");
IndividualSearch = GetPrivateProfileBool(linebuf, entryBuf, 0, iniFile);

strcpy(entryBuf,"IndividualMax");
IndividualMax = GetPrivateProfileBool(linebuf, entryBuf, 0, iniFile);

strcpy(entryBuf,"MultipleStops");
MultipleStops = GetPrivateProfileBool(linebuf, entryBuf, 0, iniFile);

strcpy(entryBuf,"SplitLines");
SplitLines = GetPrivateProfileBool(linebuf, entryBuf, 0, iniFile);


if(NumCentroids == 0) {
	printf("\nerror: number of centroids cannot be zero.\n");
	exit(1);
	}
if(HiNodeNumber <=  0) {
	printf("\nerror: number of nodes cannot be zero.\n");
	exit(1);
	}
if(NetworkScale <= 0) {
	printf("\nerror: network scale must be a positive value.\n");
	exit(1);
	}
if(MaxDistance < MinDistance) {
	printf("\nerror: maximum distance cannot be less than minimum.\n");
	exit(1);
	}
if((MaxLinks <= 0) || (MaxLinks > MAXcandidateS)) {
	printf("\nerror: maximum links is out of range.\n");
	exit(1);
	}
if(WalkSpeed <= 0) {
	printf("\nerror: walk speed must be a positive value.\n");
	exit(1);
	}


/* REPORTS Section */
strcpy(linebuf,"Reports");

strcpy(entryBuf,"EchoTransitLines");
EchoTransitLines = GetPrivateProfileBool(linebuf, entryBuf, 1, iniFile);

strcpy(entryBuf,"PrintUnconnected");
PrintUnconnected = GetPrivateProfileBool(linebuf, entryBuf, 1, iniFile);

strcpy(entryBuf,"PrintConnections");
PrintConnections = GetPrivateProfileBool(linebuf, entryBuf, 1, iniFile);

strcpy(entryBuf,"PrintStats");
PrintStats = GetPrivateProfileBool(linebuf, entryBuf, 1, iniFile);


/* these are undocumented options */
strcpy(entryBuf,"LinesAccessed");
LinesAccessed = GetPrivateProfileBool(linebuf, entryBuf, 0, iniFile);

strcpy(entryBuf,"Debug");
Debug = GetPrivateProfileBool(linebuf, entryBuf, 0, iniFile);

strcpy(entryBuf,"PrintLine");
strcpy(defaultBuf,"\0");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, PrintLine, iniFile);

strcpy(entryBuf,"PrintLines");
PrintLines = GetPrivateProfileBool(linebuf, entryBuf, 0, iniFile);


/* pad the line id with blanks */
if(strlen(PrintLine) > 0) {
	i=strlen(PrintLine);
	j=6;
	for(k=i; k < j; k++)
		PrintLine[k] = ' ';
	PrintLine[j] = '\0';
	}


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

   if (IndividualSearch) {
		strcpy(entryBuf,"User1");
		strcpy(defaultBuf,"\0");
		GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
		pick_frto(tempbuf, &fcentu1.from, &fcentu1.to, &fcentu1.length);

		strcpy(entryBuf,"User2");
		strcpy(defaultBuf,"\0");
		GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
		pick_frto(tempbuf, &fcentu2.from, &fcentu2.to, &fcentu2.length);
   	}

   if (IndividualMax) {
		strcpy(entryBuf,"User3");
		strcpy(defaultBuf,"\0");
		GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
		pick_frto(tempbuf, &fcentu3.from, &fcentu3.to, &fcentu3.length);
   	}

	strcpy(entryBuf,"FirstCentroid");
	FirstCentroid = GetPrivateProfileInt(linebuf, entryBuf, 1, iniFile);
	}


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

	strcpy(entryBuf,"FirstNode");
	FirstNode = GetPrivateProfileInt(linebuf, entryBuf, 1, iniFile);
	}


/* TRANSIT FORMAT Section */
strcpy(linebuf,"Transit Format");

strcpy(entryBuf,"TransitFormat");
strcpy(defaultBuf,"e");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
TransitFormat = tolower(tempbuf[0]);
if( (TransitFormat != 'e') && (TransitFormat != 't') && (TransitFormat != 'm') ) {
	printf("\nerror: invalid *TransitFormat* entry = %c\n",TransitFormat);
	exit(1);
	}


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

strcpy(entryBuf,"ModesIn");
strcpy(defaultBuf,"i");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, newlink.modesin, iniFile);

strcpy(entryBuf,"ModesOut");
strcpy(defaultBuf,"o");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, newlink.modesout, iniFile);

strcpy(entryBuf,"LinkFormat");
strcpy(defaultBuf,"e");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
LinkFormat = tolower(tempbuf[0]);
if( (LinkFormat != 'e') && (LinkFormat != 't') && (LinkFormat != 'i') && (LinkFormat != 'm') ) {
	printf("\nerror: invalid *LinkFormat* entry = %s\n",tempbuf);
	exit(1);
	}

strcpy(entryBuf,"Direction");
strcpy(defaultBuf,"twoway");
GetPrivateProfileString(linebuf, entryBuf, defaultBuf, tempbuf, iniFile);
newlink.direction = tolower(tempbuf[0]);
if( (newlink.direction != 't') && (newlink.direction != 'i') &&
		(newlink.direction != 'o') ) {
	printf("\nerror: invalid *Direction* entry = %s\n",tempbuf);
	exit(1);
	}

if(TransitFormat == 't') {
	SplitLines = FALSE;
	}
if( (LinkFormat == 't') || (LinkFormat == 'i') || (LinkFormat == 'm') ) {
	DeleteLinks = FALSE;
	}
}

/*************************************************************************/
void echo_transit(void)
{
int i, j, Printed;
struct RouteList *Chain;

fprintf(rptr,"\nTRANSIT LINES READ = %4d\n", NumRoutes);
fprintf(rptr,  "--------------------------\n\n");
Printed = 0;

for(j=1; j <= NumRoutes; j++) {
	if(TrLine[j] != NULL) {
		fprintf(rptr,"   %6s",TrLine[j]->Line);
		Printed++;
		if( Printed % 8 == 0)
			fprintf(rptr,"\n");
		}
	}

fprintf(rptr,"\n");

if(strlen(PrintLine) > 0) {
	fprintf(rptr,"\nPRINT LINE = %6s\n", PrintLine);
	fprintf(rptr,  "--------------------\n\n");
	for (i=1; i < MAXROUTES; i++) {
		if (TrLine[i] != NULL) {
			if (strncmp (TrLine[i]->Line, PrintLine,strlen(TrLine[i]->Line)) == 0) {
				 fprintf(rptr,  "Description = %s\n",TrLine[i]->Desc);
				 if(TransitFormat == 'e')
					fprintf(rptr,  "Mode = %c\n",TrLine[i]->cMode);
				 else
				 if(TransitFormat == 't')
					fprintf(rptr,  "Mode = %d\n",TrLine[i]->iMode);

				 Chain = TrLine[i]->Route;
				 j=0;
				 while (Chain != NULL) {
					if (j++ % 8 == 0)
						fprintf(rptr,"\n");
					fprintf(rptr,"%8ld", Chain->Node);
					Chain = Chain->next;
					}
				fprintf(rptr,"\n");
				break;
				}
			}
		}
	}

if(PrintLines) {
	for (i=1; i < MAXROUTES; i++) {
		if (TrLine[i] != NULL) {
			fprintf(rptr,  "\nDescription = %s\n",TrLine[i]->Desc);
			if(TransitFormat == 'e')
				fprintf(rptr,  "Mode = %c\n",TrLine[i]->cMode);
			else
				if(TransitFormat == 't')
					fprintf(rptr,  "Mode = %d\n",TrLine[i]->iMode);

			Chain = TrLine[i]->Route;
			j=0;
			while (Chain != NULL) {
				if (j++ % 8 == 0)
					fprintf(rptr,"\n");
				fprintf(rptr,"%8ld", Chain->Node);
				Chain = Chain->next;
				}

			fprintf(rptr,"\n");
			}
		}
	}
}

/*************************************************************************/
void wlklinks(void)
{
int    i,j,k,m;
int    cNodes, fNodes, CurDir;
int    Printed, UseNode, UseLine, maxlinks, index;
char   temp[MAXSTR];
char   mstring[MAXSTR];
char   *periodLabels[] = { "am", "md", "pm" };
long   l, tnode, hnode;
double X, Y, Arg, sumdist, nitems;
double mindist, maxdist, dist;
struct RouteList *Chain;
struct cdata *candidate;
struct cent_link *link;

/* allocate memory for candidate array */
if( (candidate = (cdata *) calloc( MAXcandidateS+1, sizeof(struct cdata))) == NULL ) {
	printf("\nerror: allocating candidate node buffer\n");
	exit(1);
	}

/* create output file */
if(LinkFormat == 'm') {
	
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




if(LinkFormat == 'e') {
	fprintf(optr,"c========================================\n");
	fprintf(optr,"c %s (TM)  Ver %s  Rel  %s\n", ProgName, VER, REL);
	fprintf(optr,"c========================================\n");
	fprintf(optr,"t links\n");
	}

if(LinesAccessed) {
	if( (aptr = fopen(AccessFile,"w")) == NULL ) {
		printf("\nerror: opening file \"%s\" for writing.\n",AccessFile);
		exit(1);
		}
	rpt_header(aptr);
	fprintf(aptr,"\n");
	}

/* get global search values */
mindist  = MinDistance;
maxdist  = MaxDistance;
maxlinks = MaxLinks;

/* loop through all the centroids and connect them to nodes */
LinkCount = 0;

printf("\nProcessing Zone:\n");

for (i=1; i <= CentCount; i++) {
	printf("%ld\r",cent[i].num);

	if ( i==589 ){
		i=589;
	}

	if(Debug) {
		fprintf(rptr,"\nCentroid=%d\n",i);
		if(IndividualSearch) {
			fprintf(rptr,"Min Dist=%.3f\n",cent[i].mindist);
			fprintf(rptr,"Max Dist=%.3f\n",cent[i].maxdist);
			}
		if(IndividualMax)
			fprintf(rptr,"Max Links=%.3f\n",cent[i].maxlinks);
		}

	/* clear candidate arrays */
	for (l=1; l <= HiNodeNumber; l++) {
		node[l].candidate = 0;
		node[l].distance = 0;
		node[l].station = 0;
		node[l].periods[0] = 0;
		node[l].periods[1] = 0;
		node[l].periods[2] = 0;
		}
	for(k=0; k <= MAXcandidateS; k++) {
		candidate[k].node = 0;
		candidate[k].dist = 0;
		candidate[k].station = 0;
		candidate[k].periods[0] = 0;
		candidate[k].periods[1] = 0;
		candidate[k].periods[2] = 0;
		}

	/* set search values based on current centroid */
	if(IndividualSearch) {
		mindist = cent[i].mindist;
		maxdist = cent[i].maxdist;
		}

	if(IndividualMax)
		maxlinks = (int)cent[i].maxlinks;

	/* loop through nodes in transit lines to find those
		within the search distance */
	for(j=1; j <= NumRoutes; j++) {

		if(TrLine[j] != NULL) {

			UseLine = TRUE;
			if(SelectModes) {
				switch (TransitFormat) {
					case 'e':
						mstring[0] = TrLine[j]->cMode;
						mstring[1] = '\0';
						break;
					case 't':
					case 'm':
						sprintf(mstring,"%d",TrLine[j]->iMode);
						break;
					}
				if(str_index(SearchModes,mstring) < 0)
					UseLine = FALSE;
				}

			 if(UseLine) {
				Chain = TrLine[j]->Route;
				while(Chain != NULL) {
					if(Chain->Node < 0) {
						tnode = labs(Chain->Node);

						if(tnode > HiNodeNumber) {
							fprintf(rptr,"error: transit stop %ld is greater than highest node number\n",
								tnode);
							exit(1);
							}

						/* if the node is a regular highway node */
						if (stn[tnode].station == 0) { /* regular highway node, not a station */

							/* set the onRoute flag if tnode is one end of a highway network centroid connector */
							link = cent[i].links;
							while (link != NULL) {
								if (link->hwyNode == node[tnode].num)
									link->onRoute = 1;
								link = link->next;
								}


							if ( tnode==3855 ){
								tnode=3855;
							}

							/* calculate distance */
							switch(Distance) {
								case CARTESIAN :
									X=(double)((node[tnode].x-cent[i].x)*(node[tnode].x-cent[i].x));
									Y=(double)((node[tnode].y-cent[i].y)*(node[tnode].y-cent[i].y));
									Arg = X + Y;
									if(Arg <= 0) {
										fprintf(rptr,"warning: invalid distance for centroid = %ld, node = %ld\n",
											cent[i].num, node[tnode].num);
										}
									dist = sqrt(Arg);
									break;
								case MANHATTEN :
									X=(double)fabs((node[tnode].x-cent[i].x));
									Y=(double)fabs((node[tnode].y-cent[i].y));
									dist = X + Y;
									break;
								}



							/* mark node if in search distance */
							if( (dist >= mindist) && (dist <= maxdist) ) {
								node[tnode].candidate++;
								node[tnode].distance = (float)dist;
								if (TrLine[j]->H1 > 0)
									node[tnode].periods[0]++;
								if (TrLine[j]->H2 > 0)
									node[tnode].periods[1]++;
								if (TrLine[j]->H3 > 0)
									node[tnode].periods[2]++;
								}

							}
							else {

								for (k=0; k < (stn[tnode].hwy[1] > 0 ? 2 : 1); k++) { /* there is either 1 or 2 highway nodes associated with station */
						
									hnode = stn[tnode].hwy[k];

									/* calculate distance */
									switch(Distance) {
										case CARTESIAN :
											X=(double)((node[hnode].x-cent[i].x)*(node[hnode].x-cent[i].x));
											Y=(double)((node[hnode].y-cent[i].y)*(node[hnode].y-cent[i].y));
											Arg = X + Y;
											if(Arg <= 0) {
												fprintf(rptr,"warning: invalid distance for centroid = %ld, station node = %ld, highway node = %ld\n",
													cent[i].num, tnode, hnode);
												}
											dist = sqrt(Arg);
											break;
										case MANHATTEN :
											X=(double)fabs((node[hnode].x-cent[i].x));
											Y=(double)fabs((node[hnode].y-cent[i].y));
											dist = X + Y;
											break;
										}

									/* mark node if in search distance */
									if( (dist >= mindist) && (dist <= maxdist) ) {
										node[hnode].candidate++;
										node[hnode].distance = (float)dist;
										node[hnode].station = tnode;
										if (TrLine[j]->H1 > 0)
											node[hnode].periods[0]++;
										if (TrLine[j]->H2 > 0)
											node[hnode].periods[1]++;
										if (TrLine[j]->H3 > 0)
											node[hnode].periods[2]++;
										}

									} /* node is regular highway node */

							} /* for highway nodes associated with station */

						} /* Chain->Node < 0 */

					Chain = Chain->next;

					} /* chain != null */
				} /* if UseLine */
			} /* Trline != null */
		} /* for j */

	/* build candidate array based on markers */
	cNodes = 0;
	for (l=1; l <= HiNodeNumber; l++) {
		if(node[l].candidate > 0) {
			cNodes++;
			if(cNodes > MAXcandidateS) {
				cNodes = MAXcandidateS;
				break;;
				}
			candidate[cNodes].node = (long)node[l].num;
			candidate[cNodes].dist = (float)node[l].distance;
			if (node[l].station > 0)
				candidate[cNodes].station = node[l].station;
			candidate[cNodes].periods[0] = node[l].periods[0];
			candidate[cNodes].periods[1] = node[l].periods[1];
			candidate[cNodes].periods[2] = node[l].periods[2];
			} /* if > 0 */
		} /* for l */

	if(Debug) {
		if(cNodes > 0) {
			fprintf(rptr,"Nodes=%d\nbefore sort...\n",cNodes);
			for (j=1; j <= cNodes; j++)
				fprintf(rptr,"%2d.  num=%6ld  dist=%7.3f\n",
					j,candidate[j].node,candidate[j].dist);
			}
		}

	/* sort candidate nodes by distance */
	qsort((void *)&candidate[1], cNodes, sizeof(struct cdata), sort_function);

	if(Debug) {
		if(cNodes > 0) {
			fprintf(rptr,"after sort...\n");
			for(j=1; j <= cNodes; j++)
				fprintf(rptr,"%2d.  num=%6ld  dist=%7.3f\n",
					j,candidate[j].node,candidate[j].dist);
			}
		}

	/* clear "used" flag before checking candidate nodes */
	for(j=1; j <= NumRoutes; j++)
		for(k=0; k <= MAXDIR; k++)
			TrLine[j]->Dir[k] = 0;

	if(Debug) {
		if(cNodes > 0)
			fprintf(rptr,"during line check...\n");
		}

	/* loop through candidates nodes, figure out which ones makes it */
	fNodes = 0;
	for(k=1; k <= cNodes; k++) {

		UseNode = 0;
		if(fNodes < maxlinks) {

			/* loop through transit nodes and mark connected lines */
			if(! MultipleStops) {
				for(j=1; j <= NumRoutes; j++) {
					if(TrLine[j] != NULL) {
						Chain = TrLine[j]->Route;
						while(Chain != NULL) {
							tnode = labs(Chain->Node);
							if (stn[tnode].station > 0)
								for (m=0; m < (stn[tnode].hwy[1] > 0 ? 2 : 1); m++) {
									hnode = stn[tnode].hwy[m];
									if(hnode == candidate[k].node) {
										if(SplitLines) {
											CurDir = Chain->Dir;
											if(TrLine[j]->Dir[CurDir] == 0) {
												TrLine[j]->Dir[CurDir] = 1;
												UseNode++;
												}
											}
										else {
											if(TrLine[j]->Dir[0] == 0) {
												TrLine[j]->Dir[0] = 1;
												UseNode++;
												}
											}
										}
								}
								else {
									if(tnode == candidate[k].node) {
										if(SplitLines) {
											CurDir = Chain->Dir;
											if(TrLine[j]->Dir[CurDir] == 0) {
												TrLine[j]->Dir[CurDir] = 1;
												UseNode++;
												}
											}
										else {
											if(TrLine[j]->Dir[0] == 0) {
												TrLine[j]->Dir[0] = 1;
												UseNode++;
												}
											}
										}
								}

							Chain = Chain->next;

							} /* chain != null */
						} /* Trline != null */
					} /* for j */
				} /* if ! MultipleStops */
			else {
				UseNode++;
				}

			if(Debug)
				fprintf(rptr,"node=%6ld  new lines=%3d\n",candidate[k].node,UseNode);

			} /* if fNodes <= maxlinks */

		if(UseNode > 0)
			fNodes++;
		else
			candidate[k].node = 0;

		//NEXTNODE: ;
		} /* for cNodes */

	if(Debug) {
		if(fNodes > 0) {
			fprintf(rptr,"after line check...\n");
			for(j=1; j <= cNodes; j++)
				if(candidate[j].node > 0)
					fprintf(rptr,"%2d.  num=%6ld  dist=%7.3f\n",
						j,candidate[j].node,candidate[j].dist);
			}
		}

	/* calculate statistics for each centroid */
	sumdist = nitems = 0;
	if(fNodes > 0) {
		stat[i].mindist = 99999999.0f;
		stat[i].maxdist = 0;
		stat[i].cNodes = cNodes;
		for(j=1; j <= cNodes; j++) {
			if(candidate[j].node > 0) {
				sumdist += candidate[j].dist;
				nitems++;
				if(candidate[j].dist < stat[i].mindist)
					stat[i].mindist = candidate[j].dist;
				if(candidate[j].dist > stat[i].maxdist)
					stat[i].maxdist = candidate[j].dist;
				}
			}
		if(nitems == 0) {
			stat[i].mindist = 0;
			stat[i].maxdist = 0;
			stat[i].avgdist = 0;
			stat[i].connect = 0;
			}
		else {
			stat[i].avgdist = (float)(sumdist / nitems);
			stat[i].connect = (long)nitems;
			}

		} /* if fNodes */

	/* calculate average distance for all links */
	sumdist = nitems = 0;
	if( (AverageDistance) && (fNodes > 0) ) {
		for(j=1; j <= cNodes; j++) {
			if(candidate[j].node > 0) {
				sumdist += candidate[j].dist;
				nitems++;
				}
			}
		if(nitems == 0) {
			fprintf(rptr,"warning: average connector distance is zero for centroid = %ld\n",
				cent[i].num);
			dist = MaxDistance;
			}
		else
			dist = sumdist / nitems;

		/* assign average distance to each node */
		for(j=1; j <= cNodes; j++) {
			if(candidate[j].node > 0)
				candidate[j].dist = (float)dist;
			}

		} /* if AverageDistance */


	
	/* set the isCandidate flag if candidate node is one end of a highway network centroid connector */
	for(j=1; j <= cNodes; j++) {
		if(candidate[j].node > 0) {
			link = cent[i].links;
			while (link != NULL) {
				if (link->hwyNode == candidate[j].node) {
					link->isCandidate = 1;
					if (candidate[j].dist > 0.25)
						candidate[j].dist = 0.25;
					}
				link = link->next;
				}
			}
		}



	/* write out links to final nodes */
	if(fNodes > 0)
		for(j=1; j <= cNodes; j++)
			if(candidate[j].node > 0) {
				/* if it's minutp format, write a link file for each period */
				if ( mptr != NULL) {
					if ( candidate[j].periods[0] > 0 ) {	/* am period */
						optr = mptr[0];
						/* write a link for the candidate hwy node */
						write_link(cent[i].num, candidate[j].node, candidate[j].dist);
						/* if the candidate node is a station, write an additional link for the station */
						if (candidate[j].station > 0)
							write_link(cent[i].num, candidate[j].station + 100, (float)(candidate[j].dist + stn[candidate[j].station].d[0]/100.0) );
						}
					if ( candidate[j].periods[1] > 0 ) {	/* md period */
						optr = mptr[1];
						/* write a link for the candidate hwy node */
						write_link(cent[i].num, candidate[j].node, candidate[j].dist);
						/* if the candidate node is a station, write an additional link for the station */
						if (candidate[j].station > 0)
							write_link(cent[i].num, candidate[j].station + 100, (float)(candidate[j].dist + stn[candidate[j].station].d[0]/100.0) );
						}
					if ( candidate[j].periods[2] > 0 ) {	/* pm period */
						optr = mptr[2];
						/* write a link for the candidate hwy node */
						write_link(cent[i].num, candidate[j].node, candidate[j].dist);
						/* if the candidate node is a station, write an additional link for the station */
						if (candidate[j].station > 0)
							write_link(cent[i].num, candidate[j].station + 100, (float)(candidate[j].dist + stn[candidate[j].station].d[0]/100.0) );
						}
					}
					else {
						write_link(cent[i].num, candidate[j].node, candidate[j].dist);
						}
				cent[i].connected += 1;
				node[candidate[j].node].connected += 1;
				}

		/* if it's minutp format, write a link file for each period; if not Minutp, do nothing */
		if ( mptr != NULL) {
			/* for any centroid connector that is not already a candidate but is on a route, write access links */
			link = cent[i].links;
			while (link != NULL) {
				if (link->isCandidate == 0 && link->onRoute) {
					for (j=0; j < 3; j++) {
						/* write an access link for the centroid connector to transit stop link for each time period */
						optr = mptr[j];
						write_link(cent[i].num, link->hwyNode, (float)(link->dist > 0.25 ? 0.25 : link->dist) );
						}
					cent[i].connected += 1;
					node[link->hwyNode].connected += 1;
					}
					link = link->next;
				}
			}


	if(LinesAccessed) {
		fprintf(aptr,"Centroid = %ld\n",cent[i].num);
		for(j=1; j < NumRoutes; j++) {
			if(TrLine[j] != NULL) {
				m = 0;
				for(k=0; k <= MAXDIR; k++)
					m += TrLine[j]->Dir[k];
				if(m > 0)
					fprintf(aptr," %6s\n",TrLine[j]->Line);
				}
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
		if(! cent[i].connected) {
			fprintf(rptr," %6ld",cent[i].num);
			Printed++;
			if( Printed % 10 == 0)
				fprintf(rptr,"\n");
			}
		}
	fprintf(rptr,"\n");

	} /* if  PrintUnconnected */

if(PrintConnections) {
	fprintf(rptr,"\nNUMBER OF TWO-WAY LINKS/CENTROID\n");
	fprintf(rptr,  "---------------------------------\n\n");
	Printed = 0;
	for (i=1; i <= CentCount; i++) {
		if(cent[i].connected > 0) {
			fprintf(rptr," %6ld = %6ld |",cent[i].num,cent[i].connected);
			Printed++;
			if( Printed % 4 == 0)
				fprintf(rptr,"\n");
			}
		}
	fprintf(rptr,"\n");

/* this code works but it could be a long output
	fprintf(rptr,"\nNUMBER OF TWO-WAY LINKS/NODE\n");
	fprintf(rptr,  "-----------------------------\n\n");
	Printed = 0;
	for (i=1; i <= HiNodeNumber; i++) {
		if(node[i].connected > 0) {
			fprintf(rptr," %6ld = %6ld |",node[i].num,node[i].connected);
			Printed++;
			if( Printed % 4 == 0)
				fprintf(rptr,"\n");
			}
		}
	fprintf(rptr,"\n");
*/
	} /* if PrintConnections */

if(PrintStats) {
	fprintf(rptr,"\nCENTROID CONNECTION STATISTICS\n");
	fprintf(rptr,  "-------------------------------\n\n");
	fprintf(rptr,"            candidate     Final       min       max       avg\n");
	fprintf(rptr,"Centroid       links     links      dist      dist      dist\n");
	fprintf(rptr,"--------------------------------------------------------------\n");
	for (i=1; i <= CentCount; i++) {
		fprintf(rptr,"%8ld  = %8ld %9ld  %8.3f  %8.3f  %8.3f\n",cent[i].num,
			stat[i].cNodes, stat[i].connect,stat[i].mindist, stat[i].maxdist,
			stat[i].avgdist);
		}

	} /* if  PrintStats */

for (i=0; i < 3; i++)
	fclose(mptr[i]);
if(LinesAccessed)
	fclose(aptr);
}

/*************************************************************************/
int sort_function( const void *a, const void *b)
{
struct cdata *i, *j;

i = (struct cdata *)a;
j = (struct cdata *)b;

if(i->dist < j->dist)
	return -1;
else
if(i->dist == j->dist)
	return 0;
if(i->dist > j->dist)
	return 1;

return 0;
}

/*************************************************************************/
void add_to_route (struct RouteList *a, struct RouteList *b)
{
	if (a->next == NULL)
		a->next = b;
	else
		add_to_route (a->next, b);
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

		cent[CentCount].mindist
				= getfloat(&linebuf[1],fcentu1.from,fcentu1.length);
		cent[CentCount].maxdist
				= getfloat(&linebuf[1],fcentu2.from,fcentu2.length);
		cent[CentCount].maxlinks
				= getfloat(&linebuf[1],fcentu3.from,fcentu3.length);

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
long n;

NodeCount = lines = 0;

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
		n = getlong(&linebuf[1],fnode.from,fnode.length);

		if(n > HiNodeNumber) {
			printf("\nerror: node number higher than %d found.\n", HiNodeNumber);
			exit(1);
			}

		node[n].num = n;
		node[n].x   = getfloat(&linebuf[1],fnodex.from,fnodex.length);
		node[n].y   = getfloat(&linebuf[1],fnodey.from,fnodey.length);

		node[n].x = node[n].x / NetworkScale;
		node[n].y = node[n].y / NetworkScale;

		if(node[n].num == 0) {
			printf("\nerror: reading node data at line %d.\n", lines);
			exit(1);
			}
		}
	}

fclose(iptr);
}

/*************************************************************************/
void read_m2lines (void)
{
char InputString[MAXSTR];
char Value[15], special;
long RealNode;
float dwell;
int FoundNodes, Stop, CurDir;
int i, c1, c2, slen;
struct RouteList *Chain;
struct LineDesc  *TransitLine;
FILE *fp;

NumRoutes = 0;

/* Open the Transit Line file for reading */
if ((fp = fopen(TransitFile, "r")) == NULL) {
	printf("\nerror: opening file \"%s\" for reading.\n",TransitFile);
	exit(1);
	}

printf("\nReading transit lines...");

while( (fgets(&InputString[0], MAXSTR, fp)) != NULL) {
	c1=c2=0;
	slen = strlen(InputString);

	if( (InputString[0] == 'c') || (InputString[0] == 't') )
		goto NEXTLINE;

	if(InputString[0] == 'a') {
		NumRoutes++;
		FoundNodes = FALSE;
		TransitLine = ((struct LineDesc *) malloc (sizeof(struct LineDesc)));

		for(i=0; i <= MAXDIR; i++)
			TransitLine->Dir[i] = 0;

		if(TransitLine == NULL) {
			printf ("\nerror: out of memory, line# %d\n", NumRoutes);
			return;
			}
		if(NumRoutes > MAXROUTES) {
			printf ("\nerror: too many transit lines found.\n");
			return;
			}

		/* transit line id */
		c1 = 1;  c2 = c1;
		while( (InputString[c2] != '\'') && (c2 < slen) )
			c2++;
		c2++;  c1 = c2;
		while( (InputString[c2] != '\'') && (c2 < slen) )
			c2++;

		strncpy(TransitLine->Line,&InputString[c1],(c2-c1));
		TransitLine->Line[c2-c1] = '\0';

		/* transit line mode */
		c2++;
		while( (InputString[c2] == ' ') && (c2 < slen) )
			c2++;

		TransitLine->cMode = InputString[c2];

		/* vehicle type */
		c2++;
		while( (InputString[c2] == ' ') && (c2 < slen) )
			c2++;

		Value[0] = InputString[c2];
		Value[1] = '\0';
		TransitLine->VehTyp = atoi(Value);

		/* headway */
		c2++;
		while( (InputString[c2] == ' ') && (c2 < slen) )
			c2++;
		c1 = c2;
		while( (InputString[c2] != ' ') && (c2 < slen) )
			c2++;

		strncpy(Value,&InputString[c1],(c2-c1));
		Value[c2-c1] = '\0';
		TransitLine->H1 = (float)atof(Value);

		/* default speed */
		c2++;
		while( (InputString[c2] == ' ') && (c2 < slen) )
			c2++;
		c1 = c2;
		while( (InputString[c2] != ' ') && (c2 < slen) )
			c2++;

		strncpy(Value,&InputString[c1],(c2-c1));
		Value[c2-c1] = '\0';
		TransitLine->S = (float)atof(Value);

		/* line description */
		c2++;
		while( (InputString[c2] != '\'') && (c2 < slen) )
			c2++;
		c2++; c1 = c2;
		while( (InputString[c2] != '\'') && (c2 < slen) )
			c2++;

		strncpy(TransitLine->Desc,&InputString[c1],(c2-c1));
		TransitLine->Desc[c2-c1] = '\0';
		/* ut1 field */
		c2++;
		while( (InputString[c2] == ' ') && (c2 < slen) )
			c2++;
		c1 = c2;
		while( (InputString[c2] != ' ') && (c2 < slen) )
			c2++;

		strncpy(Value,&InputString[c1],(c2-c1));
		Value[c2-c1] = '\0';
		TransitLine->td1 = (float)atof(Value);

		/* ut2 field */
		c2++;
		while( (InputString[c2] == ' ') && (c2 < slen) )
			c2++;
		c1 = c2;
		while( (InputString[c2] != ' ') && (c2 < slen) )
			c2++;

		strncpy(Value,&InputString[c1],(c2-c1));
		Value[c2-c1] = '\0';
		TransitLine->td2 = (float)atof(Value);

		/* ut3 field */
		c2++;
      while( (InputString[c2] == ' ') && (c2 < slen) )
			c2++;
		c1 = c2;
      while( (InputString[c2] != ' ') && (c2 < slen) )
			c2++;

		strncpy(Value,&InputString[c1],(c2-c1));
		Value[c2-c1] = '\0';
		TransitLine->td3 = (float)atof(Value);

		TrLine[NumRoutes] = TransitLine;

		/* set default dwell time and stop flags */
		dwell = 0.01f;
		Stop  = TRUE;
		CurDir = 1;

		goto NEXTLINE;
		} /* if "a" */

	/* move to next field */
	while(c2 < slen) {

		while( (InputString[c2] == ' ') && (c2 < slen) )
			c2++;

		if(! isdigit(InputString[c2]) ) {

			/* dwell time */
			if( (InputString[c2] == 'd') && (InputString[c2+2] == 't') ) {
				c2 += 4; c1 = c2;
				while( (InputString[c2] == ' ') && (c2 < slen) )
					c2++;

				special = ' ';
				if( (! isdigit(InputString[c2])) && (InputString[c2] != '.') ) {
					special = InputString[c2];
					c2++;
					}

				while( (InputString[c2] != ' ') && (c2 < slen) )
					c2++;

				strncpy(Value,&InputString[c1],(c2-c1));
				Value[c2-c1] = '\0';
				dwell = (float)atof(Value);

				if(dwell > 0)
					Stop = TRUE;

				/* modify stop based on special characters */
				if(special == '#')
					Stop = FALSE;
				else
				if(special == '+')
					Stop = TRUE;
				else
				if(special == '*')
					Stop = TRUE;

				} /* if dwell time */

			/* layover */
			else
			if( (InputString[c2] == 'l') && (InputString[c2+2] == 'y') ) {
				c2 += 4; c1 = c2;
				while( (InputString[c2] == ' ') && (c2 < slen) )
					c2++;

				/* read over layover value */
				while( (InputString[c2] != ' ') && (c2 < slen) )
					c2++;

				CurDir++;
				if(CurDir > MAXDIR)
					CurDir = MAXDIR;

				} /* if layover */

			else {

				/* what ever it is - skip it */
				while( (InputString[c2] != ' ') && (c2 < slen) )
					c2++;
				}

			} /* ! isdigit */

		else {

			/* we have a node number */
			c1 = c2;
			while( (InputString[c2] != ' ') && (c2 < slen) )
				c2++;

			strncpy(Value,&InputString[c1],(c2-c1));
			Value[c2-c1] = '\0';

			RealNode = atol(Value);

			if(RealNode != 0) {
				Chain = ((struct RouteList *) malloc (sizeof(struct RouteList)));
				if(Chain == NULL) {
					printf ("\nerror: out of memory, line# %d\n",NumRoutes);
					return;
					}

				if(Stop)
					RealNode = (-1*RealNode);

				Chain->next = NULL;
				Chain->Node = RealNode;
				Chain->Dwell= dwell;
				Chain->Dir  = CurDir;

				if(! FoundNodes) {
					TrLine[NumRoutes]->Route = Chain;
					FoundNodes = TRUE;
					}
				else
					add_to_route(TrLine[NumRoutes]->Route, Chain);
				}
			}

		c2++;
		} /* while c2 < slen */

	NEXTLINE:
	;
	} /* while ! EOF */

printf("%d found\n",NumRoutes);
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

/*clrscr();*/
printf("\n");
printf("%s (TM) TRANSIT WALK LINK GENERATION PROGRAM\n",ProgName);
printf("Ver %s  Rel %s  (%s-Bit Version)\n", VER, REL, BIT);
for(i=1; i < 52; i++)
	printf("-");
printf("\n");
}

/*************************************************************************/
void  rpt_header(FILE *ptr)
{
int i;

fprintf(ptr,"%s (TM) TRANSIT WALK LINK GENERATION PROGRAM\n",ProgName);
fprintf(ptr,"Ver %s  Rel %s  (%s-Bit Version)\n", VER, REL, BIT);
for(i=1; i < 52; i++)
	fprintf(ptr,"-");
fprintf(ptr,"\n");
fflush(ptr);
}

/*************************************************************************/
void read_m2cents(void)
{
int  lines, endfile;

CentCount = lines = 0;

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

	if(sscanf(&linebuf[3]," %ld %f %f %f %f %f",
			&cent[CentCount].num,
			&cent[CentCount].x,
			&cent[CentCount].y,
			&cent[CentCount].mindist,
			&cent[CentCount].maxdist,
			&cent[CentCount].maxlinks) < 3) {
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
long n;

NodeCount = lines = 0;

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
while( (linebuf[1] == 'a') && (endfile != EOF) ) {
	sscanf(&linebuf[3]," %ld",&n);

	if(n > HiNodeNumber) {
		printf("\nerror: more than %d nodes found.\n", HiNodeNumber);
		exit(1);
		}

	if(sscanf(&linebuf[3]," %ld %f %f",
		&node[n].num,
		&node[n].x,
		&node[n].y) < 3) {
		printf("\nerror: in \"%s\" invalid file format, line %d\n",
			NodeFile,lines);
		exit(1);
		}
	node[n].x = node[n].x / NetworkScale;
	node[n].y = node[n].y / NetworkScale;

	endfile = fget_line(&linebuf[1],MAXSTR,iptr);
	lines++;
	NodeCount++;
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
void read_tplines(void)
{
char InputString[MAXSTR];
char Value[15];
long RealNode;
float dwell;
int FoundNodes, CurDir;
int i, c1, c2, slen, n;
struct RouteList *Chain;
struct LineDesc  *TransitLine;
FILE *fp;

NumRoutes = 0;

/* Open the Transit Line file for reading */
if ((fp = fopen(TransitFile, "r")) == NULL) {
	printf("\nerror: opening file \"%s\" for reading.\n",TransitFile);
	exit(1);
	}

printf("\nReading transit lines...");

while( (fgets(&InputString[0], MAXSTR, fp)) != NULL) {

	c1 = 0;
	slen = strlen(InputString);

	if( str_index(InputString, "&ROUTE") >= 0) {
		NumRoutes++;
		FoundNodes = FALSE;
		TransitLine = ((struct LineDesc *) malloc (sizeof(struct LineDesc)));

		if(TransitLine == NULL) {
			printf ("\nerror: out of memory, route# %d\n",NumRoutes);
			return;
			}

		for(i=0; i <= MAXDIR; i++)
			TransitLine->Dir[i] = 0;

		/* transit line number */
		if( (c1=(str_index(InputString, "L="))) >= 0) {
			c1 += 2;  c2 = c1;
			while( (InputString[c2] == ' ') && (c2 < slen) )
				c2++;
			c1 = c2;
			while( (InputString[c2] != ',') && (c2 < slen) )
				c2++;
			strncpy(TransitLine->Line,&InputString[c1],(c2-c1));
			TransitLine->Line[c2-c1] = '\0';
			}

		/* transit line mode */
		if( (c1=(str_index(InputString, "M="))) >= 0) {
			c1 += 2;  c2 = c1;
			while( (InputString[c2] != ',') && (c2 < slen) )
				c2++;
			strncpy(Value,&InputString[c1],(c2-c1));
			Value[c2-c1] = '\0';
			TransitLine->iMode = atoi(Value);
			if(TransitLine->iMode > 999) {
				printf ("\nerror: invalid mode number, route# %d\n",NumRoutes);
				exit(-1);
				}

			}

		/* transit line description */
		if( (c1=(str_index(InputString, "ID="))) >= 0) {
			c1 += 2;  c2 = c1;
			while( (InputString[c2] != '\'') && (c2 < slen) )
				c2++;

			c2++;  c1 = c2;
			while( (InputString[c2] != '\'') && (c2 < slen) )
				c2++;

			strncpy(TransitLine->Desc,&InputString[c1],(c2-c1));
			TransitLine->Desc[c2-c1] = '\0';
			}

		TrLine[NumRoutes] = TransitLine;

		/* set default dwell time and direction flags */
		dwell = 0.01f;
		CurDir = 1;

		} /* if "&ROUTE" */

	/* move to first node */
	if( (n=(str_index(InputString, "N="))) >= 0) {
		FoundNodes = TRUE;

		c1 = n + 2;  c2 = c1;
		while( (InputString[c2] != ',') && (c2 < slen) )
			c2++;
		strncpy(Value,&InputString[c1],(c2-c1));
		Value[c2-c1] = '\0';
		c1 = c2 + 1;

		RealNode = atol(Value);

		Chain = ((struct RouteList *) malloc (sizeof(struct RouteList)));
		if(Chain == NULL) {
			printf ("\nerror: out of memory, route# %d\n",NumRoutes);
			return;
		  }

		Chain->next = NULL;
		Chain->Node = RealNode;
		Chain->Dwell= dwell;
		Chain->Dir  = CurDir;

		TrLine[NumRoutes]->Route = Chain;
		}

	/* look for nodes */
	while( (c1 < slen) && (FoundNodes) ) {

		c2 = c1;
		while( ( (InputString[c2] != '-') && (! isdigit(InputString[c2])) ) &&
				 (c2 < slen) )
			c2++;

		c1 = c2;
		while( (InputString[c2] != ',') && (c2 < slen) )
			c2++;

		strncpy(Value,&InputString[c1],(c2-c1));
      Value[c2-c1] = '\0';
		c1 = c2 + 1;

		RealNode = atol(Value);

		if(RealNode != 0) {
			Chain = ((struct RouteList *) malloc (sizeof(struct RouteList)));
			if(Chain == NULL) {
				printf ("\nerror: out of memory, route# %d\n",NumRoutes);
				return;
				}
         Chain->next = NULL;
			Chain->Node = RealNode;
			Chain->Dwell= dwell;
			Chain->Dir  = CurDir;

			add_to_route(TrLine[NumRoutes]->Route, Chain);
			}

		} /* while < slen) */

	} /* while ! EOF */

printf("%d found\n",NumRoutes);

}

/*************************************************************************/
void read_mplines(void)
{
char InputString[MAXSTR];
char tempDescription[MAXSTR];
char Value[15];
long RealNode;
float dwell;
int FoundNodes, CurDir, lineNum;
int i, c1, c2, slen, n;
int startLineIndex, index;
int numNodesInRoute;
struct RouteList *Chain;
struct LineDesc  *TransitLine;
FILE *fp;


NumRoutes = 0;
lineNum = 0;

/* Open the Transit Line file for reading */
if ((fp = fopen(TransitFile, "r")) == NULL) {
	printf("\nerror: opening file \"%s\" for reading.\n",TransitFile);
	exit(1);
	}

printf("\nReading transit lines...");

try {

	while( (fgets(&InputString[0], MAXSTR, fp)) != NULL) {

		c1 = 0;
		slen = strlen(InputString);

		/* transit line description */
		if( (c1=(str_index(InputString, "$"))) >= 0) {
			c2 = c1;
			while( (InputString[c2] == ' ') && (c2 < slen) )
				c2++;
			strcpy(tempDescription,&InputString[c2]);
			FoundNodes = FALSE;
			}

		startLineIndex = str_index(InputString, "LINE ");
		if( startLineIndex >= 0 ) {
			numNodesInRoute=0;
			NumRoutes++;
			FoundNodes = FALSE;
			TransitLine = ((struct LineDesc *) malloc (sizeof(struct LineDesc)));

			if(TransitLine == NULL) {
				printf ("\nerror: out of memory, route# %d\n",NumRoutes);
				return;
				}

			for(i=0; i <= MAXDIR; i++)
				TransitLine->Dir[i] = 0;

		/* transit line name */
			if( (c1=startLineIndex+strlen("LINE ")) >= 0) {
				c2 = c1;
				while( (InputString[c2] == ' ') && (c2 < slen) )
					c2++;
				c1 = c2;
				while( (InputString[c2] != ' ') && (InputString[c2] != '*') && (c2 < slen) )
					c2++;
				strncpy(TransitLine->Line,&InputString[c1],(c2-c1));
				TransitLine->Line[c2-c1] = '\0';

				if ( (strcmp(TransitLine->Line, "AAW")) == 0 ) {
					i = 0;
					}
				}

			/* transit line mode */
			c1 = -1;
			while( true )
				if ( tolower(InputString[++c1]) == 'm' && tolower(InputString[c1+1]) == 'o' && InputString[c1+2] == '=' )
					break;
			if( c1 >= 0) {
				c1 += 3;  c2 = c1;
				while( (InputString[c2] != ',') && (c2 < slen) )
					c2++;
				strncpy(Value,&InputString[c1],(c2-c1));
				Value[c2-c1] = '\0';
				TransitLine->iMode = atoi(Value);
				if(TransitLine->iMode > 10 || TransitLine->iMode < 1) {
					printf ("\nerror: invalid mode number, route# %d, name=%s\n",NumRoutes, TransitLine->Line);
					exit(-1);
					}

				}

			/* am period headway value */
			index = str_index(InputString, "H1");
			if ( index < 0 )
				index = str_index(InputString, "h1");
			if ( index >= 0 ) {
				c1 = -1;
				while( true )
					if ( tolower(InputString[++c1]) == 'h' && tolower(InputString[c1+1]) == '1' && InputString[c1+2] == '=' )
						break;
				if( c1 >= 0) {
					c1 += 3;  c2 = c1;
					while( (InputString[c2] != ',') && (c2 < slen) )
						c2++;
					strncpy(Value,&InputString[c1],(c2-c1));
					Value[c2-c1] = '\0';
					TransitLine->H1 = (float)atof(Value);
					if(TransitLine->H1 < 0 || TransitLine->H1 > 999) {
						printf ("\nerror: invalid H1=%d value, outside of range [0,999], route# %d, name=%s\n", TransitLine->H1, NumRoutes, TransitLine->Line);
						exit(-1);
						}

					}
				}

			/* md period headway value */
			index = str_index(InputString, "H2");
			if ( index < 0 )
				index = str_index(InputString, "h2");
			if ( index >= 0 ) {
				c1 = -1;
				while( true )
					if ( tolower(InputString[++c1]) == 'h' && tolower(InputString[c1+1]) == '2' && InputString[c1+2] == '=' )
						break;
				if( c1 >= 0) {
					c1 += 3;  c2 = c1;
					while( (InputString[c2] != ',') && (c2 < slen) )
						c2++;
					strncpy(Value,&InputString[c1],(c2-c1));
					Value[c2-c1] = '\0';
					TransitLine->H2 = (float)atof(Value);
					if(TransitLine->H2 < 0 || TransitLine->H2 > 999) {
						printf ("\nerror: invalid H2=%d value, outside of range [0,999], route# %d, name=%s\n", TransitLine->H2, NumRoutes, TransitLine->Line);
						exit(-1);
						}

					}
				}

			/* pm period headway value */
			index = str_index(InputString, "H3");
			if ( index < 0 )
				index = str_index(InputString, "h3");
			if ( index >= 0 ) {
				c1 = -1;
				while( true )
					if ( tolower(InputString[++c1]) == 'h' && tolower(InputString[c1+1]) == '3' && InputString[c1+2] == '=' )
						break;
				if( c1 >= 0) {
					c1 += 3;  c2 = c1;
					while( (InputString[c2] != ',') && (c2 < slen) )
						c2++;
					strncpy(Value,&InputString[c1],(c2-c1));
					Value[c2-c1] = '\0';
					TransitLine->H3 = (float)atof(Value);
					if(TransitLine->H3 < 0 || TransitLine->H3 > 999) {
						printf ("\nerror: invalid H3=%d value, outside of range [0,999], route# %d, name=%s\n", TransitLine->H3, NumRoutes, TransitLine->Line);
						exit(-1);
						}

					}
				}



			/* transit line description */
			strcpy(TransitLine->Desc,tempDescription);


			TrLine[NumRoutes] = TransitLine;

			/* set default dwell time and direction flags */
			dwell = 0.01f;
			CurDir = 1;

			} /* if "LINE " */
		else {
			if (NumRoutes == 0)
				continue;
			}

		/* move to first node */
		if( (c1=(str_index(InputString, "n="))) >= 0 || (c2=(str_index(InputString, "N="))) >= 0 ) {


			if ( c2 > c1)
				c1 = c2;

			c1 += 2;

			while( (c1 < slen) ) {

				if( c1 >= 0) {
					FoundNodes = TRUE;

					c2 = c1;
					while( (InputString[c2] != '-') && (c2 < slen) )
						c2++;
					strncpy(Value,&InputString[c1],(c2-c1));
					Value[c2-c1] = '\0';
					c1 = c2 + 1;

					if ( (n=(str_index(Value, "*"))) > 0) {
						Value[n] = '\0';
						RealNode = atol(Value);
						RealNode = (-1*RealNode); /* stop nodes are assumed to be negative in wlklinks function. */

						Chain = ((struct RouteList *) malloc (sizeof(struct RouteList)));
						if(Chain == NULL) {
							printf ("\nerror: out of memory, route# %d\n",NumRoutes);
							return;
						  }

						Chain->next = NULL;
						Chain->Node = RealNode;
						Chain->Dwell= dwell;
						Chain->Dir  = CurDir;

						if (numNodesInRoute == 0)
							TrLine[NumRoutes]->Route = Chain;
						else
							try {
								add_to_route(TrLine[NumRoutes]->Route, Chain);
							}
							catch ( ... ) {
								printf( "caught exception in read_mplines() on route %s adding node %d.\n", TrLine[NumRoutes]->Line, (int)fabs(RealNode) );
							}

						numNodesInRoute++;
						}
					else {
						continue;
						}
				
					}

				} /* while */

			}	/* if n= or N= */
		else {
			c1 = 0;
			}


		/* look for nodes */
		while( (c1 < slen) && (FoundNodes) ) {

			c2 = c1;
			while( ( (InputString[c2] != '-') && (InputString[c2] != ' ') && (InputString[c2] != '\r') && (! isdigit(InputString[c2])) ) && (c2 < slen) )
				c2++;

			c1 = c2;
			while( (InputString[c2] != '-') && (InputString[c2] != ' ') && (InputString[c2] != '\r') && (c2 < slen) )
				c2++;

			strncpy(Value,&InputString[c1],(c2-c1));
			Value[c2-c1] = '\0';
			c1 = c2 + 1;

			if ( (n=(str_index(Value, "*"))) > 0) {
				Value[n] = '\0';
				RealNode = atol(Value);
				RealNode = (-1*RealNode); /* stop nodes are assumed to be negative in wlklinks function. */

				if(RealNode != 0) {
					Chain = ((struct RouteList *) malloc (sizeof(struct RouteList)));
					if(Chain == NULL) {
						printf ("\nerror: out of memory, route# %d\n",NumRoutes);
						return;
						}
					Chain->next = NULL;
					Chain->Node = RealNode;
					Chain->Dwell= dwell;
					Chain->Dir  = CurDir;

					try {
						add_to_route(TrLine[NumRoutes]->Route, Chain);
					}
					catch ( ... ) {
						printf( "caught exception in read_mplines() on route %s adding node %d.\n", TrLine[NumRoutes]->Line, (int)fabs(RealNode) );
					}

					numNodesInRoute++;
					}
				
				} /* if "*" */

			} /* while < slen) */

		} /* while ! EOF */

	}
	catch ( ... ) {
		printf( "caught exception in read_mplines() on route %s in main while().\n", TrLine[NumRoutes]->Line );
		fflush(stdout);
	}

printf("%d found\n",NumRoutes);

}

/*************************************************************************/
void read_mpstations(void)
{
char InputString[MAXSTR];
char temp[MAXSTR];
int stationNode, hwyNode1, hwyNode2, dist1, dist2;
int i, c1, c2, k;
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


	if (hwyNode1 > 0) {
		stn[stationNode].station = stationNode;
		stn[stationNode].hwy[0] = hwyNode1;
		stn[stationNode].d[0] = dist1;
		stn[stationNode].hwy[1] = hwyNode2;
		stn[stationNode].d[1] = dist2;
		NumStations++;
		}

	} /* while ! EOF */

printf("%d found\n",NumStations);

}


/*************************************************************************/
void read_mplinks(void)
{
int linkCount, lines, endfile;
int an, bn, dist, limita, limitm, limitp;
struct cent_link *link;
struct cent_link *nlink;

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

/* start reading nodes - first one has been read */
while( endfile != EOF ) {

	lines++;

	sscanf(linebuf,"%d %d %*s %d %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %d %d %d %*s %*s %*s %*s %*s %*s",&an, &bn, &dist, &limita, &limitm, &limitp);

	if(an > HiNodeNumber || bn > HiNodeNumber) {
		printf("\nerror: link (%d,%d) has one of its nodes > specified largest node number %d.\n", an, bn, HiNodeNumber);
		exit(1);
		}


	if (an < CentCount) {
		if ( (link = ((struct cent_link *) malloc (sizeof(struct cent_link)))) == NULL )
			printf ("error allocating centroid link for an = %d.\n", an);
		link->dist = 0.0;
		link->hwyNode = 0;
		link->isCandidate = 0;
		link->onRoute = 0;
		link->next = NULL;

		link->hwyNode = bn;
		link->dist = (float)(dist/100.0);

		if (cent[an].links == NULL) {
			cent[an].links = link;
			}
		else {
			if (cent[an].links->next == NULL) {
				cent[an].links->next = link;
				}
			else {
				nlink = cent[an].links->next;
				while (nlink->next != NULL)
					nlink = nlink->next;
				nlink->next = link;
				}
			}
		}

	else if (bn < CentCount) {
		if ( (link = ((struct cent_link *) malloc (sizeof(struct cent_link)))) == NULL )
			printf ("error allocating centroid link for bn = %d.\n", bn);
		link->dist = 0.0;
		link->hwyNode = 0;
		link->isCandidate = 0;
		link->onRoute = 0;
		link->next = NULL;

		link->hwyNode = an;
		link->dist = (float)(dist/100.0);

		if (cent[bn].links == NULL) {
			cent[bn].links = link;
			}
		else {
			if (cent[bn].links->next == NULL) {
				cent[bn].links->next = link;
				}
			else {
				nlink = cent[bn].links->next;
				while (nlink->next != NULL)
					nlink = nlink->next;
				nlink->next = link;
				}
			}
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

if(LinkFormat == 'e') {
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
	ltime = (long)((dist/WalkSpeed)*60*10);

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
				cent,cnode,newlink.modesout,fdist,WalkSpeed);
			LinkCount++;
			fprintf(optr,FormatString,
				cnode,cent,newlink.modesin,fdist,WalkSpeed);
			LinkCount++;
			break;
		case 'i':
			fprintf(optr,FormatString,
				cnode,cent,newlink.modesin,fdist,WalkSpeed);
			LinkCount++;
			break;
		case 'o':
			fprintf(optr,FormatString,
				cent,cnode,newlink.modesout,fdist,WalkSpeed);
			LinkCount++;
			break;
		}

	} /* if LinkFormat == i */
else
if(LinkFormat == 'm') {

	fprintf (optr, "link %04d-%04d,%03d,%.1f\n", cent, cnode, (int)(dist*100.0), WalkSpeed);

	} /* if LinkFormat == m */

}
