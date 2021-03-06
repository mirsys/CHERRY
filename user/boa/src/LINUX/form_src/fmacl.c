
/*
 *      Web server handler routines for ACL stuffs
 *
 */


/*-- System inlcude files --*/
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/route.h>

/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "multilang.h"

#ifdef MAC_ACL
int obtainMacAclEntry(request * wp, MIB_CE_ACL_MAC_Tp pEntry)
{
	char	*str;
	char macAddr[18];
	char tmpBuf[100];

	str = boaGetVar(wp, "aclMac", "");
	if (str[0]) {
		int i;
		memcpy(macAddr, str, 17);
		for(i=0;i<17;i++){
			if((i+1)%3 != 0)
				macAddr[i-(i+1)/3] = macAddr[i];
		}
		macAddr[12] = '\0';
		if ( !string_to_hex(macAddr, pEntry->macAddr, 12)) {
			strcpy(tmpBuf, Tinvalid_source_mac);
			goto setErr_route;
		}
	}
	str = boaGetVar(wp, "enablemac", "");
	if ( str[0] == '1' ) {
		pEntry->Enabled = 1;
	}else
		pEntry->Enabled = 0;

	str = boaGetVar(wp, "macinterface", "");
	if ( str[0]=='0' ) {
		pEntry->Interface = IF_DOMAIN_LAN;   // LAN
	} else if ( str[0]=='1') {
		pEntry->Interface = IF_DOMAIN_WAN;   // WAN
	} else {
		strcpy(tmpBuf, strSetInterfaceerror);
		goto setErr_route;
	}
	return 1;

setErr_route:
	ERR_MSG(tmpBuf);
	return 0;
}
#endif

#ifdef IP_ACL
int obtainIpAclEntry(request * wp, MIB_CE_ACL_IP_Tp pEntry)
{
	char	*str;
	char tmpBuf[100];
	int isnet;
	struct sockaddr acl_ip;
	unsigned long mask, mbit;
	struct in_addr *addr;
	unsigned char aclmask[4]={0}, aclip[4];
	int totalEntry, i;
	MIB_CE_ACL_IP_T Entry;
	char *temp;
	long nAclip;
	unsigned short uPort;

#ifdef ACL_IP_RANGE	
	struct in_addr curIpAddr,curSubnet,secondIpAddr,secondSubnet;
	char enable;
	unsigned long vstart,vend,v2,v3,v4,v5;
	
	if (pEntry->Interface == IF_DOMAIN_LAN) {
		mib_get( MIB_ADSL_LAN_IP,  (void *)&curIpAddr );
		mib_get( MIB_ADSL_LAN_SUBNET,  (void *)&curSubnet );
		mib_get( MIB_ADSL_LAN_ENABLE_IP2, (void *)&enable );
		mib_get( MIB_ADSL_LAN_IP2, (void *)&secondIpAddr );
		mib_get( MIB_ADSL_LAN_SUBNET2, (void *)&secondSubnet);

		v2 = *((unsigned long *)&curIpAddr);
		v3 = *((unsigned long *)&curSubnet);
		v4 = *((unsigned long *)&secondIpAddr);
		v5 = *((unsigned long *)&secondSubnet);
	}

	str = boaGetVar(wp, "aclstartIP", "");
	if (str[0]){
		if ((isnet = INET_resolve(str, &acl_ip)) < 0) {
			snprintf(tmpBuf, 100, "%s %s", strCantResolve, str);
			goto setErr_route;
		}
		
		vstart = *((unsigned long *)&(((struct sockaddr_in *)&acl_ip)->sin_addr));
		if (pEntry->Interface == IF_DOMAIN_LAN) {
			if ((vstart & v3) != (v2 & v3)) {
				if (enable) {
					if ((vstart & v5) == (v4 & v5)) {
						goto startIP_OK;
					}
				}
				strcpy(tmpBuf, Tinvalid_ip_net);				
				goto setErr_route;
			}
		}
startIP_OK:
		// add into configuration (chain record)
		addr = (struct in_addr *)&(pEntry->startipAddr);
		*addr = ((struct sockaddr_in *)&acl_ip)->sin_addr;
	}

	str = boaGetVar(wp, "aclendIP", "");
	if (str[0]){
		if ((isnet = INET_resolve(str, &acl_ip)) < 0) {
			snprintf(tmpBuf, 100, "%s %s", strCantResolve, str);
			goto setErr_route;
		}
		
		vend = *((unsigned long *)&(((struct sockaddr_in *)&acl_ip)->sin_addr));
		if (pEntry->Interface == IF_DOMAIN_LAN) {
			if ((vend & v3) != (v2 & v3)) {
				if (enable) {
					if ((vend & v5) == (v4 & v5)) {
						printf("\nenter 1!\n");
						if((vend & v5) != (vstart & v5)){
							printf("\nenter 2!\n");
							strcpy(tmpBuf, "Wrong Range IP");
							goto setErr_route;
						}
						goto endIP_OK;
					}
				}
				strcpy(tmpBuf, Tinvalid_ip_net);				
				goto setErr_route;
			}
			if((vend & v3) != (vstart & v3)){
				strcpy(tmpBuf, "Wrong Range IP");
				goto setErr_route;
			}
		}
endIP_OK:
		// add into configuration (chain record)
		addr = (struct in_addr *)&(pEntry->endipAddr);
		*addr = ((struct sockaddr_in *)&acl_ip)->sin_addr;
	}
	
	if((int)(vstart-vend )> 0){
		strcpy(tmpBuf, "Wrong Range IP");
		goto setErr_route;
	}
	
#else
	str = boaGetVar(wp, "aclIP", "");
	if (str[0]){
		if ((isnet = INET_resolve(str, &acl_ip)) < 0) {
			snprintf(tmpBuf, 100, "%s %s", strCantResolve, str);
			goto setErr_route;
		}

		// add into configuration (chain record)
		addr = (struct in_addr *)&(pEntry->ipAddr);
		*addr = ((struct sockaddr_in *)&acl_ip)->sin_addr;
	}

	str = boaGetVar(wp, "aclMask", "");
	if (str[0]) {
		if (!isValidNetmask(str, 0)) {
			strcpy(tmpBuf, strWrongMask);
			goto setErr_route;
		}
		inet_aton(str, (struct in_addr *)aclmask);
		inet_aton(str, (struct in_addr *)&mask);
		mbit=0;
		while (1) {
			if (mask&0x80000000) {
				mbit++;
				mask <<= 1;
			}
			else
				break;
		}
		pEntry->maskbit = mbit;
	}
	// Jenny, for checking duplicated acl IP address
	aclip[0] = pEntry->ipAddr[0] & aclmask[0];
	aclip[1] = pEntry->ipAddr[1] & aclmask[1];
	aclip[2] = pEntry->ipAddr[2] & aclmask[2];
	aclip[3] = pEntry->ipAddr[3] & aclmask[3];

	totalEntry = mib_chain_total(MIB_ACL_IP_TBL);
	temp = inet_ntoa(*((struct in_addr *)aclip));
	nAclip = ntohl(inet_addr(temp));
	for (i=0; i<totalEntry; i++) {
		unsigned long v1, v2, pAclip;
		int m;
		if (!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&Entry)) {
			strcpy(tmpBuf, strGetChainerror);
			goto setErr_route;
		}
		temp[0] = '\0';
		temp = inet_ntoa(*((struct in_addr *)Entry.ipAddr));
		v1 = ntohl(inet_addr(temp));
		v2 = 0xFFFFFFFFL;
		for (m=32; m>Entry.maskbit; m--) {
			v2 <<= 1;
			v2 |= 0x80000000;
		}
		pAclip = v1&v2;
		// If all parameters of Entry are all the same as mew rule, drop this new rule.
		if (nAclip == pAclip && Entry.Interface == pEntry->Interface) {
			strcpy(tmpBuf, Tinvalid_rule);
			goto setErr_route;
		}
	}
#endif

	str = boaGetVar(wp, "enable", "");
	if ( str && str[0] ) {
		pEntry->Enabled = 1;
	}

	str = boaGetVar(wp, "interface", "");
	if ( str[0]=='0' ) {
		pEntry->Interface = IF_DOMAIN_LAN;   // LAN
	} else if ( str[0]=='1') {
		pEntry->Interface = IF_DOMAIN_WAN;   // WAN
	} else {
		strcpy(tmpBuf, strSetInterfaceerror);
		goto setErr_route;
	}
	
	// Add service
	// any
	str = boaGetVar(wp, "l_any", "");
	if (str[0]=='1')
		pEntry->any = 0x01;
	else if (pEntry->Interface == IF_DOMAIN_LAN)
		pEntry->icmp = 0x02;  // LAN side access is always enabled
	
	// telnet
#ifdef CONFIG_USER_TELNETD_TELNETD
	str = boaGetVar(wp, "l_telnet", "");
	if (str[0]=='1')
		pEntry->telnet |= 0x02;
	str = boaGetVar(wp, "w_telnet", "");
	if (str[0]=='1')
		pEntry->telnet |= 0x01;

	uPort = 0;
	str = boaGetVar(wp, "w_telnet_port", "");
	if (str[0]) {
		sscanf(str, "%hu", &uPort);
		pEntry->telnet_port = uPort;
	}
	if (!pEntry->telnet_port)
		pEntry->telnet_port = 23;
#endif

	// ftp
#ifdef CONFIG_USER_FTPD_FTPD
	str = boaGetVar(wp, "l_ftp", "");
	if (str[0]=='1')
		pEntry->ftp |= 0x02;
	str = boaGetVar(wp, "w_ftp", "");
	if (str[0]=='1')
		pEntry->ftp |= 0x01;

	uPort = 0;
	str = boaGetVar(wp, "w_ftp_port", "");
	if (str[0]) {
		sscanf(str, "%hu", &uPort);
		pEntry->ftp_port = uPort;
	}
	if (!pEntry->ftp_port)
		pEntry->ftp_port = 21;
#endif

	// tftp
#ifdef CONFIG_USER_TFTPD_TFTPD
	str = boaGetVar(wp, "l_tftp", "");
	if (str[0]=='1')
		pEntry->tftp |= 0x02;
	str = boaGetVar(wp, "w_tftp", "");
	if (str[0]=='1')
		pEntry->tftp |= 0x01;
#endif

	// web
	str = boaGetVar(wp, "l_web", "");
	if (str[0]=='1')
		pEntry->web |= 0x02;
	str = boaGetVar(wp, "w_web", "");
	if (str[0]=='1')
		pEntry->web |= 0x01;

	uPort = 0;
	str = boaGetVar(wp, "w_web_port", "");
	if (str[0]) {
		sscanf(str, "%hu", &uPort);
		pEntry->web_port = uPort;
	}
	if (!pEntry->web_port)
		pEntry->web_port = 80;

	//https
#ifdef CONFIG_USER_BOA_WITH_SSL
	str = boaGetVar(wp, "l_https", "");
	if (str[0]=='1')
		pEntry->https |= 0x02;
	str = boaGetVar(wp, "w_https", "");
	if (str[0]=='1')
		pEntry->https |= 0x01;

	uPort = 0;
	str = boaGetVar(wp, "w_https_port", "");
	if (str[0]) {
		sscanf(str, "%hu", &uPort);
		pEntry->https_port = uPort;
	}
	if (!pEntry->https_port)
		pEntry->https_port = 443;
#endif //end of CONFIG_USER_BOA_WITH_SSL

	// snmp
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	str = boaGetVar(wp, "l_snmp", "");
	if (str[0]=='1')
		pEntry->snmp |= 0x02;
	str = boaGetVar(wp, "w_snmp", "");
	if (str[0]=='1')
		pEntry->snmp |= 0x01;
#endif

	// ssh
#ifdef CONFIG_USER_SSH_DROPBEAR
	str = boaGetVar(wp, "l_ssh", "");
	if (str[0]=='1')
		pEntry->ssh |= 0x02;
	str = boaGetVar(wp, "w_ssh", "");
	if (str[0]=='1')
		pEntry->ssh |= 0x01;

	uPort = 0;
	str = boaGetVar(wp, "w_ssh_port", "");
	if (str[0]) {
		sscanf(str, "%hu", &uPort);
		pEntry->ssh_port = uPort;
	}
	if (!pEntry->ssh_port)
		pEntry->ssh_port = 22;
#endif

	// icmp
	// LAN side access is always enabled	
	str = boaGetVar(wp, "w_icmp", "");
	if (str[0]=='1')
		pEntry->icmp |= 0x01;
	
	return 1;

setErr_route:
	ERR_MSG(tmpBuf);
	return 0;

}

///////////////////////////////////////////////////////////////////
int LANACLItem(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
#ifndef CONFIG_GENERAL_WEB	
	nBytesSent += boaWrite(wp, "<div ID=\"serviceLANID\" style=\"display:block\">\n");
	nBytesSent += boaWrite(wp, "<table>\n"); 
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=\"table_item\">%s%s</td>\n"
		"<td width=80 align=center class=\"table_item\">%s</td>\n"		
		"</tr>\n", multilang(LANG_SERVICE), multilang(LANG_NAME),  multilang(LANG_LAN));
	
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind>Any</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_any value=1 onClick=serviceChange()></td>\n"
		"</tr>\n");
	nBytesSent += boaWrite(wp, "</table>\n"); 	
	nBytesSent += boaWrite(wp, "</div>\n");
	
	nBytesSent += boaWrite(wp, "<div ID=\"serviceLANID2\" style=\"display:block\">\n");
	nBytesSent += boaWrite(wp, "<table>\n"); 	
#ifdef CONFIG_USER_TELNETD_TELNETD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150align=left class=tdkind>TELNET</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_telnet value=1></td>\n"
		"</tr>\n");
#endif

#ifdef CONFIG_USER_FTPD_FTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind>FTP</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_ftp value=1></td>\n"
		"</tr>\n");
#endif

#ifdef CONFIG_USER_TFTPD_TFTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind>TFTP</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_tftp value=1></td>\n"
		"</tr>\n");
#endif

	nBytesSent += boaWrite(wp, "<tr>\n"
	       "<td width=150 align=left class=tdkind>HTTP</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_web value=1></td>\n"
		"</tr>\n");

#ifdef CONFIG_USER_BOA_WITH_SSL
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind>HTTPS</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_https value=1></td>\n"
		"</tr>\n");
		
#endif //end of CONFIG_USER_BOA_WITH_SSL

#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind>SNMP</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_snmp value=1></td>\n"
		"</tr>\n");
#endif

#ifdef CONFIG_USER_SSH_DROPBEAR
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind>Secure Shell(SSH)</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_ssh value=1></td>\n"
		"</tr>\n");
#endif

	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind>PING</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_icmp value=1 checked disabled></td>\n"
		"</tr>\n");
	nBytesSent += boaWrite(wp, "</table>\n"); 	
	nBytesSent += boaWrite(wp, "</div>\n");	
#else	//CONFIG_GENERAL_WEB
	
	nBytesSent += boaWrite(wp, "<div id=\"div_lan\" class=\"data_vertical data_common_notitle\" style=\"display:block\">\n<div class=\"data_common \">\n");
	nBytesSent += boaWrite(wp, "<table ID=\"serviceLANID\">\n"); 
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<th width=\"30%%\">%s%s</td>\n"
		"<th>%s</td>\n" 	
		"</tr>\n", multilang(LANG_SERVICE), multilang(LANG_NAME),  multilang(LANG_LAN));
	
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">Any</td>\n"
		"<td><input type=checkbox name=l_any value=1 onClick=serviceChange()></td>\n"
		"</tr>\n");
	nBytesSent += boaWrite(wp, "</table>\n");	
//	nBytesSent += boaWrite(wp, "</div>\n</div>");
	
//	nBytesSent += boaWrite(wp, "<div ID=\"serviceLANID2\" class=\"data_vertical data_common_notitle\" style=\"display:block\">\n<div class=\"data_common \">\n");
	nBytesSent += boaWrite(wp, "<table ID=\"serviceLANID2\">\n");	
#ifdef CONFIG_USER_TELNETD_TELNETD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">TELNET</td>\n"
		"<td><input type=checkbox name=l_telnet value=1></td>\n"
		"</tr>\n");
#endif

#ifdef CONFIG_USER_FTPD_FTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">FTP</td>\n"
		"<td><input type=checkbox name=l_ftp value=1></td>\n"
		"</tr>\n");
#endif

#ifdef CONFIG_USER_TFTPD_TFTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">TFTP</td>\n"
		"<td><input type=checkbox name=l_tftp value=1></td>\n"
		"</tr>\n");
#endif

	nBytesSent += boaWrite(wp, "<tr>\n"
		   "<td width=\"30%%\">HTTP</td>\n"
		"<td><input type=checkbox name=l_web value=1></td>\n"
		"</tr>\n");

#ifdef CONFIG_USER_BOA_WITH_SSL
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">HTTPS</td>\n"
		"<td><input type=checkbox name=l_https value=1></td>\n"
		"</tr>\n");
		
#endif //end of CONFIG_USER_BOA_WITH_SSL

#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">SNMP</td>\n"
		"<td><input type=checkbox name=l_snmp value=1></td>\n"
		"</tr>\n");
#endif

#ifdef CONFIG_USER_SSH_DROPBEAR
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">SSH</td>\n"
		"<td><input type=checkbox name=l_ssh value=1></td>\n"
		"</tr>\n");
#endif

	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">PING</td>\n"
		"<td><input type=checkbox name=l_icmp value=1 checked disabled></td>\n"
		"</tr>\n");
	nBytesSent += boaWrite(wp, "</table>\n");	
	nBytesSent += boaWrite(wp, "</div>\n</div>");	
#endif
	return nBytesSent;
}

int WANACLItem(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;
#ifndef CONFIG_GENERAL_WEB	
	nBytesSent += boaWrite(wp, "<div ID=\"serviceWANID\" style=\"display:none\">\n");
	nBytesSent += boaWrite(wp, "<table>\n"); 
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=\"table_item\">%s%s</td>\n"
		"<td width=80 align=center class=\"table_item\">%s</td>\n"
		"<td width=80 align=center class=\"table_item\">%s</td>\n"	
		"</tr>\n", multilang(LANG_SERVICE), multilang(LANG_NAME), multilang(LANG_WAN), multilang(LANG_WAN_PORT));
		
#ifdef CONFIG_USER_TELNETD_TELNETD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind>TELNET</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_telnet value=1></td>\n"
		"<td width=80 align=center><input type=text size=5 name=w_telnet_port value=23></td>\n</tr>\n");
#endif

#ifdef CONFIG_USER_FTPD_FTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind>FTP</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_ftp value=1></td>\n"
		"<td width=80 align=center><input type=text size=5 name=w_ftp_port value=21></td>\n</tr>\n");
#endif

#ifdef CONFIG_USER_TFTPD_TFTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind>TFTP</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_tftp value=1></td>\n</tr>\n");
#endif

	nBytesSent += boaWrite(wp, "<tr>\n"
	       "<td width=150 align=left class=tdkind>HTTP</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_web value=1></td>\n"
		"<td width=80 align=center><input type=text size=5 name=w_web_port value=80></td>\n</tr>\n");

#ifdef CONFIG_USER_BOA_WITH_SSL
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind>HTTPS</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_https value=1></td>\n"
		"<td width=80 align=center><input type=text size=5 name=w_https_port value=443></td>\n</tr>\n");
#endif //end of CONFIG_USER_BOA_WITH_SSL

#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind>SNMP</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_snmp value=1></td>\n</tr>\n");
#endif

#ifdef CONFIG_USER_SSH_DROPBEAR
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind>Secure Shell(SSH)</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_ssh value=1></td>\n"
		"<td width=80 align=center><input type=text size=5 name=w_ssh_port value=22></td>\n</tr>\n");
#endif

	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind>PING</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_icmp value=1></td>\n</tr>\n");
	nBytesSent += boaWrite(wp, "</table>\n"); 
	nBytesSent += boaWrite(wp, "</div>\n");
#else	//CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<div ID=\"serviceWANID\" class=\"data_vertical data_common_notitle\" style=\"display:none\">\n<div class=\"data_common \">\n");
	nBytesSent += boaWrite(wp, "<table>\n"); 
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<th width=\"30%%\">%s%s</td>\n"
		"<th>%s</td>\n"
		//"<th>%s</th>\n"	
		"</tr>\n", multilang(LANG_SERVICE), multilang(LANG_NAME), multilang(LANG_WAN));
		
#ifdef CONFIG_USER_TELNETD_TELNETD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">TELNET</td>\n"		
		"<td><input type=checkbox name=w_telnet value=1></td>\n");
		//"<td><input type=text size=5 name=w_telnet_port value=23></td>\n</tr>\n");
#endif

#ifdef CONFIG_USER_FTPD_FTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">FTP</td>\n"		
		"<td><input type=checkbox name=w_ftp value=1></td>\n");
		//"<td><input type=text size=5 name=w_ftp_port value=21></td>\n</tr>\n");
#endif

#ifdef CONFIG_USER_TFTPD_TFTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">TFTP</td>\n"		
		"<td><input type=checkbox name=w_tftp value=1></td>\n");
		//"<td>&nbsp;</td>\n</tr>\n");
#endif

	nBytesSent += boaWrite(wp, "<tr>\n"
	       "<td width=\"30%%\">HTTP</td>\n"		
		"<td><input type=checkbox name=w_web value=1></td>\n");
		//"<td><input type=text size=5 name=w_web_port value=80></td>\n</tr>\n");

#ifdef CONFIG_USER_BOA_WITH_SSL
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">HTTPS</td>\n"		
		"<td><input type=checkbox name=w_https value=1></td>\n");
		//"<td><input type=text size=5 name=w_https_port value=443></td>\n</tr>\n");
#endif //end of CONFIG_USER_BOA_WITH_SSL

#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">SNMP</td>\n"		
		"<td><input type=checkbox name=w_snmp value=1></td>\n");
		//"<td>&nbsp;</td>\n</tr>\n");
#endif

#ifdef CONFIG_USER_SSH_DROPBEAR
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">SSH</td>\n"		
		"<td><input type=checkbox name=w_ssh value=1></td>\n");
		//"<td><input type=text size=5 name=w_ssh_port value=22></td>\n</tr>\n");
#endif

	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">PING</td>\n"		
		"<td><input type=checkbox name=w_icmp value=1></td>\n");
		//"<td>&nbsp;</td>\n</tr>\n");
	nBytesSent += boaWrite(wp, "</table>\n"); 
	nBytesSent += boaWrite(wp, "</div>\n</div>\n");
#endif
	return nBytesSent;
}

void formACL(request * wp, char *path, char *query)
{
	char	*str, *submitUrl, *strVal;
	char tmpBuf[100];
	struct rtentry rt;
	int xflag, isnet;
	int skfd;
	struct sockaddr acl_ip;
	unsigned char aclcap;
	struct in_addr *addr;
	MIB_CE_ACL_IP_T entry;
#ifdef MAC_ACL
	MIB_CE_ACL_MAC_T macEntry;
#endif
#ifndef NO_ACTION
	int pid;
#endif
	unsigned char vChar;

	// Set ACL Capability
	str = boaGetVar(wp, "apply", "");
	if (str[0]) {
		str = boaGetVar(wp, "aclcap", "");
		if (str[0]) {
			if (str[0] == '0')
				aclcap = 0;
			else {
				aclcap = 1;
				if ( !checkACL(tmpBuf, -1, 1))
					goto setErr_route;
			}
			
			if ( !mib_set(MIB_ACL_CAPABILITY, (void *)&aclcap)) {
				strcpy(tmpBuf, strSetACLCAPerror);
				goto setErr_route;
			}
		}
		goto setOk_route;
 	}
	// Delete all IP
	str = boaGetVar(wp, "delAllIP", "");
	if (str[0]) {
		mib_chain_clear(MIB_ACL_IP_TBL); /* clear chain record */
		goto setOk_route;
	}

	/* Delete selected IP */
	str = boaGetVar(wp, "delIP", "");
	if (str[0]) {
		unsigned int i;
		unsigned int idx;
		unsigned int totalEntry = mib_chain_total(MIB_ACL_IP_TBL); /* get chain record size */
		unsigned int deleted = 0;

		for (i=0; i<totalEntry; i++) {

			idx = totalEntry-i-1;
			snprintf(tmpBuf, 20, "select%d", idx);
			strVal = boaGetVar(wp, tmpBuf, "");

			if ( !gstrcmp(strVal, "ON") ) {
				deleted ++;
				if ( !mib_get(MIB_ACL_CAPABILITY, (void *)&aclcap)) {
					strcpy(tmpBuf, strSetACLCAPerror);
					goto setErr_route;
				}
				if (!checkACL(tmpBuf, idx, aclcap)) {
					goto setErr_route;
				}
				if(mib_chain_delete(MIB_ACL_IP_TBL, idx) != 1) {
					strcpy(tmpBuf, Tdelete_chain_error);
					goto setErr_route;
				}
			}
		}
		if (deleted <= 0) {
			strcpy(tmpBuf, strNoItemSelectedToDelete);
			goto setErr_route;
		}

		goto setOk_route;
	}

	//Delete MAC
#ifdef MAC_ACL
	str = boaGetVar(wp, "delMac", "");
	if (str[0]) {
		unsigned int i;
		unsigned int idx;
		MIB_CE_ACL_MAC_T Entry;
		unsigned int totalEntry = mib_chain_total(MIB_ACL_MAC_TBL); /* get chain record size */

		str = boaGetVar(wp, "selectMac", "");

		if (str[0]) {
			for (i=0; i<totalEntry; i++) {
				idx = totalEntry-i-1;
				snprintf(tmpBuf, 4, "ms%d", idx);

				if ( !gstrcmp(str, tmpBuf) ) {
					// delete from chain record
					if(mib_chain_delete(MIB_ACL_MAC_TBL, idx) != 1) {
						strcpy(tmpBuf, Tdelete_chain_error);
						goto setErr_route;
					}
				}
			} // end of for
		}
		goto setOk_route;
	}
#endif

	memset(&entry, '\0', sizeof(MIB_CE_ACL_IP_T));

#ifdef MAC_ACL
	// Update
	str = boaGetVar(wp, "updateMacACL", "");
	if (str[0]) {
		unsigned int i, j;
		unsigned int idx;
		MIB_CE_ACL_MAC_T Entry, tmpEntry;
		unsigned int totalEntry = mib_chain_total(MIB_ACL_MAC_TBL); /* get chain record size */

		memset(&macEntry, 0, sizeof(MIB_CE_ACL_MAC_T));
		if ( !obtainMacAclEntry(wp, &macEntry))
			return;

		str = boaGetVar(wp, "selectMac", "");
		if (str[0]) {
			for (i=0; i<totalEntry; i++) {
				idx = totalEntry-i-1;
				snprintf(tmpBuf, 4, "ms%d", idx);

				if ( !gstrcmp(str, tmpBuf) ) {
					//check if the mac existed.
					for (j=0; j<totalEntry; j++) {
						if (j==idx)
							continue;
						if(!mib_chain_get(MIB_ACL_MAC_TBL, j, (void*)&tmpEntry))
							continue;
						if ( (tmpEntry.macAddr[0]==macEntry.macAddr[0]) &&
							(tmpEntry.macAddr[1]==macEntry.macAddr[1]) &&
							(tmpEntry.macAddr[2]==macEntry.macAddr[2]) &&
							(tmpEntry.macAddr[3]==macEntry.macAddr[3]) &&
							(tmpEntry.macAddr[4]==macEntry.macAddr[4]) &&
							(tmpEntry.macAddr[5]==macEntry.macAddr[5])) {
							snprintf(tmpBuf, 100, strMacExist, str);
							goto setErr_route;
						}
					}

					mib_chain_update(MIB_ACL_MAC_TBL, &macEntry, idx);
					break;
				}
			} // end of for
		}
		goto setOk_route;
	}

	// Add
	str = boaGetVar(wp, "addMac", "");
	if (str[0]) {
		int mibtotal,i, intVal;
		MIB_CE_ACL_MAC_T tmpentry;

		memset(&macEntry, 0, sizeof(MIB_CE_ACL_MAC_T));
		if ( !obtainMacAclEntry(wp, &macEntry))
			return;

		mibtotal = mib_chain_total(MIB_ACL_MAC_TBL);
		for(i=0;i<mibtotal;i++)
		{
			if(!mib_chain_get(MIB_ACL_MAC_TBL,i,(void*)&tmpentry))
				continue;
			if( (tmpentry.macAddr[0]==macEntry.macAddr[0]) &&
				(tmpentry.macAddr[1]==macEntry.macAddr[1]) &&
				(tmpentry.macAddr[2]==macEntry.macAddr[2]) &&
				(tmpentry.macAddr[3]==macEntry.macAddr[3]) &&
				(tmpentry.macAddr[4]==macEntry.macAddr[4]) &&
				(tmpentry.macAddr[5]==macEntry.macAddr[5]) )
			{
				snprintf(tmpBuf, 100, strMacExist, str);
				goto setErr_route;
			}
		}

		intVal = mib_chain_add(MIB_ACL_MAC_TBL, (unsigned char*)&macEntry);
		if (intVal == 0) {
			//boaWrite(wp, "%s", strAddChainerror);
			//return;
			strcpy(tmpBuf, strAddChainerror);
			goto setErr_route;
		}
		else if (intVal == -1) {
			strcpy(tmpBuf, strTableFull);
			goto setErr_route;
		}

		goto setOk_route;
	}
#endif

	// Update
	str = boaGetVar(wp, "updateACL", "");
	if (str[0]) {
		unsigned int i;
		unsigned int idx;
		MIB_CE_ACL_IP_T Entry;
		unsigned int totalEntry = mib_chain_total(MIB_ACL_IP_TBL); /* get chain record size */
		struct in_addr dest;
		char sdest[16];

		memset(&entry, 0, sizeof(MIB_CE_ACL_IP_T));
		if ( !obtainIpAclEntry(wp, &entry))
			return;

		str = boaGetVar(wp, "select", "");

		if (str[0]) {
			for (i=0; i<totalEntry; i++) {
				idx = totalEntry-i-1;
				snprintf(tmpBuf, 4, "s%d", idx);

				if ( !gstrcmp(str, tmpBuf) ) {
					mib_chain_update(MIB_ACL_IP_TBL, &entry, idx);
					//ql_xu add
					break;
				}
			} // end of for
		}

		goto setOk_route;
	}

	// Add
	str = boaGetVar(wp, "addIP", "");
	if (str[0]) {
		int mibtotal,i, intVal;
		MIB_CE_ACL_IP_T tmpentry;

		memset(&entry, 0, sizeof(MIB_CE_ACL_IP_T));
		if ( !obtainIpAclEntry(wp, &entry))
			return;

		mibtotal = mib_chain_total(MIB_ACL_IP_TBL);
		for(i=0;i<mibtotal;i++)
		{
			if(!mib_chain_get(MIB_ACL_IP_TBL,i,(void*)&tmpentry))
				continue;
#ifdef ACL_IP_RANGE
			if(((*(int*)(entry.startipAddr))==(*(int*)(tmpentry.startipAddr)))&&((*(int*)(entry.endipAddr))==(*(int*)(tmpentry.endipAddr))))
#else
			if((*(int*)(entry.ipAddr))==(*(int*)(tmpentry.ipAddr)))
#endif
			{
				snprintf(tmpBuf, 100, strIpExist, str);
				goto setErr_route;
			}
		}

		intVal = mib_chain_add(MIB_ACL_IP_TBL, (unsigned char*)&entry);
		if (intVal == 0) {
			//boaWrite(wp, "%s", strAddChainerror);
			//return;
			strcpy(tmpBuf, strAddChainerror);
			goto setErr_route;
		}
		else if (intVal == -1) {
			strcpy(tmpBuf, strTableFull);
			goto setErr_route;
		}
	}


setOk_route:
#ifdef APPLY_CHANGE
	restart_acl();
#endif

//Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

#ifndef NO_ACTION
	pid = fork();
	if (pid)
		waitpid(pid, NULL, 0);
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _CONFIG_SCRIPT_PROG);
#ifdef HOME_GATEWAY
		execl( tmpBuf, _CONFIG_SCRIPT_PROG, "gw", "bridge", NULL);
#else
		execl( tmpBuf, _CONFIG_SCRIPT_PROG, "ap", "bridge", NULL);
#endif
		exit(1);
	}
#endif
	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;
setErr_route:
	ERR_MSG(tmpBuf);
}

int showACLTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;

	unsigned int entryNum, i;
	MIB_CE_ACL_IP_T Entry;
#ifndef ACL_IP_RANGE
	struct in_addr dest;
#endif
	char sdest[35];
	char service[128] ="";
	char port[64]="";
	char tmp_port[6]="";
	unsigned char testing_bit;

	entryNum = mib_chain_total(MIB_ACL_IP_TBL);
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr><font size=1>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td></font></tr>\n",	
#else
	nBytesSent += boaWrite(wp, "<tr>"
	"<th width=\"5%%\">%s</th>\n"
	"<th width=\"20%%\">%s</th>\n"
	"<th width=\"20%%\">%s</th>\n"
	"<th width=\"20%%\">%s</th>\n"
	"<th width=\"20%%\">%s</th>\n"
	"<th width=\"20%%\">%s</th></tr>\n",	
#endif
	multilang(LANG_SELECT), multilang(LANG_STATE), multilang(LANG_INTERFACE), multilang(LANG_IP_ADDRESS), multilang(LANG_SERVICES), multilang(LANG_PORT));

	for (i=0; i<entryNum; i++) {
		service[0] ='\0';
		port[0]='\0';

		if (!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, "Get chain record error!\n");
			return -1;
		}

#ifdef ACL_IP_RANGE
		struct in_addr startip,endip;
		char sstart[16],send[16];

		startip.s_addr = *(unsigned long *)Entry.startipAddr;
		endip.s_addr = *(unsigned long *)Entry.endipAddr;

		strcpy(sstart, inet_ntoa(startip));
		strcpy(send, inet_ntoa(endip));
		strcpy(sdest, sstart);
		if(strcmp(sstart,send)){
			strcat(sdest, "-");
			strcat(sdest, send);
		}

#else
		dest.s_addr = *(unsigned long *)Entry.ipAddr;

		// inet_ntoa is not reentrant, we have to
		// copy the static memory before reuse it
		snprintf(sdest, sizeof(sdest), "%s/%d", inet_ntoa(dest), Entry.maskbit);
#endif
		testing_bit = (Entry.Interface == IF_DOMAIN_WAN) ? 0x01 : 0x02;

		if (Entry.any == 0x01) { 	// service == any(0x01)
			strcat(service, "any");
			strcat(port, "--");
		} else {
#ifndef CONFIG_GENERAL_WEB
			#ifdef CONFIG_USER_TELNETD_TELNETD		
			if (Entry.telnet & testing_bit) {			
				strcat(service, "telnet,");
				if (Entry.Interface == IF_DOMAIN_LAN)
					strcat(port, "23,");
				else {
					snprintf(tmp_port, sizeof(tmp_port), "%hu,", Entry.telnet_port);
					strcat(port, tmp_port);
				}
			}
			#endif
			#ifdef CONFIG_USER_FTPD_FTPD
			if (Entry.ftp & testing_bit) {			
				strcat(service, "ftp,");
				if (Entry.Interface == IF_DOMAIN_LAN)
					strcat(port, "21,");
				else {
					snprintf(tmp_port, sizeof(tmp_port), "%hu,", Entry.ftp_port);
					strcat(port, tmp_port);
				}			
			}
			#endif
			#ifdef CONFIG_USER_TFTPD_TFTPD
			if (Entry.tftp & testing_bit) {			
				strcat(service, "tftp,");
				strcat(port, "69,");
			}
			#endif
			if (Entry.web & testing_bit) {			
				strcat(service, "web,");
				if (Entry.Interface == IF_DOMAIN_LAN)
					strcat(port, "80,");
				else {
					snprintf(tmp_port, sizeof(tmp_port), "%hu,", Entry.web_port);
					strcat(port, tmp_port);
				}
			}
			#ifdef CONFIG_USER_BOA_WITH_SSL
			if (Entry.https & testing_bit) {			
				strcat(service, "https,");
				if (Entry.Interface == IF_DOMAIN_LAN)
					strcat(port, "443,");
				else {
					snprintf(tmp_port, sizeof(tmp_port), "%hu,", Entry.https_port);
					strcat(port, tmp_port);
				}
			}
			#endif //end of CONFIG_USER_BOA_WITH_SSL
			#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
			if (Entry.snmp & testing_bit) {			
				strcat(service, "snmp,");
				strcat(port, "161,162,");
			}
			#endif
			#ifdef CONFIG_USER_SSH_DROPBEAR
			if (Entry.ssh & testing_bit) {			
				strcat(service, "ssh,");
				if (Entry.Interface == IF_DOMAIN_LAN)
					strcat(port, "22,");
				else {
					snprintf(tmp_port, sizeof(tmp_port), "%hu,", Entry.ssh_port);
					strcat(port, tmp_port);
				}
			}
			#endif
			if (Entry.icmp & testing_bit) {			
				strcat(service, "ping,");
			}
#else
			#ifdef CONFIG_USER_TELNETD_TELNETD		
			if (Entry.telnet & testing_bit) {			
				strcat(service, "telnet,");
				strcat(port, "23,");
			}
			#endif
			#ifdef CONFIG_USER_FTPD_FTPD
			if (Entry.ftp & testing_bit) {			
				strcat(service, "ftp,");
				strcat(port, "21,");		
			}
			#endif
			#ifdef CONFIG_USER_TFTPD_TFTPD
			if (Entry.tftp & testing_bit) {			
				strcat(service, "tftp,");
				strcat(port, "69,");
			}
			#endif
			if (Entry.web & testing_bit) {			
				strcat(service, "web,");
				strcat(port, "80,");
			}
			#ifdef CONFIG_USER_BOA_WITH_SSL
			if (Entry.https & testing_bit) {			
				strcat(service, "https,");
				strcat(port, "443,");
			}
			#endif //end of CONFIG_USER_BOA_WITH_SSL
			#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
			if (Entry.snmp & testing_bit) {			
				strcat(service, "snmp,");
				strcat(port, "161,162,");
			}
			#endif
			#ifdef CONFIG_USER_SSH_DROPBEAR
			if (Entry.ssh & testing_bit) {			
				strcat(service, "ssh,");
				strcat(port, "22,");
			}
			#endif
			if (Entry.icmp & testing_bit) {			
				strcat(service, "ping,");
				strcat(port, "--,");
			}
#endif
			
			// trim ',' on service/port string.
			service[strlen(service) - 1] = '\0';
			port[strlen(port) - 1] = '\0';			
		}

		nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>\n"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\" class=\"table_item\">%s</td>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\" class=\"table_item\">%s</td>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\" class=\"table_item\">%s</td>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\" class=\"table_item\">%s</td>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\" class=\"table_item\">%s</td>"
#else
		"<td align=center width=\"5%%\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>\n"
		"<td align=center width=\"20%%\">%s</td>"
		"<td align=center width=\"20%%\">%s</td>"
		"<td align=center width=\"20%%\">%s</td>"
		"<td align=center width=\"20%%\">%s</td>"
		"<td align=center width=\"20%%\">%s</td>"
#endif
		"</tr>\n",
		i,
		multilang(Entry.Enabled ? LANG_ENABLE : LANG_DISABLE), (Entry.Interface == IF_DOMAIN_LAN)? "LAN" : "WAN", sdest, service, port);
	}

	return 0;
}

#ifdef CONFIG_GENERAL_WEB
int checkAclItems(int eid, request * wp, int argc, char **argv)
{
	char *name;
	unsigned char vChar;
	unsigned short vUShort;
	unsigned int vUInt;

   	if (boaArgs(argc, argv, "%s", &name) < 1) {
   		boaError(wp, 400, "Insufficient args\n");
   		return -1;
   	}
	if ( !strcmp(name, "show_input") ) {
#ifdef ACL_IP_RANGE
		boaWrite(wp, "<th>%s %s:</th>\n"
				"<td><input type=\"text\" name=\"aclstartIP\" size=\"15\" maxlength=\"15\"></td>\n"
				"</tr>\n"
				"<tr>\n"
				"<th>%s %s:</th>\n"
				"<td><input type=\"text\" name=\"aclendIP\" size=\"15\" maxlength=\"15\"></td>\n"
				"</tr>\n", multilang(LANG_START), multilang(LANG_IP_ADDRESS), multilang(LANG_END), multilang(LANG_IP_ADDRESS));
#else
		boaWrite(wp, "<th>%s:</th>\n"
				"<td><input type=\"text\" name=\"aclIP\" size=\"15\" maxlength=\"15\"></td>\n"
				"</tr>\n"
				"<tr>\n"
				"<th>%s:</th>\n"
				"<td><input type=\"text\" name=\"aclMask\" size=\"15\" maxlength=\"15\"></td>\n"     
				"</tr>\n", multilang(LANG_IP_ADDRESS), multilang(LANG_SUBNET_MASK));
#endif
		return 0;
	}
	else if(!strcmp(name, "if_ip_valid")) {
#ifdef ACL_IP_RANGE
		boaWrite(wp,"if (!checkNetIP(document.acl.aclstartIP, 1))\n"
			"return false;\n"
			"if (!checkNetIP(document.acl.aclendIP, 1))\n"
			"return false;\n");
#else
		boaWrite(wp,"if (!checkNetIP(document.acl.aclIP, 1))\n"
				"return false;\n"
				"if (!checkNetmask(document.acl.aclMask, 1))\n"
				"return false;\n");
#endif
		
	}
	else if(!strcmp(name, "if_service_open")) {
		int nBytesSent = 0;
#ifdef CONFIG_USER_TELNETD_TELNETD
	nBytesSent += boaWrite(wp, "_telnet = w_telnet.checked ? 1:0\n");
#endif

#ifdef CONFIG_USER_FTPD_FTPD
	nBytesSent += boaWrite(wp, "_ftp = w_ftp.checked ? 1:0;\n");
#endif

#ifdef CONFIG_USER_TFTPD_TFTPD
	nBytesSent += boaWrite(wp, "_tftp = w_tftp.checked ? 1:0;\n");
#endif

	nBytesSent += boaWrite(wp, "_web = w_web.checked ? 1:0\n");

#ifdef CONFIG_USER_BOA_WITH_SSL
	nBytesSent += boaWrite(wp, "_https = w_https.checked ? 1:0\n");
#endif //end of CONFIG_USER_BOA_WITH_SSL

#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	nBytesSent += boaWrite(wp, "_snmp = w_snmp.checked ? 1:0\n");
#endif

#ifdef CONFIG_USER_SSH_DROPBEAR
	nBytesSent += boaWrite(wp, "_ssh = w_ssh.checked ? 1:0\n");
#endif

	nBytesSent += boaWrite(wp, "_icmp = w_icmp.checked ? 1:0\n");
		
		return 0;
	}
		
}
#endif

#ifdef CONFIG_IPV6
int LANV6ACLItem(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;

#ifndef CONFIG_GENERAL_WEB	
	nBytesSent += boaWrite(wp, "<div ID=\"serviceLANID\" style=\"display:block\">\n");
	nBytesSent += boaWrite(wp, "<table border=0 cellspacing=0 cellpadding=0>\n"); 
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left><font size=2><b>%s%s</b></td>\n"
		"<td width=80 align=center><font size=2><b>%s</b></td>\n"		
		"</tr>\n", multilang(LANG_SERVICE), multilang(LANG_NAME),  multilang(LANG_LAN));
	
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind><font size=2>Any</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_any value=1 onClick=serviceChange()></td>\n"
		"</tr>\n");
	nBytesSent += boaWrite(wp, "</table>\n"); 	
	nBytesSent += boaWrite(wp, "</div>\n");
	
	nBytesSent += boaWrite(wp, "<div ID=\"serviceLANID2\" style=\"display:block\">\n");
	nBytesSent += boaWrite(wp, "<table border=0 cellspacing=0 cellpadding=0>\n"); 	
#ifdef CONFIG_USER_TELNETD_TELNETD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150align=left class=tdkind><font size=2>TELNET</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_telnet value=1></td>\n"
		"</tr>\n");
#endif

#ifdef CONFIG_USER_FTPD_FTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind><font size=2>FTP</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_ftp value=1></td>\n"
		"</tr>\n");
#endif

#ifdef CONFIG_USER_TFTPD_TFTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind><font size=2>TFTP</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_tftp value=1></td>\n"
		"</tr>\n");
#endif

	nBytesSent += boaWrite(wp, "<tr>\n"
	       "<td width=150 align=left class=tdkind><font size=2>HTTP</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_web value=1></td>\n"
		"</tr>\n");

#ifdef CONFIG_USER_BOA_WITH_SSL
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind><font size=2>HTTPS</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_https value=1></td>\n"
		"</tr>\n");
		
#endif //end of CONFIG_USER_BOA_WITH_SSL

#ifdef CONFIG_USER_SSH_DROPBEAR
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind><font size=2>Secure Shell(SSH)</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_ssh value=1></td>\n"
		"</tr>\n");
#endif
	
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind><font size=2>PING</td>\n"
		"<td width=80 align=center><input type=checkbox name=l_icmp value=1 checked disabled></td>\n"
		"</tr>\n");
	nBytesSent += boaWrite(wp, "</table>\n"); 	
	nBytesSent += boaWrite(wp, "</div>\n");	
	
#else //CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<div id=\"div_lan\" class=\"data_vertical data_common_notitle\" style=\"display:block\">\n<div class=\"data_common \">\n");
	nBytesSent += boaWrite(wp, "<table ID=\"serviceLANID\">\n"); 
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<th width=\"30%%\">%s%s</td>\n"
		"<th>%s</td>\n"		
		"</tr>\n", multilang(LANG_SERVICE), multilang(LANG_NAME),  multilang(LANG_LAN));
	
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">Any</td>\n"
		"<td><input type=checkbox name=l_any value=1 onClick=serviceChange()></td>\n"
		"</tr>\n");
	nBytesSent += boaWrite(wp, "</table>\n"); 	
//	nBytesSent += boaWrite(wp, "</div>\n</div>");
	
//	nBytesSent += boaWrite(wp, "<div ID=\"serviceLANID2\" class=\"data_vertical data_common_notitle\" style=\"display:block\">\n<div class=\"data_common \">\n");
	nBytesSent += boaWrite(wp, "<table ID=\"serviceLANID2\">\n"); 	
#ifdef CONFIG_USER_TELNETD_TELNETD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">TELNET</td>\n"
		"<td><input type=checkbox name=l_telnet value=1></td>\n"
		"</tr>\n");
#endif

#ifdef CONFIG_USER_FTPD_FTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">FTP</td>\n"
		"<td><input type=checkbox name=l_ftp value=1></td>\n"
		"</tr>\n");
#endif

#ifdef CONFIG_USER_TFTPD_TFTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">TFTP</td>\n"
		"<td><input type=checkbox name=l_tftp value=1></td>\n"
		"</tr>\n");
#endif

	nBytesSent += boaWrite(wp, "<tr>\n"
	       "<td width=\"30%%\">HTTP</td>\n"
		"<td><input type=checkbox name=l_web value=1></td>\n"
		"</tr>\n");

#ifdef CONFIG_USER_BOA_WITH_SSL
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">HTTPS</td>\n"
		"<td><input type=checkbox name=l_https value=1></td>\n"
		"</tr>\n");
		
#endif //end of CONFIG_USER_BOA_WITH_SSL

#ifdef CONFIG_USER_SSH_DROPBEAR
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">SSH</td>\n"
		"<td><input type=checkbox name=l_ssh value=1></td>\n"
		"</tr>\n");
#endif
	
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">PING</td>\n"
		"<td><input type=checkbox name=l_icmp value=1 checked disabled></td>\n"
		"</tr>\n");
	nBytesSent += boaWrite(wp, "</table>\n"); 	
	nBytesSent += boaWrite(wp, "</div>\n</div>");	
#endif
	return nBytesSent;
}

int WANV6ACLItem(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent = 0;

#ifndef CONFIG_GENERAL_WEB	
	nBytesSent += boaWrite(wp, "<div ID=\"serviceWANID\" style=\"display:none\">\n");
	nBytesSent += boaWrite(wp, "<table border=0 cellspacing=0 cellpadding=0>\n"); 
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left><font size=2><b>%s%s</b></td>\n"
		"<td width=80 align=center><font size=2><b>%s</b></td>\n"
		"</tr>\n", multilang(LANG_SERVICE), multilang(LANG_NAME),  multilang(LANG_WAN));
		
#ifdef CONFIG_USER_TELNETD_TELNETD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind><font size=2>TELNET</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_telnet value=1></td>\n</tr>\n");
#endif

#ifdef CONFIG_USER_FTPD_FTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind><font size=2>FTP</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_ftp value=1></td>\n</tr>\n");
#endif

#ifdef CONFIG_USER_TFTPD_TFTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind><font size=2>TFTP</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_tftp value=1></td>\n</tr>\n");
#endif

	nBytesSent += boaWrite(wp, "<tr>\n"
	       "<td width=150 align=left class=tdkind><font size=2>HTTP</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_web value=1></td>\n</tr>\n");

#ifdef CONFIG_USER_BOA_WITH_SSL
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind><font size=2>HTTPS</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_https value=1></td>\n</tr>\n");		
#endif //end of CONFIG_USER_BOA_WITH_SSL

#ifdef CONFIG_USER_SSH_DROPBEAR
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind><font size=2>Secure Shell(SSH)</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_ssh value=1></td>\n</tr>\n");
#endif		

	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=150 align=left class=tdkind><font size=2>PING</td>\n"		
		"<td width=80 align=center><input type=checkbox name=w_icmp value=1></td>\n</tr>\n");
	nBytesSent += boaWrite(wp, "</table>\n"); 
	nBytesSent += boaWrite(wp, "</div>\n");
#else	//CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<div ID=\"serviceWANID\" class=\"data_vertical data_common_notitle\" style=\"display:none\">\n<div class=\"data_common \">\n");
	nBytesSent += boaWrite(wp, "<table>\n"); 
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<th width=\"30%%\">%s%s</td>\n"
		"<th >%s</td>\n"
		"</tr>\n", multilang(LANG_SERVICE), multilang(LANG_NAME),  multilang(LANG_WAN));
		
#ifdef CONFIG_USER_TELNETD_TELNETD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">TELNET</td>\n"		
		"<td ><input type=checkbox name=w_telnet value=1></td>\n</tr>\n");
#endif

#ifdef CONFIG_USER_FTPD_FTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">FTP</td>\n"		
		"<td ><input type=checkbox name=w_ftp value=1></td>\n</tr>\n");
#endif

#ifdef CONFIG_USER_TFTPD_TFTPD
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">TFTP</td>\n"		
		"<td ><input type=checkbox name=w_tftp value=1></td>\n</tr>\n");
#endif

	nBytesSent += boaWrite(wp, "<tr>\n"
	       "<td width=\"30%%\">HTTP</td>\n"		
		"<td ><input type=checkbox name=w_web value=1></td>\n</tr>\n");

#ifdef CONFIG_USER_BOA_WITH_SSL
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">HTTPS</td>\n"		
		"<td ><input type=checkbox name=w_https value=1></td>\n</tr>\n");		
#endif //end of CONFIG_USER_BOA_WITH_SSL

#ifdef CONFIG_USER_SSH_DROPBEAR
	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">SSH</td>\n"		
		"<td ><input type=checkbox name=w_ssh value=1></td>\n</tr>\n");
#endif		

	nBytesSent += boaWrite(wp, "<tr>\n"
		"<td width=\"30%%\">PING</td>\n"		
		"<td ><input type=checkbox name=w_icmp value=1></td>\n</tr>\n");
	nBytesSent += boaWrite(wp, "</table>\n"); 
	nBytesSent += boaWrite(wp, "</div>\n</div>\n");
#endif	//CONFIG_GENERAL_WEB
	return nBytesSent;
}

int obtainIpv6AclEntry(request * wp, MIB_CE_V6_ACL_IP_Tp pEntry)
{
	char *str;
	char tmpBuf[100];
	unsigned int  uPort;

	// Source IP
	str = boaGetVar(wp, "sip6Start", "");
	if (str[0]) {
		if (!inet_pton(PF_INET6, str, &(pEntry->sip6Start))){
			printf("Invalid sip6Start for IPv6 ACL!");
			goto setErr_route;
		}
	}
	
	// source PrefixLen
	str = boaGetVar(wp, "sip6PrefixLen", "");
	if (str[0]) 
		pEntry->sip6PrefixLen = (char)atoi(str);

	str = boaGetVar(wp, "enable", "");
	if ( str && str[0] ) {
		pEntry->Enabled = 1;
	}

	str = boaGetVar(wp, "interface", "");
	if ( str[0]=='0' ) {
		pEntry->Interface = IF_DOMAIN_LAN;   // LAN
	} else if ( str[0]=='1') {
		pEntry->Interface = IF_DOMAIN_WAN;   // WAN
	} else {
		strcpy(tmpBuf, strSetInterfaceerror);
		goto setErr_route;
	}
	
	// Add service
	// any
	str = boaGetVar(wp, "l_any", "");
	if (str[0]=='1')
		pEntry->any = 0x01;
	else if (pEntry->Interface == IF_DOMAIN_LAN)
		pEntry->icmp = 0x02;  // LAN side access is always enabled
	
	// telnet
	#ifdef CONFIG_USER_TELNETD_TELNETD
	str = boaGetVar(wp, "l_telnet", "");
	if (str[0]=='1')
		pEntry->telnet |= 0x02;
	str = boaGetVar(wp, "w_telnet", "");
	if (str[0]=='1')
		pEntry->telnet |= 0x01;
	#endif

	// ftp
	#ifdef CONFIG_USER_FTPD_FTPD
	str = boaGetVar(wp, "l_ftp", "");
	if (str[0]=='1')
		pEntry->ftp |= 0x02;
	str = boaGetVar(wp, "w_ftp", "");
	if (str[0]=='1')
		pEntry->ftp |= 0x01;	
	#endif

	// tftp
	#ifdef CONFIG_USER_TFTPD_TFTPD
	str = boaGetVar(wp, "l_tftp", "");
	if (str[0]=='1')
		pEntry->tftp |= 0x02;
	str = boaGetVar(wp, "w_tftp", "");
	if (str[0]=='1')
		pEntry->tftp |= 0x01;
	#endif

	// web
	str = boaGetVar(wp, "l_web", "");
	if (str[0]=='1')
		pEntry->web |= 0x02;
	str = boaGetVar(wp, "w_web", "");
	if (str[0]=='1')
		pEntry->web |= 0x01;	
	
	//https
	#ifdef CONFIG_USER_BOA_WITH_SSL
	str = boaGetVar(wp, "l_https", "");
	printf("get from web :%s",str);
	if (str[0]=='1')
		pEntry->https |= 0x02;
	str = boaGetVar(wp, "w_https", "");
	printf("get from web :%s",str);
	if (str[0]=='1')
		pEntry->https |= 0x01;		
	#endif //end of CONFIG_USER_BOA_WITH_SSL

	// ssh
	#ifdef CONFIG_USER_SSH_DROPBEAR
	str = boaGetVar(wp, "l_ssh", "");
	if (str[0]=='1')
		pEntry->ssh |= 0x02;
	str = boaGetVar(wp, "w_ssh", "");
	if (str[0]=='1')
		pEntry->ssh |= 0x01;
	#endif
	
	// icmp
	// LAN side access is always enabled	
	str = boaGetVar(wp, "w_icmp", "");
	if (str[0]=='1')
		pEntry->icmp |= 0x01;
	
	return 1;

setErr_route:
	ERR_MSG(tmpBuf);
	return 0;

}

void formV6ACL(request * wp, char *path, char *query)
{
	char *str, *submitUrl, *strVal;
	char tmpBuf[100];
	struct rtentry rt;
	int xflag, isnet;
	int skfd;
	struct sockaddr acl_ip;
	unsigned char aclcap;
	struct in_addr *addr;
	MIB_CE_V6_ACL_IP_T entry;

#ifndef NO_ACTION
	int pid;
#endif
	unsigned char vChar;

	// Set IPv6 ACL Capability
	str = boaGetVar(wp, "apply", "");
	if (str[0]) {
		str = boaGetVar(wp, "aclcap", "");
		if (str[0]) {
			if (str[0] == '0')
				aclcap = 0;
			else {
				aclcap = 1;
			}
			
			if ( !mib_set(MIB_V6_ACL_CAPABILITY, (void *)&aclcap)) {
				strcpy(tmpBuf, strSetACLCAPerror);
				goto setErr_route;
			}						
		}
		goto setOk_route;
 	}
	// Delete all IP
	str = boaGetVar(wp, "delAllIP", "");
	if (str[0]) {
		mib_chain_clear(MIB_V6_ACL_IP_TBL); /* clear chain record */
		goto setOk_route;
	}

	/* Delete selected IP */
	str = boaGetVar(wp, "delIP", "");
	if (str[0]) {
		unsigned int i;
		unsigned int idx;
		unsigned int totalEntry = mib_chain_total(MIB_V6_ACL_IP_TBL); /* get chain record size */
		unsigned int deleted = 0;

		for (i=0; i<totalEntry; i++) {

			idx = totalEntry-i-1;
			snprintf(tmpBuf, 20, "select%d", idx);
			strVal = boaGetVar(wp, tmpBuf, "");

			if ( !gstrcmp(strVal, "ON") ) {
				deleted ++;
				if ( !mib_get(MIB_V6_ACL_CAPABILITY, (void *)&aclcap)) {
					strcpy(tmpBuf, strSetACLCAPerror);
					goto setErr_route;
				}				
				if(mib_chain_delete(MIB_V6_ACL_IP_TBL, idx) != 1) {
					strcpy(tmpBuf, "Delete chain record error!");
					goto setErr_route;
				}
			}
		}
		if (deleted <= 0) {
			strcpy(tmpBuf, "There is no item selected to delete!");
			goto setErr_route;
		}

		goto setOk_route;
	}

	memset(&entry, '\0', sizeof(MIB_CE_V6_ACL_IP_T));

	// Add
	str = boaGetVar(wp, "addIP", "");
	if (str[0]) {
		int mibtotal,i, intVal;
		MIB_CE_V6_ACL_IP_T tmpentry;

		memset(&entry, 0, sizeof(MIB_CE_V6_ACL_IP_T));
		if ( !obtainIpv6AclEntry(wp, &entry))
			return;

		intVal = mib_chain_add(MIB_V6_ACL_IP_TBL, (unsigned char*)&entry);
		if (intVal == 0) {
			strcpy(tmpBuf, strAddChainerror);
			goto setErr_route;
		}
		else if (intVal == -1) {
			strcpy(tmpBuf, strTableFull);
			goto setErr_route;
		}
	}


setOk_route:
	restart_v6_acl();

//Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif

#ifndef NO_ACTION
	pid = fork();
	if (pid)
		waitpid(pid, NULL, 0);
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _CONFIG_SCRIPT_PROG);
#ifdef HOME_GATEWAY
		execl( tmpBuf, _CONFIG_SCRIPT_PROG, "gw", "bridge", NULL);
#else
		execl( tmpBuf, _CONFIG_SCRIPT_PROG, "ap", "bridge", NULL);
#endif
		exit(1);
	}
#endif	
	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);	
	return;
setErr_route:
	ERR_MSG(tmpBuf);
}

int showV6ACLTable(int eid, request * wp, int argc, char **argv)
{
	int nBytesSent=0;

	unsigned int entryNum, i;
	MIB_CE_V6_ACL_IP_T Entry;
	char service[128] ="";
	char port[64]="";
	char tmp_port[6]="";
	char srcip[55];
	
	entryNum = mib_chain_total(MIB_V6_ACL_IP_TBL);
#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr><font size=1>"
	"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td>\n"
	"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td></font></tr>\n",
#else
	nBytesSent += boaWrite(wp, "<tr>"
	"<th align=center width=\"5%%\">%s</th>\n"
	"<th align=center width=\"20%%\">%s</th>\n"
	"<th align=center width=\"20%%\">%s</th>\n"
	"<th align=center width=\"20%%\">%s</th>\n"
	"<th align=center width=\"20%%\">%s</th>\n"
	"<th align=center width=\"20%%\">%s</th></tr>\n",
#endif
	multilang(LANG_SELECT), multilang(LANG_STATE), multilang(LANG_INTERFACE), multilang(LANG_IP_ADDRESS), multilang(LANG_SERVICES), multilang(LANG_PORT));

	for (i=0; i<entryNum; i++) {
		service[0] ='\0';
		port[0]='\0';

		if (!mib_chain_get(MIB_V6_ACL_IP_TBL, i, (void *)&Entry))
		{
  			boaError(wp, 400, "Get chain record error!\n");
			return;
		}		
		
		// source ip, prefixLen
		inet_ntop(PF_INET6, (struct in6_addr *)Entry.sip6Start, srcip, 48);
		if (Entry.sip6PrefixLen!=0)
			snprintf(srcip, sizeof(srcip), "%s/%d", srcip, Entry.sip6PrefixLen);
		
		if (Entry.any == 0x01) { 	// service == any(0x01)
			strcat(service, "any");
			strcat(port, "--");
		}
		else {
			#ifdef CONFIG_USER_TELNETD_TELNETD		
			if ((Entry.telnet & 0x01) || (Entry.telnet & 0x02)) {			
				strcat(service, "telnet,");
				strcat(port, "23,");
			}
			#endif
			#ifdef CONFIG_USER_FTPD_FTPD
			if ((Entry.ftp & 0x01) || (Entry.ftp & 0x02)) {
				strcat(service, "ftp,");
				strcat(port, "21,");
			}
			#endif
			#ifdef CONFIG_USER_TFTPD_TFTPD
			if ((Entry.tftp & 0x01) || (Entry.tftp & 0x02)) {
				strcat(service, "tftp,");
				strcat(port, "69,");
			}
			#endif
			if ((Entry.web & 0x01) || (Entry.web & 0x02)) {
				strcat(service, "web,");
				strcat(port, "80,");
			}
			#ifdef CONFIG_USER_BOA_WITH_SSL
			if ((Entry.https & 0x01) || (Entry.https & 0x02)) {
				strcat(service, "https,");
				strcat(port, "443,");
			}
			#endif
			#ifdef CONFIG_USER_SSH_DROPBEAR
			if ((Entry.ssh & 0x01) || (Entry.ssh & 0x02)) {
				strcat(service, "ssh,");
				strcat(port, "22,");
			}
			#endif
			
			// LAN side access is always enabled
			if (Entry.Interface == IF_DOMAIN_WAN) {
				if (Entry.icmp & 0x01)
					strcat(service, "ping,");
			}
			else
				strcat(service, "ping,");
			
			// trim ',' on service/port string.
			service[strlen(service) - 1] = '\0';
			port[strlen(port) - 1] = '\0';			
		}

		nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
		"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>\n"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\" ><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><b>%s</b></font></td>"
		"</tr>\n",
#else
		"<td align=center width=\"5%%\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>\n"
		"<td align=center width=\"20%%\">%s</td>"
		"<td align=center width=\"20%%\">%s</td>"
		"<td align=center width=\"20%%\">%s</td>"
		"<td align=center width=\"20%%\">%s</td>"
		"<td align=center width=\"20%%\">%s</td>"
		"</tr>\n",
#endif
		i,
		multilang(Entry.Enabled ? LANG_ENABLE : LANG_DISABLE), (Entry.Interface == IF_DOMAIN_LAN)? "LAN" : "WAN", srcip, service, port);
	}

	return 0;
}
#endif
#endif

#ifdef NAT_CONN_LIMIT
int showConnLimitTable(int eid, request * wp, int argc, char * * argv)
{
	int nBytesSent=0;

	unsigned int entryNum, i;
	MIB_CE_CONN_LIMIT_T Entry;
	struct in_addr dest;
	char sdest[16];

	entryNum = mib_chain_total(MIB_CONN_LIMIT_TBL);

#ifndef CONFIG_GENERAL_WEB
	nBytesSent += boaWrite(wp, "<tr><font size=1>"
		"<td align=center width=\"5%%\" bgcolor=\"#808080\">%s</td>\n"
		"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s/td>\n"
		"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s/td>\n"
		"<td align=center width=\"20%%\" bgcolor=\"#808080\">%s</td></font></tr>\n",
#else
	nBytesSent += boaWrite(wp, "<tr>"
		"<th align=center width=\"5%%\">%s</th>\n"
		"<th align=center width=\"20%%\">%s/th>\n"
		"<th align=center width=\"20%%\">%s/th>\n"
		"<th align=center width=\"20%%\">%s</th></tr>\n",
#endif
		multilang(LANG_SELECT), "Status", multilang(LANG_MAX_PORTS), multilang(LANG_IP_ADDRESS));

	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_CONN_LIMIT_TBL, i, (void *)&Entry))
		{
			boaError(wp, 400, "Get chain record error!\n");
			return;
		}

		dest.s_addr = *(unsigned long *)Entry.ipAddr;
		strcpy(sdest, inet_ntoa(dest));

		nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
			"<td align=center width=\"5%%\" bgcolor=\"#C0C0C0\"><input type=\"radio\" name=\"select\""
			" value=\"s%d\""
			" onClick=\"postConn(%d, %d, '%s')\""
			"></td>\n"
			"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\" class=\"table_item\">%s</td>\n"
			"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\" class=\"table_item\">%d</td>\n"
			"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\" class=\"table_item\">%s</td>"
#else			
			"<td align=center width=\"5%%\"><input type=\"radio\" name=\"select\""
			" value=\"s%d\""
			" onClick=\"postConn(%d, %d, '%s')\""
			"></td>\n"
			"<td align=center width=\"20%%\">%s</td>\n"
			"<td align=center width=\"20%%\">%d</td>\n"
			"<td align=center width=\"20%%\">%s</td>"
#endif
			"</tr>\n",
			i,
			Entry.Enabled, Entry.connNum, sdest,
			Entry.Enabled ? multilang(LANG_ENABLED) : multilang(LANG_DISABLED), Entry.connNum, sdest);
	}

	return 0;
}

int connlimitflag=0;
void formConnlimit(request * wp, char *path, char *query)
{
	char	*str, *submitUrl;
	char tmpBuf[100];
	struct sockaddr conn_ip;
	unsigned char connlimitEn;
	MIB_CE_CONN_LIMIT_T entry;
#ifndef NO_ACTION
	int pid;
#endif
	unsigned char vChar;

	// Set nat session num limit Capability
	str = boaGetVar(wp, "apply", "");
	if (str[0]) {
		str = boaGetVar(wp, "enableConnlimit", "");
		if (str[0] == '1')
			connlimitEn = 1;
		else
			connlimitEn = 0;
		if ( !mib_set(MIB_NAT_CONN_LIMIT, (void *)&connlimitEn)) {
			strcpy(tmpBuf, strSetNatSessionError);
			goto setErr_route;
		}
		goto setOk_route;
	}

 	// Delete IP
	str = boaGetVar(wp, "del", "");
	if (str[0]) {
		unsigned int i;
		unsigned int idx;
		unsigned int totalEntry = mib_chain_total(MIB_CONN_LIMIT_TBL); /* get chain record size */

		str = boaGetVar(wp, "select", "");
		if (str[0]) {
			for (i=0; i<totalEntry; i++) {
				idx = totalEntry-i-1;
				snprintf(tmpBuf, 4, "s%d", idx);

				if ( !gstrcmp(str, tmpBuf) ) {
					// delete from chain record
					if(mib_chain_delete(MIB_CONN_LIMIT_TBL, idx) != 1) {
						strcpy(tmpBuf, Tdelete_chain_error);
						goto setErr_route;
					}
				}
			} // end of for
		}
		goto setOk_route;
	}

	// Update
	str = boaGetVar(wp, "update", "");
	if (str[0]) {
		unsigned int i, j;
		unsigned int idx;
		MIB_CE_CONN_LIMIT_T Entry;
		unsigned int totalEntry = mib_chain_total(MIB_CONN_LIMIT_TBL); /* get chain record size */
		struct in_addr dest;
		char sdest[16];

		memset(&entry, 0, sizeof(MIB_CE_CONN_LIMIT_T));
		str = boaGetVar(wp, "IP", "");
		if (str[0]){
			if ( !inet_aton(str, (struct in_addr *)&entry.ipAddr) ) {
				strcpy(tmpBuf, strIPAddresserror);
				goto setErr_route;
			}
		}

		str = boaGetVar(wp, "enable", "");
		if ( str && str[0] ) {
			entry.Enabled = 1;
		}

		str = boaGetVar(wp, "connNum", "");
		if ( str[0] ) {
			unsigned int vUInt;
			sscanf(str, "%u", &vUInt);
			entry.connNum = vUInt;
		}

		str = boaGetVar(wp, "select", "");
		if (str[0]) {
			for (i=0; i<totalEntry; i++) {
				idx = totalEntry-i-1;
				snprintf(tmpBuf, 4, "s%d", idx);

				if ( !gstrcmp(str, tmpBuf) ) {
					//check if the ip has been existed
					for(j=0; j<totalEntry; j++)
					{
						if (j == idx)
							continue;
						if(!mib_chain_get(MIB_CONN_LIMIT_TBL, j, (void*)&Entry))
							continue;

						if((*(int*)(entry.ipAddr))==(*(int*)(Entry.ipAddr)))
						{
							snprintf(tmpBuf, 100, strIpExist, str);
							goto setErr_route;
						}
					}

					mib_chain_update(MIB_CONN_LIMIT_TBL, &entry, idx);
					break;
				}
			} // end of for
		}

		goto setOk_route;
	}

	// Add
	str = boaGetVar(wp, "add", "");
	if (str[0]) {
		int mibtotal,i, intVal;
		MIB_CE_CONN_LIMIT_T tmpentry;

		memset(&entry, 0, sizeof(MIB_CE_CONN_LIMIT_T));
		str = boaGetVar(wp, "IP", "");
		if (str[0]){
			if ( !inet_aton(str, (struct in_addr *)&entry.ipAddr) ) {
				strcpy(tmpBuf, strIPAddresserror);
				goto setErr_route;
			}
		}

		str = boaGetVar(wp, "enable", "");
		if ( str && str[0] ) {
			entry.Enabled = 1;
		}

		str = boaGetVar(wp, "connNum", "");
		if ( str[0] ) {
			unsigned int vUInt;
			sscanf(str, "%u", &vUInt);
			entry.connNum = vUInt;
		}

		mibtotal = mib_chain_total(MIB_CONN_LIMIT_TBL);
		for(i=0;i<mibtotal;i++)
		{
			if(!mib_chain_get(MIB_CONN_LIMIT_TBL,i,(void*)&tmpentry))
				continue;
			if((*(int*)(entry.ipAddr))==(*(int*)(tmpentry.ipAddr)))
			{
				snprintf(tmpBuf, 100, strIpExist, str);
				goto setErr_route;
			}
		}

		intVal = mib_chain_add(MIB_CONN_LIMIT_TBL, (void *)&entry);
		if (intVal == 0) {
			//boaWrite(wp, "%s", strAddChainerror);
			//return;
			strcpy(tmpBuf, strAddChainerror);
			goto setErr_route;
		}
		else if (intVal == -1) {
			strcpy(tmpBuf, strTableFull);
			goto setErr_route;
		}
	}

setOk_route:
	/* upgdate to flash */
	//	mib_update(CURRENT_SETTING);
	connlimitflag= 1;

#ifndef NO_ACTION
	pid = fork();
	if (pid)
		waitpid(pid, NULL, 0);
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _CONFIG_SCRIPT_PROG);
#ifdef HOME_GATEWAY
		execl( tmpBuf, _CONFIG_SCRIPT_PROG, "gw", "bridge", NULL);
#else
		execl( tmpBuf, _CONFIG_SCRIPT_PROG, "ap", "bridge", NULL);
#endif
		exit(1);
	}
#endif

	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;

setEffect_route:
	restart_connlimit();
	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG(submitUrl);
	return;

setErr_route:
	ERR_MSG(tmpBuf);
}

#endif

#ifdef TCP_UDP_CONN_LIMIT
int showConnLimitTable(int eid, request * wp, int argc, char * * argv)
{
	int 						nBytesSent=0;
	unsigned int 				entryNum, i;
	MIB_CE_TCP_UDP_CONN_LIMIT_T 	Entry;
	struct in_addr 			dest;
	char 					sdest[16];

	entryNum = mib_chain_total(MIB_TCP_UDP_CONN_LIMIT_TBL);

	nBytesSent += boaWrite(wp,
#ifndef CONFIG_GENERAL_WEB
		"<tr><td align=center width=\"15%%\" bgcolor=\"#808080\" class=\"table_item\">%s</td>"
		"<td align=center width=\"30%%\" bgcolor=\"#808080\" class=\"table_item\">%s %s</td>"
		"<td align=center width=\"20%%\" bgcolor=\"#808080\" class=\"table_item\">%s</td>"
		"<td align=center width=\"25%%\" bgcolor=\"#808080\" class=\"table_item\">%s</td></tr>",
#else
		"<tr><th align=center width=\"15%%\">%s</th>"
		"<th align=center width=\"30%%\">%s %s</th>"
		"<th align=center width=\"20%%\">%s</th>"
		"<th align=center width=\"25%%\">%s</th></tr>",
#endif
	multilang(LANG_SELECT), multilang(LANG_LOCAL), multilang(LANG_IP_ADDRESS),
	multilang(LANG_PROTOCOL), multilang(LANG_MAX_PORTS));

	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_TCP_UDP_CONN_LIMIT_TBL, i, (void *)&Entry))
		{
			boaError(wp, 400, "Get chain record error!\n");
			return;
		}

		dest.s_addr = *(unsigned long *)Entry.ipAddr;
		strcpy(sdest, inet_ntoa(dest));

		nBytesSent += boaWrite(wp, "<tr>"
#ifndef CONFIG_GENERAL_WEB
			"<td align=center width=\"15%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>\n"
			"<td align=center width=\"30%%\" bgcolor=\"#C0C0C0\" class=\"table_item\">%s</td>\n"
			"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\" class=\"table_item\">%s</td>\n"
			"<td align=center width=\"25%%\" bgcolor=\"#C0C0C0\" class=\"table_item\">%d</td>"
#else	
			"<td align=center width=\"15%%\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>\n"
			"<td align=center width=\"30%%\">%s</td>\n"
			"<td align=center width=\"20%%\">%s</td>\n"
			"<td align=center width=\"25%%\">%d</td>"
#endif
			"</tr>\n",
			i,
			sdest, Entry.protocol? "UDP":"TCP",
			Entry.connNum );

	}
	return 0;
}

int connlimitflag=0;
#define debug
void formConnlimit(request * wp, char *path, char *query)
{
	char							*str, *submitUrl;
	char 						tmpBuf[100];
	struct sockaddr 				conn_ip;
	unsigned char 				connlimitEn;
	MIB_CE_TCP_UDP_CONN_LIMIT_T	entry;
	int							tmpi;
	unsigned char 				vChar;

	// Set nat session num limit Capability
	str = boaGetVar(wp, "apply", "");

	if (str[0]) {
		str = boaGetVar(wp, "connLimitcap", "");

		if (str[0] == '1')
			connlimitEn = 1;
		else
			connlimitEn = 0;
		if ( !mib_set(MIB_CONNLIMIT_ENABLE, (void *)&connlimitEn)) {
			strcpy(tmpBuf, strSetNatSessionError);
			goto setErr_route;
		}
		str = boaGetVar(wp, "tcpconnlimit", "");

		sscanf(str, "%d",&tmpi);
		if (tmpi  < 10000){
			if ( !mib_set(MIB_CONNLIMIT_TCP, (void *)&tmpi)) {
				strcpy(tmpBuf, strSetNatSessionError);
				goto setErr_route;
			}
		}
		str = boaGetVar(wp, "udpconnlimit", "");
		sscanf(str, "%d",&tmpi);
			printf("tmpi: %d", tmpi);
		if (tmpi  < 10000){
			if ( !mib_set(MIB_CONNLIMIT_UDP, (void *)&tmpi)) {
				strcpy(tmpBuf, strSetNatSessionError);
				goto setErr_route;
			}
		}

		goto setEffect_route;
	}

/* Delete entry */
		str = boaGetVar(wp, "deleteSelconnLimit", "");
	if (str[0])
	{
		unsigned int i;
		unsigned int idx;
		unsigned int totalEntry = mib_chain_total(MIB_TCP_UDP_CONN_LIMIT_TBL); /* get chain record size */
		unsigned char *strVal;

		for (i=0; i<totalEntry; i++) {

			idx = totalEntry-i-1;
			snprintf(tmpBuf, 20, "select%d", idx);

			strVal = boaGetVar(wp, tmpBuf, "");
			if ( !gstrcmp(strVal, "ON") ) {

				if(mib_chain_delete(MIB_TCP_UDP_CONN_LIMIT_TBL, idx) != 1) {
					strcpy(tmpBuf, Tdelete_chain_error);
					goto setErr_route;
				}
			}
		}

		goto setEffect_route;
	}
/*delete all entries*/
	str = boaGetVar(wp, "deleteAllconnLimit", "");
	if (str[0])
	{
		mib_chain_clear(MIB_TCP_UDP_CONN_LIMIT_TBL); /* clear chain record */
		goto setEffect_route;
	}

// Add
	str = boaGetVar(wp, "addconnlimit", "");
	if (str[0]) {
		int mibtotal,i, intVal;
		MIB_CE_TCP_UDP_CONN_LIMIT_T tmpentry;

		memset(&entry, 0, sizeof(MIB_CE_TCP_UDP_CONN_LIMIT_T));
		//protocol
		str = boaGetVar(wp, "protocol", "");
		entry.protocol = str[0]- '0';
		//ip
		str = boaGetVar(wp, "ip", "");
		if (str[0]){
			struct in_addr curIpAddr, curSubnet;
			unsigned long v1, v2, v3;
			if ( !inet_aton(str, (struct in_addr *)&entry.ipAddr) ) {
				strcpy(tmpBuf, strIPAddresserror);
				goto setErr_route;
			}

			mib_get( MIB_ADSL_LAN_IP,  (void *)&curIpAddr);
			mib_get( MIB_ADSL_LAN_SUBNET,  (void *)&curSubnet);

			v1 = *((unsigned long *)entry.ipAddr);
			v2 = *((unsigned long *)&curIpAddr);
			v3 = *((unsigned long *)&curSubnet);

			if ( (v1 & v3) != (v2 & v3) ) {
				//strcpy(tmpBuf, strInvalidIPAddrShouldInSubnet);
				strcpy(tmpBuf, "error here");
				printf("here(1)\n");
				goto setErr_route;
			}

		}
		else
				goto setOk_route;
/*		str = boaGetVar(wp, "cnlm_enable", "");

		if ( str && str[0] ) {
			entry.Enabled = 1;
		}
*/
		entry.Enabled = 1;
		str = boaGetVar(wp, "connnum", "");
		if ( str[0] ) {
			unsigned int vUInt;
			sscanf(str, "%u", &vUInt);
			entry.connNum = vUInt;
		}
		else
				goto setOk_route;
		mibtotal = mib_chain_total(MIB_TCP_UDP_CONN_LIMIT_TBL);
		for(i=0;i<mibtotal;i++){
			if(!mib_chain_get(MIB_TCP_UDP_CONN_LIMIT_TBL,i,(void*)&tmpentry))
				continue;
			if(((*(int*)(entry.ipAddr))==(*(int*)(tmpentry.ipAddr))) &&
				(entry.protocol == tmpentry.protocol))
			{
				snprintf(tmpBuf, 100, strIpExist, str);
				goto setErr_route;
			}
		}

		intVal = mib_chain_add(MIB_TCP_UDP_CONN_LIMIT_TBL, (void *)&entry);
		if (intVal == 0) {
			//boaWrite(wp, "%s", strAddChainerror);
			//return;
			strcpy(tmpBuf, strAddChainerror);
			goto setErr_route;
		}
		else if (intVal == -1) {
			strcpy(tmpBuf, strTableFull);
			goto setErr_route;
		}
		else
			goto setEffect_route;
	}
setOk_route:
	/* upgdate to flash */
	//	mib_update(CURRENT_SETTING);
	//connlimitflag= 1;

	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;

setEffect_route:
	restart_connlimit();// iptable control goes here
	submitUrl = boaGetVar(wp, "submit-url", "");
	OK_MSG(submitUrl);
	return;

setErr_route:
	ERR_MSG(tmpBuf);

}
 #endif //TCP_UDP_CONN_LIMIT


