
#include <config/autoconf.h>
#include "../webs.h"
//#include "fmdefs.h"
#include "mib.h"
#include "utility.h"
#ifdef CONFIG_RTK_L34_ENABLE
#include "../rtusr_rg_api.h"
#endif

//#ifdef CONFIG_MCAST_VLAN
void formMcastVlanMapping(request * wp, char *path, char *query)
{
	char *strData;
	char *submitUrl;
	char *sWanName;
	int IgmpVlan;
	int IfIndex;
	int ifidx, entryNum, i, chainNum=-1;
	MIB_CE_ATM_VC_T entry;
	
	strData = boaGetVar(wp, "if_index", "");
	ifidx = atoi(strData);
	strData = boaGetVar(wp, "mVlan", "");
	IgmpVlan = atoi(strData);
	if(ifidx == -1 || IgmpVlan ==-1)
		goto back2add;
	sWanName = boaGetVar(wp, "WanName", "");
	IfIndex = getIfIndexByName(sWanName);

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, &entry)) {
			printf("get MIB chain error\n");
			return;
		}
		if(IfIndex == entry.ifIndex){
			printf("%s-%d i=%x\n",__func__,__LINE__,i);
			chainNum = i;
			break;
		}
	}
	entry.mVid = IgmpVlan;
	if(chainNum != -1)
		mib_chain_update(MIB_ATM_VC_TBL, (void *)&entry, chainNum);
//#if defined(CONFIG_MCAST_VLAN) && defined(CONFIG_RTK_L34_ENABLE)
#if defined(CONFIG_RTK_L34_ENABLE)
	RTK_RG_ACL_Flush_mVlan();
	RTK_RG_ACL_Add_mVlan();
	RTK_RG_Flush_IGMP_proxy_ACL_rule();
	RTK_RG_set_IGMP_proxy_ACL_rule();
#ifdef CONFIG_USER_ECMH
	RTK_RG_Flush_MLD_proxy_ACL_rule();
	RTK_RG_set_MLD_proxy_ACL_rule();
#endif
#endif
#if defined(CONFIG_RTL9600_SERIES) && defined(CONFIG_RTK_L34_ENABLE)
	checkVlanConfict();
#endif
#ifdef COMMIT_IMMEDIATELY
	Commit();
#endif
	back2add: /*mean user cancel modify, refresh web page again!*/
	submitUrl = boaGetVar(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		boaRedirect(wp, submitUrl);
	else
		boaDone(wp, 200);
	return;


}
//#endif



