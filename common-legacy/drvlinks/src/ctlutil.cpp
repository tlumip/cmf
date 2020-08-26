#include "stdutil.h"
#include "ctlutil.h"

/*********************************************************************/
void GetPrivateProfileString(char *Section,
			     char *Entry,
			     char *DefString,
			     char *RetString,
			     char *FilePath)
{
int BufLen, c;
int SectionSize;
int EntrySize;
char *str;

FILE *fptr;

if ((str = (char *) malloc(100)) == NULL)
   terror("not enough memory to allocate buffer",-1,NULL);

if( (fptr=fopen(FilePath,"r")) == NULL) {
	sprintf (str, "file %s not found for reading.", FilePath);
    terror(str,-1,NULL);
}

strcpy(RetString,DefString);

SectionSize = strlen(Section);
EntrySize = strlen(Entry);

for(c=0; c <= SectionSize; c++)
   Section[c] = toupper(Section[c]);
for(c=0; c <= EntrySize; c++)
   Entry[c] = toupper(Entry[c]);

while( fget_line(str,99,fptr) != EOF) {
   if(str[0] == '[') {
      BufLen=strlen(str);
      for(c=0; c < BufLen; c++)
	 str[c] = toupper(str[c]);

      if(strncmp(Section,&str[1],SectionSize) == 0) {
	 while( (fget_line(str,99,fptr) != EOF) &&
		(str[0] != '[') ) {

	    BufLen=strlen(str);
	    for(c=0; c < BufLen; c++) {
	       if(str[c] == '=')
		  break;
	       str[c] = toupper(str[c]);
	       }

	    if(strncmp(Entry,str,EntrySize) == 0) {
	       if( (c = str_index(str,"=")) < 0)
		  goto END;
	       while(str[++c] == ' ')
		  ;
	       strcpy(RetString,&str[c]);
	       goto END;
	       }
	    }
	 }
      }
   }
END:
free(str);
fclose(fptr);
return;
}

/*********************************************************************/
float GetPrivateProfileFloat(char  *Section,
			     char  *Entry,
			     float  DefVal,
			     char  *FilePath)
{
int BufLen, c;
int SectionSize;
int EntrySize;
char *str;
float RetVal;

FILE *fptr;

if ((str = (char *) malloc(100)) == NULL)
   terror("not enough memory to allocate buffer",-1,NULL);

if( (fptr=fopen(FilePath,"r")) == NULL) {
	sprintf (str, "file %s not found for reading.", FilePath);
    terror(str,-1,NULL);
}

RetVal = DefVal;

SectionSize = strlen(Section);
EntrySize = strlen(Entry);

for(c=0; c <= SectionSize; c++)
   Section[c] = toupper(Section[c]);
for(c=0; c <= EntrySize; c++)
   Entry[c] = toupper(Entry[c]);

while( fget_line(str,99,fptr) != EOF) {
   if(str[0] == '[') {
      BufLen=strlen(str);
      for(c=0; c < BufLen; c++)
	 str[c] = toupper(str[c]);

      if(strncmp(Section,&str[1],SectionSize) == 0) {
	 while( (fget_line(str,99,fptr) != EOF) &&
		(str[0] != '[') ) {

	    BufLen=strlen(str);
	    for(c=0; c < BufLen; c++)
			 str[c] = toupper(str[c]);

	    if(strncmp(Entry,str,EntrySize) == 0) {
	       if( (c = str_index(str,"=")) < 0)
		  goto END;
	       sscanf(&str[c+1],"%f",&RetVal);
	       goto END;
	       }
	    }
	 }
      }
   }
END:
free(str);
fclose(fptr);
return (RetVal);
}

/*********************************************************************/
int GetPrivateProfileInt(char  *Section,
			 char  *Entry,
			 int    DefVal,
			 char  *FilePath)
{
int BufLen, c;
int SectionSize;
int EntrySize;
char *str;
int RetVal;

FILE *fptr;

if ((str = (char *) malloc(100)) == NULL)
   terror("not enough memory to allocate buffer",-1,NULL);

if( (fptr=fopen(FilePath,"r")) == NULL) {
	sprintf (str, "file %s not found for reading.", FilePath);
    terror(str,-1,NULL);
}

RetVal = DefVal;

SectionSize = strlen(Section);
EntrySize = strlen(Entry);

for(c=0; c <= SectionSize; c++)
   Section[c] = toupper(Section[c]);
for(c=0; c <= EntrySize; c++)
   Entry[c] = toupper(Entry[c]);

while( fget_line(str,99,fptr) != EOF) {
	if(str[0] == '[') {
      BufLen=strlen(str);
      for(c=0; c < BufLen; c++)
	 str[c] = toupper(str[c]);

      if(strncmp(Section,&str[1],SectionSize) == 0) {
	 while( (fget_line(str,99,fptr) != EOF) &&
		(str[0] != '[') ) {

	    BufLen=strlen(str);
	    for(c=0; c < BufLen; c++)
	       str[c] = toupper(str[c]);

	    if(strncmp(Entry,str,EntrySize) == 0) {
	       if( (c = str_index(str,"=")) < 0)
		  goto END;
	    sscanf(&str[c+1],"%d",&RetVal);
	       goto END;
	       }
	    }
	 }
      }
   }
END:
free(str);
fclose(fptr);
return (RetVal);
}

/*********************************************************************/
int GetPrivateProfileBool(char  *Section,
			  char  *Entry,
			  int    DefVal,
			  char  *FilePath)
{
int BufLen, c;
int SectionSize;
int EntrySize;
char *str;
int RetVal;

FILE *fptr;

if ((str = (char *) malloc(100)) == NULL)
	terror("not enough memory to allocate buffer",-1,NULL);

if( (fptr=fopen(FilePath,"r")) == NULL) {
	sprintf (str, "file %s not found for reading.", FilePath);
    terror(str,-1,NULL);
}

RetVal = DefVal;

SectionSize = strlen(Section);
EntrySize = strlen(Entry);

for(c=0; c <= SectionSize; c++)
   Section[c] = toupper(Section[c]);
for(c=0; c <= EntrySize; c++)
   Entry[c] = toupper(Entry[c]);

while( fget_line(str,99,fptr) != EOF) {
	if(str[0] == '[') {
      BufLen=strlen(str);
      for(c=0; c < BufLen; c++)
	 str[c] = toupper(str[c]);

		if(strncmp(Section,&str[1],SectionSize) == 0) {
	 while( (fget_line(str,99,fptr) != EOF) &&
		(str[0] != '[') ) {

	    BufLen=strlen(str);
	    for(c=0; c < BufLen; c++)
			 str[c] = toupper(str[c]);

	    if(strncmp(Entry,str,EntrySize) == 0) {
	       if( (c = str_index(str,"=")) < 0)
		  goto END;
	       while(str[++c] == ' ')
		  ;

	       if(strncmp(&str[c],".T",2) == 0)
		  RetVal = 1;
	       else
	       if(strncmp(&str[c],"T",1) == 0)
		  RetVal = 1;
			 else
			 if(strncmp(&str[c],"Y",1) == 0)
		  RetVal = 1;
	       else
			 if(strncmp(&str[c],"1",1) == 0)
		  RetVal = 1;
	       else
		  RetVal = 0;

	       goto END;
			 }
	    }
	 }
      }
   }
END:
free(str);
fclose(fptr);
return (RetVal);
}

/*********************************************************************/
int FindSec(char *section, int maxstr, int clen, FILE *fptr)
{
char *buf;
int c, BufLen;
int line;

line = 0;
rewind(fptr);

if ((buf = (char *) malloc(maxstr)) == NULL)
	terror("not enough memory to allocate buffer",-1,NULL);

while( fget_line(buf,maxstr,fptr) != EOF ) {
	line++;
	BufLen=strlen(buf);
	for(c=0; c < BufLen; c++)
		buf[c] = toupper(buf[c]);

	if( (buf[0] == '[') &&
		(strncmp(&buf[1],section,clen) == 0) ) {
		free(buf);
		return(line);
      }
	}

free(buf);
return(0);
}

