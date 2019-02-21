/* vi:set tabstop=2 cindent shiftwidth=2: */
/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Some changes Copyright (C) 1996 Charles F. Randall <crandall@goldsys.com>
 *  Some changes Copyright (C) 1996 Larry Doolittle <ldoolitt@jlab.org>
 *  Some changes Copyright (C) 1996,97,98 Jon Nelson <nels0988@tc.umn.edu>
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

/* boa: boa.c */
// run BOA as console (menu-driven CLI)
//#define CONSOLE_BOA

#include "./LINUX/form_src/multilang.h"
#include "./LINUX/form_src/multilang_set.h"

#include "asp_page.h"
#include "boa.h"
#include <grp.h>
#include "syslog.h"
#include <sys/param.h>
#include <signal.h>
#ifdef SERVER_SSL
#ifdef USES_MATRIX_SSL
#include <sslSocket.h>
#include <pthread.h>
#else
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <pthread.h>
#endif /*USES_MATRIX_SSL*/
#endif /*SERVER_SSL*/

// Added by Mason Yu(2 level)
#include <stdio.h>
#ifdef USE_LIBMD5
#include <libmd5wrapper.h>
#else
#include "md5.h"
#endif //USE_LIBMD5
#include "./LINUX/mib.h"
#include "./LINUX/utility.h"
// Added by ql
#include "./LINUX/mib_reserve.h"
#ifdef CONFIG_MIDDLEWARE
#include <rtk/midwaredefs.h>
#endif

#include "msgq.h"
#include "./LINUX/debug.h"
#include <crypt.h>

#ifdef PARENTAL_CTRL
#include <time.h>
#endif

#ifdef CONFIG_HTTP_DOWNLOAD_TEST
#include "./LINUX/httptest.h"
#endif

#define        LOG_PID         0x01
int server_s;					/* boa socket */

static int this_pid;

#ifdef SERVER_SSL
#define SSL_KEYF "/etc/ssl_key.pem"
#define SSL_CERTF "/etc/ssl_cert.pem"
	int server_ssl;				/*ssl socket */
	int do_ssl = 1;					/*We want to actually perform all of the ssl stuff.*/
	int do_sock = 1;				/*We may not want to actually connect to normal sockets*/
#ifdef USES_MATRIX_SSL
	sslConn_t		*mtrx_cp = NULL;
	sslKeys_t			*mtrx_keys;
#else
	SSL_CTX *ctx;				/*SSL context information*/
	SSL_METHOD *meth;			/*SSL method information*/
#endif
	int ssl_server_port = 443;		/*The port that the server should listen on*/
	/*Note that per socket ssl information is stored in */
#ifdef IPV6
	struct sockaddr_in6 server_sockaddr;		/* boa ssl socket address */
#else
	struct sockaddr_in ssl_server_sockaddr;		/* boa ssl socket address */
#endif/*IPV6*/
extern int InitSSLStuff(void);
extern void get_ssl_request(void);
#endif /*SERVER_SSL*/

int backlog = SO_MAXCONN;
#ifdef IPV6
struct sockaddr_in6 server_sockaddr;		/* boa socket address */
#else
struct sockaddr_in server_sockaddr;		/* boa socket address */
#endif

struct timeval req_timeout;		/* timeval for select */

extern char *optarg;			/* getopt */

fd_set block_read_fdset;
fd_set block_write_fdset;

int sighup_flag = 0;			/* 1 => signal has happened, needs attention */
int sigchld_flag = 0;			/* 1 => signal has happened, needs attention */
int lame_duck_mode = 0;

time_t time_counter = 0;

static int last_tm_mday = 0, tm_mday = 0;

int sock_opt = 1;
int do_fork = 1;

static int max_fd = 0;

// Added by Mason Yu
char suName[MAX_NAME_LEN];
char usName[MAX_NAME_LEN];
char e8bdName[MAX_NAME_LEN];

//add by xl_yue
#ifdef USE_LOGINWEB_OF_SERVER

#ifdef ONE_USER_LIMITED
struct account_status suStatus;
struct account_status usStatus;
#endif

struct user_info * user_login_list = NULL;

#ifdef LOGIN_ERR_TIMES_LIMITED
struct errlogin_entry * errlogin_list = NULL;
#endif

#endif

#ifdef CU_APP_SCHEDULE_LOG
unsigned char upload_log_flag = 0;
#endif

//add by xl_yue
#ifdef USE_LOGINWEB_OF_SERVER
#ifdef ONE_USER_BY_SESSIONID
struct user_info * search_login_list(request * req)
{
	struct user_info * pUser_info = NULL;
	char sessid[32] = {0};

	if(req == NULL || req->cookie == NULL)
		return NULL;

	if(get_sessionid_from_cookie(req, sessid) == 0)
		return NULL;

	for(pUser_info = user_login_list;pUser_info;pUser_info = pUser_info->next)
	{
		if(!strcmp(sessid, pUser_info->paccount->sessionid))
			break;
	}
	return pUser_info;
}
#else
struct user_info * search_login_list(request * req)
{
	struct user_info * pUser_info;
	for(pUser_info = user_login_list;pUser_info;pUser_info = pUser_info->next){
		if(!strcmp(req->remote_ip_addr,pUser_info->remote_ip_addr))
			break;
	}
	return pUser_info;
}
#endif

static void ulist_free_login_entry(struct user_info * bUser_info,struct user_info * pUser_info)
{
	if(pUser_info == user_login_list)
		user_login_list = pUser_info->next;
	else
		bUser_info->next = pUser_info->next;

	if(pUser_info->directory)
		free(pUser_info->directory);

#ifdef ONE_USER_LIMITED
	if(pUser_info->paccount)
		pUser_info->paccount->busy = 0;
#endif

/* BEGIN: Added by piyajee_chen, 2016/9/9   PN:support logout by system self when timeout. */
    UNTIMEOUT(0, 0, pUser_info->autologout);
/* END:   Added by piyajee_chen, 2016/9/9   PN:support logout by system self when timeout. */

	free(pUser_info);
}
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
void free_loginList()
{
	struct user_info * pUser_info;
	struct user_info * bUser_info = NULL;

	for(pUser_info = user_login_list;pUser_info;bUser_info = pUser_info,pUser_info = pUser_info->next){
		ulist_free_login_entry(bUser_info,pUser_info);
	}

	return;
}
#endif

#ifdef ONE_USER_BY_SESSIONID
int free_from_login_list(request * req)
{
	struct user_info * pUser_info;
	struct user_info * bUser_info = NULL;
	char sessid[32] = {0};

	if(req == NULL || req->cookie == NULL)
		return 0;

	if(get_sessionid_from_cookie(req, sessid) == 0)
		return 0;

	for(pUser_info = user_login_list;pUser_info;bUser_info = pUser_info,pUser_info = pUser_info->next){
		if(!strcmp(pUser_info->paccount->sessionid, sessid))
			break;
	}

	if(!pUser_info)
		return 0;

	ulist_free_login_entry(bUser_info,pUser_info);

	return 1;
}
#else
int free_from_login_list(request * req)
{
	struct user_info * pUser_info;
	struct user_info * bUser_info = NULL;

	for(pUser_info = user_login_list;pUser_info;bUser_info = pUser_info,pUser_info = pUser_info->next){
		if(!strcmp(pUser_info->remote_ip_addr, req->remote_ip_addr))
			break;
	}

	if(!pUser_info)
		return 0;

	ulist_free_login_entry(bUser_info,pUser_info);

	return 1;
}
#endif

#ifdef ONE_USER_LIMITED
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

int free_from_login_list_by_sessionid(char *sess)
{
	struct user_info * pUser_info;
	struct user_info * bUser_info = NULL;

	for(pUser_info = user_login_list;pUser_info;bUser_info = pUser_info,pUser_info = pUser_info->next){
		if(!strcmp(pUser_info->paccount->sessionid, sess))
			break;
	}

	if(!pUser_info)
		return 0;

	ulist_free_login_entry(bUser_info,pUser_info);

	return 1;
}
#else
int free_from_login_list_by_ip_addr(char * remote_ip_addr)
{
	struct user_info * pUser_info;
	struct user_info * bUser_info = NULL;

	for(pUser_info = user_login_list;pUser_info;bUser_info = pUser_info,pUser_info = pUser_info->next){
		if(!strcmp(pUser_info->remote_ip_addr, remote_ip_addr))
			break;
	}

	if(!pUser_info)
		return 0;

	ulist_free_login_entry(bUser_info,pUser_info);

	return 1;
}
#endif
#endif

#ifdef LOGIN_ERR_TIMES_LIMITED
//added by xl_yue
struct errlogin_entry * search_errlog_list(request * req)
{
	struct errlogin_entry * perrlog;

	for(perrlog = errlogin_list;perrlog;perrlog = perrlog->next){
		if(!strcmp(perrlog->remote_ip_addr, req->remote_ip_addr))
			break;
	}

	return perrlog;
}
void ulist_free_errlog_entry(struct errlogin_entry * berrlog,struct errlogin_entry * perrlog)
{
	//unlist and free
	if(perrlog == errlogin_list)
		errlogin_list = perrlog->next;
	else
		berrlog->next = perrlog->next;

	free(perrlog);
}

//added by xl_yue
int free_from_errlog_list(request * req)
{
	struct errlogin_entry * perrlog;
	struct errlogin_entry * berrlog = NULL;

	for(perrlog = errlogin_list;perrlog;berrlog = perrlog,perrlog = perrlog->next){
		if(!strcmp(perrlog->remote_ip_addr, req->remote_ip_addr))
			break;
	}

	if(!perrlog)
		return 0;

	ulist_free_errlog_entry(berrlog,perrlog);
	return 1;
}
#endif

#endif


#ifdef EMBED
static void log_pid()
{
	FILE *f;
	pid_t pid;
	char *pidfile = BOA_RUNFILE;

	pid = getpid();
	this_pid = pid;
	if((f = fopen(pidfile, "w")) == NULL)
		return;
	fprintf(f, "%d\n", pid);
	fclose(f);
}
#endif



//jim luo
//#if 0
#if 0 //def CONFIG_CTC_E8_CLIENT_LIMIT
// int maxPCClients=1;//MAXPCCLIENTS;
// int maxCameraClients=1;//MAXCAMERACLIENTS;
#define CLIENTSMONITOR  "/proc/ClientsMonitor"
 struct itimerval read_proc_interval;
//struct CTC_Clients accepted_PC_Clients[MAXPCCLIENTS];
//struct CTC_Clients accepted_Camera_Clients[MAXCAMERACLIENTS];
static void read_proc_handler(int dummy)
{
	FILE *fp;
	int totallen;
	int parsedlen=0;
	int i;
	char buffer[1024];
	char *cur_ptr=buffer;
	char *str_end=buffer;
	int str_found;
	char cmd_str[256];
	//printf("read proc \n");
	fp=fopen(CLIENTSMONITOR, "r");
	if(fp==NULL)
	{
		printf("file %s open failed\n", CLIENTSMONITOR);
		return;
	}
	memset(buffer, 0, sizeof(buffer));
	totallen=fread(buffer, 1, 1024, fp);
	if(totallen!=0)
		printf("read size=%d, buffer=%s\n", totallen, buffer);
	if(totallen < 0)
		goto err;

	while(1)
	{
		str_found=0;
		memset(cmd_str, 0, sizeof(cmd_str));
		for(i=0; i<totallen-parsedlen; i++)
		{
			if(*str_end=='\n')
			{
				str_found=1;
				break;
			}
			str_end++;
		}
		if(str_found)
		{
			memcpy(cmd_str, cur_ptr, (unsigned)str_end-(unsigned)cur_ptr);
			printf("cmd parsed out: %s", cmd_str);
			system(cmd_str);//execute it....
			parsedlen+=i;
			str_end+=1;
			cur_ptr=str_end;

		}else
		{
			break;
		}
	}
err:
	fclose(fp);
	return;
}
#endif


// Kaohj
// enable menu-driven CLI pthread
//#define MENU_CLI

#ifdef MENU_CLI
/******************************
 *	Start the CLI pthread
 */

void climenu (void);

void startCli()
{
	pthread_t ptCliId;
	//pthread_attr_t attCliAttributes;
	//struct sched_param priorityParams;

	//if (pthread_attr_init(&attCliAttributes) != 0)
	//	return;
	//if (pthread_create( &ptCliId, &attCliAttributes, &climenu, 0 )!=0)
	//	return;
	if (pthread_create( &ptCliId, 0, &climenu, 0 )!=0)
		return;
}
#endif

// Added by Mason Yu
void calPasswdMD5(char *pass, char *passMD5)
{
	char temps[0x100],*pwd;
	struct MD5Context mc;
 	unsigned char final[16];
	char encoded_passwd[0x40];
	//char *pass="user";

  	/* Encode password ('pass') using one-way function and then use base64
	 encoding. */

	MD5Init(&mc);
	{

	//printf("calPasswdMD5: pass=%s\n", pass);
	MD5Update(&mc, pass, strlen(pass));
	}
	MD5Final(final, &mc);
	strcpy(encoded_passwd,"$1$");
	base64_encode(final, encoded_passwd+3, 16);
        //printf("encoded_passwd=%s for %s\n",encoded_passwd, pass);

        strcpy(passMD5, encoded_passwd);

}

#ifdef EMBED
#ifndef MULTI_USER_PRIV
// Added by Mason Yu(2 level)
void writePasswdFile()
{
	FILE *fp, *fp2, *fp3;
	char suPasswd[MAX_NAME_LEN], usPasswd[MAX_NAME_LEN], e8bdPasswd[MAX_NAME_LEN];
	char suPasswdMD5[50], usPasswdMD5[50], e8bdPasswdMD5[50];
#ifdef ACCOUNT_CONFIG
	MIB_CE_ACCOUNT_CONFIG_T entry;
	unsigned int totalEntry;
	int i;

	totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL); /* get chain record size */
#endif
	unsigned char province_sichuan_e8c_backdoor_enable = 0;
	mib_get(PROVINCE_SICHUAN_E8C_BACKDOOR_ENABLE, (void *)&province_sichuan_e8c_backdoor_enable);

	// Added by Mason Yu for write superUser into Current Setting
	if ( !mib_get(MIB_SUSER_NAME, (void *)suName) ) {
		printf("ERROR: Get superuser name from MIB database failed.\n");
		return;
	}

	if ( !mib_get(MIB_USER_NAME, (void *)usName) ) {
		printf("ERROR: Get user name from MIB database failed.\n");
		return;
	}

	if ( !mib_get(MIB_SUSER_PASSWORD, (void *)suPasswd) ) {
		printf("ERROR: Set superuser password to MIB database failed.\n");
		return;
	}
	
	if ( !mib_get(MIB_USER_PASSWORD, (void *)usPasswd) ) {
		printf("ERROR: Set superuser password to MIB database failed.\n");
		return;
	}

	calPasswdMD5(suPasswd, suPasswdMD5);
	calPasswdMD5(usPasswd, usPasswdMD5);
	
#if !defined(CONFIG_CMCC) && !defined(CONFIG_CU)
	mib_get(MIB_HW_E8BDUSER_NAME, (void *)e8bdName);
	if(e8bdName[0] == '\0')
	{
		if ( !mib_get(MIB_E8BDUSER_NAME, (void *)e8bdName) ) {
			printf("ERROR: Get rtuser name from MIB database failed.\n");
			return;
		}

		if ( !mib_get(MIB_E8BDUSER_PASSWORD, (void *)e8bdPasswd) ) {
			printf("ERROR: Set superuser password to MIB database failed.\n");
			return;
		}
	}
	else
		mib_get(MIB_HW_E8BDUSER_PASSWORD, (void *)e8bdPasswd);

	calPasswdMD5(e8bdPasswd, e8bdPasswdMD5);
#endif
	
	//calPasswdMD5("realtek", PasswdMD5);
	//printf("PasswdMD5=%s\n", PasswdMD5);


	// boa.passwd  "/var/snmpComStr.conf"
	if ((fp = fopen("/var/boaUser.passwd", "w")) == NULL)
	{
		printf("***** Open file /var/boaUser.passwd failed !\n");
		return;
	}

	if ((fp2 = fopen("/var/boaSuper.passwd", "w")) == NULL)
	{
		printf("***** Open file /var/boaSuper.passwd failed !\n");
		fclose(fp);
		return;
	}

	if(province_sichuan_e8c_backdoor_enable)
	{
	if ((fp3 = fopen("/var/boaE8BD.passwd", "w")) == NULL)
	{
		printf("***** Open file /var/boaE8BD.passwd failed !\n");
		fclose(fp);
		fclose(fp2);
		return;
	}
	}

	fprintf(fp, "%s:%s\n", usName, usPasswdMD5);
	fprintf(fp, "%s:%s\n", suName, suPasswdMD5);
	fprintf(fp2, "%s:%s\n", suName, suPasswdMD5);
#if !defined(CONFIG_CMCC) && !defined(CONFIG_CU)
	if(province_sichuan_e8c_backdoor_enable)
	fprintf(fp, "%s:%s\n", e8bdName, e8bdPasswdMD5);
	if(province_sichuan_e8c_backdoor_enable)
	fprintf(fp2, "%s:%s\n", e8bdName, e8bdPasswdMD5);
	if(province_sichuan_e8c_backdoor_enable)
	fprintf(fp3, "%s:%s\n", e8bdName, e8bdPasswdMD5);
#endif
#ifdef ACCOUNT_CONFIG
	for (i=0; i<totalEntry; i++) {
		if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&entry)) {
			printf("ERROR: Get account configuration information from MIB database failed.\n");
			fclose(fp);
			fclose(fp2);
			if(province_sichuan_e8c_backdoor_enable)
			fclose(fp3);
			return;
		}
		calPasswdMD5(entry.userPassword, usPasswdMD5);

		switch(entry.privilege) {
			case PRIV_USER:
				fprintf(fp, "%s:%s\n", entry.userName, usPasswdMD5);
				break;

			case PRIV_ENG:
				break;

			case PRIV_ROOT:
				fprintf(fp, "%s:%s\n", entry.userName, usPasswdMD5);
				fprintf(fp2, "%s:%s\n", entry.userName, usPasswdMD5);
				break;

			default:
				printf("Wrong privilege!\n");
				break;
		}
	}
#endif

	fclose(fp);
	fclose(fp2);
	if(province_sichuan_e8c_backdoor_enable)
	fclose(fp3);

	#ifdef SUPPORT_AUTH_DIGEST
	if(!(fp = fopen("/var/DigestUser.passwd", "w")))
	{
		printf("***** Open file /var/DigestUser.passwd failed !\n");
		return;
	}
	fprintf(fp, "%s\n%s\n%s\n%s\n", usName, usPasswd, suName, suPasswd);
	fclose(fp);

	if(!(fp = fopen("/var/DigestSuper.passwd", "w")))
	{
		printf("***** Open file /var/DigestSuper.passwd failed!\n");
		return;
	}
	fprintf(fp, "%s\n%s\n", suName, suPasswd);
	fclose(fp);
	#endif


	#if 0 //def _CWMP_MIB_
	{
		char PasswdMD5[256+1];
		char *tmpStr = PasswdMD5;
		if ((fp = fopen("/var/digest.passwd", "w")) == NULL)
		{
			printf("***** Open file /var/digest.passwd failed !\n");
			return;
		}
		mib_get(CWMP_CONREQ_USERNAME, (void *)tmpStr);
		fprintf(fp, "%s\n", tmpStr);
		mib_get(CWMP_CONREQ_PASSWORD, (void *)tmpStr);
		fprintf(fp, "%s\n", tmpStr);
		fclose(fp);
        }
	#endif
}
#else
void writePasswdFile()
{
	FILE *fp;
#ifdef SUPPORT_AUTH_DIGEST
	FILE *fpd;
#endif
	char suPasswd[MAX_NAME_LEN], usPasswd[MAX_NAME_LEN];
	char suPasswdMD5[50], usPasswdMD5[50];
	MIB_CE_ACCOUNT_CONFIG_T entry;
	unsigned int totalEntry;
	int i;

	if (!mib_get(MIB_SUSER_NAME, (void *)suName)) {
		printf("ERROR: Get superuser name from MIB database failed.\n");
		return;
	}
	if (!mib_get(MIB_USER_NAME, (void *)usName)) {
		printf("ERROR: Get user name from MIB database failed.\n");
		return;
	}
	if (!mib_get(MIB_SUSER_PASSWORD, (void *)suPasswd)) {
		printf("ERROR: Set superuser password to MIB database failed.\n");
		return;
	}
	if (!mib_get(MIB_USER_PASSWORD, (void *)usPasswd)) {
		printf("ERROR: Set user password to MIB database failed.\n");
		return;
	}
	calPasswdMD5(suPasswd, suPasswdMD5);
	calPasswdMD5(usPasswd, usPasswdMD5);
#ifdef SUPPORT_AUTH_DIGEST
	if(!(fpd = fopen("/var/DigestUser.passwd", "w")))
	{
		printf("***** Open file /var/DigestUser.passwd failed !\n");
		return;
	}
	fprintf(fpd, "%s\n%s\n%s\n%s\n", usName, usPasswd, suName, suPasswd);
#endif
	if ((fp = fopen("/var/boaUser.passwd", "w")) == NULL) {
		printf("***** Open file /var/boaUser.passwd failed !\n");
#ifdef SUPPORT_AUTH_DIGEST
		fclose(fpd);
#endif
		return;
	}
	fprintf(fp, "%s:%s\n", usName, usPasswdMD5);
	fprintf(fp, "%s:%s\n", suName, suPasswdMD5);

	totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL); /* get chain record size */
	for (i=0; i<totalEntry; i++) {
		if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&entry)) {
			printf("ERROR: Get account configuration information from MIB database failed.\n");
			fclose(fp);
#ifdef SUPPORT_AUTH_DIGEST
			fclose(fpd);
#endif
			return;
		}
		calPasswdMD5(entry.userPassword, usPasswdMD5);
#ifdef SUPPORT_AUTH_DIGEST
		fprintf(fpd, "%s\n%s\n", entry.userName, entry.userPassword);
#endif
		fprintf(fp, "%s:%s\n", entry.userName, usPasswdMD5);
	}
#ifdef SUPPORT_AUTH_DIGEST
	fclose(fpd);
#endif
	fclose(fp);

	#if 0
	//#ifdef _CWMP_MIB_
	{
		char PasswdMD5[256];
		char *tmpStr = PasswdMD5;
		if ((fp = fopen("/var/digest.passwd", "w")) == NULL)
		{
			printf("***** Open file /var/digest.passwd failed !\n");
			return;
		}
		mib_get(CWMP_CONREQ_USERNAME, (void *)tmpStr);
		fprintf(fp, "%s\n", tmpStr);
		mib_get(CWMP_CONREQ_PASSWORD, (void *)tmpStr);
		fprintf(fp, "%s\n", tmpStr);
		fclose(fp);
        }
        #endif
}
#endif

void updateUserAccount(void)
{
	FILE *fp;
	char userName[MAX_NAME_LEN], userPass[MAX_NAME_LEN];
	char *xpass;
	unsigned char province_sichuan_e8c_backdoor_enable = 0;
#ifdef ACCOUNT_CONFIG
	MIB_CE_ACCOUNT_CONFIG_T entry;
	unsigned int totalEntry;
#endif
	int i;

	fp = fopen("/var/passwd", "w+");
#ifdef ACCOUNT_CONFIG
	totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL); /* get chain record size */
	for (i=0; i<totalEntry; i++) {
		if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&entry)) {
			printf("ERROR: Get account configuration information from MIB database failed.\n");
			fclose(fp);
			return;
		}
		strcpy(userName, entry.userName);
		strcpy(userPass, entry.userPassword);
		xpass = crypt(userPass, "$1$");
		if (xpass) {
			if (entry.privilege == (unsigned char)PRIV_ROOT)
				fprintf(fp, "%s:%s:0:0::%s:%s\n", userName, xpass, PW_HOME_DIR, PW_CMD_SHELL);
			else
				fprintf(fp, "%s:%s:1:0::%s:%s\n", userName, xpass, PW_HOME_DIR, PW_CMD_SHELL);
		}
	}
#endif
	mib_get( MIB_SUSER_NAME, (void *)userName );
	mib_get( MIB_SUSER_PASSWORD, (void *)userPass );
	xpass = crypt(userPass, "$1$");
	if (xpass)
		fprintf(fp, "%s:%s:0:0::%s:%s\n", userName, xpass, PW_HOME_DIR, PW_CMD_SHELL);

#if !defined(CONFIG_CMCC) && !defined(CONFIG_CU)
	// Added by Mason Yu for others user
	mib_get( MIB_SUPER_NAME, (void *)userName );
	mib_get( MIB_SUPER_PASSWORD, (void *)userPass );
	xpass = crypt(userPass, "$1$");
	if (xpass)
		fprintf(fp, "%s:%s:0:0::%s:%s\n", userName, xpass, PW_HOME_DIR, PW_CMD_SHELL);

	mib_get(PROVINCE_SICHUAN_E8C_BACKDOOR_ENABLE, (void *)&province_sichuan_e8c_backdoor_enable);
	if(province_sichuan_e8c_backdoor_enable)
	{
		mib_get( MIB_E8BDUSER_NAME, (void *)userName );
		mib_get( MIB_E8BDUSER_PASSWORD, (void *)userPass );
		xpass = crypt(userPass, "$1$");
		if (xpass)
			fprintf(fp, "%s:%s:0:0::%s:%s\n", userName, xpass, PW_HOME_DIR, PW_CMD_SHELL);
	}
#endif
#if 0 // anonymous ftp
	// added for anonymous ftp
	fprintf(fp, "%s:%s:10:10::/tmp:/dev/null\n", "ftp", "x");
#endif
	fprintf(fp, "%s:%s:0:0::/tmp:/dev/null\n", "nobody", "x");

	mib_get( MIB_USER_NAME, (void *)userName );
	if (userName[0]) {
		mib_get( MIB_USER_PASSWORD, (void *)userPass );
		xpass = crypt(userPass, "$1$");
		if (xpass)
			fprintf(fp, "%s:%s:1:0::%s:%s\n", userName, xpass, PW_HOME_DIR, PW_CMD_SHELL);
	}
#ifdef CONFIG_USER_DBUS
	fprintf(fp, "%s:%s:0:0:root:%s:%s\n", "root", "x", PW_HOME_DIR, PW_CMD_SHELL);
#endif
	fclose(fp);

	chmod(PW_HOME_DIR, 0x1fd);	// let owner and group have write access
}
#endif


#ifdef WLAN_WEB_REDIRECT //jiunming,web_redirect
int redir_server_s;
void get_redir_request(void);
int init_redirect_socket(void)
{
	int	ret = -1;
#ifdef IPV6
	struct sockaddr_in6 server_sockaddr;		/* socket address */
#else
	struct sockaddr_in server_sockaddr;		/* socket address */
#endif


#ifdef IPV6
	if ((redir_server_s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1)
#else
	if ((redir_server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
#endif
		die(NO_CREATE_SOCKET);

	/* server socket is nonblocking */
	if (fcntl(redir_server_s, F_SETFL, NOBLOCK) == -1)
		die(NO_FCNTL);

	if ((setsockopt(redir_server_s, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
					sizeof(sock_opt))) == -1)
		die(NO_SETSOCKOPT);

	/* internet socket */
	//maybe, binding to 127.0.0.1 is better.
#ifdef IPV6
	server_sockaddr.sin6_family = AF_INET6;
	memcpy(&server_sockaddr.sin6_addr,&in6addr_any,sizeof(in6addr_any));
	server_sockaddr.sin6_port = htons(8080);
#else
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sockaddr.sin_port = htons(8080);
#endif

	if (bind(redir_server_s, (struct sockaddr *) &server_sockaddr,
			 sizeof(server_sockaddr)) == -1)
		die(NO_BIND);

	//printf("\nCreate a web redirect socket at port 8080.");

	if (listen(redir_server_s, 5) == -1)
		die(NO_LISTEN);

	return redir_server_s;
}
#endif


#ifdef WEB_REDIRECT_BY_MAC
int mac_redir_server_s;
int mac_redir_server_port=WEB_REDIR_BY_MAC_PORT;
void get_mac_redir_request(void);
int init_mac_redirect_socket(void)
{
	int	ret = -1;
#ifdef IPV6
	struct sockaddr_in6 server_sockaddr;		/* socket address */
#else
	struct sockaddr_in server_sockaddr;		/* socket address */
#endif


#ifdef IPV6
	if ((mac_redir_server_s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1)
#else
	if ((mac_redir_server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
#endif
		die(NO_CREATE_SOCKET);

	/* server socket is nonblocking */
	if (fcntl(mac_redir_server_s, F_SETFL, NOBLOCK) == -1)
		die(NO_FCNTL);

	if ((setsockopt(mac_redir_server_s, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
					sizeof(sock_opt))) == -1)
		die(NO_SETSOCKOPT);

	/* internet socket */
	//maybe, binding to 127.0.0.1 is better.
#ifdef IPV6
	server_sockaddr.sin6_family = AF_INET6;
	memcpy(&server_sockaddr.sin6_addr,&in6addr_any,sizeof(in6addr_any));
	server_sockaddr.sin6_port = htons(mac_redir_server_port);
#else
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sockaddr.sin_port = htons(mac_redir_server_port);
#endif

	if (bind(mac_redir_server_s, (struct sockaddr *) &server_sockaddr,
			 sizeof(server_sockaddr)) == -1)
		die(NO_BIND);

	//printf("\nCreate a web redirect socket at port mac_redir_server_port.");

	if (listen(mac_redir_server_s, 5) == -1)
		die(NO_LISTEN);

	return mac_redir_server_s;
}
#endif

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
int captiveportal_server_s;
void get_captiveportal_request(void);
int init_captiveportal_socket(void)
{
	int	ret = -1;
#ifdef IPV6
	struct sockaddr_in6 server_sockaddr;		/* socket address */
#else
	struct sockaddr_in server_sockaddr;		/* socket address */
#endif

#ifdef IPV6
	if ((captiveportal_server_s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1)
#else
	if ((captiveportal_server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
#endif
		die(NO_CREATE_SOCKET);

	/* server socket is nonblocking */
	if (fcntl(captiveportal_server_s, F_SETFL, NOBLOCK) == -1)
		die(NO_FCNTL);

	if ((setsockopt(captiveportal_server_s, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt, sizeof(sock_opt))) == -1)
		die(NO_SETSOCKOPT);

	/* internet socket */
	//maybe, binding to 127.0.0.1 is better.
#ifdef IPV6
	server_sockaddr.sin6_family = AF_INET6;
	memcpy(&server_sockaddr.sin6_addr, &in6addr_any, sizeof(in6addr_any));
	server_sockaddr.sin6_port = htons(CAPTIVEPORTAL_PORT);
#else
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sockaddr.sin_port = htons(CAPTIVEPORTAL_PORT);
#endif

	if (bind(captiveportal_server_s, (struct sockaddr *) &server_sockaddr, sizeof(server_sockaddr)) == -1)
		die(NO_BIND);

	//printf("\nCreate a web redirect socket at port %d\n", CAPTIVEPORTAL_PORT);

	if (listen(captiveportal_server_s, 5) == -1)
		die(NO_LISTEN);

	return captiveportal_server_s;
}
#endif

#if 0
int mib_chain_backup(int id, unsigned char ** ptr)
{
	unsigned int chainSize;
	unsigned int size;
	int index, idx=0;
	unsigned int currentRecord=0;
	MIB_CHAIN_ENTRY_Tp pChain = NULL;

	chainSize = __mib_chain_total(id);
	index = __mib_chain_mib2tbl_id(id);
	if (chainSize > 0)
	{
		size += chainSize * mib_chain_record_table[index].per_record_size;
	}

	*ptr = (unsigned char *)malloc(chainSize);
	pChain = mib_chain_record_table[index].pChainEntry;
	while(pChain != NULL)
	{
		memcpy(*ptr+idx, pChain->pValue, mib_chain_record_table[index].per_record_size);
		pChain = pChain->pNext;
		idx += mib_chain_record_table[index].per_record_size;
		currentRecord++;
	}
	if (currentRecord != chainSize)
	{
		TRACE("(currentRecord!=chainSize) \n");
		return 0;
	}
	return 1;
}
#endif

static void init_global_parm()
{
	FILE	*fp=NULL;
	char buff[256], tmp1[20], tmp2[20];
#ifdef CONFIG_RTL8686
	int size=0;
#endif
	// MAX_UPLOAD_FILESIZE
	// Check MTD block size
	g_max_upload_size = MAX_UPLOAD_FILESIZE;

#ifndef CONFIG_RTK_USE_ONE_UBI_DEVICE
	if (!(fp=fopen("/proc/mtd", "r")))
		printf("/proc/mtd not exists.\n");
	else {
		fgets(buff, sizeof(buff), fp);
		while (fgets(buff, sizeof(buff), fp) != NULL) {
			if (sscanf(buff, "%*s%s%*s%s", tmp1, tmp2) != 2) {
				printf("Unsuported MTD partition format\n");
				break;
			}
#ifdef CONFIG_RTL8686
			if (strcmp(tmp2, "\"linux\"") == 0) {
				size += strtol(tmp1, 0, 16);
				printf("size of linux : %d\n",size);
				g_max_upload_size = size;
			}
			if (strcmp(tmp2, "\"rootfs\"") == 0) {
				size += strtol(tmp1, 0, 16);
				printf("size of linux : %d\n",size);
				g_max_upload_size = size;
				printf("%s%d::size of g_max_upload_size : %d\n",__func__,__LINE__,g_max_upload_size);
				break;
			}
#else
			if (strcmp(tmp2, "\"rootfs\"") == 0) {
				g_max_upload_size = strtol(tmp1, 0, 16);
				break;
			}
#endif
		}
		fclose(fp);
	}
#ifdef CONFIG_YUEME
	g_max_upload_size += (10*1024*1024);
        printf("increase g_max_upload_size 10M : %d\n",g_max_upload_size);
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	g_max_upload_size = 40 * 1024 * 1024;    //for osgi patition
#endif
#else
	system("ubinfo -d0 -Nubi_r0 | grep Size | awk '{print $4}'| grep -o '[0-9]\\+' > /tmp/max_upload_size");

	if (!(fp=fopen("/tmp/max_upload_size", "r")))
		printf("/tmp/max_upload_size not exists.\n");
	else
	{
		fgets(buff, sizeof(buff), fp);
		printf("buff = %s strtol(buff, 0, 10)=%d\n", buff, strtol(buff, 0, 10));
		g_max_upload_size = strtol(buff, 0, 10);
	}
	fclose(fp);
#endif //CONFIG_RTK_USE_ONE_UBI_DEVICE
}

#ifdef SUPPORT_WEB_PUSHUP
#define SEVEN_DAY	(7*24*60*60)

static void web_push_schedule(void)
{
	struct  timeval  tv;
	static  unsigned int startTime=0;
	static  unsigned int webPushSched=1;

	if (!webPushSched)
		return;
	
	if (0 == startTime)
		mib_get(MIB_UPGRADE_WEB_PUSH_TIME, (void *)&startTime);

	if (0 == startTime)
	{
		webPushSched = 0;
		return;
	}
	
    gettimeofday(&tv,NULL);

	if (tv.tv_sec < startTime)//time sysnc fail
		return;

	if (tv.tv_sec >= (startTime+SEVEN_DAY))
		upgradeWebSet(1);
	else
		startPushwebTimer(SEVEN_DAY-(tv.tv_sec-startTime));

	webPushSched = 0;
}
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)

static int createMsgForOMCI(){
		//according with CATV spec,only set the internet type interface vlan tag
		//if (RT_ERR_OK == ctc_clf_rule_for_vlanTag_create(port, pTagCfg))
		//	return EPON_OAM_ERR_OK;
		key_t msgQKey = ftok("/bin/omci_app", 's');
	
		
		   /* Get message queue to send event to state keeper */
		   /* S_IRUSR | S_IWUSR | State keeper can read/write message
			* S_IRGRP | S_IWGRP | All others can read/write message
			* S_IROTH | S_IWOTH   All others can read/write message
			*/
		unsigned int permits = 00666;
		omci_msg2boaData_t event;
		int ret = 0;
		permits |= IPC_CREAT;
		if(!msg2boaQId){
			msg2boaQId = msgget(msgQKey, permits);
			if(-1 == msg2boaQId){
				   printf("[OAM:%s:%d] msgq create failed %d\n", __FILE__, __LINE__, errno);
				   return -1;
			}
		}
					
}
int msg2boaQId=0;
int processMsgForQos(){
	int ret=0;
	omci_msg2boaData_t eventData;
	int i = 0;	
	while(1){
		int msg_ret = msgrcv(msg2boaQId, (void *) &eventData, sizeof(eventData.msgqData), 0, IPC_NOWAIT); /* Blocking call */
		if(-1 == msg_ret){
			if(EINTR == errno)
				continue;
			else// if(ENOMSG==errno)
			{
				break;
			}
					
		}	

		switch(eventData.mtype)
		{
			case OMCI_COMPLETE_CONFIG:	
				printf("%s %d trigger the qos config after OMCI complete config\n",__FUNCTION__,__LINE__);
				take_qos_effect_v3();
       		break;
			default:
			break;
		}
		break;
	}
	return ret;
}


#endif	

int main(int argc, char **argv)
{
	int c;						/* command line arg */
#ifdef CONFIG_YUEME
	int s_port = 8080;
#else
	int s_port = 80;
#endif
	FILE *fp, *fp2;
	// Added by Mason Yu for Reset button
	FILE *fp3, *fp4;
	int i, i2;
	char reset_button_flag=1;
#ifdef CONFIG_RTL8676_CHECK_WIFISTATUS
		// Added by Ken for Wifi 88ER Check Status
		FILE *wifi_status_fp ;
		int wifi_status_num = 0; 
#endif
	char reset_led_status=0;
	char led_status=0;
#ifdef CONFIG_SUPPORT_AUTO_DIAG
		//int autoBridgePPPfifo;
#endif

//xl_yue add
#ifdef USE_LOGINWEB_OF_SERVER
	struct user_info * pUser_info;
	struct user_info * bUser_info = NULL;

#ifdef LOGIN_ERR_TIMES_LIMITED
	struct errlogin_entry * perrlog;
	struct errlogin_entry * berrlog = NULL;
#endif

#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	createMsgForOMCI();
#endif

	char ifname[IFNAMSIZ];
	pid_t boa_pid;
	unsigned char value[16];

	// print debug message
	//DEBUGMODE(STA_INFO|STA_SCRIPT|STA_WARNING|STA_ERR);
#ifdef SUPPORT_ASP
	boaASPInit(argc,argv);
#endif
#ifdef EMBED
	log_pid();
#endif
	openlog("boa", LOG_PID, 0);


	// Added by davian_kuo
	/* Init the language set with English. */
//	char mStr[MAX_LANGSET_LEN] = {0};
//	if (!mib_get (MIB_MULTI_LINGUAL, (void *)mStr)) {
//		fprintf (stderr, "mib get multi-lingual setting (boa) failed!\n");
//		return;
//	}

	// Prevent segmentation fault or index wrong lang after updating rootfs.
//	unsigned char langIdx = 0;
//	for (langIdx = 0; langIdx < LANG_MAX; langIdx++) {
//		if (!strcmp(mStr, lang_set[langIdx].langType)) break;
//	}
//	if (langIdx < LANG_MAX) {
//		g_language_state = g_language_state_prev = langIdx;
//	} else {
//		fprintf (stderr,
//			"Original [%s] is missing!! Switch to new default [%s].\n",
//			mStr, lang_set[0].langType);

//			g_language_state = g_language_state_prev = 0;
//	}

	//printf ("Language state => %d\n", g_language_state);
//#if MULTI_LANG_DL == 1
//	if (dl_handle != NULL) dlclose (dl_handle);

//	char *lib_name = (char *) malloc (sizeof(char) * 25);
//	if (lib_name == NULL) {
//		fprintf (stderr, "lib_name malloc failed!\n"); return;
//	}
//	char *strtbl_name = (char *) malloc (sizeof(char) * 15);
//	if (strtbl_name == NULL) {
//		fprintf (stderr, "strtbl_name malloc failed!\n"); return;
//	}
//	snprintf (lib_name, 25, "libmultilang_%s.so", lang_set[g_language_state].langType);
//	snprintf (strtbl_name, 15, "strtbl_%s", lang_set[g_language_state].langType);
//	dl_handle = dlopen (lib_name, RTLD_LAZY);
//	strtbl = (const char **) dlsym (dl_handle, strtbl_name);

//	free (lib_name);
//	free (strtbl_name);
//#else
	//strtbl = strtbl_name[g_language_state];
	strtbl = strtbl_name[LANG_EN];
//#endif


#ifdef SERVER_SSL
	while ((c = getopt(argc, argv, "p:vc:dns")) != -1)
#else
	while ((c = getopt(argc, argv, "p:vc:d")) != -1)
#endif /*SERVER_SSL*/
	{
		switch (c) {
		case 'c':
			server_root = strdup(optarg);
			break;
		case 'v':
			verbose_logs = 1;
			break;
		case 'd':
			do_fork = 0;
			break;
#ifdef EMBED
		case 'p':
			s_port= atoi(optarg);
			break;
#endif
#if SERVER_SSL
		case 'n':
			do_sock = 0;		/*We don't want to do normal sockets*/
			break;
		case 's':
			do_ssl = 0;		/*We don't want to do ssl sockets*/
			break;

#endif /*SERVER_SSL*/
		default:
#if 0
			fprintf(stderr, "Usage: %s [-v] [-s] [-n] [-c serverroot] [-d]\n", argv[0]);
#endif
			exit(1);
		}
	}

	fixup_server_root();

#ifdef EMBED
	// Added by Mason Yu(2 level)
	fp = fopen("/var/boaSuper.passwd", "w+");
	fp2 = fopen("/var/boaUser.passwd", "w+");
	fclose(fp);
	fclose(fp2);
	#if SUPPORT_AUTH_DIGEST
	fp = fopen("/var/DigestUser.passwd", "w+");
	fclose(fp);
	fp = fopen("/var/DigestSuper.passwd", "w+");
	fclose(fp);
	#endif
#endif


	read_config_files();
#ifdef EMBED
	if(s_port != 80)
		set_server_port(s_port);
#endif
#ifdef BOA_TIME_LOG
	open_logs();
#endif
	create_common_env();

#ifdef SERVER_SSL
	if (do_ssl) {
		if (InitSSLStuff() != 1) {
			/*TO DO - emit warning the SSL stuff will not work*/
			syslog(LOG_ALERT, "Failure initialising SSL support - ");
			if (do_sock == 0) {
				syslog(LOG_ALERT, "    normal sockets disabled, so exiting");fflush(NULL);
				return 0;
			} else {
				syslog(LOG_ALERT, "    supporting normal (unencrypted) sockets only");fflush(NULL);
				do_sock = 2;
			}
	  }
	} else
		do_sock = 2;
#endif /*SERVER_SSL*/

#ifdef SERVER_SSL
	if(do_sock){
#endif /*SERVER_SSL*/
#ifdef IPV6
	if ((server_s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1)
#else
	if ((server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
#endif
		die(NO_CREATE_SOCKET);

	/* server socket is nonblocking */
	if (fcntl(server_s, F_SETFL, NOBLOCK) == -1)
		die(NO_FCNTL);

	if ((setsockopt(server_s, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
					sizeof(sock_opt))) == -1)
		die(NO_SETSOCKOPT);

	/* internet socket */
#ifdef IPV6
	server_sockaddr.sin6_family = AF_INET6;
	memcpy(&server_sockaddr.sin6_addr,&in6addr_any,sizeof(in6addr_any));
	server_sockaddr.sin6_port = htons(server_port);
#else
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sockaddr.sin_port = htons(server_port);
#endif

	if (bind(server_s, (struct sockaddr *) &server_sockaddr,
			 sizeof(server_sockaddr)) == -1)
		die(NO_BIND);

	/* listen: large number just in case your kernel is nicely tweaked */
	if (max_connections != -1)
		backlog = MIN(backlog, max_connections);
	if (listen(server_s, backlog) == -1)
		die(NO_LISTEN);

	if (server_s > max_fd)
		max_fd = server_s;
#ifdef SERVER_SSL
	}
#endif /*SERVER_SSL*/


#ifdef WLAN_WEB_REDIRECT //jiunming,web_redirect
	if( init_redirect_socket() > 0 )
		if (redir_server_s > max_fd)
			max_fd = redir_server_s;
#endif
#ifdef WEB_REDIRECT_BY_MAC
	if( init_mac_redirect_socket() > 0 )
		if (mac_redir_server_s > max_fd)
			max_fd = mac_redir_server_s;
#endif
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	if( init_captiveportal_socket() > 0 )
		if (captiveportal_server_s > max_fd)
			max_fd = captiveportal_server_s;
#endif

	init_global_parm();
	init_signals();
#ifdef CONSOLE_BOA
	signal(SIGINT, SIG_IGN);
#endif
	/* background ourself */

#ifndef EMBED
	if (do_fork)
		if (fork())
			exit(0);
#endif

	/* close server socket on exec
	 * so cgi's can't write to it */

	if (fcntl(server_s, F_SETFD, 1) == -1) {
#if 0
		perror("can't set close-on-exec on server socket!");
#endif
		exit(0);
	}


#if 0
	/* close STDIN on exec so cgi's can't read from it */
	if (fcntl(STDIN_FILENO, F_SETFD, 1) == -1) {
#if 0
		perror("can't set close-on-exec on STDIN!");
#endif
		exit(0);
	}
#endif

	/* translate paths to server_chroot */
	chroot_aliases();
	chroot_virtual_hosts();

#ifdef USE_CHROOT
	if (server_chroot)
	{
		if (!strncmp(server_root,server_chroot,strlen(server_chroot)))
				strcpy(server_root,server_root + strlen(server_chroot) -
						(server_chroot[strlen(server_chroot)-1]=='/'?1:0) );
		else
		{
#ifdef BOA_TIME_LOG
			log_error_time();
			fprintf(stderr,"Warning: server_root not accessible from %s\n",
					server_chroot);
#endif
			syslog(LOG_ERR, "server root not accessible");
		}
	}
  if (server_chroot)
  {
    if (!strncmp(dirmaker,server_chroot,strlen(server_chroot)))
        strcpy(dirmaker,dirmaker + strlen(server_chroot) -
            (server_chroot[strlen(server_chroot)-1]=='/'?1:0) );
    else
    {
#ifdef BOA_TIME_LOG
      log_error_time();
      fprintf(stderr,"Warning: directory maker not accessible from %s\n",
          server_chroot);
#endif
	  syslog(LOG_ERR, "directory maker not accessible");
    }
  }
#endif

#ifdef USE_AUTH
	auth_check();	/* Check Auth'ed directories */
#endif

	DBG(printf("main:give out privs\n");)
	/* give away our privs if we can */
#ifdef EMBED
	server_gid = getgid();
	server_uid = getuid();
#else
	if (getuid() == 0) {
		struct passwd *passwdbuf;
		passwdbuf = getpwuid(server_uid);
		if (passwdbuf == NULL)
			die(GETPWUID);
		if (initgroups(passwdbuf->pw_name, passwdbuf->pw_gid) == -1)
			die(INITGROUPS);
                if (server_chroot)
                  if (chroot(server_chroot))
                    die(CANNOT_CHROOT);
		if (setgid(server_gid) == -1)
			die(NO_SETGID);
		if (setuid(server_uid) == -1)
			die(NO_SETUID);
	} else {
		if (server_gid || server_uid) {
#ifdef BOA_TIME_LOG
			log_error_time();
			fprintf(stderr, "Warning: "
					"Not running as root: no attempt to change to uid %d gid %d\n",
					server_uid, server_gid);
#endif
		}
		server_gid = getgid();
		server_uid = getuid();
	}
#endif /* EMBED */

	/* main loop */

#ifdef EMBED

	// Added by Mason Yu(2 level)
	writePasswdFile();

#ifdef MENU_CLI
	// start climenu thread
	startCli();
#endif
#endif
	timestamp();
	FD_ZERO(&block_read_fdset);
	FD_ZERO(&block_write_fdset);

	status.connections = 0;
	status.requests = 0;
	status.errors = 0;
//#if 0
//jim


#ifdef EMBED
#ifdef CONFIG_CTC_E8_CLIENT_LIMIT
	//memset(accepted_PC_Clients, 0, sizeof(accepted_PC_Clients));
	//memset(accepted_Camera_Clients, 0, sizeof(accepted_Camera_Clients));
#ifdef _CWMP_MIB_
#ifdef _PRMT_X_CT_COM_MWBAND_
#ifndef CONFIG_YUEME
	proc_write_for_mwband();
#endif
#endif
#endif
#if 0
	printf("creat process to hander read_proc_handler\n");
	{
		pid_t pid;
		pid=fork();
		if(pid==-1)
		{
			printf("fork error\n");
			return;
		}
		if(pid==0)
		{	//child process
			//no need to
			/*
			signal(SIGALRM, read_proc_handler);
			read_proc_interval;
			read_proc_interval.it_value.tv_sec=1;
			read_proc_interval.it_value.tv_usec=0;
			read_proc_interval.it_interval=read_proc_interval.it_value;
			setitimer(ITIMER_REAL, &read_proc_interval, NULL );
			while(1);*/
			while(1)
			{
				sleep(1);
				read_proc_handler(0);  //for iptables
			}
		}
		if(pid)
		{
			printf("child process pid=%d created in boa\n ", pid);
		}
	}
#endif
#endif
#endif // of EMBED

#ifdef PARENTAL_CTRL
	parent_ctrl_table_init();
#endif
#ifdef _PRMT_X_CMCC_SECURITY_
	ParentalCtrl_rule_init();
#endif

#ifdef SUPPORT_WEB_REDIRECT
	welcomeRedirectCheck(NULL);
#ifdef CONFIG_RTK_L34_ENABLE
	webRedirectSetRGRule(1);

	web404RedirectCheck();
#endif
#endif

#ifdef EMBED
#ifdef WEB_REDIRECT_BY_MAC
	TIMEOUT(clearLandingPageRule, 0, 10, landingPage_ch);
#endif

#ifdef AUTO_DETECT_DMZ
	TIMEOUT(poll_autoDMZ, 0, 10, autoDMZ_ch);
#endif

/*
#ifdef CONFIG_LED_INDICATOR_TIMER
	TIMEOUT(led_schedule, 0, 60, ledsched_ch);
#endif

#ifdef CONFIG_RG_SLEEPMODE_TIMER
	TIMEOUT(sleepmode_schedule, 0, 60, sleepmode_ch);
#endif
*/
#ifdef SUPPORT_WAN_BANDWIDTH_INFO
	TIMEOUT(poll_bandwidth, 0, 1, bandwidth_ch); /* default interval 1 second */	
#endif
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
	TIMEOUT(poll_ponFlowTxBytes, 0, 1, ponFlowTxBytes_ch); 
#endif
#if 0//def CONFIG_USER_LAN_BANDWIDTH_CONTROL
	TIMEOUT((void*)poll_macinfo, 0, 1, macinfo_ch);
#endif

#ifdef CONFIG_USER_QUICKINSTALL
	TIMEOUT(poll_wan_downspeed, 0, 1, downspeed_ch);
#endif

#ifdef CONFIG_HTTP_DOWNLOAD_TEST
	TIMEOUT(httpTestSchedule, 0, 1, httptest_ch);
#endif

#ifdef CONFIG_SUPPORT_AUTO_DIAG
	//TIMEOUT(startAutoBridgePppoeSimulation, 0, 1, autoSimulation_ch);
	//autoBridgePPPfifo = initAutoBridgeFIFO();
#endif

#ifdef CONFIG_USER_CUMANAGEDEAMON
	TIMEOUT(cuManageSechedule, 0, 1, cuManage_ch);
#endif

#if defined(PON_HISTORY_TRAFFIC_MONITOR)
	TIMEOUT(pon_traffic_monitor, 0, PON_HISTORY_TRAFFIC_MONITOR_INTERVAL, pon_traffic_monitor_ch);
#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT
	TIMEOUT(ntpSechedule_wapi, 0, 10, ntp_ch);
#endif

#endif // of EMBED
#if defined(CONFIG_CT_AWIFI_JITUAN_SMARTWIFI)
  unsigned char functype=0;
  mib_get(AWIFI_PROVINCE_CODE, &functype);
  if(functype == AWIFI_ZJ){
    TIMEOUT(wifiAuthCheck, 0, 5, wifiAuth_ch);
  }
#endif

#ifdef CTC_TELNET_SCHEDULED_CLOSE
	TIMEOUT((void*)schTelnetCheck, 0, 10, sch_telnet_ch);
#endif

	while (1) {
		static int dsl_link = 0;
		static int do_AutoSearchPVC = 0;
		int mypid;
		unsigned char vChar;
		char filename[64];
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
		processMsgForQos();
#endif

#ifdef EMBED
		calltimeout();
#if 0
//#ifdef CONFIG_SUPPORT_AUTO_DIAG
		if(poll_msg(autoBridgePPPfifo))
		{
			int i, read_res, aargc=0;
			char aargv[7][32+1];
			char buff[256];
			char *arg_ptr;

			read_res = read(autoBridgePPPfifo, buff, sizeof(buff));
			if(read_res > 0)
			{
				int arg_idx = 0;
				arg_ptr = buff;
				for(i=0; arg_ptr[i]!='\0'; i++) 
				{
					if(arg_ptr[i]==' ')
					{
						aargv[aargc][arg_idx]='\0';
						aargc++;
						arg_idx=0;
					}
					else 
					{
						if(arg_idx<32) 
						{
							aargv[aargc][arg_idx]=arg_ptr[i];
							arg_idx++;
						}
					}
				}
				aargv[aargc][arg_idx]='\0';
			}
			if(!strcmp(aargv[0],"start"))
			{
				printf("[%s %d]Get a start, --->autoSimulation<----\n", __func__, __LINE__);
				startAutoBridgePppoeSimulation();
			}
			else if(!strcmp(aargv[0],"stop"))
			{
				printf("[%s %d]Get a stop, --->autoSimulation<----\n", __func__, __LINE__);
				stopAutoBridgePppoeSimulation();
			}
			else if(!strcmp(aargv[0],"restart"))
			{
				printf("[%s %d]Get a restart, --->autoSimulation<----\n", __func__, __LINE__);
				stopAutoBridgePppoeSimulation();
				startAutoBridgePppoeSimulation();
			}
			else if(!strcmp(aargv[0],"omci"))
			{
				printf("[%s %d]Get a omci, --->omci success<----\n", __func__, __LINE__);
				setOmciState(1);
			}
			else
			{
				printf("Command %s not supported yet.\n", buff);
			}
		}
#endif

#ifdef SUPPORT_WEB_PUSHUP
		web_push_schedule();
#endif
#ifdef CONFIG_LEDPWR_PBC
		if((fp = fopen("/proc/led_power","r")) != NULL)
		{
			static unsigned int last_led_power_flag = 1;
			unsigned int led_power_flag = 1;
			fscanf(fp,"%u",&led_power_flag);
			fclose(fp);
			if(led_power_flag != last_led_power_flag)
			{
				last_led_power_flag = led_power_flag;
				if(led_power_flag == 0)// all led off 
				{
					system("/bin/mpctl led off");
				}
				else if(led_power_flag == 1)//all led restore original state
				{
					system("/bin/mpctl led restore");
				}
			}	
		}	
#endif//CONFIG_LEDPWR_PBC

		// Kaohj --- Internet LED
		//time_t newtime,oldtime=0;
#ifdef WLAN_SUPPORT
		int button_flag;
		unsigned char wlan_flag;
		FILE *fpwlan;
#endif
//star: for wlan on/off button
#if defined( WLAN_SUPPORT) && defined(CONFIG_WLAN_ON_OFF_BUTTON)
		int orig_wlanid = wlan_idx;
		MIB_CE_MBSSIB_T mEntry;
		fpwlan = fopen("/proc/wlan_onoff","r+");
		if(fpwlan)
		{
			fscanf(fpwlan,"%d",&button_flag);

			if(button_flag==1)
			{
#if defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU)
#ifdef YUEME_3_0_SPEC
				mib_local_mapping_get(MIB_WLAN_DISABLED, 0, (void *)&vChar);
				vChar = vChar? 0:1;
				mib_local_mapping_set(MIB_WLAN_DISABLED, 0, (void *)&vChar);
#ifdef WLAN_DUALBAND_CONCURRENT
				mib_local_mapping_set(MIB_WLAN_DISABLED, 1, (void *)&vChar);
#endif
#else
				mib_get(MIB_WIFI_MODULE_DISABLED, (void *)&vChar);
				vChar = vChar? 0:1;
				mib_set(MIB_WIFI_MODULE_DISABLED, (void *)&vChar);
#endif
#else
				wlan_idx=0;
				wlan_getEntry(&mEntry, 0);
				vChar = mEntry.wlanDisabled;
				if(vChar == 0)
					vChar = 1;
				else
					vChar = 0;
				mEntry.wlanDisabled = vChar;
				wlan_setEntry(&mEntry, 0);
				#ifdef WLAN_DUALBAND_CONCURRENT
				wlan_idx=1;
				wlan_getEntry(&mEntry, 0);
				mEntry.wlanDisabled = vChar;
				wlan_setEntry(&mEntry, 0);
				#endif				
				wlan_idx = orig_wlanid;
#ifdef CONFIG_WIFI_SIMPLE_CONFIG // WPS
				update_wps_configured(0);
#endif
#endif

				if(mib_update(CURRENT_SETTING, CONFIG_MIB_ALL) == 0)
					printf("CS Flash error! \n");
				config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);

				//wlan_flag = '0';
				//fwrite(&wlan_flag,1,1,fpwlan);
			}
#ifdef CONFIG_CT_PUSHKEYEVENT
			else if(button_flag==4)//2 and 3 for push key event
#else
			else if(button_flag==2)
#endif
			{
				config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);

				//wlan_flag = '0';
				//fwrite(&wlan_flag,1,1,fpwlan);
			}

			wlan_flag = '0';
			fwrite(&wlan_flag,1,1,fpwlan);

			fclose(fpwlan);

#ifdef CONFIG_CT_PUSHKEYEVENT
#ifdef CONFIG_MIDDLEWARE
			mib_get(CWMP_TR069_ENABLE,(void *)&vChar);
			if(vChar == 0 || vChar == 2){//enable Middleware
				if(button_flag == 1){
					printf("wifi button is short pushed\n");
					vChar = KEYEVENT_WLANSHORT;
				}
				else if(button_flag == 2){
					vChar = KEYEVENT_WLANLONG;
					printf("wifi button is long pushed\n");
				}
				else if(button_flag == 3){
					vChar = KEYEVENT_WLANDOUBLE;
					printf("wifi button is double pushed\n");
				}
				else
					vChar = 0;
				
				if(vChar != 0){
					sendInformEventMsg2MidProcess( vChar );
				}
			}
#endif	//end of CONFIG_MIDDLEWARE
#endif	//end of CONFIG_CT_PUSHKEYEVENT
		}
#endif	//end of WLAN_SUPPORT && CONFIG_WLAN_ON_OFF_BUTTON
//cathy, wsc write "R" to gpio for restarting wlan, if "2" is read from gpio->restart wlan
#if defined(WLAN_SUPPORT) && defined(CONFIG_WIFI_SIMPLE_CONFIG)
		{
#if defined(WLAN_DUALBAND_CONCURRENT)
			if (getInFlags((char *)WLANIF[0], 0) || getInFlags((char *)WLANIF[1], 0)) {
#else
			if (getInFlags((char *)WLANIF[0], 0)) {
#endif
				int wps_wlan_restart;
				//unsigned char wps_flag;
				FILE *fpwps;
				fpwps = fopen("/proc/gpio","r+");
				if(fpwps) {
					fscanf(fpwps,"%d",&wps_wlan_restart);
					if(wps_wlan_restart==2) {
						#ifndef WLAN_WPS_VAP
						update_wps_from_mibtable();
						#endif
						printf("wps wlan restart\n");
						config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
					}
					fclose(fpwps);
				}
			}
		}
			
#ifdef CONFIG_CT_PUSHKEYEVENT
		fpwlan = fopen("/proc/wps_button","r+");
		if(fpwlan)
		{
			fscanf(fpwlan,"%d",&button_flag);
			if(button_flag)
			{
				if (1 == button_flag){//start wps 
					update_wps_from_mibtable();
					printf("wps wlan restart\n");
					config_WLAN(ACT_RESTART, CONFIG_SSID_ALL);
				}	
				wlan_flag = '0';
				fwrite(&wlan_flag,1,1,fpwlan);
			}		
			fclose(fpwlan);
#ifdef CONFIG_MIDDLEWARE
			mib_get(CWMP_TR069_ENABLE,(void *)&vChar);
			if(vChar == 0 || vChar == 2){//enable middleware
				if(button_flag == 1) {
					vChar = KEYEVENT_WPSSHORT;
					printf("wps button is short pushed\n");
				}
				else if(button_flag == 2){
					vChar = KEYEVENT_WPSLONG;
					printf("wps button is long pushed\n");
				}
				else if(button_flag == 3){
					vChar = KEYEVENT_WPSDOUBLE;
					printf("wps button is double pushed\n");
				}else {
					vChar = 0;
				}	
				if(vChar != 0){
					sendInformEventMsg2MidProcess( vChar );	
				}
			}
#endif	//end of CONFIG_MIDDLEWARE
		}
#endif	//end of CONFIG_CT_PUSHKEYEVENT
#endif	// end of WLAN_SUPPORT) && CONFIG_WIFI_SIMPLE_CONFIG

#endif // of EMBED

#ifdef EMBED
                #ifdef CONFIG_LEDPWR_PBC
                {
                        FILE *fpled;
                        int led_st = 0;
                        static int led_cur_st = 0;
                        fpled = fopen("/proc/ledpwr","r");
                        if(fpled)
                        {
                                fscanf(fpled,"%d",&led_st);
                                if(led_st == 1)
                                {
                                        system("mpctl led on");
                                        led_cur_st = 1;
                                }
                                else if(led_st == 0 && led_cur_st == 1)
                                {
                                        system("mpctl led restore");
                                        led_cur_st = 0;
                                }
                                fclose(fpled);
                        }
                }
                #endif //CONFIG_LEDPWR_PBC

		// Added by Mason Yu for check Reset button
		// (1) Check Reset to default
		mib_get(PROVINCE_DEV_REG_TYPE,(void *)&vChar);
		if(vChar==DEV_REG_TYPE_AH)
		{
			snprintf(filename, 64, "/proc/load_ah_default");
		}
		else
		{
			snprintf(filename, 64, "/proc/load_default");
		}

		fp3=fopen(filename,"r");

		if (fp3 && reset_button_flag)
		{
			fscanf(fp3,"%d",&i);
			fclose(fp3);
			if(i==1 || i==2)
			{
				printf("Going to Reload Default, longreset = %u\n", i == 2);
				if(i==1)
					reset_cs_to_default(3);
				else{
					unsigned int reset_button = 0;
					mib_get(PROVINCE_SICHUAN_RESET_BUTTON, &reset_button);
					printf("PROVINCE_SICHUAN_RESET_BUTTON = %u\n", reset_button);
					if(reset_button){
						system("mpctl led off");
						system("echo 1 > /proc/led_power");
						}
					reset_cs_to_default(4);
#ifdef CONFIG_CT_AWIFI_JITUAN_SMARTWIFI
        unsigned char functype=0;
        mib_get(AWIFI_PROVINCE_CODE, &functype);
        if(functype == AWIFI_ZJ){
          system("cp -f /var/config/awifi/awifi_bak.conf /var/config/awifi/awifi.conf");
          if(access("/var/config/awifi/binversion", F_OK) == 0)
            system("rm -f /var/config/awifi/binversion");
        }
#endif
					}
				cmd_reboot();
				reset_button_flag = 0;
			}
			else if(i==3){
				if(led_status==0)
					system("mpctl led on");
				else
					system("mpctl led off");
				led_status = 1 - led_status;
				reset_led_status = 1;
			}
			if(reset_led_status==1){
				if(i==0){
					system("mpctl led restore");
					led_status = 0;
					reset_led_status = 0;
				}
			}
		}
		else if (fp3)
			fclose(fp3);

        	// (2) Check Reset to current
		fp4=fopen("/proc/load_reboot","r");

		if (fp4 && reset_button_flag)
		{
        		fscanf(fp4,"%d",&i2);
        		fclose(fp4);
        		if(i2==1)
        		{
            			printf("Going to Reboot\n");
						sleep(1); //For OSGI could have time to send response then reboot.
						cmd_reboot();
						reset_button_flag = 0;
        		}
        	}
        	else if (fp4)
        		fclose(fp4);

#ifdef CONFIG_RTL8676_CHECK_WIFISTATUS
			wifi_status_fp = fopen("/proc/wifi_checkstatus","r");

			if (wifi_status_fp)
			{
				fscanf(wifi_status_fp,"%d",&wifi_status_num);
				fclose(wifi_status_fp);
				if(wifi_status_num==1)
				{
					fprintf(stderr, "Wi-Fi Restart\n");
					system("echo 1 > /proc/wifi_checkstatus");
					cmd_wlan_delay_restart();
				}
			}
#endif

#endif // of EMBED
#ifdef EMBED

		if ( get_net_link_status(ALIASNAME_NAS0) == 1)
		{
			if (dsl_link == 0) { // down --> up
				unsigned int igmpItf;
				int flags;
				syslog(LOG_INFO, "DSL Link up");
				// signal dhcp client to renew
				mypid = read_pid((char*)DHCPC_PID);
				if (mypid > 0) { // renew
					kill(mypid, SIGUSR1);
				}
				updateLinkTime(1);


#ifdef CONFIG_USER_IGMPPROXY
				// Kaohj -- if igmpproxy upstream interface is up, signal to reset multicast MFC
				mypid = read_pid("/var/run/igmp_pid");
				if (mypid >= 1) {
					#ifdef CONFIG_IGMPPROXY_MULTIWAN
					kill(mypid, SIGUSR2);
					#else
					if (mib_get(MIB_IGMP_PROXY_ITF, (void *)&igmpItf) != 0)
					{
						if (ifGetName(igmpItf, ifname, sizeof(ifname)) {
							if (getInFlags( ifname, &flags) == 1){
								if (flags & IFF_UP)
									kill(mypid, SIGUSR2);
							}
						}
						else
						{
							printf("Error: IGMP proxy interface not available !\n");
						}
					}
					#endif
				}
#endif // of CONFIG_USER_IGMPPROXY

			}
			dsl_link = 1;

#ifdef E8B_NEW_DIAGNOSE
			check_inform_status(0);
#endif
		}
		else {
			if (dsl_link == 1) { // up --> down
				syslog(LOG_INFO, "DSL Link down");
#ifdef CONFIG_USER_IGMPPROXY
				// Kaohj -- if igmpproxy upstream interface is down, signal to reset multicast MFC
				mypid = read_pid("/var/run/igmp_pid");
				if (mypid >= 1)
					kill(mypid, SIGUSR2);
#endif // of CONFIG_USER_IGMPPROXY
			}
			dsl_link = 0;

		}
#endif // of EMBED

		// jim the internet polling will slow down boa response time....
		// espacially, it will cause ZTE 531B web upgrade fail....
		// Kaohj --- Internet LED (moved from startup.c)
		//newtime = time(0);
		//if(newtime-oldtime>=5){
		//	Internet_status();
		//	oldtime = newtime;
		//}
{
	//printf("check time to reset register time every 24 hours\n");
	time_t tm;
	static const char REGISTER_NUM_FILE[] = "/tmp/oam_register_num";
	static const char REGISTER_NUM_FILE_LAST[] = "/tmp/oam_register_num_last";
	char exeCmd[100] = {};
	
	time(&tm);
	tm_mday = localtime(&tm)->tm_mday;
	//printf("tm_mday=%d,last_tm_mday=%d!\n",tm_mday,last_tm_mday);
	if(tm_mday-last_tm_mday){
		printf("reset DHCP register times!\n");
#ifdef 	_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterNumberEnable_
		clearDHCPRegisterNumber();
		defRegisterNumberITMS();
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterSuccessNumberEnable_
		clearDHCPSuccessNumber();
		defRegisterSuccNumITMS();
#endif
#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DialingNumberEnable_)\
|| defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DialingErrorEnable_)
		unlink("/tmp/ppp_dialing_info");
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterNumberEnable_
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterSuccessNumberEnable_
		system("/bin/oamcli get ctc registernum 1"); /* save current number to /tmp/oam_register_num */
		sprintf(exeCmd, "cp %s %s", REGISTER_NUM_FILE, REGISTER_NUM_FILE_LAST); //copy the old register number file
		system(exeCmd);
#endif
#endif
	}
	last_tm_mday = tm_mday;
}
#ifdef PARENTAL_CTRL
		time_counter = getSYSInfoTimer();
		if ((time_counter & 0x07) == 0 )
		{
			time_t tm;
			struct tm the_tm;

	 		time(&tm);
			memcpy(&the_tm, localtime(&tm), sizeof(the_tm));
			parent_ctrl_table_rule_update();
			//printf("!!!UPdate!! \r\n");
			//parent_ctrl_table_rule_update();

//			printf("day of the week:%d\r\n",(int)the_tm.tm_wday);
//			printf("month:%d",the_tm.tm_mon);
//			printf("time_counter:%d\r\n",time_counter);

		}
#endif
#ifdef _PRMT_X_CMCC_SECURITY_
		time_counter = getSYSInfoTimer();
		if ((time_counter & 0x07) == 0)
		{
			unsigned char rule_reset = 0;
			mib_get(MIB_RS_PARENTALCTRL_RULE_RESET, (void *)&rule_reset);
			if (rule_reset)
			{
				rule_reset = 0;
				mib_set(MIB_RS_PARENTALCTRL_RULE_RESET, (void *)&rule_reset);
				ParentalCtrl_rule_init();
			}
			ParentalCtrl_rule_set();
		}

#endif

#ifdef CU_APP_SCHEDULE_LOG
		{
			time_t t_cur = time(NULL);
			time_t t_schd[2];
			FILE * fp_sched = NULL;

			fp_sched = fopen("/var/config/syslog_schedule_upload","r");
			if(fp_sched)
			{
				fread(&t_schd,sizeof(t_schd),1,fp_sched);
				fclose(fp_sched);

				if(t_cur >= t_schd[1])
				{
					unsigned char upload_log_flag = 0;
					if(mib_get(SCHEDULE_LOG_FLAG,(void*)&upload_log_flag) && upload_log_flag == 0){
						printf("Create log - Start from : %s\n",ctime(&t_schd[0]));
						printf("Create log - End at : %s\n",ctime(&t_schd[1]));
						GenerateScheduledLog(t_schd[0],t_schd[1]);
						UploadScheduledLog();
						upload_log_flag = 1;
						mib_set(SCHEDULE_LOG_FLAG,(void*)&upload_log_flag);
						Commit();
					}
				}
			}
		}
#endif

		if (sighup_flag)
			sighup_run();
		if (sigchld_flag)
			sigchld_run();

		switch(lame_duck_mode) {
			case 1:
				lame_duck_mode_run(server_s);
                        case 2:
				if (!request_ready && !request_block)
					die(SHUTDOWN);
				break;
			default:
				break;
		}

		/* move selected req's from request_block to request_ready */
		fdset_update();

		if (!request_ready) {
			request *current;

			max_fd = 0;
			max_fd = MAX(server_s, max_fd);
#ifdef WLAN_WEB_REDIRECT //jiunming,web_redirect
			max_fd = MAX(redir_server_s, max_fd);
			FD_SET(redir_server_s, &block_read_fdset);
#endif
#ifdef WEB_REDIRECT_BY_MAC
			max_fd = MAX(mac_redir_server_s, max_fd);
			FD_SET(mac_redir_server_s, &block_read_fdset);
#endif
#ifdef SERVER_SSL
			if (do_sock < 2)
				max_fd = MAX(server_ssl, max_fd);
#endif
			for (current = request_block; current; current = current->next) {
				max_fd = MAX(current->fd, max_fd);
				max_fd = MAX(current->data_fd, max_fd);
				max_fd = MAX(current->post_data_fd, max_fd);
			}

			req_timeout.tv_sec =1;
			req_timeout.tv_usec = 0;
			if (select(max_fd + 1, &block_read_fdset, &block_write_fdset, NULL,
					   &req_timeout) == -1) 
			{
				if (errno == EINTR || errno == EBADF)
					continue;	/* while(1) */
				else
					die(SELECT);
			}
#ifdef SERVER_SSL
			if(do_sock){
				if (FD_ISSET(server_s, &block_read_fdset))
					get_request();
			}
#else
			if (FD_ISSET(server_s, &block_read_fdset))
				get_request();
#ifdef WLAN_WEB_REDIRECT //jiunming,web_redirect
			if (FD_ISSET(redir_server_s, &block_read_fdset))
				get_redir_request();
#endif
#ifdef WEB_REDIRECT_BY_MAC
			if (FD_ISSET(mac_redir_server_s, &block_read_fdset))
				get_mac_redir_request();
#endif
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
			if (FD_ISSET(captiveportal_server_s, &block_read_fdset))
				get_captiveportal_request();
#endif
#endif /*SERVER_SSL*/

#ifdef SERVER_SSL
			if (do_sock < 2) {
				if(FD_ISSET(server_ssl, &block_read_fdset)){ /*If we have the main SSL server socket*/
					get_ssl_request();
				}
			}
#endif /*SERVER_SSL*/
		}
		process_requests();		/* any blocked req's move from request_ready to request_block */
	}
}

/*
 * Name: fdset_update
 *
 * Description: iterate through the blocked requests, checking whether
 * that file descriptor has been set by select.  Update the fd_set to
 * reflect current status.
 */

void fdset_update(void)
{
	request *current, *next;
	time_t current_time;

	current = request_block;
	time_counter = getSYSInfoTimer();
	current_time = time_counter;
	while (current) {
		time_t time_since;
		next = current->next;

		time_since = current_time - current->time_last;

		/* hmm, what if we are in "the middle" of a request and not
		 * just waiting for a new one... perhaps check to see if anything
		 * has been read via header position, etc... */
		if (current->kacount && (time_since >= ka_timeout) && !current->logline) {
			SQUASH_KA(current);
			free_request(&request_block, current);
		} else if (time_since > REQUEST_TIMEOUT) {
#ifdef BOA_TIME_LOG
			log_error_doc(current);
			fputs("connection timed out\n", stderr);
#endif
			SQUASH_KA(current);
			free_request(&request_block, current);
		} else if (current->buffer_end) {
			if (FD_ISSET(current->fd, &block_write_fdset))
				ready_request(current);
		} else {
			switch (current->status) {
			case PIPE_WRITE:
			case WRITE:
				if (FD_ISSET(current->fd, &block_write_fdset))
					ready_request(current);
				else
					FD_SET(current->fd, &block_write_fdset);
				break;
			case PIPE_READ:
				if (FD_ISSET(current->data_fd, &block_read_fdset))
					ready_request(current);
				else
					FD_SET(current->data_fd, &block_read_fdset);
				break;
			case BODY_WRITE:
				if (FD_ISSET(current->post_data_fd, &block_write_fdset))
					ready_request(current);
				else
					FD_SET(current->post_data_fd, &block_write_fdset);
				break;
			default:
				if (FD_ISSET(current->fd, &block_read_fdset))
					ready_request(current);
				else
					FD_SET(current->fd, &block_read_fdset);
				break;
			}
		}
		current = next;
	}

	if (!lame_duck_mode &&
      (max_connections == -1 || status.connections < max_connections)) {
#ifdef SERVER_SSL
    if (do_sock) {
			FD_SET(server_s, &block_read_fdset);	/* server always set */
		}
#else
		FD_SET(server_s, &block_read_fdset);	/* server always set */
#endif /*SERVER_SSL*/
	} else {
		if (server_s != -1
#ifdef SERVER_SSL
        && do_sock
#endif
        )
			FD_CLR(server_s, &block_read_fdset);
	}

#ifdef SERVER_SSL
	if (do_sock < 2) {
		if (max_connections == -1 || status.connections < max_connections) {
		  FD_SET(server_ssl, &block_read_fdset);
 	   /* printf("Added server_ssl to fdset\n");*/
		} else {
		  FD_CLR(server_ssl, &block_read_fdset);
		}
	}
#endif /*SERVER_SSL*/

	req_timeout.tv_sec = (ka_timeout ? ka_timeout : REQUEST_TIMEOUT);
	req_timeout.tv_usec = 0l;	/* reset timeout */
}

/*
 * Name: die
 * Description: die with fatal error
 */

void die(int exit_code)
{
#ifdef BOA_TIME_LOG
	log_error_time();

	switch (exit_code) {
	case SERVER_ERROR:
		fputs("fatal error: exiting\n", stdout);
		break;
	case OUT_OF_MEMORY:
		perror("malloc");
		break;
	case NO_CREATE_SOCKET:
		perror("socket create");
		break;
	case NO_FCNTL:
		perror("fcntl");
		break;
	case NO_SETSOCKOPT:
		perror("setsockopt");
		break;
	case NO_BIND:
		perror("bind");
		break;
	case NO_LISTEN:
		perror("listen");
		break;
	case NO_SETGID:
		perror("setgid/initgroups");
		break;
	case NO_SETUID:
		perror("setuid");
		break;
	case NO_OPEN_LOG:
		perror("logfile fopen");	/* ??? */
		break;
	case SELECT:
		perror("select");
		break;
	case GETPWUID:
		perror("getpwuid");
		break;
	case INITGROUPS:
		perror("initgroups");
		break;
	case CANNOT_CHROOT:
                perror("chroot");
                break;
	case SHUTDOWN:
		fputs("completing shutdown\n", stderr);
		break;
	default:
		break;
	}
#endif
	syslog(LOG_WARNING, "Shutting down - %d", exit_code);

	fclose(stderr);
	exit(exit_code);
}

#ifdef SERVER_SSL
#ifdef USES_MATRIX_SSL
int
InitSSLStuff(void)
{
 	syslog(LOG_NOTICE, "Enabling SSL security system");
#ifdef IPV6
	if ((server_ssl = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1){
#else
	if ((server_ssl = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
#endif
		syslog(LOG_ALERT,"Couldn't create socket for ssl");
		die(NO_CREATE_SOCKET);
		return 0;
	}

	/* server socket is nonblocking */
	if (fcntl(server_ssl, F_SETFL, NOBLOCK) == -1){
		syslog(LOG_ALERT, "%s, %i:Couldn't fcntl", __FILE__, __LINE__);
		die(NO_FCNTL);
		return 0;
	}
	if ((setsockopt(server_ssl, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
		sizeof(sock_opt))) == -1){
		syslog(LOG_ALERT,"%s, %i:Couldn't sockopt", __FILE__,__LINE__);
		die(NO_SETSOCKOPT);
		return 0;
	}
	/* internet socket */
#ifdef IPV6
	server_sockaddr.sin6_family = AF_INET6;
	memcpy(&server_sockaddr.sin6_addr,&in6addr_any,sizeof(in6addr_any));
	server_sockaddr.sin6_port = htons(ssl_server_port);
#else
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sockaddr.sin_port = htons(ssl_server_port);
#endif

	if (bind(server_ssl, (struct sockaddr *) &server_sockaddr,
		sizeof(server_sockaddr)) == -1){
#ifdef IPV6
		syslog(LOG_ALERT, "Couldn't bind ssl to port %d", ntohs(server_sockaddr.sin6_port));
#else
		syslog(LOG_ALERT, "Couldn't bind ssl to port %d", ntohs(server_sockaddr.sin_port));
#endif
		die(NO_BIND);
		return 0;
	}

	/* listen: large number just in case your kernel is nicely tweaked */
	if (listen(server_ssl, backlog) == -1){
		die(NO_LISTEN);
		return 0;
	}

	if (server_ssl > max_fd)
		max_fd = server_ssl;

	/*Matrix SSL initialization */
	if (matrixSslOpen() < 0) {
		die(NO_SSL);
		fprintf(stderr, "matrixSslOpen failed, exiting...");
		return 0;
	}

	/* read ssl key and cert from file*/
	if (matrixSslReadKeys(&mtrx_keys,SSL_CERTF ,SSL_KEYF , NULL, NULL) < 0)  {
		fprintf(stderr, "Error reading or parsing %s or %s.\n",
			SSL_CERTF, SSL_KEYF);
		return 0;
	}

	return 1;
}

#else /*!USES_MATRIX_SSL*/
int
InitSSLStuff(void)
{
	syslog(LOG_NOTICE, "Enabling SSL security system");
#ifdef IPV6
	if ((server_ssl = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		die(NO_CREATE_SOCKET);
		return 0;
	}
#else
	if ((server_ssl = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
		syslog(LOG_ALERT,"Couldn't create socket for ssl");
		die(NO_CREATE_SOCKET);
		return 0;
	}
#endif /*IPV6*/

	/* server socket is nonblocking */
	if (fcntl(server_ssl, F_SETFL, NOBLOCK) == -1){
		syslog(LOG_ALERT, "%s, %i:Couldn't fcntl", __FILE__, __LINE__);
		die(NO_FCNTL);
		return 0;
	}

	if ((setsockopt(server_ssl, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
		sizeof(sock_opt))) == -1){
		syslog(LOG_ALERT,"%s, %i:Couldn't sockopt", __FILE__,__LINE__);
		die(NO_SETSOCKOPT);
		return 0;
	}

	/* internet socket */
#ifdef IPV6
	server_sockaddr.sin6_family = AF_INET6;
	memcpy(&server_sockaddr.sin6_addr,&in6addr_any,sizeof(in6addr_any));
	server_sockaddr.sin6_port = htons(ssl_server_port);
#else
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sockaddr.sin_port = htons(ssl_server_port);
#endif

	if (bind(server_ssl, (struct sockaddr *) &server_sockaddr,
		sizeof(server_sockaddr)) == -1){
#ifdef IPV6
		syslog(LOG_ALERT, "Couldn't bind ssl to port %d", ntohs(server_sockaddr.sin6_port));
#else
		syslog(LOG_ALERT, "Couldn't bind ssl to port %d", ntohs(server_sockaddr.sin_port));
#endif
		die(NO_BIND);
		return 0;
	}

	/* listen: large number just in case your kernel is nicely tweaked */
	if (listen(server_ssl, backlog) == -1){
		die(NO_LISTEN);
		return 0;
	}

	if (server_ssl > max_fd)
		max_fd = server_ssl;

	/*Init all of the ssl stuff*/
//	i don't know why this line is commented out... i found it like that - damion may-02
/*	SSL_load_error_strings();*/
	SSLeay_add_ssl_algorithms();
	meth = SSLv23_server_method();
	if(meth == NULL){
		ERR_print_errors_fp(stderr);
		syslog(LOG_ALERT, "Couldn't create the SSL method");
		die(NO_SSL);
		return 0;
	}
	ctx = SSL_CTX_new(meth);
	if(!ctx){
		syslog(LOG_ALERT, "Couldn't create a connection context\n");
		ERR_print_errors_fp(stderr);
		die(NO_SSL);
		return 0;
	}

	if (SSL_CTX_use_certificate_file(ctx, SSL_CERTF, SSL_FILETYPE_PEM) <= 0) {
		syslog(LOG_ALERT, "Failure reading SSL certificate file: %s",SSL_CERTF);fflush(NULL);
		close(server_ssl);
		return 0;
	}
	syslog(LOG_DEBUG, "Loaded SSL certificate file: %s",SSL_CERTF);fflush(NULL);

	if (SSL_CTX_use_PrivateKey_file(ctx, SSL_KEYF, SSL_FILETYPE_PEM) <= 0) {
		syslog(LOG_ALERT, "Failure reading private key file: %s",SSL_KEYF);fflush(NULL);
		close(server_ssl);
		return 0;
	}
	syslog(LOG_DEBUG, "Opened private key file: %s",SSL_KEYF);fflush(NULL);

	if (!SSL_CTX_check_private_key(ctx)) {
		syslog(LOG_ALERT, "Private key does not match the certificate public key");fflush(NULL);
		close(server_ssl);
		return 0;
	}

	/*load and check that the key files are appropriate.*/
	syslog(LOG_NOTICE,"SSL security system enabled");
	return 1;
}
#endif /* USES_MATRIX_SSL */
#endif /*SERVER_SSL*/
