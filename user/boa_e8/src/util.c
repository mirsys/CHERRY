/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Some changes Copyright (C) 1996,97 Larry Doolittle <ldoolitt@jlab.org>
 *  Some changes Copyright (C) 1996 Charles F. Randall <crandall@goldsys.com>
 *  Some changes Copyright (C) 1996,97 Jon Nelson <nels0988@tc.umn.edu>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* boa: util.c */

#include "asp_page.h"
#include "boa.h"
#include <ctype.h>
#include "syslog.h"
#ifdef SERVER_SSL
#ifdef USES_MATRIX_SSL
#include <sslSocket.h>
#else /*!USES_MATRIX_SSL*/
#include <openssl/ssl.h>
#endif /*USES_MATRIX_SSL*/
#endif

#define HEX_TO_DECIMAL(char1, char2)	\
  (((char1 >= 'A') ? (((char1 & 0xdf) - 'A') + 10) : (char1 - '0')) * 16) + \
  (((char2 >= 'A') ? (((char2 & 0xdf) - 'A') + 10) : (char2 - '0')))

#define INT_TO_HEX(x) \
  ((((x)-10)>=0)?('A'+((x)-10)):('0'+(x)))

static time_t cur_time;
const char month_tab[48] = "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec ";
const char day_tab[] = "Sun,Mon,Tue,Wed,Thu,Fri,Sat,";

#include "escape.h"


/*
 * Name: clean_pathname
 * 
 * Description: Replaces unsafe/incorrect instances of:
 *  //[...] with /
 *  /./ with /
 *  /../ with / (technically not what we want, but browsers should deal 
 *   with this, not servers)
 *
 * Stops parsing at '?'
 */

void clean_pathname(char *pathname)
{
	char *cleanpath, c;
	int cgiarg = 0;

	cleanpath = pathname;
	while ((c = *pathname++)) {
		if (c == '/' && !cgiarg) {
			while (1) {
				if (*pathname == '/')
					pathname++;
				else if (*pathname == '.' && *(pathname + 1) == '/')
					pathname += 2;
				else if (*pathname == '.' && *(pathname + 1) == '.' &&
						 *(pathname + 2) == '/') {
					pathname += 3;
				} else
					break;
			}
			*cleanpath++ = '/';
		} else {
			*cleanpath++ = c;
			cgiarg |= (c == '?');
		}
	}

	*cleanpath = '\0';
}

/*
 * Name: strmatch
 *
 * Descripton: This function implements simple regexp. '?' is ANY character,
 *	'*' is zero or more characters. Returns 1 when strings matches.
 */
int strmatch(char *str,char *s2)
{
 while (*s2)
 {
  if (*s2=='*' )
  {
   if (s2[1]==0)
    return 1;
   /* Removes two '**' ... */
   while (s2[1]=='*')  s2++;
   while (*str)
    {
     if (strmatch(str,s2+1))
       return 1;
     str++;
    }
   /* Test if matches "" string */
   if (strmatch(str,s2+1))
     return 1;
   return 0;
  }

//  if (*str) return 1;

  if (*s2!='?')
  {
   if (*str!=*s2)
    return 0;
  } else
    if(!*str)
      return 0;

  str++;
  s2++;
 }
 if (*str)
  return 0;
 return 1;
}


/*
 * Name: modified_since
 * Description: Decides whether a file's mtime is newer than the 
 * If-Modified-Since header of a request.
 * 
 * RETURN VALUES:
 *  0: File has not been modified since specified time.
 *  1: File has been.
 */

int modified_since(time_t * mtime, char *if_modified_since)
{
	struct tm *file_gmt;
	char *ims_info;
	char monthname[MAX_HEADER_LENGTH + 1];
	int day, month, year, hour, minute, second;
	int comp;

	ims_info = if_modified_since;
	
	/* the pre-space in the third scanf skips whitespace for the string */
	//modified by ramen 
	if (sscanf(ims_info, "%*s %d %3s %d %d:%d:%d GMT",	/* RFC 1123 */
		&day, monthname, &year, &hour, &minute, &second) == 6);
	else if (sscanf(ims_info, "%d-%3s-%d %d:%d:%d GMT",  /* RFC 1036 */
		 &day, monthname, &year, &hour, &minute, &second) == 6)
		year += 1900;
	else if (sscanf(ims_info, " %3s %d %d:%d:%d %d",  /* asctime() format */
		monthname, &day, &hour, &minute, &second, &year) == 6);
	else 
		return -1;				/* error */
	
	file_gmt = gmtime(mtime);
	month = month2int(monthname);	
	/* Go through from years to seconds -- if they are ever unequal,
	   we know which one is newer and can return */

#ifdef EMBED
	if (1900 + file_gmt->tm_year != year)
		return 1;
	if (file_gmt->tm_mon != month)
		return 1;
	if (file_gmt->tm_mday != day)
		return 1;
	if (file_gmt->tm_hour != hour)
		return 1;
	if (file_gmt->tm_min != minute)
		return 1;
	if (file_gmt->tm_sec != second)
		return 1;
#else
	if ((comp = 1900 + file_gmt->tm_year - year))
		return (comp > 0);
	if ((comp = file_gmt->tm_mon - month))
		return (comp > 0);
	if ((comp = file_gmt->tm_mday - day))
		return (comp > 0);
	if ((comp = file_gmt->tm_hour - hour))
		return (comp > 0);
	if ((comp = file_gmt->tm_min - minute))
		return (comp > 0);
	if ((comp = file_gmt->tm_sec - second))
		return (comp > 0);
#endif

	return 0;					/* this person must really be into the latest/greatest */
}

/*
 * Name: month2int
 * 
 * Description: Turns a three letter month into a 0-11 int
 * 
 * Note: This function is from wn-v1.07 -- it's clever and fast
 */

int month2int(char *monthname)
{
	switch (*monthname) {
	case 'A':
		return (*++monthname == 'p' ? 3 : 7);
	case 'D':
		return (11);
	case 'F':
		return (1);
	case 'J':
		if (*++monthname == 'a')
			return (0);
		return (*++monthname == 'n' ? 5 : 6);
	case 'M':
		return (*(monthname + 2) == 'r' ? 2 : 4);
	case 'N':
		return (10);
	case 'O':
		return (9);
	case 'S':
		return (8);
	default:
		return (-1);
	}
}


/*
 * Name: to_upper
 * 
 * Description: Turns a string into all upper case (for HTTP_ header forming)
 * AND changes - into _ 
 */

char *to_upper(char *str)
{
	char *start = str;

	while (*str) {
		if (*str == '-')
			*str = '_';
		else
			*str = toupper(*str);

		str++;
	}

	return start;
}

/*
 * Name: unescape_uri
 *
 * Description: Decodes a uri, changing %xx encodings with the actual 
 * character.  The query_string should already be gone.
 * 
 * Return values:
 *  1: success
 *  0: illegal string
 */

int unescape_uri(char *uri)
{
	char c, d;
	char *uri_old;

	uri_old = uri;

	while ((c = *uri_old)) {
		if (c == '%') {
			uri_old++;
			if ((c = *uri_old++) && (d = *uri_old++))
				*uri++ = HEX_TO_DECIMAL(c, d);
			else
				return 0;		/* NULL in chars to be decoded */
		} else {
			*uri++ = c;
			uri_old++;
		}
	}

	*uri = '\0';
	return 1;
}

/*
 * Name: close_unused_fds
 *
 * Description: Closes child's unused file descriptors, in particular
 * all the active transaction channels.  Leaves std* untouched.
 * Call once for request_block, and once for request_ready.
 *
 */

void close_unused_fds(request * head)
{
	request *current;
	for (current = head; current; current = current->next) {
		close(current->fd);
	}
}

/*
 * Name: fixup_server_root
 *
 * Description: Makes sure the server root is valid.
 *
 */

void fixup_server_root()
{
	char *dirbuf;
	int dirbuf_size;

	if (!server_root) {
#ifdef SERVER_ROOT
		server_root = strdup(SERVER_ROOT);
#else
#if 0
		fputs(
				 "boa: don't know where server root is.  Please #define "
				 "SERVER_ROOT in boa.h\n"
				 "and recompile, or use the -c command line option to "
				 "specify it.\n", stderr);
#endif
		exit(1);
#endif
	}
	if (chdir(server_root) == -1) {
#if 0
		fprintf(stderr, "Could not chdir to %s: aborting\n", server_root);
#endif
		syslog(LOG_ERR, "Could not chdir to server root");
		exit(1);
	}
	if (server_root[0] == '/')
		return;

	/* if here, server_root (as specified on the command line) is
	 * a relative path name. CGI programs require SERVER_ROOT
	 * to be absolute.
	 */

	dirbuf_size = MAX_PATH_LENGTH * 2 + 1;
	if ((dirbuf = (char *) malloc(dirbuf_size)) == NULL) {
#if 0
		fprintf(stderr,
				"boa: Cannot malloc %d bytes of memory. Aborting.\n",
				dirbuf_size);
#endif
		syslog(LOG_ERR, "Cannot allocate memory");
		exit(1);
	}
	if (getcwd(dirbuf, dirbuf_size) == NULL) {
#if 0
		if (errno == ERANGE)
			
			perror("boa: getcwd() failed - unable to get working directory. "
				   "Aborting.");
		else if (errno == EACCES)
			perror("boa: getcwd() failed - No read access in current "
				   "directory. Aborting.");
		else
			perror("boa: getcwd() failed - unknown error. Aborting.");
#endif
		syslog(LOG_ERR, "getcwd() failed");
		exit(1);
	}
#if 0
	fprintf(stderr,
			"boa: Warning, the server_root directory specified"
			" on the command line,\n"
			"%s, is relative. CGI programs expect the environment\n"
			"variable SERVER_ROOT to be an absolute path.\n"
		 "Setting SERVER_ROOT to %s to conform.\n", server_root, dirbuf);
#endif
	free(server_root);
	server_root = dirbuf;
}

/* rfc822 (1123) time is exactly 29 characters long
 * "Sun, 06 Nov 1994 08:49:37 GMT"
 */

int req_write_rfc822_time(request *req, time_t s)
{	
	struct tm *t;
	char *p;
	unsigned int a;

#ifdef SUPPORT_ASP
	if (req->buffer_end + 29 > req->max_buffer_size)
#else
	if (req->buffer_end + 29 > BUFFER_SIZE)
#endif
		return 0;	
	
	if (!s) {
		time(&cur_time);
		t = gmtime(&cur_time);
	} else
		t = gmtime(&s);
	
	p = req->buffer + req->buffer_end + 28;
	/* p points to the last char in the buf */
	
	p -= 3; /* p points to where the ' ' will go */
	memcpy(p--, " GMT", 4);
	
	a = t->tm_sec;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = t->tm_min;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = t->tm_hour;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ' ';
	a = 1900 + t->tm_year;
	while (a) {
		*p-- = '0' + a % 10;
		a /= 10;
	}
	/* p points to an unused spot to where the space will go */
	p -= 3;
	/* p points to where the first char of the month will go */
	memcpy(p--, month_tab + 4 * (t->tm_mon), 4);
	*p-- = ' ';
	a = t->tm_mday;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ' ';
	p -= 3;
	memcpy(p, day_tab + t->tm_wday * 4, 4);
	
	req->buffer_end += 29;	
	return 29;
}



/*
 * Name: get_commonlog_time
 *
 * Description: Returns the current time in common log format in a static
 * char buffer.
 *
 * commonlog time is exactly 24 characters long
 * because this is only used in logging, we add "[" before and "] " after
 * making 27 characters
 * "27/Feb/1998:20:20:04 GMT"
 */

char *get_commonlog_time(void)
{
#ifdef BOA_TIME_LOG
	struct tm *t;
	char *p;
	unsigned int a;
	static char buf[27];
	
	time(&cur_time);
	t = gmtime(&cur_time);
	
	p = buf + 27 - 1 - 5;
	
	memcpy(p--, " GMT] ", 6);
	
	a = t->tm_sec;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = t->tm_min;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = t->tm_hour;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = 1900 + t->tm_year;
	while (a) {
		*p-- = '0' + a % 10;
		a /= 10;
	}
	/* p points to an unused spot */
	*p-- = '/';	
	p -= 2;
	memcpy(p--, month_tab + 4 * (t->tm_mon), 3);
	*p-- = '/';
	a = t->tm_mday;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p = '[';
	return p; /* should be same as returning buf */
#else
	return NULL;
#endif
}

char *simple_itoa(int i)
{
	/* 21 digits plus null terminator, good for 64-bit or smaller ints */
	/* for bigger ints, use a bigger buffer! */
	static char local[22];
	char *p = &local[21];
	*p-- = '\0';
	do {
		*p-- = '0' + i % 10;
		i /= 10;
	} while (i > 0);
	return p + 1;
}

/*
 * Name: escape_string
 * 
 * Description: escapes the string inp.  Uses variable buf.  If buf is
 *  NULL when the program starts, it will attempt to dynamically allocate
 *  the space that it needs, otherwise it will assume that the user 
 *  has already allocated enough space for the variable buf, which
 *  could be up to 3 times the size of inp.  If the routine dynamically
 *  allocates the space, the user is responsible for freeing it afterwords
 * Returns: NULL on error, pointer to string otherwise.
 */

char *_escape_string(char *inp, char *buf, int n)
{
	int max;
	char *index;
	unsigned char c;

	max = strlen(inp) * 3;

	if (buf == NULL && max)
		buf = malloc(sizeof(char) * max + 1);
	else{
		max = (max > n) ? ((n<0) ? 0 : n) : max;
	}

	if (buf == NULL)
		return NULL;

	index = buf;
	while ((c = *inp++) && max > 0) {
		if (needs_escape((unsigned int) c)) {
			if((max-3) < 0) break;
			*index++ = '%';
			*index++ = INT_TO_HEX(c >> 4);
			*index++ = INT_TO_HEX(c & 15);
			max -= 3;
		} else {
			*index++ = c;
			max--;
		}
	}
	*index = '\0';
	return buf;
}

/*
 * Name: req_write
 * 
 * Description: Buffers data before sending to client.
 */

int req_write(request *req, char *msg)
		
{
	int msg_len;
	
	msg_len = strlen(msg);
	if (!msg_len)
		return 1;

#ifdef SUPPORT_ASP
	if (req->buffer_end + msg_len > req->max_buffer_size) {
#else
	if (req->buffer_end + msg_len > BUFFER_SIZE) {
#endif
		log_error_time();
#if 0
		fprintf(stderr, "Ran out of Buffer space!\n");
#endif
		syslog(LOG_ERR, "Out of buffer space");
		return 0;
	}
	
	memcpy(req->buffer + req->buffer_end, msg, msg_len);
	req->buffer_end += msg_len;
	return 1;
}

/*
 * Name: flush_req
 * 
 * Description: Sends any backlogged buffer to client.
 */

// Kaohj
int waitsec=0;
time_t lasttime;
extern time_t time_counter;
int req_flush(request * req)
{
	int bytes_to_write;
	
	bytes_to_write = req->buffer_end - req->buffer_start;
	if (bytes_to_write) {
		int bytes_written;

#ifdef USE_NLS
	if (req->cp_table)
	{
		nls_convert(req->buffer + req->buffer_conv,
				req->cp_table,
				req->buffer_end-req->buffer_conv);
		req->buffer_conv = req->buffer_end;
	}
#endif
#ifdef SERVER_SSL
	if(req->ssl == NULL){
#endif /*SERVER_SSL*/
		// Kaohj -- applying timeout for socket write
		time_counter = getSYSInfoTimer();
		if (waitsec == 0) {
			waitsec = 10; // allow write() trying for waitsec
			lasttime = time_counter;
		}
		bytes_written = write(req->fd, 
				req->buffer + req->buffer_start,
				bytes_to_write);
		// Kaohj -- still trying before timeout
		if (bytes_written == -1) {
			waitsec -= (time_counter - lasttime);
			lasttime = time_counter;
			if (waitsec == 0) {
				//printf("time is out\n");
				req->buffer_start = req->buffer_end = 0;
				return 0;
			}
		}
		else
			waitsec = 0;
#ifdef SERVER_SSL
	}else{
#ifdef USES_MATRIX_SSL
		int mtrx_status;
		if (bytes_to_write  >SSL_MAX_PLAINTEXT_LEN)
			bytes_to_write = SSL_MAX_PLAINTEXT_LEN;
		bytes_written = sslWrite(req->ssl, req->buffer + req->buffer_start, bytes_to_write,&mtrx_status);
#else /* ! USES_MATRIX_SSL*/
		bytes_written = SSL_write(req->ssl, req->buffer + req->buffer_start, bytes_to_write);
#endif /* USES_MATRIX_SSL*/
	}
#endif /*SERVER_SSL*/
		
		if (bytes_written == -1) {
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				return -1;			/* request blocked at the pipe level, but keep going */
			else {
				req->buffer_start = req->buffer_end = 0;
#ifdef USE_NLS
				req->buffer_conv = 0;
#endif
#if 0
				if (errno != EPIPE)
					perror("buffer flush");	/* OK to disable if your logs get too big */
#endif
				return 0;
			}
		}
		req->buffer_start += bytes_written;
    }
	if (req->buffer_start == req->buffer_end)
	{
		req->buffer_start = req->buffer_end = 0;
#ifdef USE_NLS
		req->buffer_conv = 0;
#endif
	}
	return 1; /* successful */
}

//#ifdef CHECK_IP_MAC /*This stuff is hacked into place for Dan!!*/
#if defined(CHECK_IP_MAC) || defined(WEB_REDIRECT_BY_MAC)

#define ARP "/proc/net/arp"

int get_mac_from_IP(char *MAC, char *remote_IP){
	FILE *fd;
	char tmp[81];
	int len = 81;
	int i;
	char *str;
	int amtRead;
	if((fd = fopen(ARP, "r")) == 0){/*should be able to open the ARP cache*/
		return 0;
	}
	while(fgets(tmp, len, fd)){
		/*MN - should do some more checks!!*/
		str = strtok(tmp, " \t");
		if(strcmp(str, remote_IP) == 0){ /*found IP address*/
			for(i=0;i<3;i++){  /*Move along until the HW address*/
				str = strtok(NULL, " \t");
			}
			strcpy(MAC, str);
			/*MATT2 you could obtain the ethernet device in here if you wanted*/
			return 1;
		}
	}
	return 0;
}

int do_mac_crap(char *IP, char *MAC)
{

}

#endif /*CHECK_IP_MAC*/

#ifdef ONE_USER_BY_SESSIONID
int get_sessionid_from_cookie(request *req, char *id)
{
	char *sess;

	if(req == NULL || req->cookie == NULL || id == NULL)
		return 0;

	//fprintf(stderr, "<%s:%d> req->cookie=%s\n", __func__, __LINE__, req->cookie);

	sess = strstr(req->cookie, "sessionid=");
	if(sess && sscanf(sess, "sessionid=%[^;]", id) > 0 && id[0])
	{
		//fprintf(stderr, "<%s:%d> got sessionid: %s\n", __func__, __LINE__, id);
		return 1;
	}
	else
		return 0;
}
#endif

