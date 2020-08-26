/*   ctlutil.h

     Standard Control File Utilities.

     Copyright (c) Tim Heier 1995
     All Rights Reserved.
*/


void GetPrivateProfileString(char *Section, char *Entry, char *DefString,
			     char *RetString, char *FilePath);
float GetPrivateProfileFloat(char *Section, char *Entry, float DefVal,
			     char *FilePath);
int GetPrivateProfileInt(char *Section, char *Entry, int DefVal,
			 char  *FilePath);
int GetPrivateProfileBool(char *Section,char *Entry, int DefVal,
			  char *FilePath);
int FindSec(char *section, int maxstr, int clen, FILE *fptr);

