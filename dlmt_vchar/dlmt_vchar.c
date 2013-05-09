/****************************************************************************
 *  Filename    : dlmt_vchar.c
 *  Type        : c
 ****************************************************************************
 *
 *  Program     dlmt_vchar
 *  ======= 
 *
 *  Purpose     FastExport 'pass through' outmod to export VARCHAR
 *  =======     data into delimited file format without "2 byte issue".
 *  
 *  Comments    - Use as 'MODE RECORD FORMAT TEXT OUTMOD <path_to_outmod>/dlmt_vchar.so'
 *  ========      in FastExport .EXPORT clause.  Use dlmt_vchar.dll rather than 
 *                dlmt_vchar.so  in the outmod clause when executing on windows 
 *                platforms.
 *
 *              - All columns in the select statement MUST be of VARCHAR
 *                type. Use 'cast' if necessary.  If you don't do this the 
 *                export will fail!
 *
 *              - By default columns are delimited by '|' (pipe). Default value
 *                can be overriden by FEXP_DELIMITER environment variable.
 *
 *              - Optionaly column data may be enclosed in quotes. Export
 *                FEXP_QUOTE=\' or FEXP_QUOTE=\" for single or double qoutes.
 *
 *              - Checkpoints and restarts ARE NOT SUPPORTED
 *
 *              ==============================================================
 *              - This program is field developed software, and as such is 
 *                supplied as is, with no warranty whatsoever.
 *              ==============================================================
 *
 *  Sample Fastexport Script:
 *  =========================
 *
 *              .logtable uid.test_dbase;                   
 *              .logon tdpid/uid,passwd;                    
 *                                                          
 *              .begin export sessions 4;                   
 *              .export                                     
 *                  outfile test_dbase.out                  
 *                  outmod /home/dheard/outmod/dlmt_vchar.so
 *                  MODE RECORD FORMAT TEXT;                
 *                                                          
 *              select                                      
 *                 cast(OwnerName  as VARCHAR(30))          
 *                ,cast(DatabaseName as VARCHAR(30))        
 *                ,CommentString                            
 *                ,cast(PermSpace  as VARCHAR(20))          
 *                ,cast(SpoolSpace as VARCHAR(20))          
 *              from                                        
 *                DBC.DBase                                 
 *              order by                                    
 *                1, 2;                                     
 *                                                          
 *              .end export;                                
 *              .logoff;                                    
 *
 *  Compilation:
 *  ============
 *
 *  MP-RAS:-    cc -G -KPIC -o dlmt_vchar.so dlmt_vchar.c
 *
 *  Linux:-     gcc -shared -o dlmt_vchar.so dlmt_vchar.c
 *
 *  Solaris:-   gcc -shared -o dlmt_vchar.so dlmt_vchar.c
 *
 *              NOTE: May want to add -fPIC or -fpic for solaris, I am just
 *                    pulling this from memory as I don't currently have
 *                    access to a solaris box
 *
 *  Win32:-     cl /DWIN32 /LD dlmt_vchar.c
 *
 *              NOTE: Ensure you run vcvars32.bat (see your visual c installation
 *                    directory) before you execute this command so that the
 *                    compiler paths are set up correctly!  I have verified this
 *                    using Visual C++ 6.0, VC.Net 2003 and VC.Net 2005 (Express)
 *
 *  -------------------------------------------------------------------------
 *  Project     : N/A
 *  Subproject  : N/A
 *  Department  : NCR Teradata
 *  Version     : 1.2
 *  Date        : 10/02/2006
 *
 *  (c) 2004, 2005, 2006 by NCR Australia
 *  -------------------------------------------------------------------------
 *  History:
 *
 *  1.0    21/06/2004   Eugene Svistunov
 *  - Initial Version 
 *
 *  1.1    18/03/2005   Duncan Heard
 *  - Added debugging code (hexdump routine and extra reporting)
 *  - Added compile command lines for additional operating systems 
 *  - Corrected some typos
 *
 *  1.2    10/02/2006   Duncan Heard
 *  - Removed debugging code
 *  - Added handling for incorrect data being retrieved to avoid core dumps
 *  - Added additional reporting of delimiter/quotes being used
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned short Int16;
typedef unsigned char Int8;
typedef unsigned long int Int32;

/* Global vars*/
char Delimiter = '|';
char Quote = 0;
long RecCount = 0;
int FoundError = 0;

/* Prototypes */
void init(void);
int proses(int *, char *, int *, char *);
int replace(char *, int, char *, char);

/*********************************************************************/
/* Main Entry Point:                                                 */
/*   Blatant plagiarism from FastExport manual ;-)                   */
/*********************************************************************/
#ifdef WIN32 /* Change for WIN32 */
__declspec(dllexport) Int32 _dynamn( int *code,
        int *stmno,
        int *InLen,
        char *InBuf,
        int *OutLen,
        char *OutBuf
)
#else
Int32 _dynamn(code, stmno, InLen, InBuf, OutLen, OutBuf )
        int *code;
        int *stmno;
        int *InLen;
        char *InBuf;
        int *OutLen;
        char *OutBuf;
#endif
{
        switch (*code) {
        case 1: /* Initialization, no other values */
                fprintf(stdout, "**** OUTMOD Commencing export using dlmt_vchar (v1.2 10/02/2006)\n");
                fprintf(stdout, "     Copyright 2004-2006 NCR Australia\n");
                init();
                break;
        case 2: /* Cleanup call, no other values */
                fprintf(stdout, "**** OUTMOD Finishig export. %d records processed.\n", RecCount);
                break;
        case 3: /* Process response record */
                *OutLen = process(InLen, InBuf, OutLen, OutBuf);
                /* Require FastExport to write a record */
                *InLen=0;
                RecCount++;
                break;
        case 4: /* Checkpoint, no other values */
                fprintf(stdout, "**** OUTMOD Checkpoint Entry\n");
                break;
        case 5: /* DBC restart - close and reopen the output files */
                fprintf(stdout, "**** OUTMOD DBC Restart Entry\n");
                break;
        case 6: /* Host restart */
                fprintf(stdout, "**** OUTMOD Host Restart Entry\n");
                break;
        default:
                fprintf(stdout, "**** OUTMOD Invalid Entry code\n");
                break;
        }
        return (FoundError);
}

/*********************************************************************/
/* init: Check environment variables FEXP_DELIMITER and FEXP_QUOTE   */
/*       and set Delimiter and Quote global vars if needed           */
/*********************************************************************/
void init (void) {
        
        char *env;
        
        if(env = getenv("FEXP_DELIMITER")) {
                Delimiter = *env;
        } 
        fprintf(stdout, "**** OUTMOD Using delimiter character %c\n", Delimiter);
        if(env = getenv("FEXP_QUOTE")) {
                Quote = *env;
                fprintf(stdout, "**** OUTMOD Fields will be quoted using %c\n", Quote);
        } else {
                fprintf(stdout, "**** OUTMOD Fields will NOT be quoted\n");
        }
        return;
}

/*********************************************************************/
/* process: Copy data from *InBuf to *Outbuf with delimiter and      */
/*          quotes if required. Skip over 2 byte VARCHAR length      */
/*********************************************************************/
int process(int *InLen, char *InBuf, int *OutLen, char *OutBuf) {

        char *from_col = InBuf, *to_col = OutBuf;
        Int16 *col_len;
        int in_len = *InLen, out_len = 0;
        
        while(in_len) {
                /* Insert delimiter for each but first column */
                if(in_len != *InLen) {
                        *to_col++ = Delimiter;
                        out_len++;
                }
                
                /* Insert pre-field Quote if defined*/
                if(Quote) {
                        *to_col++ = Quote;
                        out_len++;
                }
                
                /* Get varchar length */
                col_len  = (Int16*)from_col;
                
                /* Copy field data to output record */
                memcpy(to_col, from_col+2, *col_len);
                replace(to_col, *col_len, "\n\x00", ' ');

                /* Move the pointers */
                from_col += *col_len + 2;
                in_len   -= *col_len + 2;
                to_col   += *col_len;
                out_len  += *col_len;
                
                /* Insert post-field Quote if defined*/
                if(Quote) {
                        *to_col++ = Quote;
                        out_len++;
                }
                
                /* Check we have not gone out of bounds */
                if (in_len < 0) {
                    /* OOOPS - we have an error - probably data which 
                       was not cast to varchar when it was selected 
                       from the database, or alternatively    
                       MODE RECORD FORMAT TEXT was not specified
                       - In this case we return a null string (out_len = 0)
                         and set the global error found variable so we can
                         terminate the job.
                    */
                    fprintf(stdout, "**** ERROR: Bad record encountered\n");
                    fprintf(stdout, "     Please ensure that all fields in SQL are retrieved as VARCHAR\n");
                    fprintf(stdout, "     and export command contains MODE RECORD FORMAT TEXT!\n");
                    FoundError = 1;
                    return(0);
                }
        }
        *OutLen = out_len;
        return(out_len);
}

/* replace function - add by Simon */
int replace(char *buf, int len, char *src, char tgt) {
  int i = 0;
  char *s = buf;
  while (i < len) {
    if (strchr(src, *s) != NULL) {
      *s = tgt;
    }
    i++;
    s++;
  }
}
