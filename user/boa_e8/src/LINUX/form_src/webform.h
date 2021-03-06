/*
 *      Include file of form handler
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *      Authors: Dick Tam	<dicktam@realtek.com.tw>
 *
 */


#ifndef _INCLUDE_WEBFORM_H
#define _INCLUDE_WEBFORM_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../globals.h"
#include "../../port.h"
#if HAVE_STDBOOL_H
# include <stdbool.h>
#else
typedef enum {false = 0, true = 1} bool;
#endif

#ifdef EMBED
#include <linux/config.h>
#else
#include "../../../include/linux/autoconf.h"
#endif
#include "options.h"
#include "mib.h"

#include "../defs.h"

#include "boa.h"


#ifdef __i386__
  #define _CONFIG_SCRIPT_PATH "."
  #define _LITTLE_ENDIAN_
#else
  #define _CONFIG_SCRIPT_PATH "/bin"
#endif

//#define _UPMIBFILE_SCRIPT_PROG  "mibfile_run.sh"
#define _CONFIG_SCRIPT_PROG "init.sh"
#define _WLAN_SCRIPT_PROG "wlan.sh"
#define _PPPOE_SCRIPT_PROG "pppoe.sh"
#define _FIREWALL_SCRIPT_PROG	"firewall.sh"
#define _PPPOE_DC_SCRIPT_PROG	"disconnect.sh"

static int __inline__ string_to_hex(char *string, unsigned char *key, int len)
{
	char tmpBuf[4];
	int i, j = 0;

	for (i = 0; i < len; i += 2)
	{
		tmpBuf[0] = string[i];
		tmpBuf[1] = string[i+1];
		tmpBuf[2] = 0;

		if (!isxdigit(tmpBuf[0]) || !isxdigit(tmpBuf[1]))
			return 0;

		key[j++] = (unsigned char)strtol(tmpBuf, (char**)NULL, 16);
	}
	return 1;
}

static int __inline__ string_to_dec(char *string, int *val)
{
	int i;
	int len = strlen(string);

	for (i=0; i<len; i++)
	{
		if (!isdigit(string[i]))
			return 0;
	}

	*val = strtol(string, (char**)NULL, 10);
	return 1;
}

static unsigned int is_backdoor_userlogin(request * wp)
{
	struct user_info * pUser_info;
	pUser_info = search_login_list(wp);
	if(pUser_info->priv == 2) //backdoor user
		return 1;
	return 0;
}


//added by xl_yue
#ifdef USE_LOGINWEB_OF_SERVER
#define ERR_MSG2(msg) { \
	boaHeader(wp); \
	boaWrite(wp, "<head><META http-equiv=content-type content=\"text/html; charset=gbk\"></head>");\
	boaWrite(wp, "<body><blockquote><form>\n"); \
	boaWrite(wp, "<TABLE width=\"100%%\">\n"); \
	boaWrite(wp, "<TR><TD align = middle><h4>%s</h4></TD></TR>\n",msg); \
	boaWrite(wp, "<TR><TD align = middle><input type=\"button\" onclick=\"history.go (-1)\" value=\"  OK  \" name=\"OK\"></TD></TR>"); \
	boaWrite(wp, "</TABLE></form></blockquote></body>\n"); \
	boaFooter(wp); \
	boaDone(wp, 200); \
}
#endif

#define ERR_MSG1(msg, url) { \
	boaHeader(wp); \
	boaWrite(wp, "<head><META http-equiv=content-type content=\"text/html; charset=gbk\"></head>");\
	boaWrite(wp, "<body><blockquote><form>\n"); \
	boaWrite(wp, "<TABLE width=\"100%%\">\n"); \
	boaWrite(wp, "<TR><TD align = middle><h4>%s</h4></TD></TR>\n",msg); \
	boaWrite(wp, "<TR><TD align = middle><input type=\"button\" onclick=window.location.replace(\"%s\") value=\"  OK  \" name=\"OK\"></TD></TR>", url); \
	boaWrite(wp, "</TABLE></form></blockquote></body>\n"); \
	boaFooter(wp); \
	boaDone(wp, 200); \
}

#define ERR_MSG(msg) { \
	boaHeader(wp); \
	boaWrite(wp, "<head><META http-equiv=content-type content=\"text/html; charset=gbk\"></head>");\
	boaWrite(wp, "<body><blockquote><h4>%s</h4>\n", msg); \
	boaWrite(wp, "<form><input type=\"button\" onclick=\"history.go (-1)\" value=\"  OK  \" name=\"OK\"></form></blockquote></body>"); \
	boaFooter(wp); \
	boaDone(wp, 200); \
}

#define OK_MSG(url) { \
	boaHeader(wp); \
	boaWrite(wp, "<head><META http-equiv=content-type content=\"text/html; charset=gbk\"></head>");\
	boaWrite(wp, "<body><blockquote><h4>Change setting successfully!</h4>\n"); \
	if (url[0]) boaWrite(wp, "<form><input type=button value=\"  OK  \" OnClick=window.location.replace(\"%s\")></form></blockquote></body>", url);\
	else boaWrite(wp, "<form><input type=button value=\"  OK  \" OnClick=window.close()></form></blockquote></body>");\
	boaFooter(wp); \
	boaDone(wp, 200); \
}

#define OK_MSG1(msg, url) { \
	boaHeader(wp); \
	boaWrite(wp, "<head><META http-equiv=content-type content=\"text/html; charset=gbk\"></head>");\
	boaWrite(wp, "<body><blockquote><h4>%s</h4>\n", msg); \
	if (url) boaWrite(wp, "<form><input type=button value=\"  OK  \" OnClick=window.location.replace(\"%s\")></form></blockquote></body>", url);\
	else boaWrite(wp, "<form><input type=button value=\"  OK  \" OnClick=window.close()></form></blockquote></body>");\
	boaFooter(wp); \
	boaDone(wp, 200); \
}

#define APPLY_COUNTDOWN_TIME 5
#define OK_MSG_FW(msg, url, c, ip) { \
	boaHeader(wp); \
	boaWrite(wp, "<head><script language=JavaScript><!--\n");\
	boaWrite(wp, "var count = %d;function get_by_id(id){with(document){return getElementById(id);}}\n", c);\
	boaWrite(wp, "function do_count_down(){get_by_id(\"show_sec\").innerHTML = count\n");\
	boaWrite(wp, "if(count == 0) {parent.location.href='http://%s/'; return false;}\n", ip);\
	boaWrite(wp, "if (count > 0) {count--;setTimeout('do_count_down()',1000);}}");\
	boaWrite(wp, "//-->\n");\
	boaWrite(wp,"</script></head>");\
	boaWrite(wp, "<body onload=\"do_count_down();\"><blockquote><h4>%s</h4>\n", msg);\
	boaWrite(wp, "<P align=left><h4>Please wait <B><SPAN id=show_sec></SPAN></B>&nbsp;seconds ...</h4></P>");\
	boaWrite(wp, "</blockquote></body>");\
	boaFooter(wp); \
	boaDone(wp, 200); \
}


/* Routines exported in fmget.c */
// Kaohj
extern int checkWrite(int eid, request * wp, int argc, char **argv);
extern int initPage(int eid, request * wp, int argc, char **argv);
#ifdef WLAN_QoS
extern void ShowWmm(int eid, request * wp, int argc, char **argv);
#endif
extern void write_wladvanced(int eid, request * wp, int argc, char **argv);
extern int getInfo(int eid, request * wp, int argc, char **argv);
extern int gettopstyle(int eid, request * wp, int argc, char **argv);
extern int getNameServer(int eid, request * wp, int argc, char **argv);
extern int getDefaultGWMask(int eid, request * wp, int argc, char **argv);
extern int getDefaultGW(int eid, request * wp, int argc, char **argv);

#ifdef CONFIG_IPV6
extern int getDefaultGW_ipv6(int eid, request * wp, int argc, char **argv);
#endif

extern int srvMenu(int eid, request * wp, int argc, char **argv);

extern void formUploadWapiCert1(request * wp, char * path, char * query);
extern void formUploadWapiCert2(request * wp, char * path, char * query);
extern void formWapiReKey(request * wp, char * path, char * query);


/* Routines exported in fmtcpip.c */
extern void formTcpipLanSetup(request * wp, char *path, char *query);
extern int lan_setting(int eid, request * wp, int argc, char **argv);
extern int checkIP2(int eid, request * wp, int argc, char **argv);
extern int lan_script(int eid, request * wp, int argc, char **argv);
#ifdef WEB_REDIRECT_BY_MAC
void formLanding(request * wp, char *path, char *query);
#endif
//#ifdef CONFIG_USER_PPPOMODEM
extern int wan3GTable(int eid, request * wp, int argc, char **argv);
extern int fm3g_checkWrite(int eid, request * wp, int argc, char **argv);
extern void form3GConf(request * wp, char *path, char *query);

#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
extern int ponGetStatus(int eid, request * wp, int argc, char **argv);
#ifdef CONFIG_GPON_FEATURE
extern int showgpon_status(int eid, request * wp, int argc, char **argv);
#endif

#ifdef CONFIG_EPON_FEATURE
extern int showepon_status(int eid, request * wp, int argc, char **argv);
#endif
#endif

//#endif //CONFIG_USER_PPPOMODEM
extern int wanPPTPTable(int eid, request * wp, int argc, char **argv);
extern int wanL2TPTable(int eid, request * wp, int argc, char **argv);
extern int wanIPIPTable(int eid, request * wp, int argc, char **argv);
#ifdef CONFIG_USER_CUPS
int printerList(int eid, request * wp, int argc, char **argv);
#endif
//#ifdef _CWMP_MIB_
extern void formTR069Config(request * wp, char *path, char *query);
extern void formTR069CPECert(request * wp, char *path, char *query);
extern void formTR069CACert(request * wp, char *path, char *query);
extern void formTR069CACertDel(request * wp, char *path, char *query);
extern void formMidwareConfig(request * wp, char *path, char *query);
extern int TR069ConPageShow(int eid, request * wp, int argc, char **argv);
extern int initCtMDWConfig(int eid, request * wp, int argc, char **argv);
extern int portFwTR069(int eid, request * wp, int argc, char **argv);
extern int TR069DumpCWMP(int eid, request * wp, int argc, char **argv);
//#endif

/* Routines exported in fmfwall.c */
#ifdef PORT_FORWARD_GENERAL
extern void formPortFw(request * wp, char *path, char *query);
#endif
#ifdef NATIP_FORWARDING
extern void formIPFw(request * wp, char *path, char *query);
#endif
#ifdef PORT_TRIGGERING
extern void formGaming(request * wp, char *path, char *query);
#endif
extern void formFilter(request * wp, char *path, char *query);
#ifdef LAYER7_FILTER_SUPPORT
extern int AppFilterList(int eid, request * wp, int argc, char **argv);
extern void formLayer7(request * wp, char *path, char *query);
#endif
#ifdef PARENTAL_CTRL
extern void formParentCtrl(request * wp, char *path, char *query);
extern int parentalCtrlList(int eid, request * wp, int argc, char **argv);
#endif
extern void formDMZ(request * wp, char *path, char *query);
extern int portFwList(int eid, request * wp, int argc, char **argv);
#ifdef NATIP_FORWARDING
extern int ipFwList(int eid, request * wp, int argc, char **argv);
#endif
extern int ipPortFilterList(int eid, request * wp, int argc, char **argv);
#ifdef MAC_FILTER
extern int macFilterList(int eid, request * wp, int argc, char **argv);
#endif

/* Routines exported in fmmgmt.c */
extern void formPasswordSetup(request * wp, char *path, char *query);
extern void formUserPasswordSetup(request * wp, char *path, char *query);
#ifdef ACCOUNT_CONFIG
extern void formAccountConfig(request * wp, char *path, char *query);
extern int accountList(int eid, request * wp, int argc, char **argv);
#endif
#ifdef SUPPORT_WEB_PUSHUP
extern void formUpgradePop(request * wp, char * path, char * query);
extern void formUpgradeRedirect(request * wp, char *path, char *query);
#endif
#ifdef WEB_UPGRADE
extern void formUpload(request * wp, char * path, char * query);
#ifdef CONFIG_DOUBLE_IMAGE
extern void formStopUpload(request * wp, char * path, char * query);
#endif
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT
extern void formSaveWapiCert(request * wp, char *path, char *query);
#endif
extern void formSaveConfig(request * wp, char *path, char *query);
#ifdef CONFIG_USER_SNMPD_SNMPD_V2CTRAP
extern void formSnmpConfig(request * wp, char *path, char *query);
#endif
#ifdef CONFIG_USER_RADVD
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
extern void formlanipv6raconf(request * wp, char *path, char *query);
#else
extern void formRadvdSetup(request * wp, char *path, char *query);
#endif
#endif
extern void formAdslDrv(request * wp, char *path, char *query);
extern void formSetAdsl(request * wp, char *path, char *query);
#ifdef FIELD_TRY_SAFE_MODE
extern void formAdslSafeMode(request * wp, char *path, char *query);
#endif
extern void formDiagAdsl(request * wp, char *path, char *query);
extern void formSetAdslTone(request * wp, char *path, char *query);
extern void formSetAdslPSD(request * wp, char *path, char *query);
extern void formSetAdslPSD(request * wp, char *path, char *query);
extern void formStatAdsl(request * wp, char *path, char *query);
extern void formStats(request * wp, char *path, char *query);
extern void formRconfig(request * wp, char *path, char *query);
extern void formSysCmd(request * wp, char *path, char *query);
extern int sysCmdLog(int eid, request * wp, int argc, char **argv);
#ifdef CONFIG_USER_RTK_SYSLOG
extern void formSysLog(request * wp, char *path, char *query);
extern int sysLogList(int eid, request * wp, int argc, char **argv);
extern void RemoteSyslog(int eid, request * wp, int argc, char **argv);
#endif
#ifdef DOS_SUPPORT
extern void formDosCfg(request * wp, char *path, char *query);
#endif
#if 0
extern int adslDrvSnrTblGraph(int eid, request * wp, int argc, char **argv);
extern int adslDrvSnrTblList(int eid, request * wp, int argc, char **argv);
extern int adslDrvBitloadTblGraph(int eid, request * wp, int argc, char **argv);
extern int adslDrvBitloadTblList(int eid, request * wp, int argc, char **argv);
#endif

extern int adslToneDiagTbl(int eid, request * wp, int argc, char **argv);
extern int adslToneDiagList(int eid, request * wp, int argc, char **argv);
extern int adslToneConfDiagList(int eid, request * wp, int argc, char **argv);
extern int adslPSDMaskTbl(int eid, request * wp, int argc, char **argv);
extern int adslPSDMeasure(int eid, request * wp, int argc, char **argv);
extern int pktStatsList(int eid, request * wp, int argc, char **argv);


/* Routines exported in fmatm.c */
extern int atmVcList(int eid, request * wp, int argc, char **argv);
extern int atmVcList2(int eid, request * wp, int argc, char **argv);
extern int wanConfList(int eid, request * wp, int argc, char **argv);
extern int DSLStatus(int eid, request * wp, int argc, char **argv);
extern int DSLVer(int eid, request * wp, int argc, char **argv);
#ifdef CONFIG_IPV6
extern int wanip6ConfList(int eid, request * wp, int argc, char **argv);
#endif
#if defined(CONFIG_ETHWAN)
extern void formEth(request * wp, char *path, char *query);
#endif
extern void formPPPEdit(request * wp, char *path, char *query);
extern void formIPEdit(request * wp, char *path, char *query);
extern void formBrEdit(request * wp, char *path, char *query);
extern void formStatus(request * wp, char *path, char *query);
#ifdef CONFIG_IPV6
extern void formStatus_ipv6(request * wp, char *path, char *query);
#endif
extern void formDate(request * wp, char *path, char *query);
/* Routines exported in fmbridge.c */
extern void formBridge(request * wp, char *path, char *query);
extern void formRefleshFdbTbl(request * wp, char *path, char *query);
extern int bridgeFdbList(int eid, request * wp, int argc, char **argv);
extern int ARPTableList(int eid, request * wp, int argc, char **argv);

/* Routines exported in fmroute.c */
extern void formRoute(request * wp, char *path, char *query);
#ifdef CONFIG_IPV6
extern void formIPv6EnableDisable(request * wp, char *path, char *query);
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
extern void formIPv6Route(request * wp, char *path, char *query);
#endif
extern void formFilterV6(request * wp, char *path, char *query);
extern int ipPortFilterListV6(int eid, request * wp, int argc, char **argv);
#endif
#if defined(CONFIG_USER_ROUTED_ROUTED) || defined(CONFIG_USER_ZEBRA_OSPFD_OSPFD)
extern void formRip(request * wp, char *path, char *query);
#endif
#ifdef CONFIG_USER_ROUTED_ROUTED
extern int showRipIf(int eid, request * wp, int argc, char **argv);
#endif
#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
extern int showOspfIf(int eid, request * wp, int argc, char **argv);
#endif
extern int ShowDefaultGateway(int eid, request * wp, int argc, char **argv);
extern int GetDefaultGateway(int eid, request * wp, int argc, char **argv);
extern void DisplayDGW(int eid, request * wp, int argc, char **argv);
extern void DisplayTR069WAN(int eid, request * wp, int argc, char **argv);
extern int showStaticRoute(int eid, request * wp, int argc, char **argv);
#ifdef CONFIG_IPV6
extern void formlanipv6dns(request * wp, char *path, char *query);
extern void formlanipv6(request * wp, char *path, char *query);
extern void formlanipv6prefix(request * wp, char *path, char *query);
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
extern int showIPv6StaticRoute(int eid, request * wp, int argc, char **argv);
#endif
extern void formIPv6RefleshRouteTbl(request * wp, char *path, char *query);
extern int routeIPv6List(int eid, request * wp, int argc, char **argv);
#endif
extern void formRefleshRouteTbl(request * wp, char *path, char *query);
extern int routeList(int eid, request * wp, int argc, char **argv);
#ifdef CONFIG_USER_RTK_WAN_CTYPE
extern void ShowConnectionType(int eid, request * wp, int argc, char **argv);
#endif
extern void ShowIpProtocolType(int eid, request * wp, int argc, char **argv);
extern void ShowIPV6Settings(int eid, request * wp, int argc, char **argv);
extern void ShowDSLiteSetting(int eid, request * wp, int argc, char **argv);
extern int ShowPortMapping(int eid, request * wp, int argc, char **argv);
extern void Show6rdSetting(int eid, request * wp, int argc, char **argv);

/* Routines exported in fmdhcpd.c */
extern void formDhcpd(request * wp, char *path, char *query);
extern void formReflashClientTbl(request * wp, char *path, char *query);
extern int dhcpClientList(int eid, request * wp, int argc, char **argv);

#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
extern void formlanipv6dhcp(request * wp, char *path, char *query);
#endif
extern void formDhcpv6(request * wp, char *path, char *query);
extern int showDhcpv6SNameServerTable(int eid, request * wp, int argc, char **argv);
extern int showDhcpv6SDOMAINTable(int eid, request * wp, int argc, char **argv);
#endif

/* Routines exported in fmDNS.c */
extern void formDns(request * wp, char *path, char *query);

/* Routines exported in fmDDNS.c */
extern void formDDNS(request * wp, char *path, char *query);
extern int showDNSTable(int eid, request * wp, int argc, char **argv);


/* Routines exported in fmDNS.c */
extern void formDhcrelay(request * wp, char *path, char *query);

#ifdef ADDRESS_MAPPING
extern void formAddressMap(request * wp, char *path, char *query);
#ifdef MULTI_ADDRESS_MAPPING
extern int showMultAddrMappingTable(int eid, request * wp, int argc, char **argv);
#endif // MULTI_ADDRESS_MAPPING
#endif

/* Routines exported in fmcapture.c */
extern void formCapture(request * wp, char *path, char *query);

/* Routines exported in fmoamlb.c */
extern void formOamLb(request * wp, char *path, char *query);

/* Routines exported in fmreboot.c */
extern void formReboot(request * wp, char *path, char *query);
//xl_yue added,inform itms that maintenance finished
extern void formFinishMaintenance(request * wp, char *path, char *query);
#ifdef USE_LOGINWEB_OF_SERVER
//xl_yue added
extern void formLogin(request * wp, char *path, char *query);
extern void formLogout(request * wp, char *path, char *query);
// Kaohj
extern int passwd2xmit(int eid, request * wp, int argc, char **argv);
#endif


/* Routines exported in fmdhcpmode.c */
extern void formDhcpMode(request * wp, char *path, char *query);

/* Routines exported in fmupnp.c */
extern void formUpnp(request * wp, char *path, char *query);
extern int ifwanList_upnp(int eid, request * wp, int argc, char **argv);
extern void formMLDProxy(request * wp, char *path, char *query);				// Mason Yu. MLD Proxy
extern void formMLDSnooping(request * wp, char *path, char *query);    		// Mason Yu. MLD snooping

/* Routines exported in fmdms.c */
#ifdef CONFIG_USER_MINIDLNA
extern int fmDMS_checkWrite(int eid, request * wp, int argc, char **argv);
extern void formDMSConf(request * wp, char *path, char *query);
#endif

/* Routines exported in fmigmproxy.c */
#ifdef CONFIG_USER_IGMPPROXY
extern void formIgmproxy(request * wp, char *path, char *query);
#endif
extern void formIgmpSnooping(request * wp, char *path, char *query);    // Mason Yu. IGMP snooping for e8b
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
extern void formIgmpMldSnooping(request * wp, char *path, char *query);
extern void formIgmpMldProxy(request * wp, char *path, char *query);
#endif
#ifdef STB_L2_FRAME_LOSS_RATE
extern int initTermInsp(int eid, request *wp, int argc, char **argv);
#endif
extern int ifwanList(int eid, request * wp, int argc, char **argv);

/* Routines exported in fmothers.c */
extern void formOthers(request * wp, char *path, char *query);
extern int diagMenu(int eid, request * wp, int argc, char **argv);
extern int adminMenu(int eid, request * wp, int argc, char **argv);

//xl_yue
extern int userAddAdminMenu(int eid, request * wp, int argc, char **argv);

/* Routines exported in fmsyslog.c*/
//extern void formSysLog(request * wp, char *path, char *query);

#ifdef IP_ACL
/* Routines exported in fmacl.c */
extern void formACL(request * wp, char *path, char *query);
extern int showACLTable(int eid, request * wp, int argc, char **argv);
#endif
#ifdef NAT_CONN_LIMIT
extern int showConnLimitTable(int eid, request * wp, int argc, char **argv);
extern void formConnlimit(request * wp, char *path, char *query);
#endif
#ifdef TCP_UDP_CONN_LIMIT
extern int showConnLimitTable(int eid, request * wp, int argc, char **argv);
extern void formConnlimit(request * wp, char *path, char *query);
#endif

extern void formmacBase(request * wp, char *path, char *query);
#ifdef IMAGENIO_IPTV_SUPPORT
extern void formIpRange(request * wp, char *path, char *query);
#endif
extern int showMACBaseTable(int eid, request * wp, int argc, char **argv);
#ifdef IMAGENIO_IPTV_SUPPORT
extern int showDeviceIpTable(int eid, request * wp, int argc, char **argv);
#endif //#ifdef IMAGENIO_IPTV_SUPPORT

#ifdef URL_BLOCKING_SUPPORT
/* Routines exported in fmurl.c */
extern void formURL(request * wp, char *path, char *query);
extern int showURLTable(int eid, request * wp, int argc, char **argv);
extern int showKeywdTable(int eid, request * wp, int argc, char **argv);
#endif
#ifdef SUPPORT_DNS_FILTER
extern void formDNSFilter(request * wp, char *path, char *query);
#endif
#ifdef URL_ALLOWING_SUPPORT
extern int showURLALLOWTable(int eid, request * wp, int argc, char **argv);
#endif

#ifdef CONFIG_LED_INDICATOR_TIMER
extern void formLedTimer(request * wp, char *path, char *query);
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
/* Routines exported in fmdomainblk.c */
extern void formDOMAINBLK(request * wp, char *path, char *query);
extern int showDOMAINBLKTable(int eid, request * wp, int argc, char **argv);
#endif

#ifdef TIME_ZONE
extern void formNtp(request * wp, char *path, char *query);
extern int timeZoneList(int eid, request * wp, int argc, char **argv);
#endif

extern int vportMenu(int eid, request * wp, int argc, char **argv);

#ifdef ITF_GROUP
//extern void formMpMode(request * wp, char *path, char *query);
/* Routines exported in fmeth2pvc.c */
// Mason Yu. combine_1p_4p_PortMapping
//extern void formEth2pvc(request * wp, char *path, char *query);
//extern int eth2pvcPost(int eid, request * wp, int argc, char **argv);
extern void formItfGroup(request * wp, char *path, char *query);
extern int ifGroupList(int eid, request * wp, int argc, char **argv);
#endif // of ITF_GROUP

#if defined(CONFIG_RTL_MULTI_LAN_DEV)
#ifdef ELAN_LINK_MODE
extern void formLink(request * wp, char *path, char *query);
extern int show_lanport(int eid, request * wp, int argc, char **argv);
#endif
#else // of CONFIG_RTL_MULTI_LAN_DEV
#ifdef ELAN_LINK_MODE_INTRENAL_PHY
extern void formLink(request * wp, char *path, char *query);
#endif
#endif	// of CONFIG_RTL_MULTI_LAN_DEV
//#ifdef IP_QOS
#if defined(IP_QOS) || defined(NEW_IP_QOS_SUPPORT)
extern void formQos(request * wp, char *path, char *query);
extern int default_qos(int eid, request * wp, int argc, char **argv);
extern int qosList(int eid, request * wp, int argc, char **argv);
extern int priority_outif(int eid, request * wp, int argc, char **argv);
extern int confDscp(int eid, request * wp, int argc, char **argv);
#ifdef QOS_DIFFSERV
extern void formDiffServ(request * wp, char *path, char *query);
extern int diffservList(int eid, request * wp, int argc, char **argv);
#endif
#ifdef QOS_DSCP_MATCH
extern int match_dscp(int eid, request * wp, int argc, char **argv);
#endif
extern void formQueueAdd(request * wp, char *path, char *query);
extern int ipQosQueueList(int eid, request * wp, int argc, char **argv);
#endif
#ifdef NEW_IP_QOS_SUPPORT
extern void formQosShape(request * wp, char *path, char *query);
extern int  initTraffictlPage(int eid, request * wp, int argc, char **argv);
#endif
extern int iflanList(int eid, request * wp, int argc, char **argv);
extern int policy_route_outif(int eid, request * wp, int argc, char **argv);

#ifdef CONFIG_8021P_PRIO
extern int setting_1ppriority(int eid, request * wp, int argc, char **argv);
#ifdef NEW_IP_QOS_SUPPORT
extern int setting_predprio(int eid, request * wp, int argc, char **argv);
#endif
#endif
#ifdef CONFIG_USER_PPTP_CLIENT_PPTP
extern void formPPtP(request * wp, char *path, char *query);
extern int pptpWuiList(int eid, request * wp, int argc, char **argv);
extern int pptpGdbusList(int eid, request * wp, int argc, char **argv);
extern int pptpGdbusAttachList(int eid, request * wp, int argc, char **argv);
#endif //end of CONFIG_USER_PPTP_CLIENT_PPTP
#ifdef CONFIG_USER_PPTPD_PPTPD
extern int pptpServerList(int eid, request * wp, int argc, char **argv);
#endif
#ifdef CONFIG_USER_L2TPD_L2TPD
extern void formL2TP(request * wp, char *path, char *query);
extern int l2tpWuiList(int eid, request * wp, int argc, char **argv);
extern int l2tpGdbusList(int eid, request * wp, int argc, char **argv);
extern int l2tpGdbusAttachList(int eid, request * wp, int argc, char **argv);
#endif //end of CONFIG_USER_L2TPD_L2TPD
#ifdef CONFIG_USER_L2TPD_LNS
extern int l2tpServerList(int eid, request * wp, int argc, char **argv);
#endif

#ifdef CONFIG_XFRM
extern void formIPsec(request * wp, char *path, char *query);
extern int ipsec_wanList(int eid, request * wp, int argc, char **argv);
extern int ipsec_ikePropList(int eid, request * wp, int argc, char **argv);
extern int ipsec_saPropList(int eid, request * wp, int argc, char **argv);
extern int ipsec_infoList(int eid, request * wp, int argc, char **argv);
#endif

#ifdef CONFIG_NET_IPIP
extern void formIPIP(request * wp, char *path, char *query);
extern int ipipList(int eid, request * wp, int argc, char **argv);
#endif //end of CONFIG_NET_IPIP
#ifdef CONFIG_USER_IP_QOS_3
extern int  initQosLanif(int eid, request * wp, int argc, char **argv);
#endif

extern void formQosPolicy(request * wp, char *path, char *query);
#ifdef _PRMT_X_CT_COM_DATA_SPEED_LIMIT_
extern int initQosSpeedLimitRule(int eid, request * wp, int argc, char **argv);
extern void formQosSpeedLimit(request * wp, char *path, char *query);
#endif
extern int  initQueuePolicy(int eid, request * wp, int argc, char **argv);
extern int  ifWanList_tc(int eid, request * wp, int argc, char **argv);
extern void formQosTraffictl(request * wp, char *path, char *query);
extern void formQosTraffictlEdit(request * wp, char *path, char *query);
extern int  initTraffictlPage(int eid, request * wp, int argc, char **argv);
extern void formQosRuleEdit(request * wp, char* path, char* query);
extern void formQosAppRule(request * wp, char* path, char* query);
extern void formQosAppRuleEdit(request * wp, char* path, char* query);
extern void formQosRule(request * wp, char* path, char* query);
extern int  initConnType(int eid, request * wp, int argc, char **argv);
extern int  initQosRulePage(int eid, request * wp, int argc, char **argv);
extern int  initQosAppRulePage(int eid, request * wp, int argc, char **argv);
extern int  initRulePriority(int eid, request * wp, int argc, char **argv);
extern int  initOutif(int eid, request * wp, int argc, char **argv);
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
extern void formQosClassficationRuleEdit(request * wp, char* path, char* query);
extern int getQosClassficaitonQueueArray(int eid, request * wp, int argc, char **argv);
extern int getQosTypeQueueArray(int eid, request * wp, int argc, char **argv);
extern int getWANItfArray(int eid, request * wp, int argc, char **argv);
extern int initQosTypeLanif(int eid, request * wp, int argc, char **argv);
extern void formQosVlan(request * wp, char* path, char* query);
#endif
extern void formAcc(request * wp, char *path, char *query);
extern int accPost(int eid, request * wp, int argc, char **argv);
extern int accItem(int eid, request * wp, int argc, char **argv);
extern void ShowAutoPVC(int eid, request * wp, int argc, char **argv);	// auto-pvc-search

extern void ShowChannelMode(int eid, request * wp, int argc, char **argv); // China telecom e8-a
extern void ShowBridgeMode(int eid, request * wp, int argc, char **argv); // For PPPoE pass through
extern void ShowPPPIPSettings(int eid, request * wp, int argc, char **argv); // China telecom e8-a
extern void ShowNAPTSetting(int eid, request * wp, int argc, char **argv); // China telecom e8-a
//#ifdef CONFIG_RTL_MULTI_ETH_WAN
extern void initPageWaneth(int eid, request * wp, int argc, char **argv);
//#endif
#ifdef CONFIG_USER_IGMPPROXY
extern void ShowIGMPSetting(int eid, request * wp, int argc, char **argv); // Telefonica
#endif
// Mason Yu. t123
//extern int createMenu(int eid, request * wp, int argc, char **argv);
//extern int createMenu_user(int eid, request * wp, int argc, char **argv);
extern int createMenuEx (int eid, request * wp, int argc, char ** argv);
extern void formWanRedirect(request * wp, char *path, char *query);
extern void formWanRedirect(request * wp, char *path, char *query);
extern int getWanIfDisplay(int eid, request * wp, int argc, char **argv);
#ifdef WLAN_SUPPORT
extern int wlanMenu(int eid, request * wp, int argc, char **argv);
extern int wlanStatus(int eid, request * wp, int argc, char **argv);
extern int wlan_ssid_select(int eid, request * wp, int argc, char **argv);
extern void formWlanRedirect(request * wp, char *path, char *query);
#if defined(CONFIG_RTL_92D_SUPPORT)
extern void formWlanBand2G5G(request * wp, char *path, char *query);
#endif //CONFIG_RTL_92D_SUPPORT
/* Routines exported in fmwlan.c */
extern void formWlanSetup(request * wp, char *path, char *query);
#ifdef WLAN_ACL
extern int wlAcList(int eid, request * wp, int argc, char **argv);
extern void formWlAc(request * wp, char *path, char *query);
#endif
extern void formAdvanceSetup(request * wp, char *path, char *query);
extern int wirelessClientList(int eid, request * wp, int argc, char **argv);
extern int wirelessClientList2(int eid, request * wp, int argc, char **argv);
extern void formWirelessTbl(request * wp, char *path, char *query);

#ifdef WLAN_WPA
extern void formWlEncrypt(request * wp, char *path, char *query);
#endif
//WDS
#ifdef WLAN_WDS
extern void formWlWds(request * wp, char *path, char *query);
extern void formWdsEncrypt(request * wp, char *path, char *query);
extern int wlWdsList(int eid, request * wp, int argc, char **argv);
extern int wdsList(int eid, request * wp, int argc, char **argv);
#endif
#ifdef WLAN_CLIENT
extern void formWlSiteSurvey(request * wp, char *path, char *query);
extern int wlSiteSurveyTbl(int eid, request * wp, int argc, char **argv);
#endif

#ifdef CONFIG_WIFI_SIMPLE_CONFIG //WPS
extern void formWsc(request * wp, char *path, char *query);
#endif
extern int wlStatus_parm(int eid, request * wp, int argc, char **argv);
extern int wlan_interface_status(int eid, request * wp, int argc, char **argv);
extern void formWifiTimerEx(request * wp, char *path, char *query);
extern void formWifiTimer(request * wp, char *path, char *query);
extern int ShowWifiTimerMask(int eid, request * wp, int argc, char **argv);
#ifdef _PRMT_X_CMCC_WLANSHARE_
extern void formWlanShare(request * wp, char *path, char *query);
#endif
#ifdef WLAN_11R
extern void formFt(request * wp, char *path, char *query);
extern int wlFtKhList(int eid, request * wp, int argc, char **argv);
extern int ShowDot11r(int eid, request * wp, int argc, char **argv);
#endif
#ifdef WLAN_11K
extern int ShowDot11k_v(int eid, request * wp, int argc, char **argv);
#endif
#endif // of WLAN_SUPPORT
extern int oamSelectList(int eid, request * wp, int argc, char **argv);
#ifdef DIAGNOSTIC_TEST
extern void formDiagTest(request * wp, char *path, char *query);	// Diagnostic test
extern int lanTest(int eid, request * wp, int argc, char **argv);	// Ethernet LAN connection test
extern int adslTest(int eid, request * wp, int argc, char **argv);	// ADSL service provider connection test
extern int internetTest(int eid, request * wp, int argc, char **argv);	// Internet service provider connection test
#endif
#ifdef WLAN_MBSSID
extern int wlmbssid_asp(int eid, request * wp, int argc, char **argv);
extern void formWlanMBSSID(request * wp, char *path, char *query);
extern int getVirtualIndex(int eid, request * wp, int argc, char **argv);
extern void formWlanMultipleAP(request * wp, char *path, char *query);
extern int wirelessVAPClientList(int eid, request * wp, int argc, char **argv);
extern void formWirelessVAPTbl(request * wp, char *path, char *query);
#if 0
extern int postSSID(int eid, request * wp, int argc, char **argv);
extern int postSSIDWEP(int eid, request * wp, int argc, char **argv);
#endif
extern int checkSSID(int eid, request * wp, int argc, char **argv);
extern int SSIDStr(int eid, request * wp, int argc, char **argv);
#endif

#ifdef PORT_FORWARD_ADVANCE
extern void formPFWAdvance(request * wp, char *path, char *query);
extern int showPFWAdvTable(int eid, request * wp, int argc, char **argv);
#endif
extern int showPFWAdvForm(int eid, request * wp, int argc, char **argv);
#ifdef WEB_ENABLE_PPP_DEBUG
extern void ShowPPPSyslog(int eid, request * wp, int argc, char **argv);
#endif
extern int multilang_asp(int eid, request * wp, int argc, char **argv);
extern int WANConditions(int eid, request * wp, int argc, char **argv);
extern void set_user_profile(void);

#ifdef CONFIG_USER_ROUTED_ROUTED
extern int ifRipNum(); // fmroute.c
#endif

#ifdef E8B_NEW_DIAGNOSE
extern int createMenuDiag(int eid, request *wp, int argc, char ** argv);
extern int dumpPingInfo(int eid, request *wp, int argc, char **argv);
extern void formPing(request *wp, char *path, char *query);
extern int dumpTraceInfo(int eid, request *wp, int argc, char ** argv);
extern void formTracert(request *wp, char *path, char *query);
extern void formTr069Diagnose(request *wp, char *path, char *query);
#ifdef CONFIG_SUPPORT_AUTO_DIAG
extern void formAutoDiag(request *wp, char *path, char *query);
extern void formQOE(request *wp, char *path, char *query);
#endif
#endif

extern void formVersionMod(request * wp, char *path, char *query);
extern void formExportOMCIlog(request * wp, char *path, char *query);
extern void formImportOMCIShell(request *wp, char *path, char *query);
extern void formTelnetEnable(request * wp, char *path, char *query);
extern void formpktmirrorEnable(request * wp, char *path, char *query);
extern void formPingWAN(request * wp, char *path, char *query);

#ifdef SUPPORT_LOID_BURNING
extern void form_loid_burning(request * wp, char *path, char *query);
#endif

#ifdef CONFIG_YUEME
extern int pluginLogList(int eid, request * wp, int argc, char **argv);
extern int pluginModuleList(int eid, request * wp, int argc, char **argv);
extern int listPlatformService(int eid, request * wp, int argc, char **argv);
#endif

extern const char * const BRIDGE_IF;
extern const char * const ELAN_IF;
//extern const char * const WLAN_IF;

extern int g_remoteConfig;
extern int g_remoteAccessPort;
extern int g_rexpire;
extern short g_rSessionStart;

/* constant string */
static const char DOCUMENT[] = "document";
extern int lanSetting(int eid, request * wp, int argc, char **argv);
extern int getPortMappingInfo(int eid, request * wp, int argc, char **argv);
//cathy, MENU_MEMBER_T should be updated if RootMenu structure is modified
typedef enum {
	MEM_NAME,
	MEM_TYPE,
	MEM_U,
	MEM_TIP,
	MEM_CHILDRENNUMS,
	MEM_EOL,
	MEM_HIDDEN
} MENU_MEMBER_T;

typedef enum {
	MENU_DISPLAY,
	MENU_HIDDEN
} MENU_HIDDEN_T;

typedef enum {
	MENU_FOLDER,
	MENU_URL
} MENU_T;

struct RootMenu{
	char  name[128];
	MENU_T type;
	union {
		void *addr;
		void *url;
	} u;
	char tip[128];
	int childrennums;
	int eol;	// end of layer
	int hidden;
};

#ifdef CONFIG_DEFAULT_WEB
extern struct RootMenu rootmenu[];
extern struct RootMenu rootmenu_user[];
#endif	//CONFIG_DEFAULT_WEB

#if defined(CONFIG_RTL_92D_SUPPORT)
extern void wlanMenuUpdate(struct RootMenu *menu);
#endif //CONFIG_RTL_92D_SUPPORT

//add by ramen for ALG ON-OFF
#ifdef CONFIG_IP_NF_ALG_ONOFF
void formALGOnOff(request * wp, char *path, char *query);
void GetAlgTypes(request * wp);
void initAlgOnOff(request * wp);
void CreatejsAlgTypeStatus(request * wp);
#endif

#ifdef CONFIG_USER_SAMBA
void formSamba(request * wp, char *path, char *query);
#endif

#ifdef CONFIG_USER_RTK_LBD
void formLBD(request * wp, char *path, char *query);
int initLBDPage(int eid, request * wp, int argc, char **argv);
#endif

#ifdef CONFIG_INIT_SCRIPTS
extern void formInitStartScript(request * wp, char *path, char *query);
extern void formInitStartScriptDel(request * wp, char *path, char *query);
extern void formInitEndScript(request * wp, char *path, char *query);
extern void formInitEndScriptDel(request * wp, char *path, char *query);
#endif
int initE8clientUserRegPage(int eid, request * wp, int argc, char ** argv);
int getProvinceInfo(int eid, request * wp, int argc, char ** argv);
int regresultBodyStyle(int eid, request * wp, int argc, char ** argv);
int regresultMainDivStyle(int eid, request * wp, int argc, char ** argv);
int regresultBlankDivStyle(int eid, request * wp, int argc, char ** argv);
int regresultLoginStyle(int eid, request * wp, int argc, char ** argv);
int regresultLoginFontStyle(int eid, request * wp, int argc, char ** argv);
extern void formStorage(request * wp, char *path, char *query);
#endif // _INCLUDE_APFORM_H
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_SEREnable_
extern int showSER(int eid, request * wp, int argc, char **argv);
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_ErrorCodeEnable_
extern int showErrorCode(int eid, request * wp, int argc, char **argv);
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_PLREnable_
extern int showPLR(int eid, request * wp, int argc, char **argv);
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_PacketLostEnable_
extern int showPacketLost(int eid, request * wp, int argc, char **argv);
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterNumberEnable_
extern int showRegisterNumberITMS(int eid, request * wp, int argc, char **argv);
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterSuccessNumberEnable_
extern int showRegisterSuccNumITMS(int eid, request * wp, int argc, char **argv);
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterNumberEnable_
extern int showDHCPRegisterNumber(int eid, request * wp, int argc, char **argv);
extern int showRegisterOLTNumber(int eid, request * wp, int argc, char **argv);
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterSuccessNumberEnable_
extern int showDHCPSuccessNumber(int eid, request * wp, int argc, char **argv);
extern int showRegisterOLTSuccNumber(int eid, request * wp, int argc, char **argv);
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_LANxStateEnable_
extern int showLANxState(int eid, request * wp, int argc, char **argv);
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_UpDataEnable_
extern int showUpData(int eid, request * wp, int argc, char **argv);
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DownDataEnable_
extern int showDownData(int eid, request * wp, int argc, char **argv);
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_LANxWorkBandwidthEnable_
extern int showLANxWorkBandwidth(int eid, request * wp, int argc, char **argv);
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_AllDeviceNumberEnable_
extern int showAllDeviceNumber(int eid, request * wp, int argc, char **argv);
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_WLANDeviceMACEnable_
extern int showWLANDeviceMAC(int eid, request * wp, int argc, char **argv);
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_LANDeviceMACEnable_
extern int showLANDeviceMAC(int eid, request * wp, int argc, char **argv);
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DevicePacketLossEnable_
extern int showDevicePacketLoss(int eid, request * wp, int argc, char **argv);
#endif

#ifdef  _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_CPURateEnable_
extern int showCPURate(int eid, request * wp, int argc, char **argv);
#endif
#ifdef  _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_MemRateEnable_
extern int showMemRate(int eid, request * wp, int argc, char **argv);
#endif

#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DialingNumberEnable_
extern int showDialingNumber(int eid, request * wp, int argc, char **argv);
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DialingErrorEnable_
extern int showDialingError(int eid, request * wp, int argc, char **argv);
#endif
#ifdef  _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_TEMPEnable_
extern int showTEMP(int eid, request * wp, int argc, char **argv);
#endif
#ifdef  _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_TEMPEnable_
extern int showOpticalInPower(int eid, request * wp, int argc, char **argv);
#endif
#ifdef  _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_OpticalOutPowerEnable_
extern int showOpticalOutPower(int eid, request * wp, int argc, char **argv);
#endif
#ifdef  _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RoutingModeEnable_
extern int showRoutingMode(int eid, request * wp, int argc, char **argv);
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterNumberEnable_
extern int showRegisterOLTNumber(int eid, request * wp, int argc, char **argv);
#endif
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterSuccessNumberEnable_
extern int showRegisterOLTSuccNumber(int eid, request * wp, int argc, char **argv);
#endif
#ifdef  _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_MulticastNumberEnable_
extern int showMulticastNumber(int eid, request * wp, int argc, char **argv);
#endif

