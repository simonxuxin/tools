/* Generic function to get to_extract_value by appending a specified date amount to the base date 
 * Author: Simon Xu <xix@ebay.com> */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#define DATE_FORMAT_MAX 64
#define INC_TYPE_MAX	7
#define DATE_FORMAT_TYPE_MAX 7

//extern char *optarg;

enum {YEAR = 0, Year, MONTH, DAY, HOUR, MINUTE, SECOND};

int 	cal[][13] = {{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}, {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};
char 	dateFmt[][5] = {"YYYY", "YY", "MM", "DD", "HH", "MI", "SS"};

struct date {
	int value;
	int index;
};

/* time.h construct for reference
extern clock_t clock(void);
extern double  difftime(time_t, time_t);
extern time_t  mktime(struct tm *);
extern time_t  time(time_t *);
extern char *asctime(const struct tm *);
extern char *ctime (const time_t *);
struct tm   *gmtime(const time_t *);
struct tm   *localtime(const time_t *);
extern size_t  strftime(char *, size_t, const char *, const struct tm *);
*/


#ifdef _LINUX
void cftime(char *s, const char *format, time_t *time)
{
	strftime(s, 256, format, localtime(time));
}
#endif

const char *usage = "\
Usage: 	%s [-c] <-d date> <-f date_format> [-z timezone] <-a increment_amount> <-t increment_type>\n\
Options:\n\
-c                  : check to_extract_value against server current date - optional\n\
-d date             : base date\n\
-f date_format      : base date format(YYYY, YY, MM, DD, HH, MI, SS)\n\
-z timezone         : timezone of base date, must be provided when -c is selected - optional\n\
-a increment_amount : amount of increment_type add to the base date\n\
-t increment_type   : one of the followings: year(s), month(s), day(s), hour(s), minute(s)\n";

/*
void
usage(char *name)
{
	printf("usage: %s [-c] <-d date> <-f date_format> <-i increment_amount> <-t increment_type> [-z timezone]\n", name);
}
*/

int
month_last_day(int year, int month) {
	int leap = 0;

	/* leap? */
	if((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
		leap = 1;

	return cal[leap][month];
}

int
upper(char *s) {
	int i;
	for (i = 0; i < strlen(s); i++)
		s[i] = toupper(s[i]);

	return 0;
}

int
valid_date(int valid_type, struct date* d) {
	switch(valid_type) {
		case YEAR:
			if(d[valid_type].value < 1000 || d[valid_type].value > 9999)
				return -1;
			break;

		case Year:
			if(d[valid_type].value < 0 || d[valid_type].value > 99)
				return -1;
			break;
		
		case MONTH:
			if(d[valid_type].value < 1 || d[valid_type].value > 12)
				return -1;
			break;

		case DAY:
			if(d[valid_type].value < 0 || d[valid_type].value > month_last_day(d[YEAR].value, d[MONTH].value))
				return -1;
			break;

		case HOUR:
			if(d[valid_type].value < 0 || d[valid_type].value > 23)
				return -1;
			break;

		case MINUTE:
		case SECOND:
			if(d[valid_type].value < 0 || d[valid_type].value > 59)
				return -1;
			break;
	}

	return 0;
}

int parse_date(char *date_in, char *fmt_in, int inctype, struct date *date_out) {
	if (strlen(date_in) != strlen(fmt_in)) {
		fprintf(stderr, "Error: Unmatched date value and format\n");
		return -1;
	}

	int i;
	upper(fmt_in);
	for (i = 0; i < DATE_FORMAT_TYPE_MAX; i++) {
		char *p;
		char tmp[5];
		if ((p = strstr(fmt_in, dateFmt[i])) != NULL) {
			date_out[i].index = p - fmt_in;
			char *q = date_in + date_out[i].index;
			strncpy(tmp, q, strlen(dateFmt[i]));
			tmp[strlen(dateFmt[i])] = 0;
			date_out[i].value = atoi(tmp);
			if(valid_date(i, date_out) < 0) {
				fprintf(stderr, "Error: Malformed date value\n");
				return -1;
			}
		}
		else if(inctype >= i && i != YEAR && i != Year) {
			fprintf(stderr, "Error: %s not found in the input date\n", dateFmt[i]);
			return -1;
		}
	}

	/* YEAR? Year? */

	if(date_out[YEAR].index == -1 && date_out[Year].index == -1) {
		fprintf(stderr, "Error: YYYY or YY not found in the input date\n");
		return -1;
	}

	if (date_out[YEAR].index < 0 && date_out[Year].index >= 0) {
		date_out[YEAR].value = 2000 + date_out[Year].value;
	}

	if (date_out[YEAR].index >= 0) {
		date_out[Year].index = -1;
		date_out[Year].value = date_out[YEAR].value - 2000;
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	struct date Date[DATE_FORMAT_TYPE_MAX] = {{0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1}, {0, -1}};
	int 		incAmt;
	char 		dateInFmt[DATE_FORMAT_MAX + 1];
	char 		dateIn[DATE_FORMAT_MAX + 1];
	int 		inctype;
	int 		future_day_check = 0;
	char 		*src_tz = NULL;
	char		*tgt_tz;
	char		*tzenv;
	int 		c;
	int			leastArgs = 4;
	struct 		tm myTm;
	time_t 		incInSeconds;
	time_t 		timeInSeconds;
	time_t  	srvtime;
	time_t		src_offset;
	time_t		tgt_offset;
	time_t		offset;
	char		dateTime[DATE_FORMAT_MAX];
	int 		y,m;
	int 		i;
	int 		cmpntLen;
	char 		strDate[5];


	/* check parameters */
	while((c = getopt(argc, argv, "cd:f:a:t:z:")) != EOF) {
		switch(c) {
			case 'c':
				future_day_check = 1;
				break;
			case 'd':
				if(strlen(optarg) > DATE_FORMAT_MAX) {
					fprintf(stderr, "Error: date format cannot exceed %i characters.\n",DATE_FORMAT_MAX);
					return -1;
				}
				strcpy(dateIn, optarg);
				--leastArgs;
				break;
			case 'f':
				if(strlen(optarg) > DATE_FORMAT_MAX) {
					fprintf(stderr, "Error: date format cannot exceed %i characters.\n",DATE_FORMAT_MAX);
					return -1;
				}
				strcpy(dateInFmt, optarg);
				--leastArgs;
				break;
			case 'a':
				incAmt = atoi(optarg);
				--leastArgs;
				break;
			case 't':
				upper(optarg);
				if (strcmp(optarg, "YEARS") == 0 || strcmp(optarg, "YEAR") == 0)
					inctype = Year;
				else if (strcmp(optarg, "MONTHS") == 0 || strcmp(optarg, "MONTH")==0)
					inctype = MONTH;
				else if (strcmp(optarg, "DAYS") == 0 || strcmp(optarg, "DAY") == 0)
					inctype = DAY;
				else if (strcmp(optarg, "HOURS") == 0 || strcmp(optarg, "HOUR") == 0)
					inctype = HOUR;
				else if (strcmp(optarg, "MINUTES") ==0 || strcmp(optarg, "MINUTE") == 0)
					inctype = MINUTE;
				else {
					fprintf(stderr, "Error: increment_type must be one of: year(s), month(s), day(s), hour(s), minute(s)\n");
					return -1;
				}
				--leastArgs;
				break;
			case 'z':
				src_tz = (char *)malloc(strlen(optarg + 1));
				strcpy(src_tz, optarg);
				break;
			default:
				printf(usage, argv[0]);
				return -1;
		}
	}

	if (leastArgs) {
		printf(usage, argv[0]);
		return -1;
	}

	if(future_day_check && src_tz == NULL) {
		fprintf(stderr, "Error: timezone should be provided when doing future day checking\n");
		return -1;
	}

	if(src_tz != NULL && ! future_day_check)
		free(src_tz);

	if(parse_date(dateIn, dateInFmt, inctype, Date) == -1) {
		return -1;
	}

	switch (inctype) {
		case Year:
			Date[YEAR].value += incAmt;
			Date[Year].value += incAmt;
			break;

		case MONTH:
			y = Date[YEAR].value;
			m = Date[MONTH].value;

			Date[MONTH].value += incAmt;
			if(Date[MONTH].value % 12 == 0) {
				Date[YEAR].value += (Date[MONTH].value / 12) - 1;
				Date[Year].value += (Date[MONTH].value / 12) - 1;
			} else {
				Date[YEAR].value += (Date[MONTH].value / 12);
				Date[Year].value += (Date[MONTH].value / 12);
			}
			Date[MONTH].value %= 12;
			Date[MONTH].value = Date[MONTH].value ? Date[MONTH].value : 12;

			if(Date[DAY].value == month_last_day(y, m))
				Date[DAY].value = month_last_day(Date[YEAR].value, Date[MONTH].value);

			break;

		case DAY:
		case HOUR:
		case MINUTE:
			myTm.tm_year = Date[YEAR].value - 1900;
			myTm.tm_mon = Date[MONTH].value - 1;
			myTm.tm_mday = Date[DAY].value;
			myTm.tm_hour = Date[HOUR].value;
			myTm.tm_min = Date[MINUTE].value;
			myTm.tm_sec = Date[SECOND].value;

			myTm.tm_wday = -1;
			myTm.tm_yday = -1;
			myTm.tm_isdst = -1;


			if (inctype == DAY)
				incInSeconds = incAmt * 24 * 60 * 60;
			else if (inctype == HOUR)
				incInSeconds = incAmt * 60 * 60;
			else if (inctype == MINUTE)
				incInSeconds = incAmt * 60;
	
			if((timeInSeconds = mktime(&myTm)) == -1) {
				fprintf(stderr, "Error: Cannot convert date into seconds");
				return -1;
			}

			timeInSeconds += incInSeconds;
		
			if(future_day_check) {
				tzset();
				tgt_offset = timezone;
				if((tgt_tz = getenv("TZ")) == NULL) {
					fprintf(stderr, "Error: Cannot get Server timezone\n");
					return -1;
				}
				if((tzenv = (char *)malloc(strlen(src_tz) + 4)) == NULL) {
					perror("malloc");
					return -1;
				}
				sprintf(tzenv, "TZ=%s", src_tz);
				if(putenv(tzenv) != 0) {
					fprintf(stderr, "Error: Cannot set Server timezone\n");
					return -1;
				}
				/*
				if(setenv("TZ", src_tz, 1) != 0) {
					fprintf(stderr, "Error: Cannot set Server timezone\n");
				}
				*/
				tzset();
				src_offset = timezone;
				offset = src_offset - tgt_offset;
				srvtime = time(NULL);
				if((tzenv = realloc(tzenv, strlen(tgt_tz) + 4)) == NULL) {
					perror("realloc");
					return -1;
				}
				sprintf(tzenv, "TZ=%s", tgt_tz);
				if(putenv(tzenv) != 0) {
					fprintf(stderr, "Error: Cannot set Server timezone\n");
					return -1;
				}
				/*
				if(setenv("TZ", tgt_tz, 1) != 0) {
					fprintf(stderr, "Error: Cannot set Server timezone\n");
				}
				*/
				free(src_tz);
				free(tzenv);

				if(timeInSeconds + offset > srvtime) {
					fprintf(stderr, "Error: To extact value is greater than current time\n");
					return -1;
				}
			}
			cftime(dateTime, "%Y-%m-%d %T", &timeInSeconds);
			/*
			localtime_r(&timeInSeconds, &myTm);
			strftime(dateTime, sizeof(dateTime) - 1, "%Y-%m-%d %T", &myTm); */

			Date[YEAR].value	= atoi(dateTime);
			Date[Year].value	= atoi(dateTime + 2);
			Date[MONTH].value	= atoi(dateTime + 5);
			Date[DAY].value	= atoi(dateTime + 8);
			Date[HOUR].value	= atoi(dateTime + 11);
			Date[MINUTE].value	= atoi(dateTime + 14);
			Date[SECOND].value	= atoi(dateTime + 17);
	}

	/* construct new date in input format by overwriting old values with new values based on original index */
	/* location of format strings (YYYY,MM,...) */

	
	/*
	for (i = 0; i < DATE_FORMAT_TYPE_MAX; i++)
		printf("%d\n", Date[i].index);
	*/

	for (i = 0; i < DATE_FORMAT_TYPE_MAX; i++)
		if (Date[i].index != -1) {
			cmpntLen = strlen(dateFmt[i]);

			if (sprintf(strDate,"%0*i",cmpntLen, Date[i].value) < cmpntLen) {
				fprintf(stderr, "Error: Failed converting new %s to string.\n",dateFmt[i]);
				return -1;
			}
			memcpy(dateIn + Date[i].index, strDate, cmpntLen);
		}

	/* output new date in input format */
	puts(dateIn);

	return 0;
}
