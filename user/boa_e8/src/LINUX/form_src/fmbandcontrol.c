/*
 *      Web server handler routines for URL stuffs
 */
#include "options.h"

/*-- System inlcude files --*/
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <net/route.h>

/*-- Local inlcude files --*/
#include "../webs.h"
#include "webform.h"
#include "mib.h"
#include "utility.h"
#include "fmdefs.h"


#ifdef CONFIG_USER_LAN_BANDWIDTH_MONITOR
void formBandwidthMonitor(request * wp, char *path, char *query)
{
	char *submitUrl, *strMonitor, *strInterval;
	unsigned char bandMonitorEnable = 0, oldMode=0;
	unsigned int bandInterval = 0, oldValue=0;
	
	strMonitor = boaGetVar(wp, "monitor", "");
	if ( strMonitor[0] ) {
		bandMonitorEnable = atoi(&strMonitor[0]);
		mib_get(MIB_LANHOST_BANDWIDTH_MONITOR_ENABLE, (void *)&oldMode);		
		if(oldMode != bandMonitorEnable)
			mib_set(MIB_LANHOST_BANDWIDTH_MONITOR_ENABLE, (void *)&bandMonitorEnable);
	}

	strInterval = boaGetVar(wp, "bdw_interval", "");
	if ( strInterval[0] ) {
		bandInterval = atoi(strInterval);
		mib_get(MIB_LANHOST_BANDWIDTH_INTERVAL, (void *)&oldValue);		
		if(oldValue != bandInterval)
			mib_set(MIB_LANHOST_BANDWIDTH_INTERVAL, (void *)&bandInterval);
	}
	
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif	

	submitUrl = boaGetVar(wp, "submit-url", "");
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;	
}
#endif

#ifdef CONFIG_USER_LAN_BANDWIDTH_CONTROL
void formBandWidth(request * wp, char *path, char *query)
{
	char *submitUrl;
	char *stemp = "";
	unsigned char bandcontrolEnable;
	unsigned int act_idx;
	int us_band=0;
	int ds_band=0;
	int chainNum=-1;
	int	lineno = __LINE__;
	MIB_LAN_HOST_BANDWIDTH_T entry;

	clear_maxBandwidth();// clear old bandwidth rate limit

	FETCH_INVALID_OPT(stemp, "action", _NEED);
	if(strcmp(stemp, "sw") == 0)	// switch
	{
		_GET_BOOL(bandcontrolEnable, _NEED);
			
		if(!mib_set(MIB_LANHOST_BANDWIDTH_CONTROL_ENABLE, (void *)&bandcontrolEnable))
		{
			printf("Set Bandwidth Control Enable error!");
			goto check_err;
		}
	}
	else if(strcmp(stemp, "modify") == 0)//modify
	{
		FETCH_INVALID_OPT(stemp, "idx", _NEED);
		act_idx = atoi(stemp);
			
		FETCH_INVALID_OPT(stemp, "usBandwidth", _OPT);
		us_band = atoi(stemp);
		
		FETCH_INVALID_OPT(stemp, "dsBandwidth", _OPT);
		ds_band = atoi(stemp);

		chainNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);
		if(chainNum>act_idx)
		{
			if (!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, act_idx, (void *)&entry)) {
				printf("get bandwidth control chain error!\n");
				goto check_err;
			}
			
			entry.maxUsBandwidth = us_band;
			entry.maxDsBandwidth = ds_band;
			
			mib_chain_update(MIB_LAN_HOST_BANDWIDTH_TBL, (void*)&entry, act_idx);
		}
	}
	else if(strcmp(stemp, "del") == 0)//del
	{
		FETCH_INVALID_OPT(stemp, "idx", _NEED);
		act_idx = atoi(stemp);

		chainNum = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);
		if(chainNum>act_idx)
		{
			mib_chain_delete(MIB_LAN_HOST_BANDWIDTH_TBL, act_idx);
		}
	}
	else if(strcmp(stemp, "add") == 0)//add
	{
		FETCH_INVALID_OPT(stemp, "mac", _NEED);
		changeStringToMac(entry.mac, stemp);

		FETCH_INVALID_OPT(stemp, "us_cfg", _OPT);
		if(stemp)
			entry.maxUsBandwidth = atoi(stemp);
		else
			entry.maxUsBandwidth = 0;
		
		FETCH_INVALID_OPT(stemp, "ds_cfg", _OPT);
		if(stemp)
			entry.maxDsBandwidth = atoi(stemp);
		else
			entry.maxDsBandwidth = 0;

		mib_chain_add(MIB_LAN_HOST_BANDWIDTH_TBL, &entry);
	}else
	{
		goto check_err;
	}

	if(!mib_get(MIB_LANHOST_BANDWIDTH_CONTROL_ENABLE, (void *)&bandcontrolEnable))	
	{
			printf("Set Bandwidth Control Enable error!");
			goto check_err;
	}
	
#ifdef COMMIT_IMMEDIATELY
		Commit();
#endif

	apply_maxBandwidth();
check_err:
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;
}

int initPageBandwidthControl(int eid, request * wp, int argc, char ** argv)
{
	unsigned char bandcontrolEnable;
	MIB_LAN_HOST_BANDWIDTH_T entry;
	unsigned char macString[32]={0};
	
	int total = 0;
	int idx = 0;
	int lineno = __LINE__;

	_TRACE_CALL;

	if (!mib_get(MIB_LANHOST_BANDWIDTH_CONTROL_ENABLE, (void *)&bandcontrolEnable))
		return -1;

	_PUT_BOOL(bandcontrolEnable);

	if(bandcontrolEnable)
	{
		total = mib_chain_total(MIB_LAN_HOST_BANDWIDTH_TBL);
		for(idx = 0; idx < total; idx++)
		{
			if (!mib_chain_get(MIB_LAN_HOST_BANDWIDTH_TBL, idx, (void *)&entry)) {
				printf("get ACCESS RIGHT chain error!\n");
				goto check_err;
			}
			changeMacToString(entry.mac, macString);
			fillcharZeroToMacString(macString);
			boaWrite (wp, "push(new it_nr(\"%d\""_PTS _PTI _PTI"));\n", idx, "mac", macString, "usBandwidth", entry.maxUsBandwidth, "dsBandwidth", entry.maxDsBandwidth);
		}
	}

check_err:
	_TRACE_LEAVEL;
	return 0;
}

#endif //CONFIG_USER_LAN_BANDWIDTH_CONTROL