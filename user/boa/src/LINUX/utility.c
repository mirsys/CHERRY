/*
 *      Utiltiy function to communicate with TCPIP stuffs
 *
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *
 *      $Id: utility.c,v 1.845 2013/01/25 07:31:11 kaohj Exp $
 *
 */

/*-- System inlcude files --*/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <pthread.h>

#include <netinet/if_ether.h>
#include <linux/sockios.h>
#ifdef EMBED
#include <linux/config.h>
#endif

/*-- Local include files --*/
#include "mib.h"
#include "utility.h"
#include "adsl_drv.h"

/* for open(), lseek() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>

#include "debug.h"

#include <linux/wireless.h>
#ifdef CONFIG_ATM_CLIP
#include <linux/atmclip.h>
#include <linux/atmarp.h>
#endif

#include <linux/if_bridge.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
// Kaohj -- get_net_link_status() and get_net_link_info()
#include <linux/ethtool.h>
#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_ENABLE)
#include "rtusr_rg_api.h"
#endif

#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
#include "rtk/gpon.h"
#endif

#if defined(CONFIG_RTL8686) || defined(CONFIG_RTL9607C)
#if defined(CONFIG_RTK_L34_ENABLE)
#include <rtk_rg_liteRomeDriver.h>
#else
#include "rtk/ponmac.h"
#include "rtk/gpon.h"
#include "rtk/epon.h"
#endif
#endif

#ifdef CONFIG_HWNAT
#include "hwnat_ioctl.h"
#endif

#ifdef SUPPORT_FON_GRE
#include <linux/netlink.h>
#include <linux/rtnetlink.h>	
#include "rtl_types.h"
#endif

#define PR_VC_START	1
#define	PR_PPP_START	16

#if defined CONFIG_IPV6
#include "ipv6_info.h"
#endif

#ifdef CONFIG_USER_CMD_CLIENT_SIDE
#include "rtk_xdsl_api.h"
#define DSL_CHIP_ID	0
#define DSL_CHIP_PORT 0

#define GFAST_DEBUG(args...) 
//#define GFAST_DEBUG(format, args...) printf("[%s:%d] "format, __FILE__, __LINE__, ##args)

#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
// RLCM_USER_SET_DSL_DATA_PORT case on remote handler
int commserv_setGFast(unsigned int id, void *data){
	int ret = -1;
	struct controller_states ctrld_states;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	ret = rtk_xdsl_get_controllerd_status(h, DSL_CHIP_ID, &ctrld_states, sizeof(struct controller_states));
	if (ret || !(ctrld_states.linkstate)) {
		printf("[%s]commserv is not link up.\n", __FUNCTION__);
		goto err;
	}

	msgToDslPorts setMsg, getMsg;
	memset(&setMsg, 0, sizeof(msgToDslPorts));
	memset(&getMsg, 0, sizeof(msgToDslPorts));
	
	setMsg.ICNr = DSL_CHIP_ID;
	setMsg.PortStart= DSL_CHIP_PORT;
	setMsg.PortEnd = DSL_CHIP_PORT;

	switch(id){
		/******** G. Fast Set case ********/
		//for 1 argument
		case SetVdslBandOff:
		case SetGfastSupportProfile:
			setMsg.message = id;
			((unsigned int*)(setMsg.intVal))[0] = *(unsigned int *)data;
			ret = Set_Gfast_UserData(h, &setMsg, &getMsg, 0);		//flag unuse
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				GFAST_DEBUG("[%s] set support Profile success(0x%x)\n", __FUNCTION__, ((unsigned int*)(setMsg.intVal))[0]);
			}
			break;
		//for 2 argument
		case SetProfile:
		case SetSNRMargin:
			setMsg.message = id;
			((unsigned int*)(setMsg.intVal))[0] = *(unsigned int *)data;
			((unsigned int*)(setMsg.intVal))[1] = *((unsigned int *)data+1);
			ret = Set_Gfast_UserData(h, &setMsg, &getMsg, 0);		//flag unuse
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				GFAST_DEBUG("[%s] set support Profile success(0x%x)\n", __FUNCTION__, ((unsigned int*)(setMsg.intVal))[0]);
			}
			break;
		case SetTest:
			/* temporary for control g.fast phy */
			if(*(unsigned int *)data == 0)
				ret = Set_Gfast_Disable_Ports(h, &setMsg, &getMsg, 0);
			else
				ret = Set_Gfast_Enable_Ports(h, &setMsg, &getMsg, 0);

			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		//unknown argument
		case SetPortPtr:
		case SetVdslFextBandTx: //{Tx0_ToneStart, Tx0_ToneEnd, ...... Tx4_ToneStart, Tx4_ToneEnd}; G.FAST TX does not use these 5 bands
		case SetVdslFextBandRx: //{Rx0_ToneStart, Rx0_ToneEnd, ...... Rx4_ToneStart, Rx4_ToneEnd}; G.FAST RX does not use these 5 bands
		case SetTddMds: //Tdd=36, Mds=28; Mus=36-28-1=7
		case SetLowestCarrier:
		case SetHighestCarrier:
		case SetTddRMCOffset: //Tdd=36, Mds=28; Mus=36-28-1=7
		case SetPMDParam:
		case SetPMSTCParam:
		case SetTrellis:
		case SetDSPversion:
		case SetPortAmin:
		default:
			break;
	}

err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1);	//follow _adsl_drv_get return
}

//RLCM_USER_GET_DSL_DATA_PORT case on remote handler
int commserv_getGFast(unsigned int id, void *data){
	int ret = -1;
	static uint8_t testbuf[12000];
	msgToDslPorts setMsg, getMsg;
	struct controller_states ctrld_states;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	ret = rtk_xdsl_get_controllerd_status(h, DSL_CHIP_ID, &ctrld_states, sizeof(struct controller_states));
	if (ret || !(ctrld_states.linkstate)) {
		printf("[%s]dsl phy is not ready.\n", __FUNCTION__);
		goto err;
	}

	switch(id){
		/******** G. Fast Get case ********/
		case UserGetLineState:
			ret = rtk_gfast_dsp_GetLineState(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);	
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((unsigned int *)data, &((unsigned int *)((msgToDslPorts *)testbuf)->intVal)[1], sizeof(unsigned int));
				GFAST_DEBUG("[%s] case %d, State=%02x\n", __FUNCTION__, id, *(unsigned int *)data);
#if 0				
				switch(*(unsigned int *)data){
					case IDLE_L3:
						printf("IDLE_L3\n");
						break;
					case ACTIVATING:
						printf("ACTIVATING\n");
						break;
					case ITU_HANDSHAKING:
						printf("ITU_HANDSHAKING\n");
						break;
					case INITIALIZING:
						printf("INITIALIZING\n");
						break;
					case SHOWTIME_L0:
						printf("SHOWTIME_L0\n");
						break;
					default:
						printf("Unknown state\n");
						break;
				}
#endif
			}
			break;
		case UserGetGfastSupportProfile:
			ret = rtk_gfast_dsp_GetSupProfile(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((unsigned int *)data, (unsigned int *)((msgToDslPorts *)testbuf)->intVal, sizeof(unsigned int));
				GFAST_DEBUG("[%s] case %d, Supported Profile=%x\n", __FUNCTION__, id, *(unsigned int *)data);
				if(PROFILE_106A & *(unsigned int *)data)
					GFAST_DEBUG("PROFILE_106A\n");
				if(PROFILE_212A & *(unsigned int *)data)
					GFAST_DEBUG("PROFILE_212A\n");
				if(PROFILE_106B & *(unsigned int *)data)
					GFAST_DEBUG("PROFILE_106B\n");
				if(PROFILE_106C & *(unsigned int *)data)
					GFAST_DEBUG("PROFILE_106C\n");
				if(PROFILE_212C & *(unsigned int *)data)
					GFAST_DEBUG("PROFILE_212C\n");
				if(PROFILE_212B & *(unsigned int *)data)
					GFAST_DEBUG("PROFILE_212B\n");
				if(PROFILE_8A_MASK & *(unsigned int *)data)
					GFAST_DEBUG("VDSL2_PROFILE_8A\n");
				if(PROFILE_8B_MASK & *(unsigned int *)data)
					GFAST_DEBUG("VDSL2_PROFILE_8B\n");
				if(PROFILE_8C_MASK & *(unsigned int *)data)
					GFAST_DEBUG("VDSL2_PROFILE_8C\n");
				if(PROFILE_8D_MASK & *(unsigned int *)data)
					GFAST_DEBUG("VDSL2_PROFILE_8D\n");
				if(PROFILE_12A_MASK & *(unsigned int *)data)
					GFAST_DEBUG("VDSL2_PROFILE_12A\n");
				if(PROFILE_12B_MASK & *(unsigned int *)data)
					GFAST_DEBUG("VDSL2_PROFILE_12B\n");
				if(PROFILE_17A_MASK & *(unsigned int *)data)
					GFAST_DEBUG("VDSL2_PROFILE_17A\n");
				if(PROFILE_30A_MASK & *(unsigned int *)data)
					GFAST_DEBUG("VDSL2_PROFILE_30A\n");
				if(PROFILE_35B_MASK & *(unsigned int *)data)
					GFAST_DEBUG("VDSL2_PROFILE_35B\n");
			}
			break;
		case UserGetGfastSelectProfile:
			ret = rtk_gfast_dsp_GetSelProfile(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((unsigned int *)data, (unsigned int *)((msgToDslPorts *)testbuf)->intVal, sizeof(unsigned int));
				GFAST_DEBUG("[%s] case %d, Selected Profile=%x\n", __FUNCTION__, id, *(unsigned int *)data);
				switch(*(unsigned int *)data){
					case PROFILE_106A:
						GFAST_DEBUG("PROFILE_106A\n");
						break;
					case PROFILE_106B:
						GFAST_DEBUG("PROFILE_106B\n");
						break;
					case PROFILE_106C:
						GFAST_DEBUG("PROFILE_106C\n");
						break;
					case PROFILE_212A:
						GFAST_DEBUG("PROFILE_212A\n");
						break;
					case PROFILE_212B:
						GFAST_DEBUG("PROFILE_212B\n");
						break;
					case PROFILE_212C:
						GFAST_DEBUG("PROFILE_212C\n");
						break;
					default:
						GFAST_DEBUG("No supported profile\n");
						break;
				}
			}
			break;
		case UserGetPMDParam:
			ret = rtk_gfast_dsp_GetPmdMode(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((unsigned int *)data, (unsigned int *)((msgToDslPorts *)testbuf)->intVal, sizeof(unsigned int)*6);
				GFAST_DEBUG("[%s] case %d\n", __FUNCTION__, id);
				GFAST_DEBUG("PMD MODE:\n");
				GFAST_DEBUG("Tf: %9d\n", ((unsigned int *)data)[0]);
				GFAST_DEBUG("Msf: %8d\n", ((unsigned int *)data)[1]);
				GFAST_DEBUG("Mds_Start: %2d\n", ((unsigned int *)data)[2]);
				GFAST_DEBUG("Mds_End: %4d\n", ((unsigned int *)data)[3]);
				GFAST_DEBUG("Mus_Start: %2d\n", ((unsigned int *)data)[4]);
				GFAST_DEBUG("Mus_End: %4d\n", ((unsigned int *)data)[5]);
			}
			break;
		case UserGetOlrType:
			ret = rtk_gfast_dsp_GetOlrType(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((unsigned int *)data, (unsigned int *)((msgToDslPorts *)testbuf)->intVal, sizeof(unsigned int));
				GFAST_DEBUG("[%s] case %d, OLR Type:0x%02x%02x%02x%02x\n\n", __FUNCTION__, id, ((uint8_t *)data)[0], ((uint8_t *)data)[1], ((uint8_t *)data)[2], ((uint8_t *)data)[3]);
			}
			break;
		case UserGetErrorCount:
			ret = rtk_gfast_dsp_GetErrCount(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((unsigned int *)data, (unsigned int *)((msgToDslPorts *)testbuf)->intVal, sizeof(unsigned int)*4);
				GFAST_DEBUG("[%s] case %d\n", __FUNCTION__, id);
				GFAST_DEBUG("Upstream CRC of this Port in Lp0: %04x\n", ((unsigned int *)data)[0]);
				GFAST_DEBUG("Upstream CRC of this Port in Lp1: %04x\n", ((unsigned int *)data)[1]);
				GFAST_DEBUG("Downstream CRC of this Port in Lp0: %04x\n", ((unsigned int *)data)[2]);
				GFAST_DEBUG("Downstream CRC of this Port in Lp1: %04x\n", ((unsigned int *)data)[3]);
			}
			break;
		case UserGetSNRMargin:
			ret = rtk_gfast_dsp_GetUserData(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), UserGetSNRMargin, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case UserGetSNR:
			ret = rtk_gfast_dsp_GetNeSnr(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				int i=0;
				memcpy(&i, ((msgToDslPorts *)testbuf)->intVal, sizeof(int));	/* first 4 byte ==> tone number */
				memcpy((int *)data, (int *)((msgToDslPorts *)testbuf)->intVal, sizeof(int)*i);
				GFAST_DEBUG("[%s] case %d, NeSnr Tone size=%d\n", __FUNCTION__, id, ((int *)data)[0]);
#if 0
				if (!((int *)data)[0]) {
					printf("data NA, not in showtime\n");
				} else {
					for (i=0; i<((int *)data)[0]; i++) {
						printf("tone %d:\t%4d\n",i*8,((int *)data)[i+1]);
					}
				}
				printf("OK\n");
#endif
			}
			break;
		case UserGetTrellisMode:
			ret = rtk_gfast_dsp_GetTrellisMode(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((struct Modem_Trellis*)data, (struct Modem_Trellis*)((msgToDslPorts *)testbuf)->intVal, sizeof(struct Modem_Trellis));
				GFAST_DEBUG("[%s] case %d, Trellis(Tx, Rx) = (%x, %x)\n", __FUNCTION__, id, ((struct Modem_Trellis*)data)->upstreamTrellis, ((struct Modem_Trellis*)data)->downstreamTrellis);
			}
			break;
		case UserGetNEBitAlloc:
			ret = rtk_gfast_dsp_GetNeBi(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				int i=0;
				memcpy(&i, ((msgToDslPorts *)testbuf)->intVal, sizeof(int));	/* first 4 byte ==> tone number */
				memcpy((int *)data, (int *)((msgToDslPorts *)testbuf)->intVal, sizeof(int)*i);
				GFAST_DEBUG("[%s] case %d, NeBi Tone size=%d\n", __FUNCTION__, id, ((int *)data)[0]);
#if 0
				if (!((int *)data)[0]) {
					printf("data NA, not in showtime\n");
				} else {
					for (i=0; i<((int *)data)[0]; i++) {
						printf("tone %d:\t%4d\n",i*8,((int *)data)[i+1]);
					}
				}
				printf("OK\n");
#endif
			}
			break;
		case UserGetFEBitAlloc:
			ret = rtk_gfast_dsp_GetFeBi(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				int i=0;
				memcpy(&i, ((msgToDslPorts *)testbuf)->intVal, sizeof(int));	/* first 4 byte ==> tone number */
				memcpy((int *)data, (int *)((msgToDslPorts *)testbuf)->intVal, sizeof(int)*i);
				GFAST_DEBUG("[%s] case %d, FeBi Tone size=%d\n", __FUNCTION__, id, ((int *)data)[0]);
#if 0				
				if (!((int *)data)[0]) {
					printf("data NA, not in showtime\n");
				} else {
					for (i=0; i<((int *)data)[0]; i++) {
						printf("tone %d:\t%4d\n",i*8,((int *)data)[i+1]);
					}
				}
				printf("OK\n");
#endif
			}
			break;
		case UserGetLinkSpeed:
			ret = rtk_gfast_dsp_GetLineRate(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				memcpy((struct Modem_LinkSpeed*)data, (struct Modem_LinkSpeed*)((msgToDslPorts *)testbuf)->intVal, sizeof(struct Modem_LinkSpeed));
				GFAST_DEBUG("[%s] case %d, US: %d kbps, DS: %d kbps\n", __FUNCTION__, id, ((struct Modem_LinkSpeed*)data)->upstreamRate, ((struct Modem_LinkSpeed*)data)->downstreamRate);
			}
			break;
		case UserGetLinkPowerState:
			ret = rtk_gfast_dsp_GetUserData(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), UserGetLinkPowerState, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case UserGetDSPversion:
			ret = rtk_gfast_dsp_GetDspVersion(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else {
				strcpy((unsigned char *)data , (unsigned char *)((msgToDslPorts *)testbuf)->intVal);
				GFAST_DEBUG("[%s] case %d, DSP Version: %s\n", __FUNCTION__, id, (unsigned char *)data);
			}
			break;
		case UserGetDSPBuild:
			ret = rtk_gfast_dsp_GetDspBuild(h, DSL_CHIP_ID, DSL_CHIP_PORT, testbuf, sizeof(testbuf), NULL, 0);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);	
			break;
		case GetTest:
			break;
		default:
			break;
	}

err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1);	//follow _adsl_drv_get return
}
#endif /* CONFIG_USER_CMD_SUPPORT_GFAST */

// RLCM_UserGetDslData case on remote handler
int commserv_getVDSL(unsigned int id, void *data){
	int ret = -1;
	struct controller_states ctrld_states;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	ret = rtk_xdsl_get_controllerd_status(h, DSL_CHIP_ID, &ctrld_states, sizeof(struct controller_states));
	if (ret || !(ctrld_states.linkstate)) {
		printf("[%s]commserv is not link up.\n", __FUNCTION__);
		goto err;
	}

	switch(id){
		case GetPmdMode:
			ret = rtk_xdsl_dsp_GetPmdMode(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case GetVdslProfile:
			break;
		case GetTrellis:
			ret = rtk_xdsl_dsp_GetTrellis(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case GetPhyR:
			ret = rtk_xdsl_dsp_GetPhyR(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case GetHskXdslMode:
			break;
		case GetUPBOKLE:
			break;
		case GetPmsPara_Nfec:
			ret = rtk_xdsl_dsp_GetPmsPara_Nfec(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case GetPmsPara_L:
			ret = rtk_xdsl_dsp_GetPmsPara_L(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case GetACTSNRMODE:
			break;
		case GetACTUALCE:
			break;
		case GetPmsPara_INP:
			ret = rtk_xdsl_dsp_GetPmsPara_INP(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case GetTpsMode:
			ret = rtk_xdsl_dsp_GetTpsMode(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
		case GetAttNDR:
			ret = rtk_xdsl_dsp_GetAttainableRate(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;

		// data len 4 byte
		case GetLimitMask:
		case GetUs0Mask:
		case GetDslStateLast:
		case GetDslStateCurr:
		case GetInpReport:
		case GetPTMStatus:
		case GetDeviceType:
		case GetEvCntDs_e127:
		case GetEvCntUs_e127:
		case GetEvCnt15MinDs_e127:
		case GetEvCnt15MinUs_e127:
		case GetEvCnt1DayDs_e127:
		case GetEvCnt1DayUs_e127:
		case GetInmCntDs_e127:
		case GetInmCntUs_e127:
		case GetInmCnt15MinDs_e127:
		case GetInmCnt15MinUs_e127:
		case GetInmCnt1DayDs_e127:
		case GetInmCnt1DayUs_e127:
		case GetVectorMode:
		case GetVdslProfileCap:
			ret = rtk_xdsl_dsp_GetUserData(h, DSL_CHIP_ID, DSL_CHIP_PORT, id, (uint32_t *)data, 1);
			if (ret)
				printf("[%s:%d]id=%d, Error = %d\n", __FILE__, __LINE__, id, ret);
			break;
		// data len 4 byte * 2
		case GetSATN:
		case GetLATN:
		case GetRxPwr:
		case GetDataLpId:
		case GetRAMode:
		case GetReTxOhRate:
		case GetReTxFrmType:
		case GetReTxActInpRein:
		case GetReTxH:
		case GetFECS:
		case GetLOSs:
		case GetLOFs:
		case GetRSCorr:
		case GetRSUnCorr:
		case GetReTxRtx:
		case GetReTxRtxCorr:
		case GetReTxRtxUnCorr:
		case GetReTxLEFTRs:
		case GetReTxMinEFTR:
		case GetReTxErrFreeBits:
		case GetLOL:
		case GetLOLs:
		case GetLPR:
		case GetLPRs:
		case GetHEC:
		case GetOCD:
		case GetLCD:
		case GetTotalCell:
		case GetDataCell:
		case GetBitError:
			ret = rtk_xdsl_dsp_GetUserData(h, DSL_CHIP_ID, DSL_CHIP_PORT, id, (uint32_t *)data, 2);
			if (ret)
				printf("[%s:%d]id=%d, Error = %d\n", __FILE__, __LINE__, id, ret);
			break;

		// data len 4 byte * 4
		case GetPmsPara_I:
			ret = rtk_xdsl_dsp_GetUserData(h, DSL_CHIP_ID, DSL_CHIP_PORT, id, (uint32_t *)data, 4);
			if (ret)
				printf("[%s:%d]id=%d, Error = %d\n", __FILE__, __LINE__, id, ret);
			break;
		case GetSNRpb:
		case GetSATNpb:
		case GetLATNpb:
		case GetTxPwrpb:
		case GetRxPwrpb:
			break;
		case GetPSD_MD_Ds:
		case GetPSD_MD_Us:
			break;
		case GetDsBand:
		case GetUsBand:
			break;
		case GetBitPerToneDs:
		case GetBitPerToneUs:
		case GetGainPerToneDs:
		case GetGainPerToneUs:
			break;
			break;

		default:
			break;
	}

err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1);	//follow _adsl_drv_get return
}

// // RLCM_UserSetDslData case on remote handler
int commserv_setVDSL(unsigned int id, uint32_t data){
	int ret = 1;
	struct controller_states ctrld_states;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	ret = rtk_xdsl_get_controllerd_status(h, DSL_CHIP_ID, &ctrld_states, sizeof(struct controller_states));
	if (ret || !(ctrld_states.linkstate)) {
		printf("[%s]commserv is not link up.\n", __FUNCTION__);
		goto err;
	}

	switch(id){
		case SetPmdMode:
			ret = rtk_xdsl_dsp_SetPmdMode(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] SetPmdMode(val=%u, ret=%d)\n", __FUNCTION__, data, ret);
			break;
		case SetVdslProfile:
			ret = rtk_xdsl_dsp_SetVdslProfile(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] SetVdslProfile(val=%u, ret=%d)\n", __FUNCTION__, data, ret);
			break;
		case SetGInp:
			ret = rtk_xdsl_dsp_SetGInp(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] SetGInp(val=%u, ret=%d)\n", __FUNCTION__, data, ret);
			break;
		case SetOlr:
			ret = rtk_xdsl_dsp_SetOlr(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] SetOlr(val=%u, ret=%d)\n", __FUNCTION__, data, ret);
			break;
		case SetGVector:
			ret = rtk_xdsl_dsp_SetGVector(h, DSL_CHIP_ID, DSL_CHIP_PORT, data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] SetGVector(val=%u, ret=%d)\n", __FUNCTION__, data, ret);
			break;
		case SetDslBond:
			ret = rtk_xdsl_dsp_SetBondingMode(h, DSL_CHIP_ID, DSL_CHIP_PORT, (uint8_t) data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);	
			else
				GFAST_DEBUG("[%s] SetDslBond(val=%u, ret=%d)\n", __FUNCTION__, data, ret);
			break;
		case SetSwApiDef:
		case SetChnProfUs:
		case SetChnProfDs:
		case SetChnProfGinpUs:
		case SetChnProfGinpDs:
		case SetLineMarginUs:
		case SetLineMarginDs:
		case SetLinePowerUs:
		case SetLinePowerDs:
		case SetLineBSUs:
		case SetLineBSDs:
		case SetOHrateUs:
		case SetOHrateDs:
		case SetLineTxMode:
		case SetLineADSL:
		case SetLineClassMask:
		case SetLineLimitMask:
		case SetLineUs0tMask:
		case SetLineUPBO:
		case SetLineRTMode:
		case SetLineUS0:
		case SetLineRAUs:
		case SetLineRADs:
		case SetLineSOSUs:
		case SetLineSOSDs:
		case SetLineROCUs:
		case SetLineROCDs:
		case SetLineDPBO:
		case SetLineDPBOPSD:
		case SetLineRFI:
		case SetInmFE:
		case SetInmNE:
		case SetLinePSDMIBUs:
		case SetLinePSDMIBDs:
		case SetLineVNUs:
		case SetLineVNDs:
			ret = rtk_xdsl_dsp_SetUserData(h, DSL_CHIP_ID, DSL_CHIP_PORT, id, data);
			if (ret)
				printf("[%s:%d]id= %d, Error = %d\n", __FILE__, __LINE__, id, ret);
			else
				GFAST_DEBUG("[%s] case %d set success(val=%u, ret=%d)\n", __FUNCTION__, id, data, ret);
			break;
		default:
			break;
	}

err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1);	//follow _adsl_drv_get return
}

// 
int commserv_getxDSL(unsigned int id, void *data, unsigned int len){
	int ret = -1;
	struct controller_states ctrld_states;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	ret = rtk_xdsl_get_controllerd_status(h, DSL_CHIP_ID, &ctrld_states, sizeof(struct controller_states));
	if (ret || !(ctrld_states.linkstate)) {
		printf("[%s]commserv is not link up.\n", __FUNCTION__);
		goto err;
	}

	switch(id){
		case RLCM_PHY_ENABLE_MODEM:
			ret = rtk_xdsl_dsp_EnableModemLine(h, DSL_CHIP_ID, DSL_CHIP_PORT);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_PHY_ENABLE_MODEM(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_PHY_DISABLE_MODEM:
			ret = rtk_xdsl_dsp_DisableModemLine(h, DSL_CHIP_ID, DSL_CHIP_PORT);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_PHY_DISABLE_MODEM(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_SNR_MARGIN:
			ret = rtk_xdsl_dsp_GetSnrMargin(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_Data *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else{
				GFAST_DEBUG("[%s] RLCM_GET_SNR_MARGIN(ret=%d)\n", __FUNCTION__, ret);
				GFAST_DEBUG("dsdata: %d, usdata: %d\n", ((struct Modem_Data *)data)->dsdata, ((struct Modem_Data *)data)->usdata);
			}
			break;
		case RLCM_MODEM_FAR_END_LINE_DATA_REQ:
			ret = rtk_xdsl_dsp_GetPowerDS(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_FarEndLineOperData *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_MODEM_FAR_END_LINE_DATA_REQ(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_MODEM_NEAR_END_LINE_DATA_REQ:
			ret = rtk_xdsl_dsp_GetPowerUS(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_NearEndLineOperData *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_MODEM_NEAR_END_LINE_DATA_REQ(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_INIT_COUNT_LAST_LINK_RATE:
			ret = rtk_xdsl_dsp_GetInitCountAndLastLinkRate(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_InitLastRate *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_INIT_COUNT_LAST_LINK_RATE(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_DSL_ORHERS:
			ret = rtk_xdsl_dsp_get_sync_time(h, DSL_CHIP_ID, DSL_CHIP_PORT, (unsigned long *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_DSL_ORHERS(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_REHS_COUNT:
			ret = rtk_xdsl_dsp_get_sync_number(h, DSL_CHIP_ID, DSL_CHIP_PORT, (unsigned long *) data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_REHS_COUNT(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_DS_PMS_PARAM1:
			ret = rtk_xdsl_dsp_GetUsPMSParam1(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_PMSParm *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_DS_PMS_PARAM1(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_US_PMS_PARAM1:
			ret = rtk_xdsl_dsp_GetDsPMSParam1(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_PMSParm *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_US_PMS_PARAM1(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_US_ERROR_COUNT:
			ret = rtk_xdsl_dsp_GetUsErrorCount(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_MgmtCounter *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_US_ERROR_COUNT(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_DS_ERROR_COUNT:
			ret = rtk_xdsl_dsp_GetDsErrorCount(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_MgmtCounter *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_DS_ERROR_COUNT(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_FRAME_COUNT:
			ret = rtk_xdsl_dsp_GetFrameCount(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_Data *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_FRAME_COUNT(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_MODEM_RETRAIN:
			ret = rtk_xdsl_dsp_Retrain(h, DSL_CHIP_ID, DSL_CHIP_PORT);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_MODEM_RETRAIN(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_GET_VDSL2_DIAG_SNR:
			ret = rtk_xdsl_dsp_GetVdsl2DiagSnr(h, DSL_CHIP_ID, DSL_CHIP_PORT, data, len);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_GET_VDSL2_DIAG_SNR(ret=%d)\n", __FUNCTION__, ret);
			break;
		case RLCM_REPORT_MODEM_STATE:
			ret = rtk_xdsl_dsp_ReportModemStates(h, DSL_CHIP_ID, DSL_CHIP_PORT, (char *)data, (int)len);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else
				GFAST_DEBUG("[%s] RLCM_REPORT_MODEM_STATE(ret=%d). data=%s\n", __FUNCTION__, ret, (char *)data);
			break;
		case RLCM_GET_LINK_SPEED:
			ret = rtk_xdsl_dsp_GetLinkSpeed(h, DSL_CHIP_ID, DSL_CHIP_PORT, (struct Modem_LinkSpeed *)data);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			else{
				GFAST_DEBUG("[%s] RLCM_GET_LINK_SPEED(ret=%d)\n", __FUNCTION__, ret);
				GFAST_DEBUG("US: %d kbps, DS: %d kbps\n", ((struct Modem_LinkSpeed*)data)->upstreamRate, ((struct Modem_LinkSpeed*)data)->downstreamRate);
			}
			break;
		default:
			ret = rtk_xdsl_dsp_drv_set(h, DSL_CHIP_ID, DSL_CHIP_PORT, id, data, len);
			if (ret)
				printf("[%s:%d]Error = %d\n", __FILE__, __LINE__, ret);
			break;
	}

err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1);	//follow _adsl_drv_get return
}

int commserv_OAMLoobback(unsigned int action, void *data){
	int ret = -1;
	struct controller_states ctrld_states;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	ret = rtk_xdsl_get_controllerd_status(h, DSL_CHIP_ID, &ctrld_states, sizeof(struct controller_states));
	if (ret || !(ctrld_states.linkstate)) {
		printf("[%s]commserv is not link up.\n", __FUNCTION__);
		goto err;
	}

	switch(action){		
		case 0:/*OAM_START*/
			ret = rtk_xdsl_oam_loopback_start(h, DSL_CHIP_ID, (struct rtk_xdsl_oam_loopback_req *)data, &(((struct rtk_xdsl_oam_loopback_req *)data)->Tag));
			if (ret){
				printf("oam loopback start failed\n");
			} else {
				GFAST_DEBUG("oam loopback start successfuly: (%d, %d)tag is %d\n", ((struct rtk_xdsl_oam_loopback_req *)data)->vpi
																	, ((struct rtk_xdsl_oam_loopback_req *)data)->vci
																	, ((struct rtk_xdsl_oam_loopback_req *)data)->Tag);
			}
			break;
		case 1:/*OAM_STOP*/
			ret = rtk_xdsl_oam_loopback_stop(h, DSL_CHIP_ID, (struct rtk_xdsl_oam_loopback_req *)data, &(((struct rtk_xdsl_oam_loopback_req *)data)->Tag));
			if (ret){
				printf("oam loopback stop failed\n");
			} else {
				GFAST_DEBUG("oam loopback stop successfuly: (%d, %d)tag is %d\n", ((struct rtk_xdsl_oam_loopback_req *)data)->vpi
																	, ((struct rtk_xdsl_oam_loopback_req *)data)->vci
																	, ((struct rtk_xdsl_oam_loopback_req *)data)->Tag);
			}
			break;
		case 2:/*OAM_RESULT*/
			ret = rtk_xdsl_oam_loopback_result(h, DSL_CHIP_ID, (struct rtk_xdsl_oam_loopback_state *)data);
			if (ret){
				printf("oam loopback result: fail\n");
			} else {
				GFAST_DEBUG("oam loopback success:\n");
				GFAST_DEBUG("(vpi, vci):  (%d, %d)\n", ((struct rtk_xdsl_oam_loopback_state *)data)->vpi, ((struct rtk_xdsl_oam_loopback_state *)data)->vci);
				GFAST_DEBUG("count:  %d\n", ((struct rtk_xdsl_oam_loopback_state *)data)->count[0]);
				GFAST_DEBUG("time:   %d\n", ((struct rtk_xdsl_oam_loopback_state *)data)->rtt[0]);
				GFAST_DEBUG("status: %d\n", ((struct rtk_xdsl_oam_loopback_state *)data)->status[0]);
			}
			break;
		default:
			break;
	}

	usleep(1000);	//avoid continuous send c-api
err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1);	//follow _adsl_drv_get return
}

int commserv_atmSetup(unsigned int action, void *data){
	int ret = -1;
	struct rtk_xdsl_atm_channel_info vc;
	struct controller_states ctrld_states;
	struct api_handle *h = rtk_xdsl_api_init();
	if(!h){
		printf("[%s]init api_handler error.\n", __FUNCTION__);
		return 0;
	}

	ret = rtk_xdsl_get_controllerd_status(h, DSL_CHIP_ID, &ctrld_states, sizeof(struct controller_states));
	if (ret || !(ctrld_states.linkstate)) {
		printf("[%s]commserv is not link up.\n", __FUNCTION__);
		goto err;
	}

	switch(action){		
		case 0:/*ADD*/
			ret = rtk_xdsl_create_vc(h, DSL_CHIP_ID, (struct rtk_xdsl_atm_channel_info *)data);
			if (ret){
				printf("rtk_xdsl_create_vc fail\n");
			}else{
				GFAST_DEBUG("Create remote VC success,\nch_no=%d, (vpi,vci)=(%d, %d),\t\nrfc=%d, framing=%d,\n\tQoS_T=%d, pcr=%d, scr=%d, mbs=%d\n", 
					((struct rtk_xdsl_atm_channel_info *)data)->ch_no,
					((struct rtk_xdsl_atm_channel_info *)data)->vpi, ((struct rtk_xdsl_atm_channel_info *)data)->vci,
					((struct rtk_xdsl_atm_channel_info *)data)->rfc, ((struct rtk_xdsl_atm_channel_info *)data)->framing,
					((struct rtk_xdsl_atm_channel_info *)data)->qos_type, ((struct rtk_xdsl_atm_channel_info *)data)->pcr, ((struct rtk_xdsl_atm_channel_info *)data)->scr, ((struct rtk_xdsl_atm_channel_info *)data)->mbs);
			}
			break;
		case 1:/*DELETE*/
			ret = rtk_xdsl_delete_vc(h, DSL_CHIP_ID, *((int *)data));
			if (ret){
				printf("rtk_xdsl_create_vc fail, ch_no=%d\n", *((int *)data));
			}else{
				GFAST_DEBUG("rtk_xdsl_create_vc fail, ch_no=%d\n", *((int *)data));
			}			
			break;
		default:
			break;
	}

err:
	rtk_xdsl_api_fini(h);
	return ((ret)?0:1);	//follow _adsl_drv_get return
}
#endif

const char LANIF[] = "br0";
const char LAN_ALIAS[] = "br0:0";	// alias for secondary IP
#ifdef IP_PASSTHROUGH
const char LAN_IPPT[] = "br0:1";	// alias for IP passthrough
#endif
const char ELANIF[] = "eth0";
#ifdef CONFIG_RTL_MULTI_LAN_DEV
const char* ELANVIF[] = {ALIASNAME_ELAN0, ALIASNAME_ELAN1, ALIASNAME_ELAN2, ALIASNAME_ELAN3, ALIASNAME_ELAN4, ALIASNAME_ELAN5};
const char* SW_LAN_PORT_IF[] = {ALIASNAME_ELAN0, ALIASNAME_ELAN1, ALIASNAME_ELAN2, ALIASNAME_ELAN3}; // used for iptables
#else
const char* ELANVIF[] = {"eth0"};
const char* SW_LAN_PORT_IF[] = {ALIASNAME_ELAN0, ALIASNAME_ELAN1, ALIASNAME_ELAN2, ALIASNAME_ELAN3}; // used for iptables
#endif
const char BRIF[] = "br0";
#if defined(CONFIG_RTL8681_PTM) && !defined(CONFIG_RTL867X_PACKET_PROCESSOR)
const char PTMIF[] = "ptm0";
#else
const char PTMIF[] = "ptm0_pp";
#endif
#ifdef CONFIG_NET_IPGRE
const char* GREIF[] = {"gret0", "gret1"};
#endif
#ifdef CONFIG_USB_ETH
const char USBETHIF[] = "usb0";
#endif //CONFIG_USB_ETH
//the name of wlan
#if defined(CONFIG_SLAVE_WLAN1_ENABLE) && !defined(CONFIG_MASTER_WLAN0_ENABLE)
const char*  wlan[]   = {"wlan1", "wlan1-vap0", "wlan1-vap1", "wlan1-vap2", "wlan1-vap3", ""};
const char*  wlan_itf[] = {"WLAN", "WLAN-AP1", "WLAN-AP2", "WLAN-AP3", "WLAN-AP4", ""};
#else
const char*  wlan[]   = {"wlan0", "wlan0-vap0", "wlan0-vap1", "wlan0-vap2", "wlan0-vap3",
						 "wlan1", "wlan1-vap0", "wlan1-vap1", "wlan1-vap2", "wlan1-vap3", ""};
const char*  wlan_itf[] = {"WLAN0", "WLAN0-AP1", "WLAN0-AP2", "WLAN0-AP3", "WLAN0-AP4",
						   "WLAN1", "WLAN1-AP1", "WLAN1-AP2", "WLAN1-AP3", "WLAN1-AP4", ""};

#endif
#ifdef WLAN_SUPPORT
const int wlan_en[] = {1 //wlan0
#ifdef WLAN_MBSSID
	,1, 1, 1, 1 //wlan0-vap0 ~ wlan0-vap3
#else
	,0, 0, 0, 0
#endif
#if defined(WLAN_DUALBAND_CONCURRENT) && !defined(WLAN1_QTN)
	,1 //wlan1
#ifdef WLAN_MBSSID
	,1, 1, 1, 1 //wlan1-vap0 ~ wlan1-vap3
#else
	,0, 0, 0, 0
#endif
#else
	,0, 0, 0, 0, 0
#endif
};
#else
const int wlan_en[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif
const char VC_BR[] = "0";
const char LLC_BR[] = "1";
const char VC_RT[] = "3";
const char LLC_RT[] = "4";
#ifdef CONFIG_ATM_CLIP
const char LLC_1577[] = "6";
#endif
const char PORT_DNS[] = "53";
const char PORT_DHCP[] = "67";
const char PORT_DHCPV6[] = "546";
const char BLANK[] = "";
const char ARG_ADD[] = "add";
const char ARG_CHANGE[] = "change";
const char ARG_DEL[] = "del";
const char ARG_ENCAPS[] = "encaps";
const char ARG_QOS[] = "qos";
const char ARG_255x4[] = "255.255.255.255";
const char ARG_0x4[] = "0.0.0.0";
const char ARG_BKG[] = "&";
const char ARG_I[] = "-i";
const char ARG_O[] = "-o";
const char ARG_T[] = "-t";
const char ARG_TCP[] = "TCP";
const char ARG_UDP[] = "UDP";
const char ARG_NO[] = "!";
const char ARG_ICMP[] = "ICMP";
const char FW_BLOCK[] = "block";
const char FW_INACC[] = "inacc";
const char PORTMAP_IPTBL[] = "portmapping_dhcp";
const char PORTMAP_IGMP[] = "portmapping_igmp"; // ebtables INPUT IGMP blocking
const char FW_ACL[] = "aclblock";
const char FW_CWMP_ACL[] = "cwmp_aclblock";
#if  defined(REMOTE_ACCESS_CTL) || defined(IP_ACL)
const char RMACC_MARK[] = "0x1000";
#endif
const char FW_DHCP_PORT_FILTER[] = "dhcp_port_filter";
#ifdef IP_PORT_FILTER
const char FW_IPFILTER[] = "ipfilter";
#ifdef CONFIG_00R0
const char FW_IPFILTER_LOCAL[] = "ipfilter_local";
#endif
#endif
const char FW_PARENTAL_CTRL[] ="parental_ctrl";
const char FW_BR_WAN[] = "br_wan";
const char FW_BR_WAN_OUT[] = "br_wan_out";
const char FW_BR_PPPOE[] = "br_pppoe";
#ifdef MAC_FILTER
const char FW_MACFILTER[] = "macfilter";
#endif
const char FW_DHCPS_DIS[] = "dhcps_disable";
#ifdef CONFIG_USER_VLAN_ON_LAN
const char BR_VLAN_ON_LAN[] = "vlan_on_lan";
#endif
const char FW_APPFILTER[] = "appfilter";
const char FW_APP_P2PFILTER[] = "appp2pfilter";
const char FW_IPQ_MANGLE_DFT[] = "m_ipq_dft";
const char FW_IPQ_MANGLE_USER[] = "m_ipq_user";
#ifdef PORT_FORWARD_ADVANCE
const char FW_PPTP[] = "pptp";
const char FW_L2TP[] = "l2tp";
#endif
#ifdef PORT_FORWARD_GENERAL
const char PORT_FW[] = "portfw";
#endif
#if defined(PORT_FORWARD_GENERAL) || defined(DMZ)
#ifdef NAT_LOOPBACK
const char PORT_FW_PRE_NAT_LB[] = "portfwPreNatLB";
const char PORT_FW_POST_NAT_LB[] = "portfwPostNatLB";
#endif
#endif
#ifdef DMZ
#ifdef NAT_LOOPBACK
const char IPTABLE_DMZ_PRE_NAT_LB[] = "dmzPreNatLB";
const char IPTABLE_DMZ_POST_NAT_LB[] = "dmzPostNatLB";
#endif
const char IPTABLE_DMZ[] = "dmz";
#endif
#ifdef NATIP_FORWARDING
const char IPTABLE_IPFW[] = "ipfw";
const char IPTABLE_IPFW2[] = "ipfw_PostRouting";
#endif
#ifdef PORT_TRIGGERING
const char IPTABLES_PORTTRIGGER[] = "portTrigger";
#endif
const char IPTABLE_TR069[] = "tr069";
const char FW_DROP[] = "DROP";
const char FW_ACCEPT[] = "ACCEPT";
const char FW_RETURN[] = "RETURN";
const char FW_FORWARD[] = "FORWARD";
const char FW_INPUT[] = "INPUT";
const char FW_OUTPUT[] = "OUTPUT";
const char FW_PREROUTING[] = "PREROUTING";
const char FW_POSTROUTING[] = "POSTROUTING";
const char FW_DPORT[] = "--dport";
const char FW_SPORT[] = "--sport";
const char FW_ADD[] = "-A";
const char FW_DEL[] = "-D";
const char FW_INSERT[] = "-I";
const char CONFIG_HEADER[] = "<Config_Information_File_8671>";
const char CONFIG_TRAILER[] = "</Config_Information_File_8671>";
const char CONFIG_HEADER_HS[] = "<Config_Information_File_HS>";
const char CONFIG_TRAILER_HS[] = "</Config_Information_File_HS>";
const char CONFIG_XMLFILE[] = "/tmp/config.xml";
const char CONFIG_RAWFILE[] = "/tmp/config.bin";
const char CONFIG_XMLFILE_HS[] = "/tmp/config_hs.xml";
const char CONFIG_RAWFILE_HS[] = "/tmp/config_hs.bin";
const char PPP_SYSLOG[] = "/tmp/ppp_syslog";
const char PPP_DEBUG_LOG[] = "/tmp/ppp_debug_log";

#ifdef CONFIG_USER_CMD_CLIENT_SIDE
const char CMDSERV_CTRLERD[] = "/bin/controllerd";
#endif
#ifdef CONFIG_USER_CMD_SERVER_SIDE
const char CMDSERV_ENDPTD[] = "/bin/endptd";
#endif
const char ADSLCTRL[] = "/bin/adslctrl";
const char IFCONFIG[] = "/bin/ifconfig";
const char BRCTL[] = "/bin/brctl";
const char MPOAD[] = "/bin/mpoad";
const char MPOACTL[] = "/bin/mpoactl";
const char DHCPD[] = "/bin/udhcpd";
const char DHCPC[] = "/bin/udhcpc";
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
const char DNSRELAY[] = "/bin/dnsmasq";
const char DNSRELAYPID[] = "/var/run/dnsmasq.pid";
#endif
const char WEBSERVER[] = "/bin/boa";
const char SNMPD[] = "/bin/snmpd";
const char ROUTE[] = "/bin/route";
const char IPTABLES[] = "/bin/iptables";
#ifdef CONFIG_IPV6
const char IP6TABLES[] = "/bin/ip6tables";
const char FW_IPV6FILTER[] = "ipv6filter";
const char ARG_ICMPV6[] = "ICMPV6";
#endif
const char EMPTY_MAC[MAC_ADDR_LEN] = {0};
/*ql 20081114 START need ebtables support*/
const char EBTABLES[] = "/bin/ebtables";
const char ZEBRA[] = "/bin/zebra";
const char RIPD[] = "/bin/ripd";
#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
const char OSPFD[] = "/bin/ospfd";
#endif
const char ROUTED[] = "/bin/routed";
const char IGMPROXY[] = "/bin/igmpproxy";
const char MLDPROXY[] = "/bin/ecmh";		// Mason Yu. MLD Proxy
#if defined(CONFIG_RTL867X_NETLOG) && defined(CONFIG_USER_NETLOGGER_SUPPORT)
const char NETLOGGER[]="/bin/netlogger";
#endif
const char TC[] = "/bin/tc";
const char STARTUP[] = "/bin/startup";
const char OMCICLI[] = "/bin/omcicli";
#ifdef TIME_ZONE
const char SNTPC[] = "/bin/vsntp";
const char SNTPC_PID[] = "/var/run/vsntp.pid";
#endif
#ifdef CONFIG_USER_DDNS
const char DDNSC_PID[] = "/var/run/updatedd.pid";
#endif
const char ROUTED_PID[] = "/var/run/routed.pid";
#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
const char ZEBRA_PID[] = "/var/run/zebra.pid";
const char OSPFD_PID[] = "/var/run/ospfd.pid";
#endif
const char IGMPPROXY_PID[] = "/var/run/igmp_pid";
const char MLDPROXY_PID[] = "/var/run/ecmh.pid";	// Mason Yu. MLD Proxy

const char PROC_DYNADDR[] = "/proc/sys/net/ipv4/ip_dynaddr";
const char PROC_IPFORWARD[] = "/proc/sys/net/ipv4/ip_forward";
#ifdef CONFIG_IPV6
#if defined(CONFIG_USER_DHCPV6_ISC_DHCP411) || defined(CONFIG_USER_RADVD)
const char RADVD_CONF[] = "/var/radvd.conf";
const char RADVD_PID[] = "/var/run/radvd.pid";
#endif
const char PROC_IP6FORWARD[] = "/proc/sys/net/ipv6/conf/all/forwarding";
const char PROC_MC6FORWARD[] = "/proc/sys/net/ipv6/conf/all/mc_forwarding";
#endif // of CONFIG_IPV6
const char PROC_NET_ATM_PVC[] = "/proc/net/atm/pvc";
#if defined(_LINUX_2_6_) || defined(_LINUX_3_18_)
const char PROC_FORCE_IGMP_VERSION[] = "/proc/sys/net/ipv4/conf/default/force_igmp_version";
#endif
const char MPOAD_FIFO[] = "/tmp/serv_fifo";
const char DHCPC_PID[] = "/var/run/udhcpc.pid";
const char DHCPC_ROUTERFILE[] = "/var/udhcpc/router";
const char DHCPC_SCRIPT[] = "/etc/scripts/udhcpc.sh";
const char DHCPC_SCRIPT_NAME[] = "/var/udhcpc/udhcpc";
const char DHCPD_CONF[] = "/var/udhcpd/udhcpd.conf";
const char DHCPD_LEASE[] = "/var/udhcpd/udhcpd.leases";
const char DHCPSERVERPID[] = "/var/run/udhcpd.pid";
const char DHCPRELAYPID[] = "/var/run/dhcrelay.pid";
const char IPOA_IPINFO[] = "/tmp/IPoAHalfBridge";
const char MER_GWINFO[] = "/tmp/MERgw";
#ifdef CONFIG_USER_CUPS
const char CUPSDPRINTERCONF[] = "/var/cups/conf/printers.conf";
#endif // CONFIG_USER_CUPS
#define MINI_UPNPDPID  "/var/run/mini_upnpd.pid"	//cathy
#define MINIUPNPDPID  "/var/run/linuxigd.pid"
const char MINIDLNAPID[] = "/var/run/minidlna.pid";
const char STR_DISABLE[] = "Disabled";
const char STR_ENABLE[] = "Enabled";
const char STR_AUTO[] = "Auto";
const char STR_MANUAL[] = "Manual";
const char STR_UNNUMBERED[] = "unnumbered";
const char STR_ERR[] = "err";
const char STR_NULL[] = "null";
const char rebootWord0[] = "The System is Restarting ...";
const char rebootWord1[] = "This device has been configured and is rebooting.";
const char rebootWord2[] = "Close the Configuration window and wait"
			" for a minute before reopening your web browser."
			" If necessary, reconfigure your PC's IP address to match"
			" your new configuration.";
// TR-111: ManufacturerOUI, ProductClass
const char MANUFACTURER_OUI[] = DEF_MANUFACTUREROUI_STR;//"00E04C";
const char PRODUCT_CLASS[] = DEF_PRODUCTCLASS_STR;//"IGD";

#ifdef CONFIG_BOA_WEB_E8B_CH
const char PW_HOME_DIR[] = "/mnt";
#else
const char PW_HOME_DIR[] = "/tmp";
#endif
#ifdef CONFIG_USER_CLI
const char PW_CMD_SHELL[] = "/bin/cli";
#else
const char PW_CMD_SHELL[] = "/bin/sh";
#endif
const char PW_NULL_SHELL[] = "/dev/null"; //didn't allow to login, IulianWu
#ifdef CONFIG_USER_SAMBA
const char SMBDPID[] = "/var/run/smbd.pid";
#ifdef CONFIG_USER_NMBD
const char NMBDPID[] = "/var/run/nmbd.pid";
#endif
#endif

const char RESOLV[] = "/var/resolv.conf";
const char DNSMASQ_CONF[] = "/var/dnsmasq.conf";
const char RESOLV_BACKUP[] = "/var/resolv_backup.conf";
static const char MANUAL_RESOLV[] = "/var/resolv.manual.conf";
// Mason Yu. for copy the ppp & dhcp nameserver to the /var/resolv.conf
const char AUTO_RESOLV[] = "/var/resolv.auto.conf";	/*add resolv.auto.conf to distinguish DNS manual and auto config*/
const char DNS_RESOLV[] = "/var/udhcpc/resolv.conf";
const char DNS6_RESOLV[] = "/var/resolv6.conf";
const char PPP_RESOLV[] = "/etc/ppp/resolv.conf";
const char HOSTS[] = "/var/tmp/hosts";

#if defined(IP_QOS) || defined(NEW_IP_QOS_SUPPORT)
const char *n0to7[] = {
	"n/a", "0", "1", "2", "3", "4", "5", "6", "7"
};

// Note: size of prioLevel depends on the IPQOS_NUM_PKT_PRIO
const char *prioLevel[] = {
	"p0", "p1", "p2", "p3", "p4", "p5", "p6", "p7"
};

//alex_huang
#ifdef CONFIG_8021P_PRIO
const char *set1ptable[]={"set1ptbl0","set1ptbl1","set1ptbl2","set1ptbl3","set1ptbl4","set1ptbl5","set1ptbl6","set1ptbl7"};
#endif
// priority mapping of packet priority against priority queue
// ex, priomap[3] == 2 means packet priority 3 is mapping to priority queue_2 by default
//const int priomap[8] = {3, 3, 2, 2, 2, 1, 1, 1};
//const int priomap[8] = {4, 4, 3, 3, 2, 2, 1, 1};
const int priomap[8] = {3, 4, 4, 3, 2, 2, 1, 1}; //cathy
const char *ipTos[] = {
	"n/a", "Normal Service", "Minimize Cost", "Maximize Reliability",
	"Maximize Throughput", "Minimize Delay"
};
#endif

const char *ppp_auth[] = {
	"AUTO", "PAP", "CHAP", "CHAPMSV2", "NONE"
};

const char errGetEntry[] = "Get table entry error!";

// Added by Davian
#ifdef CONFIG_USER_XMLCONFIG
const char *shell_name = "/bin/ash";
#endif

//Alan 20160728
const char hideErrMsg1[] = ">/dev/null";
const char hideErrMsg2[] = "2>&1";

/*setup mac addr if mac[n-1]++ > 0xff and set mac[n-2]++, etc.*/
void setup_mac_addr(unsigned char *macAddr, int index)
{
	if((macAddr[5]+index) > 0xff)
	{
	  printf("macAddr[5]+%d = %x > 0xff\n",index,macAddr[5]+index);
	  //macAddr[4]++;
	  if((macAddr[4]+1) > 0xff)
	  {
	  	printf("macAddr[4]+1 = %x > 0xff\n",macAddr[4]+1);	  
	  	if((macAddr[3]+1) > 0xff)
		{
		  printf("something wrong for setting your macAddr[3] > 0xff!!\n");
		}
		macAddr[3]+=1;
	  }
	  macAddr[4]+=1;
    }
	AUG_PRT("macAddr[5]+%d = %x\n",index,macAddr[5]+index);

	macAddr[5]+=index;
}

int isIPAddr(char * IPStr)	//check is ip or domain name
{
	char tmpBuf[300];
	char * delim = ".";
	char * p;
	int count = 0;

	strcpy(tmpBuf,IPStr);
	p = strtok(tmpBuf,delim);
	while(p)
	{
		int i;
		int data;

		count++;
		if(strlen(p) > 3 || strlen(p) < 1)
			return 0;
		for(i=0;i<strlen(p);i++){
			if(*(p+i) < '0' || *(p+i) > '9')
				return 0;
		}
		data = atoi(p);
		if(data > 255 || data < 0)
			return 0;
		p=strtok(NULL,delim);
	}
	if(count != 4)
		return 0;

	return 1;
}

int startSSDP(void)
{
#ifdef CONFIG_USER_MINI_UPNPD
	char *argv[10];
	int i = 0, pid = 0;

	pid = read_pid((char *)MINI_UPNPDPID);
	if (pid > 0)
	{
		kill(pid, 9);
		unlink(MINI_UPNPDPID);
	}

	argv[i++]="/bin/mini_upnpd";
#ifdef WLAN_SUPPORT
#ifdef CONFIG_WIFI_SIMPLE_CONFIG
	char wscd_pid_name[32];
	getWscPidName(wscd_pid_name);
	if (read_pid(wscd_pid_name) > 0){
		argv[i++]="-wsc";
		argv[i++]="/tmp/wscd_config";
	}
#endif
#endif

#ifdef CONFIG_USER_MINIUPNPD
	if (read_pid((char *)MINIUPNPDPID) > 0){
		argv[i++]="-igd";
		argv[i++]="/tmp/igd_config";
	}
#endif
	argv[i]=NULL;
	do_cmd( argv[0], argv, 0 );
#endif
	return 0;
}

/*
vc0 return 1, vc1 return 2 ...
ppp0 return 11, ppp1 return 12 ...
*/
static int caculate_tblid_ITF_SourceRoute(uint32_t ifid)
{
	int tbl_id;

	uint32_t ifindex;

	ifindex = ifid;

	//ifindex of vc* is 0xff01, 0xff02, ...
	//ifindex 0f ppp* is 0x00, 0x0101, 0x0202 ...
#ifdef CONFIG_ETHWAN
	//if(CHECK_NAS_IDX(ifindex))
	if( MEDIA_INDEX(ifindex)==MEDIA_ETH )
	{
		if(PPP_INDEX(ifindex) == DUMMY_PPP_INDEX)
			tbl_id = ETH_INDEX(ifindex) + ITF_SOURCE_ROUTE_NAS_START;
		else
			tbl_id = PPP_INDEX(ifindex) + ITF_SOURCE_ROUTE_PPP_START;
	}
	else
#endif /*CONFIG_ETHWAN*/
#ifdef CONFIG_PTMWAN
	if( MEDIA_INDEX(ifindex)==MEDIA_PTM )
	{
		if(PPP_INDEX(ifindex) == DUMMY_PPP_INDEX)
			tbl_id = PTM_INDEX(ifindex) + ITF_SOURCE_ROUTE_PTM_START;
		else
			tbl_id = PPP_INDEX(ifindex) + ITF_SOURCE_ROUTE_PPP_START;
	}
	else
#endif /*CONFIG_PTMWAN*/
	{
	//ifindex of vc* is 0xff01, 0xff02, ...
	//ifindex of ppp* is 0x00, 0x0101, 0x0202 ...
	if (PPP_INDEX(ifindex) == DUMMY_PPP_INDEX)
		tbl_id = VC_INDEX(ifindex) + ITF_SOURCE_ROUTE_VC_START;
	else
		tbl_id = PPP_INDEX(ifindex) + ITF_SOURCE_ROUTE_PPP_START;
	}

	// Mason Yu.
	//printf("ifindex=0x%x, tbl_id=0x%x\n", ifindex, tbl_id);
	return tbl_id;
}

//rule_mark[0] = "0x20000"  which means:  mark the 0 as 0x20000/0xe0000
const char*  rule_mark[] = {"0x20000/0xe0000",    "0x40000/0xe0000",
						   "0x60000/0xe0000",
						   "0x80000/0xe0000",
						   ""};

const char*  rule_mark_ppp[] = {"0xa0000/0xe0000", "0xc0000/0xe0000", "0xe0000/0xe0000",""};

#define ITF_SourceRoute_DHCP_MARK_NUM	4
#define ITF_SourceRoute_PPP_MARK_NUM	3

//eason
#ifdef _PRMT_USB_ETH_
//0:ok, -1:error
int getUSBLANMacAddr( char *p )
{
	unsigned char *hwaddr;
	struct ifreq ifr;
	strcpy(ifr.ifr_name, USBETHIF);
	do_ioctl(SIOCGIFHWADDR, &ifr);
	hwaddr = (unsigned char *)ifr.ifr_hwaddr.sa_data;
//	printf("The result of SIOCGIFHWADDR is type %d  "
//	       "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x.\n",
//	       ifr.ifr_hwaddr.sa_family, hwaddr[0], hwaddr[1],
//	       hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
	memcpy( p, hwaddr, 6 );
	return 0;
}

//0:high, 1:full, 2:low
int getUSBLANRate( void )
{
	struct ifreq ifr;
	strcpy(ifr.ifr_name, USBETHIF);
	do_ioctl( SIOCUSBRATE, &ifr );
//	fprintf( stderr, "getUSBLANRate=%d\n", *(int*)(&ifr.ifr_ifindex) ); //ifr_ifru.ifru_ivalue );
	return ifr.ifr_ifindex;//ifr_ifru.ifru_ivalue;
}

//0:up, 1:nolink
int getUSBLANStatus(void )
{
	struct ifreq ifr;
	strcpy(ifr.ifr_name, USBETHIF);
	do_ioctl( SIOCUSBSTAT, &ifr );
//	fprintf( stderr, "getUSBLANStatus=%d\n", *(int*)(&ifr.ifr_ifindex) ); //ifr_ifru.ifru_ivalue );
	return ifr.ifr_ifindex; //ifr_ifru.ifru_ivalue;
}
#endif

#if defined(CONFIG_DSL_ON_SLAVE)
#define ADSL_DFAULT_DEV 	"/dev/xdsl_ipc"
#elif defined(CONFIG_XDSL_CTRL_PHY_IS_SOC)
#define ADSL_DFAULT_DEV 	"/dev/xdsl0"
#else
#define ADSL_DFAULT_DEV 	"/dev/adsl0"
#endif

#define XDSL_SLAVE_DEV		"/dev/xdsl1"

static FILE* adslFp = NULL;

static const char *dhcp_mode[] = {
	"None", "DHCP Relay", "DHCP Server"
};

#ifdef IP_PASSTHROUGH
static void set_IPPT_LAN_access();
#endif

int virtual_port_enabled;
const int virt2user[] = {
	1, 2, 3, 4, 5
};

#ifdef CONFIG_DEV_xDSL
#if defined(AUTO_PVC_SEARCH_TR068_OAMPING) || defined(AUTO_PVC_SEARCH_PURE_OAMPING) || defined(AUTO_PVC_SEARCH_AUTOHUNT)
// Timer for auto search PVC
struct callout_s autoSearchPVC_ch;
#endif
#endif

// Mason Yu
#ifdef PORT_FORWARD_ADVANCE
const char *PFW_Gategory[] = {"VPN", "Game"};
const char *PFW_Rule[] = {"PPTP", "L2TP"};
#endif

//keep the same order with ITF_T
//if want to convert lan device name to br0, call LANDEVNAME2BR0() macro
const char *strItf[] = {
	"",		//ITF_ALL
	"",		//ITF_WAN
	"br0",		//ITF_LAN

	"eth0",		//ITF_ETH0
	"eth0_sw0",	//ITF_ETH0_SW0
	"eth0_sw1",	//ITF_ETH0_SW1
	"eth0_sw2",	//ITF_ETH0_SW2
	"eth0_sw3",	//ITF_ETH0_SW3

	"wlan0",	//ITF_WLAN0
	"wlan0-vap0",	//ITF_WLAN0_VAP0
	"wlan0-vap1",	//ITF_WLAN0_VAP1
	"wlan0-vap2",	//ITF_WLAN0_VAP2
	"wlan0-vap3",	//ITF_WLAN0_VAP3

	"wlan1",	//ITF_WLAN0
	"wlan1-vap0",	//ITF_WLAN0_VAP0
	"wlan1-vap1",	//ITF_WLAN0_VAP1
	"wlan1-vap2",	//ITF_WLAN0_VAP2
	"wlan1-vap3",	//ITF_WLAN0_VAP3

	"usb0",		//ITF_USB0

	""		//ITF_END
};

int IfName2ItfId(char *s)
{
	int i;
	if( !s || s[0]==0 ) return ITF_ALL;
	if( (strncmp(s, ALIASNAME_PPP, strlen(ALIASNAME_PPP))==0) || (strncmp(s, ALIASNAME_VC, strlen(ALIASNAME_VC))==0)
		|| (strncmp(s, ALIASNAME_NAS, strlen(ALIASNAME_NAS))==0) || (strncmp(s, ALIASNAME_PTM, strlen(ALIASNAME_PTM))==0))
		return ITF_WAN;

	for( i=0;i<ITF_END;i++ )
	{
		if( strcmp( strItf[i],s )==0 ) return i;
	}

	return -1;
}

// Mason Yu. 090903
int do_ioctl(unsigned int cmd, struct ifreq *ifr)
{
	int skfd, ret;

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return (-1);
	}

	ret = ioctl(skfd, cmd, ifr);
	close(skfd);
	return ret;
}

/*
 *	Check if a host is direct connected to local interfaces.
 *	pEntry: check with local interface pEntry; null to check all local interfaces.
 */
int isDirectConnect(struct in_addr *haddr, MIB_CE_ATM_VC_Tp pEntry)
{
	char buff[256];
	int flgs;
	struct in_addr dest, mask;
	FILE *fp;

	if (pEntry) {
		dest = *((struct in_addr *)pEntry->ipAddr);
		mask = *((struct in_addr *)pEntry->netMask);
		if ((dest.s_addr & mask.s_addr) == (haddr->s_addr & mask.s_addr)) {
			//printf("dest=0x%x, mask=0x%x\n", dest.s_addr, mask.s_addr);
			return 1;
		}
	}
	else { // pEntry == NULL
		if (!(fp = fopen("/proc/net/route", "r"))) {
			printf("Error: cannot open /proc/net/route - continuing...\n");
			return 0;
		}
		fgets(buff, sizeof(buff), fp);
		while (fgets(buff, sizeof(buff), fp) != NULL) {
			if (sscanf(buff, "%*s%x%*x%x%*d%*d%*d%x", &dest, &flgs, &mask) != 3) {
				printf("Unsuported kernel route format\n");
				fclose(fp);
				return 0;
			}
			if ((flgs & RTF_UP) && mask.s_addr != 0) {
				if ((dest.s_addr & mask.s_addr) == (haddr->s_addr & mask.s_addr)) {
					//printf("dest=0x%x, mask=0x%x\n", dest.s_addr, mask.s_addr);
					fclose(fp);
					return 1;
				}
			}
		}
		fclose(fp);
	}
	return 0;
}

/*
 * Get Interface Addr (MAC, IP, Mask)
 */
int getInAddr(char *interface, ADDR_T type, void *pAddr)
{
	struct ifreq ifr;
	int found=0;
	struct sockaddr_in *addr;

	strcpy(ifr.ifr_name, interface);
	if (do_ioctl(SIOCGIFFLAGS, &ifr) < 0)
		return (0);

	if (type == HW_ADDR) {
		if (do_ioctl(SIOCGIFHWADDR, &ifr) >= 0) {
			memcpy(pAddr, &ifr.ifr_hwaddr, sizeof(struct sockaddr));
			found = 1;
		}
	}
	else if (type == IP_ADDR) {
		if (do_ioctl(SIOCGIFADDR, &ifr) == 0) {
			addr = ((struct sockaddr_in *)&ifr.ifr_addr);
			*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
			found = 1;
		}
	}
	else if (type == DST_IP_ADDR) {
		if (do_ioctl(SIOCGIFDSTADDR, &ifr) == 0) {
			addr = ((struct sockaddr_in *)&ifr.ifr_addr);
			*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
			found = 1;
		}
	}
	else if (type == SUBNET_MASK) {
		if (do_ioctl(SIOCGIFNETMASK, &ifr) >= 0) {
			addr = ((struct sockaddr_in *)&ifr.ifr_addr);
			*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
			found = 1;
		}
	}
	return found;
}

int getInFlags(char *interface, int *flags)
{
	struct ifreq ifr;
	int found=0;

#ifdef EMBED
	strcpy(ifr.ifr_name, interface);

	if (do_ioctl(SIOCGIFFLAGS, &ifr) == 0) {
		if (flags)
			*flags = ifr.ifr_flags;
		found = 1;
	}
#endif
	return found;
}

int setInFlags(char *interface, int flags)
{
	struct ifreq ifr;
	int ret=0;

#ifdef EMBED
	strcpy(ifr.ifr_name, interface);
	ifr.ifr_flags = flags;

	if (do_ioctl(SIOCSIFFLAGS, &ifr) == 0)
		ret = 1;
#endif
	return ret;
}

int INET_resolve(char *name, struct sockaddr *sa)
{
	struct sockaddr_in *s_in = (struct sockaddr_in *)sa;

	s_in->sin_family = AF_INET;
	s_in->sin_port = 0;

	/* Default is special, meaning 0.0.0.0. */
	if (strcmp(name, "default")==0) {
		s_in->sin_addr.s_addr = INADDR_ANY;
		return 1;
	}
	/* Look to see if it's a dotted quad. */
	if (inet_aton(name, &s_in->sin_addr)) {
		return 0;
	}
	/* guess not.. */
	return -1;
}

enum { ROUTE_ADD, ROUTE_DEL };
/*
 *	Add a route
 */
static int route_modify(struct rtentry *rt, int action)
{
	int skfd;

	if (rt==0)
		return -1;

	rt->rt_flags = RTF_UP;
	if (((struct sockaddr_in *)(&rt->rt_gateway))->sin_addr.s_addr)
		rt->rt_flags |= RTF_GATEWAY;

	/* Create a socket to the INET kernel. */
	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	// Added by Mason Yu. According the netmask, we input the correct dst address.
	((struct sockaddr_in *)(&rt->rt_dst))->sin_addr.s_addr =
	(((struct sockaddr_in *)(&rt->rt_dst))->sin_addr.s_addr & ((struct sockaddr_in *)(&rt->rt_genmask))->sin_addr.s_addr );

	if (action == ROUTE_ADD) {
	/* Tell the kernel to accept this route. */
		if (ioctl(skfd, SIOCADDRT, rt) < 0) {
			perror("SIOCADDRT");
			close(skfd);
			return -1;
		}
	} else {
		if (ioctl(skfd, SIOCDELRT, rt) < 0) {
			perror("SIOCDELRT");
			close(skfd);
			return -1;
		}
	}

	/* Close the socket. */
	(void) close(skfd);
	return 0;

}

static inline int INET_addroute(struct rtentry *rt)
{
	return route_modify(rt, ROUTE_ADD);
}

/*
*	Del a route
*/
static inline int INET_delroute(struct rtentry *rt)
{
	return route_modify(rt, ROUTE_DEL);
}

void setup_ipforwarding(int enable)
{
	FILE *fp;
	int ipv6forward;

	if (enable != 0)
		enable = 1;
	fp = fopen(PROC_IPFORWARD, "w");
	if(fp)
	{
		fprintf(fp, "%d\n", enable);
		fclose(fp);
	}
#ifdef CONFIG_IPV6
	//  Because the /proc/.../conf/all/forwarding value, will affect /proc/.../conf/vc0/forwarding value.
	fp = fopen(PROC_IP6FORWARD, "r+");
	if(fp)
	{
		fscanf(fp,"%d",&ipv6forward);
		if(ipv6forward !=enable)
			fprintf(fp, "%d\n", enable);
		fclose(fp);
	}
#endif
}

#ifdef ROUTING
// update corresponding field of rtentry from MIB_CE_IP_ROUTE_T
static void updateRtEntry(MIB_CE_IP_ROUTE_T *pSrc, struct rtentry *pDst)
{
	struct sockaddr_in *s_in;

	//pDst->rt_flags = RTF_UP;
	s_in = (struct sockaddr_in *)&pDst->rt_dst;
	s_in->sin_family = AF_INET;
	s_in->sin_port = 0;
	s_in->sin_addr = *(struct in_addr *)pSrc->destID;

	s_in = (struct sockaddr_in *)&pDst->rt_genmask;
	s_in->sin_family = AF_INET;
	s_in->sin_port = 0;
	s_in->sin_addr = *(struct in_addr *)pSrc->netMask;

	//if (pSrc->nextHop[0]&&pSrc->nextHop[1]&&pSrc->nextHop[2]&&pSrc->nextHop[3]) {
	if (pSrc->nextHop[0] || pSrc->nextHop[1] || pSrc->nextHop[2] ||pSrc->nextHop[3]) {
		s_in = (struct sockaddr_in *)&pDst->rt_gateway;
		s_in->sin_family = AF_INET;
		s_in->sin_port = 0;
		s_in->sin_addr = *(struct in_addr *)pSrc->nextHop;
	}
}

/*del=>  1: delete the route entry,
         0: add the route entry(skip ppp part),
        -1: add the route entry*/
int route_cfg_modify(MIB_CE_IP_ROUTE_T *pRoute, int del, int entryID)
{
	struct rtentry rt;
	char ifname[IFNAMSIZ];
	int ret = 0;
	MIB_CE_ATM_VC_T vc_entry = {0};
	unsigned int nh;
	int advRoute=0;
	char destIP[16], netmask[16], toDest[64];
	int32_t tbl_id;
	char str_tblid[10]="";

	if( pRoute==NULL ) return -1;
	if(!pRoute->Enable) return -1;

	memset(&rt, 0, sizeof(rt));
	updateRtEntry(pRoute, &rt);
	rt.rt_dev = ifGetName(pRoute->ifIndex, ifname, sizeof(ifname));
	if (pRoute->FWMetric > -1)
		rt.rt_metric = pRoute->FWMetric + 1;

	nh = *(unsigned int *)pRoute->nextHop;
	if (nh == 0) { // no next hop, check interface
		// If no interface specified, return failed.
		if (!getATMVCEntryByIfIndex(pRoute->ifIndex, &vc_entry))
			return -1;
		// no next hop, so go per-interface policy routing table
		if (vc_entry.cmode == CHANNEL_MODE_IPOE && !isDefaultRouteWan(&vc_entry)) {
			strncpy(destIP, inet_ntoa(*((struct in_addr *)pRoute->destID)), 16);
			strncpy(netmask, inet_ntoa(*((struct in_addr *)pRoute->netMask)), 16);
			snprintf(toDest, 64, "%s/%s", destIP, netmask);
			tbl_id = caculate_tblid_ITF_SourceRoute(vc_entry.ifIndex);
			snprintf(str_tblid, sizeof(str_tblid), "%d", tbl_id);
			advRoute = 1;
		}
	}

	if (del>0) {
#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_ENABLE)
		// rg delete wan
		ret = RG_del_static_route(pRoute);
#endif
		if (advRoute) {
			va_cmd("/bin/ip", 4, 1, "rule", "del", "to", toDest);
		}
		else
			ret |= INET_delroute(&rt);
		return ret;
	}

#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_ENABLE)
	if(!strncmp(ifname,"ppp",3)){
		//for pppoe connection.
		struct in_addr inAddr;
		if(getInAddr(ifname,IP_ADDR,(void *)&inAddr)){
		//check if pppoe interface up!
			int wan_ifIndex=-1;
			int ret=-1;
			wan_ifIndex = getIfIndexByName(ifname);
			if (wan_ifIndex == -1){
				printf("%s-%d error get wan ifIndex",__func__,__LINE__);
				goto SKIP_RG_STATIC_ROUTE;
			}
			if(!getATMVCEntryByIfIndex(wan_ifIndex, &vc_entry)){
				printf("%s-%d error get wan ATMVC Entry",__func__,__LINE__);
				goto SKIP_RG_STATIC_ROUTE;
			}
			ret = RG_add_static_route_PPP(pRoute,&vc_entry,entryID);
		}
	}else{
		ret = rg_add_route(pRoute, entryID);
	}
	SKIP_RG_STATIC_ROUTE:
#endif
	if (advRoute) { // this route will go per-interface routing table
		va_cmd("/bin/ip", 4, 1, "rule", "del", "to", toDest);
		va_cmd("/bin/ip", 6, 1, "rule", "add", "to", toDest, "table", str_tblid);
	}
	else
		ret |= INET_addroute(&rt);

	return ret;
}
#endif

#ifdef CONFIG_IPV6
int route_v6_cfg_modify(MIB_CE_IPV6_ROUTE_T *pRoute, int del)
{
	char ifname[IFNAMSIZ];
	char metric[5];
	int ret = -1;

	if( pRoute==NULL ) return ret;
	if(!pRoute->Enable) return ret;

	ifname[0]='\0';
	ifGetName(pRoute->DstIfIndex, ifname, sizeof(ifname));
	sprintf(metric,"%d",pRoute->FWMetric);

	if(pRoute->DstIfIndex== DUMMY_IFINDEX)
	{
		if (del>0) {
			ret = va_cmd("/bin/route", 8, 1,
					"-A", "inet6", "del", pRoute->Dstination, "gw",  pRoute->NextHop, "metric", metric);
			return ret;
		}

		//route -A inet6 add  2003::/3 gw 2003::1 metric 10 dev nas0_0
		ret = va_cmd("/bin/route", 8, 1,
				"-A", "inet6", "add", pRoute->Dstination, "gw",  pRoute->NextHop, "metric", metric);
	}
	else
	{
		if (del>0) {
			ret = va_cmd("/bin/route", 10, 1,
					"-A", "inet6", "del", pRoute->Dstination, "gw",  pRoute->NextHop, "metric", metric,"dev", ifname);
			return ret;
		}

		ret = va_cmd("/bin/route", 10, 1,
				"-A", "inet6", "add", pRoute->Dstination, "gw",  pRoute->NextHop, "metric", metric,"dev", ifname);
	}

	return ret;
}
#endif

#ifdef ROUTING
void route_ppp_ifup(unsigned long pppGW, char *ifname)
{
	unsigned int entryNum, i;
	char ifname0[IFNAMSIZ];//, *ifname;
	MIB_CE_IP_ROUTE_T Entry;
	struct rtentry rt;
#ifdef CONFIG_USER_CWMP_TR069
	int cwmp_msgid;
	struct cwmp_message cwmpmsg;
#endif

	entryNum = mib_chain_total(MIB_IP_ROUTE_TBL);

	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_IP_ROUTE_TBL, i, (void *)&Entry))
		{
			continue;
		}
		if( !Entry.Enable ) continue;

		memset(&rt, 0, sizeof(rt));

		rt.rt_dev = ifGetName(Entry.ifIndex, ifname0, sizeof(ifname0));

		if (rt.rt_dev && !strncmp(rt.rt_dev, "ppp", 3)) {
			updateRtEntry(&Entry, &rt);
			rt.rt_metric = Entry.FWMetric + 1;
			route_modify(&rt, ROUTE_ADD);
		}
		else if (Entry.ifIndex == DUMMY_IFINDEX) {	// Interface "any"
			struct in_addr *addr;
			addr = (struct in_addr *)&Entry.nextHop;
			if (addr->s_addr == pppGW) {
				updateRtEntry(&Entry, &rt);
				rt.rt_metric = Entry.FWMetric + 1;
				route_modify(&rt, ROUTE_ADD);
			}
		}
	}

#ifdef CONFIG_USER_CWMP_TR069
	memset(&cwmpmsg,0,sizeof(struct cwmp_message));
	if((cwmp_msgid = msgget((key_t)1234, 0)) > 0 )
	{
		cwmpmsg.msg_type = MSG_ACTIVE_NOTIFY;
		msgsnd(cwmp_msgid, (void *)&cwmpmsg, MSG_SIZE, 0);
	}
#endif
}
#endif

/*
* Convert ifIndex to system interface name, e.g. eth0,vc0...
*/
char *ifGetName(int ifindex, char *buffer, unsigned int len)
{
	MEDIA_TYPE_T mType;

	if ( ifindex == DUMMY_IFINDEX )
		return 0;
	if (PPP_INDEX(ifindex) == DUMMY_PPP_INDEX)
	{
		mType = MEDIA_INDEX(ifindex);
		if (mType == MEDIA_ATM) {
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			if (VC_MINOR_INDEX(ifindex) == DUMMY_VC_MINOR_INDEX) // major vc devname
				snprintf( buffer, len,  "%s%u", ALIASNAME_VC, VC_MAJOR_INDEX(ifindex));
			else
				snprintf( buffer, len,  "%s%u_%u", ALIASNAME_VC, VC_MAJOR_INDEX(ifindex),  VC_MINOR_INDEX(ifindex));
#else
			snprintf( buffer, len,  "%s%u", ALIASNAME_VC, VC_INDEX(ifindex) );
#endif
		}
		else if (mType == MEDIA_ETH)
#ifdef CONFIG_RTL_MULTI_ETH_WAN
			snprintf( buffer, len, "%s%d", ALIASNAME_MWNAS, ETH_INDEX(ifindex));
#else
			snprintf( buffer, len,  "%s%u", ALIASNAME_NAS, ETH_INDEX(ifindex) );
#endif
#ifdef CONFIG_PTMWAN
		else if (mType == MEDIA_PTM)
			snprintf( buffer, len, "%s%d", ALIASNAME_MWPTM, PTM_INDEX(ifindex));
#endif /*CONFIG_PTMWAN*/
#ifdef WLAN_WISP
		else if (mType == MEDIA_WLAN)
			snprintf( buffer, len, "wlan%d-vxd", ETH_INDEX(ifindex));
#endif
		else if (mType == MEDIA_IPIP)    // Mason Yu. Add VPN ifIndex
			snprintf( buffer, len, "ipip%u", IPIP_INDEX(ifindex));
		else
			return 0;

	}else{
		snprintf( buffer, len,  "ppp%u", PPP_INDEX(ifindex) );

	}
	return buffer;
}

int getNameByIP(char *ip, char *buffer, unsigned int len)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ];
	struct in_addr inAddr;
	char *itfIP;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
  			printf("getNameByIP: Get chain record error!\n");
			return 0;
		}

		if (Entry.enable == 0)
			continue;

		ifGetName(Entry.ifIndex,ifname,sizeof(ifname));
		if (getInAddr(ifname, IP_ADDR, (void *)&inAddr) == 1) {
			itfIP = inet_ntoa(inAddr);
			if(!strcmp(itfIP, ip)){
				strncpy(buffer, ifname, len);
				buffer[len-1]='\0';
				//printf("getNameByIP: The %s IPv4 address is %s. Found\n", ifname, itfIP);
				break;
			}
		}
	}

	if(i>= entryNum){
		printf("getNameByIP: not find this interface!\n");
		return 0;
	}

	return 1;
}

int getIfIndexByName(char *pIfname)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ];

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
  			printf("Get chain record error!\n");
			return -1;
		}

		if (Entry.enable == 0)
			continue;

		ifGetName(Entry.ifIndex,ifname,sizeof(ifname));

		if(!strcmp(ifname,pIfname)){
			break;
		}
	}

	if(i>= entryNum){
		//printf("not find this interface!\n");
		return -1;
	}

	return(Entry.ifIndex);
}
/*ericchung add, get applicationtype by interface name */
int getapplicationtypeByName(char *pIfname)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ];

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
  			printf("Get chain record error!\n");
			return -1;
		}

		if (Entry.enable == 0)
			continue;

		ifGetName(Entry.ifIndex,ifname,sizeof(ifname));

		if(!strcmp(ifname,pIfname)){
			break;
		}
	}

	if(i> entryNum){
		printf("not find this interface!\n");
		return -1;
	}

	return(Entry.applicationtype);
}

#ifdef CONFIG_HWNAT
static inline int setEthPortMapping(struct ifreq *ifr)
{
	int ret=0;

#ifdef EMBED
	if (do_ioctl(SIOCSITFGROUP, ifr) == 0)
		ret = 1;

#endif
	return ret;
}
#endif

#ifdef CONFIG_USER_CWMP_TR069
int set_TR069_Firewall(int enable)
{
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_TR069);

	if (enable)
	{
		unsigned int conreq_port = 0;
		char strPort[8] = {0}, ifname[IFNAMSIZ] = {0};
		mib_get(CWMP_CONREQ_PORT, (void *)&conreq_port);
		if (conreq_port == 0) 
			conreq_port = 7547;

		sprintf(strPort, "%u", conreq_port);

#ifdef CONFIG_USER_RTK_WAN_CTYPE
		int i = 0, total = mib_chain_total(MIB_ATM_VC_TBL);
		MIB_CE_ATM_VC_T entry;
	
		for (i = 0; i < total; i++)
		{
			if (mib_chain_get(MIB_ATM_VC_TBL, i, &entry) == 0)
				continue;
	
			if (entry.applicationtype & X_CT_SRV_TR069)
			{
				ifGetName(entry.ifIndex, ifname, IFNAMSIZ);
				break;
			}
		}
#endif

		// LAN side
		// iptables -A tr069 -i br0 -p tcp --dport 7547 -j REJECT --reject-with tcp-reset
		va_cmd(IPTABLES, 12, 1, "-A", IPTABLE_TR069, (char *)ARG_I, (char *)LANIF, "-p", "tcp", (char *)FW_DPORT, strPort, "-j", "REJECT", "--reject-with", "tcp-reset");

		// WAN side
#ifdef CONFIG_USER_RTK_WAN_CTYPE
		if (strlen(ifname) > 0)
		{
			// iptables -A tr069 ! -i nas0_0 -p tcp --dport 7547 -j REJECT --reject-with tcp-reset
			va_cmd(IPTABLES, 13, 1, "-A", IPTABLE_TR069, "!", (char *)ARG_I, ifname, "-p", "tcp", (char *)FW_DPORT, strPort, "-j", "REJECT", "--reject-with", "tcp-reset");
		}
#endif
	}
	return 0;
}
#endif

unsigned int hextol(unsigned char *hex)
{
	//printf("%02x %02x %02x %02x\n",hex[0], hex[1], hex[2], hex[3]);
	return ( (hex[0] << 24) | (hex[1] << 16) | (hex[2] << 8) | (hex[3]));
}

#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_ENABLE)
void handleIGMPandMLD(int isIGMPenable, int isMLDenable)
{
	/*
	 * romDriver version after from 1984.
	 *
	 * ?��?romeDriver保�??��??�igmpSnooping總�??��?
	 *
	 * ?��?多新增�???proc/rg/mcast_protocol，可?�制?�獨?��?igmp?�mld?�兩?��??��?�?	 * echo 0 > /proc/rg/mcast_protocol => IGMP_MLD_Both
	 * echo 1 > /proc/rg/mcast_protocol => IGMP_only
	 * echo 2 > /proc/rg/mcast_protocol => MLD_only
	 */
	unsigned char mode;

	printf("%s: isIGMPenable:%d, isMLDenable:%d\n",__func__,isIGMPenable,isMLDenable);

	mib_get(MIB_MPMODE, (void *)&mode);
	/*
	* if IGMP/MLD snooping state change, then we restart IGMP/MLD proxy
	* for reset rg_flow_idx that igmpproxy/ecmh have recorded
	* & flush the MC HW flow we for according IP version
	*/
#if defined(CONFIG_USER_IGMPPROXY)
	if(!!(mode&MP_IGMP_MASK) != isIGMPenable) {
		AUG_PRT("Restart IGMP proxy only\n");
#ifdef CONFIG_IGMPPROXY_MULTIWAN
		setting_Igmproxy();
#else
		startIgmproxy();
#endif
	}
#endif
#if defined(CONFIG_IPV6) && defined(CONFIG_USER_ECMH)
	if(!!(mode&MP_MLD_MASK) != isMLDenable) {
		AUG_PRT("Restart MLD proxy only\n");
		startMLDproxy();
	}
#endif
	/*
	* if both IGMP snooping & proxy disabled
	* we change mcast_protocol to MLD only
	* and do not trap IGMP to PS anymore
	* vice versa
	*/
	RTK_RG_multicastFlow_reset();
	RTK_RG_Ipv6_multicastFlow_reset();
	checkIGMPMLDProxySnooping(isIGMPenable, isMLDenable, isIgmproxyEnabled(), isMLDProxyEnabled());
}

int isIGMPSnoopingEnabled(void)
{
	unsigned char mode;

	mib_get(MIB_MPMODE, (void *)&mode);
	return ((mode&MP_IGMP_MASK)==MP_IGMP_MASK)?1:0;
}

int isMLDSnoopingEnabled(void)
{
	unsigned char mode;

	mib_get(MIB_MPMODE, (void *)&mode);
	return ((mode&MP_MLD_MASK)==MP_MLD_MASK)?1:0;
}

#ifdef ROUTING
#ifdef CONFIG_00R0
int isRouteExist(struct in_addr ipddr, struct in_addr netmask, struct in_addr gwaddr)
{
	char buff[256];
	int flgs;
	unsigned long dest, gateway, mask;
	FILE *fp;
	if (!(fp = fopen("/proc/net/route", "r"))) {
		printf("Error: cannot open /proc/net/route - continuing...\n");
		return 0;
	}
	fgets(buff, sizeof(buff), fp);
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		if (sscanf(buff, "%*s%x%x%x%*d%*d%*d%x", &dest, &gateway, &flgs, &mask) != 4) {
			printf("Unsuported kernel route format\n");
			fclose(fp);
			return 0;
		}

		if ((flgs & RTF_UP) && mask != 0) {
			if (((dest & mask) == (ipddr.s_addr & netmask.s_addr)) &&
				 (gateway == gwaddr.s_addr )) {
					fclose(fp);
					return 1;
			}
		}
	}
	fclose(fp);
	return 0;
}
#endif
int rg_add_route(MIB_CE_IP_ROUTE_T *entry, int entryID)
{
	int i, n, ret;
	FILE *fp;
	int flags;
	uint32_t mask, gateway;
	uint32_t dest, tmask, nH;
	char buf[256], tdevice[256], device[256];
	struct sockaddr hwaddr;
	char mac_str[20]={0};
	fp = fopen("/proc/net/route", "r");
	n = 0;
	mask = 0;
	device[0] = '\0';
	nH = hextol(entry->nextHop);
//printf("%s-%d nH=%x\n",__func__,__LINE__,nH);
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		++n;
		if (n == 1 && strncmp(buf, "Iface", 5) == 0)
			continue;
		i = sscanf(buf, "%255s %lx %lx %x %*s %*s %*s %x",
					tdevice, &dest, &gateway, &flags, &tmask);
//printf("tdevice:%s, dest=%x, gateway=%x, tmask=%x\n",tdevice,dest,gateway,tmask);
		if ((nH & tmask) == dest
		 && (tmask > mask || mask == 0) && (flags & RTF_UP) && (dest != 0x7f000000) && (dest !=0)
		) {
			mask = tmask;
			strcpy(device, tdevice);
//printf("[%s-%d] n:%d tdevice:%s, dest=%x, gateway=%x, tmask=%x\n",__func__,__LINE__,n,tdevice,dest,gateway,tmask);
			break;
		}
	}
	fclose(fp);
	if (device[0] == '\0') {
		printf("can't find interface");
		ret = 0;
	} else {
		getInAddr(device, HW_ADDR, &hwaddr);
		sprintf( mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
		(unsigned char)hwaddr.sa_data[0], (unsigned char)hwaddr.sa_data[1], (unsigned char)hwaddr.sa_data[2],
		(unsigned char)hwaddr.sa_data[3], (unsigned char)hwaddr.sa_data[4], (unsigned char)hwaddr.sa_data[5] );
//		printf("%s-%d mac_str:%s\n",__func__,__LINE__,mac_str);
		ret = RG_add_static_route(entry,(char*)hwaddr.sa_data,entryID);
	}

	return ret;
}
/*use in reboot time to add static route*/
int Flush_RG_static_route(void)
{
 	unsigned int totalEntry = mib_chain_total(MIB_IP_ROUTE_TBL); /* get chain record size */
	MIB_CE_IP_ROUTE_T Entry;
	int i;
	//AUG_PRT("%s-%d totalEntry=%d\n",__func__,__LINE__,totalEntry);
		for (i=0; i<totalEntry; i++) {
			if (!mib_chain_get(MIB_IP_ROUTE_TBL, i, (void *)&Entry))
				continue;
	//AUG_PRT("%s-%d Entry.ifIndex=%x\n",__func__,__LINE__,Entry.ifIndex);
			RG_del_static_route(&Entry);
			mib_chain_update(MIB_IP_ROUTE_TBL,(void *)&Entry,i);
		}
	return 0;
}

int check_RG_static_route(void)
{
 	unsigned int totalEntry = mib_chain_total(MIB_IP_ROUTE_TBL); /* get chain record size */
	MIB_CE_IP_ROUTE_T Entry;
	int i;

	for (i=0; i<totalEntry; i++) {
		if (!mib_chain_get(MIB_IP_ROUTE_TBL, i, (void *)&Entry))
			continue;
		route_cfg_modify(&Entry,0,i);
	}

	return 1;
}
int check_RG_static_route_PPP(MIB_CE_ATM_VC_T *vc_entry)
{
 	unsigned int totalEntry = mib_chain_total(MIB_IP_ROUTE_TBL); /* get chain record size */
	MIB_CE_IP_ROUTE_T Entry;
	int i;
	for (i=0; i<totalEntry; i++) {
		if (!mib_chain_get(MIB_IP_ROUTE_TBL, i, (void *)&Entry))
			return 0;
		//if user assign specify out interface (pppoe)
		if(Entry.ifIndex == vc_entry->ifIndex){
			RG_add_static_route_PPP(&Entry,vc_entry,i);
		}
	}
	return 1;
}
#endif
#endif

#if defined(CONFIG_RTL_IGMP_SNOOPING)
// enable/disable IGMP snooping
void __dev_setupIGMPSnoop(int flag)
{
	struct ifreq ifr;
	struct ifvlan ifvl;
	unsigned char mode;

#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_ENABLE)
		mib_get(MIB_MPMODE, (void *)&mode);
		handleIGMPandMLD(flag, ((mode&MP_MLD_MASK)==MP_MLD_MASK)?1:0);
#else
	if(flag){
		system("/bin/echo 1 > /proc/br_igmpsnoop");
		system("/bin/echo 1 > /proc/br_igmpquery");
	}
	else{
		system("/bin/echo 0 > /proc/br_igmpsnoop");
		system("/bin/echo 0 > /proc/br_igmpquery");
	}
#endif
	printf("IGMP Snooping: %s\n", flag?"enabled":"disabled");
}
#endif

// Mason Yu. MLD snooping
//#ifdef CONFIG_IPV6
#if defined(CONFIG_RTL_MLD_SNOOPING)
// enable/disable MLD snooping
void __dev_setupMLDSnoop(int flag)
{
	unsigned char mode;

#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_ENABLE)
	mib_get(MIB_MPMODE, (void *)&mode);
	handleIGMPandMLD(((mode&MP_IGMP_MASK)==MP_IGMP_MASK)?1:0, flag);
#else
	if(flag){
		system("/bin/echo 1 > /proc/br_mldsnoop");
		system("/bin/echo 1 > /proc/br_mldquery");
	}
	else{
		system("/bin/echo 0 > /proc/br_mldsnoop");
		system("/bin/echo 0 > /proc/br_mldquery");
	}
#endif
	printf("MLD Snooping: %s\n", flag?"enabled":"disabled");
}
#endif

// ioctl for direct bridge mode, jiunming
void __dev_setupDirectBridge(int flag)
{
	struct ifreq ifr;
	int ret;

	strcpy(ifr.ifr_name, ELANIF);
	ifr.ifr_ifru.ifru_ivalue = flag;
#ifdef EMBED
#if defined(CONFIG_RTL8672NIC)
	if( do_ioctl(SIOCDIRECTBR, &ifr)==0 ) {
		ret = 1;
		//printf("Set direct bridge mode %s!\n", flag?"enable":"disable" );
	}
	else {
		ret = 0;
		//printf("Set direct bridge mode error!\n");
	}
#endif
#endif

}

// Mason Yu, For Set IPQOS
//ql 20081117 START for ip qos
#ifdef CONFIG_USER_IP_QOS
#ifdef CONFIG_HWNAT
int hwnat_wanif_rule(void)
{
	hwnat_ioctl_cmd hifr;
	struct hwnat_ioctl_qos_cmd hiqc;
	struct rtl867x_hwnat_qos_rule *qos_rule;
	MIB_CE_ATM_VC_T Entry;
	int vcTotal, i;
	char ipWanif[IFNAMSIZ];
	memset(&hiqc, 0, sizeof(hiqc));
	hifr.type = HWNAT_IOCTL_QOS_TYPE;
	hifr.data = &hiqc;
	hiqc.type = QOSRULE_CMD;
	qos_rule = &(hiqc.u.qos_rule.qos_rule_pattern);
	unsigned int portmask;


	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);

	for (i = 0; i < vcTotal; i++){

		portmask=0;
		mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry);
		if(Entry.vlan==0)
			continue;
		if(Entry.vprio==0)
			continue;
		if(Entry.cmode != CHANNEL_MODE_BRIDGE)
			continue;
		ifGetName(Entry.ifIndex, ipWanif, sizeof(ipWanif));
		{
		int var;
		for(var = 0; var < 14; var ++)
		portmask|=Entry.itfGroup & (0x1 << var) ;
		}
		memcpy(qos_rule->outIfname, ipWanif, strlen(ipWanif));
		qos_rule->rule_type = RTL867x_IPQos_Format_dstIntf_1pRemark;
		hiqc.u.qos_rule.qos_rule_remark_8021p = Entry.vprio;	// 802.1p: 0 means disalbe
		//q_index
		hiqc.u.qos_rule.qos_rule_queue_index = 3;
		send_to_hwnat(hifr);
	}



}

static int hwnat_single_queue_setup(void)
{
	hwnat_ioctl_cmd hifr;
	struct hwnat_ioctl_qos_cmd hiqc;
	struct rtl867x_hwnat_qos_queue *qos_queue;

	memset(&hiqc, 0, sizeof(hiqc));
	hifr.type = HWNAT_IOCTL_QOS_TYPE;
	hifr.data = &hiqc;
	hiqc.type = OUTPUTQ_CMD;
	qos_queue = &(hiqc.u.qos_queue);
	qos_queue->action = QDISC_ENABLE;
	qos_queue->sp_num = 1;
	//qos_queue->bandwidth = -1;
	qos_queue->default_queue = 0;
	send_to_hwnat(hifr);
	return 0;
}

int setWanIF1PMark(void)
{
	int vcTotal, i;

	MIB_CE_ATM_VC_T Entry;
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	int idx;
	unsigned int HWQosEnable=0;
	unsigned int WanIF1PEnable=0;

	for (i = 0; i < vcTotal; i++)
	{
		mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry);

		if(Entry.vlan==0)
			continue;
		if(Entry.vprio==0)
			continue;
		WanIF1PEnable=1;
		if(Entry.enableIpQos)
			HWQosEnable=1;
	}
	if(WanIF1PEnable)
	if(!HWQosEnable){
		//AUG_PRT("HWQOSDisable\n");
		//hwnat_single_queue_setup();
		hwnat_wanif_rule();
	}
	return 1;
}
#else
int hwnat_wanif_rule(void) {}
int setWanIF1PMark(void) {}
#endif
#endif

// enable/disable IPQoS
void __dev_setupIPQoS(int flag)
{
//ql 20081117 START for ip qos
#ifdef CONFIG_USER_IP_QOS
	if ( flag == 0 )
		va_cmd("/bin/ethctl", 2, 1, "setipqos", "0");
	else if (flag == 1)
		va_cmd("/bin/ethctl", 2, 1, "setipqos", "1");
#endif
}

#ifdef QOS_DIFFSERV
static char* proto2layer2[] = {
    [0]" ",
    [1]"6",
    [2]"17",
    [3]"1",
};

static char* strPolicing[] = {
    [0]" ",
    [1]"drop",
    [2]"continue",
};

void cleanupDiffservRule(int idx)
{
	unsigned int num, i;
	MIB_CE_IP_QOS_T qEntry;
	char wanif[IFNAMSIZ];

	mib_chain_get(MIB_IP_QOS_TBL, idx, (void *)&qEntry);
	ifGetName(qEntry.ifIndex, wanif, sizeof(wanif));

	// tc qdisc del dev vc0 root
	va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", wanif, "root");
}

#if 0
int deleteDiffservEntry()
{
	unsigned int totalEntry, i;
	MIB_CE_IP_QOS_T entry;

	totalEntry = mib_chain_total(MIB_IP_QOS_TBL); /* get chain record size */
	// Delete all existed diffserv entry
	for (i = totalEntry - 1; i >= 0; i ++) {
		if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void *)&entry)) {
			printf("%s\n", errGetEntry);
			return 1;
		}
		if (entry.enDiffserv == 1) {
			// delete from chain record
			if (mib_chain_delete(MIB_IP_QOS_TBL, i) != 1) {
				printf("Delete MIB_IP_QOS_TBLchain record error!\n");
				return 1;
			}
		}
	}

	return 0;
}
#endif

static void diffserv_filter_rule(MIB_CE_IP_QOS_Tp qEntry, char *prio, char *classid)
{
	char *argv[60], wanif[IFNAMSIZ];
	char saddr[20], daddr[20], sport[6], dport[6], strdscp[6] = {0}, strUpLinkRate[10];
	char *psaddr, *pdaddr;
	int idx, i;

	ifGetName(qEntry->ifIndex, wanif, sizeof(wanif));

	// source ip, mask
	snprintf(saddr, 20, "%s", inet_ntoa(*((struct in_addr *)qEntry->sip)));
	if (strcmp(saddr, ARG_0x4) == 0)
		psaddr = 0;
	else {
		if (qEntry->smaskbit!=0)
			snprintf(saddr, 20, "%s/%d", saddr, qEntry->smaskbit);
		psaddr = saddr;
	}
	// destination ip, mask
	snprintf(daddr, 20, "%s", inet_ntoa(*((struct in_addr *)qEntry->dip)));
	if (strcmp(daddr, ARG_0x4) == 0)
		pdaddr = 0;
	else {
		if (qEntry->dmaskbit!=0)
			snprintf(daddr, 20, "%s/%d", daddr, qEntry->dmaskbit);
		pdaddr = daddr;
	}
	snprintf(sport, 6, "%d", qEntry->sPort);
	snprintf(dport, 6, "%d", qEntry->dPort);

	// Classifier setup for 1:0
	// tc filter add dev vc0 parent 1:0 protocol ip prio 1 u32
	//	match ip src 192.168.1.3/32 match ip dst 192.168.8.11/32
	//	match ip tos 0x38 0xff match ip protocol 6 0xff
	//	match ip sport 1090 0xff match ip dport 21 0xff
	//	police rate 500kbit burst 10k drop classid 1:1
	argv[1] = "filter";
	argv[2] = (char *)ARG_ADD;
	argv[3] = "dev";
	argv[4] = wanif;
	argv[5] = "parent";
	argv[6] = "1:0";
	argv[7] = "protocol";
	argv[8] = "ip";
	argv[9] = "prio";
	argv[10] = prio;
	argv[11] = "u32";
	idx = 12;

	// match filter
	// src ip
	if (psaddr != 0) {
		argv[idx++] = "match";
		argv[idx++] = "ip";
		argv[idx++] = "src";
		argv[idx++] = psaddr;
	}
	// dst ip
	if (pdaddr != 0) {
		argv[idx++] = "match";
		argv[idx++] = "ip";
		argv[idx++] = "dst";
		argv[idx++] = pdaddr;
	}
	//dscp match
#ifdef QOS_DSCP_MATCH
	if (0 != qEntry->qosDscp) {
		argv[idx++] = "match";
		argv[idx++] = "ip";
		argv[idx++] = "tos";
		snprintf(strdscp, 6, "0x%x", (qEntry->qosDscp-1)&0xFF);
		argv[idx++] = strdscp;
		argv[idx++] = "0xff";
	}
#endif
	// protocol
	if (qEntry->protoType != PROTO_NONE) {
		argv[idx++] = "match";
		argv[idx++] = "ip";
		argv[idx++] = "protocol";
		argv[idx++] = proto2layer2[qEntry->protoType];
		argv[idx++] = "0xff";
	}
	// src port
	if ((qEntry->protoType==PROTO_TCP ||
		qEntry->protoType==PROTO_UDP) &&
		qEntry->sPort != 0) {
		argv[idx++] = "match";
		argv[idx++] = "ip";
		argv[idx++] = "sport";
		argv[idx++] = sport;
		argv[idx++] = "0xff";
	}
	// dst port
	if ((qEntry->protoType==PROTO_TCP ||
		qEntry->protoType==PROTO_UDP) &&
		qEntry->dPort != 0) {
		argv[idx++] = "match";
		argv[idx++] = "ip";
		argv[idx++] = "dport";
		argv[idx++] = dport;
		argv[idx++] = "0xff";
	}

	// police
	if (0 != qEntry->limitSpeed) {
		argv[idx++] = "police";
		argv[idx++] = "rate";
		snprintf(strUpLinkRate, 10, "%dKbit", qEntry->limitSpeed);
		argv[idx++] = strUpLinkRate;
		argv[idx++] = "burst";
		argv[idx++] = "10k";
		argv[idx++] = strPolicing[qEntry->policing];
	}

	argv[idx++] = "classid";
	argv[idx++] = classid;
	argv[idx++] = NULL;

	printf("%s", TC);
	for (i=1; i<idx-1; i++)
		printf(" %s", argv[i]);
	printf("\n");
	do_cmd(TC, argv, 1);
}

static void diffserv_HTB_bw_div(MIB_CE_IP_QOS_Tp qEntry, char *classid)
{
	unsigned short ceil, rateBE;
	char wanif[IFNAMSIZ], strCeil[10], strRate[10], strRateBE[10];
	int htbrate = 0;

	ifGetName(qEntry->ifIndex, wanif, sizeof(wanif));

	//patch: actual bandwidth maybe a little greater than configured limit value, so I minish 7% of the configured limit value ahead.
	//ceil = qEntry->totalBandwidth / 100 * 93;
	ceil = qEntry->totalBandwidth;
	htbrate = (qEntry->htbRate>=qEntry->totalBandwidth)?qEntry->totalBandwidth-100:qEntry->htbRate;
	rateBE = ceil - htbrate;
	snprintf(strCeil, 10, "%dKbit", ceil);
	snprintf(strRate, 10, "%dKbit", htbrate);
	snprintf(strRateBE, 10, "%dKbit", rateBE);

	// tc qdisc add dev vc0 parent 1:0 handle 2:0 htb
	va_cmd(TC, 9, 1, "qdisc", (char *)ARG_ADD, "dev", wanif, "parent", "1:0", "handle", "2:0", "htb");
	// tc class add dev vc0 parent 2:0 classid 2:1 htb rate 1Mbit ceil 1Mbit
	va_cmd(TC, 13, 1, "class", (char *)ARG_ADD, "dev", wanif, "parent", "2:0", "classid", "2:1", "htb", "rate", strCeil, "ceil", strCeil);
	// tc class add dev vc0 parent 2:1 classid 2:2 htb rate 1Mbit ceil 1Mbit
	va_cmd(TC, 13, 1, "class", (char *)ARG_ADD, "dev", wanif, "parent", "2:1", "classid", "2:2", "htb", "rate", strRate, "ceil", strCeil);
	// tc class add dev vc0 parent 2:1 classid 2:3 htb rate 1Mbit ceil 1Mbit
	va_cmd(TC, 13, 1, "class", (char *)ARG_ADD, "dev", wanif, "parent", "2:1", "classid", classid, "htb", "rate", strRateBE, "ceil", strCeil);
}

static void calculate_RED(MIB_CE_IP_QOS_Tp qEntry, char *strLimit, char *strMin, char *strMax, char *strBurst)
{
	int max, min, limit, burst;

	max = (int)(qEntry->totalBandwidth / 8 * qEntry->latency / 1000);
	min = (int)(max / 3);
	limit = (int)(8 * max);
	burst = (int)(( 2 * min * 1000 + max * 1000 ) / ( 3* 1000 ));

	snprintf(strLimit, 10, "%dKB", limit);
	snprintf(strMin, 10, "%dKB", min);
	snprintf(strMax, 10, "%dKB", max);
	snprintf(strBurst, 6, "%d", burst);
}

static void diffserv_be_queue(MIB_CE_IP_QOS_Tp qEntry, char *classid)
{
	char wanif[IFNAMSIZ], strMax[10], strMin[10], strLimit[10], strBurst[6], strBw[10];

	ifGetName(qEntry->ifIndex, wanif, sizeof(wanif));
	snprintf(strBw, 10, "%dKbit", qEntry->totalBandwidth);
	calculate_RED(qEntry, strLimit, strMin, strMax, strBurst);

	// tc qdisc add dev vc0 parent 2:3 red limit 6KB min 1.5KB max 4.5KB burst 20 avpkt 1000 bandwidth 1Mbit probability 0.4
	printf("%s qdisc %s dev %s parent %s red limit %s min %s max %s burst %s avpkt 1000 bandwidth %s probability 0.4\n", TC
		, (char *)ARG_ADD, wanif, classid, strLimit, strMin, strMax, strBurst, strBw);
	va_cmd(TC, 21, 1, "qdisc", (char *)ARG_ADD, "dev", wanif, "parent", classid,
		"red", "limit", strLimit, "min", strMin, "max", strMax, "burst", strBurst,
		"avpkt", "1000", "bandwidth", strBw, "probability", "0.4");
}

static int setupEFTraffic(int efindex)
{
	MIB_CE_IP_QOS_T qEntry;
	char wanif[IFNAMSIZ], prio[2], classid[6], strdscp[6];
	int phb;

	mib_chain_get(MIB_IP_QOS_TBL, efindex, (void *)&qEntry);
	phb = qEntry.m_ipprio << 3 | qEntry.m_iptos << 1;
	ifGetName(qEntry.ifIndex, wanif, sizeof(wanif));

	// Create the egress root "dsmark" qdisc on wan interface
	// tc qdisc add dev vc0 root handle 1:0 dsmark indices 64 default_index 2
	va_cmd(TC, 12, 1, "qdisc", (char *)ARG_ADD, "dev", wanif,
		"root", "handle", "1:0", "dsmark", "indices", "64", "default_index", "2");

	snprintf(prio, 2, "1");
	snprintf(classid, 6, "1:1");

	// Classifier setup for 1:0
	diffserv_filter_rule(&qEntry, prio, classid);

	// Classes to specify DSCPs
	// tc class change dev vc0 parent 1:0 classid 1:1 dsmark mask 0x3 value 0xb8
	snprintf(strdscp, 6, "0x%x", (phb << 2)&0xFF);
	va_cmd(TC, 13, 1, "class", "change", "dev", wanif,
		"parent", "1:0", "classid", classid, "dsmark", "mask", "0x3", "value", strdscp);
	// BE
	va_cmd(TC, 13, 1, "class", "change", "dev", wanif, "parent", "1:0", "classid", "1:2", "dsmark", "mask", "0x3", "value", "0x0");

	// bandwidth division with HTB
	snprintf(classid, 6, "2:3");
	diffserv_HTB_bw_div(&qEntry, classid);

	// queue setup
	// tc qdisc add dev vc0 parent 2:2 pfifo limit 5
	va_cmd(TC, 9, 1, "qdisc", (char *)ARG_ADD, "dev", wanif, "parent", "2:2", "pfifo", "limit", "5");
	diffserv_be_queue(&qEntry, classid);

	// classifier setup for 2:1
	// tc filter add dev vc0 parent 2:0 protocol ip prio 1 handle 1 tcindex classid 2:2
	va_cmd(TC, 15, 1, "filter", (char *)ARG_ADD, "dev", wanif, "parent", "2:0", "protocol", "ip",
		"prio", "1", "handle", "1", "tcindex", "classid", "2:2");
	// tc filter add dev vc0 parent 2:0 protocol ip prio 2 handle 2 tcindex classid 2:3
	va_cmd(TC, 15, 1, "filter", (char *)ARG_ADD, "dev", wanif, "parent", "2:0", "protocol", "ip",
		"prio", "2", "handle", "2", "tcindex", "classid", classid);

	return 0;
}

static int setupAFTraffic(int *afindex, int aftotal, char *wanif)
{
	MIB_CE_IP_QOS_T qEntry;
	char prio[2], classid[6], strdscp[6];
	int i, phb;
	//int htbrate = 0;
	char strMax[10], strMin[10], strLimit[10], strBurst[6], strBw[10], strDP[2], strProbability[6];

	// Create the root "dsmark" qdisc on wan interface
	// tc qdisc add dev vc0 handle 1:0 root dsmark indices 64
	va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", wanif,
		"handle", "1:0", "root", "dsmark", "indices", "64");

	// dsmark classes to specify DSCPs
	// tc class change dev vc0 parent 1:0 classid 1:11 dsmark mask 0x3 value 0x28
	for (i = 0; i < aftotal; i ++) {
		mib_chain_get(MIB_IP_QOS_TBL, afindex[i], (void *)&qEntry);
		phb = qEntry.m_ipprio << 3 | qEntry.m_iptos << 1;
		snprintf(strdscp, 6, "0x%x", (phb << 2)&0xFF);
		snprintf(classid, 6, "1:%d%d", qEntry.m_ipprio, qEntry.m_iptos);
		va_cmd(TC, 13, 1, "class", "change", "dev", wanif,
			"parent", "1:0", "classid", classid, "dsmark", "mask", "0x3", "value", strdscp);
	}
	// BE
	va_cmd(TC, 13, 1, "class", "change", "dev", wanif, "parent", "1:0", "classid", "1:5", "dsmark", "mask", "0x3", "value", "0x0");

	// Classifier setup for 1:0
	for (i = 0; i < aftotal; i ++) {
		mib_chain_get(MIB_IP_QOS_TBL, afindex[i], (void *)&qEntry);
		snprintf(prio, 2, "%d", i + 1);
		snprintf(classid, 6, "1:%d%d", qEntry.m_ipprio, qEntry.m_iptos);
		//htbrate += qEntry.limitSpeed;
		diffserv_filter_rule(&qEntry, prio, classid);
	}
	// BE: tc filter add dev vc0 parent 1:0 protocol ip prio 5 u32 match ip protocol 0 0 flowid 1:5
	va_cmd(TC, 18, 1, "filter", (char *)ARG_ADD, "dev", wanif,
		"parent", "1:0", "protocol", "ip", "prio", "5", "u32", "match", "ip", "protocol", "0", "0", "flowid", "1:5");

	// bandwidth division with HTB
	snprintf(classid, 6, "2:6");
	//htbrate = (htbrate>qEntry.totalBandwidth)?qEntry.totalBandwidth-100:htbrate;
	diffserv_HTB_bw_div(&qEntry, classid);

	// queue setup
	// GRED setup for AF class
	// tc qdisc add dev vc0 parent 2:2 gred setup DPs 3 default 2 grio
	va_cmd(TC, 13, 1, "qdisc", (char *)ARG_ADD, "dev", wanif,
		"parent", "2:2", "gred", "setup", "DPs", "3", "default", "2", "grio");
	// tc qdisc change dev vc0 parent 2:2 gred limit 6KB min 1.5KB max 4.5KB burst 20 avpkt 1000 bandwidth 1Mbit DP 1 probability 0.02 prio 2
	snprintf(strBw, 10, "%dKbit", qEntry.totalBandwidth);
	for (i = 0; i < 3; i ++) {
		calculate_RED(&qEntry, strLimit, strMin, strMax, strBurst);
		snprintf(strDP, 2, "%d", i + 1);
		snprintf(strProbability, 6, "0.0%d", 2 * (i + 1));
		snprintf(prio, 2, "%d", i + 2);
		va_cmd(TC, 25, 1, "qdisc", "change", "dev", wanif, "parent", "2:2",
			"gred", "limit", strLimit, "min", strMin, "max", strMax, "burst", strBurst,
			"avpkt", "1000", "bandwidth", strBw, "DP", strDP, "probability", strProbability, "prio", prio);
		printf("%s qdisc change dev %s parent 2:2 gred limit %s min %s max %s burst %s avpkt 1000 bandwidth %s DP %s probability %s prio %s\n"
			, TC, wanif, strLimit, strMin, strMax, strBurst, strBw, strDP, strProbability, prio);
	}
	// RED setup for BE
	diffserv_be_queue(&qEntry, classid);

	// classifier setup for 2:1
	// tc filter add dev vc0 parent 2:0 protocol ip prio 1 handle 17 tcindex classid 2:2
	for (i = 0; i < aftotal; i ++) {
		mib_chain_get(MIB_IP_QOS_TBL, afindex[i], (void *)&qEntry);
		snprintf(strdscp, 6, "%d", qEntry.m_ipprio * 16 + qEntry.m_iptos);
		va_cmd(TC, 15, 1, "filter", (char *)ARG_ADD, "dev", wanif, "parent", "2:0", "protocol", "ip",
			"prio", "1", "handle", strdscp, "tcindex", "classid", "2:2");
	}
	// tc filter add dev vc0 parent 2:0 protocol ip prio 1 handle 5 tcindex classid 2:3
	va_cmd(TC, 15, 1, "filter", (char *)ARG_ADD, "dev", wanif, "parent", "2:0", "protocol", "ip",
		"prio", "1", "handle", "5", "tcindex", "classid", classid);

	return 0;
}

int setupDiffServ(void)
{
	unsigned int num, i;
	MIB_CE_IP_QOS_T qEntry;
	char wanif[IFNAMSIZ];
	unsigned char phbclass;
	int efIndex=0, afIndex[3], afcount = 0;

	mib_get(MIB_DIFFSERV_PHBCLASS, (void *)&phbclass);
	num = mib_chain_total(MIB_IP_QOS_TBL);
	for (i = 0; i < num; i ++) {
		if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void *)&qEntry))
			continue;
		if (qEntry.enDiffserv == 0) // non-Diffserv entry
			continue;
		//if (qEntry.m_ipprio == 5 && qEntry.m_iptos == 3) {	// EF
		if (phbclass == qEntry.m_ipprio) {
			if (phbclass == 5) {	// EF
				efIndex = i;
				break;
			}
			else {	// AF
				afIndex[afcount] = i;
				afcount ++;
				ifGetName(qEntry.ifIndex, wanif, sizeof(wanif));
			}
		}
	}
	if (num > 0) {
		if (afcount > 0)
			setupAFTraffic(afIndex, afcount, wanif);
		else
			setupEFTraffic(efIndex);
	}

	return 0;
}
#endif // #ifdef QOS_DIFFSERV

/*------------------------------------------------------------------
 * Get a list of interface info. (itfInfo) of the specified ifdomain.
 * where,
 * info: a list of interface info entries
 * len: max length of the info list
 * ifdomain: interface domain
 *-----------------------------------------------------------------*/
int get_domain_ifinfo(struct itfInfo *info, int len, int ifdomain)
{
	unsigned int swNum, vcNum;
	int i, num;
	int mib_wlan_num;
	char mygroup;
	char strlan[]="LAN0";
	char wanif[IFNAMSIZ];
	MIB_CE_ATM_VC_T pvcEntry;
	num=0;

#ifdef WLAN_SUPPORT
	int ori_wlan_idx;
#endif

	if (ifdomain&DOMAIN_ELAN) {
		// LAN ports
#ifdef IP_QOS_VPORT
		swNum = SW_LAN_PORT_NUM;
		for (i=0; i<swNum; i++) {
			strlan[3] = '0' + virt2user[i];	// user index
			info[num].ifdomain = DOMAIN_ELAN;
			info[num].ifid=i;	// virtual index
			strncpy(info[num].name, strlan, 8);
			num++;
			if (num > len)
				break;
		}
#else
		info[num].ifdomain = DOMAIN_ELAN;
		info[num].ifid=0;
		strncpy(info[num].name, strlan, 8);
		num++;
#endif
	}

#ifdef WLAN_SUPPORT
	if (ifdomain&DOMAIN_WLAN) {
		ori_wlan_idx = wlan_idx;

		for( wlan_idx = 0; wlan_idx < NUM_WLAN_INTERFACE; wlan_idx++ )
		{
			// wlan0
			mib_get(MIB_WLAN_ITF_GROUP, (void *)&mygroup);
			info[num].ifdomain = DOMAIN_WLAN;
			info[num].ifid = 0 + wlan_idx * 10;  // Magician: ifid of wlan0 = 0; ifid of wlan1 = 10
			strncpy(info[num].name, getWlanIfName(), 8);
			num++;

//jim luo add it to support QoS on Virtual AP...
#ifdef WLAN_MBSSID
			for (i=1; i<IFGROUP_NUM; i++) {
				mib_get(MIB_WLAN_VAP0_ITF_GROUP + ((i-1)<<1), (void *)&mygroup);
				info[num].ifdomain = DOMAIN_WLAN;
				info[num].ifid = i + wlan_idx * 10;  // Magician: ifid of wlan0-vap0~3 = 1~4; ifid of wlan1-vap0~3 = 11~14
				sprintf(info[num].name, "wlan%d-vap%d", wlan_idx, i-WLAN_VAP_ITF_INDEX);
				num++;
			}
#endif  // WLAN_MBSSID
		}
		wlan_idx = ori_wlan_idx;
	}
#endif  // WLAN_SUPPORT

#ifdef CONFIG_USB_ETH
	if (ifdomain&DOMAIN_ULAN) {
		// usb0
		mib_get(MIB_USBETH_ITF_GROUP, (void *)&mygroup);
		info[num].ifdomain = DOMAIN_ULAN;
		info[num].ifid=0;
		sprintf(info[num].name, "%s", USBETHIF );
		num++;
	}
#endif //CONFIG_USB_ETH

	if (ifdomain&DOMAIN_WAN) {
		// vc
		vcNum = mib_chain_total(MIB_ATM_VC_TBL);

		for (i=0; i<vcNum; i++) {
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&pvcEntry))
			{
  				//boaError(wp, 400, "Get chain record error!\n");
  				printf("Get chain record error!\n");
				return 0;
			}

			if (pvcEntry.enable == 0)
				continue;

			info[num].ifdomain = DOMAIN_WAN;
			info[num].ifid=pvcEntry.ifIndex;
			ifGetName(pvcEntry.ifIndex, wanif, sizeof(wanif));
			strncpy(info[num].name, wanif, 8);
			num++;

			if (num > len)
				break;
		}
	}

	return num;
}

int read_pid(const char *filename)
{
	FILE *fp;
	int pid = 0;

	if ((fp = fopen(filename, "r")) == NULL)
		return -1;
	if(fscanf(fp, "%d", &pid) != 1 || kill(pid, 0) != 0){
		pid = 0;
	}
	fclose(fp);

	return pid;
}

/*
 * Kill process by pidfile.
 * Return pid on success, <=0 if pid not exist.
 */
int kill_by_pidfile(const char *pidfile, int sig)
{
	pid_t pid;
	
	pid = read_pid(pidfile);
	if (pid <= 0)
		return pid;
	kill(pid, sig);
	// wait for process termination	
	while (kill(pid, 0) == 0) {
		usleep(100000);
		//printf("wait for process(%s) termination\n", pidfile);
		kill(pid, SIGKILL);	//force terminate
	}
	unlink(pidfile);
	return pid;
}

// Added by Kaohj
//return 0:OK, other:fail
int do_cmd(const char *filename, char *argv[], int dowait)
{
	pid_t pid, wpid;
	int ret, status;
	sigset_t tmpset, origset;

	sigfillset(&tmpset);
	sigprocmask(SIG_BLOCK, &tmpset, &origset);
	pid = vfork();
	sigprocmask(SIG_SETMASK, &origset, NULL);

	if (pid == 0) {
		/* the child */
//#ifndef CONFIG_LUNA
#if 0
		char *env[3];
#endif

		signal(SIGINT, SIG_IGN);
		argv[0] = (char *)filename;
//#ifndef CONFIG_LUNA
#if 0
		env[0] = "PATH=/bin:/usr/bin:/etc:/sbin:/usr/sbin";
		env[1] = NULL;
#endif

//#ifndef CONFIG_LUNA
#if 0
		execve(filename, argv, env);
#else
		execv(filename, argv);
#endif

		fprintf(stderr, "exec %s failed\n", filename);
		_exit(EXIT_FAILURE);
	} else if (pid > 0) {
		if (!dowait)
			ret = 0;
		else {
			/* parent, wait till rc process dies before spawning */
			while ((wpid = waitpid(pid,&status,0)) != pid) {
				if (wpid == -1 && errno == ECHILD) {	/* see wait(2) manpage */
					break;
				}
			}

			ret = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
		}
	} else if (pid < 0) {
		fprintf(stderr, "fork of %s failed\n", filename);
		ret = -1;
	}

	return ret;
}

// Run a command and output result to file if specified
//return 0:OK, other:fail
int do_cmd_fout(const char *filename, char *argv[], int dowait, char *output)
{
	pid_t pid, wpid;
	int ret=0, status;
	sigset_t tmpset, origset;

	sigfillset(&tmpset);
	sigprocmask(SIG_BLOCK, &tmpset, &origset);
	pid = vfork();
	sigprocmask(SIG_SETMASK, &origset, NULL);

	if (pid == 0)
	{
		/* the child */
		signal(SIGINT, SIG_IGN);
		argv[0] = (char *)filename;

		if(output)
		{
			int fd;

			if((fd = open(output, O_RDWR | O_CREAT))==-1)
			{
				fprintf(stderr, "<%s:%d> open() failed\n", __FUNCTION__, __LINE__);
				_exit(EXIT_FAILURE);
			}

			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
			close(fd);
		}

		if(dowait)
		{
			execv(filename, argv);

			fprintf(stderr, "exec %s failed\n", filename);
			_exit(EXIT_FAILURE);
		}
		else
		{
			pid = vfork();
			if(pid < 0)
			{
				fprintf(stderr, "fork of %s failed\n", filename);
				ret = -1;
			}
			else if(pid == 0)
			{
				//grandson
				execv(filename, argv);

				fprintf(stderr, "exec %s failed\n", filename);
				_exit(EXIT_FAILURE);
			}
			else
			{
				//child
				exit(0);
			}
		}
	} else if (pid > 0) {
		if (!dowait)
			waitpid (pid, NULL, 0);	//child will exit after fork
		else {
			/* parent, wait till rc process dies before spawning */
			while ((wpid = waitpid(pid,&status,0)) != pid) {
				if (wpid == -1 && errno == ECHILD) {	/* see wait(2) manpage */
					break;
				}
			}

			ret = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
		}
	} else if (pid < 0) {
		fprintf(stderr, "fork of %s failed\n", filename);
		ret = -1;
	}

	return ret;
}


//return 0:OK, other:fail
int va_cmd(const char *cmd, int num, int dowait, ...)
{
	va_list ap;
	int k;
	char *s;
	char *argv[32];
	int status;

	TRACE(STA_SCRIPT, "%s ", cmd);
	va_start(ap, dowait);

	for (k=0; k<num; k++)
	{
		s = va_arg(ap, char *);
		argv[k+1] = s;
		TRACE(STA_SCRIPT|STA_NOTAG, "%s ", s);
	}

	TRACE(STA_SCRIPT|STA_NOTAG, "\n");
	argv[k+1] = NULL;
	status = do_cmd(cmd, argv, dowait);
	va_end(ap);

	return status;
}

//return 0:OK, other:fail
int do_cmd_ex(const char *filename, char *argv[], int dowait, int noError)
{
	pid_t pid, wpid;
	int ret, status;
	sigset_t tmpset, origset;

	sigfillset(&tmpset);
	sigprocmask(SIG_BLOCK, &tmpset, &origset);
	pid = vfork();
	sigprocmask(SIG_SETMASK, &origset, NULL);

	if (pid == 0) {
		/* the child */
#ifndef CONFIG_LUNA
		char *env[3];
#endif

		signal(SIGINT, SIG_IGN);
		argv[0] = (char *)filename;
#ifndef CONFIG_LUNA
		env[0] = "PATH=/bin:/usr/bin:/etc:/sbin:/usr/sbin";
		env[1] = NULL;
#endif
		//no error output
		if(noError==1){
			int fd = open("/dev/null", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			dup2(fd, 1);
			dup2(fd, 2);
			close(fd);
		}
#ifndef CONFIG_LUNA
		execve(filename, argv, env);
#else
		execv(filename, argv);
#endif

		fprintf(stderr, "exec %s failed\n", filename);
		_exit(EXIT_FAILURE);
	} else if (pid > 0) {
		if (!dowait)
			ret = 0;
		else {
			/* parent, wait till rc process dies before spawning */
			while ((wpid = waitpid(pid,&status,0)) != pid) {
				if (wpid == -1 && errno == ECHILD) {	/* see wait(2) manpage */
					break;
				}
			}

			ret = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
		}
	} else if (pid < 0) {
		fprintf(stderr, "fork of %s failed\n", filename);
		ret = -1;
	}

	return ret;
}
//return 0:OK, other:fail
int va_cmd_no_error(const char *cmd, int num, int dowait, ...)
{
	va_list ap;
	int k;
	char *s;
	char *argv[24];
	int status;

	TRACE(STA_SCRIPT, "%s ", cmd);
	va_start(ap, dowait);

	for (k=0; k<num; k++)
	{
		s = va_arg(ap, char *);
		argv[k+1] = s;
		TRACE(STA_SCRIPT|STA_NOTAG, "%s ", s);
	}

	TRACE(STA_SCRIPT|STA_NOTAG, "\n");
	argv[k+1] = NULL;
	status = do_cmd_ex(cmd, argv, dowait, 1);
	va_end(ap);

	return status;
}


//return 0:OK, other:fail
/*same function as va_cmd(). Execute silently.
  No print out command string in console.
*/
int va_cmd_no_echo(const char *cmd, int num, int dowait, ...)
{
	va_list ap;
	int k;
	char *s;
	char *argv[24];
	int status;

	va_start(ap, dowait);

	for (k=0; k<num; k++)
	{
		s = va_arg(ap, char *);
		argv[k+1] = s;
	}

	argv[k+1] = NULL;
	status = do_cmd(cmd, argv, dowait);
	va_end(ap);

	return status;
}

//return 0:OK, other:fail
int call_cmd(const char *filename, int num, int dowait, ...)
{
	va_list ap;
	char *s;
	char *argv[24];
	int status=0, st, k;
	pid_t pid, wpid;

	va_start(ap, dowait);

	for (k=0; k<num; k++)
	{
		s = va_arg(ap, char *);
		argv[k+1] = s;
	}

	argv[k+1] = NULL;
	if((pid = vfork()) == 0) {
		/* the child */
		char *env[3];

		signal(SIGINT, SIG_IGN);
		argv[0] = (char *)filename;
		env[0] = "PATH=/bin:/usr/bin:/etc:/sbin:/usr/sbin";
		env[1] = NULL;

		execve(filename, argv, env);

		printf("exec %s failed\n", filename);
		_exit(2);
	} else if(pid > 0) {
		if (!dowait)
			status = 0;
		else {
			/* parent, wait till rc process dies before spawning */
			while ((wpid = wait(&st)) != pid)
				if (wpid == -1 && errno == ECHILD) { /* see wait(2) manpage */
					break;
				}
		}
	} else if(pid < 0) {
		printf("fork of %s failed\n", filename);
		status = -1;
	}
	if (wpid>0)
		if (WIFEXITED(st))
			status = WEXITSTATUS(st);
	va_end(ap);

	return status;
}

#ifdef CONFIG_PPP
void write_to_pppd(struct data_to_pass_st *pmsg)
{
#if defined(CONFIG_LUNA) && defined(GEN_WAN_MAC)
	//Chuck: use system call to trigger spppd main.c pause(), fit for ql_xu performance tune fix
	system(pmsg->data);
#else
	int pppd_fifo_fd=-1;

	pppd_fifo_fd = open(PPPD_FIFO, O_WRONLY);
	if (pppd_fifo_fd == -1)
		fprintf(stderr, "Sorry, no spppd server\n");
	else
	{
		write(pppd_fifo_fd, pmsg, sizeof(*pmsg));
		close(pppd_fifo_fd);
	}
#endif
}
#endif

// return value:
// 0  : successful
// -1 : failed
int write_to_mpoad(struct data_to_pass_st *pmsg)
{
	int mpoad_fifo_fd=-1;
	int status=0;
	int ret=0;
#ifdef CONFIG_DEV_xDSL
	mpoad_fifo_fd = open(MPOAD_FIFO, O_WRONLY);
	if (mpoad_fifo_fd == -1) {
		fprintf(stderr, "Sorry, no mpoad server\n");
		status = -1;
	} else
	{
		// Modified by Mason Yu
		//write(mpoad_fifo_fd, pmsg, sizeof(*pmsg));
REWRITE:
		ret = write(mpoad_fifo_fd, pmsg, sizeof(*pmsg));
		if(ret<0 && errno==EPIPE)
			 goto REWRITE;
#ifdef _LINUX_2_6_
		//sleep more time for mpoad to wake up
		usleep(100*1000);
#else
		usleep(30*1000);
#endif

		close(mpoad_fifo_fd);
		// wait server to consume it
#ifdef _LINUX_2_6_
		//sleep more time for mpoad to wake up
		usleep(100*1000);
#else
		usleep(1000);
#endif
	}
#endif // CONFIG_DEV_xDSL
	return status;
}

void WRITE_DHCPC_FILE(int fh, unsigned char *buf)
{
	if ( write(fh, buf, strlen(buf)) != strlen(buf) ) {
		printf("Write udhcpc script file error!\n");
		close(fh);
	}
}

#ifdef CONFIG_USER_DHCPCLIENT_MODE
int setupDHCPClient(void){
	int status=0;
	unsigned char value[64];
	unsigned int count=0;

	/* run dhcpc daemon on bridge interface */
	startDhcpc(ALIASNAME_BR0, NULL);

err:
	return status;
}

static void writeDhcpClientScript(char *fname)
{
	int fh;
	int mark;
	char buff[64];
	unsigned char lanIP[16]={0};
	fh = open(fname, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);

	if (fh == -1) {
		printf("Create udhcpc script file %s error!\n", fname);
		return;
	}

	WRITE_DHCPC_FILE(fh, "#!/bin/sh\n");
	snprintf(buff, 64, "RESOLV_CONF=\"/var/udhcpc/resolv.conf.$interface\"\n");
	WRITE_DHCPC_FILE(fh, buff);

	if (!getMIB2Str(MIB_ADSL_LAN_IP, lanIP)) {
		snprintf(buff, 64, "ORIG_LAN_IP=\"%s\"\n", lanIP);
		WRITE_DHCPC_FILE(fh, buff);
	}

	WRITE_DHCPC_FILE(fh, "[ \"$broadcast\" ] && BROADCAST=\"broadcast $broadcast\"\n");
	WRITE_DHCPC_FILE(fh, "[ \"$subnet\" ] && NETMASK=\"netmask $subnet\"\n");
	WRITE_DHCPC_FILE(fh, "ifconfig $interface mtu 1500\n");
	WRITE_DHCPC_FILE(fh, "echo interface=$interface\n");
	WRITE_DHCPC_FILE(fh, "echo ciaddr=$ciaddr\n");
	WRITE_DHCPC_FILE(fh, "echo ip=$ip\n");
	WRITE_DHCPC_FILE(fh, "if [ \"$ip\" != \"$ciaddr\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "  echo clear_ip\n");
	WRITE_DHCPC_FILE(fh, "  ifconfig $interface 0.0.0.0\n");
	WRITE_DHCPC_FILE(fh, "fi\n");
	WRITE_DHCPC_FILE(fh, "ifconfig $interface $ip $BROADCAST $NETMASK -pointopoint\n");

	/**************************** Important *****************************/
	/* Should set policy route before write resolv.conf, or there are   */
	/* still some packets may go to wrong pvc.                          */
	/*************************************** ****************************/
	WRITE_DHCPC_FILE(fh, "ip ro add default via $router dev $interface\n\n");

	WRITE_DHCPC_FILE(fh, "if [ \"$dns\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "  rm $RESOLV_CONF\n");
	WRITE_DHCPC_FILE(fh, "  for i in $dns\n");
	WRITE_DHCPC_FILE(fh, "    do\n");
	WRITE_DHCPC_FILE(fh, "      if [ $i = '0.0.0.0' ]|| [ $i = '255.255.255.255' ]; then\n");
	WRITE_DHCPC_FILE(fh, "        continue\n");
	WRITE_DHCPC_FILE(fh, "      fi\n");
	WRITE_DHCPC_FILE(fh, "      echo 'DNS=' $i\n");
	//WRITE_DHCPC_FILE(fh, "\techo nameserver $i >> $RESOLV_CONF\n");
	// echo 192.168.88.21@192.168.99.100
	snprintf(buff, 64, "      echo $i@$ip >> $RESOLV_CONF\n");
	WRITE_DHCPC_FILE(fh, buff);
	WRITE_DHCPC_FILE(fh, "    done\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

	//Microsoft AUTO IP in udhcp-0.9.9-pre2/dhcpc.c, compare first 8 char with "169.254." prefix
	WRITE_DHCPC_FILE(fh, "if [ \"${ip:0:8}\" = \"169.254.\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "  echo \"DHCP sever no response ==> recover to original LAN IP($ORIG_LAN_IP)\"\n");
	WRITE_DHCPC_FILE(fh, "  ifconfig $interface 0.0.0.0\n");
	WRITE_DHCPC_FILE(fh, "  ifconfig $interface $ORIG_LAN_IP\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

	close(fh);
}
#endif //CONFIG_USER_DHCPCLIENT_MODE

static void write_to_dhcpc_script(char *fname, MIB_CE_ATM_VC_Tp pEntry)
{
	int fh;
	int mark;
	char buff[64];
#ifdef DEFAULT_GATEWAY_V2
	unsigned int dgw;
#endif
#ifdef IP_POLICY_ROUTING
	int i, num, found;
	MIB_CE_IP_QOS_T qEntry;
#endif
#ifdef NEW_PORTMAPPING
	char iproutecmd[80];
	int  tableId;
#endif
	char ifname[30];

	ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));

	fh = open(fname, O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);

	if (fh == -1) {
		printf("Create udhcpc script file %s error!\n", fname);
		return;
	}

	WRITE_DHCPC_FILE(fh, "#!/bin/sh\n");
	snprintf(buff, 64, "RESOLV_CONF=\"/var/udhcpc/resolv.conf.%s\"\n", ifname);
	WRITE_DHCPC_FILE(fh, buff);
	WRITE_DHCPC_FILE(fh, "[ \"$broadcast\" ] && BROADCAST=\"broadcast $broadcast\"\n");
	WRITE_DHCPC_FILE(fh, "[ \"$subnet\" ] && NETMASK=\"netmask $subnet\"\n");
	WRITE_DHCPC_FILE(fh, "MER_GW_INFO=\"/tmp/MERgw.\"$interface\n");	// Jenny, write MER1483 gateway info
	WRITE_DHCPC_FILE(fh, "echo $router > $MER_GW_INFO\n");
	snprintf(buff, 64, "ifconfig $interface mtu %d\n", pEntry->mtu);
	WRITE_DHCPC_FILE(fh, buff);
	WRITE_DHCPC_FILE(fh, "echo interface=$interface\n");
	WRITE_DHCPC_FILE(fh, "echo ciaddr=$ciaddr\n");
	WRITE_DHCPC_FILE(fh, "echo ip=$ip\n");
	WRITE_DHCPC_FILE(fh, "if [ \"$ip\" != \"$ciaddr\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "echo clear_ip\n");
	WRITE_DHCPC_FILE(fh, "ifconfig $interface 0.0.0.0\n");
	WRITE_DHCPC_FILE(fh, "fi\n");
        WRITE_DHCPC_FILE(fh, "ifconfig $interface $ip $BROADCAST $NETMASK -pointopoint\n");

	if (isDefaultRouteWan(pEntry))
	{
		WRITE_DHCPC_FILE(fh, "if [ \"$router\" ]; then\n");
#ifdef DEFAULT_GATEWAY_V2
		if (ifExistedDGW() == 1)	// Jenny, delete existed default gateway first
			WRITE_DHCPC_FILE(fh, "\troute del default\n");
#endif
		WRITE_DHCPC_FILE(fh, "\twhile route del -net default gw 0.0.0.0 dev $interface\n");
		WRITE_DHCPC_FILE(fh, "\tdo :\n");
		WRITE_DHCPC_FILE(fh, "\tdone\n\n");
		WRITE_DHCPC_FILE(fh, "\tfor i in $router\n");
		WRITE_DHCPC_FILE(fh, "\tdo\n");
//		WRITE_DHCPC_FILE(fh, "\tifconfig $interface pointopoint $i\n");
		WRITE_DHCPC_FILE(fh, "\troute add -net default gw $i dev $interface\n");
#ifdef NEW_PORTMAPPING
		tableId = caculate_tblid(pEntry->ifIndex);
		snprintf(iproutecmd, 80, "\tip route add $netip/$mask via $ip table %d\n",tableId);
		WRITE_DHCPC_FILE(fh, iproutecmd);
		snprintf(iproutecmd, 80, "\tip route add default dev $interface via $i table %d\n", tableId);	
		WRITE_DHCPC_FILE(fh, iproutecmd);
#endif
		WRITE_DHCPC_FILE(fh, "\tdone\n");
//		WRITE_DHCPC_FILE(fh, "\tifconfig $interface -pointopoint\n");
		WRITE_DHCPC_FILE(fh, "fi\n");
	}
	else {
#ifdef DEFAULT_GATEWAY_V2	// Jenny, assign default gateway by remote WAN IP
		unsigned char dgwip[16];
		if (mib_get(MIB_ADSL_WAN_DGW_ITF, (void *)&dgw) != 0) {
			if (dgw == DGW_NONE && getMIB2Str(MIB_ADSL_WAN_DGW_IP, dgwip) == 0) {
				if (ifExistedDGW() == 1)
					WRITE_DHCPC_FILE(fh, "\troute del default\n");
				// route add default gw remotip
				snprintf(buff, 64, "\troute add default gw %s\n", dgwip);
				WRITE_DHCPC_FILE(fh, buff);
			}
		}
#endif
#ifdef NEW_PORTMAPPING
		tableId = caculate_tblid(pEntry->ifIndex);
		WRITE_DHCPC_FILE(fh, "if [ \"$router\" ]; then\n");
		snprintf(iproutecmd, 80, "\tip route del default table %d\n", tableId);
		WRITE_DHCPC_FILE(fh, iproutecmd);
		WRITE_DHCPC_FILE(fh, "\tfor i in $router\n");
		WRITE_DHCPC_FILE(fh, "\tdo\n");
		snprintf(iproutecmd, 80, "\tip route add $netip/$mask via $ip table %d\n", tableId);
		WRITE_DHCPC_FILE(fh, iproutecmd);
		snprintf(iproutecmd, 80, "\tip route add default dev $interface via $i table %d\n", tableId);
		WRITE_DHCPC_FILE(fh, iproutecmd);
		WRITE_DHCPC_FILE(fh, "\tdone\n");
		WRITE_DHCPC_FILE(fh, "fi\n");
#endif
	}

#ifdef CONFIG_USER_RTK_WAN_CTYPE
	if (pEntry->applicationtype&X_CT_SRV_TR069)
	{
		char cmd[64];

		//snprintf(ifname,sizeof(ifname),"vc%d",VC_INDEX(pEntry->ifIndex));
		sprintf(buff,"%s.%s", DHCPC_ROUTERFILE, ifname);
		WRITE_DHCPC_FILE(fh, "if [ \"$router\" ]; then\n");
		WRITE_DHCPC_FILE(fh, "\tfor i in $router\n");
		WRITE_DHCPC_FILE(fh, "\tdo\n");
		unlink(buff);
		sprintf(cmd,"\techo $router > %s\n",buff);
		WRITE_DHCPC_FILE(fh, cmd);
		WRITE_DHCPC_FILE(fh, "\tdone\n");
		WRITE_DHCPC_FILE(fh, "fi\n");
	}
#endif

	/**************************** Important *****************************/
	/* Should set policy route before write resolv.conf, or there are   */
	/* still some packets may go to wrong pvc.                          */
	/*************************************** ****************************/
	set_dhcp_source_route(fh, pEntry);

	WRITE_DHCPC_FILE(fh, "if [ \"$dns\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "\trm $RESOLV_CONF\n");
	WRITE_DHCPC_FILE(fh, "\tfor i in $dns\n");
	WRITE_DHCPC_FILE(fh, "\tdo\n");
	WRITE_DHCPC_FILE(fh, "\tif [ $i = '0.0.0.0' ]|| [ $i = '255.255.255.255' ]; then\n");
	WRITE_DHCPC_FILE(fh, "\t\tcontinue\n");
	WRITE_DHCPC_FILE(fh, "\tfi\n");
	WRITE_DHCPC_FILE(fh, "\techo 'DNS=' $i\n");
	//WRITE_DHCPC_FILE(fh, "\techo nameserver $i >> $RESOLV_CONF\n");
	// echo 192.168.88.21@192.168.99.100
	snprintf(buff, 64, "\techo $i@$ip >> $RESOLV_CONF\n");
	WRITE_DHCPC_FILE(fh, buff);
	WRITE_DHCPC_FILE(fh, "\tdone\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

#if defined(PORT_FORWARD_GENERAL) || defined(DMZ)
#ifdef NAT_LOOPBACK
	set_dhcp_NATLB(fh, pEntry);
#endif
#endif

#ifdef IP_POLICY_ROUTING
	num = mib_chain_total(MIB_IP_QOS_TBL);
	found = 0;
	// set advanced-routing rule
	for (i=0; i<num; i++) {
		if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void *)&qEntry))
			continue;
#ifdef QOS_DIFFSERV
		if (qEntry.enDiffserv == 1) // Diffserv entry
			continue;
#endif
		if (qEntry.outif == pEntry->ifIndex) {

			found = 1;
			mark = get_classification_mark(i);
			if (mark != 0) {
				WRITE_DHCPC_FILE(fh, "if [ \"$router\" ]; then\n");
				snprintf(buff, 64, "\tip ru add fwmark %x table %d\n", mark, VC_INDEX(pEntry->ifIndex)+PR_VC_START);
				WRITE_DHCPC_FILE(fh, buff);
			}
		}
	}
	if (found) {
		snprintf(buff, 64, "\tip ro add default via $router dev $interface table %d\n", VC_INDEX(pEntry->ifIndex)+PR_VC_START);
		WRITE_DHCPC_FILE(fh, buff);
		WRITE_DHCPC_FILE(fh, "fi\n");
	}
#endif

#ifdef CONFIG_CWMP_TR181_SUPPORT
	// Collect DHCP Information for Device.DHCPv4.Client.{i}.
	WRITE_DHCPC_FILE(fh, "\nINFO_FILE=\"/tmp/udhcpc_info.$interface\"\n");
	WRITE_DHCPC_FILE(fh, "rm $INFO_FILE\n");

	// IP Address
	WRITE_DHCPC_FILE(fh, "if [ \"$ip\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "	echo ip=$ip >> $INFO_FILE\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

	// DHCP Server
	WRITE_DHCPC_FILE(fh, "if [ \"$siaddr\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "	echo siaddr=$siaddr >> $INFO_FILE\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

	// Gateway info
	WRITE_DHCPC_FILE(fh, "if [ \"$router\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "	echo router=$router >> $INFO_FILE\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

	// DNS Servers
	WRITE_DHCPC_FILE(fh, "DNS=''\n");
	WRITE_DHCPC_FILE(fh, "if [ \"$dns\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "	for i in $dns\n");
	WRITE_DHCPC_FILE(fh, "	do\n");
	WRITE_DHCPC_FILE(fh, "		DNS=\"$DNS$i,\"\n");
	WRITE_DHCPC_FILE(fh, "	done\n");
	WRITE_DHCPC_FILE(fh, "	echo dns=$DNS >> $INFO_FILE\n");
	WRITE_DHCPC_FILE(fh, "fi\n");

	// Expire time
	WRITE_DHCPC_FILE(fh, "if [ \"$expire\" ]; then\n");
	WRITE_DHCPC_FILE(fh, "	echo expire=$expire >> $INFO_FILE\n");
	WRITE_DHCPC_FILE(fh, "fi\n");
#endif
	close(fh);
}

int set_dhcp_source_route(int fh, MIB_CE_ATM_VC_Tp pEntry)
{
	char buff[64];
	char buff2[128];
	int32_t tbl_id;
	char str_tblid[10]="";
	char iproutecmd[80];	// Mason Yu. for ACS and NTP

	//This interface is not default route
	if (!isDefaultRouteWan(pEntry))
	{
		tbl_id = caculate_tblid_ITF_SourceRoute(pEntry->ifIndex);
		snprintf(str_tblid, sizeof(str_tblid), "%d", tbl_id);

		//(1) ip route flush table 64
		snprintf(buff, 64, "\tip route flush table %s\n", str_tblid);
		WRITE_DHCPC_FILE(fh, buff);

		// (2) ip rule del fwmark 1 table 200

		// (3) Add default route for policy route table
		WRITE_DHCPC_FILE(fh, "if [ \"$router\" ]; then\n");
		//ip route add default dev vc0 via 192.168.1.254 table 64
		snprintf(iproutecmd, 80, "\tip route add default dev $interface via $i table %s\n", str_tblid);
		WRITE_DHCPC_FILE(fh, "\tfor i in $router\n");
		WRITE_DHCPC_FILE(fh, "\tdo\n");
		WRITE_DHCPC_FILE(fh, iproutecmd);
		WRITE_DHCPC_FILE(fh, "\tdone\n");
		WRITE_DHCPC_FILE(fh, "fi\n");

		// (4) Add net route for this policy route table
		// ip route add 192.168.8.0/24 via 192.168.8.1 table 200
		snprintf(buff2, 128, "\tip route add $netip/$mask via $ip table %s\n", str_tblid);
		WRITE_DHCPC_FILE(fh, buff2);

		// (5) Add source IP rule
		snprintf(buff2, 128, "\tip rule del from $ip table %s\n", str_tblid);		
		WRITE_DHCPC_FILE(fh, buff2);
		snprintf(buff2, 128, "\tip rule add from $ip table %s\n", str_tblid);
		WRITE_DHCPC_FILE(fh, buff2);
	}
	return 0;
}

int set_static_source_route(MIB_CE_ATM_VC_Tp pEntry)
{
	int32_t tbl_id;
	char str_tblid[10]="";
	char ifname[10];
	char *temp;
	char ipAddr[20], remoteIp[20];
	unsigned long wanmask, wanmbit, wanip, wansubnet;
	unsigned char wansubnetStr[20];
	char wansubnetStr_tmp[30];
	char wanipStr[20];

	ifGetName(pEntry->ifIndex, ifname, sizeof(ifname));
	// If this interface set DNS IP manually, and this interface is not default route
	if ( ((pEntry->cmode == CHANNEL_MODE_RT1483) || (pEntry->cmode == CHANNEL_MODE_IPOE)) &&
		((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) && !isDefaultRouteWan(pEntry))
	{
		tbl_id = caculate_tblid_ITF_SourceRoute(pEntry->ifIndex);
		snprintf(str_tblid, sizeof(str_tblid), "%d", tbl_id);

		// (1) flush source route
		//( ip route flush table 64
		va_cmd("/bin/ip", 4, 1, "route", "flush", "table", str_tblid);

		// (2) Get some WAN IP address
		temp = inet_ntoa(*((struct in_addr *)pEntry->ipAddr));
		if (strcmp(temp, "0.0.0.0"))
		{
			strcpy(ipAddr, temp);
		}

		wanip = *(unsigned long *)(pEntry->ipAddr);
		wanmask = *(unsigned long *)(pEntry->netMask);
		wansubnet = (wanip&wanmask);
		//printf("***** wanip=0x%x, wanmask=0x%x, wansubnet=0x%x\n", wanip, wanmask, wansubnet);
		strncpy(wansubnetStr, inet_ntoa(*((struct in_addr *)&wansubnet)), 16);
		wansubnetStr[15] = '\0';
		strncpy(wanipStr, inet_ntoa(*((struct in_addr *)&wanip)), 16);
		wanipStr[15] = '\0';

		wanmbit=0;
		while (1) {
			if (wanmask&0x80000000) {
				wanmbit++;
				wanmask <<= 1;
			}
			else
				break;
		}
		snprintf(wansubnetStr_tmp,30,"%s/%d",wansubnetStr,wanmbit);

		// (3) Add default route for policy route table
		temp = inet_ntoa(*((struct in_addr *)pEntry->remoteIpAddr));
		if (strcmp(temp, "0.0.0.0"))
		{
			strcpy(remoteIp, temp);
			//ip route add default dev vc0 via 192.168.1.254 table 64
			va_cmd("/bin/ip", 9, 1, "route", "add", "default", "dev", ifname, "via", remoteIp, "table", str_tblid);
		}

		// (4) Add net route for this policy route table
		// ip route add 192.168.8.0/24 via 192.168.8.1 table 200
		if (pEntry->cmode != CHANNEL_MODE_RT1483) // p-to-p link do not need interface route.
			va_cmd("/bin/ip", 7, 1, "route", "add", wansubnetStr_tmp, "via", ipAddr, "table", str_tblid);

		// (5) Add source rule from this interface
		// delete rule before adding it.
		// ip rule del table 64
		va_cmd("/bin/ip", 6, 1, "rule", "del", "from", wanipStr, "table", str_tblid);
		va_cmd("/bin/ip", 6, 1, "rule", "add", "from", wanipStr, "table", str_tblid);
	}
	return 0;
}

#ifdef CONFIG_IPV6
int set_ipv6_static_source_route(MIB_CE_ATM_VC_T *pEntry, char *ifname, struct in6_addr * ip6addr)
{
	int32_t tbl_id;
	unsigned int ifIndex;
	char str_tblid[10]={0};
	char SourceRouteIp[64]={0};
	int applicationtype=0;
	struct ipv6_ifaddr ipv6_addr;
	int numOfIpv6;

	if(!ifname || !ip6addr){
		printf("Error! NULL input!\n");
		return -1;
	}
	inet_ntop(AF_INET6, (const void *)ip6addr, SourceRouteIp, INET6_ADDRSTRLEN);
	AUG_PRT("ifname=%s pEntry->dgw=%d SourceRouteIp=%s\n",ifname,pEntry->dgw,SourceRouteIp);

	//This interface is not default route
	if ( pEntry->dgw== 0)
	{
		struct in6_addr prefix = {0};
		const char *dst = NULL;
		char	buf[64]={0};
		char	*buf1 = NULL;
		ifIndex = getIfIndexByName(ifname);
		applicationtype=getapplicationtypeByName(ifname);
		tbl_id = caculate_tblid_ITF_SourceRoute(ifIndex);

		snprintf(str_tblid, sizeof(str_tblid), "%d", tbl_id);

		//ip -6 route flush table 64
		va_cmd("/bin/ip", 5, 1, "-6", "route", "flush", "table", str_tblid);
		// ip -6 rule del table 64
		va_cmd("/bin/ip", 5, 1, "-6", "rule", "del", "table", str_tblid);

		inet_ntop(AF_INET6, (const void *)pEntry->RemoteIpv6Addr, SourceRouteIp, INET6_ADDRSTRLEN);

		//del default route
		sprintf(buf,"echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr",ifname);
		AUG_PRT("buf=%s\n",buf);

		system(buf);

		if(!strcmp(SourceRouteIp,"::")){
			//remote gateway not get yet!
			AUG_PRT("remote gateway not get yet ::\n");

		}else{
			AUG_PRT("del ipv6 secondary default route %s\n",SourceRouteIp);
			//del ipv6 secondary default route
			//ip -6 route del default via fe80::a00:27ff:fe29:3e63 dev nas0_2
			va_cmd("/bin/ip", 8, 1, "-6", "route", "del", "default", "via", SourceRouteIp,"dev", ifname);
		}

		// Add source rule
		//add gateway ip
		//ip -6 rule
		//from fe80::a00:27ff:fe29:3e63 lookup 114
		//ip -6 rule add from  fe80::a00:27ff:fe29:3e63 table 114
		va_cmd("/bin/ip", 7, 1, "-6", "rule", "add", "from", SourceRouteIp,"table", str_tblid);
		//ip -6 route add default via fe80::a00:27ff:fe29:3e63 dev nas0_2 table 114
		va_cmd("/bin/ip", 10, 1, "-6", "route", "add", "default", "via", SourceRouteIp, "dev", ifname, "table", str_tblid);
		memset(&ipv6_addr, 0, sizeof(struct ipv6_ifaddr));
		//get global addr
		numOfIpv6 = getifip6(ifname, IPV6_ADDR_UNICAST, &ipv6_addr, 1);
		if(numOfIpv6 > 0){
			inet_ntop(AF_INET6, &ipv6_addr.addr, SourceRouteIp, INET6_ADDRSTRLEN);
			//AUG_PRT("global ip=%s, prefix-len=%d\n",SourceRouteIp,ipv6_addr.prefix_len);
		}
		ip6toPrefix(&ipv6_addr.addr, ipv6_addr.prefix_len, &prefix);
		dst = inet_ntop(AF_INET6, &prefix, buf, sizeof(buf));
		//AUG_PRT("dst=%s, buf=%s prefix-len=%d\n",dst,buf,ipv6_addr.prefix_len);
		if(dst){
			sprintf(buf, "%s/%d", buf,ipv6_addr.prefix_len);
			buf1 = strdup(buf);
			//AUG_PRT("buf=%s, buf1=%s prefix-len=%d\n",buf,buf1,ipv6_addr.prefix_len);
		}
		//get ipv6-linklocal addr
		numOfIpv6=getifip6(ifname, IPV6_ADDR_LINKLOCAL, &ipv6_addr, 1);
		if(numOfIpv6 > 0){
			inet_ntop(AF_INET6, &ipv6_addr.addr, SourceRouteIp, INET6_ADDRSTRLEN);
			//sprintf(netmask, "%d\0", ipv6_addr.prefix_len);
			//AUG_PRT("aaaLINKLOCAL=%s, prefix-len=%d\n",SourceRouteIp,ipv6_addr.prefix_len);
		}
		//AUG_PRT("aaaLINKLOCAL=%s, prefix-len=%d numOfIpv6=%d buf1=%s\n",SourceRouteIp,ipv6_addr.prefix_len,numOfIpv6,buf1);
		//ip -6 route add 2004::/64 via fe80::2e0:4cff:fe86:511e dev nas0_2 table 114
		va_cmd("/bin/ip", 10, 1, "-6", "route", "add", buf1, "via", SourceRouteIp, "dev", ifname, "table", str_tblid);
		if(buf1 != NULL){
			free(buf1);
		}
	}

	return 0;
}


int set_pppv6_source_route(MIB_CE_ATM_VC_T *pEntry, char *ifname, struct in6_addr * ip6addr)
{
	int32_t tbl_id;
	unsigned int ifIndex;
	char str_tblid[10]="";
	char SourceRouteIp[64]={0};
	int applicationtype=0;
//AUG_PRT("%s-%d ifname=%s pEntry->dgw=%d\n",__func__,__LINE__,ifname,pEntry->dgw);

	if(!ifname || !ip6addr){
		printf("Error! NULL input!\n");
		return -1;
	}
	inet_ntop(AF_INET6, (const void *)ip6addr, SourceRouteIp, 64);

AUG_PRT("%s-%d ifname=%s pEntry->dgw=%d SourceRouteIp=%s\n",__func__,__LINE__,ifname,pEntry->dgw,SourceRouteIp);

	//This interface is not default route
	if ( pEntry->dgw== 0)
	{
		char	buf[128]={0};
		ifIndex = getIfIndexByName(ifname);
		applicationtype=getapplicationtypeByName(ifname);
		tbl_id = caculate_tblid_ITF_SourceRoute(ifIndex);
AUG_PRT("ifname=%s ifIndex=%x applicationtype=%d tbl_id=%d\n",ifname,ifIndex,applicationtype,tbl_id);
		snprintf(str_tblid, sizeof(str_tblid), "%d", tbl_id);
		//del default route, and add policy route
		sprintf(buf,"echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr",ifname);
		AUG_PRT("buf=%s\n",buf);
		system(buf);
		inet_ntop(AF_INET6, (const void *)pEntry->RemoteIpv6Addr, SourceRouteIp, 64);
		if(!strcmp(SourceRouteIp,"::")){
			//remote gateway not get yet!
			AUG_PRT("remote gateway not get yet ::\n");

		}else{
			AUG_PRT("del ipv6 secondary default route %s\n",SourceRouteIp);
			//del ipv6 secondary default route
			//ip -6 route del default via fe80::a00:27ff:fe29:3e63 dev ppp1
			va_cmd("/bin/ip", 8, 1, "-6", "route", "del", "default", "via", SourceRouteIp,"dev", ifname);
		}

		//ip -6 route flush table 64
		va_cmd("/bin/ip", 5, 1, "-6", "route", "flush", "table", str_tblid);
		// ip -6 rule del table 64
		va_cmd("/bin/ip", 5, 1, "-6", "rule", "del", "table", str_tblid);

		inet_ntop(AF_INET6, (const void *)ip6addr, SourceRouteIp, 64);
AUG_PRT("%s-%d SourceRouteIp=%s\n",__func__,__LINE__,SourceRouteIp);

		//strcpy(SourceRouteIp,inet_ntoa(*((struct in_addr *)&ppp_info->myip)) );
		// Add source rule
		//ip -6 rule add from 2004::2e0:4cff:fe86:5120 table 64
		va_cmd("/bin/ip", 7, 1, "-6", "rule", "add", "from", SourceRouteIp,"table", str_tblid);
		// ip -6 route add default dev ppp0 table 200
		va_cmd("/bin/ip", 8, 1, "-6", "route", "add", "default", "dev", ifname, "table", str_tblid);

	}

	return 0;
}
#endif
int set_ppp_source_route(struct ppp_policy_route_info *ppp_info)
{
	int32_t tbl_id;
	unsigned int ifIndex;
	char str_tblid[10]="";
	char SourceRouteIp[20]={0};
	char sourceroute_tmp[50];
	int applicationtype;

	ifIndex = getIfIndexByName(ppp_info->if_name);
	applicationtype=getapplicationtypeByName(ppp_info->if_name);
	tbl_id = caculate_tblid_ITF_SourceRoute(ifIndex);
	snprintf(str_tblid, sizeof(str_tblid), "%d", tbl_id);

	//ip route flush table 64
	va_cmd("/bin/ip", 4, 1, "route", "flush", "table", str_tblid);
	// ip rule del table 64
	va_cmd("/bin/ip", 4, 1, "rule", "del", "table", str_tblid);

	strcpy(SourceRouteIp,inet_ntoa(*((struct in_addr *)&ppp_info->myip)) );
	//snprintf(sourceroute_tmp,50,"%s/%d",SourceRouteIp,24);
	// Add source rule
	va_cmd("/bin/ip", 6, 1, "rule", "add", "from", SourceRouteIp,"table", str_tblid);
	// ip route add default dev ppp0 table 200
	va_cmd("/bin/ip", 7, 1, "route", "add", "default", "dev", ppp_info->if_name, "table", str_tblid);

	return 0;
}

// DHCP client configuration
// return value:
// 1  : successful
int startDhcpc(char *inf, MIB_CE_ATM_VC_Tp pEntry)
{
	unsigned char value[32], value2[32];
	char devName[MAX_NAME_LEN];
	FILE *fp;
	DNS_TYPE_T dnsMode;
	char * argv[16];

	unsigned int i, vcTotal, resolvopt;

	mib_get(MIB_DEVICE_NAME, (void *)devName);

	argv[1] = inf;
	argv[2] = "up";
	argv[3] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s\n", IFCONFIG, argv[1], argv[2]);
	do_cmd(IFCONFIG, argv, 1);

	// udhcpc -i vc0 -p pid -s script
	argv[1] = (char *)ARG_I;
	argv[2] = inf;
	argv[3] = "-p";
	snprintf(value2, 32, "%s.%s", (char*)DHCPC_PID, inf);
	argv[4] = (char *)value2;
	argv[5] = "-s";
	snprintf(value, 32, "%s.%s", (char *)DHCPC_SCRIPT_NAME, inf);
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	if(!strcmp(ALIASNAME_BR0, inf))
		writeDhcpClientScript(value);
	else
		write_to_dhcpc_script(value, pEntry);
#else
	write_to_dhcpc_script(value, pEntry);
#endif
	argv[6] = (char *)DHCPC_SCRIPT;
	// Add option 12 Host Name
	argv[7] = "-H";
	argv[8] = (char *)devName;
	argv[9] = NULL;

	if (strcmp(inf, LANIF) == 0)
	{
		// LAN interface
		// enable Microsoft auto IP configuration
		argv[9] = "-a";
		argv[10] = NULL;
	}

	TRACE(STA_SCRIPT, "%s %s %s %s ", DHCPC, argv[1], argv[2], argv[3]);
	TRACE(STA_SCRIPT, "%s %s %s\n", argv[4], argv[5], argv[6]);
	do_cmd(DHCPC, argv, 0);

	return 1;
}

int reWriteAllDhcpcScript()
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char wanif[IFNAMSIZ];
	unsigned char value[32];

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
			printf("reWriteAllDhcpcScript: cannot get ATM_VC_TBL(ch=%d) entry\n", i);
			return 0;
		}

		if ((DHCP_T)Entry.ipDhcp == DHCP_CLIENT)
		{
			ifGetName(PHY_INTF(Entry.ifIndex),wanif,sizeof(wanif));
			snprintf(value, 32, "%s.%s", (char *)DHCPC_SCRIPT_NAME, wanif);
			write_to_dhcpc_script(value, &Entry);
		}

	}
}

int clean_SourceRoute(MIB_CE_ATM_VC_Tp pEntry)
{
	//char buff2[128];
	int32_t tbl_id;
	char str_tblid[10]="";

	if ( ((pEntry->cmode == CHANNEL_MODE_RT1483) || (pEntry->cmode == CHANNEL_MODE_IPOE) || (pEntry->cmode == CHANNEL_MODE_PPPOE) || (pEntry->cmode == CHANNEL_MODE_PPPOA))
	    && !isDefaultRouteWan(pEntry))
	{
		tbl_id = caculate_tblid_ITF_SourceRoute(pEntry->ifIndex);
		snprintf(str_tblid, sizeof(str_tblid), "%d", tbl_id);

		//ip route flush table 64
		va_cmd("/bin/ip", 4, 1, "route", "flush", "table", str_tblid);
		// ip rule del table 64 for fwmark
		va_cmd("/bin/ip", 4, 1, "rule", "del", "table", str_tblid);
	}
	return 0;
}

int cleanAll_SourceRoute()
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
			printf("cleanAll_SourceRoute: cannot get ATM_VC_TBL(ch=%d) entry\n", i);
			return 0;
		}

		clean_SourceRoute(&Entry);
	}
}

#ifdef ADDRESS_MAPPING
#ifdef MULTI_ADDRESS_MAPPING
static int GetIP_AddressMap(MIB_CE_MULTI_ADDR_MAPPING_LIMIT_T *entry, ADDRESSMAP_IP_T *ip_info)
{
	unsigned char value[32];

        // Get Local Start IP
	if (((struct in_addr *)entry->lsip)->s_addr != 0)
	{
		strncpy(ip_info->lsip, inet_ntoa(*((struct in_addr *)entry->lsip)), 16);
		ip_info->lsip[15] = '\0';
	}
	else
		ip_info->lsip[0] = '\0';


	// Get Local End IP
	if (((struct in_addr *)entry->leip)->s_addr != 0)
	{
		strncpy(ip_info->leip, inet_ntoa(*((struct in_addr *)entry->leip)), 16);
		ip_info->leip[15] = '\0';
	}
	else
		ip_info->leip[0] = '\0';

	// Get Global Start IP
	if (((struct in_addr *)entry->gsip)->s_addr != 0)
	{
		strncpy(ip_info->gsip, inet_ntoa(*((struct in_addr *)entry->gsip)), 16);
		ip_info->gsip[15] = '\0';
	}
	else
		ip_info->gsip[0] = '\0';

	// Get Global End IP
	if (((struct in_addr *)entry->geip)->s_addr != 0)
	{
		strncpy(ip_info->geip, inet_ntoa(*((struct in_addr *)entry->geip)), 16);
		ip_info->geip[15] = '\0';
	}
	else
		ip_info->geip[0] = '\0';

	sprintf(ip_info->srcRange, "%s-%s", ip_info->lsip, ip_info->leip);
	sprintf(ip_info->globalRange, "%s-%s", ip_info->gsip, ip_info->geip);
//	printf( "\r\nGetIP_AddressMap %s-%s", ip_info->lsip, ip_info->leip);
//	printf( "\r\nGetIP_AddressMap %s-%s", ip_info->gsip, ip_info->geip);
	return 1;
}
#else //!MULTI_ADDRESS_MAPPING
static int GetIP_AddressMap(ADDRESSMAP_IP_T *ip_info)
{
	unsigned char value[32];

        // Get Local Start IP
        if (mib_get(MIB_LOCAL_START_IP, (void *)value) != 0)
	{
		if (((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(ip_info->lsip, inet_ntoa(*((struct in_addr *)value)), 16);
			ip_info->lsip[15] = '\0';
		}
		else
			ip_info->lsip[0] = '\0';
	}

	// Get Local End IP
        if (mib_get(MIB_LOCAL_END_IP, (void *)value) != 0)
	{
		if (((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(ip_info->leip, inet_ntoa(*((struct in_addr *)value)), 16);
			ip_info->leip[15] = '\0';
		}
		else
			ip_info->leip[0] = '\0';
	}

	// Get Global Start IP
	if (mib_get(MIB_GLOBAL_START_IP, (void *)value) != 0)
	{
		if (((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(ip_info->gsip, inet_ntoa(*((struct in_addr *)value)), 16);
			ip_info->gsip[15] = '\0';
		}
		else
			ip_info->gsip[0] = '\0';
	}

	// Get Global End IP
	if (mib_get(MIB_GLOBAL_END_IP, (void *)value) != 0)
	{
		if (((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(ip_info->geip, inet_ntoa(*((struct in_addr *)value)), 16);
			ip_info->geip[15] = '\0';
		}
		else
			ip_info->geip[0] = '\0';
	}

	sprintf(ip_info->srcRange, "%s-%s", ip_info->lsip, ip_info->leip);
	sprintf(ip_info->globalRange, "%s-%s", ip_info->gsip, ip_info->geip);

	return 1;
}
#endif //end of !MULTI_ADDRESS_MAPPING
#endif

// Setup one NAT rule for a specfic interface
static int startAddressMap(MIB_CE_ATM_VC_Tp pEntry)
{
	char wanif[IFNAMSIZ];
	char myip[16];
#ifdef ADDRESS_MAPPING
	ADDRESSMAP_IP_T ip_info;
#ifdef MULTI_ADDRESS_MAPPING
	MIB_CE_MULTI_ADDR_MAPPING_LIMIT_T		entry;
	int		i,entryNum;


#else // ! MULTI_ADDRESS_MAPPING
	char vChar;
	ADSMAP_T mapType;

	GetIP_AddressMap(&ip_info);

	mib_get( MIB_ADDRESS_MAP_TYPE,  (void *)&vChar);
        mapType = (ADSMAP_T)vChar;
#endif  //end of !MULTI_ADDRESS_MAPPING
#endif

	if ( !pEntry->enable || ((DHCP_T)pEntry->ipDhcp != DHCP_DISABLED)
		|| ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
		|| ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
		|| ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOA)
		|| (!pEntry->napt)
		)
		return -1;

	//snprintf(wanif, 6, "vc%u", VC_INDEX(pEntry->ifIndex));
	ifGetName( PHY_INTF(pEntry->ifIndex), wanif, sizeof(wanif));
	strncpy(myip, inet_ntoa(*((struct in_addr *)pEntry->ipAddr)), 16);
	myip[15] = '\0';

#ifdef ADDRESS_MAPPING
#ifdef MULTI_ADDRESS_MAPPING
	entryNum = mib_chain_total(MULTI_ADDRESS_MAPPING_LIMIT_TBL);

	for (i = 0; i<entryNum; i++)
	{
		mib_chain_get(MULTI_ADDRESS_MAPPING_LIMIT_TBL, i, &entry);
		GetIP_AddressMap(&entry, &ip_info);

		// add customized mapping
		if ( entry.addressMapType == ADSMAP_ONE_TO_ONE ) {
			va_cmd(IPTABLES, 12, 1, "-t", "nat", FW_ADD, "POSTROUTING", "-s", ip_info.lsip,
				ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.gsip);

		}
		else if ( entry.addressMapType == ADSMAP_MANY_TO_ONE ) {
			va_cmd(IPTABLES, 14, 1, "-t", "nat", FW_ADD, "POSTROUTING", "-m", "iprange", "--src-range", ip_info.srcRange,
				ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.gsip);

		}
		else if ( entry.addressMapType == ADSMAP_MANY_TO_MANY ) {
			va_cmd(IPTABLES, 14, 1, "-t", "nat", FW_ADD, "POSTROUTING", "-m", "iprange", "--src-range", ip_info.srcRange,
				ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.globalRange);

		}
		else if ( entry.addressMapType == ADSMAP_ONE_TO_MANY ) {
			va_cmd(IPTABLES, 12, 1, "-t", "nat", FW_ADD, "POSTROUTING", "-s", ip_info.lsip,
				ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.globalRange);
		}

	}
#else //!MULTI_ADDRESS_MAPPING
	// add customized mapping
	if ( mapType == ADSMAP_ONE_TO_ONE ) {
		va_cmd(IPTABLES, 12, 1, "-t", "nat", FW_ADD, "POSTROUTING", "-s", ip_info.lsip,
			ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.gsip);

	}
	else if ( mapType == ADSMAP_MANY_TO_ONE ) {
		va_cmd(IPTABLES, 14, 1, "-t", "nat", FW_ADD, "POSTROUTING", "-m", "iprange", "--src-range", ip_info.srcRange,
			ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.gsip);

	}
	else if ( mapType == ADSMAP_MANY_TO_MANY ) {
		va_cmd(IPTABLES, 14, 1, "-t", "nat", FW_ADD, "POSTROUTING", "-m", "iprange", "--src-range", ip_info.srcRange,
			ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.globalRange);

	}
	// Mason Yu on True
#if 1
	else if ( mapType == ADSMAP_ONE_TO_MANY ) {
		va_cmd(IPTABLES, 12, 1, "-t", "nat", FW_ADD, "POSTROUTING", "-s", ip_info.lsip,
			ARG_O, wanif, "-j", "SNAT", "--to-source", ip_info.globalRange);

	}
#endif
#endif //end of !MULTI_ADDRESS_MAPPING
#endif
	// add default mapping
	va_cmd(IPTABLES, 10, 1, "-t", "nat", FW_ADD, "POSTROUTING",
		ARG_O, wanif, "-j", "SNAT", "--to-source", myip);
}

// Delete one NAT rule for a specfic interface
static void stopAddressMap(MIB_CE_ATM_VC_Tp pEntry)
{
	char *argv[16];
	char wanif[IFNAMSIZ];
	char myip[16];

#ifdef ADDRESS_MAPPING
	ADDRESSMAP_IP_T ip_info;
#ifdef MULTI_ADDRESS_MAPPING
	MIB_CE_MULTI_ADDR_MAPPING_LIMIT_T		entry;
	int		i,entryNum;


#else //MULTI_ADDRESS_MAPPING
	char vChar;
	ADSMAP_T mapType;

	GetIP_AddressMap(&ip_info);

	mib_get( MIB_ADDRESS_MAP_TYPE,  (void *)&vChar);
        mapType = (ADSMAP_T)vChar;
#endif //!MULTI_ADDRESS_MAPPING
#endif

	if ( !pEntry->enable || ((DHCP_T)pEntry->ipDhcp != DHCP_DISABLED)
		|| ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
		|| ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
		|| ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOA)
		|| (!pEntry->napt)
		)
		return;

	//snprintf(wanif, 6, "vc%u", VC_INDEX(pEntry->ifIndex));
	ifGetName( PHY_INTF(pEntry->ifIndex), wanif, sizeof(wanif));
	strncpy(myip, inet_ntoa(*((struct in_addr *)pEntry->ipAddr)), 16);
	myip[15] = '\0';

	if (pEntry->cmode != CHANNEL_MODE_BRIDGE && pEntry->napt == 1)
	{	// Enable NAPT
		argv[1] = "-t";
		argv[2] = "nat";
		argv[3] = (char *)FW_DEL;
		argv[4] = "POSTROUTING";

		// remove default mapping
		argv[5] = "-o";
		argv[6] = wanif;
		argv[7] = "-j";
		if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
			strncpy(myip, inet_ntoa(*((struct in_addr *)pEntry->ipAddr)), 16);
			myip[15] = '\0';
			argv[8] = "SNAT";
			argv[9] = "--to-source";
			argv[10] = myip;
			argv[11] = NULL;
		}
		else {
			argv[8] = "MASQUERADE";
			argv[9] = NULL;
		}
		do_cmd(IPTABLES, argv, 1);

#ifdef ADDRESS_MAPPING
#ifdef MULTI_ADDRESS_MAPPING
		entryNum = mib_chain_total(MULTI_ADDRESS_MAPPING_LIMIT_TBL);

		for (i = 0; i<entryNum; i++)
		{
			mib_chain_get(MULTI_ADDRESS_MAPPING_LIMIT_TBL, i, &entry);
			GetIP_AddressMap(&entry, &ip_info);

			// remove customized mapping
			if ( entry.addressMapType  == ADSMAP_ONE_TO_ONE ) {
				argv[5] = "-s";
				argv[6] = ip_info.lsip;
				argv[7] = "-o";
				argv[8] = wanif;
				argv[9] = "-j";
				if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
					argv[10] = "SNAT";
					argv[11] = "--to-source";
					argv[12] = ip_info.gsip;
					argv[13] = NULL;
				}
				else {
					argv[8] = "MASQUERADE";
					argv[9] = NULL;
				}

			} else if ( entry.addressMapType  == ADSMAP_MANY_TO_ONE ) {

				argv[5] = "-m";
				argv[6] = "iprange";
				argv[7] = "--src-range";
				argv[8] = ip_info.srcRange;
				argv[9] = "-o";
				argv[10] = wanif;
				argv[11] = "-j";

				if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
					argv[12] = "SNAT";
					argv[13] = "--to-source";
					argv[14] = ip_info.gsip;
					argv[15] = NULL;
				}
				else {
					argv[8] = "MASQUERADE";
					argv[9] = NULL;
				}

			} else if ( entry.addressMapType  == ADSMAP_MANY_TO_MANY ) {
				argv[5] = "-m";
				argv[6] = "iprange";
				argv[7] = "--src-range";
				argv[8] = ip_info.srcRange;
				argv[9] = "-o";
				argv[10] = wanif;
				argv[11] = "-j";
				if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
					argv[12] = "SNAT";
					argv[13] = "--to-source";
					argv[14] = ip_info.globalRange;
					argv[15] = NULL;
				}
				else {
					argv[8] = "MASQUERADE";
					argv[9] = NULL;
				}

			}

			else if ( entry.addressMapType  == ADSMAP_ONE_TO_MANY ) {
				argv[5] = "-s";
				argv[6] = ip_info.lsip;
				argv[7] = "-o";
				argv[8] = wanif;
				argv[9] = "-j";
				if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
					argv[10] = "SNAT";
					argv[11] = "--to-source";
					argv[12] = ip_info.globalRange;
					argv[13] = NULL;
				}
				else {
					argv[8] = "MASQUERADE";
					argv[9] = NULL;
				}
			}

			else
				return;
			TRACE(STA_SCRIPT, "%s %s %s %s %s ", IPTABLES, argv[1], argv[2], argv[3], argv[4]);
			TRACE(STA_SCRIPT, "%s %s %s %s\n", argv[5], argv[6], argv[7], argv[8]);
			do_cmd(IPTABLES, argv, 1);
		}
#else //!MULTI_ADDRESS_MAPPING
		// remove customized mapping
		if ( mapType == ADSMAP_ONE_TO_ONE ) {
			argv[5] = "-s";
			argv[6] = ip_info.lsip;
			argv[7] = "-o";
			argv[8] = wanif;
			argv[9] = "-j";
			if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
				argv[10] = "SNAT";
				argv[11] = "--to-source";
				argv[12] = ip_info.gsip;
				argv[13] = NULL;
			}
			else {
				argv[8] = "MASQUERADE";
				argv[9] = NULL;
			}

		} else if ( mapType == ADSMAP_MANY_TO_ONE ) {
			argv[5] = "-m";
			argv[6] = "iprange";
			argv[7] = "--src-range";
			argv[8] = ip_info.srcRange;
			argv[9] = "-o";
			argv[10] = wanif;
			argv[11] = "-j";
			if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
				argv[12] = "SNAT";
				argv[13] = "--to-source";
				argv[14] = ip_info.gsip;
				argv[15] = NULL;
			}
			else {
				argv[8] = "MASQUERADE";
				argv[9] = NULL;
			}

		} else if ( mapType == ADSMAP_MANY_TO_MANY ) {
			argv[5] = "-m";
			argv[6] = "iprange";
			argv[7] = "--src-range";
			argv[8] = ip_info.srcRange;
			argv[9] = "-o";
			argv[10] = wanif;
			argv[11] = "-j";
			if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
				argv[12] = "SNAT";
				argv[13] = "--to-source";
				argv[14] = ip_info.globalRange;
				argv[15] = NULL;
			}
			else {
				argv[8] = "MASQUERADE";
				argv[9] = NULL;
			}

		}

		// Msason Yu on True
#if 1
		else if ( mapType == ADSMAP_ONE_TO_MANY ) {
			argv[5] = "-s";
			argv[6] = ip_info.lsip;
			argv[7] = "-o";
			argv[8] = wanif;
			argv[9] = "-j";
			if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
				argv[10] = "SNAT";
				argv[11] = "--to-source";
				argv[12] = ip_info.globalRange;
				argv[13] = NULL;
			}
			else {
				argv[8] = "MASQUERADE";
				argv[9] = NULL;
			}
		}
#endif
		else
			return;
		TRACE(STA_SCRIPT, "%s %s %s %s %s ", IPTABLES, argv[1], argv[2], argv[3], argv[4]);
		TRACE(STA_SCRIPT, "%s %s %s %s\n", argv[5], argv[6], argv[7], argv[8]);
		do_cmd(IPTABLES, argv, 1);

#endif //end of !MULTI_ADDRESS_MAPPING
#endif // of ADDRESS_MAPPING
	}

}

// Config all NAT rules.
// If action= ACT_STOP, delete all NAT rules.
// If action= ACT_START, setup all NAT rules.
void config_AddressMap(int action)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char wanif[6];
	char myip[16];

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
			printf("restartAddressMap: cannot get ATM_VC_TBL(ch=%d) entry\n", i);
			return;
		}

		if (Entry.enable == 0)
			continue;
		if ( action == ACT_STOP )
			stopAddressMap(&Entry);
		else if ( action == ACT_START) {
			startAddressMap(&Entry);
		}
	}

	if ( action == ACT_START) {
		va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
	}

}

int startIP_v4(char *inf, MIB_CE_ATM_VC_Tp pEntry, CHANNEL_MODE_T ipEncap)
{
	char myip[16], remoteip[16], netmask[16];
#ifdef IP_PASSTHROUGH
	unsigned int ippt_itf;
	int ippt=0;
	struct in_addr net;
	char netip[16];
#endif
	FILE *fp;
	// Mason Yu
	unsigned long broadcastIpAddr;
	char broadcast[16];

#ifdef IP_PASSTHROUGH
	// check IP passthrough
	if (mib_get(MIB_IPPT_ITF, (void *)&ippt_itf) != 0)
	{
		if (ippt_itf == pEntry->ifIndex)
			ippt = 1; // this interface enable the IP passthrough
	}
#endif

	if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED)
	{
		if (!get_net_link_status(inf))
			return 0;
		// ifconfig vc0 ipaddr
		strncpy(myip, inet_ntoa(*((struct in_addr *)pEntry->ipAddr)), 16);
		myip[15] = '\0';
		strncpy(remoteip, inet_ntoa(*((struct in_addr *)pEntry->remoteIpAddr)), 16);
		remoteip[15] = '\0';
		strncpy(netmask, inet_ntoa(*((struct in_addr *)pEntry->netMask)), 16);
		netmask[15] = '\0';
		// Mason Yu
		broadcastIpAddr = ((struct in_addr *)pEntry->ipAddr)->s_addr | ~(((struct in_addr *)pEntry->netMask)->s_addr);
		strncpy(broadcast, inet_ntoa(*((struct in_addr *)&broadcastIpAddr)), 16);
		broadcast[15] = '\0';
#ifdef CONFIG_ATM_CLIP
		if (ipEncap == CHANNEL_MODE_RT1483 || ipEncap == CHANNEL_MODE_RT1577)
#else
		if (ipEncap == CHANNEL_MODE_RT1483)
#endif
		{
#ifdef IP_PASSTHROUGH
			if (!ippt)
			{
#endif
				if (pEntry->ipunnumbered)	// Jenny, for IP unnumbered determination temporarily
					va_cmd(IFCONFIG, 8, 1, inf, "10.0.0.1", "-arp",
						"-broadcast", "pointopoint", "10.0.0.2",
						"netmask", ARG_255x4);
				else
					// ifconfig vc0 myip -arp -broadcast pointopoint
					//   remoteip netmask 255.255.255.255
					va_cmd(IFCONFIG, 8, 1, inf, myip, "-arp",
						"-broadcast", "pointopoint", remoteip,
//						"netmask", netmask);	// Jenny, subnet mask added
						"netmask", ARG_255x4);
#ifdef IP_PASSTHROUGH
			}
			else	// IP passthrough
			{
				// ifconfig vc0 10.0.0.1 -arp -broadcast pointopoint
				//   10.0.0.2 netmask 255.255.255.255
				va_cmd(IFCONFIG, 8, 1, inf, "10.0.0.1", "-arp",
					"-broadcast", "pointopoint", "10.0.0.2",				// Jenny, for IP passthrough determination temporarily
					"netmask", netmask);
//				va_cmd(IFCONFIG, 8, 1, inf, "10.0.0.1", "-arp",
//					"-broadcast", "pointopoint", "10.0.0.2",
//					"netmask", ARG_255x4);
				// ifconfig br0:1 remoteip
				va_cmd(IFCONFIG, 2, 1, LAN_IPPT, remoteip);

				// Mason Yu. Add route for Public IP
               			net.s_addr = (((struct in_addr *)pEntry->remoteIpAddr)->s_addr) & (((struct in_addr *)pEntry->netMask)->s_addr);
               			strncpy(netip, inet_ntoa(net), 16);
				netip[15] = '\0';
				va_cmd(ROUTE, 7, 1, ARG_DEL, "-net", netip, "netmask", netmask, "dev", LANIF);
				va_cmd(ROUTE, 5, 1, ARG_ADD, "-host", myip, "dev", LANIF);

				// write ip to file for DHCP server
				if (fp = fopen(IPOA_IPINFO, "w"))
				{
					fwrite( pEntry->ipAddr, 4, 1, fp);
					fwrite( pEntry->remoteIpAddr, 4, 1, fp);
					fwrite( pEntry->netMask, 4, 1, fp);
					fclose(fp);
				}
			}
#endif
			if (isDefaultRouteWan(pEntry))
			{
#ifdef DEFAULT_GATEWAY_V2
				if (ifExistedDGW() == 1)	// Jenny, delete existed default gateway first
					va_cmd(ROUTE, 2, 1, ARG_DEL, "default");
#endif
				// route add default vc0
				va_cmd(ROUTE, 3, 1, ARG_ADD, "default", inf);
				//va_cmd(ROUTE, 4, 1, ARG_ADD, "default", "gw", remoteip);
			}
		}
		else
		{
			// Mason Yu. Set netmask and broadcast
			// ifconfig vc0 myip
			//va_cmd(IFCONFIG, 2, 1, inf, myip);

			va_cmd(IFCONFIG, 6, 1, inf, myip, "netmask", netmask, "broadcast",  broadcast);

			if (isDefaultRouteWan(pEntry))
			{
#ifdef DEFAULT_GATEWAY_V2
					if (ifExistedDGW() == 1)	// Jenny, delete existed default gateway first
						va_cmd(ROUTE, 2, 1, ARG_DEL, "default");
#endif
				// route add default gw remotip
				va_cmd(ROUTE, 4, 1, ARG_ADD, "default", "gw", remoteip);
			}
			if (ipEncap == CHANNEL_MODE_IPOE) {
				unsigned char value[32];
				snprintf(value, 32, "%s.%s", (char *)MER_GWINFO, inf);
				if (fp = fopen(value, "w"))
				{
					fprintf(fp, "%s\n", remoteip);
					fclose(fp);
				}
			}
		}
	}
	else
	{
		int dhcpc_pid;
		unsigned char value[32];
		// Enabling support for a dynamically assigned IP (ISP DHCP)...
		va_cmd(IPTABLES, 16, 1, FW_ADD, FW_INPUT, ARG_I, inf, "-p",
			ARG_UDP, FW_DPORT, "69", "-d", ARG_255x4, "-m",
			"state", "--state", "NEW", "-j", FW_ACCEPT);

		if (fp = fopen(PROC_DYNADDR, "w"))
		{
			fprintf(fp, "1\n");
			fclose(fp);
		}
		else
		{
			printf("Open file %s failed !\n", PROC_DYNADDR);
		}

		snprintf(value, 32, "%s.%s", (char*)DHCPC_PID, inf);
		dhcpc_pid = read_pid((char*)value);
		if (dhcpc_pid > 0)
			kill(dhcpc_pid, SIGTERM);
		if (startDhcpc(inf, pEntry) == -1)
		{
			printf("start DHCP client failed !\n");
		}
	}

#ifdef CONFIG_HWNAT
	if (pEntry->napt == 1){
		if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED){
			send_extip_to_hwnat(ADD_EXTIP_CMD, inf, *(uint32 *)pEntry->ipAddr);
		}
		else{
			send_extip_to_hwnat(ADD_EXTIP_CMD, inf, 0);
		}
	}
#endif
#ifdef ROUTING
	// When interface IP reset, the static route will also be reseted.
	deleteStaticRoute();
	addStaticRoute();
#endif
	set_static_source_route(pEntry);

	return 1;
}
// IP interface: 1483-r or MER
// return value:
// 1  : successful
int startIP(char *inf, MIB_CE_ATM_VC_Tp pEntry, CHANNEL_MODE_T ipEncap)
{
	unsigned char buffer[7];

	// Move MTU setup to setup_ethernet_config()
	#if 0
	// Set MTU for 1483-r or MER
	sprintf(buffer, "%u", pEntry->mtu);
	va_cmd(IFCONFIG,3,1,inf, "mtu", buffer);
	#endif

#ifdef CONFIG_IPV6
	if (pEntry->IpProtocol & IPVER_IPV4) {
#endif
		startIP_v4(inf, pEntry, ipEncap);
#ifdef CONFIG_IPV6
	}
	if (pEntry->IpProtocol & IPVER_IPV6)
		startIP_for_V6(pEntry);
#endif
	return 1;
}

#ifdef CONFIG_PPP
// Jenny, stop PPP
void stopPPP(void)
{
	int i;
	char tp[10];
	struct data_to_pass_st msg;
#ifdef CONFIG_USER_PPPOMODEM
	for (i=0; i<(MAX_PPP_NUM+MAX_MODEM_PPPNUM); i++)
#else
	for (i=0; i<MAX_PPP_NUM; i++)
#endif //CONFIG_USER_PPPOMODEM
	{
		sprintf(tp, "ppp%d", i);
		if (find_ppp_from_conf(tp)) {
			snprintf(msg.data, BUF_SIZE, "spppctl del %u", i);
			TRACE(STA_SCRIPT, "%s\n", msg.data);
			write_to_pppd(&msg);
		}
	}
#ifdef CONFIG_USER_PPTP_CLIENT_PPTP
	for (i=9; i<=10; i++)
	{
		sprintf(tp, "ppp%d", i);
		if (find_ppp_from_conf(tp)) {
			snprintf(msg.data, BUF_SIZE, "spppctl del %d pptp 0", i);
			TRACE(STA_SCRIPT, "%s\n", msg.data);
			write_to_pppd(&msg);
		}
	}
#endif
#ifdef CONFIG_USER_L2TPD_L2TPD
	for (i=11; i<=12; i++)
	{
		sprintf(tp, "ppp%d", i);
		if (find_ppp_from_conf(tp)) {
			snprintf(msg.data, BUF_SIZE, "spppctl del %u l2tp 0", i);
			TRACE(STA_SCRIPT, "%s\n", msg.data);
			write_to_pppd(&msg);
		}
	}
#endif

	usleep(2000000);

}

// PPP connection
// Input: inf == "vc0","vc1", ...
// pppEncap: 0 : PPPoE, 1 : PPPoA
// return value:
// 1  : successful
int startPPP(char *inf, MIB_CE_ATM_VC_Tp pEntry, char *qos, CHANNEL_MODE_T pppEncap)
{
	char ifIdx[3], pppif[6], stimeout[7];
#ifdef IP_PASSTHROUGH
	unsigned int ippt_itf;
	int ippt=0;
#endif
	struct data_to_pass_st msg;
	int lastArg, pppinf, pppdbg = 0;
#ifdef NEW_PORTMAPPING
	int tbl_id;
	char str_tblid[16];
	char devname[IFNAMSIZ];
#endif

#ifdef IP_PASSTHROUGH
	// check IP passthrough
	if (mib_get(MIB_IPPT_ITF, (void *)&ippt_itf) != 0)
	{
		if (ippt_itf == pEntry->ifIndex)
			ippt = 1; // this interface enable the IP passthrough
	}
#endif

	snprintf(ifIdx, 3, "%u", PPP_INDEX(pEntry->ifIndex));
	snprintf(pppif, 6, "ppp%u", PPP_INDEX(pEntry->ifIndex));

#ifdef CONFIG_IPV6
	if(pEntry->IpProtocol & IPVER_IPV6){
		char cmdBuf[100]={0};
		char ppp_dev[IFNAMSIZ];

		if (pppEncap == CHANNEL_MODE_PPPOE) {
			// disable IPv6 for ppp device
			ifGetName(PHY_INTF(pEntry->ifIndex), ppp_dev, sizeof(ppp_dev));
			setup_disable_ipv6(ppp_dev, 1);
		}

		setup_disable_ipv6(pppif, 0);
		/*	Enable autoconf when selected autoconf and accept default route in RA only when the WAN belong to default GW , IulianWu */
		if (pEntry->AddrMode & IPV6_WAN_AUTO) { /* If select SLAAC */
			sprintf(cmdBuf, "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/autoconf", pppif);
			system(cmdBuf);
			if (isDefaultRouteWan(pEntry))
			{ /* Accept the deafult router in RA only on default GW */
				sprintf(cmdBuf, "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", pppif);
			}
			else {
				sprintf(cmdBuf, "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", pppif);
			}
			system(cmdBuf);
		}
		else {
			sprintf(cmdBuf, "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/autoconf", pppif);
			system(cmdBuf);			
		}
		
		if (pEntry->Ipv6Dhcp) {/* If select DHCPv6  */
			if (isDefaultRouteWan(pEntry))
			{ /* Accept the deafult router in RA only on default GW */
				sprintf(cmdBuf, "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", pppif);
			}
			else {
				sprintf(cmdBuf, "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", pppif);
			}
			system(cmdBuf);
		}

		if (pEntry->AddrMode & IPV6_WAN_STATIC) { /* If select Static IPV6	*/
			sprintf(cmdBuf, "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", pppif);
			system(cmdBuf);
		}	
	}
	else 
	{
		setup_disable_ipv6(pppif, 1);
	}
#endif

	if (pppEncap == CHANNEL_MODE_PPPOE)	// PPPoE
	{
		// Kaohj -- set pppoe txqueuelen to 0 to make it no queue as the queueing is
		//		just in the underlying vc interface
		// ifconfig ppp0 txqueuelen 0
		// tc qdisc replace dev ppp0 root pfifo
		// tc qdisc del dev ppp0 root
		va_cmd(IFCONFIG, 3, 1, pppif, "txqueuelen", "0");
		// workaround to remove default qdisc if any.
		va_cmd(TC, 6, 1, "qdisc", "replace", "dev", pppif, "root", "pfifo");
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", pppif, "root");
//#ifdef CONFIG_USER_PPPOE_PROXY
#if 0
              printf("enable pppoe proxy %d \n",pEntry->PPPoEProxyEnable);
              printf("maxuser = %d ...",pEntry->PPPoEProxyMaxUser);
             if(pEntry->PPPoEProxyEnable)
	    {
                FILE  *fp_pap;
                pppoe_proxy pp_proxy;
	         if ((fp_pap = fopen(PPPD_PAPFILE, "a+")) == NULL)
		  {
			printf("Open file %s failed !\n", PPPD_PAPFILE);
			return -1;
		  }
		  fprintf(fp_pap, "%s * \"%s\"  *\n", pEntry->pppUsername, pEntry->pppPassword);
		  fclose(fp_pap);

		  if(!has_pppoe_init){
				pp_proxy.cmd =PPPOE_PROXY_ENABLE;
				ppp_proxy_ioctl(&pp_proxy,SIOCPPPOEPROXY);
				has_pppoe_init =1;
		   }
		    pp_proxy.wan_unit =PPP_INDEX(pEntry->ifIndex) ;
		    pp_proxy.cmd =PPPOE_WAN_UNIT_SET;
		    strcpy(pp_proxy.user,pEntry->pppUsername);
		    strcpy(pp_proxy.passwd,pEntry->pppPassword);
		    pp_proxy.maxShareNum = pEntry->PPPoEProxyMaxUser;
		    ppp_proxy_ioctl(&pp_proxy,SIOCPPPOEPROXY);

         	    if (pEntry->napt == 1)
        	   {	// Enable NAPT
		      va_cmd(IPTABLES, 8, 1, "-t", "nat", FW_ADD, "POSTROUTING",
			 "-o", pppif, "-j", "MASQUERADE");
        	    }


		   	return 1;
             }
#endif

		if (pEntry->pppCtype != MANUAL){	// Jenny
			// spppctl add 0 pppoe vc0 username USER password PASSWORD
			//         gw 1 mru xxxx acname xxx
			snprintf(msg.data, BUF_SIZE,
				"spppctl add %s pppoe %s username %s password %s"
				" gw %d mru %d", ifIdx,
				inf, pEntry->pppUsername, pEntry->pppPassword,
				isDefaultRouteWan(pEntry), pEntry->mtu);
		}
		else {
			snprintf(msg.data, BUF_SIZE,
				"spppctl new %s pppoe %s username %s password %s"
				" gw %d mru %d", ifIdx,
				inf, pEntry->pppUsername, pEntry->pppPassword,
				isDefaultRouteWan(pEntry), pEntry->mtu);
		}
		if (strlen(pEntry->pppACName))
			snprintf(msg.data, BUF_SIZE, "%s acname %s", msg.data, pEntry->pppACName);
		// Set Service Name
		if (strlen(pEntry->pppServiceName))
			snprintf(msg.data, BUF_SIZE, "%s servicename %s", msg.data, pEntry->pppServiceName);
#ifdef CONFIG_SPPPD_STATICIP
		// Jenny, set PPPoE static IP
		if (pEntry->pppIp) {
			unsigned long addr;
			addr = *((unsigned long *)pEntry->ipAddr);
			if (addr)
				snprintf(msg.data, BUF_SIZE, "%s staticip %x", msg.data, addr);
		}
#endif
#ifdef CONFIG_USER_PPPOE_PROXY
		snprintf(msg.data, BUF_SIZE, "%s proxy %d maxuser %d itfgroup %d", msg.data, pEntry->PPPoEProxyEnable, pEntry->PPPoEProxyMaxUser, pEntry->itfGroup);
#endif
	}
	#ifdef CONFIG_DEV_xDSL
	else	// PPPoA
	{
		if (pEntry->pppCtype != MANUAL)	// Jenny
			// spppctl add 0 pppoa vpi.vci encaps ENCAP qos xxx
			//         username USER password PASSWORD gw 1 mru xxxx
			snprintf(msg.data, BUF_SIZE,
				"spppctl add %s pppoa %u.%u encaps %d qos %s "
				"username %s password %s gw %d mru %d",
				ifIdx, pEntry->vpi, pEntry->vci, pEntry->encap,	qos,
				pEntry->pppUsername, pEntry->pppPassword, isDefaultRouteWan(pEntry), pEntry->mtu);
		else
			snprintf(msg.data, BUF_SIZE,
				"spppctl new %s pppoa %u.%u encaps %d qos %s "
				"username %s password %s gw %d mru %d",
				ifIdx, pEntry->vpi, pEntry->vci, pEntry->encap,	qos,
				pEntry->pppUsername, pEntry->pppPassword, isDefaultRouteWan(pEntry), pEntry->mtu);
	}
	#endif // CONFIG_DEV_xDSL

	// Set Authentication Method
	if ((PPP_AUTH_T)pEntry->pppAuth >= PPP_AUTH_PAP && (PPP_AUTH_T)pEntry->pppAuth <= PPP_AUTH_CHAP)
		snprintf(msg.data, BUF_SIZE, "%s auth %s", msg.data, ppp_auth[pEntry->pppAuth]);

#ifdef IP_PASSTHROUGH
	// Set IP passthrough
	snprintf(msg.data, BUF_SIZE, "%s ippt %d", msg.data, ippt);
#endif

	// set PPP debug
	pppdbg = pppdbg_get(PPP_INDEX(pEntry->ifIndex));
	snprintf(msg.data, BUF_SIZE, "%s debug %d", msg.data, pppdbg);

#ifdef _CWMP_MIB_
	// Set Auto Disconnect Timer
	if (pEntry->autoDisTime > 0)
		snprintf(msg.data, BUF_SIZE, "%s disctimer %d", msg.data, pEntry->autoDisTime);
	// Set Warn Disconnect Delay
	if (pEntry->warnDisDelay > 0)
		snprintf(msg.data, BUF_SIZE, "%s discdelay %d", msg.data, pEntry->warnDisDelay);
#endif

#ifdef CONFIG_IPV6
	snprintf(msg.data, BUF_SIZE, "%s ipt %u", msg.data, pEntry->IpProtocol - 1);
#endif

	if (pEntry->pppCtype == CONTINUOUS)	// Continuous
	{
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
		// set the ppp keepalive timeout
		if(pEntry->pppLcpEcho)
			snprintf(msg.data, BUF_SIZE, "spppctl katimer %hu", pEntry->pppLcpEcho);
		else
#if defined(CONFIG_00R0)
			snprintf(msg.data, BUF_SIZE, "spppctl katimer 30");	//default
#else
			snprintf(msg.data, BUF_SIZE, "spppctl katimer 100");//default
#endif
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);

		// set the ppp keepalive timeout
		if(pEntry->pppLcpEchoRetry)
			snprintf(msg.data, BUF_SIZE, "spppctl echo_retry %hu", pEntry->pppLcpEchoRetry);
		else
#if defined(CONFIG_00R0)
			snprintf(msg.data, BUF_SIZE, "spppctl echo_retry 5");	//default
#else
			snprintf(msg.data, BUF_SIZE, "spppctl echo_retry 6");	//default
#endif
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
		printf("PPP Connection (Continuous)...\n");
	}
	else if (pEntry->pppCtype == CONNECT_ON_DEMAND)	// On-demand
	{
		snprintf(msg.data, BUF_SIZE, "%s timeout %u", msg.data, pEntry->pppIdleTime);
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
		printf("PPP Connection (On-demand)...\n");
	}
	else if (pEntry->pppCtype == MANUAL)	// Manual
	{
		// Jenny, for PPP connecting/disconnecting manually
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
		printf("PPP Connection (Manual)...\n");
	}

#ifdef NEW_PORTMAPPING
	// Add initial default route to dev (ex. nas0_0) when ppp not ready.
	tbl_id = caculate_tblid(pEntry->ifIndex);
	snprintf(str_tblid, 16, "%d", tbl_id);
	ifGetName(PHY_INTF(pEntry->ifIndex), devname, sizeof(devname));
	printf("%s: add default dev %s table %d\n", __FUNCTION__, devname, tbl_id);
	va_cmd("/bin/ip", 7, 1, "route", "add", "default", "dev", devname, "table", str_tblid);
#endif
	if (pEntry->napt == 1)
	{
#ifdef CONFIG_HWNAT
		/*QL: pppoe interface should do HW SNAT as well as mer interface*/
		send_extip_to_hwnat(ADD_EXTIP_CMD, pppif, 0);
#endif
	}

	return 1;
}

// find if pppif exists in /var/ppp/ppp.conf
int find_ppp_from_conf(char *pppif)
{
	char buff[256];
	FILE *fp;
	char strif[6];
	int found=0;

	if (!(fp=fopen(PPP_CONF, "r"))) {
		printf("%s not exists.\n", PPP_CONF);
	}
	else {
		fgets(buff, sizeof(buff), fp);
		while( fgets(buff, sizeof(buff), fp) != NULL ) {
			if(sscanf(buff, "%s", strif)!=1) {
				found=0;
				printf("Unsuported ppp configuration format\n");
				break;
			}

			if ( !strcmp(pppif, strif) ) {
				found = 1;
				break;
			}
		}
		fclose(fp);
	}
	return found;
}
#endif

#ifdef CONFIG_DEV_xDSL
// find if vc exists in /proc/net/atm/pvc
static int find_pvc_from_running(int vpi, int vci)
{
	char buff[256];
	FILE *fp;
	int tvpi, tvci;
	int found=0;

	if (!(fp=fopen(PROC_NET_ATM_PVC, "r"))) {
		fclose(fp);
		printf("%s not exists.\n", PROC_NET_ATM_PVC);
	}
	else {
		fgets(buff, sizeof(buff), fp);
		while( fgets(buff, sizeof(buff), fp) != NULL ) {
			if(sscanf(buff, "%*d%d%d", &tvpi, &tvci)!=2) {
				found=0;
				printf("Unsuported pvc configuration format\n");
				break;
			}
			if ( tvpi==vpi && tvci==vci ) {
				found = 1;
				break;
			}
		}
		fclose(fp);
	}
	return found;
}
#endif//CONFIG_DEV_xDSL

// calculate the 15-0(bits) Cell Rate register value (PCR or SCR)
// return its corresponding register value
static int cr2reg(int pcr)
{
#ifdef CONFIG_RTL8672
	return pcr;
#else
	int k, e, m, pow2, reg;

	k = pcr;
	e=0;

	while (k>1) {
		k = k/2;
		e++;
	}

	//printf("pcr=%d, e=%d\n", pcr,e);
	pow2 = 1;
	for (k = 1; k <= e; k++)
		pow2*=2;

	//printf("pow2=%d\n", pow2);
	//m = ((pcr/pow2)-1)*512;
	k = 0;
	while (pcr >= pow2) {
		pcr -= pow2;
		k++;
	}
	m = (k-1)*512 + pcr*512/pow2;
	//printf("m=%d\n", m);
	reg = (e<<9 | m );
	//printf("reg=%d\n", reg);
	return reg;
#endif
}

/*
 *	get the mark value for a traffic classification
 */

int _get_classification_mark( int entryNo, MIB_CE_IP_QOS_T *p )
{
	int mark=0;

	if(p==NULL) return 0;

	// mark the packet:  8-bits(high) |   3-bits(low)
	//                    class id    |  802.1p (if any)
	mark = ((entryNo+1) << 8);	// class id
	#if 0
	#ifdef QOS_SPEED_LIMIT_SUPPORT
	//use the first 3 bit
	if(p->limitSpeedEnabled)
		{
			int tmpmark=p->limitSpeedRank;

		  	mark|=tmpmark<<12;
			printf("limitSpeedRank=%d,mark=%d\n",p->limitSpeedRank,mark);
		}

	#endif
	#endif
	if (p->m_1p != 0)
		mark |= (p->m_1p-1);	// 802.1p

	return mark;
}

int get_classification_mark(int entryNo)
{
	MIB_CE_IP_QOS_T qEntry;
	int i, num, mark;

	mark = 0;
	num = mib_chain_total(MIB_IP_QOS_TBL);
	if (entryNo >= num)
		return 0;
	// get fwmark
	if (!mib_chain_get(MIB_IP_QOS_TBL, entryNo, (void *)&qEntry))
		return 0;

#if 1
	mark =  _get_classification_mark( entryNo, &qEntry );
#else
	// mark the packet:  8-bits(high) |   8-bits(low)
	//                    class id    |  802.1p (if any)
	mark = ((entryNo+1) << 8);	// class id
	if (qEntry.m_1p != 0)
		mark |= (qEntry.m_1p-1);	// 802.1p
#endif

	return mark;
}

/*
 *	generate the ifup_ppp(vc)x script for WAN interface
 */
static int generate_ifup_script(unsigned int ifIndex, MIB_CE_ATM_VC_Tp pEntry)
{
	int mark, ret;
	FILE *fp;
	char wanif[8], ifup_path[32];
#ifdef IP_POLICY_ROUTING
	int i, num, found;
	MIB_CE_IP_QOS_T qEntry;
#endif
	char ipv6Enable =-1;


	ret = 0;

	ifGetName(ifIndex, wanif, sizeof(wanif));

	if (PPP_INDEX(ifIndex) != DUMMY_PPP_INDEX) {
		// PPP interface

		snprintf(ifup_path, 32, "/var/ppp/ifup_%s", wanif);
		if (fp=fopen(ifup_path, "w+") ) {
			fprintf(fp, "#!/bin/sh\n\n");
#ifdef IP_POLICY_ROUTING
			num = mib_chain_total(MIB_IP_QOS_TBL);
			found = 0;
			// set advanced-routing rule
			for (i=0; i<num; i++) {
				if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void *)&qEntry))
					continue;
#ifdef QOS_DIFFSERV
				if (qEntry.enDiffserv == 1) // Diffserv entry
					continue;
#endif
				if (qEntry.outif == ifIndex) {

					found = 1;
					mark = get_classification_mark(i);
					if (mark != 0) {
						// Don't forget to point out that fwmark with
						// ipchains/iptables is a decimal number, but that
						// iproute2 uses hexadecimal number.
						fprintf(fp, "ip ru add fwmark %x table %d\n", mark, PPP_INDEX(ifIndex)+PR_PPP_START);
					}
				}
			}
			if (found) {
				fprintf(fp, "ip ro add default dev %s table %d\n", wanif, PPP_INDEX(ifIndex)+PR_PPP_START);
			}
#endif
#if defined(CONFIG_USER_UPNPD)||defined(CONFIG_USER_MINIUPNPD)
			// Added by Mason Yu for Start upnpd.
			fprintf(fp, "/bin/upnpctrl sync\n");
#endif
#ifdef NEW_PORTMAPPING
			//august: 2012 March 3rd for fixing the pppoe re-up portmapping bugs
			fprintf(fp, "ip ro flush table %d\n", caculate_tblid(ifIndex));
			fprintf(fp, "ip ro add default dev %s table %d\n", wanif, caculate_tblid(ifIndex));
#endif

			fclose(fp);
			chmod(ifup_path, 484);

			// Added by Mason Yu. For IPV6
#ifdef CONFIG_IPV6
			mib_get(MIB_V6_IPV6_ENABLE, (void *)&ipv6Enable);
			if(ipv6Enable == 1)
			{
				snprintf(ifup_path, 32, "/var/ppp/ifupv6_%s", wanif);
				if (fp=fopen(ifup_path, "w+") ) {
					unsigned char Ipv6AddrStr[48], RemoteIpv6AddrStr[48];
					unsigned char pidfile[30], leasefile[30];
					unsigned char value[128];
					char dhcpc_cf_buf[32]={0};

					fprintf(fp, "#!/bin/sh\n\n");
#ifdef CONFIG_00R0
					if ((pEntry->AddrMode & 0x1) != 0x1) { //Bitmap, bit0: Slaac, bit1: Static, bit2: DS-Lite , bit3: 6rd
						// SLAAC
						fprintf(fp, "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/autoconf\n", wanif);
					}
					else
						fprintf(fp, "/bin/echo 1 > /proc/sys/net/ipv6/conf/%s/autoconf\n", wanif);

					// Set forwarding=0 to accept RA
					fprintf(fp, "/bin/echo 0 > /proc/sys/net/ipv6/conf/%s/forwarding\n", wanif);
#endif
					if ( ((pEntry->AddrMode & 0x2)) == 0x2 ) {
						inet_ntop(PF_INET6, (struct in6_addr *)pEntry->Ipv6Addr, Ipv6AddrStr, sizeof(Ipv6AddrStr));
						inet_ntop(PF_INET6, (struct in6_addr *)pEntry->RemoteIpv6Addr, RemoteIpv6AddrStr, sizeof(RemoteIpv6AddrStr));

						// Add WAN static IP
						snprintf(Ipv6AddrStr, 48, "%s/%d", Ipv6AddrStr, pEntry->Ipv6AddrPrefixLen);
						fprintf(fp, "/bin/ifconfig %s add %s\n", wanif, Ipv6AddrStr);

						// Add default gw
						if (isDefaultRouteWan(pEntry)) {
							// route -A inet6 add ::/0 gw 3ffe::0200:00ff:fe00:0100 dev ppp0
							fprintf(fp, "/bin/route -A inet6  add ::/0 gw %s dev %s\n", RemoteIpv6AddrStr, wanif);
						}
					}

#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
					// Start DHCPv6 client
					// dhclient -6 -sf /var/dhclient-script -lf /var/dhclient6-leases -pf /var/run/dhclient6.pid ppp0 -d -q -N -P
					if ( pEntry->Ipv6Dhcp == 1) {
						snprintf(leasefile, 30, "/var/%s%s.leases", DHCPCV6STR, wanif);
						snprintf(pidfile, 30, "/var/run/%s%s.pid", DHCPCV6STR, wanif);
						make_dhcpcv6_conf(pEntry, dhcpc_cf_buf, sizeof(dhcpc_cf_buf));
						snprintf(value, 128, "/bin/dhclient -6 -sf /etc/dhclient-script -lf %s -pf %s %s -d -q ", leasefile, pidfile, wanif);
						fprintf(fp, value);
						// Request Address
						if ( (pEntry->Ipv6DhcpRequest & 0x1) == 0x1 ) {
							fprintf(fp, "-N ");
						}

						// Request Prefix or DNS
						// Request DNS : Because the para -S  Use Information-request to get only stateless configuration parameters (i.e., without address).  
						//This implies -6.  It also doesn't rewrite the  lease database.
						if ( ((pEntry->Ipv6DhcpRequest & 0x2) == 0x2) || pEntry->dnsv6Mode==REQUEST_DNS ) {
							fprintf(fp, "-P ");
						}
						fprintf(fp, "-cf %s ", dhcpc_cf_buf);
						fprintf(fp, "&\n");
					}
#endif
					fclose(fp);
					chmod(ifup_path, 484);
				}
			}  //End of if(ipv6Enable == 1)
#endif

		}
		else
			ret = -1;
	}
	else {
		// not supported till now
		return -1;
	}

	return ret;
}

/*
 *	generate the ifdown_ppp(vc)x script for WAN interface
 */
static int generate_ifdown_script(unsigned int ifIndex, MIB_CE_ATM_VC_Tp pEntry)
{
	int mark, ret;
	FILE *fp;
	char wanif[6], ifdown_path[32];
	char devname[IFNAMSIZ];
	unsigned char vChar;

	ret = 0;

	if (PPP_INDEX(ifIndex) != DUMMY_PPP_INDEX) {
		// PPP interface
		snprintf(wanif, 6, "ppp%u", PPP_INDEX(ifIndex));
		snprintf(ifdown_path, 32, "/var/ppp/ifdown_%s", wanif);
		if (fp=fopen(ifdown_path, "w+") ) {
			fprintf(fp, "#!/bin/sh\n\n");
#ifdef NEW_PORTMAPPING
			if (pEntry->cmode == CHANNEL_MODE_PPPOE) {
				ifGetName(PHY_INTF(ifIndex), devname, sizeof(devname));
				fprintf(fp, "ip ro change default dev %s table %d\n", devname, caculate_tblid(ifIndex));
			}
#endif

#if defined(CONFIG_IPV6) && defined(CONFIG_BHS) && defined(CONFIG_USER_DHCPV6_ISC_DHCP411)
            /* For Telefonica IPv6 Test Plan, Test Case ID 20178, if PPPoE session is down,			   *
			 * LAN PC global IPv6 address must change the state from "Prefered" to "Obsolete".		   *
			 * So here, if PPPoE connection is down, and DHCPv6 is auto mode, kill the DHCPv6 process  */

			mib_get( MIB_DHCPV6_MODE, (void *)&vChar);
			if (vChar==3) {  // 3:auto mode
				fprintf(fp, "kill `cat /var/run/dhcpd6.pid`\n");
            }
#endif
			fclose(fp);
			chmod(ifdown_path, 484);
		}
		else
			ret = -1;
	}
	else {
		// not supported till now
		return -1;
	}

	return ret;
}

/*
 *	setup the policy-routing for static link
 */
#ifdef IP_POLICY_ROUTING
static int setup_static_PolicyRouting(unsigned int ifIndex)
{
	int i, num, mark, ret, found;
	char str_mark[8], str_rtable[8], ipAddr[32], wanif[IFNAMSIZ];
	struct in_addr inAddr;
	MIB_CE_IP_QOS_T qEntry;

	ret = -1;

	if (ifIndex == DUMMY_IFINDEX)
		return ret;
	else {
		if (PPP_INDEX(ifIndex) != DUMMY_PPP_INDEX) {
			// PPP interface
			return ret;
		}
		else {
			// vc interface
			ifGetName(ifIndex,wanif,sizeof(wanif));
			snprintf(str_rtable, 8, "%d", VC_INDEX(ifIndex)+PR_VC_START);
			if (getInAddr( wanif, DST_IP_ADDR, (void *)&inAddr) == 1)
			{
				strcpy(ipAddr, inet_ntoa(inAddr));
			}
			else
				return ret;
		}

		if (!getInFlags(wanif, 0))
			return ret;	// interface not found
	}

	num = mib_chain_total(MIB_IP_QOS_TBL);
	found = 0;
	// set advanced-routing rule
	for (i=0; i<num; i++) {
		if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void *)&qEntry))
			continue;
#ifdef QOS_DIFFSERV
		if (qEntry.enDiffserv == 1) // Diffserv entry
			continue;
#endif
		if (qEntry.outif == ifIndex) {

			found = 1;
			mark = get_classification_mark(i);
			if (mark != 0) {
				snprintf(str_mark, 8, "%x", mark);
				// Don't forget to point out that fwmark with
				// ipchains/iptables is a decimal number, but that
				// iproute2 uses hexadecimal number.
				// ip ru add fwmark xxx table 3
				va_cmd("/bin/ip", 6, 1, "ru", "add", "fwmark", str_mark, "table", str_rtable);
				// ip ro add default via ipaddr dev vc0 table 3
			}
		}
	}
	if (found) {
		// ip ro add default via ipaddr dev vc0 table 3
		va_cmd("/bin/ip", 9, 1, "ro", "add", "default", "via", ipAddr, "dev", wanif, "table", str_rtable);
		ret = 0;
	}

	return ret;
}
#endif


#ifdef CONFIG_USER_WT_146
#define BFD_SERVER_FIFO_NAME "/tmp/bfd_serv_fifo"
#define BFD_CONF_PREFIX "/var/bfd/bfdconf_"
static void wt146_write_to_bfdmain(struct data_to_pass_st *pmsg)
{
	int bfdmain_fifo_fd=-1;

	bfdmain_fifo_fd = open(BFD_SERVER_FIFO_NAME, O_WRONLY);
	if (bfdmain_fifo_fd == -1)
		fprintf(stderr, "Sorry, no bfdmain server\n");
	else
	{
		write(bfdmain_fifo_fd, pmsg, sizeof(*pmsg));
		close(bfdmain_fifo_fd);
	}
}

#define BFDCFG_DBGLOG		"/var/bfd/bfd_dbg_log"
int wt146_dbglog_get(unsigned char *ifname)
{
	char buff[256];
	FILE *fp;
	unsigned char name[32];
	int dbgflag=0;

	//printf( "%s> enter\n", __FUNCTION__ );
	if(ifname)
	{
		//printf( "%s> open %s, search for %s\n", __FUNCTION__, BFDCFG_DBGLOG, ifname );
		fp = fopen(BFDCFG_DBGLOG, "r");
		if(fp)
		{
			while(fgets(buff, sizeof(buff), fp)!=NULL)
			{
				//printf( "%s> got %s", __FUNCTION__, buff );
				int flag;
				if (sscanf(buff, "%s %d", name, &flag) != 2)
					continue;
				else{
					if( strcmp( name, ifname )==0 )
					{
						dbgflag=flag?1:0;
						break;
					}
				}
			}
			fclose(fp);
		}
	}

	//printf( "%s> exit, dbgflag=%d\n", __FUNCTION__, dbgflag );
	return dbgflag;
}

void wt146_create_wan(MIB_CE_ATM_VC_Tp pEntry, int reset_bfd_only )
{
	if(pEntry)
	{
		char wanif[16], conf_name[32];
		FILE *fp;

		if(pEntry->enable==0)
			return;

		if( (pEntry->cmode!=CHANNEL_MODE_IPOE) &&
			(pEntry->cmode!=CHANNEL_MODE_RT1483) )
			return;

		if( pEntry->bfd_enable==0 ) return 0;

		ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));
		snprintf(conf_name, 32, "%s%s", BFD_CONF_PREFIX, wanif);
		fp=fopen( conf_name, "w" );
		if(fp)
		{
			int bfddebug;
			if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED)
			{
				fprintf( fp, "LocalIP=%s\n",  inet_ntoa(*((struct in_addr *)pEntry->ipAddr)) );
				fprintf( fp, "RemoteIP=%s\n",  inet_ntoa(*((struct in_addr *)pEntry->remoteIpAddr)) );
			}

			fprintf( fp, "OpMode=%u\n", pEntry->bfd_opmode );
			fprintf( fp, "Role=%u\n", pEntry->bfd_role );
			fprintf( fp, "DetectMult=%u\n", pEntry->bfd_detectmult );
			fprintf( fp, "MinTxInt=%u\n", pEntry->bfd_mintxint );
			fprintf( fp, "MinRxInt=%u\n", pEntry->bfd_minrxint );
			fprintf( fp, "MinEchoRxInt=%u\n", pEntry->bfd_minechorxint );
			fprintf( fp, "AuthType=%u\n", pEntry->bfd_authtype );
			if( pEntry->bfd_authtype!=BFD_AUTH_NONE )
			{
				int authkeylen=0;
				fprintf( fp, "AuthKeyID=%u\n", pEntry->bfd_authkeyid );
				fprintf( fp, "AuthKey=" );
					while( authkeylen<pEntry->bfd_authkeylen  )
					{
						fprintf( fp, "%02x", pEntry->bfd_authkey[authkeylen] );
						authkeylen++;
					}
					fprintf( fp, "\n" );
			}
			fprintf( fp, "DSCP=%u\n", pEntry->bfd_dscp );
			if(pEntry->cmode==CHANNEL_MODE_IPOE)
				fprintf( fp, "EthPrio=%u\n", pEntry->bfd_ethprio );
			bfddebug=wt146_dbglog_get( wanif );
			fprintf( fp, "debug=%d\n", bfddebug );
			fclose(fp);

			if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED)
			{
				struct data_to_pass_st msg;
				snprintf(msg.data, BUF_SIZE,"bfdctl add %s file %s", wanif, conf_name);
				TRACE(STA_SCRIPT, "%s\n", msg.data);
				//printf( "%s> %s\n",  __FUNCTION__, msg.data);
				wt146_write_to_bfdmain(&msg);
			}else{
				//only re-init bfd, not re-init wanconnection
				if(reset_bfd_only)
				{
					int dhcpc_pid;
					char dhcp_pid_buf[32];
					snprintf(dhcp_pid_buf, 32, "%s.%s", (char*)DHCPC_PID, wanif);
					dhcpc_pid = read_pid((char*)dhcp_pid_buf);
					if (dhcpc_pid > 0)
						kill(dhcpc_pid, SIGHUP);
				}
			}
		}

		//printf( "%s> create %s bfd session\n", __FUNCTION__, wanif );
	}
}

void wt146_del_wan(MIB_CE_ATM_VC_Tp pEntry)
{
	if(pEntry)
	{
		if(pEntry->enable==0)
			return;

		if( (pEntry->cmode!=CHANNEL_MODE_IPOE) &&
			(pEntry->cmode!=CHANNEL_MODE_RT1483) )
			return;

		//if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED)
		{
				struct data_to_pass_st msg;
				char wanif[16], conf_name[32];

				ifGetName(pEntry->ifIndex, wanif, sizeof(wanif));
				snprintf(conf_name, 32, "%s%s", BFD_CONF_PREFIX, wanif);
				unlink( conf_name);

				snprintf(msg.data, BUF_SIZE,"bfdctl del %s", wanif);
				TRACE(STA_SCRIPT, "%s\n", msg.data);
				//printf( "%s> %s\n",  __FUNCTION__, msg.data);
				wt146_write_to_bfdmain(&msg);

				//printf( "%s> delete %s bfd session\n", __FUNCTION__, wanif );
		}
	}
}

void wt146_set_default_config(MIB_CE_ATM_VC_Tp pEntry)
{
	if(pEntry)
	{
		if( (pEntry->cmode==CHANNEL_MODE_IPOE) ||
			(pEntry->cmode==CHANNEL_MODE_RT1483) )
			pEntry->bfd_enable=1;
		else
			pEntry->bfd_enable=0;

		pEntry->bfd_opmode=BFD_DEMAND_MODE;
		pEntry->bfd_role=BFD_PASSIVE_ROLE;
		pEntry->bfd_detectmult=3;
		pEntry->bfd_mintxint=1000000;
		pEntry->bfd_minrxint=1000000;
		pEntry->bfd_minechorxint=0;
		pEntry->bfd_authtype=BFD_AUTH_NONE;
		pEntry->bfd_authkeyid=0;
		pEntry->bfd_authkeylen=0;
		memset( pEntry->bfd_authkey, 0, sizeof(pEntry->bfd_authkey) );
		pEntry->bfd_dscp=0;
		pEntry->bfd_ethprio=0;
	}
}

void wt146_copy_config(MIB_CE_ATM_VC_Tp pto, MIB_CE_ATM_VC_Tp pfrom)
{
	if(pto && pfrom)
	{
		//printf( "\n\nwt146_copy_config\n" );
		pto->bfd_enable=pfrom->bfd_enable;
		pto->bfd_opmode=pfrom->bfd_opmode;
		pto->bfd_role=pfrom->bfd_role;
		pto->bfd_detectmult=pfrom->bfd_detectmult;
		pto->bfd_mintxint=pfrom->bfd_mintxint;
		pto->bfd_minrxint=pfrom->bfd_minrxint;
		pto->bfd_minechorxint=pfrom->bfd_minechorxint;
		pto->bfd_authtype=pfrom->bfd_authtype;
		pto->bfd_authkeyid=pfrom->bfd_authkeyid;
		pto->bfd_authkeylen=pfrom->bfd_authkeylen;
		memcpy( pto->bfd_authkey, pfrom->bfd_authkey, sizeof(pto->bfd_authkey) );
		pto->bfd_dscp=pfrom->bfd_dscp;
		pto->bfd_ethprio=pfrom->bfd_ethprio;
	}
}
#endif //CONFIG_USER_WT_146

//Inform kernel the number of routing interface.
// flag = 1 to increase 1
// flag = 0 to decrease 1
static void internetLed_route_if(int flag)
{
	FILE *fp;

	fp = fopen ("/proc/IntnetLED", "w");
	if (fp)
	{
		if (flag)
			fprintf(fp,"+");
		else
			fprintf(fp,"-");
		fclose(fp);
	}
}

static int setup_ethernet_config(MIB_CE_ATM_VC_Tp pEntry, char *wanif)
{
	char *argv[8];
	int status=0;
	unsigned char devAddr[MAC_ADDR_LEN];
	char macaddr[MAC_ADDR_LEN*2+1];
	char sValue[8];

	// ifconfig vc0 down
	va_cmd(IFCONFIG, 2, 1, wanif, "down");
#ifdef WLAN_WISP
	if(MEDIA_INDEX(pEntry->ifIndex) != MEDIA_WLAN)
	{
#endif
#if defined(CONFIG_LUNA) && defined(GEN_WAN_MAC)
#ifdef CONFIG_ATM_CLIP
		if(pEntry->cmode != CHANNEL_MODE_RT1577)
#endif
		{
			snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
				pEntry->MacAddr[0], pEntry->MacAddr[1], pEntry->MacAddr[2], pEntry->MacAddr[3], pEntry->MacAddr[4], pEntry->MacAddr[5]);

			argv[1]=wanif;
			argv[2]="hw";
			argv[3]="ether";
			argv[4]=macaddr;
			argv[5]=NULL;
			TRACE(STA_SCRIPT, "%s %s %s %s %s\n", IFCONFIG, argv[1], argv[2], argv[3], argv[4]);
			status|=do_cmd(IFCONFIG, argv, 1);
		}
#else	// #if defined(CONFIG_LUNA) && defined(GEN_WAN_MAC)
#ifdef CONFIG_ATM_CLIP
		if (mib_get(MIB_ELAN_MAC_ADDR, (void *)devAddr) != 0 && pEntry->cmode != CHANNEL_MODE_RT1577)
#else
		if (mib_get(MIB_ELAN_MAC_ADDR, (void *)devAddr) != 0)
#endif
		{
			snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
				devAddr[0], devAddr[1], devAddr[2],
#ifdef CONFIG_USER_IPV6READYLOGO_ROUTER
				//Set vc and nas mac with br0's mac plus 1
				devAddr[3], devAddr[4], devAddr[5]+1);
#else
				devAddr[3], devAddr[4], devAddr[5]);
#endif
			argv[1]=wanif;
			argv[2]="hw";
			argv[3]="ether";
			argv[4]=macaddr;
			argv[5]=NULL;
			TRACE(STA_SCRIPT, "%s %s %s %s %s\n", IFCONFIG, argv[1], argv[2],
						argv[3], argv[4]);
			status|=do_cmd(IFCONFIG, argv, 1);
		}
#endif
#ifdef WLAN_WISP
	}
#endif

//#ifdef CONFIG_RTK_L34_ENABLE
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE) || defined(CONFIG_FIBER_FEATURE) || defined(CONFIG_RTK_L34_ENABLE) || defined(CONFIG_USER_LAN_PORT_AS_ETH_WAN)


		char sysbuf[128];
		int wanPhyPort;
#if 0
		if(mib_get(MIB_WAN_PHY_PORT,(void *)&wanPhyPort)){
			sprintf( sysbuf, "/bin/echo %d nas0 > /proc/rtl8686gmac/dev_port_mapping",wanPhyPort );
		}
#endif


#ifdef CONFIG_RTK_L34_ENABLE
		wanPhyPort = RG_get_wan_phyPortId();
#elif defined(CONFIG_USER_LAN_PORT_AS_ETH_WAN)
		wanPhyPort = -1;
		mib_get(MIB_ETH_WAN_PORT_PHY_INDEX, &wanPhyPort);
#elif defined(CONFIG_RTL9607C_SERIES)
	        wanPhyPort = 5;
#elif defined(CONFIG_RTL9600_SERIES)
	        wanPhyPort = 4;
#elif defined(CONFIG_RTL9602C_SERIES)
	        wanPhyPort = 4;
#else
			wanPhyPort = -1;
#endif

		if(wanPhyPort!=-1){
			sprintf( sysbuf, "/bin/echo %d nas0 > /proc/rtl8686gmac/dev_port_mapping",wanPhyPort );
		}

		/*
		#ifdef CONFIG_RG_API_RLE0371
		sprintf( sysbuf, "/bin/echo 5 nas0 > /proc/rtl8686gmac/dev_port_mapping" );
		#else
		sprintf( sysbuf, "/bin/echo 3 nas0 > /proc/rtl8686gmac/dev_port_mapping" );
		#endif
		*/
		printf( "system(): %s\n", sysbuf );
		system(sysbuf);

		//Temporary solution for netlink event down/up that will let spppd has ubnormal behavior.
		sleep(1);
#endif

#ifdef CONFIG_USER_CMD_CLIENT_SIDE
/*	this is not final version, 
 *	- change PTM root device to nas0 for G.fast
 *	- reserve last LAN port for remote phy communication
 *		=> 9607CP original LAN port 0~3, so reserve port 3
 *		=> should change user config CONFIG_LAN_PORT_NUM to 3
 *	- change ethernet wan to another one. */
		char buf[128]={0};
		sprintf(buf, "/bin/echo %d %s > /proc/rtl8686gmac/dev_port_mapping", SWITCH_WAN_PORT, ALIASNAME_PTM0);
		system(buf);

#ifdef CONFIG_HWNAT_FLEETCONNTRACK
		sprintf(buf, "echo %d > /proc/fc/ctrl/wan_port_mask", SWITCH_WAN_PORT_MASK);		// change fc wan port mask
		system(buf);		// change fc wan port mask
#endif
#endif

		// ifconfig vc0 mtu 1500
		if (pEntry->cmode != CHANNEL_MODE_BRIDGE && pEntry->cmode != CHANNEL_MODE_PPPOE
			&& pEntry->cmode != CHANNEL_MODE_PPPOA) {
			sprintf(sValue, "%u", pEntry->mtu);
			argv[1] = wanif;
			argv[2] = "mtu";
			argv[3] = &sValue[0];
			argv[4] = NULL;
			TRACE(STA_SCRIPT, "%s %s %s %s\n", IFCONFIG, argv[1], argv[2], argv[3]);
			status|=do_cmd(IFCONFIG, argv, 1);
		}
		
		// ifconfig vc0 txqueuelen 10
		argv[1] = wanif;
		argv[2] = "txqueuelen";
#ifdef NEW_IP_QOS_SUPPORT
		argv[3] = "100";
#else
                argv[3] = "10";
#endif
		argv[4] = NULL;
		TRACE(STA_SCRIPT, "%s %s %s %s\n", IFCONFIG, argv[1], argv[2], argv[3]);
		status|=do_cmd(IFCONFIG, argv, 1);
#ifdef CONFIG_IPV6
		// Disable ipv6 in bridge
		if (pEntry->cmode == CHANNEL_MODE_BRIDGE)
			setup_disable_ipv6(wanif, 1);
		else { // enable ipv6 if applicable
			if (pEntry->IpProtocol & IPVER_IPV6)
				setup_disable_ipv6(wanif, 0);
			else
				setup_disable_ipv6(wanif, 1);
		}
#endif
		// ifconfig vc0 up
		argv[2] = "up";
		argv[3] = NULL;
		TRACE(STA_SCRIPT, "%s %s %s\n", IFCONFIG, argv[1], argv[2]);
		status|=do_cmd(IFCONFIG, argv, 1);

		return status;
}

#ifdef CONFIG_DEV_xDSL
#ifdef CONFIG_RTL_MULTI_PVC_WAN
static void addVCVirtualDev(MIB_CE_ATM_VC_Tp pEntry)
{
	MEDIA_TYPE_T mType;
	char ifname[IFNAMSIZ];  // virtual vc device name
	char wanif[IFNAMSIZ];   // major vc device name

	// Get virtual vc device name
	ifGetName(PHY_INTF(pEntry->ifIndex), ifname, sizeof(ifname));
	// Get major vc device name
	ifGetName(MAJOR_VC_INTF(pEntry->ifIndex), wanif, sizeof(wanif));
	mType = MEDIA_INDEX(pEntry->ifIndex);

	//add new vcwan dev here.
	if (mType == MEDIA_ATM && (WAN_MODE & MODE_ATM)) {
		int tmp_group;
		const char smux_brg_cmd[]="/bin/ethctl addsmux bridge %s %s";
		const char smux_ipoe_cmd[]="/bin/ethctl addsmux ipoe %s %s";
		const char smux_pppoe_cmd[]="/bin/ethctl addsmux pppoe %s %s";
		char cmd_str[100];

		//va_cmd(IFCONFIG, 2, 1, "vc0", "up");
		if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
			snprintf(cmd_str, 100, smux_brg_cmd, wanif, ifname);
		else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_IPOE)
			snprintf(cmd_str, 100, smux_ipoe_cmd, wanif, ifname);
		else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
			snprintf(cmd_str, 100, smux_pppoe_cmd, wanif, ifname);

		if (pEntry->napt)
			strncat(cmd_str, " napt", 100);

		if (pEntry->vlan) {
			unsigned int vlantag;
			vlantag = (pEntry->vid|((pEntry->vprio) << 13));
			snprintf(cmd_str, 100, "%s vlan %d", cmd_str, vlantag );
		}
		printf("TRACE: %s\n", cmd_str);
		system(cmd_str);
	}
}
#endif

/*
 *	ATM channel: RFC2684, RFC1577, PPPoA
 */
static int setup_ATM_channel(MIB_CE_ATM_VC_Tp pEntry)
{
	struct data_to_pass_st msg;
	char *aal5Encap, qosParms[64];
	char wanif[IFNAMSIZ];
	int pcreg, screg;
	int repeat_i;
#ifdef CONFIG_ATM_CLIP
	unsigned long addr;
#endif
#ifdef CONFIG_USER_CMD_CLIENT_SIDE
	struct rtk_xdsl_atm_channel_info data;
#endif

	if (MEDIA_INDEX(pEntry->ifIndex) != MEDIA_ATM)
		return 0;

	// get the aal5 encapsulation
	if (pEntry->encap == ENCAP_VCMUX)
	{
#ifdef CONFIG_ATM_CLIP
		if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1483 || (CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1577)
#else
		if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1483)
#endif
			aal5Encap = (char *)VC_RT;
		else
			aal5Encap = (char *)VC_BR;
	}
	else	// LLC
	{
		if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1483)
			aal5Encap = (char *)LLC_RT;
#ifdef CONFIG_ATM_CLIP
		else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1577)
			aal5Encap = (char *)LLC_1577;
#endif
		else
			aal5Encap = (char *)LLC_BR;
	}

	// calculate the 15-0(bits) PCR CTLSTS value
	pcreg = cr2reg(pEntry->pcr);

	if ((ATM_QOS_T)pEntry->qos == ATMQOS_CBR)
	{
		snprintf(qosParms, 64, "cbr:pcr=%u", pcreg);
	}
	else if ((ATM_QOS_T)pEntry->qos == ATMQOS_VBR_NRT)
	{
		screg = cr2reg(pEntry->scr);
		snprintf(qosParms, 64, "nrt-vbr:pcr=%u,scr=%u,mbs=%u",
			pcreg, screg, pEntry->mbs);
	}
	else if ((ATM_QOS_T)pEntry->qos == ATMQOS_VBR_RT)
	{
		screg = cr2reg(pEntry->scr);
		snprintf(qosParms, 64, "rt-vbr:pcr=%u,scr=%u,mbs=%u",
			pcreg, screg, pEntry->mbs);
	}
	else //if ((ATM_QOS_T)pEntry->qos == ATMQOS_UBR)
	{
		snprintf(qosParms, 64, "ubr:pcr=%u", pcreg);
	}
	// Mason Yu Test
	//printf("qosParms=%s\n", qosParms);
#ifdef CONFIG_RTL_MULTI_PVC_WAN
	ifGetName(MAJOR_VC_INTF(pEntry->ifIndex), wanif, sizeof(wanif));
#else
	ifGetName(PHY_INTF(pEntry->ifIndex), wanif, sizeof(wanif));
#endif
	// start this vc
	if (pEntry->cmode != CHANNEL_MODE_PPPOA && !find_pvc_from_running(pEntry->vpi, pEntry->vci))
	{
		unsigned char devAddr[MAC_ADDR_LEN];
		char macaddr[MAC_ADDR_LEN*2+1];

		// mpoactl add vc0 pvc 0.33 encaps 4 qos xxx brpppoe x

#ifdef CONFIG_ATM_CLIP
		if (pEntry->cmode != CHANNEL_MODE_RT1577)
#endif

		// 20130426 W.H. Hung: briged PPPoE setup has been replaced
		// by ebtables rules.
		snprintf(msg.data, BUF_SIZE,
			"mpoactl add %s pvc %u.%u encaps %s qos %s", wanif,
			pEntry->vpi, pEntry->vci, aal5Encap, qosParms);
#ifdef CONFIG_ATM_CLIP
		else
			snprintf(msg.data, BUF_SIZE,
				"mpoactl addclip %s pvc %u.%u encaps %s qos %s", wanif,
				pEntry->vpi, pEntry->vci, aal5Encap, qosParms);
#endif

#ifdef CONFIG_USER_CMD_CLIENT_SIDE
#ifdef CONFIG_RTL_MULTI_PVC_WAN
		if (VC_MINOR_INDEX(MAJOR_VC_INTF(pEntry->ifIndex)) == DUMMY_VC_MINOR_INDEX) // major vc devname
			data.ch_no = VC_MAJOR_INDEX(MAJOR_VC_INTF(pEntry->ifIndex));
		else{
			//data.ch_no = VC_MAJOR_INDEX(MAJOR_VC_INTF(pEntry->ifIndex));
		}
#else
		data.ch_no = VC_INDEX(PHY_INTF(pEntry->ifIndex));
#endif
		data.vpi = pEntry->vpi;
		data.vci = pEntry->vci;
		if(aal5Encap == VC_BR){
			data.rfc=RTK_XDSL_VC_MUX;
			data.framing=RTK_XDSL_RFC1483_BRIDGED;
		}else if(aal5Encap == LLC_BR){
			data.rfc=RTK_XDSL_LLC_SNAP;
			data.framing=RTK_XDSL_RFC1483_BRIDGED;
		}else if(aal5Encap == VC_RT){
			data.rfc=RTK_XDSL_VC_MUX;
			data.framing=RTK_XDSL_RFC1483_ROUTED;
		}else if(aal5Encap == LLC_RT){
			data.rfc=RTK_XDSL_LLC_SNAP;
			data.framing=RTK_XDSL_RFC1483_ROUTED;
		}

		//setting QoS 
		data.qos_type = ((ATM_QOS_T)pEntry->qos)+1;
		data.pcr = pcreg;
		data.scr = screg;
		data.mbs = pEntry->mbs;

		commserv_atmSetup(0, &data);
#endif

		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_mpoad(&msg);

		{
			//make sure the channel is created (ifconfig hw ether may fail because the vc# is not ready)
			repeat_i = 0;
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			while( (repeat_i<10) )
#else
			while( (repeat_i<10) && (!find_pvc_from_running(pEntry->vpi, pEntry->vci)) )
#endif
			{
				usleep(50000);
				repeat_i++;
			}
		}
#ifdef CONFIG_RTL_MULTI_PVC_WAN
		// start root vc for multi-wan PVC
		setup_ethernet_config(pEntry, wanif);
#endif
	}

	switch (pEntry->cmode) {
#ifdef CONFIG_PPP
	case CHANNEL_MODE_PPPOA:
		// PPPoA
		printf("PPPoA\n");
		if (startPPP(0, pEntry, qosParms, CHANNEL_MODE_PPPOA) == -1)
		{
			printf("start PPPoA failed !\n");
			return 0;
		}

		//wait finishing sar_open; sar_open will reset SAR header, so sarctl set_header must be called after it
		repeat_i = 0;
		while( (repeat_i < 20) && (!find_pvc_from_running(pEntry->vpi, pEntry->vci)) )
		{
			usleep(50000);
			repeat_i++;
		}
		if (repeat_i >= 20) printf("\nPPPoA channel may not be configured properly, please reboot to reload the configuration!\n");
		break;
#endif
#ifdef CONFIG_ATM_CLIP
	case CHANNEL_MODE_RT1577:
		// rfc1577-routed
		printf("1577 routed\n");

		if (startIP(wanif, pEntry, CHANNEL_MODE_RT1577) == -1)
		{
			printf("start 1577-routed failed !\n");
			return 0;
		}
		addr = *((unsigned long *)pEntry->ipAddr);
		snprintf(msg.data, BUF_SIZE, "mpoactl set %s cip %lu", wanif, addr);
		write_to_mpoad(&msg);

		break;
#endif
	}

#ifdef CONFIG_RTL_MULTI_PVC_WAN
	// create virtual interface, ex. vc0_0
	addVCVirtualDev(pEntry);
#endif
	return 1;
}
#endif // CONFIG_DEV_xDSL


#ifdef CONFIG_HWNAT
void setup_hwnat_eth_member(int port, int mbr, char enable)
{
	struct ifreq ifr;
	struct ifvlan ifvl;

#ifndef CONFIG_RTL_MULTI_ETH_WAN
	strcpy(ifr.ifr_name, ALIASNAME_NAS0);
#else
	sprintf(ifr.ifr_name, "%s%d", ALIASNAME_MWNAS, ETH_INDEX(port));
#endif
	ifr.ifr_data = (char *)&ifvl;
	ifvl.enable=enable;
	ifvl.port=port;
	ifvl.member=mbr;
	if(setEthPortMapping(&ifr)!=1)
		printf("set hwnat port mapping error!\n");
	return;
}
#ifdef CONFIG_PTMWAN
void setup_hwnat_ptm_member(int port, int mbr, char enable)
{
	struct ifreq ifr;
	struct ifvlan ifvl;

	sprintf(ifr.ifr_name, "%s%d", ALIASNAME_MWPTM, PTM_INDEX(port));
	ifr.ifr_data = (char *)&ifvl;
	ifvl.enable=enable;
	ifvl.port=port;
	ifvl.member=mbr;
	if(setEthPortMapping(&ifr)!=1)
		printf("set ptm hwnat port mapping error!\n");
	return;
}
#endif /*CONFIG_PTMWAN*/
#endif

#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_ENABLE)
int Init_RTK_RG_Device(void)
{
	char wanif[IFNAMSIZ];
	int i;

	va_cmd(BRCTL, 2, 1, "addbr", BRIF );

	for(i=1;i<6;i++)
	{
		snprintf( wanif, sizeof(wanif), "eth0.%d", i);
		va_cmd(IFCONFIG, 2, 1, wanif, "down");
	}
	snprintf( wanif, sizeof(wanif), "eth0");
	va_cmd(BRCTL, 3, 1, "addif", BRIF, wanif);

//	snprintf( wanif, sizeof(wanif), "nas0");
//	va_cmd(BRCTL, 3, 1, "delif", BRIF, wanif);

/*
	for(i=1;i<6;i++)
	{
		snprintf( wanif, sizeof(wanif), "eth0.%d", i);
		va_cmd(BRCTL, 3, 1, "delif", BRIF, wanif);
	}
*/
	snprintf( wanif, sizeof(wanif), "eth0");
	va_cmd(IFCONFIG, 2, 1, wanif, "down");

	snprintf( wanif, sizeof(wanif), "nas0");
	va_cmd(IFCONFIG, 2, 1, wanif, "down");

	snprintf( wanif, sizeof(wanif), "br0");
	va_cmd(IFCONFIG, 2, 1, wanif, "down");
}
#endif

// return value:
// 0  : successful
// -1 : failed
int startConnection(MIB_CE_ATM_VC_Tp pEntry, int mib_vc_idx)
{
	struct data_to_pass_st msg;
	char *aal5Encap, qosParms[64], wanif[IFNAMSIZ];
	int brpppoe;
	int pcreg, screg;
	int status=0;
#ifdef CONFIG_USER_MINIUPNPD
	unsigned char upnpdEnable;
	unsigned int upnpItf;
#endif
	MEDIA_TYPE_T mType;
	char vChar=-1;
#if defined(CONFIG_USER_UPNPD)||defined(CONFIG_USER_MINIUPNPD)
	char ext_ifname[IFNAMSIZ];
#endif

	if(pEntry == NULL || pEntry->enable == 0)
	{
		return 0;
	}

	mType = MEDIA_INDEX(pEntry->ifIndex);
#ifdef CONFIG_DEV_xDSL
	if (mType == MEDIA_ATM)
		setup_ATM_channel(pEntry);
#else
	if (mType == MEDIA_ATM)
		return 0;
#endif
	if (pEntry->cmode != CHANNEL_MODE_BRIDGE)
		internetLed_route_if(1);//+1

#ifdef NEW_PORTMAPPING
		get_pmap_fgroup(pmap_list, MAX_VC_NUM);
		setup_wan_pmap_lanmember(mType, pEntry->ifIndex);
#endif

	//snprintf(wanif, 5, "vc%d", VC_INDEX(pEntry->ifIndex));
	ifGetName(PHY_INTF(pEntry->ifIndex),wanif,sizeof(wanif));
	//printf("%s true_%s, major dev=%s\n", __func__, MEDIA_INDEX(pEntry->ifIndex)?"ETH":"ATM", wanif);

	if (pEntry->cmode != CHANNEL_MODE_PPPOA)
	{
		setup_ethernet_config(pEntry, wanif);
	}

// Mason Yu. enable_802_1p_090722
#ifdef CONFIG_DEV_xDSL
#ifdef ENABLE_802_1Q
	// This vlan tag function is mutual exclusive with multi-wan pvc.
#ifndef CONFIG_RTL_MULTI_PVC_WAN
	// mpoactl set vc0 vlan 1 pvid 1
	snprintf(msg.data, BUF_SIZE,
		"mpoactl set %s vlan %d vid %d vprio %d", wanif,
		pEntry->vlan, pEntry->vid, pEntry->vprio);
	if (pEntry->cmode == CHANNEL_MODE_BRIDGE || pEntry->cmode == CHANNEL_MODE_IPOE ||
		pEntry->cmode == CHANNEL_MODE_PPPOE || pEntry->cmode == CHANNEL_MODE_6RD) {
		if (mType == MEDIA_ATM) {
			// mpoactl set vc0 vlan 1 pvid 1
			TRACE(STA_SCRIPT, "%s\n", msg.data);
			status|=write_to_mpoad(&msg);
		}
	}
#endif
#endif
#endif
	printf("[%s@%d] pEntry->ifIndex = %d, pEntry->cmode = %d \n", __FUNCTION__, __LINE__, pEntry->ifIndex, pEntry->cmode);

	if (pEntry->cmode == CHANNEL_MODE_BRIDGE)
	{
		// rfc1483-bridged
		// brctl addif br0 vc0
		printf("1483 bridged\n");
		// Mason Yu
		//cxy 2016-12-22: CHANNEL_MODE_BRIDGE always add wanif to br0 
		//if ( pEntry->brmode != BRIDGE_DISABLE )
		status|=va_cmd(BRCTL, 3, 1, "addif", BRIF, wanif);
	}
	else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_IPOE)
	{
		// MAC Encapsulated Routing
		printf("1483 MER\n");

		if (startIP(wanif, pEntry, CHANNEL_MODE_IPOE) == -1)
		{
			printf("start MER failed !\n");
			status=-1;
		}
#ifdef PPPOE_PASSTHROUGH
		//if (pEntry->brmode == BRIDGE_PPPOE)	// enable PPPoE pass-through
		if (pEntry->brmode != BRIDGE_DISABLE)	// enable bridge
			// brctl addif br0 vc0
			status|=va_cmd(BRCTL, 3, 1, "addif", BRIF, wanif);
#endif
		if ( pEntry->ipDhcp == DHCP_DISABLED ) {
#ifdef IP_POLICY_ROUTING
			setup_static_PolicyRouting(pEntry->ifIndex);
#endif
		}
	}
#ifdef CONFIG_PPP
	else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
	{
		// PPPoE
		printf("PPPoE\n");

		if (startPPP(wanif, pEntry, 0, CHANNEL_MODE_PPPOE) == -1)
		{
			printf("start PPPoE failed !\n");
			status=-1;
		}

#ifdef PPPOE_PASSTHROUGH
		//if (pEntry->brmode == BRIDGE_PPPOE)	// enable PPPoE pass-through
		if (pEntry->brmode != BRIDGE_DISABLE)	// enable bridge
			// brctl addif br0 vc0
			status|=va_cmd(BRCTL, 3, 1, "addif", BRIF, wanif);
#endif
	}
#endif
#ifdef CONFIG_DEV_xDSL
	else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1483)
	{
		// rfc1483-routed
		printf("1483 routed\n");
		if (startIP(wanif, pEntry, CHANNEL_MODE_RT1483) == -1)
		{
			printf("start 1483-routed failed !\n");
			status=-1;
		}
#ifdef IP_POLICY_ROUTING
		setup_static_PolicyRouting(pEntry->ifIndex);
#endif
	}
#endif
#if defined(CONFIG_IPV6) && defined(CONFIG_IPV6_SIT_6RD)
	else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_6RD)
	{
		// 6RD
		printf("6RD\n");
		if (startIP(wanif, pEntry, CHANNEL_MODE_IPOE) == -1)
		{
			printf("start 6RD failed !\n");
			status=-1;
		}
	}
#endif

// Magician: UPnP Daemon Start
#ifdef CONFIG_USER_MINIUPNPD
	if(mib_get(MIB_UPNP_DAEMON, (void *)&upnpdEnable) && upnpdEnable)
	{
		if (mib_get(MIB_UPNP_EXT_ITF, (void *)&upnpItf) && upnpItf == pEntry->ifIndex)
		{
			ifGetName(upnpItf, ext_ifname, sizeof(ext_ifname));

			va_cmd("/bin/upnpctrl", 1, 1, "sync");
			va_cmd("/bin/upnpctrl", 3, 1, "up", ext_ifname, BRIF);
		}
	}
#endif
// The end of UPnP Daemon Start

#ifdef CONFIG_USER_WT_146
	wt146_create_wan(pEntry, 0);
#endif //CONFIG_USER_WT_146

	if (status >= 0) {
		generate_ifup_script(pEntry->ifIndex, pEntry);
		generate_ifdown_script(pEntry->ifIndex, pEntry);
	}

#if defined (CONFIG_EPON_FEATURE) && defined(CONFIG_RTK_L34_ENABLE)
//cxy 2016-6-6: oam need to add iptv wan vid as default multicast vlan
	if(pEntry->applicationtype == X_CT_SRV_OTHER)
	{
		int pon_mode=0;
		int i;
		mib_get(MIB_PON_MODE, &pon_mode);
		if(pon_mode == EPON_MODE)
		{
			char cmdstr[128];
			for(i=0; i<SW_LAN_PORT_NUM; i++)
			{
				if((pEntry->itfGroup)&(1<<i))
				{
					snprintf(cmdstr, sizeof(cmdstr), "oamcli set igmp mcvlan %d %d 1", i, pEntry->vid);
					system(cmdstr);
				}
			}
		}
	}
#endif

	return status;
}

#if defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN)
static void addEthWANdev(MIB_CE_ATM_VC_Tp pEntry)
{
	MEDIA_TYPE_T mType;
	char ifname[IFNAMSIZ];
	int flag=0;

	ifGetName(PHY_INTF(pEntry->ifIndex), ifname, sizeof(ifname));
	mType = MEDIA_INDEX(pEntry->ifIndex);

	//add new ethwan dev here.
	if( ((mType == MEDIA_ETH) && (WAN_MODE & MODE_Ethernet))
	     #ifdef CONFIG_PTMWAN
	     ||((mType == MEDIA_PTM) && (WAN_MODE & MODE_PTM))
	     #endif /*CONFIG_PTMWAN*/
	){
		int tmp_group;
		const char smux_brg_cmd[]="/bin/ethctl addsmux bridge %s %s";
		const char smux_ipoe_cmd[]="/bin/ethctl addsmux ipoe %s %s";
		const char smux_pppoe_cmd[]="/bin/ethctl addsmux pppoe %s %s";

		char cmd_str[128]={0};
		unsigned char *rootdev=NULL;		
		
		#ifdef CONFIG_PTMWAN
		if(mType == MEDIA_PTM)
			rootdev=ALIASNAME_PTM0;
		else
		#endif /*CONFIG_PTMWAN*/
			rootdev=ALIASNAME_NAS0;
		
		// Mason Yu
		//va_cmd(IFCONFIG, 2, 1, rootdev, "up");
		
		if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
			snprintf(cmd_str, 100, smux_brg_cmd, rootdev, ifname);
		else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_IPOE || (CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_6RD)
			snprintf(cmd_str, 100, smux_ipoe_cmd, rootdev, ifname);
		else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
			snprintf(cmd_str, 100, smux_pppoe_cmd, rootdev, ifname);

		if (pEntry->napt)
			strncat(cmd_str, " napt", 100);

		if(pEntry->brmode != BRIDGE_DISABLE)
			strncat(cmd_str, " brpppoe", 100);

		if (pEntry->vlan) {
			unsigned int vlantag;
			vlantag = (pEntry->vid|((pEntry->vprio) << 13));
			snprintf(cmd_str, 100, "%s vlan %d", cmd_str, vlantag );
		}
		printf("TRACE: %s\n", cmd_str);
		system(cmd_str);
		#if !defined(CONFIG_RTL9601B_SERIES) && !defined(CONFIG_SFU)
		while(getInFlags(ifname, &flag)==0) { //wait the device created successly, Iulian Wu
			printf("Ethernet WAN not ready!\n");
#ifdef CONFIG_00R0
			sleep(10);
#else
			sleep(2);
#endif
		}
		#endif
	}
}
#endif

#ifdef CONFIG_RTL_MULTI_PVC_WAN
static int find_latest_used_vc(MIB_CE_ATM_VC_Tp pEntry)
{
	int entryNum=0, i, found_another=0;
	MIB_CE_ATM_VC_T Entry;

	entryNum =  mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;
		if (pEntry->ifIndex == Entry.ifIndex)
			continue;

		if (MEDIA_INDEX(Entry.ifIndex) == MEDIA_ATM) {
		if (VC_MAJOR_INDEX(Entry.ifIndex) == VC_MAJOR_INDEX(pEntry->ifIndex)) {
				found_another = 1;
		}
	}
	}
	return found_another;
}
#endif

int underlying_pppoe_exist(MIB_CE_ATM_VC_Tp pEntry)
{
	int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char wandev[IFNAMSIZ];
	
	entryNum =  mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if (Entry.cmode != CHANNEL_MODE_PPPOE)
			continue;
		if (VC_INDEX(Entry.ifIndex) != VC_INDEX(pEntry->ifIndex))
			continue;
		if (MEDIA_INDEX(pEntry->ifIndex) != MEDIA_INDEX(Entry.ifIndex))
			continue;
		if (PPP_INDEX(pEntry->ifIndex) == PPP_INDEX(Entry.ifIndex))
			continue;
		// use the same underlying VC/nas/ptm
		ifGetName( PHY_INTF(pEntry->ifIndex), wandev, sizeof(wandev));
		printf("%s:%d:use the same underlying %s interface.\n", __FUNCTION__, __LINE__, wandev);
		return 1;
	}
	return 0;
	
}

void stopConnection(MIB_CE_ATM_VC_Tp pEntry)
{
	struct data_to_pass_st msg;
	char wandev[IFNAMSIZ];
	char wanif[IFNAMSIZ];
	char myip[16];
	int i;
#ifdef CONFIG_RTL_MULTI_PVC_WAN
	char ifname[IFNAMSIZ];   // major vc device name
	int found_another=0;
#endif
#ifdef CONFIG_USER_MINIUPNPD
	unsigned char upnpdEnable;
	unsigned int upnpItf;
	char ext_ifname[IFNAMSIZ];
#endif
#ifdef CONFIG_USER_CMD_CLIENT_SIDE
	int ch_no=-1;
#endif

	clean_SourceRoute(pEntry);

#if defined(PORT_FORWARD_GENERAL) || defined(DMZ)
#ifdef NAT_LOOPBACK
	cleanOneEntry_NATLB_rule_dynamic_link(pEntry, DEL_ALL_NATLB_DYNAMIC);
#endif
#endif

#ifdef CONFIG_USER_WT_146
	wt146_del_wan(pEntry);
#endif //CONFIG_USER_WT_146

// Magician: UPnP Daemon Stop
#ifdef CONFIG_USER_MINIUPNPD
	if (mib_get(MIB_UPNP_DAEMON, &upnpdEnable) && upnpdEnable) {
		if (mib_get(MIB_UPNP_EXT_ITF, &upnpItf)
		    && upnpItf == pEntry->ifIndex) {
			ifGetName(upnpItf, ext_ifname, sizeof(ext_ifname));

			va_cmd("/bin/upnpctrl", 3, 1, "down", ext_ifname, BRIF);
		}
	}
#endif
// The end of UPnP Daemon Stop

#ifdef CONFIG_PPP
	// delete this vc
	if (pEntry->cmode == CHANNEL_MODE_PPPOE || pEntry->cmode == CHANNEL_MODE_PPPOA)
	{
		int i,entryNum;
		MIB_CE_ATM_VC_T Entry;
		// spppctl del 0
		snprintf(msg.data, BUF_SIZE,
			"spppctl del %u", PPP_INDEX(pEntry->ifIndex));		
		snprintf(wanif, 6, "ppp%u", PPP_INDEX(pEntry->ifIndex));
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
		
#if 0
		/* Andrew, need to remove all PPP using the same underlying VC or kernel will have NULL references */
		entryNum =  mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < entryNum; i++) {
			/* Retrieve entry */
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				continue;

			if ((Entry.cmode != CHANNEL_MODE_PPPOE) && (Entry.cmode != CHANNEL_MODE_PPPOA))
				continue;
			if (VC_INDEX(Entry.ifIndex) != VC_INDEX(pEntry->ifIndex))
				continue;
			if (MEDIA_INDEX(pEntry->ifIndex) != MEDIA_INDEX(Entry.ifIndex))
				continue;

			snprintf(msg.data, BUF_SIZE, "spppctl del %u", PPP_INDEX(Entry.ifIndex));
			TRACE(STA_SCRIPT, "%s\n", msg.data);
			write_to_pppd(&msg);
		}
#endif
	}
	else{
#endif
		//snprintf(wanif, 6, "vc%u", VC_INDEX(pEntry->ifIndex));
		ifGetName( PHY_INTF(pEntry->ifIndex), wanif, sizeof(wanif));
#ifdef CONFIG_PPP
	}
#endif

	ifGetName( PHY_INTF(pEntry->ifIndex), wandev, sizeof(wandev));

#ifdef CONFIG_IPV6
	stopIP_PPP_for_V6(pEntry);
#endif

	sleep(1);
	va_cmd(BRCTL, 3, 1, "delif", BRIF, wandev);

	if (pEntry->cmode != CHANNEL_MODE_BRIDGE)
		internetLed_route_if(0);//-1

	/* Kevin, stop root Qdisc before ifdown interface, for 0412 hw closing IPQoS */
	va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", wandev, "root");

	if (pEntry->cmode != CHANNEL_MODE_PPPOA) {
		//ifconfig vc0 0.0.0.0
		va_cmd(IFCONFIG, 2, 1, wanif, "0.0.0.0");
		// ifconfig vc0 down
		va_cmd(IFCONFIG, 2, 1, wanif, "down");
		// wait for sar to process the queueing packets
		//usleep(1000);
		for (i=0; i<10000000; i++);

		#ifdef CONFIG_DEV_xDSL
		if (MEDIA_INDEX(pEntry->ifIndex) == MEDIA_ATM) {
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			// Get major vc device name
			ifGetName(MAJOR_VC_INTF(pEntry->ifIndex), ifname, sizeof(ifname));
			// ethctl remsumx vcX vcX_X
			//printf("** Remove smux device %s(root %s)\n", wanif, ifname);
			if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
			{
				va_cmd("/bin/ethctl", 4, 1, "remsmux", "bridge", ifname, wandev);
			}
			else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_IPOE)
			{
				va_cmd("/bin/ethctl", 4, 1, "remsmux", "ipoe", ifname, wandev);
			}
			else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
			{
				va_cmd("/bin/ethctl", 4, 1, "remsmux", "pppoe", ifname, wandev);
			}
#endif

			// mpoactl del vc0
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			found_another = find_latest_used_vc(pEntry);
			if (!found_another) {
				// delete major vc name
				//printf("**stopConnection: delete major vc name=%s\n", ifname);
				//ifconfig vc0 0.0.0.0
				va_cmd(IFCONFIG, 2, 1, ifname, "0.0.0.0");
				// ifconfig vc0 down
				va_cmd(IFCONFIG, 2, 1, ifname, "down");
				// wait for sar to process the queueing packets
				//usleep(1000);
				for (i=0; i<10000000; i++);

				snprintf(msg.data, BUF_SIZE, "mpoactl del %s", ifname);
				TRACE(STA_SCRIPT, "%s\n", msg.data);
				write_to_mpoad(&msg);

				// make sure this vc been deleted completely
				while (find_pvc_from_running(pEntry->vpi, pEntry->vci));
			}
#else
			if (((CHANNEL_MODE_T)pEntry->cmode != CHANNEL_MODE_PPPOE) || (!underlying_pppoe_exist(pEntry)))
			{
				snprintf(msg.data, BUF_SIZE, "mpoactl del %s", wandev);
				TRACE(STA_SCRIPT, "%s\n", msg.data);
				write_to_mpoad(&msg);
					
				// make sure this vc been deleted completely
				while (find_pvc_from_running(pEntry->vpi, pEntry->vci));
			}
#endif

#ifdef CONFIG_USER_CMD_CLIENT_SIDE
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			if (VC_MINOR_INDEX(MAJOR_VC_INTF(pEntry->ifIndex)) == DUMMY_VC_MINOR_INDEX) // major vc devname
				ch_no = VC_MAJOR_INDEX(MAJOR_VC_INTF(pEntry->ifIndex));
			else{
				//ch_no = VC_MAJOR_INDEX(MAJOR_VC_INTF(pEntry->ifIndex));
			}
#else
			ch_no = VC_INDEX(PHY_INTF(pEntry->ifIndex));
#endif

			commserv_atmSetup(1, &ch_no);
#endif
		}
		#endif
#if defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN) || defined(CONFIG_RTK_DEV_AP)
		//del ethwan dev here.
		if( (MEDIA_INDEX(pEntry->ifIndex) == MEDIA_ETH)
		     #ifdef CONFIG_PTMWAN
		     || (MEDIA_INDEX(pEntry->ifIndex) == MEDIA_PTM)
		     #endif /*CONFIG_PTMWAN*/
		){
			unsigned char *rootdev;

			#ifdef CONFIG_PTMWAN
			if(MEDIA_INDEX(pEntry->ifIndex) == MEDIA_PTM)
				rootdev=ALIASNAME_PTM0;
			else
			#endif /*CONFIG_PTMWAN*/
				rootdev=ALIASNAME_NAS0;


			/* patch for unregister_device fail: unregister_netdevice: waiting for nas0_3 to become free. Usage count = 1*/
#if defined(CONFIG_IPV6) && defined(CONFIG_USER_RADVD)
			if (pEntry->IpProtocol & IPVER_IPV6) {
				int radvdpid;
				radvdpid = read_pid((char *)RADVD_PID);
				if (radvdpid > 0) {
					kill(radvdpid, SIGHUP);
				}
			}
#endif
#ifdef CONFIG_IPV6
			if (pEntry->IpProtocol & IPVER_IPV4)
#endif
			if ((DHCP_T)pEntry->ipDhcp != DHCP_DISABLED)
			{
				int dhcpc_pid = 0;
				char filename[100] = {0};

				sprintf(filename, "%s.%s", (char*)DHCPC_PID, wandev);
				dhcpc_pid = read_pid(filename);
				if (dhcpc_pid > 0)
#ifdef CONFIG_RTK_DEV_AP
					kill(dhcpc_pid, SIGTERM);
#else
					kill(dhcpc_pid, SIGHUP);
#endif
			}

#if defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN)
			printf("%s remove smux device %s\n", __func__, wandev);
			if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_BRIDGE)
			{
				va_cmd("/bin/ethctl", 4, 1, "remsmux", "bridge", rootdev, wandev);
			}
			else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_IPOE)
			{
				va_cmd("/bin/ethctl", 4, 1, "remsmux", "ipoe", rootdev, wandev);
			}
			else if ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE)
			{
				if (!underlying_pppoe_exist(pEntry))
					va_cmd("/bin/ethctl", 4, 1, "remsmux", "pppoe", rootdev, wandev);
			}
#endif
		}
#endif
#ifdef WLAN_WISP
		if(MEDIA_INDEX(pEntry->ifIndex) == MEDIA_WLAN){
			int dhcpc_pid;
			unsigned char value[32];
			snprintf(value, 32, "%s.%s", (char*)DHCPC_PID, wanif);
			dhcpc_pid = read_pid((char*)value);
			if (dhcpc_pid > 0)
				kill(dhcpc_pid, SIGTERM);
		}
#endif
	}
	#ifdef CONFIG_DEV_xDSL
	else {
		int repeat_i=0;
		//ifconfig vc0 0.0.0.0
		va_cmd(IFCONFIG, 2, 1, wanif, "0.0.0.0");
		va_cmd(IFCONFIG, 2, 1, wanif, "down");
		for (i=0; i<10000000; i++);
		while( (repeat_i<10) && (find_pvc_from_running(pEntry->vpi, pEntry->vci)) )
		{
			usleep(50000);
			repeat_i++;
		}
	}
	#endif

	// delete one NAT rule for the specific interface
	stopAddressMap(pEntry);

#ifdef CONFIG_HWNAT
	if (pEntry->napt == 1){
		if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED) {
			send_extip_to_hwnat(DEL_EXTIP_CMD, wandev, *(uint32 *)pEntry->ipAddr);
		}
		else{
			send_extip_to_hwnat(DEL_EXTIP_CMD, wandev, 0);
		}
	}
#endif

#ifdef CONFIG_RTK_L34_ENABLE
		{
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE) || defined(CONFIG_FIBER_FEATURE)
		int pon_mode=0, acl_default=0;
		if (mib_get(MIB_PON_MODE, (void *)&pon_mode) != 0)
		{
#ifdef CONFIG_RTL9602C_SERIES
			acl_default = 1;
#endif
			if ((pon_mode != GPON_MODE) || acl_default == 1)
			{
				RG_del_All_Acl_Rules();
			}
		}
#else
		/*use for 8696*/
		RG_del_All_Acl_Rules();
#endif
		}

	RG_WAN_Interface_Del(pEntry->rg_wan_idx);
	Flush_RG_static_route();
	check_RG_static_route();
#if defined(CONFIG_E8B)
	RTK_RG_ACL_Del_DHCP_WAN_IPV6(pEntry->rg_wan_idx);
#endif
#endif
#if defined (CONFIG_EPON_FEATURE) && defined(CONFIG_RTK_L34_ENABLE)
//cxy 2016-6-6: oam need to add iptv wan vid as default multicast vlan
	if(pEntry->applicationtype == X_CT_SRV_OTHER)
	{
		int pon_mode=0;
		mib_get(MIB_PON_MODE, &pon_mode);
		if(pon_mode == EPON_MODE)
		{
			char cmdstr[128];
			for(i=0; i<SW_LAN_PORT_NUM; i++)
			{
				if((pEntry->itfGroup)&(1<<i))
				{
					snprintf(cmdstr, sizeof(cmdstr), "oamcli set igmp mcvlan %d %d 0", i, pEntry->vid);
					system(cmdstr);
				}
			}
		}
	}
#endif
}

#ifdef PORT_FORWARD_ADVANCE
int config_PFWAdvance( int action_type )
{
	switch( action_type )
	{
	case ACT_START:
		startPFWAdvance();
		break;

	case ACT_RESTART:
		stopPFWAdvance();
		startPFWAdvance();
		break;

	case ACT_STOP:
		stopPFWAdvance();
		break;

	default:
		return -1;
	}
	return 0;
}
#endif

#ifdef CONFIG_USER_RTK_SYSLOG
#define SLOGDPID  "/var/run/syslogd.pid"
#define KLOGDPID  "/var/run/klogd.pid"
#define SLOGDLINK1 "/var/tmp/messages"
#define SLOGDLINK2 "/var/tmp/messages.old"

static int stopSlogD(void)
{
	int slogDid=0;
	int status=0;

	slogDid = read_pid((char*)SLOGDPID);
	if(slogDid > 0) {
		kill(slogDid, 9);
		unlink(SLOGDPID);
		unlink(SLOGDLINK1);
		unlink(SLOGDLINK2);
	}
	return 1;

}

static int stopKlogD(void)
{
	int klogDid=0;
	int status=0;

	klogDid = read_pid((char*)KLOGDPID);
	if(klogDid > 0) {
		kill(klogDid, 9);
		unlink(KLOGDPID);
	}
	return 1;

}

int stopLog(void)
{
	unsigned char vChar;
	unsigned int vInt;
	char buffer[100];

	mib_get(MIB_ADSL_DEBUG, (void *)&vChar);
	if(vChar==1){
#ifdef CONFIG_USER_BUSYBOX_KLOGD
		// Kill SlogD
		stopSlogD();

		// Kill KlogD
		stopKlogD();
#endif
	}
	else{
			// Kill SlogD
			stopSlogD();
#ifdef CONFIG_USER_BUSYBOX_KLOGD
			// Kill KlogD
			stopKlogD();
#endif
	}

	return 1;
}

int startLog(void)
{
	unsigned char vChar;
	unsigned int vInt;
	char *argv[30], loglen[8], loglevel[2];
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
	char serverip[30], serverport[6];
#ifdef CONFIG_00R0
	char remoteLevel[4];
#endif
#endif
	int idx;

	mib_get(MIB_MAXLOGLEN, &vInt);
	snprintf(loglen, sizeof(loglen), "%u", vInt);
	mib_get(MIB_SYSLOG_LOG_LEVEL, &vChar);
	snprintf(loglevel, sizeof(loglevel), "%hhu", vChar);
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
	mib_get(MIB_SYSLOG_MODE, &vChar);
#endif
	argv[1] = "-n";
	argv[2] = "-s";
	argv[3] = loglen;
	argv[4] = "-l";
	argv[5] = loglevel;
	idx = 6;
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
	/* 1: Local, 2: Remote, 3: Both */
	/* Local or Both */
	if (vChar & 1)
		argv[idx++] = "-L";
	/* Remote or Both */
	if (vChar & 2) {
		getMIB2Str(MIB_SYSLOG_SERVER_IP, serverip);
		getMIB2Str(MIB_SYSLOG_SERVER_PORT, serverport);
		snprintf(serverip, sizeof(serverip), "%s:%s", serverip, serverport);
		argv[idx++] = "-R";
		argv[idx++] = serverip;
	}
#ifdef CONFIG_00R0
	/* Remote Log Level*/
	mib_get(MIB_SYSLOG_REMOTE_LEVEL, &vChar);
	snprintf(remoteLevel, sizeof(remoteLevel), "%hhu", vChar);
	argv[idx++] = "-r";
	argv[idx++] = remoteLevel;
#endif
#endif
	argv[idx] = NULL;

	mib_get(MIB_ADSL_DEBUG, &vChar);
	if(vChar==1){
#ifdef CONFIG_USER_BUSYBOX_KLOGD
		TRACE(STA_SCRIPT, "/bin/slogd %s %s %s %s %s ...\n", argv[1], argv[2], argv[3], argv[4], argv[5]);
		printf("/bin/slogd %s %s %s %s %s ...\n", argv[1], argv[2], argv[3], argv[4], argv[5]);
		va_cmd("/bin/slogd", argv, 0);
		va_cmd("/bin/klogd",1,0,"-n");
		va_cmd("/bin/adslctrl",2,0,"debug","10");
#endif
	}
	else{
		mib_get(MIB_SYSLOG, (void *)&vChar);
		if(vChar==1){
			TRACE(STA_SCRIPT, "/bin/slogd %s %s %s %s %s ...\n", argv[1], argv[2], argv[3], argv[4], argv[5]);
			printf("/bin/slogd %s %s %s %s %s ...\n", argv[1], argv[2], argv[3], argv[4], argv[5]);
			do_cmd("/bin/slogd", argv, 0);
#ifdef CONFIG_USER_BUSYBOX_KLOGD
			va_cmd("/bin/klogd",1,0,"-n");
#endif
		}
	}

	return 1;
}
#endif

#ifdef DEFAULT_GATEWAY_V2
int ifExistedDGW(void)
{
	char buff[256];
	int flgs;
	struct in_addr dest, mask;
	FILE *fp;
	if (!(fp = fopen("/proc/net/route", "r"))) {
		printf("Error: cannot open /proc/net/route - continuing...\n");
		return -1;
	}
	fgets(buff, sizeof(buff), fp);
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		if (sscanf(buff, "%*s%x%*x%x%*d%*d%*d%x", &dest, &flgs, &mask) != 3) {
			printf("Unsuported kernel route format\n");
			fclose(fp);
			return -1;
		}
		if ((flgs & RTF_UP) && dest.s_addr == 0 && mask.s_addr == 0) {
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}
#endif

// Mason Yu. MLD Proxy
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_ECMH
// MLD proxy configuration
// return value:
// 1  : successful
// 0  : function not enable
// -1 : startup failed
int startMLDproxy()
{
	unsigned char mldproxyEnable=0;
	unsigned int mldproxyItf;
	char ifname[IFNAMSIZ];
	int mldproxy_pid;
	MIB_CE_ATM_VC_T Entry;
	unsigned int entryNum, i;
#ifdef CONFIG_MLDPROXY_MULTIWAN
	char mldproxy_cmd[120]={0};
	char wanif[IFNAMSIZ];
#endif

	// Kill old IGMP proxy
	mldproxy_pid = read_pid((char *)MLDPROXY_PID);
	if (mldproxy_pid >= 1) {
		// kill it
		if (kill(mldproxy_pid, SIGTERM) != 0) {
			printf("Could not kill pid '%d'", mldproxy_pid);
		}
	}

#if defined(CONFIG_RTK_L34_ENABLE)
//#if defined(CONFIG_MCAST_VLAN)
	RTK_RG_ACL_Flush_mVlan();
	RTK_RG_ACL_Add_mVlan();
//#endif
#ifdef CONFIG_USER_ECMH
	RTK_RG_Flush_MLD_proxy_ACL_rule();
	RTK_RG_set_MLD_proxy_ACL_rule();
	RTK_RG_Ipv6_multicastFlow_reset();
	checkIGMPMLDProxySnooping(isIGMPSnoopingEnabled(), isMLDSnoopingEnabled(), isIgmproxyEnabled(), isMLDProxyEnabled());
#endif
#endif
#ifdef CONFIG_MLDPROXY_MULTIWAN
	sprintf(mldproxy_cmd, "%s", MLDPROXY);
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			printf("error get atm vc entry\n");
			return -1;
		}
		if(Entry.enable && Entry.enableMLD && (Entry.cmode != CHANNEL_MODE_BRIDGE))
		{
			mldproxyEnable = 1;
			ifGetName(Entry.ifIndex, wanif, sizeof(wanif));
			sprintf(mldproxy_cmd, "%s -i %s", mldproxy_cmd, wanif);
		}
	}
	sprintf(mldproxy_cmd, "%s -o %s", mldproxy_cmd, LANIF);
	if(mldproxyEnable)
	{
		system("/bin/echo 1 > /proc/br_mldproxy");
		system(mldproxy_cmd);
	}
	else
	{
		system("/bin/echo 0 > /proc/br_mldproxy");
	}
#else
	// check if MLD proxy enabled ?
	if (mib_get(MIB_MLD_PROXY_DAEMON, (void *)&mldproxyEnable) != 0)
	{
		if (mldproxyEnable != 1){
			printf("%s(%d) MLD proxy not enable\n",__func__,__LINE__);
			system("/bin/echo 0 > /proc/br_mldproxy");
			return 0;	// MLD proxy not enable
		}
	}
	if(mldproxyEnable)
	{
		printf("%s(%d) MLD proxy enable\n",__func__,__LINE__);
		system("/bin/echo 1 > /proc/br_mldproxy");
	}
	if (mib_get(MIB_MLD_PROXY_EXT_ITF, (void *)&mldproxyItf) != 0)
	{
		if (!ifGetName(mldproxyItf, ifname, sizeof(ifname)))
		{
			printf("Error: MLD proxy interface not set !\n");
			return 0;
		}
	}

	// check for bridge&IPv4 WAN should not start ecmh process
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			printf("error get atm vc entry\n");
			return -1;
		}
		if (mldproxyItf == Entry.ifIndex ) break;
	}

	if (Entry.IpProtocol == IPVER_IPV4 || Entry.cmode == CHANNEL_MODE_BRIDGE )
	{
		printf("Error: MLD proxy interface is invalid, stop the %s !\n",MLDPROXY);
	} else {
		va_cmd(MLDPROXY, 4, 0, "-i", ifname, "-o", (char *)LANIF);
	}
#endif
	return 1;
}

int isMLDProxyEnabled(void)
{
	unsigned int mldproxyItf;
	unsigned int entryNum;
	char ifname[IFNAMSIZ];
	MIB_CE_ATM_VC_T Entry;
	int i;
	unsigned char is_enabled;
	int found = 0;

#ifdef CONFIG_MLDPROXY_MULTIWAN
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			printf("error get atm vc entry\n");
			continue;
		}
		if(Entry.enable && (Entry.IpProtocol & IPVER_IPV6) && Entry.enableMLD && (Entry.cmode != CHANNEL_MODE_BRIDGE))
		{
			found = 1;
			break;
		}
	}
#else
	if(!mib_get(MIB_MLD_PROXY_DAEMON, (void *)&is_enabled))
	{
		return 0;
	}

	if (mib_get(MIB_MLD_PROXY_EXT_ITF, (void *)&mldproxyItf) != 0)
	{	
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=entryNum; i>0; i--)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i-1, (void *)&Entry))
			{
				printf("error get atm vc entry\n");
				return 0;
			}
			
			if (!(Entry.IpProtocol & IPVER_IPV6) || (Entry.cmode == CHANNEL_MODE_BRIDGE))
				continue;

			if (ifGetName(mldproxyItf, ifname, sizeof(ifname)))
			{
				if(Entry.ifIndex == mldproxyItf && is_enabled)
					found = 1;
			}
		}
	}
#endif
	return found;
}

#endif
#endif

#ifdef CONFIG_USER_IGMPPROXY
// IGMP proxy configuration
// return value:
// 1  : successful
// 0  : function not enable
// -1 : startup failed
int startIgmproxy(void)
{
	unsigned char igmpEnable;
	unsigned int igmpItf;
	char ifname[IFNAMSIZ];
	int igmpproxy_pid;

	// Kill old IGMP proxy
	igmpproxy_pid = read_pid((char *)IGMPPROXY_PID);
	if (igmpproxy_pid >= 1) {
		// kill it
		if (kill(igmpproxy_pid, SIGTERM) != 0) {
			printf("Could not kill pid '%d'", igmpproxy_pid);
		}
	}

	// check if IGMP proxy enabled ?
	if (mib_get(MIB_IGMP_PROXY, (void *)&igmpEnable) != 0)
	{
		if (igmpEnable != 1)
			return 0;	// IGMP proxy not enable
	}
	if (mib_get(MIB_IGMP_PROXY_ITF, (void *)&igmpItf) != 0)
	{
		if (!ifGetName(igmpItf, ifname, sizeof(ifname)))
		{
			printf("Error: IGMP proxy interface not set !\n");
			return 0;
		}
	}


	va_cmd(IGMPROXY, 6, 0, "-c","1","-d", (char *)LANIF,"-u",ifname);
	
#if defined(CONFIG_RTK_L34_ENABLE)
	//#if defined(CONFIG_MCAST_VLAN)
		RTK_RG_ACL_Flush_mVlan();
		RTK_RG_ACL_Add_mVlan();
	//#endif
		RTK_RG_Flush_IGMP_proxy_ACL_rule();
		RTK_RG_set_IGMP_proxy_ACL_rule();
		RTK_RG_multicastFlow_reset();
		checkIGMPMLDProxySnooping(isIGMPSnoopingEnabled(), isMLDSnoopingEnabled(), isIgmproxyEnabled(), isMLDProxyEnabled());
#endif
	return 1;
}

#ifdef CONFIG_IGMPPROXY_MULTIWAN
int setting_Igmproxy(void)
{
	int igmpproxy_pid;
	MIB_CE_ATM_VC_T Entry;
	unsigned int entryNum, i;
	char igmpproxy_wan[100];
	char igmpproxy_cmd[120];
	int igmpenable =0;
	char ifname[IFNAMSIZ];

	// Kill old IGMP proxy
	igmpproxy_pid = read_pid((char *)IGMPPROXY_PID);
	if (igmpproxy_pid >= 1) {
		// kill it
		if (kill(igmpproxy_pid, SIGTERM) != 0) {
			printf("Could not kill pid '%d'", igmpproxy_pid);
		}
	}
	igmpproxy_wan[0]='\0';

	// Mason Yu. IPTV_Intf is set to "" for sar driver
#ifdef CONFIG_IP_NF_UDP
	va_cmd("/bin/sarctl",2,1,"iptv_intf", "");
#endif
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);

	for (i=0; i<entryNum; i++)
	{
		 if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		 {
  			printf("error get atm vc entry\n");
			return -1;
		 }

		// check if IGMP proxy enabled ?
		if(Entry.enable && Entry.enableIGMP)
		{
			char iptv_intf_str[10];
			igmpenable =1;
			if (ifGetName(Entry.ifIndex, ifname, sizeof(ifname))) {
#ifdef CONFIG_IGMPPROXY_SENDER_IP_ZERO
			   if(Entry.cmode == CHANNEL_MODE_PPPOE){
				   char wanif[IFNAMSIZ];
				   char if_cmd[120];
				   ifGetName(PHY_INTF(Entry.ifIndex), wanif, sizeof(wanif));
				   snprintf(ifname,sizeof(ifname),wanif);
#ifdef CONFIG_00R0
				   sprintf(if_cmd,"ifconfig %s 169.254.129.%d netmask 255.255.255.0",ifname,i+1);
#else
				   sprintf(if_cmd,"ifconfig %s 10.0.0.%d",ifname,i+1);
#endif
				   system(if_cmd);
			   }
				va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_INSERT, "POSTROUTING","-p","2",
				"-o", ifname, "-j", "SNAT","--to-source", "0.0.0.0");
#endif

				//multiple WAN interfaces, seperated by ','
               if(igmpproxy_wan[0]=='\0')
					snprintf(igmpproxy_wan, 100, "%s",ifname);
               else
					snprintf(igmpproxy_wan, 100, "%s,%s", igmpproxy_wan, ifname);
#ifdef CONFIG_IP_NF_UDP
				va_cmd("/bin/sarctl",2,1,"iptv_intf", ifname);
#endif
			}
			else
			{
				printf("Error: IGMP proxy interface not set !\n");
				return 0;
			}
		}
        }
	if(igmpenable){
		sprintf(igmpproxy_cmd,"%s -c 1 -d br0 -u %s",(char *)IGMPROXY, igmpproxy_wan);
		system(igmpproxy_cmd);
	}

	//Kevin, tell 0412 igmp snooping that the proxy is enabled/disabled
#ifndef CONFIG_RTL8672NIC
	if(igmpenable)
		system("/bin/echo 1 > /proc/br_igmpProxy");
	else
		system("/bin/echo 0 > /proc/br_igmpProxy");
#endif

	// Mason Yu. Kill all session
	va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
#if defined(CONFIG_RTK_L34_ENABLE)
//#if defined(CONFIG_MCAST_VLAN)
	RTK_RG_ACL_Flush_mVlan();
	RTK_RG_ACL_Add_mVlan();
//#endif
	RTK_RG_Flush_IGMP_proxy_ACL_rule();
	RTK_RG_set_IGMP_proxy_ACL_rule();
	RTK_RG_multicastFlow_reset();
	checkIGMPMLDProxySnooping(isIGMPSnoopingEnabled(), isMLDSnoopingEnabled(), isIgmproxyEnabled(), isMLDProxyEnabled());
#endif

	return 1;
}
#endif
int isIgmproxyEnabled(void)
{
	MIB_CE_ATM_VC_T Entry;
	unsigned int entryNum, i;
	unsigned char igmpenable =0;
	char ifname[IFNAMSIZ];
	unsigned int igmpItf;
#ifdef CONFIG_IGMPPROXY_MULTIWAN
	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++)
	{
		 if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		 {
  			printf("error get atm vc entry\n");
			return 0;
		 }
		
		// check if IGMP proxy enabled ?
		if(Entry.enable 
#ifdef CONFIG_IPV6
			&& (Entry.IpProtocol & IPVER_IPV4) 
#endif
			&& Entry.enableIGMP && (Entry.cmode != CHANNEL_MODE_BRIDGE))
		{
			igmpenable = 1;
			break;
		}
	}
#else
	if (mib_get(MIB_IGMP_PROXY, (void *)&igmpenable) != 0)
	{
		if (igmpenable != 1)
			return 0;	// IGMP proxy not enable
	}
	if (mib_get(MIB_IGMP_PROXY_ITF, (void *)&igmpItf) != 0)
	{
		if (!ifGetName(igmpItf, ifname, sizeof(ifname)))
		{
			printf("Error: IGMP proxy interface not set !\n");
			return 0;
		}
	}
#endif
	if(igmpenable)
		return 1;

	return 0;
}

#endif // of CONFIG_USER_IGMPPROXY

#ifdef ROUTING
void addStaticRoute(void)
{
	unsigned int entryNum, i;
	MIB_CE_IP_ROUTE_T Entry;
	//struct rtentry rt;
	//struct sockaddr_in *inaddr;
	//char	ifname[17];

	/* Clean out the RTREQ structure. */
	//memset((char *) &rt, 0, sizeof(struct rtentry));
	entryNum = mib_chain_total(MIB_IP_ROUTE_TBL);

	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_IP_ROUTE_TBL, i, (void *)&Entry))
		{
			return;
		}
#if !defined(CONFIG_LUNA) && !defined(CONFIG_RTK_L34_ENABLE)
		route_cfg_modify(&Entry, 0, i);
#endif
	}
}

void deleteStaticRoute(void)
{
	unsigned int entryNum, i;
	MIB_CE_IP_ROUTE_T Entry;
	//struct rtentry rt;
	//struct sockaddr_in *inaddr;
	//char	ifname[17];

	/* Clean out the RTREQ structure. */
	//memset((char *) &rt, 0, sizeof(struct rtentry));
	entryNum = mib_chain_total(MIB_IP_ROUTE_TBL);

	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_IP_ROUTE_TBL, i, (void *)&Entry))
		{
			return;
		}
		route_cfg_modify(&Entry, 1, i);
	}
}
#endif

#ifdef CONFIG_IPV6
static void addStaticV6Route()
{
	unsigned int entryNum, i;
	MIB_CE_IPV6_ROUTE_T Entry;

	entryNum = mib_chain_total(MIB_IPV6_ROUTE_TBL);

	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_IPV6_ROUTE_TBL, i, (void *)&Entry))
			return;
		route_v6_cfg_modify(&Entry, 0);
	}
}

static void deleteV6StaticRoute()
{
	unsigned int entryNum, i;
	MIB_CE_IPV6_ROUTE_T Entry;
	entryNum = mib_chain_total(MIB_IPV6_ROUTE_TBL);

	for (i=0; i<entryNum; i++)
	{
		if (!mib_chain_get(MIB_IPV6_ROUTE_TBL, i, (void *)&Entry))
			return;
		route_v6_cfg_modify(&Entry, 1);
	}
}
#endif

#ifdef CONFIG_USER_PPPOMODEM
static void _wan3g_start_each( MIB_WAN_3G_T *p, unsigned char pppidx)
{
    //printf( "%s: enter\n", __FUNCTION__ );
    //DEBUGMODE(STA_INFO|STA_SCRIPT|STA_WARNING|STA_ERR);

    if(p && p->enable)
    {
	struct data_to_pass_st msg;
	int pppdbg;
	char ifIdx[3], pppif[6];

	snprintf(ifIdx, 3, "%u", pppidx);
	snprintf(pppif, 6, "ppp%u", pppidx);

	if(p->ctype!=MANUAL)
		snprintf(msg.data, BUF_SIZE, "spppctl add %s", ifIdx);
	else
		snprintf(msg.data, BUF_SIZE, "spppctl new %s", ifIdx);

	//set device
	//snprintf(msg.data, BUF_SIZE, "%s pppomodem /dev/ttyUSB0", msg.data);
	snprintf(msg.data, BUF_SIZE, "%s pppomodem auto", msg.data);

	//set pin code
	if( p->pin!=NO_PINCODE )
		snprintf(msg.data, BUF_SIZE, "%s simpin %04u", msg.data, p->pin);

	//set apn
	if( strlen(p->apn) )
		snprintf(msg.data, BUF_SIZE, "%s apn %s", msg.data, p->apn);

	//set dial
	snprintf(msg.data, BUF_SIZE, "%s dial %s", msg.data, p->dial);

	// Set Authentication Method
	//printf( "p->auth=%d, %d, %d\n", p->auth, PPP_AUTH_NONE, p->auth==PPP_AUTH_NONE );
	if(p->auth==PPP_AUTH_NONE)
	{
		//skip or???
	}else{
		snprintf(msg.data, BUF_SIZE, "%s auth %s", msg.data, ppp_auth[p->auth]);
		snprintf(msg.data, BUF_SIZE, "%s username %s password %s", msg.data, p->username, p->password);
	}

	//set default gateway/ mtu
	snprintf(msg.data, BUF_SIZE, "%s gw %d mru %u",	msg.data, p->dgw, p->mtu);

	// set PPP debug
	pppdbg = pppdbg_get(pppidx);
	snprintf(msg.data, BUF_SIZE, "%s debug %d", msg.data, pppdbg);
	//snprintf(msg.data, BUF_SIZE, "%s debug 1", msg.data);

	//paula, set 3g backup PPP
	snprintf(msg.data, BUF_SIZE, "%s backup %d", msg.data, p->backup);

	if(p->backup)
		snprintf(msg.data, BUF_SIZE, "%s backup_timer %d", msg.data, p->backup_timer);

	if(p->ctype == CONTINUOUS) // Continuous
	{
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		//printf("\ncmd=%s\n",msg.data);
		write_to_pppd(&msg);

		// set the ppp keepalive timeout
		snprintf(msg.data, BUF_SIZE,"spppctl katimer 100");
		TRACE(STA_SCRIPT, "%s\n", msg.data);

		write_to_pppd(&msg);
		printf("PPP Connection (Continuous)...\n");
	}else if(p->ctype==CONNECT_ON_DEMAND) // On-demand
	{
		snprintf(msg.data, BUF_SIZE, "%s timeout %u", msg.data, p->idletime);
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
		printf("PPP Connection (On-demand)...\n");
	}
	else if(p->ctype==MANUAL) // Manual
	{
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
		printf("PPP Connection (Manual)...\n");
	}

	if(p->napt)
	{	// Enable NAPT
		va_cmd(IPTABLES, 8, 1, "-t", "nat", FW_ADD, "POSTROUTING",
			"-o", pppif, "-j", "MASQUERADE");
	}

    }

    return;
}

static void _wan3g_stop_each( MIB_WAN_3G_T *p, unsigned char pppidx)
{
    //printf( "%s: enter\n", __FUNCTION__ );
    //DEBUGMODE(STA_INFO|STA_SCRIPT|STA_WARNING|STA_ERR);

    if(p && p->enable)
    {
	struct data_to_pass_st msg;
    	char pppif[6];

	snprintf(pppif, 6, "ppp%u", pppidx);
	// spppctl del 0
	snprintf(msg.data, BUF_SIZE, "spppctl del %u", pppidx);
	TRACE(STA_SCRIPT, "%s\n", msg.data);
	write_to_pppd(&msg);

	//down interface
	va_cmd(IFCONFIG, 2, 1, pppif, "down");

	if(p->napt==1)
	{	// Disable NAPT
		va_cmd(IPTABLES, 8, 1, "-t", "nat", FW_DEL, "POSTROUTING",
			"-o", pppif, "-j", "MASQUERADE");
	}
    }

    return;
}

static MIB_WAN_3G_T mib_wan_3g_default_table[] = {
	{
	 /*enable */ 0,
	 /*auth */ PPP_AUTH_NONE,
	 /*ctype */ CONTINUOUS,
	 /*napt */ 1,
	 /*pin */ NO_PINCODE,
	 /*idletime */ 60,
	 /*mtu */ 1500,
	 /*dgw */ 1,
	 /*apn */ "internet",
	 /*dial */ "*99#",
	 /*username */ "",
	 /*password */ "",
	 //paula, 3g backup PPP
	 /*backup */ 0,
	 /*backup timer */ 60
	 }
};

/*
 *	Return value:
 *		0: disabled
 *		1: enabled
 */

int wan3g_start(void)
{
	unsigned char pppidx;
	MIB_WAN_3G_T entry, *p = &entry;
	int ret=0;

	pppidx = 0;
	if (!mib_chain_get(MIB_WAN_3G_TBL, pppidx, (void *)p)) {
		//printf("No entry in MIB_WAN_3G, add one\n");
		p = &mib_wan_3g_default_table[0];
		ret = mib_chain_add(MIB_WAN_3G_TBL, (void *)p);
		if(ret == 0){
			printf("Error! MIB_WAN_3G_TBL Add chain record.\n");
		}else if(ret == -1){
			printf("Error! MIB_WAN_3G_TBL Table Full.\n");
		}			
	}

	_wan3g_start_each(p, MODEM_PPPIDX_FROM + pppidx);

	if (p->enable) {
		//setup_ipforwarding(1);
		return 1;
	}

	return 0;
}

/*
 *	Return value:
 *		0: disabled
 *		1: enabled
 */
int wan3g_enable(void)
{
	unsigned char pppidx;
	MIB_WAN_3G_T entry, *p = &entry;
	int ret=0;

	pppidx = 0;
	if (!mib_chain_get(MIB_WAN_3G_TBL, pppidx, (void *)p)) {
		//printf("No entry in MIB_WAN_3G, add one\n");
		p = &mib_wan_3g_default_table[0];
		ret = mib_chain_add(MIB_WAN_3G_TBL, (void *)p);
		ret = mib_chain_add(MIB_WAN_3G_TBL, mib_wan_3g_default_table);
		if(ret == 0){
			printf("Error! MIB_WAN_3G_TBL Add chain record.\n");
		}else if(ret == -1){
			printf("Error! MIB_WAN_3G_TBL Table Full.\n");
		}		
	}

	if (p->enable) {
		//setup_ipforwarding(1);
		return 1;
	}

	return 0;
}

void wan3g_stop(void)
{
	unsigned char pppidx;
	MIB_WAN_3G_T entry, *p = &entry;
	int ret=0;
	pppidx = 0;
	if (!mib_chain_get(MIB_WAN_3G_TBL, pppidx, p)) {
		ret = mib_chain_add(MIB_WAN_3G_TBL, mib_wan_3g_default_table);
		if(ret == 0){
			printf("Error! MIB_WAN_3G_TBL Add chain record.\n");
		}else if(ret == -1){
			printf("Error! MIB_WAN_3G_TBL Table Full.\n");
		}
	}

	_wan3g_stop_each(p, MODEM_PPPIDX_FROM + pppidx);

	return;
}
#endif //CONFIG_USER_PPPOMODEM

// Added by Mason Yu
// configAll = CONFIGALL,  pEntry = NULL  : delete all WAN connections(include VC, ETHWAN, PTMWAN, VPN, 3g).
//									  It means that we want to restart all WAN channel.
// configAll = CONFIGONE, pEntry != NULL : delete specified VC, ETHWAN, PTMWAN connection and VPN, 3g connections.
// 									  It means that we delete or modify an old VC, ETHWAN, PTMWAN channel.
// configAll = CONFIGONE, pEntry = NULL  : delete VPN, 3g connections. It means that we add a new VC, ETHWAN, PTMWAN channel.
int deleteConnection(int configAll, MIB_CE_ATM_VC_Tp pEntry)
{
	unsigned int entryNum, i, idx;
	MIB_CE_ATM_VC_T Entry;

	//close all tunnel before delete wan interface.
#ifdef CONFIG_USER_PPTP_CLIENT_PPTP
	MIB_PPTP_T pptp;

	entryNum = mib_chain_total(MIB_PPTP_TBL);
	for (i=0; i<entryNum; i++)
	{
		if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&pptp) )
			return;

		if(configAll == CONFIGALL)
		{
			applyPPtP(&pptp, 0, i);
		}
		else if(configAll == CONFIGONE)
		{
			 if(pEntry && pptp.ifIndex == pEntry->ifIndex)
			 {
				applyPPtP(&pptp, 0, i);
				break;
			 }
		}
	}
#endif
#ifdef CONFIG_USER_L2TPD_L2TPD
	MIB_L2TP_T l2tp;
	entryNum = mib_chain_total(MIB_L2TP_TBL);
	for (i=0; i<entryNum; i++)
	{
		if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&l2tp) )
			return;

		if(configAll == CONFIGALL)
		{
			applyL2TP(&l2tp, 0, i);
		}
		else if(configAll == CONFIGONE)
		{
			 if(pEntry && l2tp.ifIndex == pEntry->ifIndex)
			 {
				applyL2TP(&l2tp, 0, i);
				break;
			 }
		}
	}
#endif

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < entryNum; i++) {
		/* Retrieve entry */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
			printf("deleteConnection: cannot get ATM_VC_TBL(ch=%d) entry\n", i);
			return 0;
		}

		if (configAll == CONFIGALL)
		{
			if(Entry.enable) {
				/* remove connection on driver*/
				stopConnection(&Entry);
			}
		}
	}

	if (configAll == CONFIGONE) {
		if (pEntry) {
			if (pEntry->enable) {
				/* remove connection on driver*/
				stopConnection(pEntry);
			}
		}
	}

#ifdef CONFIG_USER_PPPOMODEM
	if((configAll == CONFIGALL) || (pEntry && (TO_IFINDEX(MEDIA_3G,  MODEM_PPPIDX_FROM, 0) == pEntry->ifIndex)))
	{
		wan3g_stop();
	}
#endif //CONFIG_USER_PPPOMODEM
}

void cleanAllFirewallRule(void)
{
	// Added by Mason Yu. Clean all Firewall rules.
	va_cmd(IPTABLES, 1, 1, "-F");
	// set INPUT policy to ACCEPT to avoid input packet drop
	va_cmd(IPTABLES, 3, 1, "-P", "INPUT", "ACCEPT");
	va_cmd(EBTABLES, 1, 1, "-F");
	va_cmd(IPTABLES, 3, 1, "-t", "nat", "-F");
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 1, 1, "-F");
#endif
#ifdef IP_ACL
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_ACL);
	// delete chain(aclblock) on nat
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)FW_ACL);
	// delete chain(aclblock) on nat
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)FW_ACL);
	// iptables -t mangle -D PREROUTING -j aclblock
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", (char *)FW_ACL);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_ACL);
#endif
#endif
#ifdef _CWMP_MIB_
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_CWMP_ACL);	
	// delete chain(cwmp_aclblock) on nat
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)FW_CWMP_ACL);
	// iptables -t mangle -D PREROUTING -j cwmp_aclblock
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", (char *)FW_CWMP_ACL);
#endif
#if defined(NAT_CONN_LIMIT) || defined(TCP_UDP_CONN_LIMIT)
	va_cmd(IPTABLES, 2, 1, "-X", "connlimit");
#endif

	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_BR_WAN);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_BR_WAN_OUT);

#ifdef PPPOE_PASSTHROUGH
	va_cmd(EBTABLES, 6, 1, "-t", "broute", "-D", "BROUTING", "-j", (char *)FW_BR_PPPOE);
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X", (char *)FW_BR_PPPOE);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_BR_PPPOE);
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
	va_cmd(IPTABLES, 2, 1, "-X", "domainblk");
#endif
#ifdef PORT_FORWARD_GENERAL
	va_cmd(IPTABLES, 2, 1, "-X", (char *)PORT_FW);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)PORT_FW);
#ifdef NAT_LOOPBACK
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)PORT_FW_PRE_NAT_LB);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)PORT_FW_POST_NAT_LB);
#endif
#endif

#ifdef DMZ
	va_cmd(IPTABLES, 2, 1, "-X", (char *)IPTABLE_DMZ);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)IPTABLE_DMZ);
#endif

#ifdef DMZ
#ifdef NAT_LOOPBACK
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)IPTABLE_DMZ_PRE_NAT_LB);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)IPTABLE_DMZ_POST_NAT_LB);
#endif
#endif

#ifdef NATIP_FORWARDING
	va_cmd(IPTABLES, 2, 1, "-X", (char *)IPTABLE_IPFW);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)IPTABLE_IPFW);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)IPTABLE_IPFW2);
#endif

#ifdef PORT_FORWARD_ADVANCE
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_PPTP);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)FW_PPTP);
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_L2TP);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)FW_L2TP);
#endif

#ifdef PORT_TRIGGERING
	va_cmd(IPTABLES, 2, 1, "-X", (char *)IPTABLES_PORTTRIGGER);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)IPTABLES_PORTTRIGGER);
#endif

#ifdef REMOTE_ACCESS_CTL
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_INACC);
#endif

#ifdef CONFIG_USER_PORTMAPPING
#ifdef CONFIG_USER_DHCP_SERVER
	// iptables -X portmapping_dhcp
	va_cmd(IPTABLES, 2, 1, "-X", (char *)PORTMAP_IPTBL);
#endif
#ifdef CONFIG_USER_IGMPPROXY
	// ebtables -X portmapping_igmp
	va_cmd(EBTABLES, 2, 1, "-X", (char *)PORTMAP_IGMP);
#endif
#endif // of CONFIG_USER_PORTMAPPING

	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_DHCP_PORT_FILTER);
#ifdef IP_PORT_FILTER
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_IPFILTER);
#ifdef CONFIG_00R0
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_IPFILTER_LOCAL);
#endif
#endif
#ifdef PARENTAL_CTRL
	va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_PARENTAL_CTRL);
#endif

#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-X", (char *)FW_IPV6FILTER);
#endif

#ifdef MAC_FILTER
	//va_cmd(IPTABLES, 2, 1, "-X", (char *)FW_MACFILTER);
	va_cmd(EBTABLES, 4, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-j", (char *)FW_MACFILTER);
	va_cmd(EBTABLES, 2, 1, "-X", (char *)FW_MACFILTER);
#ifdef CONFIG_00R0
	va_cmd(IPTABLES, 2, 1, "-X", "macfilter");
#endif
#ifdef CONFIG_RTK_L34_ENABLE
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_DEL, (char *)FW_PREROUTING, "-j", (char *)FW_MACFILTER);
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", (char *)FW_MACFILTER);
#endif
#endif

#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-X", "WebRedirect_Wizard");
	unlink("/tmp/WebRedirectChain");
#endif

#ifdef URL_BLOCKING_SUPPORT
	va_cmd(IPTABLES, 2, 1, "-X", "urlblock");
#endif

#ifdef URL_ALLOWING_SUPPORT
	va_cmd(IPTABLES, 2, 1, "-X", "urlallow");
#endif

#ifdef WEB_REDIRECT_BY_MAC
	va_cmd(IPTABLES, 2, 1, "-t", "nat", "-X", "WebRedirectByMAC");
#endif

#if 0
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	va_cmd(IPTABLES, 2, 1, "-t", "nat", "-X", "CaptivePortal");
#endif
#endif

#if defined(CONFIG_USER_CWMP_TR069) && !defined(CONFIG_00R0)
	va_cmd(IPTABLES, 2, 1, "-X", IPTABLE_TR069);
#endif
#ifdef REMOTE_ACCESS_CTL
	filter_set_remote_access(0);
#endif

#ifdef CONFIG_USER_FON
	va_cmd(IPTABLES, 2, 1, "-X", "fongw");
	va_cmd(IPTABLES, 2, 1, "-X", "fongw_fwd");
#endif
}

#ifdef IP_PORT_FILTER
static int setupIPFilter()
{
	char *argv[20];
	unsigned char value[32], byte;
	int vInt, i, total;
	MIB_CE_IP_PORT_FILTER_T IpEntry;
	char *policy, *filterSIP, *filterDIP, srcPortRange[12], dstPortRange[12];
	char  srcip[20], dstip[20];

	// Delete ipfilter rule
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_IPFILTER);
#ifdef CONFIG_00R0 //make user friendly, white list set rule allow,black list set rule deny
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_IPFILTER_LOCAL);
#endif

#ifdef CONFIG_RTK_L34_ENABLE
	FlushRTK_RG_ACL_Filters();
	RTK_RG_ACL_IPPort_Filter_Allow_LAN_to_GW();
#endif

	// packet filtering
	// ip filtering
	total = mib_chain_total(MIB_IP_PORT_FILTER_TBL);
	// Add chain for ip filtering
	// iptables -N ipfilter
	//va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_IPFILTER);

	// drop wan to wan forward packets
	// ex: (before interface route is created) dhcp offers from wan will be frowarded to pppoe wan (default route)
	// iptables -A ipfilter ! -i br0 ! -o br0 -j DROP
	va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_IPFILTER, "!", ARG_I, "br+"
		,"!", ARG_O, "br+", "-j", (char *)FW_DROP);
	// accept related
	// iptables -A ipfilter -m state --state ESTABLISHED,RELATED -j RETURN
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_IPFILTER, "-m", "state",
		"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_RETURN);
	// iptables -A ipfilter -d 224.0.0.0/4 -j RETURN
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_IPFILTER, "-d",
		"224.0.0.0/4", "-j", (char *)FW_RETURN);

#ifdef CONFIG_00R0 //change to black list if these have no any rule
	if(total < 1) {
		byte = 1;
		mib_set(MIB_IPF_OUT_ACTION, (void *)&byte);
		return;
	}
#endif

	for (i = 0; i < total; i++)
	{
		int idx=0;
		/*
		 *	srcPortRange: src port
		 *	dstPortRange: dst port
		 */
		if (!mib_chain_get(MIB_IP_PORT_FILTER_TBL, i, (void *)&IpEntry))
			return -1;

#ifdef CONFIG_RTK_L34_ENABLE
		AddRTK_RG_ACL_IPPort_Filter(&IpEntry);
#endif

#ifdef CONFIG_00R0 //make user friendly, white list set rule allow,black list set rule deny
		mib_get(MIB_IPF_OUT_ACTION, (void *)&byte);
		if(byte)
			policy = (char *)FW_DROP;
		else
			policy = (char *)FW_RETURN;
#else
		if (IpEntry.action == 0)
			policy = (char *)FW_DROP;
		else
			policy = (char *)FW_RETURN;
#endif
		// source port
		if (IpEntry.srcPortFrom == 0)
			srcPortRange[0]='\0';
		else if (IpEntry.srcPortFrom == IpEntry.srcPortTo)
			snprintf(srcPortRange, 12, "%u", IpEntry.srcPortFrom);
		else
			snprintf(srcPortRange, 12, "%u:%u",
				IpEntry.srcPortFrom, IpEntry.srcPortTo);

		// destination port
		if (IpEntry.dstPortFrom == 0)
			dstPortRange[0]='\0';
		else if (IpEntry.dstPortFrom == IpEntry.dstPortTo)
			snprintf(dstPortRange, 12, "%u", IpEntry.dstPortFrom);
		else
			snprintf(dstPortRange, 12, "%u:%u",
				IpEntry.dstPortFrom, IpEntry.dstPortTo);

		// source ip, mask
		strncpy(srcip, inet_ntoa(*((struct in_addr *)IpEntry.srcIp)), 16);
		if (strcmp(srcip, ARG_0x4) == 0)
			filterSIP = 0;
		else {
			if (IpEntry.smaskbit!=0)
				snprintf(srcip, 20, "%s/%d", srcip, IpEntry.smaskbit);
			filterSIP = srcip;
		}

		// destination ip, mask
		strncpy(dstip, inet_ntoa(*((struct in_addr *)IpEntry.dstIp)), 16);
		if (strcmp(dstip, ARG_0x4) == 0)
			filterDIP = 0;
		else {
			if (IpEntry.dmaskbit!=0)
				snprintf(dstip, 20, "%s/%d", dstip, IpEntry.dmaskbit);
			filterDIP = dstip;
		}

		// interface
		argv[1] = (char *)FW_ADD;
		argv[2] = (char *)FW_IPFILTER;

		idx = 3;

		if (IpEntry.dir == DIR_IN)
			argv[idx++] = "!";

		argv[idx++] = (char *)ARG_I;
		argv[idx++] = (char *)LANIF;

		// protocol
		if (IpEntry.protoType != PROTO_NONE) {
			argv[idx++] = "-p";
			if (IpEntry.protoType == PROTO_TCP)
				argv[idx++] = (char *)ARG_TCP;
			else if (IpEntry.protoType == PROTO_UDP)
				argv[idx++] = (char *)ARG_UDP;
			else //if (IpEntry.protoType == PROTO_ICMP)
				argv[idx++] = (char *)ARG_ICMP;
		}

		// src ip
		if (filterSIP != 0)
		{
			argv[idx++] = "-s";
			argv[idx++] = filterSIP;
		}

		// src port
		if ((IpEntry.protoType==PROTO_TCP ||
			IpEntry.protoType==PROTO_UDP) &&
			srcPortRange[0] != 0) {
			argv[idx++] = (char *)FW_SPORT;
			argv[idx++] = srcPortRange;
		}

		// dst ip
		if (filterDIP != 0)
		{
			argv[idx++] = "-d";
			argv[idx++] = filterDIP;
		}
		// dst port
		if ((IpEntry.protoType==PROTO_TCP ||
			IpEntry.protoType==PROTO_UDP) &&
			dstPortRange[0] != 0) {
			argv[idx++] = (char *)FW_DPORT;
			argv[idx++] = dstPortRange;
		}

		// target/jump
		argv[idx++] = "-j";
		argv[idx++] = policy;
		argv[idx++] = NULL;

		//printf("idx=%d\n", idx);
		TRACE(STA_SCRIPT, "%s %s %s %s %s %s %s ...\n", IPTABLES, argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
		do_cmd(IPTABLES, argv, 1);

#ifdef CONFIG_00R0 //make user friendly, white list set rule allow,black list set rule deny
		argv[1] = (char *)FW_ADD;
		argv[2] = (char *)FW_IPFILTER_LOCAL;
		idx = 3;

		if (IpEntry.dir == DIR_IN)
			argv[idx++] = "!";

		argv[idx++] = (char *)ARG_I;
		argv[idx++] = (char *)LANIF;

		// protocol
		if(!strcmp(policy, FW_DROP)) //White list, allow TCP&UDP&ICMP access of ONT
		{
			if (IpEntry.protoType != PROTO_NONE) {
				argv[idx++] = "-p";
				if (IpEntry.protoType == PROTO_TCP)
					argv[idx++] = (char *)ARG_TCP;
				else if (IpEntry.protoType == PROTO_UDP)
					argv[idx++] = (char *)ARG_UDP;
				else //if (IpEntry.protoType == PROTO_ICMP)
					argv[idx++] = (char *)ARG_ICMP;
			}
		}

		// src ip
		if (filterSIP != 0)
		{
			argv[idx++] = "-s";
			argv[idx++] = filterSIP;
		}

		// src port
		if(!strcmp(policy, FW_DROP)) //White list, allow TCP&UDP&ICMP access of ONT
		{
			if ((IpEntry.protoType==PROTO_TCP ||
				IpEntry.protoType==PROTO_UDP) &&
				srcPortRange[0] != 0) {
				argv[idx++] = (char *)FW_SPORT;
				argv[idx++] = srcPortRange;
			}
		}

		// target/jump
		argv[idx++] = "-j";
		argv[idx++] = policy;
		argv[idx++] = NULL;

		//printf("idx=%d\n", idx);
		TRACE(STA_SCRIPT, "%s %s %s %s %s %s %s ...\n", IPTABLES, argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
		do_cmd(IPTABLES, argv, 1);
#endif
	}

#ifdef CONFIG_00R0 //make user friendly, white list set rule allow,black list set rule deny. default rule in here
	mib_get(MIB_IPF_OUT_ACTION, (void *)&byte);
	if(byte)
		policy = (char *)FW_RETURN;
	else
		policy = (char *)FW_DROP;
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_IPFILTER_LOCAL, (char *)ARG_I, (char *)LANIF, "-j", policy);
#endif
//#ifdef  CONFIG_USER_PPPOE_PROXY
#if 0
    char ppp_proxyif[6];
	char ppp_proxynum[2];
	int proxy_index;

	for(proxy_index = WAN_PPP_INTERFACE; proxy_index< WAN_PPP_INTERFACE+LAN_PPP_INTERFACE;proxy_index++){
		snprintf(ppp_proxyif, 6, "ppp%u", proxy_index);
		va_cmd (IPTABLES, 6, 1, (char *) FW_ADD, (char *) FW_IPFILTER, (char *)
           ARG_I, ppp_proxyif, "-j", (char *) FW_RETURN);
		}
#endif

	// allow DMZ to pass
	// Mason Yu
	#if 0
	if ((mib_get(MIB_DMZ_ENABLE, (void *)&byte) != 0) && byte) {
		if (mib_get(MIB_DMZ_IP, (void *)value) != 0)
		{
			char *ipaddr = srcip;
			strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
			ipaddr[15] = '\0';

			total = mib_chain_total (MIB_IP_PORT_FILTER_TBL) + 3;
			snprintf(value, 8, "%d", total);
			// iptables -I ipfilter 3 -i ! $LAN_IF -o $LAN_IF -d $DMZ_IP -j RETURN
			va_cmd(IPTABLES, 12, 1, (char *)FW_INSERT,
				(char *)FW_IPFILTER, value, (char *)ARG_I, "!",
				(char *)LANIF, (char *)ARG_O,
				(char *)LANIF, "-d", ipaddr, "-j", (char *)FW_RETURN);
		}
	}
	#endif
	// iptables -A FORWARD -j ipfilter
	//va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_IPFILTER);

	// Kill all conntrack (to kill the established conntrack when change iptables rules)
	va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
}
#endif

static int block_br_wan()
{
	int total;
	int i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ] = {0};

	// INPUT
	va_cmd(EBTABLES, 2, 1, "-N", FW_BR_WAN);
	va_cmd(EBTABLES, 3, 1, "-P", FW_BR_WAN, "RETURN");
	// OUTPUT
	va_cmd(EBTABLES, 2, 1, "-N", FW_BR_WAN_OUT);
	va_cmd(EBTABLES, 3, 1, "-P", FW_BR_WAN_OUT, "RETURN");

	// check all wan interfaces
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if(!Entry.enable)
			continue;

		ifGetName(PHY_INTF(Entry.ifIndex), ifname, sizeof(ifname));

		if(Entry.cmode == CHANNEL_MODE_BRIDGE)
		{
			va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_BR_WAN, "-i", ifname, "-j", "DROP");
			va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_BR_WAN_OUT, "-o", ifname, "-j", "DROP");
		}
	}

	// add to INPUT chain
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", FW_BR_WAN);
	// add to OUTPUT chain
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_OUTPUT, "-j", FW_BR_WAN_OUT);

	return 0;
}

#ifdef PPPOE_PASSTHROUGH
static int setupBrPppoe(void)
{
	int total;
	int i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ] = {0};

	// check all wan interfaces
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;

		if(!Entry.enable)
			continue;

		ifGetName(PHY_INTF(Entry.ifIndex), ifname, sizeof(ifname));

		if (Entry.brmode != BRIDGE_DISABLE)
		{
			struct sockaddr hwaddr;
			char mac_str[20]={0};
			getInAddr(ifname, HW_ADDR, &hwaddr);
			sprintf( mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
				(unsigned char)hwaddr.sa_data[0], (unsigned char)hwaddr.sa_data[1], (unsigned char)hwaddr.sa_data[2],
				(unsigned char)hwaddr.sa_data[3], (unsigned char)hwaddr.sa_data[4], (unsigned char)hwaddr.sa_data[5] );

			// route packets if the packet is from WAN and the destination is for myself
			//eg.: ebtables -t broute -A BROUTING -i vc0 -d 00:E0:4C:86:70:02 -j DROP
			va_cmd(EBTABLES, 10, 1, "-t", "broute", (char *)FW_ADD, FW_BR_PPPOE, "-i", ifname,
				"-d", mac_str, "-j", "DROP");
		}

		if (Entry.brmode == BRIDGE_PPPOE)
		{
			char vid[10] = {0};

#if defined(CONFIG_RTL_MULTI_WAN) || defined(CONFIG_RTL_MULTI_PVC_WAN)  || defined(CONFIG_RTL_MULTI_ETH_WAN)
			//bridge PPPoE session/discover packets on this WAN interface
			if(Entry.vlan)
			{
				sprintf(vid, "%u", Entry.vid);

				va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, FW_BR_PPPOE, "-i", ifname,
				"--proto", "0x8100", "--vlan-encap", "0x8863", "-j", "RETURN");

				va_cmd(EBTABLES, 10, 1, (char *)FW_ADD, FW_BR_PPPOE, "-i", ifname,
				"--proto", "0x8100", "--vlan-encap", "0x8864", "-j", "RETURN");
			}
			else
#endif
			{
				va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_PPPOE, "-i", ifname,
				"--proto", "0x8863", "-j", "RETURN");
				va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_PPPOE, "-i", ifname,
				"--proto", "0x8864", "-j", "RETURN");
			}

			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_PPPOE, "-o", ifname,
				"--proto", "0x8863", "-j", "RETURN");
			va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, FW_BR_PPPOE, "-o", ifname,
				"--proto", "0x8864", "-j", "RETURN");

			//drop other packets on this WAN interface
			va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_BR_PPPOE, "-i", ifname,
				"-j", "DROP");
			va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, FW_BR_PPPOE, "-o", ifname,
				"-j", "DROP");
		}
	}

	return 0;
}
#endif

#ifdef MAC_FILTER
#ifdef CONFIG_RTK_L34_ENABLE
int setupMacFilter(void)
{
	int i, total;
	MIB_CE_MAC_FILTER_T MacEntry;
	char mac_out_dft;
	char macaddr[18];
	char *policy;
#ifdef CONFIG_00R0
	unsigned char vChar;
#endif

	DOCMDINIT;

	// Delete all Macfilter rule
	FlushRTK_RG_MAC_Filters();
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_MACFILTER);
#ifdef CONFIG_00R0
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_MACFILTER);
#endif
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)FW_MACFILTER);

	mib_get(MIB_MACF_OUT_ACTION, (void *)&mac_out_dft);

	if (mac_out_dft != 0) {
		system("/bin/echo 0 > /proc/rg/whiteListState");
		policy = (char *)FW_DROP;
	} else {		
		system("/bin/echo 1 > /proc/rg/whiteListState");
		policy = (char *)FW_RETURN;
	}

	total = mib_chain_total(MIB_MAC_FILTER_TBL);

#ifdef CONFIG_00R0 //set black list if no any rules exists
	if(total < 1) {
		vChar = 1;
		mib_set( MIB_MACF_OUT_ACTION, (void *)&vChar);
		return;
	}
#endif

	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_MAC_FILTER_TBL, i, (void *)&MacEntry))
			return -1;

		AddRTK_RG_MAC_Filter(&MacEntry, mac_out_dft);

		if(memcmp(MacEntry.srcMac,"\x00\x00\x00\x00\x00\x00",MAC_ADDR_LEN)) {
			snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
				MacEntry.srcMac[0], MacEntry.srcMac[1],
				MacEntry.srcMac[2], MacEntry.srcMac[3],
				MacEntry.srcMac[4], MacEntry.srcMac[5]);
		}
		if(MacEntry.dir == 1 || MacEntry.dir == 0){
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -s %s -j %s", (char *)FW_MACFILTER, macaddr, policy);
			if(mac_out_dft == 1)
				va_cmd(IPTABLES, 10, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_MACFILTER, "-m", "mac", "--mac-source", macaddr, "-j", policy);
#ifdef CONFIG_00R0
			if(mac_out_dft == 0)
				va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_MACFILTER, "-i", (char *)BRIF, "-m", "mac", "--mac-source", macaddr, "-j", (char *)FW_RETURN);
#endif
		}
		if(MacEntry.dir == 2 || MacEntry.dir == 0){
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -d %s -j %s", (char *)FW_MACFILTER, macaddr, policy);
		}
	}

	// default action
	DOCMDARGVS(EBTABLES,DOWAIT,"-P %s RETURN", (char *)FW_MACFILTER);
	
	if(mac_out_dft == 0){
		policy = (char *)FW_DROP;
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -j %s", (char *)FW_MACFILTER, policy);
		//va_cmd(IPTABLES, 8, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_MACFILTER, (char *)ARG_I, (char *)LANIF, "-j", policy);
#ifdef CONFIG_00R0	// default value of white list , Iulian Wu
			va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_MACFILTER, "-i", (char *)BRIF, "-j", (char *)FW_DROP);
#endif
	}

	// Flush dynamic MAC entries after White list updated
	RTK_RG_Dynamic_MAC_Entry_flush();

	return 0;
}
#else
int setupMacFilter(void)
{
	int i, total;
	char *policy;
	char srcmacaddr[18], dstmacaddr[18];
	char smacParm[64]={0};
	char dmacParm[64]={0};
#ifdef MAC_FILTER
	MIB_CE_MAC_FILTER_T MacEntry;
	char mac_in_dft, mac_out_dft;
	char eth_mac_ctrl=0, wlan_mac_ctrl=0;
#endif

	DOCMDINIT;

#ifdef CONFIG_HWNAT
	int mac_filter_in_permit  = 1;
	int mac_filter_out_permit = 1;
#endif

	// Delete all Macfilter rule
	va_cmd(EBTABLES, 2, 1, "-F", (char *)FW_MACFILTER);

#ifdef MAC_FILTER
	mib_get(MIB_MACF_OUT_ACTION, (void *)&mac_out_dft);
	mib_get(MIB_MACF_IN_ACTION, (void *)&mac_in_dft);
	mib_get(MIB_ETH_MAC_CTRL, (void *)&eth_mac_ctrl);
	mib_get(MIB_WLAN_MAC_CTRL, (void *)&wlan_mac_ctrl);
#endif

#ifdef MAC_FILTER
	total = mib_chain_total(MIB_MAC_FILTER_TBL);

	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_MAC_FILTER_TBL, i, (void *)&MacEntry))
			return -1;

		smacParm[0]=dmacParm[0]='\0';

		if (MacEntry.action == 0)
			policy = (char *)FW_DROP;
		else
			policy = (char *)FW_RETURN;

		if(memcmp(MacEntry.srcMac,"\x00\x00\x00\x00\x00\x00",MAC_ADDR_LEN)) {
			snprintf(srcmacaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
				MacEntry.srcMac[0], MacEntry.srcMac[1],
				MacEntry.srcMac[2], MacEntry.srcMac[3],
				MacEntry.srcMac[4], MacEntry.srcMac[5]);
			snprintf(smacParm, sizeof(smacParm),"-s %s", srcmacaddr);
		}

		if(memcmp(MacEntry.dstMac,"\x00\x00\x00\x00\x00\x00",MAC_ADDR_LEN)) {
			snprintf(dstmacaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
				MacEntry.dstMac[0], MacEntry.dstMac[1],
				MacEntry.dstMac[2], MacEntry.dstMac[3],
				MacEntry.dstMac[4], MacEntry.dstMac[5]);
			snprintf(dmacParm, sizeof(dmacParm),"-d %s", dstmacaddr);
		}

		if (MacEntry.dir == DIR_OUT) {
			if (!strlen(dmacParm)) { // compatible with TR069 -- smac outgoing
				if(mac_out_dft) {
					// ebtables -A macfilter -i eth0+  -s 00:xx:..:xx -j ACCEPT/DROP
					DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER,
						(char *)ELANIF,smacParm,dmacParm,policy);
#ifdef WLAN_SUPPORT
					// ebtables -A macfilter -i wlan0+  -s 00:xx:..:xx -j ACCEPT/DROP
					DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER,
						(char *)WLANIF[0],smacParm,dmacParm,policy);
#endif
				}
				else {
					if (eth_mac_ctrl) {
						// ebtables -A macfilter -i eth0+  -s 00:xx:..:xx -j ACCEPT/DROP
						DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER,
							(char *)ELANIF,smacParm,dmacParm,policy);
					}
					if (wlan_mac_ctrl) {
#ifdef WLAN_SUPPORT
						// ebtables -A macfilter -i wlan0+  -s 00:xx:..:xx -j ACCEPT/DROP
						DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER,
							(char *)WLANIF[0],smacParm,dmacParm,policy);
#endif
					}
				}
			}
			else {
				// ebtables -A macfilter -i eth0+  -s 00:xx:..:xx -j ACCEPT/DROP
				DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER,
					(char *)ELANIF, smacParm,dmacParm,policy);
#ifdef WLAN_SUPPORT
				// ebtables -A macfilter -i wlan0+  -s 00:xx:..:xx -j ACCEPT/DROP
				DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER,
					(char *)WLANIF[0],smacParm,dmacParm,policy);
#endif
			}
		}
		else { // DIR_IN
#ifdef CONFIG_DEV_xDSL
			// ebtables -A macfilter -i vc+  -s 00:xx:..:xx -j ACCEPT/DROP
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER,
				ALIASNAME_VC, smacParm,dmacParm,policy);
#endif
#ifdef CONFIG_ETHWAN
			// ebtables -A macfilter -i nas0+  -s 00:xx:..:xx -j ACCEPT/DROP
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER, ALIASNAME_NAS0,
				smacParm,dmacParm,policy);
#endif
#ifdef CONFIG_PTMWAN
			// ebtables -A macfilter -i ptm0+  -s 00:xx:..:xx -j ACCEPT/DROP
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ %s %s -j %s", (char *)FW_MACFILTER, ALIASNAME_PTM0,
				smacParm,dmacParm,policy);
#endif
		}
	}

	// default action
	DOCMDARGVS(EBTABLES,DOWAIT,"-P %s RETURN", (char *)FW_MACFILTER);
	if(mac_out_dft == 0) { // DROP
#ifdef CONFIG_HWNAT
		mac_filter_out_permit = 0;
#endif
		if (eth_mac_ctrl) {
			// ebtables -A macfilter -i eth0+ -j DROP
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -j DROP", (char *)FW_MACFILTER, (char *)ELANIF);
		}
#ifdef WLAN_SUPPORT
		if (wlan_mac_ctrl) {
			// ebtables -A macfilter -i wlan0+ -j DROP
			DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -j DROP", (char *)FW_MACFILTER, (char *)WLANIF[0]);
		}
#endif
	}
	if(mac_in_dft == 0) { // DROP
#ifdef CONFIG_HWNAT
		mac_filter_in_permit = 0;
#endif
#ifdef CONFIG_DEV_xDSL
		// ebtables -A macfilter -i vc+ -j DROP
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -j DROP", (char *)FW_MACFILTER, ALIASNAME_VC);
#endif
#ifdef CONFIG_ETHWAN
		// ebtables -A macfilter -i nas0+ -j DROP
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -j DROP", (char *)FW_MACFILTER, ALIASNAME_NAS0);
#endif
#ifdef CONFIG_PTMWAN
		// ebtables -A macfilter -i ptm0+ -j DROP
		DOCMDARGVS(EBTABLES,DOWAIT,"-A %s -i %s+ -j DROP", (char *)FW_MACFILTER, ALIASNAME_PTM0);
#endif
	}

#if defined CONFIG_HWNAT && defined CONFIG_RTL8676_Static_ACL
	if( mac_filter_in_permit==1 && mac_filter_out_permit==1 )
	{
		TRACE(STA_SCRIPT,"/bin/echo all_permit> /proc/rtl865x/acl_mac_filter_mode\n");
		system("/bin/echo all_permit> /proc/rtl865x/acl_mac_filter_mode");
	}
	else if( mac_filter_in_permit==0 && mac_filter_out_permit==1 )
	{
		TRACE(STA_SCRIPT,"/bin/echo in_drop_out_permit> /proc/rtl865x/acl_mac_filter_mode\n");
		system("/bin/echo in_drop_out_permit> /proc/rtl865x/acl_mac_filter_mode");
	}
	else if( mac_filter_in_permit==1 && mac_filter_out_permit==0 )
	{
		TRACE(STA_SCRIPT,"/bin/echo in_permit_out_drop> /proc/rtl865x/acl_mac_filter_mode\n");
		system("/bin/echo in_permit_out_drop> /proc/rtl865x/acl_mac_filter_mode");
	}
	else if( mac_filter_in_permit==0 && mac_filter_out_permit==0 )
	{
		TRACE(STA_SCRIPT,"/bin/echo all_drop> /proc/rtl865x/acl_mac_filter_mode\n");
		system("/bin/echo all_drop> /proc/rtl865x/acl_mac_filter_mode");
	}
#endif

#endif // of MAC_FILTER

	//Kevin, clear bridge fastpath table
	TRACE(STA_SCRIPT,"/bin/echo 2 > /proc/fastbridge\n");
	system("/bin/echo 2 > /proc/fastbridge");
#ifdef CONFIG_HWNAT
	TRACE(STA_SCRIPT,"/bin/echo clean_L2 > /proc/rtl865x/acl_mode\n");
	system("/bin/echo clean_L2 > /proc/rtl865x/acl_mode");
#endif

	return 0;
}
#endif
#endif

#ifdef LAYER7_FILTER_SUPPORT
int setupAppFilter(void)
{
	int entryNum,i;
	LAYER7_FILTER_T Entry;

	// Delete all Appfilter rule
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_APPFILTER);
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_APP_P2PFILTER);
	va_cmd(IPTABLES, 4, 1, ARG_T, "mangle", "-F", (char *)FW_APPFILTER);

	entryNum = mib_chain_total(MIB_LAYER7_FILTER_TBL);

	for(i=0;i<entryNum;i++)
	{
		if (!mib_chain_get(MIB_LAYER7_FILTER_TBL, i, (void *)&Entry))
			return -1;

		if(!strcmp("smtp",Entry.appname)){
			//iptables -A appfilter -i br0 -p TCP --dport 25 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_APPFILTER, ARG_I, LANIF,
				"-p", ARG_TCP, FW_DPORT, "25", "-j", (char *)FW_DROP);
			continue;
		}
		else if(!strcmp("pop3",Entry.appname)){
			//iptables -A appfilter -i br0 -p TCP --dport 110 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_APPFILTER, ARG_I, LANIF,
				"-p", ARG_TCP, FW_DPORT, "110", "-j", (char *)FW_DROP);
			continue;
		}
		else if(!strcmp("bittorrent",Entry.appname)){
			//iptables -A appp2pfilter -m ipp2p --bit -j DROP
			va_cmd(IPTABLES, 7, 1, (char *)FW_ADD, (char *)FW_APP_P2PFILTER, "-m", "ipp2p",
				"--bit", "-j", (char *)FW_DROP);
			continue;
		}
		else if(!strcmp("chinagame",Entry.appname)){
			//iptables -A appfilter -i br0 -p TCP --dport 8000 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_APPFILTER, ARG_I, LANIF,
				"-p", ARG_TCP, FW_DPORT, "8000", "-j", (char *)FW_DROP);
			continue;
		}
		else if(!strcmp("gameabc",Entry.appname)){
			//iptables -A appfilter -i br0 -p TCP --dport 5100 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_APPFILTER, ARG_I, LANIF,
				"-p", ARG_TCP, FW_DPORT, "5100", "-j", (char *)FW_DROP);
			continue;
		}
		else if(!strcmp("haofang",Entry.appname)){
			//iptables -A appfilter -i br0 -p TCP --dport 1201 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_APPFILTER, ARG_I, LANIF,
				"-p", ARG_TCP, FW_DPORT, "1201", "-j", (char *)FW_DROP);
			continue;
		}
		else if(!strcmp("ourgame",Entry.appname)){
			//iptables -A appfilter -i br0 -p TCP --dport 2000 -j DROP
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_APPFILTER, ARG_I, LANIF,
				"-p", ARG_TCP, FW_DPORT, "2000", "-j", (char *)FW_DROP);
		}


		// iptables -t mangle -A appfilter -m layer7 --l7proto qq -j DROP
		va_cmd(IPTABLES, 10, 1, ARG_T, "mangle", (char *)FW_ADD, (char *)FW_APPFILTER, "-m", "layer7",
			"--l7proto", Entry.appname, "-j", (char *)FW_DROP);

	}

	return 0;
}
#endif
#ifdef PARENTAL_CTRL
//Uses Global variable for keep watching timeout
static MIB_PARENT_CTRL_T parentctrltable[MAX_PARENTCTRL_USER_NUM] = {0};
/********************************
 *
 *	Initialization. load from flash
 *
 ********************************/
//return if this mac should be filtered now!
static int parent_ctrl_check(MIB_PARENT_CTRL_T *entry)
{
	time_t tm;
	struct tm the_tm;
	int		tmp1,tmp2,tmp3;;

	time(&tm);
	memcpy(&the_tm, localtime(&tm), sizeof(the_tm));

	if (((entry->controlled_day) & (1 << the_tm.tm_wday))!=0)
	{
		tmp1 = entry->start_hr * 60 +  entry->start_min;
		tmp2 = entry->end_hr * 60 +  entry->end_min;
		tmp3 = the_tm.tm_hour *60 + the_tm.tm_min;
		if ((tmp3 >= tmp1) && (tmp3 <= tmp2) )
			return 1;
	}

		return 0;

}

int parent_ctrl_table_init(void)
{
	int total,i;
	MIB_PARENT_CTRL_T	entry;

	memset(&parentctrltable[0],0, sizeof(MIB_PARENT_CTRL_T)*MAX_PARENTCTRL_USER_NUM);
	total = mib_chain_total(MIB_PARENTAL_CTRL_TBL);
	if (total >= MAX_PARENTCTRL_USER_NUM)
	{
		total = MAX_PARENTCTRL_USER_NUM -1;
		printf("ERROR, CHECK!");
	}
	for ( i=0; i<total; i++)
	{
		mib_chain_get(MIB_PARENTAL_CTRL_TBL, i, &parentctrltable[i]);
	}
}

int parent_ctrl_table_add(MIB_PARENT_CTRL_T *addedEntry)
{
	int	i;

	for (i = 0; i < MAX_PARENTCTRL_USER_NUM; i++)
	{
		if (strlen(parentctrltable[i].username) == 0)
		{
			break;
		}
	}
	addedEntry->cur_state = 0;
	memcpy (&parentctrltable[i],addedEntry, sizeof(MIB_PARENT_CTRL_T));

	parent_ctrl_table_rule_update();
}

int parent_ctrl_table_del(MIB_PARENT_CTRL_T *addedEntry)
{
	int	i;
	char macaddr[20];
	char ipRangeStr[32];
	unsigned char sipStr[16]={0};
	unsigned char eipStr[16]={0};

	for (i = 0; i < MAX_PARENTCTRL_USER_NUM; i++)
	{
		if ( !strcmp(parentctrltable[i].username, addedEntry->username))
		{
			if (parentctrltable[i].cur_state == 1)
			{
				//del the entry
				if (parentctrltable[i].specfiedPC)
				{
					snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
						parentctrltable[i].mac[0], parentctrltable[i].mac[1],
						parentctrltable[i].mac[2], parentctrltable[i].mac[3],
						parentctrltable[i].mac[4], parentctrltable[i].mac[5]);
					va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "mac",
						"--mac-source",  macaddr, "-j", "DROP");
#ifdef CONFIG_RTK_L34_ENABLE
					Del_RG_MAC_Parental_Ctrl_Rule(parentctrltable[i].rg_acl_idx);
#endif
				}
				else
				{
					strncpy(sipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].sip)), 16);
					sipStr[15] = '\0';
					strncpy(eipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].eip)), 16);
					eipStr[15] = '\0';
					sprintf(ipRangeStr, "%s-%s", sipStr, eipStr);
					va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "iprange",
						"--src-range",  ipRangeStr, "-j", "DROP");
#ifdef CONFIG_RTK_L34_ENABLE
					Del_RG_IP_Parental_Ctrl_Rule(parentctrltable[i].rg_acl_idx);
#endif
				}
			}
			//remove the iptable here if it exists
			memset(&parentctrltable[i],0, sizeof(MIB_PARENT_CTRL_T));
			break;
		}
	}
}
int parent_ctrl_table_delall()
{
	int	i;
	char macaddr[20];
	char ipRangeStr[32];
	unsigned char sipStr[16]={0};
	unsigned char eipStr[16]={0};

	for (i = 0; i < MAX_PARENTCTRL_USER_NUM; i++)
	{
		if (parentctrltable[i].cur_state == 1)
		{
			//del the entry
			if (parentctrltable[i].specfiedPC)
			{
				snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
					parentctrltable[i].mac[0], parentctrltable[i].mac[1],
					parentctrltable[i].mac[2], parentctrltable[i].mac[3],
					parentctrltable[i].mac[4], parentctrltable[i].mac[5]);
				va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
					(char *)ARG_I, (char *)LANIF, "-m", "mac",
					"--mac-source",  macaddr, "-j", "DROP");
#ifdef CONFIG_RTK_L34_ENABLE
				Del_RG_MAC_Parental_Ctrl_Rule(parentctrltable[i].rg_acl_idx);
#endif
			}
			else
			{
				strncpy(sipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].sip)), 16);
				sipStr[15] = '\0';
				strncpy(eipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].eip)), 16);
				eipStr[15] = '\0';
				sprintf(ipRangeStr, "%s-%s", sipStr, eipStr);
				va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
					(char *)ARG_I, (char *)LANIF, "-m", "iprange",
					"--src-range",  ipRangeStr, "-j", "DROP");
#ifdef CONFIG_RTK_L34_ENABLE
				Del_RG_IP_Parental_Ctrl_Rule(parentctrltable[i].rg_acl_idx);
#endif
			}
		}

	}
	memset(&parentctrltable[0],0, sizeof(MIB_PARENT_CTRL_T)*MAX_PARENTCTRL_USER_NUM);
}

// update the rules to iptables according to current time
int parent_ctrl_table_rule_update()
{
	int i, check;
	char macaddr[20];
	char ipRangeStr[32];
	unsigned char sipStr[16]={0};
	unsigned char eipStr[16]={0};
	unsigned char parentalCtrlOn;

	if (mib_get(MIB_PARENTAL_CTRL_ENABLE, (void *)&parentalCtrlOn) == 0)
		parentalCtrlOn = 0;

	if(!parentalCtrlOn)
		return 1;

	for (i = 0; i < MAX_PARENTCTRL_USER_NUM; i++)
	{
		if (strlen(parentctrltable[i].username) > 0)
		{

			check = parent_ctrl_check(&parentctrltable[i]);

			if (( check == 1) &&
				(parentctrltable[i].cur_state == 0))
			{
				parentctrltable[i].cur_state = 1;

				//add the entry
				if (parentctrltable[i].specfiedPC)
				{
					snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
					parentctrltable[i].mac[0], parentctrltable[i].mac[1],
					parentctrltable[i].mac[2], parentctrltable[i].mac[3],
					parentctrltable[i].mac[4], parentctrltable[i].mac[5]);

					//for debug
					va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "mac",
						"--mac-source",  macaddr, "-j", "DROP");

#ifdef CONFIG_RTK_L34_ENABLE
					AddRTK_RG_MAC_Parental_Ctrl_Rule(&parentctrltable[i]);
#endif
				}
				else
				{
					strncpy(sipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].sip)), 16);
					sipStr[15] = '\0';
					strncpy(eipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].eip)), 16);
					eipStr[15] = '\0';
					sprintf(ipRangeStr, "%s-%s", sipStr, eipStr);
					va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "iprange",
						"--src-range",  ipRangeStr, "-j", "DROP");

#ifdef CONFIG_RTK_L34_ENABLE
					AddRTK_RG_IP_Parental_Ctrl_Rule(&parentctrltable[i]);
#endif
				}
			}
			else if ((check == 0) &&
				(parentctrltable[i].cur_state == 1))
			{
				parentctrltable[i].cur_state = 0;
				//del the entry
				if (parentctrltable[i].specfiedPC)
				{
					snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
					parentctrltable[i].mac[0], parentctrltable[i].mac[1],
					parentctrltable[i].mac[2], parentctrltable[i].mac[3],
					parentctrltable[i].mac[4], parentctrltable[i].mac[5]);

					va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "mac",
						"--mac-source",  macaddr, "-j", "DROP");
#ifdef CONFIG_RTK_L34_ENABLE
					Del_RG_MAC_Parental_Ctrl_Rule(parentctrltable[i].rg_acl_idx);
#endif
				}
				else
				{
					strncpy(sipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].sip)), 16);
					sipStr[15] = '\0';
					strncpy(eipStr, inet_ntoa(*((struct in_addr *)parentctrltable[i].eip)), 16);
					eipStr[15] = '\0';
					sprintf(ipRangeStr, "%s-%s", sipStr, eipStr);
					va_cmd(IPTABLES, 10, 1, (char *)FW_DEL, (char *)FW_PARENTAL_CTRL,
						(char *)ARG_I, (char *)LANIF, "-m", "iprange",
						"--src-range",  ipRangeStr, "-j", "DROP");
#ifdef CONFIG_RTK_L34_ENABLE
					Del_RG_IP_Parental_Ctrl_Rule(parentctrltable[i].rg_acl_idx);
#endif
				}
			}

		}
	}
	return 1;
}


#endif

#ifdef CONFIG_USER_RTK_VOIP
#include "web_voip.h"

static void voip_add_iptable_rules(int is_dmz, char *portbuff)
{
	if (is_dmz) {
		va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_DEL,
		       (char *)IPTABLE_DMZ, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_UDP, (char *)FW_DPORT, portbuff, "-j",
		       (char *)FW_ACCEPT);
		va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_ADD,
		       (char *)IPTABLE_DMZ, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_UDP, (char *)FW_DPORT, portbuff, "-j",
		       (char *)FW_ACCEPT);
	} else {
		va_cmd(IPTABLES, 11, 1, (char *)FW_DEL, (char *)FW_INPUT, "!",
		       (char *)ARG_I, (char *)LANIF, "-p",
		       (char *)ARG_UDP, (char *)FW_DPORT, portbuff,
		       "-j", (char *)"ACCEPT");
		va_cmd(IPTABLES, 11, 1, (char *)FW_INSERT, (char *)FW_INPUT,
		       "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		       (char *)FW_DPORT, portbuff, "-j", (char *)"ACCEPT");
	}
}

/*----------------------------------------------------------------------------
 * Name:
 *      voip_setup_iptable
 * Descriptions:
 *      Creat an iptable rule to allow incoming VoIP calls.
 * return:              none
 *---------------------------------------------------------------------------*/
static void voip_setup_iptable(int is_dmz)
{
	char portbuff[16];
	int i, val;
	unsigned short sip_port[VOIP_PORTS], media_port[VOIP_PORTS], T38_port[VOIP_PORTS];
	voipCfgParam_t *pCfg;
	voipCfgPortParam_t *VoIPport;

	val = voip_flash_get(&pCfg);
	if (val != 0) {
		/* default value */
		for (i = 0; i < VOIP_PORTS; i++) {
			sip_port[i] = 5060 + i;
			media_port[i] = 9000 + i * 4;
			T38_port[i] = 9008 + i * 2;
		}
	} else {
		for (i = 0; i < VOIP_PORTS; i++) {
			VoIPport = &pCfg->ports[i];

			sip_port[i] = VoIPport->sip_port;
			media_port[i] = VoIPport->media_port;
			T38_port[i] = VoIPport->T38_port;
		}
	}

	for (i = 0; i < VOIP_PORTS; i++) {
		sprintf(portbuff, "%hu", sip_port[i]);
		voip_add_iptable_rules(is_dmz, portbuff);

		sprintf(portbuff, "%hu:%hu", media_port[i], media_port[i] + 3);
		voip_add_iptable_rules(is_dmz, portbuff);

		sprintf(portbuff, "%hu", T38_port[i]);
		voip_add_iptable_rules(is_dmz, portbuff);
	}
}

#if defined(CONFIG_00R0) && defined(CONFIG_RTK_L34_ENABLE)
void voip_set_rg_1p_tag(void)
{
	voipCfgParam_t * pCfg;
	voipCfgPortParam_t *pSip_conf;
	int i,val;
	printf("voip_set_rg_1p_tag\n");
	val=voip_flash_get( &pCfg);
	pSip_conf = &pCfg->ports[0]
		;
	if (val == 0){ //success get flash setting.
	/* wanVlanPriorityData is sip packet
		wanVlanPriorityVoice is rtp packet */

		fprintf(stderr, "voip sip 1p tag is wanVlanPriorityData %d\n", pCfg->wanVlanPriorityData);
		fprintf(stderr, "voip rtp 1p tag is wanVlanPriorityVoice %d\n", pCfg->wanVlanPriorityVoice);

		RG_add_voip_sip_1p_Qos(pSip_conf->sip_port ,pCfg->wanVlanPriorityData);
		//rtp port range. reserve 10 port for rtp usage... (should modify);
		RG_add_voip_rtp_1p_Qos(pSip_conf->media_port,pSip_conf->media_port+10,pCfg->wanVlanPriorityVoice);

	}else{
		printf("get voip flash setting fail\n");
	}
}
#endif //RG
#endif

#ifdef VIRTUAL_SERVER_SUPPORT
static int setupVtlsvr(void)
{
	int i, total;
	MIB_CE_VTL_SVR_T Entry;
	char srcPortRange[12], dstPortRange[12], dstPort[12];
	char dstip[32];
	char ipAddr[20];
	//char *act;
	char ifname[16];

	total = mib_chain_total(MIB_VIRTUAL_SVR_TBL);
	// attach the vrtsvr rules to the chain for ip filtering

	for (i = 0; i < total; i++)
	{
		/*
		 *	srcPortRange: src port
		 *	dstPortRange: dst port
		 */
		if (!mib_chain_get(MIB_VIRTUAL_SVR_TBL, i, (void *)&Entry))
			return -1;

		// destination ip(server ip)
		strncpy(dstip, inet_ntoa(*((struct in_addr *)Entry.svrIpAddr)), 16);
		snprintf(ipAddr, 20, "%s/%d", dstip, 32);


		//wan port
		if (Entry.wanStartPort == 0)
			srcPortRange[0]='\0';
		else if (Entry.wanStartPort == Entry.wanEndPort)
			snprintf(srcPortRange, 12, "%u", Entry.wanStartPort);
		else
			snprintf(srcPortRange, 12, "%u:%u",
				Entry.wanStartPort, Entry.wanEndPort);

		// server port
		if (Entry.svrStartPort == 0)
			dstPortRange[0]='\0';
		else {
			if (Entry.svrStartPort == Entry.svrEndPort) {
				snprintf(dstPortRange, 12, "%u", Entry.svrStartPort);
				snprintf(dstPort, 12, "%u", Entry.svrStartPort);
			} else {
				snprintf(dstPortRange, 12, "%u-%u",
					Entry.svrStartPort, Entry.svrEndPort);
				snprintf(dstPort, 12, "%u:%u",
					Entry.svrStartPort, Entry.svrEndPort);
			}
			snprintf(dstip, 32, "%s:%s", dstip, dstPortRange);
		}

		//act = (char *)FW_ADD;
		// interface
		strcpy(ifname, LANIF);

		//printf("idx=%d\n", idx);
		if (Entry.protoType == PROTO_TCP || Entry.protoType == PROTO_UDPTCP)
		{
			// iptables -t nat -A PREROUTING ! -i $LAN_IF -p TCP --dport dstPortRange -j DNAT --to-destination ipaddr
			va_cmd(IPTABLES, 15, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "!", (char *)ARG_I,
				(char *)ifname,	"-p", (char *)ARG_TCP, (char *)FW_DPORT, srcPortRange, "-j", "DNAT",
				"--to-destination", dstip);

#ifdef IP_PORT_FILTER
			// iptables -I ipfilter 3 ! -i $LAN_IF -o $LAN_IF -p TCP --dport dstPortRange -j RETURN
			va_cmd(IPTABLES, 16, 1, (char *)FW_INSERT, (char *)FW_IPFILTER, "3", "!", (char *)ARG_I, (char *)ifname,
				(char *)ARG_O, (char *)ifname, "-p", (char *)ARG_TCP, (char *)FW_DPORT, dstPort, "-d",
				(char *)ipAddr, "-j",(char *)FW_RETURN);
#endif
		}
		if (Entry.protoType == PROTO_UDP || Entry.protoType == PROTO_UDPTCP)
		{
			va_cmd(IPTABLES, 15, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "!", (char *)ARG_I,
				(char *)ifname,	"-p", (char *)ARG_UDP, (char *)FW_DPORT, srcPortRange, "-j", "DNAT",
				"--to-destination", dstip);

#ifdef IP_PORT_FILTER
			// iptables -I ipfilter 3 -i ! $LAN_IF -o $LAN_IF -p UDP --dport dstPortRange -j RETURN
			va_cmd(IPTABLES, 16, 1, (char *)FW_INSERT, (char *)FW_IPFILTER, "3", "!", (char *)ARG_I, (char *)ifname,
				(char *)ARG_O, (char *)LANIF, "-p", (char *)ARG_UDP, (char *)FW_DPORT, dstPort, "-d",
				(char *)ipAddr, "-j",(char *)FW_RETURN);
#endif
		}
	}

	return 1;
}
#endif

#ifdef PORT_FORWARD_GENERAL
void clear_dynamic_port_fw(int (*upnp_delete_redirection)(unsigned short eport, const char * protocol))
{
	int i, total;
	MIB_CE_PORT_FW_T port_entity;

	total = mib_chain_total(MIB_PORT_FW_TBL);

	for (i = total - 1; i >= 0; i--) {
		if (!mib_chain_get(MIB_PORT_FW_TBL, i, &port_entity))
			continue;

		if (port_entity.dynamic) {
			/* created by UPnP */
			mib_chain_delete(MIB_PORT_FW_TBL, i);

			if (upnp_delete_redirection)
				upnp_delete_redirection(port_entity.externalfromport,
					port_entity.protoType == PROTO_UDP ? "UDP" : "TCP");
		}
	}
}

// Mason Yu
int setupPortFW(void)
{
	int vInt, i, total;
	unsigned char value[32];
	MIB_CE_PORT_FW_T PfEntry;

	// Clean all rules
	// iptables -F portfw
	va_cmd(IPTABLES, 2, 1, "-F", (char *)PORT_FW);
	// iptables -t nat -F portfw
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)PORT_FW);

#ifdef NAT_LOOPBACK
	// iptables -t nat -F portfwPreNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)PORT_FW_PRE_NAT_LB);
	// iptables -t nat -F portfwPreNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)PORT_FW_POST_NAT_LB);
#endif

#ifdef CONFIG_RTK_L34_ENABLE
	FlushRTK_RG_Vertual_Server();
#endif

	// Remote access not go port-fw (IP_ACL/Remote_Access prevails port_forwarding)
	va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_ADD, (char *)PORT_FW, "!", (char *)ARG_I,
		(char *)LANIF, "-m", "mark", "--mark", RMACC_MARK, "-j", FW_RETURN);
	
	vInt = 0;
	if (mib_get(MIB_PORT_FW_ENABLE, (void *)value) != 0)
		vInt = (int)(*(unsigned char *)value);

	if (vInt == 1)
	{
		int negate=0, hasRemote=0;
		char * proto = 0;
		char intPort[32], extPort[32];

		total = mib_chain_total(MIB_PORT_FW_TBL);

		for (i = 0; i < total; i++)
		{
			if (!mib_chain_get(MIB_PORT_FW_TBL, i, (void *)&PfEntry))
				return -1;

			if (PfEntry.dynamic == 1)
				continue;

#ifdef CONFIG_RTK_L34_ENABLE
			RTK_RG_Vertual_Server_Set(&PfEntry);
#endif
			portfw_modify( &PfEntry, 0 );
		}
	}//if (vInt == 1)

	return 1;
}
#endif

#ifdef PORT_FORWARD_ADVANCE
static int setupPFWAdvance(void)
{
	// iptables -N pptp
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_PPTP);
	// iptables -A FORWARD -j pptp
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_PPTP);
	// iptables -t nat -N pptp
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)FW_PPTP);
	// iptables -t nat -A PREROUTING -j pptp
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)FW_PPTP);


	// iptables -N l2tp
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_L2TP);
	// iptables -A FORWARD -j l2tp
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_L2TP);
	// iptables -t nat -N l2tp
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)FW_L2TP);
	// iptables -t nat -A PREROUTING -j l2tp
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)FW_L2TP);

	config_PFWAdvance(ACT_START);

	return 0;
}

static int stopPFWAdvance(void)
{
	unsigned int entryNum, i;
	MIB_CE_PORT_FW_ADVANCE_T Entry;
	int pptp_enable=1;
	int l2tp_enable=1;

	entryNum = mib_chain_total(MIB_PFW_ADVANCE_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_PFW_ADVANCE_TBL, i, (void *)&Entry))
		{
  			printf("stopPFWAdvance: Get chain record error!\n");
			return 1;
		}

		if ( strcmp("PPTP", PFW_Rule[(PFW_RULE_T)Entry.rule]) == 0 && pptp_enable == 1) {
			// iptables -F pptp
			va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_PPTP);
			// iptables -t nat -F pptp
			va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)FW_PPTP);
			pptp_enable = 0;
		}

		if ( strcmp("L2TP", PFW_Rule[(PFW_RULE_T)Entry.rule]) == 0 && l2tp_enable == 1) {
			// iptables -F l2tp
			va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_L2TP);
			// iptables -t nat -F l2tp
			va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)FW_L2TP);
			l2tp_enable = 0;
		}
	}
	return 0;
}

static int startPFWAdvance(void)
{
	unsigned int entryNum, i;
	MIB_CE_PORT_FW_ADVANCE_T Entry;
	char interface_name[IFNAMSIZ], lanIP[16], ip_port[32];
	struct in_addr dest;
	int pf_enable;
	unsigned char value[32];
	int pptp_enable=0;
	int l2tp_enable=0;

	entryNum = mib_chain_total(MIB_PFW_ADVANCE_TBL);


	pf_enable = 0;
	if (mib_get(MIB_PORT_FW_ENABLE, (void *)value) != 0)
		pf_enable = (int)(*(unsigned char *)value);

	if ( pf_enable != 1 ) {
		printf("Port Forwarding is disable and stop to setup Port Forwarding Advance!\n");
		return 1;
	}

	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_PFW_ADVANCE_TBL, i, (void *)&Entry))
		{
  			printf("startPFWAdvance: Get chain record error!\n");
			return 1;
		}

		if ( strcmp("PPTP", PFW_Rule[(PFW_RULE_T)Entry.rule]) == 0 && pptp_enable == 0) {
			// LAN IP Address
			dest.s_addr = *(unsigned long *)Entry.ipAddr;
			// inet_ntoa is not reentrant, we have to
			// copy the static memory before reuse it
			strcpy(lanIP, inet_ntoa(dest));
			lanIP[15] = '\0';
			sprintf(ip_port,"%s:%d",lanIP, 1723);

			// interface

			// iptables -A pptp -p tcp --destination-port 1723 --dst $LANIP -j ACCEPT
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_PPTP, "-p", (char *)ARG_TCP, "--destination-port", "1723", "--dst", lanIP, "-j", (char *)FW_ACCEPT);

			// iptables -A pptp -p 47 --dst $LANIP -j ACCEPT
			va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_PPTP, "-p", "47", "--dst", lanIP, "-j", (char *)FW_ACCEPT);


			if (ifGetName(Entry.ifIndex, interface_name, sizeof(interface_name))) {
				// iptables -t nat -A pptp -i ppp0 -p tcp --dport 1723 -j DNAT --to-destination $LANIP:1723
				va_cmd(IPTABLES, 14, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PPTP, "-i", interface_name, "-p", (char *)ARG_TCP, "--dport", "1723", "-j", "DNAT", "--to-destination", ip_port);

				// iptables -t nat -A pptp -i ppp0 -p 47 -j DNAT --to-destination $LANIP
				va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PPTP, "-i", interface_name, "-p", "47", "-j", "DNAT", "--to-destination", lanIP);
			} else {
				// iptables -t nat -A pptp -p tcp --dport 1723 -j DNAT --to-destination $LANIP:1723
				va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PPTP, "-p", (char *)ARG_TCP, "--dport", "1723", "-j", "DNAT", "--to-destination", ip_port);

				// iptables -t nat -A pptp -p 47 -j DNAT --to-destination $LANIP
				va_cmd(IPTABLES, 10, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PPTP, "-p", "47", "-j", "DNAT", "--to-destination", lanIP);
			}
			pptp_enable = 1;
		}

		if ( strcmp("L2TP", PFW_Rule[(PFW_RULE_T)Entry.rule]) == 0 && l2tp_enable == 0) {
			// LAN IP Address
			dest.s_addr = *(unsigned long *)Entry.ipAddr;
			// inet_ntoa is not reentrant, we have to
			// copy the static memory before reuse it
			strcpy(lanIP, inet_ntoa(dest));
			lanIP[15] = '\0';
			sprintf(ip_port,"%s:%d",lanIP, 1701);

			// interface

			// iptables -A l2tp -p udp --destination-port 1701 --dst $LANIP -j ACCEPT
			va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_L2TP, "-p", (char *)ARG_UDP, "--destination-port", "1701", "--dst", lanIP, "-j", (char *)FW_ACCEPT);

			// iptables -t nat -A l2tp -i ppp0 -p udp --dport 1701 -j DNAT --to-destination $LANIP:1701
			if (!ifGetName(Entry.ifIndex, interface_name, sizeof(interface_name))) {
				va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_L2TP, "-p", (char *)ARG_UDP, "--dport", "1701", "-j", "DNAT", "--to-destination", ip_port);
			} else {
				va_cmd(IPTABLES, 14, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_L2TP, "-i", interface_name, "-p", (char *)ARG_UDP, "--dport", "1701", "-j", "DNAT", "--to-destination", ip_port);
			}
			l2tp_enable = 1;
		}
	}
	return 0;
}
#endif

#ifdef DMZ
static void clearDMZ(void)
{
	// iptables -F dmz
	va_cmd(IPTABLES, 2, 1, "-F", (char *)IPTABLE_DMZ);
	// iptables -t nat -F dmz
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)IPTABLE_DMZ);
#ifdef NAT_LOOPBACK
	// iptables -t nat -N dmzPreNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)IPTABLE_DMZ_PRE_NAT_LB);
	// iptables -t nat -N dmzPostNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)IPTABLE_DMZ_POST_NAT_LB);
#endif
}

static void setDMZ(char *ip)
{
#ifdef CONFIG_USER_RTK_VOIP
	voip_setup_iptable(1);
#endif
	// Kaohj -- Multicast not involved
	// iptables -t nat -A dmz -i ! $LAN_IF -d 224.0.0.0/4 -j ACCEPT
	va_cmd(IPTABLES, 11, 1, "-t", "nat", (char *)FW_ADD, (char *)IPTABLE_DMZ, "!", (char *)ARG_I,
		(char *)LANIF, "-d", "224.0.0.0/4", "-j", (char *)FW_ACCEPT);
	// Remote access not go DMZ
	va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_ADD, (char *)IPTABLE_DMZ, "!", (char *)ARG_I,
		(char *)LANIF, "-m", "mark", "--mark", RMACC_MARK, "-j", FW_RETURN);
#if defined(CONFIG_VIRTUAL_WLAN_DRIVER) && defined(CONFIG_SLAVE_WLAN1_ENABLE)
	va_cmd(IPTABLES, 8, 1, "-t", "nat", (char *)FW_ADD, (char *)IPTABLE_DMZ, (char *)ARG_I,
		"vwlan", "-j", (char *)FW_ACCEPT);
#endif
	// iptables -t nat -A dmz -i ! $LAN_IF -p UDP --dport 9000:9050 -j ACCEPT
	va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_ADD, (char *)IPTABLE_DMZ, "!", (char *)ARG_I,
		(char *)LANIF, "-p", (char *)ARG_UDP, (char *)FW_DPORT, "9000:9050", "-j", (char *)FW_ACCEPT);

	// iptables -t nat -A dmz -i ! $LAN_IF -j DNAT --to-destination $DMZ_IP
	va_cmd(IPTABLES, 11, 1, "-t", "nat", (char *)FW_ADD, (char *)IPTABLE_DMZ, "!", (char *)ARG_I,
		(char *)LANIF, "-j", "DNAT", "--to-destination", ip);
	// iptables -A dmz -i ! $LAN_IF -o $LAN_IF -d $DMZ_IP -j ACCEPT
	va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)IPTABLE_DMZ, "!", (char *)ARG_I,
		(char *)LANIF, (char *)ARG_O, (char *)LANIF, "-d", ip, "-j", (char *)FW_ACCEPT);
#ifdef NAT_LOOPBACK
	iptable_dmz_natLB(0, ip);
#endif
}

// Mason Yu
int setupDMZ(void)
{
	int vInt;
	unsigned char value[32];
	char ipaddr[32];

	clearDMZ();

	vInt = 0;
	if (mib_get(MIB_DMZ_ENABLE, (void *)value) != 0)
		vInt = (int)(*(unsigned char *)value);

	if (mib_get(MIB_DMZ_IP, (void *)value) != 0)
	{
#ifdef CONFIG_RTK_L34_ENABLE
		RTK_RG_DMZ_Set(vInt, *(in_addr_t *)&value);
#endif

		if (vInt == 1)
		{
			strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
			ipaddr[15] = '\0';
			setDMZ(ipaddr);
		}
	}
}
#endif

#ifdef NATIP_FORWARDING
static void fw_setupIPForwarding(void)
{
	int i, total;
	char local[16], remote[16], enable;
	MIB_CE_IP_FW_T Entry;

	// Add New chain on filter and nat
	// iptables -N ipfw
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IPTABLE_IPFW);
	// iptables -A FORWARD -j ipfw
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)IPTABLE_IPFW);
	// iptables -t nat -N ipfw
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)IPTABLE_IPFW);
	// iptables -t nat -A PREROUTING -j ipfw
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)IPTABLE_IPFW);
	// iptables -t nat -N ipfw2
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)IPTABLE_IPFW2);
	// iptables -t nat -A POSTROUTING -j ipfw2
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", (char *)IPTABLE_IPFW2);


	mib_get(MIB_IP_FW_ENABLE, (void *)&enable);
	if (!enable)
		return;

	total = mib_chain_total(MIB_IP_FW_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_IP_FW_TBL, i, (void *)&Entry))
			continue;
		strncpy(local, inet_ntoa(*((struct in_addr *)Entry.local_ip)), 16);
		strncpy(remote, inet_ntoa(*((struct in_addr *)Entry.remote_ip)), 16);
		// iptables -t nat -A ipfw -d remoteip ! -i $LAN_IF -j DNAT --to-destination localip
		va_cmd(IPTABLES, 13, 1, "-t", "nat", (char *)FW_ADD,	(char *)IPTABLE_IPFW, "-d", remote, "!", (char *)ARG_I, (char *)LANIF, "-j",
			"DNAT", "--to-destination", local);
		// iptables -t nat -A ipfw2 -s localip -o ! br0 -j SNAT --to-source remoteip
		va_cmd(IPTABLES, 13, 1, "-t", "nat", FW_ADD, (char *)IPTABLE_IPFW2, "!", (char *)ARG_O, (char *)LANIF, "-s", local, "-j", "SNAT", "--to-source", remote);

		// iptables -A ipfw2 -d localip ! -i $LAN_IF -o $LAN_IF -j RETURN
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD,
			(char *)IPTABLE_IPFW, "-d", local, "!", (char *)ARG_I,
			(char *)LANIF, (char *)ARG_O,
			(char *)LANIF, "-j", (char *)FW_ACCEPT);
	}
}
#endif

#if defined (CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_L2TPD_LNS)
int set_LAN_VPN_accept(char *chain)
{
	FILE* fp;

	fp=fopen("/var/ppp/ppp.conf","r");	
	if(fp)
	{
		char *tmp,linestr[128];
		int pppidx=0;
		while(fgets(linestr,sizeof(linestr),fp))
		{
			if(strstr(linestr,"ppp"))
			{
				tmp=linestr+3;
				while((*tmp>='0') && (*tmp<='9'))
				{
					pppidx=pppidx*10+(*tmp-'0');
					tmp++;
				}
				*tmp='\0';
				if(pppidx>12)
					va_cmd (IPTABLES, 6,1, FW_ADD,chain,(char *)ARG_I,linestr,"-j",(char *)FW_ACCEPT);
			}
		}
		fclose(fp);
	}
}
#endif
			
#ifdef IP_PORT_FILTER
static int setup_default_IPFilter(void)
{
	// Set default action for ipfilter
	unsigned char value[32];
	int vInt=-1, in_policy=0, out_policy=1;

	if (mib_get(MIB_IPF_OUT_ACTION, (void *)value) != 0)
	{
		vInt = (int)(*(unsigned char *)value);
		out_policy = vInt;

		if (vInt == 0)	// DROP
		{
			// iptables -A ipfilter -i $LAN_IF -j DROP
			va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_IPFILTER, (char *)ARG_I, "br+", "-j", (char *)FW_DROP);
		}
	}

#ifdef CONFIG_RTK_L34_ENABLE
	RTK_RG_ACL_IPPort_Filter_Default_Policy(out_policy);
#endif

	if (mib_get(MIB_IPF_IN_ACTION, (void *)value) != 0)
	{
		vInt = (int)(*(unsigned char *)value);
		in_policy = vInt;

		if (vInt == 0)	// DROP
		{
#if defined (CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_L2TPD_LNS)
			set_LAN_VPN_accept((char *)FW_IPFILTER);
#endif
			// iptables -A ipfilter ! -i $LAN_IF -j DROP
			va_cmd(IPTABLES, 7, 1, (char *)FW_ADD, (char *)FW_IPFILTER, "!", (char *)ARG_I, "br+", "-j", (char *)FW_DROP);
		}
	}

	return 1;
}
#endif

#ifndef CONFIG_RTL8672NIC
void setupIPQoSflag(int flag)
{
	if(flag)
		system("ethctl setipqos 1");
	else
		system("ethctl setipqos 0");
}
#endif

#if defined(IP_PORT_FILTER) || defined(MAC_FILTER) || defined(DMZ)
int restart_IPFilter_DMZ_MACFilter(void)
{
#ifdef IP_PORT_FILTER
	setupIPFilter();
#endif
#if defined(IP_PORT_FILTER) && defined(DMZ)
	// iptables -A filter -j dmz
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_IPFILTER, "-j", (char *)IPTABLE_DMZ);
#endif
#ifdef IP_PORT_FILTER
	setup_default_IPFilter();
#endif
#ifdef MAC_FILTER
	setupMacFilter();
#endif
#ifdef CONFIG_IPV6
	restart_IPV6Filter();
#endif
	return 1;
}
#endif

#ifdef PORT_TRIGGERING
static void parse_and_add_triggerPort(char *inRange, PROTO_TYPE_T prot, char *ip)
{
	int parseLen, j;
	char tempStr1[10]={0},tempStr2[10]={0};
	char *p, dstPortRange[32];
	int idx=0,shift=0;

	parseLen = strlen(inRange);
	if (prot == PROTO_TCP)
		p = (char *)ARG_TCP;
	else
		p = (char *)ARG_UDP;

	for(j=0;j<GAMING_MAX_RANGE;j++)
	{
		if(((inRange[j]>='0')&&(inRange[j]<='9')))
		{
			if(shift>=9) continue;
			if(idx==0)
				tempStr1[shift++]=inRange[j];
			else
				tempStr2[shift++]=inRange[j];

		}
		else if((inRange[j]==',')||
				(inRange[j]=='-')||
				(inRange[j]==0))
		{
			if(idx==0)
				tempStr1[shift]=0;
			else
				tempStr2[shift]=0;

			shift=0;
			if((inRange[j]==',')||
				(inRange[j]==0))
			{
				/*
				uint16 inStart,inFinish;
				inStart=atoi(tempStr1);
				if(idx==0)
					inFinish=inStart;
				else
					inFinish=atoi(tempStr2);
				*/
				dstPortRange[0] = '\0';
				if (idx==0) // single port number
					strncpy(dstPortRange, tempStr1, 32);
				else {
					if (strlen(tempStr1)!=0 && strlen(tempStr2)!=0)
						snprintf(dstPortRange, 32, "%s:%s", tempStr1, tempStr2);
				}

				idx=0;

				// iptables -t nat -A PREROUTING -i ! $LAN_IF -p TCP --dport dstPortRange -j DNAT --to-destination ipaddr
				va_cmd(IPTABLES, 15, 1, "-t", "nat",
					(char *)FW_ADD,	(char *)IPTABLES_PORTTRIGGER,
					 "!", (char *)ARG_I, (char *)LANIF,
					"-p", p,
					(char *)FW_DPORT, dstPortRange, "-j",
					"DNAT", "--to-destination", ip);

				// iptables -I ipfilter 3 -i ! $LAN_IF -o $LAN_IF -p TCP --dport dstPortRange -j RETURN
				va_cmd(IPTABLES, 13, 1, (char *)FW_ADD,
					(char *)IPTABLES_PORTTRIGGER, "!", (char *)ARG_I,
					(char *)LANIF, (char *)ARG_O,
					(char *)LANIF, "-p", p,
					(char *)FW_DPORT, dstPortRange, "-j",
					(char *)FW_ACCEPT);
				/*
				//make inFinish always bigger than inStart
				if(inStart>inFinish)
				{
					uint16 temp;
					temp=inFinish;
					inFinish=inStart;
					inStart=temp;
				}

				if(!((inStart==0)||(inFinish==0)))
				{
					rtl8651a_addTriggerPortRule(
					dsid, //dsid
					inType,inStart,inFinish,
					outType,
					outStart,
					outFinish,localIp);
//					printf("inRange=%d-%d\n inType=%d\n",inStart,inFinish,inType);
				}
				*/
			}
			else
			{
				idx++;
			}
			if(inRange[j]==0)
				break;
		}
	}
}

static void setupPortTriggering(void)
{
	int i, total;
	char ipaddr[16];
	MIB_CE_PORT_TRG_T Entry;

	total = mib_chain_total(MIB_PORT_TRG_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_PORT_TRG_TBL, i, (void *)&Entry))
			continue;
		if(!Entry.enable)
			continue;

		strncpy(ipaddr, inet_ntoa(*((struct in_addr *)Entry.ip)), 16);

		parse_and_add_triggerPort(Entry.tcpRange, PROTO_TCP, ipaddr);
		parse_and_add_triggerPort(Entry.udpRange, PROTO_UDP, ipaddr);
	}
}
#endif // of PORT_TRIGGERING

/*
 *	Clamp TCPMSS in FORWARD chain
 */
static int clamp_tcpmss(MIB_CE_ATM_VC_Tp pEntry)
{
	char ifname[IFNAMSIZ];
	char mss[8], mss_s[16];

	ifGetName( pEntry->ifIndex, ifname, sizeof(ifname));
	sprintf(mss, "%d", pEntry->mtu-40);
	sprintf(mss_s, "%d:1536", pEntry->mtu-40);
	va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char *)FW_FORWARD,
		"-p", "tcp", "-o", (char *)ifname, "--tcp-flags", "SYN,RST", "SYN", "-j",
		"TCPMSS", "--clamp-mss-to-pmtu");
	va_cmd(IPTABLES, 17, 1, (char *)FW_ADD, (char *)FW_FORWARD,
		"-p", "tcp", "-i", (char *)ifname, "--tcp-flags", "SYN,RST", "SYN", "-m" , "tcpmss", "--mss", mss_s, "-j",
		"TCPMSS", "--set-mss", mss);

	return 0;
}

static int filter_set_tcpmss(void)
{
	int i, total;
	MIB_CE_ATM_VC_T Entry;
#ifdef CONFIG_USER_PPPOMODEM
	MIB_WAN_3G_T Entry_3G;
#endif
	#define NET_MTU 1500
	#define PPPoE_MTU 1492

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;
		if (Entry.enable == 0 || Entry.cmode == CHANNEL_MODE_BRIDGE)
			continue;
		if (Entry.cmode == CHANNEL_MODE_PPPOE || Entry.cmode == CHANNEL_MODE_PPPOA) {
			if (Entry.mtu < PPPoE_MTU)
				clamp_tcpmss(&Entry);
		}
		else {
			if(Entry.mtu < NET_MTU)
				clamp_tcpmss(&Entry);
		}
	}

#ifdef CONFIG_USER_PPPOMODEM
	if(mib_chain_get( MIB_WAN_3G_TBL, 0, (void*)&Entry_3G) == 1){
		if (Entry_3G.enable) {
			if(Entry_3G.mtu < PPPoE_MTU) {
				// clamp_tcpmss needs ifIndex and mtu
				Entry.ifIndex = TO_IFINDEX(MEDIA_3G,  MODEM_PPPIDX_FROM, 0);
				Entry.mtu = Entry_3G.mtu;
				clamp_tcpmss(&Entry);
			}
		}
	}
#endif
	// setup default PPPoE tcpmss (1492-40=1452)
	va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char *)FW_FORWARD,
		"-p", "tcp", "-o", "ppp+", "--tcp-flags", "SYN,RST", "SYN", "-j",
		"TCPMSS", "--clamp-mss-to-pmtu");
	va_cmd(IPTABLES, 17, 1, (char *)FW_ADD, (char *)FW_FORWARD,
		"-p", "tcp", "-i", "ppp+", "--tcp-flags", "SYN,RST", "SYN", "-m" , "tcpmss", "--mss", "1452:1536", "-j",
		"TCPMSS", "--set-mss", "1452");

	return 0;
}

// Execute firewall rules
// return value:
// 1  : successful
int setupFirewall(void)
{
	char *argv[20];
	unsigned char value[32];
	int vInt, i, total, vcNum;
	MIB_CE_ATM_VC_T Entry;
	char *policy, *filterSIP, *filterDIP, srcPortRange[12], dstPortRange[12];
	char ipaddr[32], srcip[20], dstip[20];
	char ifname[16], extra[32];
	char srcmacaddr[18], dstmacaddr[18];
#ifdef IP_PASSTHROUGH
	unsigned int ippt_itf;
#endif
	int spc_enable, spc_ip;
	// Added by Mason Yu for ACL
	unsigned char aclEnable, domainEnable, cwmp_aclEnable;
	unsigned char dhcpvalue[32];
	unsigned char vChar;
	int dhcpmode;
	// Added by Mason Yu for URL Blocking
#if defined( URL_BLOCKING_SUPPORT)||defined( URL_ALLOWING_SUPPORT)
	unsigned char urlEnable;
#endif
#ifdef NAT_CONN_LIMIT
	unsigned char connlimitEn;
#endif

#ifdef CONFIG_RTK_L34_ENABLE
	char sysbuf[128];
	//iptables -F
	//iptables -P INPUT ACCEPT
	sprintf(sysbuf,"/bin/echo 1 > /proc/sys/net/ipv4/ip_forward");
	printf("system(): %s\n",sysbuf);
	system(sysbuf);
#endif

	//-------------------------------------------------
	// Filter table
	//-------------------------------------------------
	//--------------- set policies --------------
#ifdef REMOTE_ACCESS_CTL
	// iptables -P INPUT DROP
	va_cmd(IPTABLES, 3, 1, "-P", (char *)FW_INPUT, (char *)FW_DROP);
#else // IP_ACL
	// iptables -P INPUT ACCEPT
	va_cmd(IPTABLES, 3, 1, "-P", (char *)FW_INPUT, (char *)FW_ACCEPT);
#endif

	// iptables -P FORWARD ACCEPT
	va_cmd(IPTABLES, 3, 1, "-P", (char *)FW_FORWARD, (char *)FW_ACCEPT);

#ifdef CONFIG_00R0 //iulian move here for test case requirement
#ifdef MAC_FILTER
		//	Add Chain(macfilter)
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_MACFILTER);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_MACFILTER);
#endif

#ifdef IP_PORT_FILTER
	// IP Filter
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_IPFILTER_LOCAL);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_IPFILTER_LOCAL);
#endif
#endif

#ifdef REMOTE_ACCESS_CTL
	// accept related
	// iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-m", "state",
		"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);
#endif

	// It must be in the beginning of FORWARD chain.
	filter_set_tcpmss();
#ifdef CONFIG_LUNA_DUAL_LINUX
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_INPUT, "-i", "vwlan", "-j", (char *)FW_ACCEPT);
#endif
#ifdef CONFIG_USER_FON
	setFonFirewall();
#endif
	// Kaohj -- allowing RIP (in case of ACL blocking)
	// iptables -A INPUT -p udp --dport 520 -j ACCEPT
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p",
		"udp", (char *)FW_DPORT, "520", "-j", (char *)FW_ACCEPT);

	// Drop UPnP SSDP from the WAN side
    va_cmd(IPTABLES, 9, 1, (char *)FW_ADD, (char *)FW_INPUT, "!", (char *)ARG_I, (char *)LANIF, "-d", "239.255.255.250", "-j", (char *)FW_DROP);

	// Allowing multicast access, ie. IGMP, RIPv2
	// iptables -A INPUT -d 224.0.0.0/4 -j ACCEPT
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_INPUT, "-d",
		"224.0.0.0/4", "-j", (char *)FW_ACCEPT);

#ifdef IP_ACL
	//  Add Chain(aclblock) for ACL
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_ACL);
	// Add chain(aclblock) on nat
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)FW_ACL);
	// Add chain(aclblock) on nat
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", (char *)FW_ACL);	
	restart_acl();	
	//iptables -A INPUT -j aclblock
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_ACL);
	// iptables -t nat -A PREROUTING -j aclblock
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)FW_ACL);
	// iptables -t mangle -A PREROUTING -j aclblock
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)FW_ACL);
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_ACL);
	mib_get(MIB_V6_ACL_CAPABILITY, (void *)&aclEnable);
	if (aclEnable == 1)  // ACL Capability is enabled
		filter_set_v6_acl(1);
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_ACL);
#endif	
#endif

#ifdef _CWMP_MIB_
	//  Add Chain(cwmp_aclblock) for ACL
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_CWMP_ACL);	
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", (char *)FW_CWMP_ACL);
	
	mib_get(CWMP_ACL_ENABLE, (void *)&cwmp_aclEnable);
	//if (cwmp_aclEnable == 1)  // ACL Capability is enabled
	//	filter_set_cwmp_acl(1);
	restart_cwmp_acl();
	
	//iptables -A INPUT -j cwmp_aclblock
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_CWMP_ACL);	
	// iptables -t mangle -A PREROUTING -j cwmp_aclblock
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)FW_CWMP_ACL);
#endif

#ifdef NAT_CONN_LIMIT
	//Add Chain(connlimit) for NAT conn limit
	va_cmd(IPTABLES, 2, 1, "-N", "connlimit");

	mib_get(MIB_NAT_CONN_LIMIT, (void *)&connlimitEn);
	if (connlimitEn == 1)
		set_conn_limit();

	//iptables -A FORWARD -j connlimit
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", "connlimit");
#endif
#ifdef TCP_UDP_CONN_LIMIT
	//Add Chain(connlimit) for NAT conn limit
	va_cmd(IPTABLES, 2, 1, "-N", "connlimit");

	mib_get(MIB_CONNLIMIT_ENABLE, (void *)&vChar);
	if (vChar == 1)
		set_conn_limit();
	//iptables -A FORWARD -j connlimit
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", "connlimit");
#endif

	block_br_wan();

#ifdef CONFIG_USER_RTK_VOIP
	voip_setup_iptable(0);
#endif

#ifdef WLAN_SUPPORT
	// setup chain for wlan blocking
	va_cmd(EBTABLES, 4, 1, "-N", "wlan_block", "-P", "RETURN");
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, "FORWARD", "-j", "wlan_block");
#endif

#ifdef PPPOE_PASSTHROUGH
	//  W.H. Hung: Add Chain br_pppoe for bridged PPPoE
	va_cmd(EBTABLES, 2, 1, "-N", FW_BR_PPPOE);
	va_cmd(EBTABLES, 3, 1, "-P", FW_BR_PPPOE, "RETURN");
	va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N", FW_BR_PPPOE);
	va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P", FW_BR_PPPOE, "RETURN");
	setupBrPppoe();
	// and to FORWORD chain
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", FW_BR_PPPOE);
	va_cmd(EBTABLES, 6, 1, "-t", "broute", (char *)FW_ADD, "BROUTING", "-j", FW_BR_PPPOE);
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
	//  Add Chain(domainblk) for ACL
	va_cmd(IPTABLES, 2, 1, "-N", "domainblk");
	mib_get(MIB_DOMAINBLK_CAPABILITY, (void *)&domainEnable);
	if (domainEnable == 1)  // Domain blocking Capability is enabled
		filter_set_domain(1);
    
    patch_for_office365();
    
	//iptables -A INPUT -j domainblk
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", "domainblk");

	//iptables -A FORWARD -j domainblk
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", "domainblk");

	//iptables -A OUTPUT -j domainblk
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, "OUTPUT", "-j", "domainblk");
#endif

#ifdef IP_PASSTHROUGH
	// IP Passthrough, LAN access
	set_IPPT_LAN_access();
#endif

#ifdef CONFIG_USER_CWMP_TR069
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IPTABLE_TR069);
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, "INPUT", "-j", (char *)IPTABLE_TR069);
	set_TR069_Firewall(1);
#endif

#ifdef REMOTE_ACCESS_CTL
	// Add chain for remote access
	// iptables -N inacc
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_INACC);
	va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_INACC,
		 "!", (char *)ARG_I, LANIF, "-m", "mark", "--mark", RMACC_MARK, "-j", FW_ACCEPT);

	filter_set_remote_access(1);

#ifdef CONFIG_USER_DHCP_SERVER
	// Added by Mason Yu for dhcp Relay. Open DHCP Relay Port for Incoming Packets.
	if (mib_get(MIB_DHCP_MODE, (void *)dhcpvalue) != 0)
	{
		dhcpmode = (unsigned int)(*(unsigned char *)dhcpvalue);
		if (dhcpmode == 1 || dhcpmode == 2 ){
			// iptables -A inacc -i ! br0 -p udp --dport 67 -j ACCEPT
			va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_INACC, "!", (char *)ARG_I, BRIF, "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_ACCEPT);

#ifdef CONFIG_USER_BRIDGE_GROUPING
#ifdef CONFIG_RTK_L34_ENABLE // Rostelecom, Port Binding function
			set_port_binding_rule(1);
#endif
#endif
		}
	}
#endif

#ifdef CONFIG_USER_ROUTED_ROUTED
	// Added by Mason Yu. Open RIP Port for Incoming Packets.
	if (mib_get( MIB_RIP_ENABLE, (void *)&vChar) != 0)
	{
		if (1 == vChar)
		{
			// iptables -A inacc -i ! br0 -p udp --dport 520 -j ACCEPT
			va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_INACC, "!", "-i", BRIF, "-p", "udp", (char *)FW_DPORT, "520", "-j", (char *)FW_ACCEPT);
		}
	}
#endif

	// iptables -A INPUT -j inacc
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_INACC);
#endif // of REMOTE_ACCESS_CTL

#ifdef CONFIG_USER_PORTMAPPING
#ifdef CONFIG_USER_DHCP_SERVER
	// iptables -N portmapping_dhcp
	va_cmd(IPTABLES, 2, 1, "-N", (char *)PORTMAP_IPTBL);	
	// iptables -A INPUT -j portmapping_dhcp
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)PORTMAP_IPTBL);
#endif

	//Drop DHCP requests from LAN if DHCP is disabled on WAN
	va_cmd(EBTABLES, 2, 1, "-N", FW_DHCPS_DIS);
	va_cmd(EBTABLES, 3, 1, "-P", FW_DHCPS_DIS, "RETURN");
	//Check packets from DHCP client
	va_cmd(EBTABLES, 12, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "ip"
           , "--ip-protocol", "17", "--ip-sport", "68", "--ip-dport", "67"
           , "-j", FW_DHCPS_DIS);

#ifdef CONFIG_USER_IGMPPROXY
	// ebtables -N portmapping_igmp
	va_cmd(EBTABLES, 2, 1, "-N", PORTMAP_IGMP);
	// ebtables -P portmapping_igmp RETURN
	va_cmd(EBTABLES, 3, 1, "-P", PORTMAP_IGMP, "RETURN");
	// ebtables -A INPUT -j portmapping_igmp
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", PORTMAP_IGMP);
#endif
#endif // of CONFIG_USER_PORTMAPPING

	// Add chain for dhcp Port base filter
	// iptables -N dhcp_port_filter
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_DHCP_PORT_FILTER);
	set_dhcp_port_base_filter(1);
	// iptables -A INPUT -j dhcp_port_filter
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", (char *)FW_DHCP_PORT_FILTER);


#ifdef REMOTE_ACCESS_CTL
	// Added by Mason Yu for accept packet with ip(127.0.0.1)
	// Magician: Modify this rule to allow to ping self. Merge 192.168.1.1-to-192.168.1.1 accepted rule and to-127.0.0.1 accpeted rule.
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_INPUT, "-i", "lo", "-j", (char *)FW_ACCEPT);
#endif

	// Single PC mode
	if (mib_get(MIB_SPC_ENABLE, (void *)value) != 0)
	{
		if (value[0]) // Single PC mode enabled
		{
			spc_enable = 1;
			mib_get(MIB_SPC_IPTYPE, (void *)value);
			if (value[0] == 0) // single private IP
			{
				spc_ip = 0;
				mib_get(MIB_ADSL_LAN_IP, (void *)value);

				// IP pool start address
				// Kaohj
				#ifndef DHCPS_POOL_COMPLETE_IP
				mib_get(MIB_ADSL_LAN_CLIENT_START, (void *)&value[3]);
				strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
				#else
				mib_get(MIB_DHCP_POOL_START, (void *)value);
				strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
				#endif
				ipaddr[15] = '\0';
				// iptables -A FORWARD -i $LANIF -s ! ipaddr -j DROP
				va_cmd(IPTABLES, 9, 1, (char *)FW_ADD,
					(char *)FW_FORWARD, (char *)ARG_I,
					(char *)LANIF, "!", "-s", ipaddr,
					"-j", (char *)FW_DROP);
			}
			else // single IP passthrough
			{
				spc_ip = 1;
			}
		}
		else
			spc_enable = 0;
	}

#ifdef IP_PASSTHROUGH
	// check vc
	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;

		if (spc_enable && (spc_ip == 1)) // single IP passthrough (public IP)
		{
			if ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_RT1483 &&
				ippt_itf == Entry.ifIndex)
			{	// ippt WAN interface (1483-r)
				if ((DHCP_T)Entry.ipDhcp == DHCP_DISABLED)
				{
					strncpy(ipaddr, inet_ntoa(*((struct in_addr *)Entry.ipAddr)), 16);
					ipaddr[15] = '\0';
					// iptables -A FORWARD -i $LANIF -s ! ipaddr -j DROP
					va_cmd(IPTABLES, 9, 1, (char *)FW_ADD,
						(char *)FW_FORWARD, (char *)ARG_I,
						(char *)LANIF, "!", "-s", ipaddr,
						"-j", (char *)FW_DROP);
				}
			}
		}
	}
#endif

#if defined(URL_BLOCKING_SUPPORT)||defined(URL_ALLOWING_SUPPORT)
 mib_get(MIB_URL_CAPABILITY, (void *)&urlEnable);
#endif

#ifdef URL_BLOCKING_SUPPORT
// Added by Mason Yu for URL Blocking
//  Add Chain(urlblock) for URL
 va_cmd(IPTABLES, 2, 1, "-N", "urlblock");
 if (urlEnable == 1)  // URL Capability enabled
   filter_set_url(1);
  //iptables -A FORWARD -j urlblock
   va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", "urlblock");
 #endif

 #ifdef URL_ALLOWING_SUPPORT
   va_cmd(IPTABLES,2,1 ,"-N","urlallow");
   if(urlEnable==2)
 	set_url(2); //alex

 	//iptables -A FORWARD -j urlallow
   va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", "urlallow");
#endif

#ifdef LAYER7_FILTER_SUPPORT
	//App Filter
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_APPFILTER);
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_APP_P2PFILTER);
	va_cmd(IPTABLES, 4, 1, ARG_T, "mangle", "-N", (char *)FW_APPFILTER);
	setupAppFilter();
	// iptables -A FORWARD -j appfilter
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_APPFILTER);
	// iptables -t mangle -A POSTROUTING -j appfilter
	va_cmd(IPTABLES, 6, 1, ARG_T, "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", (char *)FW_APPFILTER);
#endif

#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
	// iptables -t nat -N WebRedirect_Wizard
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", "WebRedirect_Wizard");
	// iptables -t nat -A PREROUTING -j WebRedirect_Wizard
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", "WebRedirect_Wizard");

	system("/bin/echo 1 > /tmp/WebRedirectChain");
#endif

#ifdef PORT_FORWARD_GENERAL
	// port forwarding
	// Add New chain on filter and nat
	// iptables -N portfw
	va_cmd(IPTABLES, 2, 1, "-N", (char *)PORT_FW);
	// iptables -A FORWARD -j portfw
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)PORT_FW);
	// iptables -t nat -N portfw
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)PORT_FW);
	// iptables -t nat -A PREROUTING -j portfw
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)PORT_FW);

#ifdef NAT_LOOPBACK
	// iptables -t nat -N portfwPreNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)PORT_FW_PRE_NAT_LB);
	// iptables -t nat -A PREROUTING -j portfwPreNatLB
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)PORT_FW_PRE_NAT_LB);
	// iptables -t nat -N portfwPostNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)PORT_FW_POST_NAT_LB);
	// iptables -t nat -A POSTROUTING -j portfwPostNatLB
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", (char *)PORT_FW_POST_NAT_LB);
#endif
	setupPortFW();
#endif

#ifdef PORT_FORWARD_ADVANCE
	setupPFWAdvance();
#endif

#ifdef NATIP_FORWARDING
	fw_setupIPForwarding();
#endif

#ifdef PORT_TRIGGERING
	// Add New chain on filter and nat
	// iptables -N portTrigger
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IPTABLES_PORTTRIGGER);
	// iptables -A FORWARD -j portTrigger
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)IPTABLES_PORTTRIGGER);
	// iptables -t nat -N portTrigger
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)IPTABLES_PORTTRIGGER);
	// iptables -t nat -A PREROUTING -j portTrigger
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)IPTABLES_PORTTRIGGER);
	setupPortTriggering();
#endif
#ifdef IP_PORT_FILTER
	// IP Filter
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_IPFILTER);
	setupIPFilter();
	// iptables -A FORWARD -j ipfilter
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_IPFILTER);
#endif

#ifdef PARENTAL_CTRL
	// parental_ctrl
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_PARENTAL_CTRL);
	// iptables -A FORWARD -j parental_ctrl
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_PARENTAL_CTRL);
#endif

#ifdef DMZ
#ifdef NAT_LOOPBACK
	// iptables -t nat -N dmzPreNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)IPTABLE_DMZ_PRE_NAT_LB);
	// iptables -t nat -A PREROUTING -j dmzPreNatLB
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)IPTABLE_DMZ_PRE_NAT_LB);
	// iptables -t nat -N dmzPostNatLB
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)IPTABLE_DMZ_POST_NAT_LB);
	// iptables -t nat -A POSTROUTING -j dmzPostNatLB
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", (char *)IPTABLE_DMZ_POST_NAT_LB);
#endif

	// DMZ
	// Add New chain on filter and nat
	// iptables -N dmz
	va_cmd(IPTABLES, 2, 1, "-N", (char *)IPTABLE_DMZ);
	// iptables -t nat -N dmz
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)IPTABLE_DMZ);
	setupDMZ();
#ifdef IP_PORT_FILTER
	// iptables -A filter -j dmz
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_IPFILTER, "-j", (char *)IPTABLE_DMZ);
#endif
	// iptables -t nat -A PREROUTING -j dmz
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_ADD, (char *)FW_PREROUTING, "-j", (char *)IPTABLE_DMZ);
#endif

#ifdef IP_PORT_FILTER
	// Set IP filter default action
	setup_default_IPFilter();
#endif

#ifdef MAC_FILTER
#ifdef CONFIG_RTK_L34_ENABLE
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-N", (char *)FW_MACFILTER);
	va_cmd(IPTABLES, 6, 1, "-t", "nat", (char *)FW_INSERT, (char *)FW_PREROUTING, "-j", (char *)FW_MACFILTER);
#endif
	// Mac Filter
	va_cmd(EBTABLES, 2, 1, "-N", (char *)FW_MACFILTER);
	setupMacFilter();
	// ebtables -A FORWARD -j macfilter
	va_cmd(EBTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_MACFILTER);
#endif

#if defined CONFIG_RTK_L34_ENABLE && defined CONFIG_APOLLO_ROMEDRIVER
	// ebtables -P FORWARD DROP, Rome driver will do it for Kernel.
	va_cmd(EBTABLES, 3, 1, "-P", (char *)FW_FORWARD, (char *)FW_DROP);
	va_cmd(EBTABLES, 2, 1, "-N", "disBCMC");
	va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "disBCMC", "-d", "Broadcast", "-j", FW_DROP);
	va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "disBCMC", "-d", "Multicast", "-j", FW_DROP);
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", "disBCMC");
	va_cmd(EBTABLES, 3, 1, "-P", "disBCMC", (char *)FW_DROP);
#ifdef CONFIG_RTL_MESH_SUPPORT
	// add rules for WIFI mesh
	va_cmd(EBTABLES, 2, 1, "-N", "wifi_mesh");
	va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, "wifi_mesh", "-i", "eth+", "-o", "wlan-msh", "-j", FW_ACCEPT);
	va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, "wifi_mesh", "-i", "wlan+", "-o", "wlan-msh", "-j", FW_ACCEPT);
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", "wifi_mesh");	
#endif
#endif

#ifdef SUPPORT_FON_GRE
	// iptables -N dhcp_queue
	va_cmd(IPTABLES, 2, 1, "-N", "dhcp_queue");
	// iptables -A FORWARD -j dhcp_queue
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", "dhcp_queue");
	// iptables -N FON_GRE_FORWARD
	va_cmd(IPTABLES, 2, 1, "-N", "FON_GRE_FORWARD");
	// iptables -I FORWARD -j FON_GRE_FORWARD
	va_cmd(IPTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", "FON_GRE_FORWARD");
	// iptables -N FON_GRE_INPUT
	va_cmd(IPTABLES, 2, 1, "-N", "FON_GRE_INPUT");
	// iptables -I INPUT -j FON_GRE_INPUT
	va_cmd(IPTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_INPUT, "-j", "FON_GRE_INPUT");
	// iptables -t mangle -N FON_GRE
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", "FON_GRE");
	// iptables -t mangle -A FORWARD -j FON_GRE
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_FORWARD, "-j", "FON_GRE");

	va_cmd(EBTABLES, 2, 1, "-N", "FON_GRE");
	va_cmd(EBTABLES, 4, 1, (char *)FW_INSERT, (char *)FW_FORWARD, "-j", "FON_GRE");
	va_cmd(EBTABLES, 3, 1, "-P", "FON_GRE", (char *)FW_RETURN);
#endif

#ifdef CONFIG_XFRM
	// iptables -t mangle -N XFRM
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", "XFRM");
	// iptables -t mangle -A POSTROUTING -j XFRM
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", (char *)FW_ADD, (char *)FW_POSTROUTING, "-j", "XFRM");
#endif

#ifdef LAYER7_FILTER_SUPPORT
	// iptables -A FORWARD -j appp2pfilter
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_APP_P2PFILTER);
#endif


#ifdef VIRTUAL_SERVER_SUPPORT
	//execute virtual server rules
	setupVtlsvr();
#endif

 	// for multicast
#if 0 // move ahead to ipfilter chain
	// iptables -A FORWARD -d 224.0.0.0/4 -j ACCEPT
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-d",
		"224.0.0.0/4", "-j", (char *)FW_ACCEPT);
#endif

	// iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
	// andrew. moved to the 1st rule, or returning PING/DNS relay will be blocked.
	//va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-m", "state",
	//	"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);

#ifdef REMOTE_ACCESS_CTL
	// iptables -A INPUT -m state --state NEW -i $LAN_INTERFACE -j ACCEPT
	va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_INPUT, "-m", "state",
		"--state", "NEW", (char *)ARG_I, (char *)LANIF,
		"-j", (char *)FW_ACCEPT);
#endif

#if 0
	/*--------------------------------------------------------------------
	 * The following are the default action and should be final rules
	 -------------------------------------------------------------------*/
	// iptables -N block
	va_cmd(IPTABLES, 2, 1, "-N", (char *)FW_BLOCK);

	// default action
	if (mib_get(MIB_IPF_OUT_ACTION, (void *)value) != 0)
	{
		vInt = (int)(*(unsigned char *)value);
		if (vInt == 1)	// ACCEPT
		{
			// iptables -A block -i $LAN_IF -j ACCEPT
			va_cmd(IPTABLES, 6, 1, (char *)FW_ADD,
				(char *)FW_BLOCK, (char *)ARG_I,
				(char *)LANIF, "-j", (char *)FW_ACCEPT);
		}
	}

	if (mib_get(MIB_IPF_IN_ACTION, (void *)value) != 0)
	{
		vInt = (int)(*(unsigned char *)value);
		if (vInt == 1)	// ACCEPT
		{
			// iptables -A block -i ! $LAN_IF -j ACCEPT
			va_cmd(IPTABLES, 7, 1, (char *)FW_ADD,
				(char *)FW_BLOCK, "!", (char *)ARG_I,
				(char *)LANIF, "-j", (char *)FW_ACCEPT);
		}
	}

	// iptables -A block -m state --state ESTABLISHED,RELATED -j ACCEPT
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_BLOCK, "-m", "state",
		"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);

	/*
	// iptables -A block -m state --state NEW -i $LAN_INTERFACE -j ACCEPT
	va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_BLOCK, "-m", "state",
		"--state", "NEW", (char *)ARG_I, (char *)LANIF,
		"-j", (char *)FW_ACCEPT);
	*/

	// iptables -A block -j DROP
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, "block", "-j", (char *)FW_DROP);

	/*
	// iptables -A INPUT -j block
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_INPUT, "-j", "block");
	*/

	// iptables -A FORWARD -j block
	va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", "block");
#endif

#ifdef CONFIG_IPV6
	// INPUT default DROP , ip6table -P INPUT  DROP
	va_cmd(IP6TABLES, 3, 1, "-P", (char *)FW_INPUT, (char *)FW_DROP);

	// ip6tables -A INPUT -p icmpv6 --icmpv6-type router-advertisement -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "icmpv6",
		"--icmpv6-type","router-advertisement","-j", (char *)FW_ACCEPT);

	// ip6tables -I INPUT  -p icmpv6 --icmpv6-type neighbour-solicitation -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "icmpv6",
		"--icmpv6-type","neighbour-solicitation","-j", (char *)FW_ACCEPT);

	// ip6tables -I INPUT  -p icmpv6 --icmpv6-type neighbour-advertisement -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "icmpv6",
		"--icmpv6-type","neighbour-advertisement","-j", (char *)FW_ACCEPT);

	// ip6tables -A INPUT -p icmpv6 -i $LAN_INTERFACE -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "icmpv6",
	(char *)ARG_I, (char *)LANIF, "-j", (char *)FW_ACCEPT);

	// ip6tables -A INPUT -m state --state NEW -i $LAN_INTERFACE -j ACCEPT
	va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_INPUT, "-m", "state",
		"--state", "NEW", (char *)ARG_I, (char *)LANIF,
		"-j", (char *)FW_ACCEPT);

	// ip6tables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-m", "state",
			"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);

	// ip6tables  -A INPUT -p udp --dport  546 -j ACCEPT
	va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_INPUT, "-p", "udp",
			"--dport", "546", "-j", (char *)FW_ACCEPT);

	//  Added by Mason Yu for dhcpv6 Relay. Open DHCPv6 Relay Port for Incoming Packets.
	// ip6tables  -A INPUT ! -i $LAN_INTERFACE -p udp --dport  547 -j ACCEPT
	va_cmd(IP6TABLES, 11, 1, (char *)FW_ADD, (char *)FW_INPUT, "!", (char *)ARG_I, (char *)LANIF,
			"-p", "udp", "--dport", "547", "-j", (char *)FW_ACCEPT);

	// IPv6 Filter
	va_cmd(IP6TABLES, 2, 1, "-N", (char *)FW_IPV6FILTER);
	// ip6tables -A FORWARD -j ipv6filter
	va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-j", (char *)FW_IPV6FILTER);
	restart_IPV6Filter();
#endif
#if defined (CONFIG_USER_PPTPD_PPTPD) || defined(CONFIG_USER_L2TPD_LNS)
	set_LAN_VPN_accept((char *)FW_INPUT);
#endif

#if defined(CONFIG_00R0) && defined(CONFIG_RTK_L34_ENABLE) && defined(CONFIG_USER_RTK_VOIP)
		voip_set_rg_1p_tag();
#endif


	return 1;
}
#ifdef CONFIG_IP_NF_ALG_ONOFF
//add by ramen return 1--sucessful
// Mason Yu. alg_onoff_20101023
int setupAlgOnOff(void)
{
	unsigned char value=0;
#ifdef CONFIG_NF_CONNTRACK_FTP
	if(mib_get(MIB_IP_ALG_FTP,&value)&& value==0)
		system("/bin/echo 0 > /proc/algonoff_ftp");
	else
		system("/bin/echo 1 > /proc/algonoff_ftp");
#endif
#ifdef CONFIG_NF_CONNTRACK_TFTP
	if(mib_get(MIB_IP_ALG_TFTP,&value)&& value==0)
		system("/bin/echo 0 > /proc/algonoff_tftp");
	else
		system("/bin/echo 1 > /proc/algonoff_tftp");
#endif
#ifdef CONFIG_NF_CONNTRACK_H323
	if(mib_get(MIB_IP_ALG_H323,&value)&& value==0)
		system("/bin/echo 0 >/proc/algonoff_h323");
	else
		system("/bin/echo 1 >/proc/algonoff_h323");
#endif
#ifdef CONFIG_NF_CONNTRACK_IRC
	if(mib_get(MIB_IP_ALG_IRC,&value)&& value==0)
		system("/bin/echo 0 >/proc/algonoff_irc");
	else
		system("/bin/echo 1 >/proc/algonoff_irc");
#endif
#ifdef CONFIG_NF_CONNTRACK_RTSP
	if(mib_get(MIB_IP_ALG_RTSP,&value)&& value==0)
		system("/bin/echo 0 >/proc/algonoff_rtsp");
	else
		system("/bin/echo 1 >/proc/algonoff_rtsp");
#endif
#ifdef CONFIG_NF_CONNTRACK_QUAKE3
	if(mib_get(MIB_IP_ALG_QUAKE3,&value)&& value==0)
		system("/bin/echo 0 > /proc/algonoff_quake3");
	else
		system("/bin/echo 1 > /proc/algonoff_quake3");
#endif
#ifdef CONFIG_NF_CONNTRACK_CUSEEME
	if(mib_get(MIB_IP_ALG_CUSEEME,&value)&& value==0)
		system("/bin/echo 0 > /proc/algonoff_cuseeme");
	else
		system("/bin/echo 1 > /proc/algonoff_cuseeme");
#endif
#ifdef CONFIG_NF_CONNTRACK_L2TP
	if(mib_get(MIB_IP_ALG_L2TP,&value)&& value==0)
		system("/bin/echo 0 > /proc/algonoff_l2tp");
	else
		system("/bin/echo 1 > /proc/algonoff_l2tp");
#endif
#ifdef CONFIG_NF_CONNTRACK_IPSEC
	if(mib_get(MIB_IP_ALG_IPSEC,&value)&& value==0)
		system("/bin/echo 0 > /proc/algonoff_ipsec");
	else
		system("/bin/echo 1 > /proc/algonoff_ipsec");
#endif
#ifdef CONFIG_NF_CONNTRACK_SIP
	if(mib_get(MIB_IP_ALG_SIP,&value)&& value==0)
		system("/bin/echo 0 > /proc/algonoff_sip");
	else
		system("/bin/echo 1 > /proc/algonoff_sip");
#endif
#ifdef CONFIG_NF_CONNTRACK_PPTP
	if(mib_get(MIB_IP_ALG_PPTP,&value)&& value==0)
		system("/bin/echo 0 > /proc/algonoff_pptp");
	else
		system("/bin/echo 1 > /proc/algonoff_pptp");
#endif
#ifdef CONFIG_RTK_L34_ENABLE
	RTK_RG_ALG_Set();
#endif
	return;
}
#endif


#ifdef WEB_REDIRECT_BY_MAC
static int start_web_redir_by_mac(void)
{
	int status=0;
	char tmpbuf[MAX_URL_LEN];
	char ipaddr[16], ip_port[32], redir_server[33];
	int  def_port=WEB_REDIR_BY_MAC_PORT;

	ipaddr[0]='\0'; ip_port[0]='\0';redir_server[0]='\0';
	if (mib_get(MIB_ADSL_LAN_IP, (void *)tmpbuf) != 0)
	{
		strncpy(ipaddr, inet_ntoa(*((struct in_addr *)tmpbuf)), 16);
		ipaddr[15] = '\0';
		sprintf(ip_port,"%s:%d",ipaddr,def_port);
	}//else ??

	//iptables -t nat -N WebRedirectByMAC
	status|=va_cmd(IPTABLES, 4, 1, "-t", "nat","-N","WebRedirectByMAC");

	//iptables -t nat -A WebRedirectByMAC -d 192.168.1.1 -j RETURN
	status|=va_cmd(IPTABLES, 8, 1, "-t", "nat",(char *)FW_ADD,"WebRedirectByMAC",
		"-d", ipaddr, "-j", (char *)FW_RETURN);

	//iptables -t nat -A WebRedirectByMAC -p tcp --dport 80 -j DNAT --to-destination 192.168.1.1:8080
	status|=va_cmd(IPTABLES, 12, 1, "-t", "nat",(char *)FW_ADD,"WebRedirectByMAC",
		"-p", "tcp", "--dport", "80", "-j", "DNAT",
		"--to-destination", ip_port);

	//iptables -t nat -A PREROUTING -i eth0 -p tcp --dport 80 -j WebRedirectByMAC
	status|=va_cmd(IPTABLES, 12, 1, "-t", "nat",(char *)FW_ADD,"PREROUTING",
		"-i", LANIF,
		"-p", "tcp", "--dport", "80", "-j", "WebRedirectByMAC");


{
	int num,i;
	unsigned char tmp2[18];
	MIB_WEB_REDIR_BY_MAC_T	wrm_entry;

	num = mib_chain_total( MIB_WEB_REDIR_BY_MAC_TBL );
	//printf( "\nnum=%d\n", num );
	for(i=0;i<num;i++)
	{
		if( !mib_chain_get( MIB_WEB_REDIR_BY_MAC_TBL, i, (void*)&wrm_entry ) )
			continue;

		sprintf( tmp2, "%02X:%02X:%02X:%02X:%02X:%02X",
				wrm_entry.mac[0], wrm_entry.mac[1], wrm_entry.mac[2], wrm_entry.mac[3], wrm_entry.mac[4], wrm_entry.mac[5] );
		//printf( "add one mac: %s \n", tmp2 );
		// iptables -A macfilter -i eth0  -m mac --mac-source $MAC -j ACCEPT/DROP
		status|=va_cmd("/bin/iptables", 10, 1, "-t", "nat", (char *)FW_INSERT, "WebRedirectByMAC", "-m", "mac", "--mac-source", tmp2, "-j", "RETURN");
	}
}

	return 0;
}
#endif

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
int start_captiveportal(void)
{
	int status = 0;
	char tmpbuf[MAX_URL_LEN];
	char lan_ipaddr[16], lan_ip_port[32];
	char ip_mask[24];
	int  def_port = CAPTIVEPORTAL_PORT, i, num;
	FILE *fp;

	lan_ipaddr[0] = '\0';
	lan_ip_port[0] = '\0';

	if (mib_get(MIB_ADSL_LAN_IP, (void *)tmpbuf) != 0)
	{
		strncpy(lan_ipaddr, inet_ntoa(*((struct in_addr *)tmpbuf)), 16);
		lan_ipaddr[15] = '\0';
		sprintf(lan_ip_port, "%s:%d", lan_ipaddr, def_port);
	}
	else
		return -1;

	//iptables -t nat -A PREROUTING -p tcp -d 192.168.1.1 --dport 80 -j RETURN
	status |= va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_ADD, "PREROUTING",	"-p", "tcp", "-d", lan_ipaddr, "--dport", "80", "-j", (char *)FW_RETURN);

	//iptables -t nat -A PREROUTING -p tcp --dport 80 -j DNAT --to-destination 192.168.1.1:18182
	status |= va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_ADD, "PREROUTING", "-p", "tcp", "--dport", "80", "-j", "DNAT", "--to-destination", lan_ip_port);

	CWMP_CAPTIVEPORTAL_ALLOWED_LIST_T ccal_entry;

	if(!(fp = fopen("/var/cp_allow_ip", "w")))
	{
		fprintf(stderr, "Open file cp_allow_ip failed!");
		return -1;
	}

	num = mib_chain_total(CWMP_CAPTIVEPORTAL_ALLOWED_LIST);

	for( i = 0; i < num; i++ )
	{
		if(!mib_chain_get(CWMP_CAPTIVEPORTAL_ALLOWED_LIST, i, (void*)&ccal_entry))
			continue;

		if( ccal_entry.mask < 32 ) // Valid subnet mask
			sprintf(ip_mask, "%s/%d", inet_ntoa(*(struct in_addr *)&ccal_entry.ip_addr), ccal_entry.mask);
		else	// Invalid subnet mask or unset subnet mask.
			sprintf(ip_mask, "%s", inet_ntoa(*(struct in_addr *)&ccal_entry.ip_addr));

		// iptables -t nat -I PREROUTING -p tcp -d 209.85.175.104/24 --dport 80 -j RETURN
		status |= va_cmd(IPTABLES, 12, 1, "-t", "nat", (char *)FW_INSERT, "PREROUTING", "-p", "tcp", "-d", ip_mask, "--dport", "80", "-j", "RETURN");
		fprintf(fp, "%s\n", ip_mask);
	}

	fclose(fp);

	return status;
}

int stop_captiveportal(void)
{
	int status = 0;
	char tmpbuf[MAX_URL_LEN];
	char lan_ipaddr[16], lan_ip_port[32];
	char ip_mask[24];
	int  def_port = CAPTIVEPORTAL_PORT, i;
	FILE *fp;

	if(fp = fopen("/var/cp_allow_ip", "r"))
	{
		char *tmp;

		while(fgets(ip_mask, 24, fp))
		{
			tmp = strchr(ip_mask, '\n');
			*tmp = '\0';

			// iptables -t nat -D PREROUTING -p tcp -d 209.85.175.104/24 --dport 80 -j RETURN
			va_cmd(IPTABLES, 12, 1, "-t", "nat", "-D", "PREROUTING", "-p", "tcp", "-d", ip_mask, "--dport", "80", "-j", "RETURN");
		}

		fclose(fp);
		unlink("/var/cp_allow_ip");
	}

	lan_ipaddr[0] = '\0';
	lan_ip_port[0] = '\0';

	if (mib_get(MIB_ADSL_LAN_IP, (void *)tmpbuf) != 0)
	{
		strncpy(lan_ipaddr, inet_ntoa(*((struct in_addr *)tmpbuf)), 16);
		lan_ipaddr[15] = '\0';
		sprintf(lan_ip_port, "%s:%d", lan_ipaddr, def_port);
	}
	else
		return -1;

	//iptables -t nat -D PREROUTING -p tcp -d 192.168.1.1 --dport 80 -j RETURN
	status |= va_cmd(IPTABLES, 12, 1, "-t", "nat", "-D", "PREROUTING", "-p", "tcp", "-d", lan_ipaddr, "--dport", "80", "-j", (char *)FW_RETURN);

	//iptables -t nat -D PREROUTING -p tcp --dport 80 -j DNAT --to-destination 192.168.1.1:18182
	status |= va_cmd(IPTABLES, 12, 1, "-t", "nat", "-D", "PREROUTING", "-p", "tcp", "--dport", "80", "-j", "DNAT", "--to-destination", lan_ip_port);

	return status;
}
#endif

#ifdef CONFIG_RTK_L34_ENABLE
#include "rtusr_rg_api.h"
#endif

static int startWanServiceDependency(int ipEnabled)
{
	char vChar=-1;
	int ret=0;
	int rc=0;

#ifdef DEFAULT_GATEWAY_V2
	unsigned char dgwip[16];
	unsigned int dgw;
	if (mib_get(MIB_ADSL_WAN_DGW_ITF, (void *)&dgw) != 0) {
		if (dgw == DGW_NONE && getMIB2Str(MIB_ADSL_WAN_DGW_IP, dgwip) == 0) {
			if (ifExistedDGW() == 1)
				va_cmd(ROUTE, 2, 1, ARG_DEL, "default");
			// route add default gw remotip
			va_cmd(ROUTE, 4, 1, ARG_ADD, "default", "gw", dgwip);
		}
	}
#endif

	// Add static routes
	// Mason Yu. Init hash table for all routes on RIP
#ifdef ROUTING
	addStaticRoute();
#endif
#ifdef CONFIG_IPV6
	addStaticV6Route();
#endif

#if defined(CONFIG_IPV6) && defined(CONFIG_USER_RADVD)
	//Kill originally running RADVD daemon.
	//Alan
	//1. fix IPv4 routing WAN only, the radvd process still exist
	//2. configd and boa both will restart radvd, we leave configd to start radvd
	rc = radvdRunningMode();
	if (rc==RADVD_RUNNING_MODE_DISABLE)
	{
		int pid = read_pid((char *)RADVD_PID);
		if ( pid > 0)
		{
			printf("Bridge mode only, kill the original radvd deamon.\n");
			kill(pid,SIGTERM);
			//radvd will remove pid file, we do not remove pid file
			//va_cmd("/bin/rm", 1, 0, (char *)RADVD_PID);
		}
	}
#endif

//IulianWu. move the proc operation to /etc/init.d/rcX
/*
	if (ipEnabled!=1)
	{
		setup_ipforwarding(0);
	}
*/

#ifdef CONFIG_USER_ROUTED_ROUTED
	if (startRip() == -1)
	{
		printf("start RIP failed !\n");
		ret = -1;
	}
#endif

#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
	if (startOspf() == -1)
	{
		printf("start OSPF failed !\n");
		ret = -1;
	}
#endif

#ifdef CONFIG_USER_IGMPPROXY
#ifdef CONFIG_IGMPPROXY_MULTIWAN
		if (setting_Igmproxy() == -1)
		{
			printf("start IGMP proxy failed !\n");
			ret = -1;
		}
#else
		if (startIgmproxy() == -1)
		{
			printf("start IGMP proxy failed !\n");
			ret = -1;
		}
#endif // of CONFIG_IGMPPROXY_MULTIWAN
#endif // of CONFIG_USER_IGMPPROXY

// Mason Yu. MLD Proxy
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_ECMH
		if (startMLDproxy() == -1)
		{
			printf("start MLD proxy failed !\n");
			ret = -1;
		}
#endif // of CONFIG_USER_ECMH
#endif

#ifdef CONFIG_USER_DOT1AG_UTILS
		startDot1ag();
#endif

	// execute firewall rules
	if (setupFirewall() == -1)
	{
		printf("execute firewall rules failed !\n");
		ret = -1;
	}
#ifdef CONFIG_IP_NF_ALG_ONOFF
	if(setupAlgOnOff()==-1)
	{
		printf("execute ALG on-off failed!\n");
	}
#endif

#if 0 //eason path for pvc add/del and conntrack killall
	// Kaohj --- remove all conntracks
	va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
#endif

#ifdef WEB_REDIRECT_BY_MAC
	if( -1==start_web_redir_by_mac() )
		ret=-1;
#endif

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	unsigned char cp_enable;
	char cp_url[MAX_URL_LEN];

	mib_get(MIB_CAPTIVEPORTAL_ENABLE, (void *)&cp_enable);
	mib_get(MIB_CAPTIVEPORTAL_URL, (void *)cp_url);

	if(cp_enable && cp_url[0])
		if( -1 == start_captiveportal() )
			ret = -1;
#endif

#ifdef CONFIG_USER_Y1731
	Y1731_start(0);
#endif
	return ret;
}

#ifdef CONFIG_EPON_FEATURE
/*****************************************************************************
 ï¿½ï¿½ ï¿½ï¿½ ï¿½ï¿½  : epon_getAuthState
 ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½  : get epon auth result
 ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿? : llid
 ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿? : ï¿½ï¿½
 ï¿½ï¿½ ï¿½ï¿½ ?µ  : 2 - auth not complete, 1 - auth success, 0 - auth fail
 ï¿½ï¿½ï¿½?ºï¿½ï¿½ï¿½  :
 ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½  :

 ï¿½?¸ï¿½ï¿½ï¿½?·      :
  1.ï¿½ï¿½    ï¿½ï¿½   : 2016ï¿½ï¿½3ï¿½ï¿½25ï¿½ï¿½
    ï¿½ï¿½    ï¿½ï¿½   : ql_xu
    ï¿½?¸ï¿½ï¿½ï¿½ï¿½ï¿½   : ï¿½ï¿½ï¿½ï¿½ï¿½?ºï¿½ï¿½ï¿½

*****************************************************************************/
int epon_getAuthState(int llid)
{
    FILE *fp;
    char cmdStr[256], line[256];
    char tmpStr[20];
    int ret=2;

    snprintf(cmdStr, 256, "oamcli get ctc auth %d > /tmp/auth.result 2>&1", llid);
    va_cmd("/bin/sh", 2, 1, "-c", cmdStr);

    fp = fopen("/tmp/auth.result", "r");
    if (fp)
    {
        fgets(line, sizeof(line), fp);
        sscanf(line, "%*[^:]: %s", tmpStr);
        if (!strcmp(tmpStr, "success"))
            ret = 1;
        else if (!strcmp(tmpStr, "fail"))
            ret = 0;

        fclose(fp);
    }
    return (ret);
}

int getEponONUState(unsigned int llidx)
{
	int ret;
	rtk_epon_llid_entry_t  llidEntry;
	rtk_epon_counter_t counter;

	memset(&llidEntry, 0, sizeof(rtk_epon_llid_entry_t));
	llidEntry.llidIdx = llidx;
#ifdef CONFIG_RTK_L34_ENABLE
	rtk_rg_epon_llid_entry_get(&llidEntry);
#else
	rtk_epon_llid_entry_get(&llidEntry);
#endif

	if(llidEntry.valid)//OLT Register successful
		return 5;

	memset(&counter, 0, sizeof(rtk_epon_counter_t));
	counter.llidIdx = llidx;
#ifdef CONFIG_RTK_L34_ENABLE
	ret = rtk_rg_epon_mibCounter_get(&counter);
#else
	ret = rtk_epon_mibCounter_get(&counter);
#endif
	if(ret == RT_ERR_OK)
	{
		if(counter.llidIdxCnt.mpcpRxGate)
			return 4;
		else if(counter.mpcpTxRegRequest)
			return 3;
		else if(counter.mpcpRxDiscGate)
			return 2;
		else
			return 1;
	}
	return 0;
}

long epon_getctcAuthSuccTime(int llid)
{
    FILE *fp;
    char cmdStr[256], line[256];
   	unsigned long authSuccTime=0;
    int ret=2;

    snprintf(cmdStr, 256, "oamcli get ctc succAuthTime %d", llid);

    fp = popen(cmdStr, "r");
    if (fp)
    {
        fgets(line, sizeof(line), fp);
        sscanf(line, "%*[^:]: %ld", &authSuccTime);

        pclose(fp);
    }
    return (authSuccTime);
}
#endif

#if ! defined(_LINUX_2_6_) && defined(CONFIG_RTL_MULTI_WAN)
int initWANMode()
{
	FILE *fp;
	int wan_mode=0;
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE) || defined(CONFIG_FIBER_FEATURE)
	mib_get(MIB_PON_MODE, &wan_mode);
#endif
	
	fp = fopen("/proc/rtk_smux/wan_mode", "w");
	if(fp){
		fprintf(fp, "%d", wan_mode);
		fclose(fp);
	}
#if defined(CONFIG_FIBER_FEATURE)	
	if(wan_mode == FIBER_MODE){
		system("/bin/ethctl enable_nas0_wan 1");
	}
#endif
	return 1;
}
static void checkWanModeStatus()
{
	int wan_mode=0, ret = 0;
	char cmd[64] = {0};
	
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE) || defined(CONFIG_FIBER_FEATURE)
	mib_get(MIB_PON_MODE, &wan_mode);
	if(wan_mode == FIBER_MODE || wan_mode == ETH_MODE)
		ret = 1;
#if defined(CONFIG_EPON_FEATURE) 
	else if(wan_mode == EPON_MODE)
	{
		rtk_epon_llid_entry_t  llidEntry;
		memset(&llidEntry, 0, sizeof(rtk_epon_llid_entry_t));
		llidEntry.llidIdx = 0;
#ifdef CONFIG_RTK_L34_ENABLE
		rtk_rg_epon_llid_entry_get(&llidEntry);
#else
		rtk_epon_llid_entry_get(&llidEntry);
#endif
		if(llidEntry.valid){
			ret = epon_getAuthState(llidEntry.llidIdx);
		}
	}
#endif
#else
	ret = 1;
#endif

	if (ret == 1){
		printf(">>>> Checked WAN Mode is connected, recover SMUX interface status ......\n");
		snprintf(cmd, sizeof(cmd)-1, "/bin/ethctl setsmux %s \"*\" carrier 1", ALIASNAME_NAS0);
		system (cmd);
	}
}
#endif

//--------------------------------------------------------
// WAN startup
// return value:
// 1  : successful
// -1 : failed
// configAll = CONFIGALL,  pEntry = NULL  : start all WAN connections(include VC, ETHWAN, PTMWAN, VPN, 3g).
// configAll = CONFIGONE, pEntry != NULL : start specified VC, ETHWAN, PTMWAN connection and VPN, 3g connections.
// configAll = CONFIGONE, pEntry = NULL  : start VPN, 3g connections.
int startWan(int configAll, MIB_CE_ATM_VC_Tp pEntry)
{
	int vcTotal, i, ret;
	MIB_CE_ATM_VC_T Entry;
	int ipEnabled;
	FILE *fp;
	char vcNum[16];
	char ifname[IFNAMSIZ];
	char vChar=-1;
#ifdef CONFIG_DEV_xDSL
	Modem_LinkSpeed vLs;
	vLs.upstreamRate=0;
#endif
	// Mason Yu
	unsigned char *rootdev=NULL;
	unsigned char devAddr[MAC_ADDR_LEN];
	char macaddr[MAC_ADDR_LEN*2+1];
	
	ret = 1;
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);

#ifdef CONFIG_00R0
	int hasVoIPWan = 0; 
#endif

	ipEnabled = 0;

#if defined(CONFIG_ARCH_RTL8198F)
	{
	char	buf[64]={0};
	
	if (pEntry == NULL) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, 0, (void *)&Entry))
			Entry.cmode = 1; // IPoE
	}
	else {
		Entry.cmode = pEntry->cmode;
	}
	sprintf(buf,"echo ChannelMode %d > /proc/driver/realtek/flash_mib", Entry.cmode);
	system(buf);
	}
#endif

	// Mason Yu.
	//If it is a router modem , we should config forwarding first.
	// Because the /proc/.../conf/all/forwarding value, will affect /proc/.../conf/vc0/forwarding value.
	//IulianWu. move the proc operation to /etc/init.d/rcX
	//setup_ipforwarding(1);

#if defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN)

//#ifdef NEW_PORTMAPPING
	// Get port-mapping bitmapped LAN ports (fgroup)
	//get_pmap_fgroup(pmap_list, MAX_VC_NUM);
//#endif

#if 0 //defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_ENABLE)
	if(Init_RG_ELan(UntagCPort, RoutingWan)!=SUCCESS){
		printf("Init_RG_ELan failed!!! \n");
		return -1;
	}
#endif	

	mib_get(MIB_ELAN_MAC_ADDR, (void *)devAddr);
	snprintf(macaddr, 13, "%02x%02x%02x%02x%02x%02x",
			devAddr[0], devAddr[1], devAddr[2],
			devAddr[3], devAddr[4], devAddr[5]);
#if defined(CONFIG_PTMWAN)	
	rootdev=ALIASNAME_PTM0;
	va_cmd(IFCONFIG, 4, 1, rootdev, "hw", "ether", macaddr);
	va_cmd(IFCONFIG, 2, 1, rootdev, "up");
#endif

#if defined(CONFIG_RTL_MULTI_ETH_WAN)	
	rootdev=ALIASNAME_NAS0;
#ifndef CONFIG_ARCH_RTL8198F
	va_cmd(IFCONFIG, 4, 1, rootdev, "hw", "ether", macaddr);
#endif
	va_cmd(IFCONFIG, 2, 1, rootdev, "up");
#endif	

	for (i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				return -1;

		if (Entry.enable == 0)
			continue;

		if(configAll == CONFIGALL)
		{
			addEthWANdev(&Entry);
#ifdef CONFIG_RTK_L34_ENABLE
			// Call RG_add_wan before addEthWANdev because addEthWANdev() will trigger link change events.
			RG_add_wan(&Entry, i);
#endif
		}
		else if(configAll == CONFIGONE)
		{
			 if(pEntry && Entry.ifIndex == pEntry->ifIndex)
			 {
				addEthWANdev(&Entry);
#ifdef CONFIG_RTK_L34_ENABLE
				// Call RG_add_wan before addEthWANdev because addEthWANdev() will trigger link change events.
				RG_add_wan(&Entry, i);
#endif
				break;
			 }
		}
	}
#endif	//defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN)

#ifdef CONFIG_00R0//iulian added cvlan for multicast
	if(RG_WAN_CVLAN() < 0){
		printf("Setting the cVlan failed !\n");
	}
#endif
	// If we set forwarding=1 for all, then the every interafce's forwarding will be modified to 1.
	// So we must set forwarding=1 for all first, then set every interafce's forwarding.
	// Mason Yu.
	//IulianWu. move the proc operation to /etc/init.d/rcX
	//setup_ipforwarding(1);
#ifdef CONFIG_USER_PPPOMODEM
	// wan3g run as routed mode
	if((configAll == CONFIGALL) || (pEntry && (TO_IFINDEX(MEDIA_3G,  MODEM_PPPIDX_FROM, 0) == pEntry->ifIndex)))
	{
		ipEnabled = wan3g_start();
	}
	else
		ipEnabled = wan3g_enable();
#endif //CONFIG_USER_PPPOMODEM

	if (configAll == CONFIGCWMP)
	{
		printf("[%s@%d] CONFIGCWMP start\n", __FUNCTION__, __LINE__);
		FILE *f_start = NULL;
		f_start = fopen(CWMP_START_WAN_LIST, "r");
		int ifIdx = 0;
		if (f_start == NULL)
		{
			printf("[%s@%d] Open list file fail\n", __FUNCTION__, __LINE__);
			return -1;
		}
		else
		{
			while (fscanf(f_start, "%d\n", &ifIdx) != EOF)
			{
				printf("[%s@%d] ifIdx = %d\n", __FUNCTION__, __LINE__, ifIdx);
				for (i = 0; i < vcTotal; i++)
				{
					if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
						continue;

					if (Entry.enable == 0)
						continue;

					if (Entry.ifIndex == ifIdx)
					{
#if defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN)			
						addEthWANdev(&Entry);
#ifdef CONFIG_RTK_L34_ENABLE
						// Call RG_add_wan before addEthWANdev because addEthWANdev() will trigger link change events.
						RG_add_wan(&Entry, i);
#endif
#endif

#ifdef CONFIG_00R0
						if (Entry.applicationtype & X_CT_SRV_VOICE) {  //For small bandwidth
							hasVoIPWan = 1; 
			
							printf("Have VoIP WAN!!\n");
							enlargeBW(0);
						}
#endif

						ret |= startConnection(&Entry, i);

#if defined(CONFIG_ATM_CLIP)&&defined(CONFIG_DEV_xDSL)
#ifdef CONFIG_DEV_xDSL
						if (adsl_drv_get(RLCM_GET_LINK_SPEED, (void *)&vLs, RLCM_GET_LINK_SPEED_SIZE) && vLs.upstreamRate != 0) {
#endif
							if (Entry.cmode == CHANNEL_MODE_RT1577) {
								struct data_to_pass_st msg;
								char wanif[16];
								unsigned long addr = *((unsigned long *)Entry.remoteIpAddr);
								ifGetName(Entry.ifIndex, wanif, sizeof(wanif));
								snprintf(msg.data, BUF_SIZE, "mpoactl set %s inarprep %lu", wanif, addr);
								write_to_mpoad(&msg);
							}
#ifdef CONFIG_DEV_xDSL
						}
#endif
#endif
						break;
					}
				}
			}
			fclose(f_start);
		}
	}
	else
	{
		for (i = 0; i < vcTotal; i++)
		{
			/* get the specified chain record */
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				return -1;

			if(!isInterfaceMatch(Entry.ifIndex))  // Magician: Only raise interfaces that was set by wan mode.
				continue;

			if (Entry.enable == 0)
				continue;
#ifdef CONFIG_00R0
			if(Entry.applicationtype & X_CT_SRV_VOICE) {  //For small bandwidth
				hasVoIPWan = 1; 

				printf("Have VoIP WAN!!\n");
				enlargeBW(0);
			}
#endif

			if (configAll == CONFIGALL) 		// config for ALL
				ret|=startConnection(&Entry, i);
			else	 if (configAll == CONFIGONE)	// config for one
			{
				if (pEntry != NULL) {
					if ( Entry.ifIndex == pEntry->ifIndex) {
						ret|=startConnection(&Entry, i);
					}
					else
						continue;
				}
				//else
				//	break;		// If pEntry == NULL, It mean we want to delete this channel.
			}

#if defined(CONFIG_ATM_CLIP)&&defined(CONFIG_DEV_xDSL)
#ifdef CONFIG_DEV_xDSL
			if (adsl_drv_get(RLCM_GET_LINK_SPEED, (void *)&vLs, RLCM_GET_LINK_SPEED_SIZE) && vLs.upstreamRate != 0) {
#endif
				if (Entry.cmode == CHANNEL_MODE_RT1577) {
					struct data_to_pass_st msg;
					char wanif[16];
					unsigned long addr = *((unsigned long *)Entry.remoteIpAddr);
					ifGetName(Entry.ifIndex, wanif, sizeof(wanif));
					snprintf(msg.data, BUF_SIZE, "mpoactl set %s inarprep %lu", wanif, addr);
					write_to_mpoad(&msg);
				}
#ifdef CONFIG_DEV_xDSL
			}
#endif
#endif
			//if (configAll == CONFIGONE)
			//	break;
		}
	}

	startSNAT();
	for (i = 0; i < vcTotal; i++) /* Check have any ROUTING WAN, IulianWu*/
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;
		if (Entry.enable == 0)
			continue;
		if ((CHANNEL_MODE_T)Entry.cmode != CHANNEL_MODE_BRIDGE)
			ipEnabled = 1;
	}

	ret = startWanServiceDependency(ipEnabled);

#ifdef CONFIG_00R0
	if(hasVoIPWan==0)
		enlargeBW(1);
#endif
#if ! defined(_LINUX_2_6_) && defined(CONFIG_RTL_MULTI_WAN)
	checkWanModeStatus();
#endif
	return ret;
}

#ifdef _CWMP_MIB_ /*jiunming, mib for cwmp-tr069*/
int startCWMP(void)
{
	char vChar=0;
	char strPort[16];
	unsigned int conreq_port=0;
	unsigned char cwmp_aclEnable;
	
	//lan interface enable or disable
	mib_get(CWMP_LAN_IPIFENABLE, (void *)&vChar);
	if(vChar==0)
	{
		va_cmd(IFCONFIG, 2, 1, BRIF, "down");
		printf("Disable br0 interface\n");
	}
	//eth0 interface enable or disable
	mib_get(CWMP_LAN_ETHIFENABLE, (void *)&vChar);
	if(vChar==0)
	{
		va_cmd(IFCONFIG, 2, 1, ELANIF, "down");
		printf("Disable eth0 interface\n");
	}

#if 0
#if defined(REMOTE_ACCESS_CTL) || defined(IP_ACL)
	mib_get(CWMP_ACL_ENABLE, (void *)&cwmp_aclEnable);
	if (cwmp_aclEnable == 0) { // ACL Capability is disabled
		/*add a wan port to pass */
		mib_get(CWMP_CONREQ_PORT, (void *)&conreq_port);
		if(conreq_port==0) conreq_port=7547;
		sprintf(strPort, "%u", conreq_port );
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", (char *)FW_ADD, (char *)FW_CWMP_ACL,
			"!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
	}
#endif
#endif

	set_TR069_Firewall(1);

#ifdef CONFIG_RTK_L34_ENABLE
	mib_get(MIB_DMZ_ENABLE, &vChar);
	if(vChar)
	{
		Flush_RTK_RG_gatewayService();
		RTK_RG_gatewayService_add();
	}
#endif

	/*start the cwmpClient program*/
	mib_get(CWMP_FLAG, (void *)&vChar);
	if( vChar&CWMP_FLAG_AUTORUN )
	{
		va_cmd( "/bin/cwmpClient", 0, 0 );
	}

	return 0;
}
#endif

int getOUIfromMAC(char* oui)
{
	unsigned char macAddr[MAC_ADDR_LEN];

	if (oui==NULL)
		return -1;
	
	mib_get( MIB_ELAN_MAC_ADDR, (void *)macAddr);
	sprintf(oui,"%02X%02X%02X",macAddr[0],macAddr[1],macAddr[2]);
	return 0;
}

#ifdef CONFIG_DEV_xDSL
static char open_adsl_drv(char *dev_name)
{
	int ret = 1;
	struct id_info *id_info_r;

	id_info_r = id_info_search(dev_name);

	if(id_info_r->file_ptr != NULL){
		printf("Error : file fd already in use.\n");
		ret = 0;
		return ret;
	}

	if ((id_info_r->file_ptr = fopen(/*adslDevice*/dev_name, "r")) == NULL) {
//		printf("ERROR: failed to open %s, error(%s)\n", /*adslDevice*/dev_name,
//				strerror(errno));
			ret = 0;
			return ret;
	}
	return ret;
}

static void close_adsl_drv(char *dev_name)
{
	struct id_info *id_info_r = NULL;

	id_info_r = id_info_search(dev_name);

	if (id_info_r->file_ptr) {
		fclose(id_info_r->file_ptr);
		id_info_r->file_ptr = NULL;
	}
}


char _adsl_drv_get(char *dev_name ,unsigned int id, void *rValue, unsigned int len)
{
	int ret = 0;
	struct id_info *id_info_r = NULL;
	id_info_r = id_info_search(dev_name);

#ifdef EMBED
	if (open_adsl_drv(dev_name)) {
		obcif_arg myarg;
		myarg.argsize = (int)len;
		myarg.arg = (int)rValue;

		if (ioctl(fileno(id_info_r->file_ptr), id, &myarg) >= 0)
			ret = 1;

		close_adsl_drv(dev_name);

		return ret;
	}
#endif
	return ret;
}


char adsl_drv_get(unsigned int id, void *rValue, unsigned int len)
{
#ifdef CONFIG_USER_CMD_CLIENT_SIDE
	return commserv_getxDSL(id, rValue, len);
#else
	return _adsl_drv_get(ADSL_DFAULT_DEV, id, rValue, len);
#endif
}

#ifdef CONFIG_VDSL
/*pval: must be an int[4]-arrary pointer*/
static char dsl_msg(unsigned int id, int msg, int *pval)
{
	MSGTODSL msg2dsl;
	char ret=0;

	msg2dsl.message=msg;
	msg2dsl.intVal=pval;
	ret=adsl_drv_get(id, &msg2dsl, sizeof(MSGTODSL));

	return ret;
}

char dsl_msg_set_array(int msg, int *pval)
{
	char ret=0;

	if(pval)
	{
#ifdef CONFIG_USER_CMD_CLIENT_SIDE
		ret = commserv_setVDSL(msg, (uint32_t) *pval);
#else
		ret=dsl_msg(RLCM_UserSetDslData, msg, pval);
#endif
	}
	return ret;
}

char dsl_msg_set(int msg, int val)
{
	int tmpint[4];
	char ret=0;

	tmpint[0]=val;
	ret=dsl_msg_set_array(msg, tmpint);
	return ret;
}

char dsl_msg_get_array(int msg, int *pval)
{
	char ret=0;

	if(pval)
	{
#ifdef CONFIG_USER_CMD_CLIENT_SIDE
		ret = commserv_getVDSL(msg, (void *) pval);
#else
		ret=dsl_msg(RLCM_UserGetDslData, msg, pval);
#endif
	}
	return ret;
}

char dsl_msg_get(int msg, int *pval)
{
	int tmpint[4];
	char ret=0;

	if(pval)
	{
		memset(tmpint, 0, sizeof(tmpint));
		ret=dsl_msg_get_array(msg, tmpint);
		if(ret) *pval=tmpint[0];
	}
	return ret;
}
#endif /*#CONFIG_VDSL*/

struct id_info id_name_mapping[] = {
	{0,ADSL_DFAULT_DEV,NULL},
	{1,XDSL_SLAVE_DEV,NULL}
};

struct id_info* id_info_search(char *dev_name){
	int i;
	struct id_info *ret = NULL;

	for(i=0;i<(sizeof(id_name_mapping) / sizeof(struct id_info));i++){
		if(strcmp(id_name_mapping[i].dev_name, dev_name) == 0){
			ret = &id_name_mapping[i];
			break;
		}
	}

	return ret;
}


static XDSL_OP xdsl0_op =
{
	0,
	adsl_drv_get,
#ifdef CONFIG_VDSL
	dsl_msg_set_array,
	dsl_msg_get_array,
	dsl_msg_set,
	dsl_msg_get,
#endif /*CONFIG_VDSL*/
	(int  (*)(int, char *, int))getAdslInfo,
};

#ifdef CONFIG_USER_XDSL_SLAVE
static XDSL_OP xdsl1_op =
{
	1,
	adsl_slv_drv_get,
#ifdef CONFIG_VDSL
	dsl_slv_msg_set_array,
	dsl_slv_msg_get_array,
	dsl_slv_msg_set,
	dsl_slv_msg_get,
#endif /*CONFIG_VDSL*/
	getAdslSlvInfo,
};
#endif /*CONFIG_USER_XDSL_SLAVE*/

static XDSL_OP *xdsl_op[]=
{
	&xdsl0_op,
#ifdef CONFIG_USER_XDSL_SLAVE
	&xdsl1_op
#endif /*CONFIG_USER_XDSL_SLAVE*/
};

XDSL_OP *xdsl_get_op(int id)
{
#ifdef CONFIG_USER_XDSL_SLAVE
	if(id)
	{
		if(id!=1) printf( "%s: error id=%d\n", __FUNCTION__, id );
		return xdsl_op[1];
	}
#endif /*CONFIG_USER_XDSL_SLAVE*/

	if(id!=0) printf( "%s: error id=%d\n", __FUNCTION__, id );
	return xdsl_op[0];
}

#ifdef CONFIG_DSL_VTUO
static void VTUOArrayDump(char *s, int *a, int size)
{
	int i;
	printf("\nDump %s array, size=%d\n", s?s:"", size );
	for(i=0; i<size; i++)
		printf( "  %02d: %d\n", i, a[i] );
	return;
}

int VTUOSetupChan(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	int tmp2dsl[6];
	unsigned int tmpUInt;
	unsigned char tmpUChar;

	mib_get(VTUO_CHAN_DS_NDR_MAX, (void *)&tmpUInt);
	tmp2dsl[0]=tmpUInt;
	mib_get(VTUO_CHAN_DS_NDR_MIN, (void *)&tmpUInt);
	tmp2dsl[1]=tmpUInt;
	mib_get(VTUO_CHAN_DS_MAX_DELAY, (void *)&tmpUChar);
	tmp2dsl[2]=tmpUChar;
	mib_get(VTUO_CHAN_DS_MIN_INP, (void *)&tmpUChar);
	if(tmpUChar==17) tmpUInt=5;
	else tmpUInt=tmpUChar*10;
	tmp2dsl[3]= tmpUInt;
	mib_get(VTUO_CHAN_DS_MIN_INP8, (void *)&tmpUChar);
	tmp2dsl[4]=tmpUChar*10;
	mib_get(VTUO_CHAN_DS_SOS_MDR, (void *)&tmpUInt);
	tmp2dsl[5]=tmpUInt;
	VTUOArrayDump( "SetChnProfDs", tmp2dsl, 6 );
	d->xdsl_msg_set_array( SetChnProfDs, tmp2dsl );

	mib_get(VTUO_CHAN_US_NDR_MAX, (void *)&tmpUInt);
	tmp2dsl[0]=tmpUInt;
	mib_get(VTUO_CHAN_US_NDR_MIN, (void *)&tmpUInt);
	tmp2dsl[1]=tmpUInt;
	mib_get(VTUO_CHAN_US_MAX_DELAY, (void *)&tmpUChar);
	tmp2dsl[2]=tmpUChar;
	mib_get(VTUO_CHAN_US_MIN_INP, (void *)&tmpUChar);
	if(tmpUChar==17) tmpUInt=5;
	else tmpUInt=tmpUChar*10;
	tmp2dsl[3]= tmpUInt;
	mib_get(VTUO_CHAN_US_MIN_INP8, (void *)&tmpUChar);
	tmp2dsl[4]=tmpUChar*10;
	mib_get(VTUO_CHAN_US_SOS_MDR, (void *)&tmpUInt);
	tmp2dsl[5]=tmpUInt;
	VTUOArrayDump( "SetChnProfUs", tmp2dsl, 6 );
	d->xdsl_msg_set_array( SetChnProfUs, tmp2dsl );
	return 0;
}

int VTUOSetupGinp(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	int tmp2dsl[11];
	unsigned int tmpUInt;
	unsigned short tmpUShort;
	unsigned char tmpUChar;

	mib_get(VTUO_GINP_DS_MODE, (void *)&tmpUChar);
	tmp2dsl[0]=tmpUChar;
	mib_get(VTUO_GINP_DS_ET_MAX, (void *)&tmpUInt);
	tmp2dsl[1]=tmpUInt;
	mib_get(VTUO_GINP_DS_ET_MIN, (void *)&tmpUInt);
	tmp2dsl[2]=tmpUInt;
	mib_get(VTUO_GINP_DS_MAX_DELAY, (void *)&tmpUChar);
	tmp2dsl[3]=tmpUChar;
	mib_get(VTUO_GINP_DS_MIN_DELAY, (void *)&tmpUChar);
	tmp2dsl[4]=tmpUChar;
	mib_get(VTUO_GINP_DS_MIN_INP, (void *)&tmpUChar);
	tmp2dsl[5]=tmpUChar*10;
	mib_get(VTUO_GINP_DS_REIN_SYM, (void *)&tmpUChar);
	tmp2dsl[6]=tmpUChar*10;
	mib_get(VTUO_GINP_DS_REIN_FREQ, (void *)&tmpUChar);
	tmp2dsl[7]=tmpUChar;
	mib_get(VTUO_GINP_DS_SHINE_RATIO, (void *)&tmpUShort);
	tmp2dsl[8]=tmpUShort;
	mib_get(VTUO_GINP_DS_LEFTR_THRD, (void *)&tmpUChar);
	tmp2dsl[9]=tmpUChar;
	mib_get(VTUO_GINP_DS_NDR_MAX, (void *)&tmpUInt);
	tmp2dsl[10]=tmpUInt;
	VTUOArrayDump( "SetChnProfGinpDs", tmp2dsl, 11 );
	d->xdsl_msg_set_array( SetChnProfGinpDs, tmp2dsl );

	mib_get(VTUO_GINP_US_MODE, (void *)&tmpUChar);
	tmp2dsl[0]=tmpUChar;
	mib_get(VTUO_GINP_US_ET_MAX, (void *)&tmpUInt);
	tmp2dsl[1]=tmpUInt;
	mib_get(VTUO_GINP_US_ET_MIN, (void *)&tmpUInt);
	tmp2dsl[2]=tmpUInt;
	mib_get(VTUO_GINP_US_MAX_DELAY, (void *)&tmpUChar);
	tmp2dsl[3]=tmpUChar;
	mib_get(VTUO_GINP_US_MIN_DELAY, (void *)&tmpUChar);
	tmp2dsl[4]=tmpUChar;
	mib_get(VTUO_GINP_US_MIN_INP, (void *)&tmpUChar);
	tmp2dsl[5]=tmpUChar*10;
	mib_get(VTUO_GINP_US_REIN_SYM, (void *)&tmpUChar);
	tmp2dsl[6]=tmpUChar*10;
	mib_get(VTUO_GINP_US_REIN_FREQ, (void *)&tmpUChar);
	tmp2dsl[7]=tmpUChar;
	mib_get(VTUO_GINP_US_SHINE_RATIO, (void *)&tmpUShort);
	tmp2dsl[8]=tmpUShort;
	mib_get(VTUO_GINP_US_LEFTR_THRD, (void *)&tmpUChar);
	tmp2dsl[9]=tmpUChar;
	mib_get(VTUO_GINP_US_NDR_MAX, (void *)&tmpUInt);
	tmp2dsl[10]=tmpUInt;
	VTUOArrayDump( "SetChnProfGinpUs", tmp2dsl, 11 );
	d->xdsl_msg_set_array( SetChnProfGinpUs, tmp2dsl );
	return 0;
}

int VTUOSetupLine(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	int tmp2dsl[10];
	unsigned int tmpUInt;
	unsigned short tmpUShort;
	unsigned char tmpUChar;
	short tmpShort;


	mib_get(VTUO_LINE_DS_MAX_SNR, (void *)&tmpShort);
	tmp2dsl[0]=tmpShort;
	mib_get(VTUO_LINE_DS_TARGET_SNR, (void *)&tmpShort);
	tmp2dsl[1]=tmpShort;
	mib_get(VTUO_LINE_DS_MIN_SNR, (void *)&tmpShort);
	tmp2dsl[2]=tmpShort;
	/*
	mib_get(VTUO_LINE_DS_MAX_SNR_NOLMT, (void *)&tmpUChar);
	tmp2dsl[3]=tmpUChar;
	*/
	VTUOArrayDump( "SetLineMarginDs", tmp2dsl, 3 );
	d->xdsl_msg_set_array( SetLineMarginDs, tmp2dsl );

	mib_get(VTUO_LINE_DS_BITSWAP, (void *)&tmpUChar);
	printf("SetLineBSDs=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineBSDs, (unsigned int)tmpUChar );

	/*
	mib_get(VTUO_LINE_DS_MAX_TXPWR, (void *)&tmpShort);
	tmp2dsl[0]=0;
	tmp2dsl[1]=tmpShort;
	tmp2dsl[2]=0;
	VTUOArrayDump( "SetLinePowerDs", tmp2dsl, 3 );
	d->xdsl_msg_set_array( SetLinePowerDs, tmp2dsl );

	mib_get(VTUO_LINE_DS_MIN_OH_RATE, (void *)&tmpUShort);
	printf("SetOHrateDs=%u\n", (unsigned int)tmpUShort);
	d->xdsl_msg_set( SetOHrateDs, (unsigned int)tmpUShort );
	*/


	mib_get(VTUO_LINE_US_MAX_SNR, (void *)&tmpShort);
	tmp2dsl[0]=tmpShort;
	mib_get(VTUO_LINE_US_TARGET_SNR, (void *)&tmpShort);
	tmp2dsl[1]=tmpShort;
	mib_get(VTUO_LINE_US_MIN_SNR, (void *)&tmpShort);
	tmp2dsl[2]=tmpShort;
	/*
	mib_get(VTUO_LINE_US_MAX_SNR_NOLMT, (void *)&tmpUChar);
	tmp2dsl[3]=tmpUChar;
	*/
	VTUOArrayDump( "SetLineMarginUs", tmp2dsl, 3 );
	d->xdsl_msg_set_array( SetLineMarginUs, tmp2dsl );

	mib_get(VTUO_LINE_US_BITSWAP, (void *)&tmpUChar);
	printf("SetLineBSUs=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineBSUs, (unsigned int)tmpUChar );
	/*
	mib_get(VTUO_LINE_US_MAX_RXPWR, (void *)&tmpShort);
	tmp2dsl[0]=tmpShort;
	mib_get(VTUO_LINE_US_MAX_TXPWR, (void *)&tmpShort);
	tmp2dsl[1]=tmpShort;
	mib_get(VTUO_LINE_US_MAX_RXPWR_NOLMT, (void *)&tmpUChar);
	tmp2dsl[2]=tmpUChar;
	*/
	VTUOArrayDump( "SetLinePowerUs", tmp2dsl, 3 );
	d->xdsl_msg_set_array( SetLinePowerUs, tmp2dsl );
	/*
	mib_get(VTUO_LINE_US_MIN_OH_RATE, (void *)&tmpUShort);
	printf("SetOHrateUs=%u\n", (unsigned int)tmpUShort);
	d->xdsl_msg_set( SetOHrateUs, (unsigned int)tmpUShort );


	mib_get(VTUO_LINE_TRANS_MODE, (void *)&tmpUChar);
	printf("SetLineTxMode=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineTxMode, (unsigned int)tmpUChar );
	*/

	mib_get(VTUO_LINE_ADSL_PROTOCOL, (void *)&tmpUInt);
	mib_get(VTUO_LINE_VDSL2_PROFILE, (void *)&tmpUShort);
	if (tmpUShort != 0)
		tmpUInt |= MODE_VDSL2;
	tmpUInt |= MODE_ANX_A;
	printf("SetPmdMode=0x%08x\n", (unsigned int)tmpUInt);
	d->xdsl_msg_set( SetPmdMode, (unsigned int)tmpUInt );

	mib_get(VTUO_LINE_VDSL2_PROFILE, (void *)&tmpUShort);
	printf("SetVdslProfile=0x%08x\n", (unsigned int)tmpUShort);
	d->xdsl_msg_set(SetVdslProfile, (unsigned int)tmpUShort);

	/*
	mib_get(VTUO_LINE_CLASS_MASK, (void *)&tmpUChar);
	printf("SetLineClassMask=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineClassMask, (unsigned int)tmpUChar );
	*/

	mib_get(VTUO_LINE_LIMIT_MASK, (void *)&tmpUChar);
	printf("SetLineLimitMask=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineLimitMask, (unsigned int)tmpUChar );

	mib_get(VTUO_LINE_US0_MASK, (void *)&tmpUChar);
	printf("SetLineUs0tMask=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineUs0tMask, (unsigned int)tmpUChar );

	mib_get(VTUO_LINE_UPBO_ENABLE, (void *)&tmpUChar);
	tmp2dsl[0]=tmpUChar;
	mib_get(VTUO_LINE_UPBOKL, (void *)&tmpShort);
	tmp2dsl[1]=tmpShort;
	mib_get(VTUO_LINE_UPBO_1A, (void *)&tmpShort);
	tmp2dsl[2]=tmpShort;
	mib_get(VTUO_LINE_UPBO_2A, (void *)&tmpShort);
	tmp2dsl[3]=tmpShort;
	mib_get(VTUO_LINE_UPBO_3A, (void *)&tmpShort);
	tmp2dsl[4]=tmpShort;
	mib_get(VTUO_LINE_UPBO_4A, (void *)&tmpShort);
	tmp2dsl[5]=tmpShort;
	mib_get(VTUO_LINE_UPBO_1B, (void *)&tmpShort);
	tmp2dsl[6]=tmpShort;
	mib_get(VTUO_LINE_UPBO_2B, (void *)&tmpShort);
	tmp2dsl[7]=tmpShort;
	mib_get(VTUO_LINE_UPBO_3B, (void *)&tmpShort);
	tmp2dsl[8]=tmpShort;
	mib_get(VTUO_LINE_UPBO_4B, (void *)&tmpShort);
	tmp2dsl[9]=tmpShort;
	VTUOArrayDump( "SetLineUPBO", tmp2dsl, 10 );
	d->xdsl_msg_set_array( SetLineUPBO, tmp2dsl );
	/*
	mib_get(VTUO_LINE_RT_MODE, (void *)&tmpUChar);
	printf("SetLineRTMode=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineRTMode, (unsigned int)tmpUChar );
	*/
	mib_get(VTUO_LINE_US0_ENABLE, (void *)&tmpUChar);
	printf("SetLineUS0=%u\n", (unsigned int)tmpUChar);
	d->xdsl_msg_set( SetLineUS0, (unsigned int)tmpUChar );
	return 0;
}



int VTUOSetupInm(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	int tmp2dsl[5];
	short tmpShort;
	unsigned char tmpUChar;

	mib_get(VTUO_INM_NE_INP_EQ_MODE, (void *)&tmpUChar);
	tmp2dsl[0]=tmpUChar;
	mib_get(VTUO_INM_NE_INMCC, (void *)&tmpUChar);
	tmp2dsl[1]=tmpUChar;
	mib_get(VTUO_INM_NE_IAT_OFFSET, (void *)&tmpUChar);
	tmp2dsl[2]=tmpUChar;
	mib_get(VTUO_INM_NE_IAT_SETUP, (void *)&tmpUChar);
	tmp2dsl[3]=tmpUChar;
	/*
	mib_get(VTUO_INM_NE_ISDD_SEN, (void *)&tmpShort);
	tmp2dsl[4]=tmpShort;
	*/
	VTUOArrayDump( "SetInmNE", tmp2dsl, 4 );
	d->xdsl_msg_set_array( SetInmNE, tmp2dsl );

	mib_get(VTUO_INM_FE_INP_EQ_MODE, (void *)&tmpUChar);
	tmp2dsl[0]=tmpUChar;
	mib_get(VTUO_INM_FE_INMCC, (void *)&tmpUChar);
	tmp2dsl[1]=tmpUChar;
	mib_get(VTUO_INM_FE_IAT_OFFSET, (void *)&tmpUChar);
	tmp2dsl[2]=tmpUChar;
	mib_get(VTUO_INM_FE_IAT_SETUP, (void *)&tmpUChar);
	tmp2dsl[3]=tmpUChar;
	/*
	mib_get(VTUO_INM_FE_ISDD_SEN, (void *)&tmpShort);
	tmp2dsl[4]=tmpShort;
	*/
	VTUOArrayDump( "SetInmFE", tmp2dsl, 4 );
	d->xdsl_msg_set_array( SetInmFE, tmp2dsl );
	return 0;
}


int VTUOSetupSra(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	int tmp2dsl[6];
	unsigned char tmpUChar;
	unsigned short tmpUShort;
	short tmpShort;


	mib_get(VTUO_SRA_DS_RA_MODE, (void *)&tmpUChar);
	tmp2dsl[0]=tmpUChar;
	mib_get(VTUO_SRA_DS_DYNAMIC_DEPTH, (void *)&tmpUChar);
	tmp2dsl[1]=tmpUChar;
	mib_get(VTUO_SRA_DS_USHIFT_SNR, (void *)&tmpShort);
	tmp2dsl[2]=tmpShort;
	mib_get(VTUO_SRA_DS_DSHIFT_SNR, (void *)&tmpShort);
	tmp2dsl[3]=tmpShort;
	mib_get(VTUO_SRA_DS_USHIFT_TIME, (void *)&tmpUShort);
	tmp2dsl[4]=tmpUShort;
	mib_get(VTUO_SRA_DS_DSHIFT_TIME, (void *)&tmpUShort);
	tmp2dsl[5]=tmpUShort;
	VTUOArrayDump( "SetLineRADs", tmp2dsl, 6 );
	d->xdsl_msg_set_array( SetLineRADs, tmp2dsl );

	mib_get(VTUO_SRA_DS_RA_MODE, (void *)&tmpUChar);
	tmp2dsl[0]=(tmpUChar==3)?1:0;
	mib_get(VTUO_SRA_DS_SOS_TIME, (void *)&tmpUShort);
	tmp2dsl[1]=tmpUShort;
	mib_get(VTUO_SRA_DS_SOS_CRC, (void *)&tmpUShort);
	tmp2dsl[2]=tmpUShort;
	mib_get(VTUO_SRA_DS_SOS_NTONE, (void *)&tmpUChar);
	tmp2dsl[3]=tmpUChar;
	mib_get(VTUO_SRA_DS_SOS_MAX, (void *)&tmpUChar);
	tmp2dsl[4]=tmpUChar;
	/*
	mib_get(VTUO_SRA_DS_SOS_MSTEP_TONE, (void *)&tmpUChar);
	tmp2dsl[5]=tmpUChar;
	*/
	VTUOArrayDump( "SetLineSOSDs", tmp2dsl, 5 );
	d->xdsl_msg_set_array( SetLineSOSDs, tmp2dsl );

	mib_get(VTUO_SRA_DS_ROC_ENABLE, (void *)&tmpUChar);
	tmp2dsl[0]=tmpUChar;
	mib_get(VTUO_SRA_DS_ROC_SNR, (void *)&tmpShort);
	tmp2dsl[1]=tmpShort;
	mib_get(VTUO_SRA_DS_ROC_MIN_INP, (void *)&tmpUChar);
	tmp2dsl[2]=tmpUChar;
	VTUOArrayDump( "SetLineROCDs", tmp2dsl, 3 );
	d->xdsl_msg_set_array( SetLineROCDs, tmp2dsl );



	mib_get(VTUO_SRA_US_RA_MODE, (void *)&tmpUChar);
	tmp2dsl[0]=tmpUChar;
	mib_get(VTUO_SRA_US_DYNAMIC_DEPTH, (void *)&tmpUChar);
	tmp2dsl[1]=tmpUChar;
	mib_get(VTUO_SRA_US_USHIFT_SNR, (void *)&tmpShort);
	tmp2dsl[2]=tmpShort;
	mib_get(VTUO_SRA_US_DSHIFT_SNR, (void *)&tmpShort);
	tmp2dsl[3]=tmpShort;
	mib_get(VTUO_SRA_US_USHIFT_TIME, (void *)&tmpUShort);
	tmp2dsl[4]=tmpUShort;
	mib_get(VTUO_SRA_US_DSHIFT_TIME, (void *)&tmpUShort);
	tmp2dsl[5]=tmpUShort;
	VTUOArrayDump( "SetLineRAUs", tmp2dsl, 6 );
	d->xdsl_msg_set_array( SetLineRAUs, tmp2dsl );

	mib_get(VTUO_SRA_US_RA_MODE, (void *)&tmpUChar);
	tmp2dsl[0]=(tmpUChar==3)?1:0;
	mib_get(VTUO_SRA_US_SOS_TIME, (void *)&tmpUShort);
	tmp2dsl[1]=tmpUShort;
	mib_get(VTUO_SRA_US_SOS_CRC, (void *)&tmpUShort);
	tmp2dsl[2]=tmpUShort;
	mib_get(VTUO_SRA_US_SOS_NTONE, (void *)&tmpUChar);
	tmp2dsl[3]=tmpUChar;
	mib_get(VTUO_SRA_US_SOS_MAX, (void *)&tmpUChar);
	tmp2dsl[4]=tmpUChar;
	/*
	mib_get(VTUO_SRA_US_SOS_MSTEP_TONE, (void *)&tmpUChar);
	tmp2dsl[5]=tmpUChar;
	*/
	VTUOArrayDump( "SetLineSOSUs", tmp2dsl, 5 );
	d->xdsl_msg_set_array( SetLineSOSUs, tmp2dsl );

	mib_get(VTUO_SRA_US_ROC_ENABLE, (void *)&tmpUChar);
	tmp2dsl[0]=tmpUChar;
	mib_get(VTUO_SRA_US_ROC_SNR, (void *)&tmpShort);
	tmp2dsl[1]=tmpShort;
	mib_get(VTUO_SRA_US_ROC_MIN_INP, (void *)&tmpUChar);
	tmp2dsl[2]=tmpUChar;
	VTUOArrayDump( "SetLineROCUs", tmp2dsl, 3 );
	d->xdsl_msg_set_array( SetLineROCUs, tmp2dsl );

	return 0;
}

int VTUOSetupDpbo(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	int tmp2dsl[8];
	unsigned char tmpUChar;
	unsigned short tmpUShort;
	short tmpShort;
	int tmpInt;

	mib_get(VTUO_DPBO_ENABLE, (void *)&tmpUChar);
	tmp2dsl[0]=tmpUChar;
	mib_get(VTUO_DPBO_ESEL, (void *)&tmpShort);
	tmp2dsl[1]=tmpShort;
	mib_get(VTUO_DPBO_ESCMA, (void *)&tmpInt);
	tmp2dsl[2]=tmpInt;
	mib_get(VTUO_DPBO_ESCMB, (void *)&tmpInt);
	tmp2dsl[3]=tmpInt;
	mib_get(VTUO_DPBO_ESCMC, (void *)&tmpInt);
	tmp2dsl[4]=tmpInt;
	mib_get(VTUO_DPBO_MUS, (void *)&tmpShort);
	tmp2dsl[5]=tmpShort;
	mib_get(VTUO_DPBO_FMIN, (void *)&tmpUShort);
	tmp2dsl[6]=tmpUShort;
	mib_get(VTUO_DPBO_FMAX, (void *)&tmpUShort);
	tmp2dsl[7]=tmpUShort;
	VTUOArrayDump( "SetLineDPBO", tmp2dsl, 8 );
	d->xdsl_msg_set_array( SetLineDPBO, tmp2dsl );

{
	MIB_CE_VTUO_DPBO_T entry, *p=&entry;
	int num, i;
	int *ptmp2dsl, int_size;

	num=mib_chain_total( MIB_VTUO_DPBO_TBL );
	int_size= 2+num*2;
	ptmp2dsl=malloc( int_size*sizeof(int) );
	if(!ptmp2dsl) return -1;
	memset( ptmp2dsl, 0, int_size*sizeof(int) );

	ptmp2dsl[0]= num?1:0;
	ptmp2dsl[1]= num;
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_DPBO_TBL, i, p))
			continue;
		ptmp2dsl[i*2+2]= p->ToneId;
		ptmp2dsl[i*2+3]= p->PsdLevel;
	}
	VTUOArrayDump( "SetLineDPBOPSD", ptmp2dsl, int_size );
	d->xdsl_msg_set_array( SetLineDPBOPSD, ptmp2dsl );
	free(ptmp2dsl);
}

	return 0;
}

int VTUOSetupPsd(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	MIB_CE_VTUO_PSD_T entry, *p=&entry;
	int num, i;
	int *ptmp2dsl, int_size;

	/*DS*/
	num=mib_chain_total( MIB_VTUO_PSD_DS_TBL );
	int_size= 2+num*2;
	ptmp2dsl=malloc( int_size*sizeof(int) );
	if(!ptmp2dsl) return -1;
	memset( ptmp2dsl, 0, int_size*sizeof(int) );

	ptmp2dsl[0]= num?1:0;
	ptmp2dsl[1]= num;
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_PSD_DS_TBL, i, p))
			continue;
		ptmp2dsl[i*2+2]= p->ToneId;
		ptmp2dsl[i*2+3]= p->PsdLevel;
	}
	VTUOArrayDump( "SetLinePSDMIBDs", ptmp2dsl, int_size );
	d->xdsl_msg_set_array( SetLinePSDMIBDs, ptmp2dsl );
	free(ptmp2dsl);

	/*US*/
	num=mib_chain_total( MIB_VTUO_PSD_US_TBL );
	int_size= 2+num*2;
	ptmp2dsl=malloc( int_size*sizeof(int) );
	if(!ptmp2dsl) return -1;
	memset( ptmp2dsl, 0, int_size*sizeof(int) );

	ptmp2dsl[0]= num?1:0;
	ptmp2dsl[1]= num;
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_PSD_US_TBL, i, p))
			continue;
		ptmp2dsl[i*2+2]= p->ToneId;
		ptmp2dsl[i*2+3]= p->PsdLevel;
	}
	VTUOArrayDump( "SetLinePSDMIBUs", ptmp2dsl, int_size );
	d->xdsl_msg_set_array( SetLinePSDMIBUs, ptmp2dsl );
	free(ptmp2dsl);

	return 0;
}
/*
int VTUOSetupVn(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	MIB_CE_VTUO_VN_T entry, *p=&entry;
	int num, i;
	int *ptmp2dsl, int_size;
	unsigned char tmpUChar;

	// DS
	num=mib_chain_total( MIB_VTUO_VN_DS_TBL );
	int_size= 2+num*2;
	ptmp2dsl=malloc( int_size*sizeof(int) );
	if(!ptmp2dsl) return -1;
	memset( ptmp2dsl, 0, int_size*sizeof(int) );

	mib_get(VTUO_VN_DS_ENABLE, (void *)&tmpUChar);
	ptmp2dsl[0]= tmpUChar?1:0;
	ptmp2dsl[1]= num;
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_VN_DS_TBL, i, p))
			continue;
		ptmp2dsl[i*2+2]= p->ToneId;
		ptmp2dsl[i*2+3]= p->NoiseLevel;
	}
	VTUOArrayDump( "SetLineVNDs", ptmp2dsl, int_size );
	d->xdsl_msg_set_array( SetLineVNDs, ptmp2dsl );
	free(ptmp2dsl);

	// US
	num=mib_chain_total( MIB_VTUO_VN_US_TBL );
	int_size= 2+num*2;
	ptmp2dsl=malloc( int_size*sizeof(int) );
	if(!ptmp2dsl) return -1;
	memset( ptmp2dsl, 0, int_size*sizeof(int) );

	mib_get(VTUO_VN_US_ENABLE, (void *)&tmpUChar);
	ptmp2dsl[0]= tmpUChar?1:0;
	ptmp2dsl[1]= num;
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_VN_US_TBL, i, p))
			continue;
		ptmp2dsl[i*2+2]= p->ToneId;
		ptmp2dsl[i*2+3]= p->NoiseLevel;
	}
	VTUOArrayDump( "SetLineVNUs", ptmp2dsl, int_size );
	d->xdsl_msg_set_array( SetLineVNUs, ptmp2dsl );
	free(ptmp2dsl);

	return 0;
}

int VTUOSetupRfi(void)
{
	XDSL_OP *d=xdsl_get_op(0);
	MIB_CE_VTUO_RFI_T entry, *p=&entry;
	int num, i;
	int *ptmp2dsl, int_size;

	num=mib_chain_total( MIB_VTUO_RFI_TBL );
	int_size= 2+num*2;
	ptmp2dsl=malloc( int_size*sizeof(int) );
	if(!ptmp2dsl) return -1;
	memset( ptmp2dsl, 0, int_size*sizeof(int) );

	ptmp2dsl[0]= num?1:0;
	ptmp2dsl[1]= num;
	for(i=0; i<num; i++)
	{
		if (!mib_chain_get(MIB_VTUO_RFI_TBL, i, p))
			continue;
		ptmp2dsl[i*2+2]= p->ToneId;
		ptmp2dsl[i*2+3]= p->ToneIdEnd;
	}
	VTUOArrayDump( "SetLineRFI", ptmp2dsl, int_size );
	d->xdsl_msg_set_array( SetLineRFI, ptmp2dsl );
	free(ptmp2dsl);

	return 0;
}
*/
int VTUOCheck(void)
{
	int val=1;
	XDSL_OP *d=xdsl_get_op(0);

	d->xdsl_msg_get(GetDeviceType,&val);
	val=(val==0)?1:0; //0:VTUO, 1:VTUR
	//printf( "%s: return %d\n", __FUNCTION__, val );
	return val;
}

int VTUOSetup(void)
{
	if( VTUOCheck() )
	{
		VTUOSetupSra();
		VTUOSetupPsd();
		/*
		VTUOSetupVn();
		*/
		VTUOSetupDpbo();
		/*
		VTUOSetupRfi();
		*/
		VTUOSetupLine();

		VTUOSetupGinp();
		VTUOSetupChan();

		VTUOSetupInm();
	}
	return 0;
}
#endif /*CONFIG_DSL_VTUO*/

int setupDsl(void)
{
	char tone[64];
	unsigned char init_line, olr;
	unsigned char dsl_anxb;
	unsigned short dsl_mode;
	int val;
	int ret=1;
	int tmp2dsl[10];

	// check INIT_LINE
	if (mib_get(MIB_INIT_LINE, (void *)&init_line) != 0)
	{
		if (init_line == 1)
		{
#ifdef CONFIG_VDSL
			unsigned int mode;
			unsigned short profile;
			unsigned char gvector;

#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
			unsigned int GfastEnableOpt=0;	//disable

			//Step 1: disable G.fast modem
			printf("G.fast disable modem\n");
			commserv_setGFast(SetTest, &GfastEnableOpt);
#else
			//disable modem,20130203
			printf("xDSL disable modem\n");
			adsl_drv_get(RLCM_PHY_DISABLE_MODEM, NULL, 0);
#endif /* CONFIG_USER_CMD_SUPPORT_GFAST */

#ifdef CONFIG_DSL_VTUO
			VTUOSetup();
#else
			//Pmd mode
			mode=0;
			mib_get( MIB_DSL_ANNEX_B, (void *)&dsl_anxb);
			mib_get(MIB_ADSL_MODE, (void *)&dsl_mode);
			if(dsl_mode&ADSL_MODE_GDMT)		mode |= MODE_GDMT;
			if(dsl_mode&ADSL_MODE_GLITE)	mode |= MODE_GLITE;
			if(dsl_mode&ADSL_MODE_ADSL2)	mode |= MODE_ADSL2;
			if(dsl_mode&ADSL_MODE_ADSL2P)	mode |= MODE_ADSL2PLUS;
			if(dsl_mode&ADSL_MODE_VDSL2)	mode |= MODE_VDSL2;
#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
			if(dsl_mode&ADSL_MODE_GFAST)	mode |= MODE_GFAST;
#endif

			//if(dsl_mode&ADSL_MODE_ANXB)
			if (dsl_anxb)
			{
				// Annex B
				mode |= MODE_ANX_B;
				if(dsl_mode&ADSL_MODE_T1413)	mode |= MODE_ETSI;
				if(dsl_mode&ADSL_MODE_ANXJ)	mode |= MODE_ANX_J;
			}else{
				// Annex A
				mode |= MODE_ANX_A;
				if(dsl_mode&ADSL_MODE_T1413)	mode |= MODE_ANSI;
				if(dsl_mode&ADSL_MODE_ANXL)		mode |= MODE_ANX_L;
				if(dsl_mode&ADSL_MODE_ANXM)		mode |= MODE_ANX_M;
				if(dsl_mode&ADSL_MODE_ANXI)	mode |= MODE_ANX_I;
			}
			//printf("SetPmdMode=0x%08x (dsl_mode=0x%04x)\n",  mode, dsl_mode);
			dsl_msg_set(SetPmdMode, mode);

			//vdsl2 profile
			mib_get(MIB_VDSL2_PROFILE, (void *)&profile);
			//printf("SetVdslProfile=0x%08x\n", (unsigned int)profile);
			dsl_msg_set(SetVdslProfile, (unsigned int)profile);


			// G.INP,	 default by dsplib
#ifdef ENABLE_ADSL_MODE_GINP
			val=0;
			if (dsl_mode & ADSL_MODE_GINP)	// G.INP
				val=3;
			//printf("SetGInp=0x%08x\n", val);
			dsl_msg_set(SetGInp, val);
#endif /*ENABLE_ADSL_MODE_GINP*/

			// G.Vector
			mib_get(MIB_DSL_G_VECTOR, (void *)&gvector);
			val = (int)gvector;
			//printf("SetGVector=0x%08x\n", val);
			dsl_msg_set(SetGVector, val);
			// set OLR Type
			mib_get(MIB_ADSL_OLR, (void *)&olr);
			val = (int)olr;
			// SRA (should include bitswap)
			if(val == 2) val = 3;
			//printf("SetOlr=0x%08x (olr=0x%02x)\n", val, olr);
			dsl_msg_set(SetOlr, val);

#endif /*CONFIG_DSL_VTUO*/

			// Start handshaking; should be the last command
			mode=0;
			adsl_drv_get(RLCM_SET_ADSL_MODE, (void *)&mode, 4);

			if (WAN_MODE & MODE_BOND){
				//set bonding master
				tmp2dsl[0] = 1; //enable
				tmp2dsl[1] = 0; //0:master, 1:slave, 2:slave2, 3:slave3
				tmp2dsl[2] = 4; //TOTAL_SLAVE_NUM
				tmp2dsl[3] = 1; //bit0: PTM, bit1: TDIM, bit2: ATM
				dsl_msg_set_array(SetDslBond, tmp2dsl);
			}

#ifdef CONFIG_USER_CMD_SUPPORT_GFAST
			//Step 2: set pmd mode => DSP team will update lib later

			//Step 3: set G.fast configure
			unsigned int gfast_profile=0;
			unsigned short tmp=0;
			mib_get( MIB_GFAST_PROFILE, (void *)&tmp);
			gfast_profile = tmp;
			commserv_setGFast(SetGfastSupportProfile, &gfast_profile);

			//Step 4: enable G.fast modem
			printf("G.fast enable modem\n");
			GfastEnableOpt = 1;
			commserv_setGFast(SetTest, &GfastEnableOpt);
#else
			//enable modem,20130203
			printf("xDSL enable modem\n");
			adsl_drv_get(RLCM_PHY_ENABLE_MODEM, NULL, 0);
#endif	/* CONFIG_USER_CMD_SUPPORT_GFAST */

			ret = 1;
#else /*CONFIG_VDSL*/
			char mode[3], inp;
			int xmode,adslmode, axB, axM, axL;

			// start adsl
			mib_get(MIB_ADSL_MODE, (void *)&dsl_mode);
			mib_get( MIB_DSL_ANNEX_B, (void *)&dsl_anxb);

			adslmode=(int)(dsl_mode & (ADSL_MODE_GLITE|ADSL_MODE_T1413|ADSL_MODE_GDMT));	// T1.413 & G.dmt
			#if 0
			adsl_drv_get(RLCM_PHY_START_MODEM, (void *)&adslmode, 4);
			#endif

			//if (dsl_mode & ADSL_MODE_ANXB) {	// Annex B
			if (dsl_anxb) {
				axB = 1;
				axL = 0;
				axM = 0;
			}
			else {	// Annex A
				axB = 0;
				if (dsl_mode & ADSL_MODE_ANXL)	// Annex L
					axL = 3; // Wide-Band & Narrow-Band Mode
				else
					axL = 0;
				if (dsl_mode & ADSL_MODE_ANXM)	// Annex M
					axM = 1;
				else
					axM = 0;
			}

			adsl_drv_get(RLCM_SET_ANNEX_B, (void *)&axB, 4);
			adsl_drv_get(RLCM_SET_ANNEX_L, (void *)&axL, 4);
			adsl_drv_get(RLCM_SET_ANNEX_M, (void *)&axM, 4);

#ifdef ENABLE_ADSL_MODE_GINP
			if (dsl_mode & ADSL_MODE_GINP)	// G.INP
				xmode = DSL_FUNC_GINP;
			else
				xmode = 0;
			adsl_drv_get(RLCM_SET_DSL_FUNC, (void *)&xmode, 4);
#endif

			xmode=0;
			if (dsl_mode & (ADSL_MODE_GLITE|ADSL_MODE_T1413|ADSL_MODE_GDMT))
				xmode |= 1;	// ADSL1
			if (dsl_mode & ADSL_MODE_ADSL2)
				xmode |= 2;	// ADSL2
			if (dsl_mode & ADSL_MODE_ADSL2P)
				xmode |= 4;	// ADSL2+
			adsl_drv_get(RLCM_SET_XDSL_MODE, (void *)&xmode, 4);

			// set OLR Type
			mib_get(MIB_ADSL_OLR, (void *)&olr);

			val = (int)olr;
			if (val == 2) // SRA (should include bitswap)
				val = 3;

			adsl_drv_get(RLCM_SET_OLR_TYPE, (void *)&val, 4);

			// set Tone mask
			mib_get(MIB_ADSL_TONE, (void *)tone);
			adsl_drv_get(RLCM_LOADCARRIERMASK, (void *)tone, GET_LOADCARRIERMASK_SIZE);

			mib_get(MIB_ADSL_HIGH_INP, (void *)&inp);
			xmode = inp;
			adsl_drv_get(RLCM_SET_HIGH_INP, (void *)&xmode, 4);

			// new_hibrid
			mib_get(MIB_DEVICE_TYPE, (void *)&inp);
			xmode = inp;
			switch(xmode) {
				case 1:
					xmode = 1052;
					break;
				case 2:
					xmode = 2099;
					break;
				case 3:
					xmode = 3099;
					break;
				case 4:
					xmode = 4052;
					break;
				case 5:
					xmode = 5099;
					break;
				case 9:
					xmode = 9099;
					break;
				default:
					xmode = 9099;
			}
			adsl_drv_get(RLCM_SET_HYBRID, (void *)&xmode, 4);
			// Start handshaking; should be the last command.
			adsl_drv_get(RLCM_SET_ADSL_MODE, (void *)&adslmode, 4);
			ret = 1;
#endif /*CONFIG_VDSL*/
		}
		else
			ret = 0;
	}
	else
		ret = -1;

#ifdef FIELD_TRY_SAFE_MODE
	unsigned char mode;
	SafeModeData vSmd;

	memset((void *)&vSmd, 0, sizeof(vSmd));
	mib_get(MIB_ADSL_FIELDTRYSAFEMODE, (void *)&mode);
	vSmd.FieldTrySafeMode = (int)mode;
	mib_get(MIB_ADSL_FIELDTRYTESTPSDTIMES, (void *)&vSmd.FieldTryTestPSDTimes);
	mib_get(MIB_ADSL_FIELDTRYCTRLIN, (void *)&vSmd.FieldTryCtrlIn);
	adsl_drv_get(RLCM_SET_SAFEMODE_CTRL, (void *)&vSmd, SAFEMODE_DATA_SIZE);
#endif

	return ret;
}
#endif // of CONFIG_DEV_xDSL

#if defined(CONFIG_USER_XDSL_SLAVE)
int setupSlvDsl(void)
{
	char tone[64];
	unsigned char init_line, olr;
	unsigned char dsl_anxb;
	unsigned short dsl_mode;
	int val;
	int ret=1;
	int tmp2dsl[10];

	// check INIT_LINE
	if (mib_get(MIB_INIT_LINE, (void *)&init_line) != 0)
	{
		if (init_line == 1)
		{
			unsigned int mode;
			unsigned short profile;
			unsigned char gvector;

			//disable modem,20130203
			printf("xDSL disable modem\n");
			adsl_slv_drv_get(RLCM_PHY_DISABLE_MODEM, NULL, 0);

			//Pmd mode
			mode=0;
			mib_get( MIB_DSL_ANNEX_B, (void *)&dsl_anxb);
			mib_get(MIB_ADSL_MODE, (void *)&dsl_mode);
			if(dsl_mode&ADSL_MODE_GDMT)		mode |= MODE_GDMT;
			if(dsl_mode&ADSL_MODE_GLITE)	mode |= MODE_GLITE;
			if(dsl_mode&ADSL_MODE_ADSL2)	mode |= MODE_ADSL2;
			if(dsl_mode&ADSL_MODE_ADSL2P)	mode |= MODE_ADSL2PLUS;
			if(dsl_mode&ADSL_MODE_VDSL2)	mode |= MODE_VDSL2;

			//if(dsl_mode&ADSL_MODE_ANXB)
			if (dsl_anxb)
			{
				// Annex B
				mode |= MODE_ANX_B;
				if(dsl_mode&ADSL_MODE_T1413)	mode |= MODE_ETSI;
				if(dsl_mode&ADSL_MODE_ANXJ)	mode |= MODE_ANX_J;
			}else{
				// Annex A
				mode |= MODE_ANX_A;
				if(dsl_mode&ADSL_MODE_T1413)	mode |= MODE_ANSI;
				if(dsl_mode&ADSL_MODE_ANXL)		mode |= MODE_ANX_L;
				if(dsl_mode&ADSL_MODE_ANXM)		mode |= MODE_ANX_M;
				if(dsl_mode&ADSL_MODE_ANXI)	mode |= MODE_ANX_I;
			}
			printf("SetPmdMode=0x%08x (dsl_mode=0x%04x)\n",  mode, dsl_mode);
			dsl_slv_msg_set(SetPmdMode, mode);

			//vdsl2 profile
			mib_get(MIB_VDSL2_PROFILE, (void *)&profile);
			printf("SetVdslProfile=0x%08x\n", (unsigned int)profile);
			dsl_slv_msg_set(SetVdslProfile, (unsigned int)profile);

			// G.INP,	 default by dsplib
#ifdef ENABLE_ADSL_MODE_GINP
			val=0;
			if (dsl_mode & ADSL_MODE_GINP)	// G.INP
				val=3;
			printf("SetGInp=0x%08x\n", val);
			dsl_slv_msg_set(SetGInp, val);
#endif /*ENABLE_ADSL_MODE_GINP*/
			// G.Vector
			mib_get(MIB_DSL_G_VECTOR, (void *)&gvector);
			val = (int)gvector;
			dsl_slv_msg_set(SetGVector, val);
			printf("SetGVector=0x%08x\n", val);
			// set OLR Type
			mib_get(MIB_ADSL_OLR, (void *)&olr);
			val = (int)olr;
			// SRA (should include bitswap)
			if(val == 2) val = 3;
			printf("SetOlr=0x%08x (olr=0x%02x)\n", val, olr);
			dsl_slv_msg_set(SetOlr, val);

			// Start handshaking; should be the last command
			mode=0;
			adsl_slv_drv_get(RLCM_SET_ADSL_MODE, (void *)&mode, 4);

			if (WAN_MODE & MODE_BOND){
				//set bonding slave
				tmp2dsl[0] = 1; //enable
				tmp2dsl[1] = 1; //0:master, 1:slave, 2:slave2, 3:slave3
				tmp2dsl[2] = 4; //TOTAL_SLAVE_NUM
				tmp2dsl[3] = 1; //bit0: PTM, bit1: TDIM, bit2: ATM
				dsl_slv_msg_set_array(SetDslBond, tmp2dsl);
			}

			//enable modem,20130203
			printf("xDSL enable modem\n");
			adsl_slv_drv_get(RLCM_PHY_ENABLE_MODEM, NULL, 0);

			ret = 1;

		}else 	
			ret = 0;
	} else
		ret = -1;
}
#endif

// return 1: found Prefix Delegation.
// return 0: can not find Prefix Delegation.
int getLeasesInfo(const char *fname, DLG_INFO_Tp pInfo)
{
	FILE *fp;
	char temps[0x100];
	char *str, *endStr;
	char ifIndex_str[16]={0};
	int offset, RNTime, RBTime, PLTime, MLTime;
	struct in6_addr addr6;
	int in_lease6=0, in_iapd=0, in_iaPrefix=0;

	int ret=0;

	if ((fp = fopen(fname, "r")) == NULL)
	{
		printf("Open file %s fail !\n", fname);
		return 0;
	}

	while (fgets(temps,0x100,fp))
	{
		if (temps[strlen(temps)-1]=='\n')
			temps[strlen(temps)-1] = 0;

		// Get interface
		if ( str=strstr(temps, "interface") )
		{
			offset = strlen("interface")+2;
			if ( endStr=strchr(str+offset, '"')) {
				*endStr=0;
				sscanf(str+offset, "%s", &ifIndex_str);
			}
			memcpy(pInfo->wanIfname, ifIndex_str,  sizeof(ifIndex_str));
		}
		
		if (str=strstr(temps, "lease6")) {
			// clean pInfo
			memset(pInfo, 0, sizeof(DLG_INFO_T));
			in_lease6 = 1;
		}
		if (str=strstr(temps, "ia-pd"))
		{
			in_iapd = 1;
			ret = 1;
			fgets(temps,0x100,fp);

			// Get renew
			fgets(temps,0x100,fp);
			if ( str=strstr(temps, "renew") ) {
				offset = strlen("renew")+1;
				if ( endStr=strchr(str+offset, ';')) {
					*endStr=0;
					sscanf(str+offset, "%u", &RNTime);
					pInfo->RNTime = RNTime;
				}
			}

			// Get rebind
			fgets(temps,0x100,fp);
			if ( str=strstr(temps, "rebind")) {
				offset = strlen("rebind")+1;
				if ( endStr=strchr(str+offset, ';')) {
					*endStr=0;
					sscanf(str+offset, "%u", &RBTime);
					pInfo->RBTime = RBTime;
				}
			}
			
			// the rest of ia-pd declaration
			while (in_iapd) {
				// Get prefix
				fgets(temps,0x100,fp);
				if ( str=strstr(temps, "iaprefix")) {
					in_iaPrefix = 1;
					offset = strlen("iaprefix")+1;
					if ( endStr=strchr(str+offset, ' ')) {
						*endStr=0;
                                	
						endStr=strchr(str+offset, '/');
						*endStr=0;
                                	
						// PrefixIP
						inet_pton(PF_INET6, (str+offset), &addr6);
						memcpy(pInfo->prefixIP, &addr6, IP6_ADDR_LEN);
                                	
						// Prefix Length
						//sscanf((endStr+1), "%d", &(Info.prefixLen));
						pInfo->prefixLen = (char)atoi((endStr+1));
					}
                                	
					fgets(temps,0x100,fp);
                        		
					// Get preferred-life
					fgets(temps,0x100,fp);
					if ( str=strstr(temps, "preferred-life")) {
						offset = strlen("preferred-life")+1;
						if ( endStr=strchr(str+offset, ';') ) {
							*endStr=0;
							sscanf(str+offset, "%u", &PLTime);
							pInfo->PLTime = PLTime;
						}
					}
                        		
					// Get max-life
					fgets(temps,0x100,fp);
					if( str=strstr(temps, "max-life")) {
						offset = strlen("max-life")+1;
						if ( endStr=strchr(str+offset, ';')) {
							*endStr=0;
							sscanf(str+offset, "%u", &MLTime);
							pInfo->MLTime = MLTime;
						}
					}
				}
				else if (strstr(temps, "}")) {
					if (in_iaPrefix)
						in_iaPrefix = 0;
					else
						in_iapd = 0;
				}
			}
		}
		if ( str=strstr(temps, "dhcp6.name-servers")) {
			offset = strlen("dhcp6.name-servers")+1;
			if ( endStr=strchr(str+offset, ';')) {
				*endStr=0;
				//printf("Name server=%s\n", str+offset);
				memcpy(pInfo->nameServer, (unsigned char *)(str+offset),  256);
			}
		}
		if (strstr(temps, "}")) {
			if (in_lease6)
				in_lease6 = 0;
		}
	}
	fclose(fp);

	return ret;
}

int getLANPortStatus(unsigned int id, char *strbuf)
{
	struct net_link_info netlink_info;
	int link_status=0;

	switch (id) {
		case 0:
			link_status = get_net_link_status(ALIASNAME_ELAN0);
			if( link_status==1)
				get_net_link_info(ALIASNAME_ELAN0, &netlink_info);
			break;
		case 1:
			link_status = get_net_link_status(ALIASNAME_ELAN1);
			if( link_status==1)
				get_net_link_info(ALIASNAME_ELAN1, &netlink_info);
			break;
		case 2:
			link_status = get_net_link_status(ALIASNAME_ELAN2);
			if( link_status==1)
				get_net_link_info(ALIASNAME_ELAN2, &netlink_info);
			break;
		case 3:
			link_status = get_net_link_status(ALIASNAME_ELAN3);
			if( link_status==1)
				get_net_link_info(ALIASNAME_ELAN3, &netlink_info);
			break;
	}

	switch(link_status)
	{
		case -1:
			strcpy(strbuf, "Error");
			goto setErr_lanport;
			//break;
		case 0:
			strcpy(strbuf, "not-connected");
			//break;
			goto setErr_lanport;
		case 1:
			strcpy(strbuf, "Up");
			break;
		default:
			goto setErr_lanport;
			//break;
	}
	sprintf(strbuf, "%s, %dMb", strbuf, netlink_info.speed);

	if(netlink_info.duplex == 0)
		sprintf(strbuf, "%s, %s", strbuf, "Half");
	else if(netlink_info.duplex == 1)
		sprintf(strbuf, "%s, %s", strbuf, "Full");

setErr_lanport:
	return 0;
}

int getMIB2Str(unsigned int id, char *strbuf)
{
	unsigned char buffer[64];

	if (!strbuf)
		return -1;

	switch (id) {
		// INET address
		case MIB_ADSL_LAN_IP:
		case MIB_ADSL_LAN_SUBNET:
		case MIB_ADSL_LAN_IP2:
		case MIB_ADSL_LAN_SUBNET2:
		case MIB_ADSL_WAN_DNS1:
		case MIB_ADSL_WAN_DNS2:
		case MIB_ADSL_WAN_DNS3:
#ifdef CONFIG_USER_DHCP_SERVER
		case MIB_ADSL_WAN_DHCPS:
#endif
#ifdef DMZ
		case MIB_DMZ_IP:
#endif
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
		case MIB_SNMP_TRAP_IP:
#endif
		case MIB_DHCP_POOL_START:
		case MIB_DHCP_POOL_END:
		case MIB_DHCPS_DNS1:
		case MIB_DHCPS_DNS2:
		case MIB_DHCPS_DNS3:
		case MIB_DHCP_SUBNET_MASK:
#ifdef AUTO_PROVISIONING
		case MIB_HTTP_SERVER_IP:
#endif
		case MIB_ADSL_LAN_DHCP_GATEWAY:
#if 1
#if defined(_PRMT_X_CT_COM_DHCP_)||defined(IP_BASED_CLIENT_TYPE)
		case CWMP_CT_STB_MINADDR:
		case CWMP_CT_STB_MAXADDR:
		case CWMP_CT_PHN_MINADDR:
		case CWMP_CT_PHN_MAXADDR:
		case CWMP_CT_CMR_MINADDR:
		case CWMP_CT_CMR_MAXADDR:
		case CWMP_CT_PC_MINADDR:
		case CWMP_CT_PC_MAXADDR:
		case CWMP_CT_HGW_MINADDR:
		case CWMP_CT_HGW_MAXADDR:
#endif //_PRMT_X_CT_COM_DHCP_
#endif
		//ql 20090122 add
#ifdef IMAGENIO_IPTV_SUPPORT
		case MIB_IMAGENIO_DNS1:
		case MIB_IMAGENIO_DNS2:
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
		case MIB_OPCH_ADDRESS:
#endif
/*ping_zhang:20090930 END*/
#endif

#ifdef ADDRESS_MAPPING
#ifndef MULTI_ADDRESS_MAPPING
		case MIB_LOCAL_START_IP:
		case MIB_LOCAL_END_IP:
		case MIB_GLOBAL_START_IP:
		case MIB_GLOBAL_END_IP:
#endif //end of !MULTI_ADDRESS_MAPPING
#endif
#ifdef DEFAULT_GATEWAY_V2
		case MIB_ADSL_WAN_DGW_IP:
#endif
#ifdef CONFIG_USER_RTK_SYSLOG
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
		case MIB_SYSLOG_SERVER_IP:
#endif
#ifdef SEND_LOG
		case MIB_LOG_SERVER_IP:
#endif
#endif
#ifdef _PRMT_TR143_
		case TR143_UDPECHO_SRCIP:
#endif //_PRMT_TR143_
			if(!mib_get( id, (void *)buffer))
				return -1;
			// Mason Yu
			if ( ((struct in_addr *)buffer)->s_addr == INADDR_NONE ) {
				sprintf(strbuf, "%s", "");
			} else {
				sprintf(strbuf, "%s", inet_ntoa(*((struct in_addr *)buffer)));
			}
			break;
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
		// INET6 address
		case MIB_DHCPV6S_RANGE_START:
		case MIB_DHCPV6S_RANGE_END:
			if(!mib_get( id, (void *)buffer))
				return -1;
			inet_ntop(PF_INET6, buffer, strbuf, 48);
			break;
#endif
		case MIB_ADSL_WAN_DNSV61:
		case MIB_ADSL_WAN_DNSV62:
		case MIB_ADSL_WAN_DNSV63:
			if(!mib_get( id, (void *)buffer))
				return -1;
			inet_ntop(PF_INET6, buffer, strbuf, 48);
			break;
#endif
		// Ethernet address
		case MIB_ELAN_MAC_ADDR:
		case MIB_WLAN_MAC_ADDR:
			if(!mib_get( id,  (void *)buffer))
				return -1;
			sprintf(strbuf, "%02x%02x%02x%02x%02x%02x", buffer[0], buffer[1],
				buffer[2], buffer[3], buffer[4], buffer[5]);
			break;
		// Char
#ifdef CONFIG_GPON_FEATURE
		case MIB_OMCC_VER:
		case MIB_OMCI_TM_OPT:
#endif
		case MIB_ADSL_LAN_CLIENT_START:
		case MIB_ADSL_LAN_CLIENT_END:
#ifdef WLAN_SUPPORT
		case MIB_WLAN_CHAN_NUM:
		case MIB_WLAN_CHANNEL_WIDTH:
		case MIB_WLAN_NETWORK_TYPE:
		case MIB_WIFI_TEST:

#ifdef WLAN_UNIVERSAL_REPEATER
		case MIB_REPEATER_ENABLED1:
#endif
#ifdef WLAN_WDS
		case MIB_WLAN_WDS_ENABLED:
#endif

#ifdef CONFIG_WIFI_SIMPLE_CONFIG // WPS
		case MIB_WSC_VERSION:
#endif
		case MIB_WLAN_DTIM_PERIOD:
#endif
#ifdef CONFIG_USER_RTK_SYSLOG
		case MIB_SYSLOG_LOG_LEVEL:
		case MIB_SYSLOG_DISPLAY_LEVEL:
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
		case MIB_SYSLOG_MODE:
#endif
#endif
#if 0
#if defined(_PRMT_X_CT_COM_DHCP_)||defined(IP_BASED_CLIENT_TYPE)
		case CWMP_CT_STB_MINADDR:
		case CWMP_CT_STB_MAXADDR:
		case CWMP_CT_PHN_MINADDR:
		case CWMP_CT_PHN_MAXADDR:
		case CWMP_CT_CMR_MINADDR:
		case CWMP_CT_CMR_MAXADDR:
		case CWMP_CT_PC_MINADDR:
		case CWMP_CT_PC_MAXADDR:
		case CWMP_CT_HGW_MINADDR:
		case CWMP_CT_HGW_MAXADDR:
#endif //_PRMT_X_CT_COM_DHCP_
#endif
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
		case MIB_DHCPV6S_PREFIX_LENGTH:
#endif
#endif
#ifdef CONFIG_RTK_L34_ENABLE
		case MIB_MAC_BASED_TAG_DECISION:
#endif
#ifdef TIME_ZONE
		case MIB_NTP_TIMEZONE_DB_INDEX:
#endif
			if(!mib_get(id,  (void *)buffer))
				return -1;
	   		sprintf(strbuf, "%u", *(unsigned char *)buffer);
	   		break;
	   	// Short
		case MIB_DHCP_PORT_FILTER:
	   	case MIB_BRCTL_AGEINGTIME:
#ifdef WLAN_SUPPORT
	   	case MIB_WLAN_FRAG_THRESHOLD:
	   	case MIB_WLAN_RTS_THRESHOLD:
	   	case MIB_WLAN_BEACON_INTERVAL:
#endif
#ifdef CONFIG_USER_RTK_SYSLOG
#ifdef CONFIG_USER_RTK_SYSLOG_REMOTE
		case MIB_SYSLOG_SERVER_PORT:
#endif
#endif
		//ql 20090122 add
#ifdef IMAGENIO_IPTV_SUPPORT
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
		case MIB_OPCH_PORT:
#endif
/*ping_zhang:20090930 END*/
#endif
			if(!mib_get( id,  (void *)buffer))
				return -1;
			sprintf(strbuf, "%u", *(unsigned short *)buffer);
			break;
	   	// Interger
		case MIB_ADSL_LAN_DHCP_LEASE:

			// Mason Yu
			if(!mib_get( id,  (void *)buffer))
				return -1;
			// if MIB_ADSL_LAN_DHCP_LEASE=0xffffffff, it indicate an infinate lease
			if ( *(unsigned long *)buffer == 0xffffffff )
				sprintf(strbuf, "-1");
			else
				sprintf(strbuf, "%u", *(unsigned int *)buffer);
			break;
		// Interger
#ifdef WEB_REDIRECT_BY_MAC
		case MIB_WEB_REDIR_BY_MAC_INTERVAL:
#endif
		case MIB_IGMP_PROXY_ITF:
#if defined(CONFIG_USER_UPNPD)||defined(CONFIG_USER_MINIUPNPD)
		case MIB_UPNP_EXT_ITF:
#endif
#ifdef TIME_ZONE
		case MIB_NTP_EXT_ITF:
#endif
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_ECMH		// Mason Yu. MLD Proxy
		case MIB_MLD_PROXY_EXT_ITF:
#endif
#endif

#ifdef IP_PASSTHROUGH
		case MIB_IPPT_ITF:
		case MIB_IPPT_LEASE:
#endif
#ifdef DEFAULT_GATEWAY_V2
		case MIB_ADSL_WAN_DGW_ITF:
#endif
		case MIB_MAXLOGLEN:
#ifdef _CWMP_MIB_
		case CWMP_INFORM_INTERVAL:
#endif
#ifdef DOS_SUPPORT
		case MIB_DOS_SYSSYN_FLOOD:
		case MIB_DOS_SYSFIN_FLOOD:
		case MIB_DOS_SYSUDP_FLOOD:
		case MIB_DOS_SYSICMP_FLOOD:
		case MIB_DOS_PIPSYN_FLOOD:
		case MIB_DOS_PIPFIN_FLOOD:
		case MIB_DOS_PIPUDP_FLOOD:
		case MIB_DOS_PIPICMP_FLOOD:
		case MIB_DOS_BLOCK_TIME:
#endif
#ifdef TCP_UDP_CONN_LIMIT
		case MIB_CONNLIMIT_TCP:
		case MIB_CONNLIMIT_UDP:
#endif
#ifdef _CWMP_MIB_
		case CWMP_CONREQ_PORT:
		case CWMP_WAN_INTERFACE:
#endif
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
		case MIB_DHCPV6S_DEFAULT_LEASE:
		case MIB_DHCPV6S_PREFERRED_LIFETIME:
		case MIB_DHCPV6S_RENEW_TIME:
		case MIB_DHCPV6S_REBIND_TIME:
#endif
#endif
#ifdef CONFIG_RTK_L34_ENABLE
		case MIB_LAN_VLAN_ID1:
		case MIB_LAN_VLAN_ID2:
#endif
		case MIB_WAN_MODE:
			if(!mib_get( id,  (void *)buffer))
				return -1;
			sprintf(strbuf, "%u", *(unsigned int *)buffer);
			break;
		// String
#ifdef CONFIG_GPON_FEATURE
		case MIB_OMCI_SW_VER1:
		case MIB_OMCI_SW_VER2:
		case MIB_HW_CWMP_PRODUCTCLASS:
		case MIB_HW_HWVER:
		case MIB_PON_VENDOR_ID:
#endif
		case MIB_ADSL_LAN_DHCP_DOMAIN:
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
		case MIB_SNMP_SYS_DESCR:
		case MIB_SNMP_SYS_CONTACT:
		case MIB_SNMP_SYS_LOCATION:
		case MIB_SNMP_SYS_OID:
		case MIB_SNMP_COMM_RO:
		case MIB_SNMP_COMM_RW:
#endif
		case MIB_SNMP_SYS_NAME:
#ifdef WLAN_SUPPORT
#ifdef WLAN_UNIVERSAL_REPEATER
		case MIB_REPEATER_SSID1:
#endif
#ifdef WLAN_WDS
		case MIB_WLAN_WDS_PSK:
#endif
#endif
#ifdef AUTO_PROVISIONING
		case MIB_CONFIG_VERSION:
#endif
#ifdef TIME_ZONE
		case MIB_NTP_ENABLED:
		case MIB_NTP_SERVER_ID:
/*ping_zhang:20081217 START:patch from telefonica branch to support WT-107*/
		case MIB_NTP_SERVER_HOST1:
		case MIB_NTP_SERVER_HOST2:
#endif
/*ping_zhang:20081217*/
#ifdef _CWMP_MIB_
		case CWMP_PROVISIONINGCODE:
		case CWMP_ACS_URL:
		case CWMP_ACS_USERNAME:
		case CWMP_ACS_PASSWORD:
		case CWMP_CONREQ_USERNAME:
		case CWMP_CONREQ_PASSWORD:
		case CWMP_CONREQ_PATH:
		case CWMP_LAN_CONFIGPASSWD:
		case CWMP_SERIALNUMBER:
		case CWMP_DL_COMMANDKEY:
		case CWMP_RB_COMMANDKEY:
		case CWMP_ACS_PARAMETERKEY:
		case CWMP_CERT_PASSWORD:
#ifdef _PRMT_USERINTERFACE_
		case UIF_AUTOUPDATESERVER:
		case UIF_USERUPDATESERVER:
#endif
		case CWMP_SI_COMMANDKEY:
		case CWMP_ACS_KICKURL:
		case CWMP_ACS_DOWNLOADURL:
#ifdef _PRMT_X_CT_COM_PORTALMNT_
		case CWMP_CT_PM_URL4PC:
		case CWMP_CT_PM_URL4STB:
		case CWMP_CT_PM_URL4MOBILE:
#endif

#endif
#ifdef CONFIG_WIFI_SIMPLE_CONFIG //WPS
		case MIB_WSC_PIN:
		case MIB_WSC_SSID:
#endif
#ifdef CONFIG_USER_RTK_SYSLOG
#ifdef SEND_LOG
		case MIB_LOG_SERVER_NAME:
#endif
#endif
#if defined(CONFIG_IPV6) && defined(CONFIG_USER_RADVD)
		case MIB_V6_MAXRTRADVINTERVAL:
		case MIB_V6_MINRTRADVINTERVAL:
		case MIB_V6_ADVCURHOPLIMIT:
		case MIB_V6_ADVDEFAULTLIFETIME:
		case MIB_V6_ADVREACHABLETIME:
		case MIB_V6_ADVRETRANSTIMER:
		case MIB_V6_ADVLINKMTU:
		case MIB_V6_PREFIX_IP:
		case MIB_V6_PREFIX_LEN:
		case MIB_V6_VALIDLIFETIME:
		case MIB_V6_PREFERREDLIFETIME:
		case MIB_V6_RDNSS1:
		case MIB_V6_RDNSS2:
		case MIB_V6_ULAPREFIX_ENABLE:
		case MIB_V6_ULAPREFIX:
		case MIB_V6_ULAPREFIX_LEN:
		case MIB_V6_ULAPREFIX_VALID_TIME:
		case MIB_V6_ULAPREFIX_PREFER_TIME:
#endif
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
		case MIB_DHCPV6S_CLIENT_DUID:
#endif
#endif
		case MIB_SUSER_NAME:
#ifdef USER_WEB_WIZARD
		case MIB_SUSER_PASSWORD:
#endif
		case MIB_USER_NAME:
#ifdef CONFIG_USER_SAMBA
#ifdef CONFIG_USER_NMBD
		case MIB_SAMBA_NETBIOS_NAME:
#endif
		case MIB_SAMBA_SERVER_STRING:
#endif
			if(!mib_get( id,  (void *)strbuf)){
				return -1;
			}
			break;

#ifdef CONFIG_RTL_WAPI_SUPPORT
		case MIB_WLAN_WAPI_MCAST_REKEYTYPE:
		case MIB_WLAN_WAPI_UCAST_REKETTYPE:
			if(!mib_get( id,  (void *)buffer)){
				return -1;
			}
			sprintf(strbuf, "%d", buffer[0]);
			break;
		case MIB_WLAN_WAPI_MCAST_TIME:
		case MIB_WLAN_WAPI_MCAST_PACKETS:
		case MIB_WLAN_WAPI_UCAST_TIME:
		case MIB_WLAN_WAPI_UCAST_PACKETS:
			if(!mib_get( id,  (void *)buffer)){
				return -1;
			}
			sprintf(strbuf, "%u", *(unsigned int *)buffer);
			break;
#endif
#ifdef CONFIG_USER_Y1731
		case Y1731_MODE:
		case Y1731_MEGLEVEL:
		case Y1731_CCM_INTERVAL:
		case Y1731_ENABLE_CCM:
			if(!mib_get(id,  (void *)buffer)){
				return -1;
			}
			sprintf(strbuf, "%u", *(unsigned char *)buffer);			
			break;

		case Y1731_MYID:
			if(!mib_get(id,  (void *)buffer)){
				return -1;
			}
			sprintf(strbuf, "%u", *(unsigned short *)buffer);
			break;
		case Y1731_MEGID:
		case Y1731_LOGLEVEL:
		case Y1731_WAN_INTERFACE:
			if(!mib_get(id,  (void *)strbuf)){
				return -1;
			}
			break;
#endif
#ifdef CONFIG_USER_8023AH
		case EFM_8023AH_MODE:		
			if(!mib_get(id,  (void *)buffer)){
				return -1;
			}
			sprintf(strbuf, "%u", *(unsigned char *)buffer);			
			break;
		case EFM_8023AH_WAN_INTERFACE:
			if(!mib_get(id,  (void *)strbuf)){
				return -1;
			}
			break;
#endif
		// below are version information for tr069 inform packet
		case RTK_DEVID_MANUFACTURER:
		case RTK_DEVID_OUI:
		case RTK_DEVID_PRODUCTCLASS:
		case RTK_DEVINFO_HWVER:
		case RTK_DEVINFO_SWVER:
		case RTK_DEVINFO_SPECVER:
		case MIB_HW_SERIAL_NUMBER:
#if defined(CONFIG_GPON_FEATURE)
		case MIB_GPON_SN:
#endif
			if(!mib_get( id,  (void *)strbuf))
			{
				return -1;
			}

			break;


		default:
			return -1;
	}

	return 0;
}

#ifdef CONFIG_USER_BOA_CSRF
int getSYSInfoTimer() {
	struct sysinfo info;
	int systimer=0;
	sysinfo(&info);
	systimer = (int)info.uptime;
	return systimer;
}
#endif

int getSYS2Str(SYSID_T id, char *strbuf)
{
	unsigned char buffer[128], vChar;
	struct sysinfo info;
	int updays, uphours, upminutes, len, i;
	time_t tm;
	struct tm tm_time, *ptm_time;
	FILE *fp;
	unsigned char tmpBuf[64], *pStr;
	unsigned short vUShort;
	unsigned int vUInt;
#ifdef CONFIG_00R0
	int swActive=0;
	char str_active[128], header_buf[128];
#endif
#ifdef CONFIG_IPV6
	struct ipv6_ifaddr ip6_addr[6];
#endif
	struct stat f_status;
#ifdef WLAN_SUPPORT
	MIB_CE_MBSSIB_T Entry;
#endif
#if defined(CONFIG_RTL_8676HWNAT) || defined(CONFIG_USER_VLAN_ON_LAN)
	MIB_CE_SW_PORT_T sw_entry;
#endif
#ifdef CONFIG_USER_DHCPCLIENT_MODE
	struct in_addr inAddr;
#endif

	if (!strbuf)
		return -1;

	strbuf[0] = '\0';

	switch (id) {
		case SYS_UPTIME:
			sysinfo(&info);
			updays = (int) info.uptime / (60*60*24);
			if (updays)
				sprintf(strbuf, "%d day%s, ", updays, (updays != 1) ? "s" : "");
			len = strlen(strbuf);
			upminutes = (int) info.uptime / 60;
			uphours = (upminutes / 60) % 24;
			upminutes %= 60;
			if(uphours)
				sprintf(&strbuf[len], "%2d:%02d", uphours, upminutes);
			else
				sprintf(&strbuf[len], "%d min", upminutes);
			break;
		case SYS_DATE:
	 		time(&tm);
			memcpy(&tm_time, localtime(&tm), sizeof(tm_time));
			strftime(strbuf, 200, "%a %b %e %H:%M:%S %Z %Y", &tm_time);
			break;
		case SYS_YEAR:
	 		time(&tm);
			ptm_time = localtime(&tm);
			snprintf(strbuf, 64, "%d", (ptm_time->tm_year+ 1900));
			break;
		case SYS_MONTH:
	 		time(&tm);
			ptm_time = localtime(&tm);
			snprintf(strbuf, 64, "%d", (ptm_time->tm_mon+ 1));
			break;
		case SYS_DAY:
	 		time(&tm);
			ptm_time = localtime(&tm);
			snprintf(strbuf, 64, "%d", (ptm_time->tm_mday));
			break;
		case SYS_HOUR:
	 		time(&tm);
			ptm_time = localtime(&tm);
			snprintf(strbuf, 64, "%d", (ptm_time->tm_hour));
			break;
		case SYS_MINUTE:
	 		time(&tm);
			ptm_time = localtime(&tm);
			snprintf(strbuf, 64, "%d", (ptm_time->tm_min));
			break;
		case SYS_SECOND:
	 		time(&tm);
			ptm_time = localtime(&tm);
			snprintf(strbuf, 64, "%d", (ptm_time->tm_sec));
			break;
		case SYS_FWVERSION:
#ifdef CONFIG_00R0 /* Read the SW FW version from U-BOOT, Iulian Wu*/
			sprintf(header_buf, "%s", BOOTLOADER_SW_ACTIVE);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "sw_active=%d", &swActive);

			sprintf(header_buf, "%s%d", BOOTLOADER_SW_VERSION, swActive);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "%*[^=]=%s", strbuf);
#else
#ifdef EMBED
			tmpBuf[0]=0;
			pStr = 0;
			fp = fopen("/etc/version", "r");
			if (fp!=NULL) {
				fgets(tmpBuf, sizeof(tmpBuf), fp);  //main version
				fclose(fp);
				pStr = strstr(tmpBuf, " --");
				*pStr=0;
			};
			sprintf(strbuf, "%s", tmpBuf);
#else
			sprintf(strbuf, "%s.%s", "Realtek", "1");
#endif
#endif
			break;
		case SYS_HWVERSION:
			if ( !mib_get( MIB_HW_HWVER, (void *)buffer) )
				return -1;
			sprintf(strbuf, "%s", buffer);
			break;
#ifdef CONFIG_00R0
		case SYS_BOOTLOADER_FWVERSION:
			sprintf(header_buf, "%s", BOOTLOADER_UBOOT_HEADER);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "%*[^=]=%[^\n]", strbuf);
			break;

		case SYS_BOOTLOADER_CRC:
			sprintf(header_buf, "%s", BOOTLOADER_UBOOT_CRC);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "%*[^=]=%[^\n]", strbuf);
			break;

		case SYS_FWVERSION_SUM_1:  //current FW CRC
			sprintf(header_buf, "%s", BOOTLOADER_SW_ACTIVE);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "sw_active=%d", &swActive);

			sprintf(header_buf, "%s%d", BOOTLOADER_SW_CRC, swActive);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "%*[^=]=%s", strbuf);
			break;

		case SYS_FWVERSION_SUM_2: //backup FW CRC
			sprintf(header_buf, "%s", BOOTLOADER_SW_ACTIVE);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "sw_active=%d", &swActive);

			sprintf(header_buf, "%s%d", BOOTLOADER_SW_CRC, 1-swActive);
			rtk_env_get(header_buf, str_active, sizeof(str_active));
			sscanf(str_active, "%*[^=]=%s", strbuf);
			break;
#endif
#ifdef CPU_UTILITY
		case SYS_CPU_UTIL:
			{
				sprintf(strbuf, "%d%%", get_cpu_usage());
			}
			break;
#endif
#ifdef MEM_UTILITY
		case SYS_MEM_UTIL:
			{
				sprintf(strbuf, "%d%%", get_mem_usage());
			}
			break;
#endif
#ifdef CONFIG_USER_DHCPCLIENT_MODE
		case SYS_LAN_IP:
			if(!getInAddr(ALIASNAME_BR0, IP_ADDR, (void *)&inAddr)){
				return -1;
			}
			sprintf(strbuf, "%s", inet_ntoa(inAddr));
			break;
		case SYS_LAN_MASK:
			if(!getInAddr(ALIASNAME_BR0, SUBNET_MASK, (void *)&inAddr)){
				return -1;
			}
			sprintf(strbuf, "%s", inet_ntoa(inAddr));
			break;
#endif
#ifdef CONFIG_USER_DHCP_SERVER
		case SYS_LAN_DHCP:
			if ( !mib_get( MIB_DHCP_MODE, (void *)buffer) )
				return -1;
			if (DHCP_LAN_SERVER == buffer[0])
				strcpy(strbuf, STR_ENABLE);
			else
				strcpy(strbuf, STR_DISABLE);
			break;
#endif
		case SYS_DHCP_LAN_IP:
#if defined(CONFIG_CONFIG_SECONDARY_IP) && !defined(DHCPS_POOL_COMPLETE_IP)
			mib_get(MIB_ADSL_LAN_ENABLE_IP2, (void *)&vChar);
			if (vChar)
				mib_get(MIB_ADSL_LAN_DHCP_POOLUSE, (void *)&vChar);
#else
			vChar = 0;
#endif
			if (vChar == 0) // primary LAN
				getMIB2Str(MIB_ADSL_LAN_IP, strbuf);
			else // secondary LAN
				getMIB2Str(MIB_ADSL_LAN_IP2, strbuf);
			break;
		case SYS_DHCP_LAN_SUBNET:
#if defined(CONFIG_SECONDARY_IP) && !defined(DHCPS_POOL_COMPLETE_IP)
			mib_get(MIB_ADSL_LAN_ENABLE_IP2, (void *)&vChar);
			if (vChar)
				mib_get(MIB_ADSL_LAN_DHCP_POOLUSE, (void *)&vChar);
#else
			vChar = 0;
#endif
			if (vChar == 0) // primary LAN
				getMIB2Str(MIB_ADSL_LAN_SUBNET, strbuf);
			else // secondary LAN
				getMIB2Str(MIB_ADSL_LAN_SUBNET2, strbuf);
			break;
			// Kaohj
		case SYS_DHCPS_IPPOOL_PREFIX:
#if defined(CONFIG_SECONDARY_IP) && !defined(DHCPS_POOL_COMPLETE_IP)
			mib_get(MIB_ADSL_LAN_ENABLE_IP2, (void *)&vChar);
			if (vChar)
				mib_get(MIB_ADSL_LAN_DHCP_POOLUSE, (void *)&vChar);
#else
			vChar = 0;
#endif
			if (vChar == 0) // primary LAN
				mib_get(MIB_ADSL_LAN_IP, (void *)&tmpBuf[0]) ;
			else // secondary LAN
				mib_get(MIB_ADSL_LAN_IP2, (void *)&tmpBuf[0]) ;
			pStr = tmpBuf;
			sprintf(strbuf, "%d.%d.%d.", pStr[0], pStr[1], pStr[2]);
			break;
#ifdef WLAN_SUPPORT
		case SYS_WLAN:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (0 == Entry.wlanDisabled)
				strcpy(strbuf, STR_ENABLE);
			else
				strcpy(strbuf, STR_DISABLE);
			break;
		case SYS_WLAN_SSID:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			strcpy(strbuf, Entry.ssid);
			break;
		case SYS_WLAN_DISABLED:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf,"%u", Entry.wlanDisabled);
			break;
		case SYS_WLAN_HIDDEN_SSID:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf,"%u", Entry.hidessid);
			break;
		case SYS_WLAN_MODE_VAL:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf,"%u", Entry.wlanMode);
			break;
		case SYS_WLAN_BCASTSSID:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (0 == Entry.hidessid)
				strcpy(strbuf, STR_ENABLE);
			else
				strcpy(strbuf, STR_DISABLE);
			break;
		case SYS_WLAN_ENCRYPT_VAL:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf,"%u", Entry.encrypt);
			break;
		case SYS_WLAN_WPA_CIPHER_SUITE:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf,"%u", Entry.unicastCipher);
			break;
		case SYS_WLAN_WPA2_CIPHER_SUITE:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf,"%u", Entry.wpa2UnicastCipher);
			break;
		case SYS_WLAN_WPA_AUTH:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf,"%u", Entry.wpaAuth);
			break;
		case SYS_WLAN_BAND:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			strcpy(strbuf, wlan_band[(BAND_TYPE_T)Entry.wlanBand]);
			break;
		case SYS_WLAN_AUTH:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if(Entry.authType >= 0 && Entry.authType <= 2)
				strcpy(strbuf, wlan_auth[(AUTH_TYPE_T)Entry.authType]);
			else
				strcpy(strbuf, "None");
			break;
		case SYS_WLAN_PREAMBLE:
			if ( !mib_get( MIB_WLAN_PREAMBLE_TYPE, (void *)buffer) )
				return -1;
			strcpy(strbuf, wlan_preamble[(PREAMBLE_T)buffer[0]]);
			break;
		case SYS_WLAN_ENCRYPT:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			switch(Entry.encrypt) {
				case WIFI_SEC_NONE:
				case WIFI_SEC_WEP:
				case WIFI_SEC_WPA:
					strcpy(strbuf, wlan_encrypt[(WIFI_SECURITY_T)Entry.encrypt]);
					break;
				case WIFI_SEC_WPA2:
					strcpy(strbuf, wlan_encrypt[3]);
					break;
				case WIFI_SEC_WPA2_MIXED:
					strcpy(strbuf, wlan_encrypt[4]);
					break;
				#ifdef CONFIG_RTL_WAPI_SUPPORT
				case WIFI_SEC_WAPI:
					strcpy(strbuf, wlan_encrypt[5]);
					break;
				#endif
				default:
					strcpy(strbuf, wlan_encrypt[0]);
			}
			break;
		// Mason Yu. 201009_new_security
		case SYS_WLAN_WPA_CIPHER:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if ( Entry.unicastCipher == 0 )
				strcpy(strbuf, "");
			else
				strcpy(strbuf, wlan_Cipher[Entry.unicastCipher-1]);
			break;
		case SYS_WLAN_WPA2_CIPHER:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if ( Entry.wpa2UnicastCipher == 0 )
				strcpy(strbuf, "");
			else
				strcpy(strbuf, wlan_Cipher[Entry.wpa2UnicastCipher-1]);
			break;
		case SYS_WLAN_PSKFMT:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			strcpy(strbuf, wlan_pskfmt[Entry.wpaPSKFormat]);
			break;
		case SYS_WLAN_PSKVAL:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			for (len=0; len<strlen(Entry.wpaPSK); len++)
				strbuf[len]='*';
			strbuf[len]='\0';
			break;
#ifdef USER_WEB_WIZARD
		case SYS_WLAN_PSKVAL_WIZARD:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf, "%s", Entry.wpaPSK);
			break;
#endif
 		case SYS_WLAN_WEP_KEYLEN:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			strcpy(strbuf, wlan_wepkeylen[Entry.wep]);
			break;
		case SYS_WLAN_WEP_KEYFMT:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			strcpy(strbuf, wlan_wepkeyfmt[Entry.wepKeyType]);
			break;
		case SYS_WLAN_WPA_MODE:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (Entry.wpaAuth == WPA_AUTH_AUTO)
				strcpy(strbuf, "Enterprise (RADIUS)");
			else if (Entry.wpaAuth == WPA_AUTH_PSK)
				strcpy(strbuf, "Personal (Pre-Shared Key)");
			break;
		case SYS_WLAN_RSPASSWD:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			for (len=0; len<strlen(Entry.rsPassword); len++)
				strbuf[len]='*';
			strbuf[len]='\0';
			break;
		case SYS_WLAN_RS_PORT:
			if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf, "%u", Entry.rsPort);
			break;
		case SYS_WLAN_RS_IP:
			if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if ( ((struct in_addr *)Entry.rsIpAddr)->s_addr == INADDR_NONE ) {
				sprintf(strbuf, "%s", "");
			} else {
				sprintf(strbuf, "%s", inet_ntoa(*((struct in_addr *)Entry.rsIpAddr)));
			}
			break;
		case SYS_WLAN_RS_PASSWORD:
			if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf, "%s", Entry.rsPassword);
			break;
		case SYS_WLAN_ENABLE_1X:
			if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf, "%u", Entry.enable1X);
			break;
		// Added by Jenny
		case SYS_TX_POWER:
			if ( !mib_get( MIB_TX_POWER, (void *)&buffer) )
				return -1;
			if (0 == buffer[0])
				strcpy(strbuf, "100%");
			else if(1 == buffer[0])
				strcpy(strbuf, "70%");
			else if(2 == buffer[0])
				strcpy(strbuf, "50%");
			else if(3 == buffer[0])
				strcpy(strbuf, "35%");
			else if(4 == buffer[0])
				strcpy(strbuf, "15%");
			break;
		case SYS_WLAN_MODE:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			strcpy(strbuf, wlan_mode[(WLAN_MODE_T)Entry.wlanMode]);
			break;
		case SYS_WLAN_TXRATE:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (0 == Entry.rateAdaptiveEnabled){
				for (i=0; i<12; i++)
					if (1<<i == Entry.fixedTxRate)
						strcpy(strbuf, wlan_rate[i]);
			}
			else if (1 == Entry.rateAdaptiveEnabled)
				strcpy(strbuf, STR_AUTO);
			break;
		case SYS_WLAN_BLOCKRELAY:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (0 == Entry.userisolation)
				strcpy(strbuf, STR_DISABLE);
			else
				strcpy(strbuf, STR_ENABLE);
			break;
		case SYS_WLAN_BLOCK_ETH2WIR:
			if ( !mib_get( MIB_WLAN_BLOCK_ETH2WIR, (void *)buffer) )
				return -1;
			if (0 == buffer[0])
				strcpy(strbuf, STR_DISABLE);
			else
				strcpy(strbuf, STR_ENABLE);
			break;
		case SYS_WLAN_AC_ENABLED:
			if ( !mib_get( MIB_WLAN_AC_ENABLED, (void *)&buffer) )
				return -1;
			if (0 == buffer[0])
				strcpy(strbuf, STR_DISABLE);
			else if(1 == buffer[0])
				strcpy(strbuf, "Allow Listed");
			else if(2 == buffer[0])
				strcpy(strbuf, "Deny Listed");
			else
				strcpy(strbuf, STR_ERR);
			break;
		case SYS_WLAN_WDS_ENABLED:
			if ( !mib_get( MIB_WLAN_WDS_ENABLED, (void *)buffer) )
				return -1;
			if (0 == buffer[0])
				strcpy(strbuf, STR_DISABLE);
			else
				strcpy(strbuf, STR_ENABLE);
			break;
#ifdef WLAN_QoS
		case SYS_WLAN_QoS:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (0 == Entry.wmmEnabled)
				strcpy(strbuf, STR_DISABLE);
			else
				strcpy(strbuf, STR_ENABLE);
			break;
#endif
#ifdef CONFIG_WIFI_SIMPLE_CONFIG // WPS
		case SYS_WLAN_WPS_ENABLED:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (0 == Entry.wsc_disabled)
				strcpy(strbuf, STR_ENABLE);
			else
				strcpy(strbuf, STR_DISABLE);
			break;
		case SYS_WLAN_WPS_STATUS:
			if( !mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			if (0 == Entry.wsc_configured)
				strcpy(strbuf, "Unconfigured");
			else
				strcpy(strbuf, "Configured");
			break;
		// for PIN brute force attack
		case SYS_WLAN_WPS_LOCKDOWN:
			if (stat("/tmp/wscd_lock_stat", &f_status) == 0) {
				//printf("[%s %d] %s exist\n",__FUNCTION__,__LINE__,WSCD_LOCK_STAT);
				strbuf[0]='1';
			}else{
				strbuf[0]='0';
			}

			strbuf[1] = '\0';
			break;
		case SYS_WSC_DISABLE:
			if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf, "%u", Entry.wsc_disabled);
			break;
		case SYS_WSC_AUTH:
			if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf, "%u", Entry.wsc_auth);
			break;
		case SYS_WSC_ENC:
			if(!mib_chain_get(MIB_MBSSIB_TBL, 0, (void *)&Entry))
				return -1;
			sprintf(strbuf, "%u", Entry.wsc_enc);
			break;
#endif
#endif // #ifdef WLAN_SUPPORT
		case SYS_DNS_MODE:
			if ( !mib_get( MIB_ADSL_WAN_DNS_MODE, (void *)buffer) )
				return -1;
			if (0 == buffer[0])
				strcpy(strbuf, STR_AUTO);
			else
				strcpy(strbuf, STR_MANUAL);
			break;

#ifdef CONFIG_USER_DHCP_SERVER
		case SYS_DHCP_MODE:
			if(!mib_get( MIB_DHCP_MODE, (void *)buffer) )
				return -1;
			strcpy(strbuf, dhcp_mode[(DHCP_TYPE_T)buffer[0]]);
			break;
#endif

		case SYS_IPF_OUT_ACTION:
			if(!mib_get(MIB_IPF_OUT_ACTION, (void *)buffer) ){
				return -1;
			}
			if (0 == buffer[0])
				strcpy(strbuf, "Deny");
			else if(1 == buffer[0])
				strcpy(strbuf, "Allow");
			else
				strcpy(strbuf, "err");
			break;

#ifdef PORT_FORWARD_GENERAL
		case SYS_DEFAULT_PORT_FW_ACTION:
			if(!mib_get(MIB_PORT_FW_ENABLE, (void *)buffer) ){
				return -1;
			}
			if (0 == buffer[0])
				strcpy(strbuf, "Disable");
			else if(1 == buffer[0])
				strcpy(strbuf, "Enable");
			else
				strcpy(strbuf, "err");
			break;
#endif
#if defined(CONFIG_RTL_IGMP_SNOOPING)
		// Added by Jenny
		case SYS_IGMP_SNOOPING:
			if(!mib_get( MIB_MPMODE, (void *)&vChar)) {
				return -1;
			}
			if (vChar&MP_IGMP_MASK)
				strcpy(strbuf, STR_ENABLE);
			else
				strcpy(strbuf, STR_DISABLE);
			break;
#endif
		case SYS_IP_QOS:
			if(!mib_get( MIB_MPMODE, (void *)&vChar)) {
				return -1;
			}
			if (vChar&MP_IPQ_MASK)
				strcpy(strbuf, STR_ENABLE);
			else
				strcpy(strbuf, STR_DISABLE);
			break;


		// Added by Mason Yu
		case SYS_IPF_IN_ACTION:
			if ( !mib_get( MIB_IPF_IN_ACTION, (void *)buffer) )
				return -1;
			if (0 == buffer[0])
				strcpy(strbuf, "Deny");
			else if(1 == buffer[0])
				strcpy(strbuf, "Allow");
			else
				strcpy(strbuf, "err");
			break;

#ifdef CONFIG_SECONDARY_IP
		case SYS_LAN_IP2:
			if (!mib_get( MIB_ADSL_LAN_ENABLE_IP2, (void *)&vChar))
				return -1;
			if (vChar == 0)
				strcpy(strbuf, STR_DISABLE);
			else
				strcpy(strbuf, STR_ENABLE);
			break;

		case SYS_LAN_DHCP_POOLUSE:
			if (!mib_get( MIB_ADSL_LAN_DHCP_POOLUSE, (void *)&vChar))
				return -1;
			if (vChar == 0)
				strcpy(strbuf, "Primary LAN");
			else
				strcpy(strbuf, "Secondary LAN");
			break;
#endif

#ifdef URL_BLOCKING_SUPPORT
		case SYS_DEFAULT_URL_BLK_ACTION:
			if(!mib_get(MIB_URL_CAPABILITY, (void *)buffer) ){
				return -1;
			}
			if (0 == buffer[0])
				strcpy(strbuf, "Disable");
			else if(1 == buffer[0])
				strcpy(strbuf, "Enable");
			else
				strcpy(strbuf, "err");
			break;
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
		case SYS_DEFAULT_DOMAIN_BLK_ACTION:
			if(!mib_get(MIB_DOMAINBLK_CAPABILITY, (void *)buffer) ){
				return -1;
			}
			if (0 == buffer[0])
				strcpy(strbuf, "Disable");
			else if(1 == buffer[0])
				strcpy(strbuf, "Enable");
			else
				strcpy(strbuf, "err");
			break;
#endif
#ifdef CONFIG_DEV_xDSL
		case SYS_DSL_OPSTATE:
			getAdslInfo(ADSL_GET_MODE, buffer, 64);
			if (buffer[0] != '\0')
				sprintf(strbuf, "%s,", buffer);
			getAdslInfo(ADSL_GET_STATE, buffer, 64);
			strcat(strbuf, buffer);
			break;
#ifdef CONFIG_USER_XDSL_SLAVE
		case SYS_DSL_SLV_OPSTATE:
			getAdslSlvInfo(ADSL_GET_MODE, buffer, 64);
			if (buffer[0] != '\0')
				sprintf(strbuf, "%s,", buffer);
			getAdslSlvInfo(ADSL_GET_STATE, buffer, 64);
			strcat(strbuf, buffer);
			break;
#endif /*CONFIG_USER_XDSL_SLAVE*/
#endif
#if defined(CONFIG_RTL_8676HWNAT) || defined(CONFIG_USER_VLAN_ON_LAN)
		case SYS_LAN1_VID:
		case SYS_LAN2_VID:
		case SYS_LAN3_VID:
		case SYS_LAN4_VID:
			mib_chain_get(MIB_SW_PORT_TBL, id - SYS_LAN1_VID, &sw_entry);
			sprintf(strbuf, "%hu", sw_entry.vid);
			break;
		case SYS_LAN1_STATUS:
		case SYS_LAN2_STATUS:
		case SYS_LAN3_STATUS:
		case SYS_LAN4_STATUS:
			getLANPortStatus(id - SYS_LAN1_STATUS, strbuf);
			break;
#endif
#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
		case SYS_DHCPV6_MODE:
			if(!mib_get( MIB_DHCPV6_MODE, (void *)buffer) )
				return -1;
			strcpy(strbuf, dhcp_mode[(DHCP_TYPE_T)buffer[0]]);
			break;

		case SYS_DHCPV6_RELAY_UPPER_ITF:
			if(!mib_get( MIB_DHCPV6R_UPPER_IFINDEX, (void *)&vUInt) )
				return -1;

			if (vUInt != DUMMY_IFINDEX)
			{
				ifGetName(vUInt, strbuf, 8);
			}
			else
			{
				snprintf(strbuf, 6, "None");
				return 0;
			}
			break;
#endif
		case SYS_LAN_IP6_LL:
			i=getifip6(LANIF, IPV6_ADDR_LINKLOCAL, ip6_addr, 1);
			if (!i)
				strbuf[0] = 0;
			else
				inet_ntop(PF_INET6, &ip6_addr[0].addr, tmpBuf, INET6_ADDRSTRLEN);
				sprintf(strbuf, "%s/%d", tmpBuf, ip6_addr[0].prefix_len);
			break;
		case SYS_LAN_IP6_GLOBAL:
			i=getifip6(LANIF, IPV6_ADDR_UNICAST, ip6_addr, 6);
			strbuf[0] = 0;
			if (i) {
				for (len=0; len<i; len++) {
					inet_ntop(PF_INET6, &ip6_addr[len].addr, tmpBuf, INET6_ADDRSTRLEN);
					if (len == 0)
						sprintf(strbuf, "%s/%d", tmpBuf, ip6_addr[len].prefix_len);
					else
						sprintf(strbuf, "%s, %s/%d", strbuf, tmpBuf, ip6_addr[len].prefix_len);
				}
			}
			break;
#endif // of CONFIG_IPV6

		default:
			return -1;
	}

	return 0;
}

int ifWanNum(const char *name)
{
	int ifnum=0;
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	int type;
	unsigned char protocol = IPVER_IPV4_IPV6; // 1:IPv4, 2: IPv6, 3: Both. Mason Yu. MLD Proxy

	if ( !strcmp(name, "all") )
		type = 0;
	else if ( !strcmp(name, "rt") )
	{
		type = 1;	// route interface
		protocol = IPVER_IPV4;
	}
	else if ( !strcmp(name, "rtv6") )	// Mason Yu. MLD Proxy
	{
		type = 1;	// route interface
		protocol = IPVER_IPV6;
	}
	else if ( !strcmp(name, "br") )
		type = 2;	// bridge interface
	else
		type = 1;	// default to route

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);

	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			return -1;
		}

		if (Entry.enable == 0)
			continue;
// Mason Yu. MLD Proxy
#ifdef CONFIG_IPV6
		if ( protocol == IPVER_IPV4_IPV6 || Entry.IpProtocol == IPVER_IPV4_IPV6 || protocol == Entry.IpProtocol ) {
			if (type == 2) {
				if (Entry.cmode == CHANNEL_MODE_BRIDGE)
				{
					ifnum++;
				}
			}
			else {  // rt or all (1 or 0)
				if (type == 1 && Entry.cmode == CHANNEL_MODE_BRIDGE)
					continue;

				ifnum++;
			}
		}
#else
		if (type == 2) {
			if (Entry.cmode == CHANNEL_MODE_BRIDGE)
			{
				ifnum++;
			}
		}
		else {  // rt or all (1 or 0)
			if (type == 1 && Entry.cmode == CHANNEL_MODE_BRIDGE)
				continue;

			ifnum++;
		}
#endif

	}

	return ifnum;
}

#ifdef REMOTE_ACCESS_CTL
void remote_access_modify(MIB_CE_ACC_T accEntry, int enable)
{
	char *act;
	char strPort[16];
	int ret;
#ifdef _CWMP_MIB_
	unsigned int cwmp_connreq_port = 0;
	unsigned char cwmp_flag = 0;
#endif

	if (enable)
		act = (char *)FW_INSERT;
	else
		act = (char *)FW_DEL;

#ifdef _CWMP_MIB_
	mib_get(CWMP_CONREQ_PORT, &cwmp_connreq_port);

	if (cwmp_connreq_port) {
		mib_get(CWMP_FLAG, &cwmp_flag);

		// can WAN access
		if (cwmp_flag | CWMP_FLAG_AUTORUN) {
			snprintf(strPort, sizeof(strPort), "%u",
				 cwmp_connreq_port);

			va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
			       (char *)FW_PREROUTING, "!", (char *)ARG_I,
			       (char *)LANIF, "-p", (char *)ARG_TCP,
			       (char *)FW_DPORT, strPort, "-j", (char *)"MARK",
			       "--set-mark", RMACC_MARK);
		}
	}
#endif

	// telnet service: bring up by inetd
#ifdef CONFIG_USER_TELNETD_TELNETD
	if (!(accEntry.telnet & 0x02)) {	// not LAN access
		// iptables -I inacc -i $LAN_IF -p TCP --dport 23 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		       (char *)FW_DPORT, "23", "-j", (char *)FW_DROP);
	}
	if (accEntry.telnet & 0x01) {	// can WAN access
		snprintf(strPort, sizeof(strPort), "%u", accEntry.telnet_port);
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_TCP, (char *)FW_DPORT, strPort, "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);

		// redirect if this is not standard port
		if (accEntry.telnet_port != 23) {
			ret = va_cmd(IPTABLES, 15, 1, "-t", "nat",
				     (char *)act, (char *)FW_PREROUTING,
				     "!", (char *)ARG_I, (char *)LANIF,
				     "-p", ARG_TCP,
				     (char *)FW_DPORT, strPort, "-j",
				     "REDIRECT", "--to-ports", "23");
		}
	}
#endif

#ifdef CONFIG_USER_FTPD_FTPD
	// ftp service: bring up by inetd
	if (!(accEntry.ftp & 0x02)) {	// not LAN access
		// iptables -I inacc -i $LAN_IF -p TCP --dport 21 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		       (char *)FW_DPORT, "21", "-j", (char *)FW_DROP);
	}
	if (accEntry.ftp & 0x01) {	// can WAN access
		snprintf(strPort, sizeof(strPort), "%u", accEntry.ftp_port);
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_TCP, (char *)FW_DPORT, strPort, "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);

		// redirect if this is not standard port
		if (accEntry.ftp_port != 21) {
			ret = va_cmd(IPTABLES, 15, 1, "-t", "nat",
				     (char *)act, (char *)FW_PREROUTING,
				     "!", (char *)ARG_I, (char *)LANIF,
				     "-p", ARG_TCP,
				     (char *)FW_DPORT, strPort, "-j",
				     "REDIRECT", "--to-ports", "21");
		}
	}
#endif

#ifdef CONFIG_USER_TFTPD_TFTPD
	// tftp service: bring up by inetd
	if (!(accEntry.tftp & 0x02)) {	// not LAN access
		// iptables -I inacc -i $LAN_IF -p UDP --dport 69 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		       (char *)FW_DPORT, "69", "-j", (char *)FW_DROP);
	}
	if (accEntry.tftp & 0x01) {	// can WAN access
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_UDP, (char *)FW_DPORT, "69", "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);
	}
#endif

	// HTTP service
	if (!(accEntry.web & 0x02)) {	// not LAN access
		// iptables -I inacc -i $LAN_IF -p TCP --dport 80 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		       (char *)FW_DPORT, "80", "-j", (char *)FW_DROP);
	}
	if (accEntry.web & 0x01) {	// can WAN access
		snprintf(strPort, sizeof(strPort), "%u", accEntry.web_port);
		// iptables -t mangle -I PREROUTING ! -i $LAN_IF -p TCP --dport 80 -j MARK --set-mark 0x1000
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_TCP, (char *)FW_DPORT, strPort, "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);

		if (accEntry.web_port != 80) {
			va_cmd(IPTABLES, 15, 1, "-t", "nat",
			       (char *)act, (char *)FW_PREROUTING,
			       "!", (char *)ARG_I, (char *)LANIF,
			       "-p", ARG_TCP,
			       (char *)FW_DPORT, strPort, "-j",
			       "REDIRECT", "--to-ports", "80");
		}

	}
#ifdef CONFIG_USER_BOA_WITH_SSL
	//HTTPS service
	if (!(accEntry.https & 0x02)) {	// not LAN access
		// iptables -I inacc -i $LAN_IF -p TCP --dport 443 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		       (char *)FW_DPORT, "443", "-j", (char *)FW_DROP);
	}
	if (accEntry.https & 0x01) {	// can WAN access
		snprintf(strPort, sizeof(strPort), "%u", accEntry.https_port);
		// iptables -t mangle -I PREROUTING ! -i $LAN_IF -p TCP --dport 443 -j MARK --set-mark 0x1000
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_TCP, (char *)FW_DPORT, strPort, "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);

		if (accEntry.https_port != 443) {
			va_cmd(IPTABLES, 15, 1, "-t", "nat",
			       (char *)act, (char *)FW_PREROUTING,
			       "!", (char *)ARG_I, (char *)LANIF,
			       "-p", ARG_TCP,
			       (char *)FW_DPORT, strPort, "-j",
			       "REDIRECT", "--to-ports", "443");
		}

	}
#endif

#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_NETSNMP_SNMPD)
	// snmp service
	if (!(accEntry.snmp & 0x02)) {	// not LAN access
		// iptables -I inacc -i $LAN_IF -p UDP --dport 161:162 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		       (char *)FW_DPORT, "161:162", "-j", (char *)FW_DROP);
	}
	if (accEntry.snmp & 0x01) {	// can WAN access
		va_cmd(IPTABLES, 21, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_UDP, (char *)FW_DPORT, "161:162", "-m",
		       "limit", "--limit", "100/s", "--limit-burst", "500",
		       "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
	}
#endif

#if defined(CONFIG_USER_SSH_DROPBEAR)||defined(CONFIG_USER_SSH_DROPBEAR_2016_73)
	// ssh service
	if (!(accEntry.ssh & 0x02)) {	// not LAN access
		// iptables -I inacc -i $LAN_IF -p TCP --dport 22 -j DROP
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
		       (char *)FW_DPORT, "22", "-j", (char *)FW_DROP);
	}
	if (accEntry.ssh & 0x01) {	// can WAN access
		snprintf(strPort, sizeof(strPort), "%u", accEntry.ssh_port);
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_TCP, (char *)FW_DPORT, strPort, "-j",
		       (char *)"MARK", "--set-mark", RMACC_MARK);

		if (accEntry.ssh_port != 22) {
			va_cmd(IPTABLES, 15, 1, "-t", "nat",
			       (char *)act, (char *)FW_PREROUTING,
			       "!", (char *)ARG_I, (char *)LANIF,
			       "-p", ARG_TCP,
			       (char *)FW_DPORT, strPort, "-j",
			       "REDIRECT", "--to-ports", "22");
		}
	}
#endif

	// ping service
	if (!(accEntry.icmp & 0x02))	// not LAN access
	{
		// iptables -I INPUT -i $LAN_IF -p ICMP --icmp-type echo-request -j ACCEPT
		va_cmd(IPTABLES, 10, 1, act, (char *)FW_INACC,
		       (char *)ARG_I, (char *)LANIF, "-p", "ICMP",
		       "--icmp-type", "echo-request", "-j", (char *)FW_DROP);
	}
	if (accEntry.icmp & 0x01)	// can WAN access
	{
		va_cmd(IPTABLES, 19, 1, ARG_T, "mangle", act,
		       (char *)FW_PREROUTING, "!", (char *)ARG_I, (char *)LANIF,
		       "-p", (char *)ARG_ICMP, "--icmp-type", "echo-request",
		       "-m", "limit", "--limit", "1/s", "-j", (char *)"MARK",
		       "--set-mark", RMACC_MARK);
	}

#ifdef CONFIG_USER_NETLOGGER_SUPPORT
	// can WAN access
	snprintf(strPort, sizeof(strPort), "%u", 0x1234);
	va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", act, (char *)FW_PREROUTING,
	       "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
	       (char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark",
	       RMACC_MARK);
#endif //CONFIG_USER_NETLOGGER_SUPPORT
}

void filter_set_remote_access(int enable)
{
	MIB_CE_ACC_T accEntry;
	if (!mib_chain_get(MIB_ACC_TBL, 0, (void *)&accEntry))
		return;
	remote_access_modify( accEntry, enable );

	return;

}
#endif // of REMOTE_ACCESS_CTL

#ifdef IP_ACL
void set_acl_service(unsigned char service, char *act, char *strIP, int isIPrange, char *protocol, unsigned short port)
{
	char strInputPort[8]="";
	
	snprintf(strInputPort, sizeof(strInputPort), "%d", port);
	if (service & 0x02) {	// can LAN access
		if (isIPrange) {
			// iptables -A inacc -i $LAN_IF -m iprange --src-range x.x.x.x-1x.x.x.x -p UDP --dport 69 -j ACCEPT
			va_cmd(IPTABLES, 14, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF, "-m", "iprange", "--src-range", strIP, "-p", protocol,
				(char *)FW_DPORT, strInputPort, "-j", (char *)FW_ACCEPT);
		}
		else {
			// iptables -A inacc -i $LAN_IF -s xxx.xxx.xxx.xxx -p UDP --dport 69 -j ACCEPT
			va_cmd(IPTABLES, 12, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF, "-s", strIP, "-p", protocol,
				(char *)FW_DPORT, strInputPort, "-j", (char *)FW_ACCEPT);
		}
	}
	if (service & 0x01) {	// can WAN access
		if (isIPrange) {
			// iptables -A inacc -i ! $LAN_IF -m iprange --src-range x.x.x.x-1x.x.x.x -p UDP --dport 69 -j ACCEPT
			va_cmd(IPTABLES, 15, 1, act, (char *)FW_ACL,
				"!", (char *)ARG_I, (char *)LANIF, "-m", "iprange", "--src-range", strIP, "-p", protocol,
				(char *)FW_DPORT, strInputPort, "-j", (char *)FW_ACCEPT);
		}
		else {
			// iptables -A inacc -i ! $LAN_IF -s xxx.xxx.xxx.xxx -p UDP --dport 69 -j ACCEPT
			va_cmd(IPTABLES, 13, 1, act, (char *)FW_ACL,
				"!", (char *)ARG_I, (char *)LANIF, "-s", strIP, "-p", protocol,
				(char *)FW_DPORT, strInputPort, "-j", (char *)FW_ACCEPT);
		}
	}
}

static void set_acl_service_redirect(unsigned char service, unsigned char Interface, unsigned short portRedirect, char *act, char *strIP, int isIPrange, char *protocol, unsigned short port)
{
	char strInputPort[16];
	char strPort[16];
	
	snprintf(strInputPort, sizeof(strInputPort), "%hu", port);	
	if (Interface == IF_DOMAIN_LAN && service & 0x02) {	//can  LAN access
		if (isIPrange) {
			// iptables -A aclblock -i $LAN_IF -m iprange --src-range x.x.x.x-1x.x.x.x -p TCP --dport 23 -j ACCEPT
			va_cmd(IPTABLES, 14, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF,  "-m", "iprange", "--src-range", strIP, "-p", protocol,
				(char *)FW_DPORT, strInputPort, "-j", (char *)FW_ACCEPT);
		}
		else {
			// iptables -A aclblock -i $LAN_IF -s 192.168.1.0/24 -p TCP --dport 23  -j ACCEPT
			va_cmd(IPTABLES, 12, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF, "-s", strIP, "-p", protocol,
				(char *)FW_DPORT, strInputPort, "-j", (char *)FW_ACCEPT);
		}
	}
	if (Interface == IF_DOMAIN_WAN && service & 0x01) {	// can WAN access
		snprintf(strPort, sizeof(strPort), "%hu", portRedirect);
		if (isIPrange) {			
			va_cmd(IPTABLES, 19, 1, ARG_T, "mangle", act, (char *)FW_ACL,
				"!", (char *)ARG_I, (char *)LANIF, "-m", "iprange", "--src-range", strIP, "-p", protocol,
				(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
	
			// redirect if this is not standard port
			if (portRedirect != port) {
				va_cmd(IPTABLES, 19, 1, "-t", "nat",
						(char *)act, (char *)FW_ACL,
						"!", (char *)ARG_I, (char *)LANIF, "-m", "iprange", "--src-range", strIP,
						"-p", protocol,
						(char *)FW_DPORT, strPort, "-j",
						"REDIRECT", "--to-ports", strInputPort);
			}
		}
		else {			
			va_cmd(IPTABLES, 17, 1, ARG_T, "mangle", act, (char *)FW_ACL,
				"!", (char *)ARG_I, (char *)LANIF, "-s", strIP, "-p", (char *)protocol,
				(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
	
			// redirect if this is not standard port
			if (portRedirect != port) {
				va_cmd(IPTABLES, 17, 1, "-t", "nat",
						(char *)act, (char *)FW_ACL,
						"!", (char *)ARG_I, (char *)LANIF, "-s", strIP,
						"-p", protocol,
						(char *)FW_DPORT, strPort, "-j",
						"REDIRECT", "--to-ports", strInputPort);
			}
		}
	}
}

static void filter_set_acl_service(MIB_CE_ACL_IP_T Entry, unsigned char Interface, int enable, char *strIP, int isIPrange)
{
	char *act;
	char strPort[16];
	int ret;


	if (enable)
		act = (char *)FW_ADD;
	else
		act = (char *)FW_DEL;

#ifdef CONFIG_USER_TELNETD_TELNETD
	// telnet service: bring up by inetd
	set_acl_service_redirect(Entry.telnet, Interface, Entry.telnet_port, act, strIP, isIPrange, (char *)ARG_TCP, 23);
#endif

#ifdef CONFIG_USER_FTPD_FTPD
	// ftp service: bring up by inetd
	set_acl_service_redirect(Entry.ftp, Interface, Entry.ftp_port, act, strIP, isIPrange, (char *)ARG_TCP, 21);
#endif	

#ifdef CONFIG_USER_TFTPD_TFTPD
	// tftp service: bring up by inetd
	set_acl_service_redirect(Entry.tftp, Interface, 69, act, strIP, isIPrange, (char *)ARG_UDP, 69);
#endif

	// HTTP service
	set_acl_service_redirect(Entry.web, Interface, Entry.web_port, act, strIP, isIPrange, (char *)ARG_TCP, 80);

#ifdef CONFIG_USER_BOA_WITH_SSL
  	//HTTPS service
	set_acl_service_redirect(Entry.https, Interface, Entry.https_port, act, strIP, isIPrange, (char *)ARG_TCP, 443);
#endif

#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
	// snmp service	
	if (Interface == IF_DOMAIN_LAN && Entry.snmp & 0x02) {	// can LAN access
		if (isIPrange) {
			// iptables -A aclblock -i $LAN_IF -m iprange --src-range x.x.x.x-x.x.x.x -p UDP --dport 161:162 -j ACCEPT
			va_cmd(IPTABLES, 14, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF, "-m", "iprange", "--src-range", strIP, "-p", (char *)ARG_UDP,
				(char *)FW_DPORT, "161:162", "-j", (char *)FW_ACCEPT);
		} else {
			// iptables -A aclblock -i $LAN_IF -s 192.168.1.0/24 -p UDP --dport 161:162 -j ACCEPT
			va_cmd(IPTABLES, 12, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF, "-s", strIP, "-p", (char *)ARG_UDP,
				(char *)FW_DPORT, "161:162", "-j", (char *)FW_ACCEPT);
		}
	}
	if (Interface == IF_DOMAIN_WAN && Entry.snmp & 0x01) {	// can WAN access
		if (isIPrange) {
			// iptables -t mangle -A aclblock ! -i $LAN_IF -m iprange --src-range x.x.x.x-x.x.x.x -p UDP --dport 161:162 -m limit
			//  --limit 100/s --limit-burst 500 -j MARK --set-mark 0x1000
			va_cmd(IPTABLES, 25, 1, ARG_T, "mangle", act, (char *)FW_ACL,
				"!", (char *)ARG_I, (char *)LANIF, "-m", "iprange", "--src-range", strIP, "-p",
				(char *)ARG_UDP, (char *)FW_DPORT, "161:162", "-m",
				"limit", "--limit", "100/s", "--limit-burst",
				"500", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
		} else {
			// iptables -t mangle -A aclblock ! -i $LAN_IF -s 192.168.1.0/24 -p UDP --dport 161:162 -m limit
			//  --limit 100/s --limit-burst 500 -j MARK --set-mark 0x1000
			va_cmd(IPTABLES, 23, 1, ARG_T, "mangle", act, (char *)FW_ACL,
				"!", (char *)ARG_I, (char *)LANIF, "-s", strIP, "-p",
				(char *)ARG_UDP, (char *)FW_DPORT, "161:162", "-m",
				"limit", "--limit", "100/s", "--limit-burst",
				"500", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
		}
	}
#endif

#ifdef CONFIG_USER_SSH_DROPBEAR
	// ssh service
	set_acl_service_redirect(Entry.ssh, Interface, Entry.ssh_port, act, strIP, isIPrange, (char *)ARG_TCP, 22);
#endif

	// ping service
	if (Interface == IF_DOMAIN_LAN && Entry.icmp & 0x02) // can LAN access
	{
		if (isIPrange) {
			// iptables -A aclblock -i $LAN_IF -m iprange --src-range x.x.x.x-x.x.x.x -p ICMP --icmp-type echo-request -m limit
			//   --limit 1/s -j ACCEPT
			va_cmd(IPTABLES, 18, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF, "-m", "iprange", "--src-range", strIP, "-p", "ICMP",
				"--icmp-type", "echo-request", "-m", "limit",
				"--limit", "1/s", "-j", (char *)FW_ACCEPT);
		} else {
			// iptables -A aclblock -i $LAN_IF -s 192.168.1.0/12 -p ICMP --icmp-type echo-request -m limit
			//   --limit 1/s -j ACCEPT
			va_cmd(IPTABLES, 16, 1, act, (char *)FW_ACL,
				(char *)ARG_I, (char *)LANIF, "-s", strIP, "-p", "ICMP",
				"--icmp-type", "echo-request", "-m", "limit",
				"--limit", "1/s", "-j", (char *)FW_ACCEPT);
		}
	}
	if (Interface == IF_DOMAIN_WAN && Entry.icmp & 0x01) // can WAN access
	{
		if (isIPrange) {
			// iptables -t mangle -A aclblock ! -i $LAN_IF -m iprange --src-range x.x.x.x-x.x.x.x -p ICMP --icmp-type echo-request -m limit
			//   --limit 1/s -j MARK --set-mark 0x1000
			va_cmd(IPTABLES, 23, 1, ARG_T, "mangle", act, (char *)FW_ACL,
				"!", (char *)ARG_I, (char *)LANIF, "-m", "iprange", "--src-range", strIP, "-p", "ICMP",
				"--icmp-type", "echo-request", "-m", "limit",
				"--limit", "1/s", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
		} else {
			// iptables -t mangle -A aclblock ! -i $LAN_IF -s 192.168.1.0/12 -p ICMP --icmp-type echo-request -m limit
			//   --limit 1/s -j MARK --set-mark 0x1000
			va_cmd(IPTABLES, 21, 1, ARG_T, "mangle", act, (char *)FW_ACL,
				"!", (char *)ARG_I, (char *)LANIF, "-s", strIP, "-p", "ICMP",
				"--icmp-type", "echo-request", "-m", "limit",
				"--limit", "1/s", "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
		}
	}	
}

static filter_acl_flush()
{
	va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_ACL);
	// Flush all rule in nat table
	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", (char *)FW_ACL);
	// Flush all rule in nat table
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)FW_ACL);
}

/*
 * In general, we don't allow device access from wan except those specified in this function.
 */
static filter_acl_allow_wan()
{
	unsigned char dhcpMode;
	unsigned char vChar;
	
	// iptables -A aclblock -i lo -j RETURN
	// Local Out DNS query will refer the /etc/resolv.conf(DNS server is 127.0.0.1). We should accept this DNS query when enable ACL.
	va_cmd(IPTABLES, 6, 1, (char *)FW_ADD, (char *)FW_ACL, "-i", "lo", "-j", (char *)FW_RETURN);		
	
	// allow for WAN
	// (1) allow service with the same mark value(0x1000) from WAN
	va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_ACL,
		"!", (char *)ARG_I, LANIF, "-m", "mark", "--mark", RMACC_MARK, "-j", FW_ACCEPT);
			
	// (2) Added by Mason Yu for dhcp Relay. Open DHCP Relay Port for Incoming Packets.
#ifdef CONFIG_USER_DHCP_SERVER
	if (mib_get(MIB_DHCP_MODE, &dhcpMode) != 0)
	{
		if (dhcpMode == 1 || dhcpMode == 2 ){
			// iptables -A aclblock -i ! br0 -p udp --dport 67 -j ACCEPT
			va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_ACL, "!", (char *)ARG_I, BRIF, "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_ACCEPT);
		}
	}
#endif

#ifdef CONFIG_USER_ROUTED_ROUTED
	// (3) Added by Mason Yu. Open RIP Port for Incoming Packets.
	if (mib_get( MIB_RIP_ENABLE, (void *)&vChar) != 0)
	{
		if (1 == vChar)
		{
			// iptables -A aclblock -i ! br0 -p udp --dport 520 -j ACCEPT
			va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_ACL, "!", "-i", BRIF, "-p", "udp", (char *)FW_DPORT, "520", "-j", (char *)FW_ACCEPT);
		}
	}
#endif
	// iptables -A aclblock -i ! $LAN_IF   -p ICMP --icmp-type echo-request -m limit
	//   --limit 1/s -j DROP
	va_cmd(IPTABLES, 15, 1, (char *)FW_ADD, (char *)FW_ACL,
		"!", (char *)ARG_I, (char *)LANIF, "-p", "ICMP",
		"--icmp-type", "echo-request", "-m", "limit",
		"--limit", "1/s", "-j", (char *)FW_DROP);
		
	// accept related	
	// iptables -A aclblock -m state --state ESTABLISHED,RELATED -j ACCEPT
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-m", "state",
		"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);
}

void filter_set_acl(int enable)
{
	int i, total;
	struct in_addr src;
	char ssrc[40];
	MIB_CE_ACL_IP_T Entry;

#ifdef ACL_IP_RANGE
	struct in_addr start,end;
	char sstart[16],send[16];
#endif

#ifdef MAC_ACL
	MIB_CE_ACL_MAC_T MacEntry;
	char macaddr[18];
#endif

	// Added by Mason Yu for ACL.
	// check if ACL Capability is enabled ?
	if (!enable) {
		filter_acl_flush();
		filter_acl_allow_wan();
		// For security issue, we do NOT allow connection attemp from wan even if ACL is disabled.
		// iptables -A -i ! br0 aclblock -j DROP
		va_cmd(IPTABLES, 7, 1, (char *)FW_ADD, (char *)FW_ACL, "!", (char *)ARG_I, (char *)LANIF, "-j", (char *)FW_DROP);
	}
	else {
		filter_acl_flush();
		// Add policy to aclblock chain
		total = mib_chain_total(MIB_ACL_IP_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&Entry))
			{
				return;
			}

//    		printf("\nstartip=%x,endip=%x\n",*(unsigned long*)Entry.startipAddr,*(unsigned long*)Entry.endipAddr);

			// Check if this entry is enabled
			if ( Entry.Enabled == 1 ) {
#ifdef ACL_IP_RANGE
				start.s_addr = *(unsigned long*)Entry.startipAddr;
				end.s_addr = *(unsigned long*)Entry.endipAddr;
				strcpy(sstart, inet_ntoa(start));
				strcpy(send, inet_ntoa(end));
				strcpy(ssrc,sstart);
				strcat(ssrc,"-");
				strcat(ssrc,send);

#else
				src.s_addr = *(unsigned long *)Entry.ipAddr;

				// inet_ntoa is not reentrant, we have to
				// copy the static memory before reuse it
				snprintf(ssrc, sizeof(ssrc), "%s/%hhu", inet_ntoa(src), Entry.maskbit);

#endif
		        	if ( Entry.Interface == IF_DOMAIN_LAN ) {
#ifdef ACL_IP_RANGE
					if(*(unsigned long*)Entry.startipAddr != *(unsigned long*)Entry.endipAddr) {
						if (Entry.any == 0x01) { 	// service = any(0x01)
					// iptables -A aclblock -m iprange --src-range x.x.x.x-1x.x.x.x -j RETURN
							va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_ACL, "-i", BRIF, "-m", "iprange", "--src-range", ssrc, "-j", (char *)FW_RETURN);
						}
						else 
							filter_set_acl_service(Entry, Entry.Interface, enable, ssrc, 1);
						
					}
					else {
						if (Entry.any == 0x01)  	// service = any(0x01)
							va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-i", BRIF, "-s", sstart, "-j", (char *)FW_RETURN);
					else
							filter_set_acl_service(Entry, Entry.Interface, enable, sstart, 0);
					}
#else
					if (Entry.any == 0x01)  	// service = any(0x01)
					// iptables -A INPUT -s xxx.xxx.xxx.xxx
						va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-i", BRIF, "-s", ssrc, "-j", (char *)FW_RETURN);
					else
						filter_set_acl_service(Entry, Entry.Interface, enable, ssrc, 0);
#endif
				} else {
					// iptables -A INPUT -s xxx.xxx.xxx.xxx
					//va_cmd(IPTABLES, 9, 1, (char *)FW_ADD, "aclblock", "!", "-i", BRIF, "-s", ssrc, "-j", (char *)FW_RETURN);
#ifdef ACL_IP_RANGE
					if(*(unsigned long*)Entry.startipAddr != *(unsigned long*)Entry.endipAddr) {
						filter_set_acl_service(Entry, Entry.Interface, enable, ssrc, 1);
					}
					else {
						filter_set_acl_service(Entry, Entry.Interface, enable, sstart, 0);
					}
						
#else
					filter_set_acl_service(Entry, Entry.Interface, enable, ssrc, 0);
#endif					
				}
			}
		}
#ifdef MAC_ACL
		total = mib_chain_total(MIB_ACL_MAC_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_ACL_MAC_TBL, i, (void *)&MacEntry))
			{
				return;
			}

			// Check if this entry is enabled
			if ( MacEntry.Enabled == 1 ) {
				snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
					MacEntry.macAddr[0], MacEntry.macAddr[1],
					MacEntry.macAddr[2], MacEntry.macAddr[3],
					MacEntry.macAddr[4], MacEntry.macAddr[5]);

		        if ( MacEntry.Interface == IF_DOMAIN_LAN ) {
		 			// iptables -A aclblock -i br0  -m mac --mac-source $MAC -j RETURN
					va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_ACL,
							(char *)ARG_I, BRIF, "-m", "mac",
							"--mac-source",  macaddr, "-j", (char *)FW_RETURN);
				} else {
					// iptables -A aclblock -i ! br0  -m mac --mac-source $MAC -j RETURN
					va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_ACL,
							 "!", (char *)ARG_I, BRIF, "-m", "mac",
							"--mac-source",  macaddr, "-j", (char *)FW_RETURN);
				}
			}
		}
#endif
		// (1) allow for LAN
		// allowing DNS request during ACL enabled
		// iptables -A aclblock -p udp -i br0 --dport 53 -j RETURN
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "udp", "-i", BRIF,(char*)FW_DPORT, (char *)PORT_DNS, "-j", (char *)FW_RETURN);
		// allowing DHCP request during ACL enabled
		// iptables -A aclblock -p udp -i br0 --dport 67 -j RETURN
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "udp", "-i", BRIF, (char*)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_RETURN);

		filter_acl_allow_wan();
		
		// iptables -A aclblock -j DROP
		va_cmd(IPTABLES, 4, 1, (char *)FW_ADD, (char *)FW_ACL, "-j", (char *)FW_DROP);
	}
}
#endif

#ifdef TCP_UDP_CONN_LIMIT
void set_conn_limit(void)
{
	int i, total;
	char ssrc[20];
	char connNum[10];
	MIB_CE_CONN_LIMIT_T Entry;

	// Add policy to connlimit chain
	//iptables -A connlimit -m state --state RELATED,ESTABLISHED -j RETURN
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, "connlimit", "-m", "state",
		"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_RETURN);

	total = mib_chain_total(MIB_CONN_LIMIT_TBL);
	for (i=0; i<total; i++) {
		if (!mib_chain_get(MIB_CONN_LIMIT_TBL, i, (void *)&Entry))
			continue;

		// Check if this entry is enabled
		if ( Entry.Enabled == 1 ) {
			// inet_ntoa is not reentrant, we have to
			// copy the static memory before reuse it
			strncpy(ssrc, inet_ntoa(*((struct in_addr *)&Entry.ipAddr)), 16);
			ssrc[15] = '\0';

			snprintf(connNum, 10, "%d", Entry.connNum);

			// iptables -A connlimit -i br0 -s xxx.xxx.xxx.xxx
			va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "connlimit", "-s", ssrc, "-m", "iplimit",
				"--iplimit-above", connNum, "--iplimit-mask", "255.255.255.255", "-j", "REJECT");
		}
	}
}
//#endif

int restart_connlimit(void)
{
	unsigned char connlimitEn;

	va_cmd(IPTABLES, 2, 1, "-F", "connlimit");

	mib_get(MIB_CONNLIMIT_ENABLE, (void *)&connlimitEn);
	if (connlimitEn == 1)
		set_conn_limit();
}

void set_conn_limit(void)
{
	int 					i, total;
	char 				ssrc[20];
	char 				connNum[10];
	MIB_CE_TCP_UDP_CONN_LIMIT_T Entry;

	// Add policy to connlimit chain, allow all established ~
	//iptables -A connlimit -m state --state RELATED,ESTABLISHED -j RETURN
	va_cmd(IPTABLES, 8, 1, (char *)FW_ADD, "connlimit", "-m", "state",
		"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_RETURN);

	total = mib_chain_total(MIB_TCP_UDP_CONN_LIMIT_TBL);
	for (i=0; i<total; i++) {
		if (!mib_chain_get(MIB_TCP_UDP_CONN_LIMIT_TBL, i, (void *)&Entry))
			continue;
		// Check if this entry is enabled and protocol is TCP
		if (( Entry.Enabled == 1 ) && ( Entry.protocol == 0 )) {
			// inet_ntoa is not reentrant, we have to
			// copy the static memory before reuse it
			strncpy(ssrc, inet_ntoa(*((struct in_addr *)&Entry.ipAddr)), 16);
			ssrc[15] = '\0';
			snprintf(connNum, 10, "%d", Entry.connNum);

			// iptables -A connlimit -p tcp -s 192.168.1.99 -m iplimit --iplimit-above 2 -j REJECT
			va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "connlimit","-p","tcp", "-s", ssrc, "-m", "iplimit",
				"--iplimit-above", connNum, "-j", "REJECT");
		}
		// UDP
		else if  (( Entry.Enabled == 1 ) && ( Entry.protocol == 1 )) {
			strncpy(ssrc, inet_ntoa(*((struct in_addr *)&Entry.ipAddr)), 16);
			ssrc[15] = '\0';
			snprintf(connNum, 10, "%d", Entry.connNum);

			//iptables -A connlimit -p udp -s 192.168.1.99 -m udplimit --udplimit-above 2 -j REJECT
			va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "connlimit","-p","udp", "-s", ssrc, "-m", "udplimit",
				"--udplimit-above", connNum, "-j", "REJECT");
		}

	}

	//The Global rules goes last ~
	//TCP
	mib_get(MIB_CONNLIMIT_TCP, (void *)&i);
	if (i >0)
	{
		snprintf(connNum, 10, "%d", i);
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, "connlimit","-p","tcp",  "-m", "iplimit",
					"--iplimit-above", connNum, "-j", "REJECT");
	}
	//UDP
	mib_get(MIB_CONNLIMIT_UDP, (void *)&i);
	if (i >0)
	{
		snprintf(connNum, 10, "%d", i);
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, "connlimit","-p","udp",  "-m", "udplimit",
					"--udplimit-above", connNum, "-j", "REJECT");
	}

}
#endif//TCP_UDP_CONN_LIMIT

#ifdef DOS_SUPPORT
void DoS_syslog(int signum)
{
	FILE *fp;
	char buff[1024];

	buff[0] = 0;
	printf("%s enter.\n", __func__);
	fp = fopen("/proc/dos_syslog","r");
	if(fp) {
		fgets(buff, sizeof(buff), fp);
		if (buff[0])
			syslog(LOG_WARNING, "%s", buff);
		fclose(fp);
	}
}
#endif

#ifdef URL_BLOCKING_SUPPORT
// Added by Mason Yu for URL Blocking
void filter_set_url(int enable)
{

	int i, j, total, totalKeywd;
	MIB_CE_URL_FQDN_T Entry;
	MIB_CE_KEYWD_FILTER_T entry;

	//Kevin, check does it need to enable/disable IP fastpath status
	UpdateIpFastpathStatus();

	// check if URL Capability is enabled ?

	if (!enable)
	{
		va_cmd(IPTABLES, 2, 1, "-F", "urlblock");
	}
	else {

		// Add URL policy to urlblock chain
		total = mib_chain_total(MIB_URL_FQDN_TBL);

		for (i=0; i<total; i++) {
			char testURL[MAX_URL_LENGTH+2];
			if (!mib_chain_get(MIB_URL_FQDN_TBL, i, (void *)&Entry))
			{
				return;
			}

			strcpy(testURL, Entry.fqdn);
			//strcat(testURL, "\r\n");
		 	// iptables -A urlblock -p tcp --dport 80 -m string --url "tw.yahoo.com" -j DROP
			//va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "urlblock", "-p", "tcp", "--dport", "80", "-m", "string", "--url", Entry.fqdn, "-j", (char *)FW_DROP);
			va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, "urlblock", "-p", "tcp", "--dport", "80", "-m", "string", "--url", testURL, "--algo", "bm", "-j", (char *)FW_DROP);
		}

		// Add Keyword filtering policy to urlblock chain
		totalKeywd = mib_chain_total(MIB_KEYWD_FILTER_TBL);

		for (i=0; i<totalKeywd; i++) {
			if (!mib_chain_get(MIB_KEYWD_FILTER_TBL, i, (void *)&entry))
			{
				return;
			}

		 	// iptables -A urlblock -p tcp --dport 80 -m string --url "tw.yahoo.com" -j DROP
			va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, "urlblock", "-p", "tcp", "--dport", "80", "-m", "string", "--url", entry.keyword, "--algo", "bm", "-j", (char *)FW_DROP);
		}

		// Kill all conntrack
		va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
#ifdef CONFIG_RTK_L34_ENABLE
		RTK_RG_URL_Filter_Set();
#endif
	}

}

int restart_urlblocking(void)
{
	unsigned char urlEnable;

	va_cmd(IPTABLES, 2, 1, "-F", "urlblock");

#ifdef CONFIG_RTK_L34_ENABLE
	Flush_RTK_RG_URL_Filter();
#endif

	mib_get(MIB_URL_CAPABILITY, (void *)&urlEnable);
	if (urlEnable == 1)  // URL Capability enabled
		filter_set_url(1);
	else
		filter_set_url(0);
	return 0;
}

#ifdef URL_ALLOWING_SUPPORT
void set_url(int enable)
{
	int i, j, total, totalKeywd;
	MIB_CE_URL_FQDN_T Entry;

	// check if URL Capability is enabled ?

	if (!enable)
		va_cmd(IPTABLES, 2, 1, "-F", "urlallow");
	else {
		// Add URL policy to urlblock chain
		total = mib_chain_total(MIB_URL_ALLOW_FQDN_TBL);

		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_URL_ALLOW_FQDN_TBL, i, (void *)&Entry))
			{
				return;
			}

		 	// iptables -A urlallow -p tcp --dport 80 -m string --urlalw "tw.yahoo.com" --algo bm -j ACCEPT
			va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, "urlallow", "-p", "tcp", "--dport", "80", "-m", "string", "--urlalw", Entry.fqdn, "--algo", "bm", "-j", (char *)FW_ACCEPT);
		}
		//&endofurl& is flag for drop other urls
		va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, "urlallow", "-p", "tcp", "--dport", "80", "-m", "string", "--urlalw", "&endofurl&", "--algo", "bm", "-j", (char *)FW_DROP);
	}
}

int restart_url(void)
{
	unsigned char urlEnable;

	va_cmd(IPTABLES, 2, 1, "-F", "urlallow");
	va_cmd(IPTABLES, 2, 1, "-F", "urlblock");
	mib_get(MIB_URL_CAPABILITY, (void *)&urlEnable);
	if (urlEnable == 1)  // URL Capability enabled
		filter_set_url(1);
	else if(urlEnable == 2) //URL allow
		set_url(2);

	return 0;
}
#endif
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
void patch_for_office365(void)
{
	/* wpad.domain.name should be forbidden by router */
	char blkDomain[MAX_DOMAIN_LENGTH]="wpad.domain.name";
	unsigned char *needle_tmp, *str;
	char len[MAX_DOMAIN_GROUP];
	unsigned char seg[MAX_DOMAIN_GROUP][MAX_DOMAIN_SUB_STRING];
	unsigned char cmpStr[MAX_DOMAIN_LENGTH]="\0";
	int j, k;

	needle_tmp = blkDomain;

	for (j=0; (str =strchr(needle_tmp, '.'))!= NULL; j++)
	{
		*str = '\0';

		strncpy(seg[j]+1, needle_tmp, (MAX_DOMAIN_SUB_STRING - 1));
		seg[j][MAX_DOMAIN_SUB_STRING - 1]='\0';
		//printf(" seg[%d]= %s...(1)\n", j, seg[j]);

		//seg[j][0]= len[j];
		seg[j][0] = strlen(needle_tmp);
		//printf(" seg[%d]= %s...(2)\n", j, seg[j]);

		needle_tmp = str+1;
	}

	// calculate the laster domain sub string lengh and form the laster compare sub string
	strncpy(seg[j]+1, needle_tmp, (MAX_DOMAIN_SUB_STRING - 1));
	seg[j][MAX_DOMAIN_SUB_STRING - 1]='\0';

	seg[j][0]= strlen(needle_tmp);
	//printf(" seg[%d]= %s...(3)\n", j, seg[j]);

	// Merge the all compare sub string into a final compare string.
	for ( k=0; k<=j; k++) {
		//printf(" seg[%d]= %s", k, seg[k]);
		strcat(cmpStr, seg[k]);
	}
    //printf("cmpStr=%s len=%d\n", cmpStr, strlen(cmpStr));
	//printf("\n");

	// iptables -A domainblk -p udp --dport 53 -m string --domain yahoo.com --algo bm -j DROP
	va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, "domainblk", "-p", "udp", "--dport", "53", "-m", "string", "--domain", cmpStr, "--algo", "bm", "-j", (char *)FW_DROP);
#ifdef CONFIG_IPV6
	// ipv6 filter
	// ip6tables -A FORWARD -p udp --dport 53 -m string --domain yahoo.com --algo bm -j DROP
	va_cmd(IP6TABLES, 14, 1, (char *)FW_DEL, (char *)FW_FORWARD, "-p", "udp", "--dport", "53", "-m", "string", "--domain", cmpStr, "--algo", "bm", "-j", (char *)FW_DROP);
	va_cmd(IP6TABLES, 14, 1, (char *)FW_ADD, (char *)FW_FORWARD, "-p", "udp", "--dport", "53", "-m", "string", "--domain", cmpStr, "--algo", "bm", "-j", (char *)FW_DROP);
#endif
}

// Added by Mason Yu for Domain Blocking
void filter_set_domain(int enable)
{
	int i, total;
	MIB_CE_DOMAIN_BLOCKING_T Entry;
	unsigned char sdest[MAX_DOMAIN_LENGTH];
	int j, k;
	unsigned char *needle_tmp, *str;
	char len[MAX_DOMAIN_GROUP];
	unsigned char seg[MAX_DOMAIN_GROUP][MAX_DOMAIN_SUB_STRING];
	unsigned char cmpStr[MAX_DOMAIN_LENGTH]="\0";

	//Kevin, check does it need to enable/disable IP fastpath status
	UpdateIpFastpathStatus();

	// check if Domain Blocking Capability is enabled ?
	if (!enable)
		va_cmd(IPTABLES, 2, 1, "-F", "domainblk");
	else {
		// Add policy to domainblk chain
		total = mib_chain_total(MIB_DOMAIN_BLOCKING_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_DOMAIN_BLOCKING_TBL, i, (void *)&Entry))
			{
				return;
			}

			// Mason Yu
			// calculate domain sub string lengh and form the compare sub string.
			// Foe example, If the domain is aa.bbb.cccc, the compare sub string 1 is 0x02 0x61 0x61.
			// The compare sub string 2 is 0x03 0x62 0x62 0x62. The compare sub string 3 is 0x04 0x63 0x63 0x63 0x63.
			needle_tmp = Entry.domain;

			for (j=0; (str =strchr(needle_tmp, '.'))!= NULL; j++)
			{
				*str = '\0';

				strncpy(seg[j]+1, needle_tmp, (MAX_DOMAIN_SUB_STRING - 1));
				seg[j][MAX_DOMAIN_SUB_STRING - 1]='\0';
				//printf(" seg[%d]= %s...(1)\n", j, seg[j]);

				//seg[j][0]= len[j];
				seg[j][0] = strlen(needle_tmp);
				//printf(" seg[%d]= %s...(2)\n", j, seg[j]);

				needle_tmp = str+1;
			}

			// calculate the laster domain sub string lengh and form the laster compare sub string
			strncpy(seg[j]+1, needle_tmp, (MAX_DOMAIN_SUB_STRING - 1));
			seg[j][MAX_DOMAIN_SUB_STRING - 1]='\0';

			seg[j][0]= strlen(needle_tmp);
			//printf(" seg[%d]= %s...(3)\n", j, seg[j]);

			// Merge the all compare sub string into a final compare string.
			for ( k=0; k<=j; k++) {
				//printf(" seg[%d]= %s", k, seg[k]);
				strcat(cmpStr, seg[k]);
				//printf(" cmpStr=%s\n", cmpStr);
			}
			//printf("\n");

		 	// iptables -A domainblk -p udp --dport 53 -m string --domain yahoo.com -j DROP
			va_cmd(IPTABLES, 14, 1, (char *)FW_ADD, "domainblk", "-p", "udp", "--dport", "53", "-m", "string", "--domain", cmpStr, "--algo", "bm", "-j", (char *)FW_DROP);
			cmpStr[0] = '\0';
		}
	}
}

int restart_domainBLK(void)
{
	unsigned char domainEnable;

	va_cmd(IPTABLES, 2, 1, "-F", "domainblk");
	mib_get(MIB_DOMAINBLK_CAPABILITY, (void *)&domainEnable);
	if (domainEnable == 1)  // domain blocking Capability enabled
		filter_set_domain(1);
	else
		filter_set_domain(0);
	return 0;
}
#endif

#if defined(CONFIG_USER_ROUTED_ROUTED) || defined(CONFIG_USER_ZEBRA_OSPFD_OSPFD)
#ifdef CONFIG_00R0
static void sync_rip_config(void)
{
	MIB_CE_ATM_VC_T wan_entry;
	MIB_CE_RIP_T rip_entry;
	int total_wan = 0, total_rip = 0;
	int i, j;

	total_wan = mib_chain_total(MIB_ATM_VC_TBL);

	for(i = 0 ; i < total_wan ; i++)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, &wan_entry) == 0)
			continue;

		// entry may be deleted, so get total entries again
		total_rip = mib_chain_total(MIB_RIP_TBL);
		for(j = 0 ; j < total_rip ; j++)
		{
			if(mib_chain_get(MIB_RIP_TBL, j, &rip_entry) == 0)
				continue;

			if(wan_entry.ifIndex != rip_entry.ifIndex)
				continue;

			if(!wan_entry.enable || !wan_entry.enableRIPv2)
			{
				//deconfig
				if(rip_entry.add_by_wan)
					mib_chain_delete(MIB_RIP_TBL, j);
			}
			else if(wan_entry.enable)
			{
				if(wan_entry.enableRIPv2)
				{
					//update setting
					rip_entry.add_by_wan = 1;
					rip_entry.ifIndex = wan_entry.ifIndex;
					rip_entry.receiveMode = RIP_V2;
					rip_entry.sendMode = RIP_V2;
					mib_chain_update(MIB_RIP_TBL, &rip_entry, j);
				}
				#if 0
				else if(rip_entry.receiveMode == RIP_V2 && rip_entry.sendMode == RIP_V2)
				{
					//update wan settings
					wan_entry.enableRIPv2 = 1;
					mib_chain_update(MIB_ATM_VC_TBL, &wan_entry, i);
					rip_entry.add_by_wan = 1;
					mib_chain_update(MIB_RIP_TBL, &rip_entry, j);
				}
				#endif
			}

			break;
		}

		// Add new entry
		if(j == total_rip && wan_entry.enable && wan_entry.enableRIPv2)
		{
			unsigned char rip_enable = 1;
			memset(&rip_entry, 0, sizeof(MIB_CE_RIP_T));

			rip_entry.add_by_wan = 1;
			rip_entry.ifIndex = wan_entry.ifIndex;
			rip_entry.receiveMode = RIP_V2;
			rip_entry.sendMode = RIP_V2;

			mib_set(MIB_RIP_ENABLE, &rip_enable);
			mib_chain_add(MIB_RIP_TBL, &rip_entry);
		}
	}
}
#endif

static const char ROUTED_CONF[] = "/var/run/routed.conf";

int startRip(void)
{
	FILE *fp;
	unsigned char ripOn, ripInf, ripVer;
	int rip_pid;

	unsigned int entryNum, i;
	MIB_CE_RIP_T Entry;
	char ifname[IFNAMSIZ], receive_mode[5], send_mode[5];

#ifdef CONFIG_00R0
	sync_rip_config();
#endif

	if (mib_get(MIB_RIP_ENABLE, (void *)&ripOn) == 0)
		ripOn = 0;
	rip_pid = read_pid((char *)ROUTED_PID);
	if (rip_pid >= 1) {
		// kill it
		if (kill(rip_pid, SIGTERM) != 0) {
			printf("Could not kill pid '%d'", rip_pid);
		}
	}

	if (!ripOn)
		return 0;

	printf("start rip!\n");
	if ((fp = fopen(ROUTED_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", ROUTED_CONF);
		return -1;
	}

#if 0
	if (mib_get(MIB_RIP_VERSION, (void *)&ripVer) == 0)
		ripVer = 1;	// default version 2

	fprintf(fp, "version %u\n", ripVer+1);

	// LAN interface
	if (mib_get(MIB_ADSL_LAN_RIP, (void *)&ripOn) != 0)
	{
		if (ripOn)
			fprintf(fp, "network br0\n");
	}

	// WAN interface
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);

	for (i=0; i<vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;

		if (Entry.enable == 0)
			continue;

		if (Entry.cmode != CHANNEL_MODE_BRIDGE && Entry.rip)
		{
			ifGetName(Entry.ifIndex, ifname, sizeof(ifname));
			fprintf(fp, "network %s\n", ifname);
		}
	}
#endif

	entryNum = mib_chain_total(MIB_RIP_TBL);

	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_RIP_TBL, i, (void *)&Entry))
		{
			fclose(fp);
  			printf("Get MIB_RIP_TBL chain record error!\n");
			return;
		}

		if (!ifGetName(Entry.ifIndex, ifname, sizeof(ifname))) {
			strncpy(ifname, BRIF, strlen(BRIF));
			ifname[strlen(BRIF)] = '\0';
		}

		fprintf(fp, "network %s %d %d\n", ifname, Entry.receiveMode, Entry.sendMode);
	}

	fclose(fp);
	// Modified by Mason Yu for always as a supplier for RIP
	//va_cmd(ROUTED, 0, 0);
	// routed will fork, so do wait to avoid zombie process
	va_cmd(ROUTED, 1, 1, "-s");
	return 1;
}
#endif	// of CONFIG_USER_ROUTED_ROUTED

#ifdef CONFIG_USER_ZEBRA_OSPFD_OSPFD
static const char ZEBRA_CONF[] = "/etc/config/zebra.conf";
static const char OSPFD_CONF[] = "/etc/config/ospfd.conf";

// OSPF server configuration
// return value:
// 0  : not enabled
// 1  : successful
// -1 : startup failed
int startOspf(void)
{
	FILE *fp;
	char *argv[6];
	int pid;
	unsigned char ospfOn;
	unsigned int entryNum, i, j;
	MIB_CE_OSPF_T Entry;
	char *netIp[20];
	unsigned int uMask;
	unsigned int uIp;

	if (mib_get(MIB_OSPF_ENABLE, (void *)&ospfOn) == 0)
		ospfOn = 0;
	//kill old zebra
	pid = read_pid((char *)ZEBRA_PID);
	if (pid >= 1) {
		if (kill(pid, SIGTERM) != 0) {
			printf("Could not kill pid '%d'\n", pid);
		}
	}
	//kill old ospfd
	pid = read_pid((char *)OSPFD_PID);
	if (pid >= 1) {
		if (kill(pid, SIGTERM) != 0)
			printf("Could not kill pid '%d'\n", pid);
	}

	if (!ospfOn)
		return 0;

	printf("start ospf!\n");
	//create zebra.conf
	if ((fp = fopen(ZEBRA_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", ZEBRA_CONF);
		return -1;
	}
	fprintf(fp, "hostname Router\n");
	fprintf(fp, "password zebra\n");
	fprintf(fp, "enable password zebra\n");
	fclose(fp);

	//create ospfd.conf
	if ((fp = fopen(OSPFD_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", OSPFD_CONF);
		return -1;
	}
	fprintf(fp, "hostname ospfd\n");
	fprintf(fp, "password zebra\n");
	fprintf(fp, "enable password zebra\n");
	fprintf(fp, "router ospf\n");
	//ql_xu test.
	//fprintf(fp, "network %s area 0\n", "192.168.2.0/24");
	entryNum = mib_chain_total(MIB_OSPF_TBL);

	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_OSPF_TBL, i, (void *)&Entry))
		{
  			printf("Get MIB_OSPF_TBL chain record error!\n");
			return -1;
		}

		uIp = *(unsigned int *)Entry.ipAddr;
		uMask = *(unsigned int *)Entry.netMask;
		uIp = uIp & uMask;
		strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));

		for (j=0; j<32; j++)
			if ((uMask>>j) & 0x01)
				break;
		uMask = 32 - j;

		sprintf(netIp, "%s/%d", netIp, uMask);
		fprintf(fp, "network %s area 0\n", netIp);
	}
	fclose(fp);

	//start zebra
	argv[1] = "-d";
	argv[2] = "-k";
	argv[3] = NULL;
	TRACE(STA_SCRIPT, "%s %s %s\n", ZEBRA, argv[1], argv[2]);
	do_cmd(ZEBRA, argv, 0);

	//start ospfd
	argv[1] = "-d";
	argv[2] = NULL;
	TRACE(STA_SCRIPT, "%s %s\n", OSPFD, argv[1]);
	do_cmd(OSPFD, argv, 0);

	return 1;
}
#endif

#ifdef CONFIG_ATM_REALTEK
#include <rtk/linux/ra867x_atm.h>
#include <linux/atm.h>
#endif

//up_flag:0, itf down; non-zero: itf up
void itfcfg(char *if_name, int up_flag)
{
#ifdef EMBED
	int fd;
	struct ifreq ifr;
	char cmd;

	if (strncmp(if_name, "sar", sizeof("sar"))==0) {  //sar enable/disable
#ifdef CONFIG_ATM_REALTEK
		struct atmif_sioc mysio;

		if((fd = socket(PF_ATMPVC, SOCK_DGRAM, 0)) < 0){
			printf("socket open error\n");
			return;
		}
		mysio.number = 0;
		mysio.length = sizeof(struct SAR_IOCTL_CFG);
		mysio.arg = (void *)NULL;
		if (up_flag) ioctl(fd, SIOCDEVPRIVATE+2, &mysio);
		else         ioctl(fd, SIOCDEVPRIVATE+3, &mysio);
		if(fd!=(int)NULL)
		    close(fd);
#else
		printf( "%s: SAR interface not supportted.\n", __FUNCTION__ );
#endif
	}
	else if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
		int flags;
		if (getInFlags(if_name, &flags) == 1) {
			if (up_flag)
				flags |= IFF_UP;
			else
				flags &= ~IFF_UP;

			setInFlags(if_name, flags);
		}
		close(fd);
	}
#endif
}

#ifdef XOR_ENCRYPT
const char XOR_KEY[] = "tecomtec";
void xor_encrypt(char *inputfile, char *outputfile)
{
	FILE *input  = fopen(inputfile, "rb");
	FILE *output = fopen(outputfile, "wb");

	if(input != NULL && output != NULL) {
		unsigned char buffer[MAX_CONFIG_FILESIZE];
		size_t count, i, j = 0;
		do {
			count = fread(buffer, sizeof *buffer, sizeof buffer, input);
			for(i = 0; i<count; ++i) {
				buffer[i] ^= XOR_KEY[j++];
				if(XOR_KEY[j] == '\0')
					j = 0; /* restart at the beginning of the key */
			}
			fwrite(buffer, sizeof *buffer, count, output);
		} while (count == sizeof buffer);
		fclose(input);
		fclose(output);
	}
}
#endif

unsigned short
ipchksum(unsigned char *ptr, int count, unsigned short resid)
{
	register unsigned int sum = resid;
       if ( count==0)
       	return(sum);

	while(count > 1) {
		//sum += ntohs(*ptr);
		sum += (( ptr[0] << 8) | ptr[1] );
		if ( sum>>31)
			sum = (sum&0xffff) + ((sum>>16)&0xffff);
		ptr += 2;
		count -= 2;
	}

	if (count > 0)
		sum += (*((unsigned char*)ptr) << 8) & 0xff00;

	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	if (sum == 0xffff)
		sum = 0;
	return (unsigned short)sum;
}

int getLinkStatus(struct ifreq *ifr)
{
	int status=0;

#ifdef __mips__
#if defined(CONFIG_RTL8672NIC)
	if (do_ioctl(SIOCGMEDIALS, ifr) == 1)
		status = 1;
#endif
#endif
	return status;
}

/***port forwarding APIs*******/
#ifdef PORT_FORWARD_GENERAL
/*move from startup.c:iptable_fw()*/
static void iptable_fw( int del, int negate, const char *ifname, const char *proto, const char *remoteIP, const char *extPort, const char *dstIP)
{
	char *act;

	if(del) act = (char *)FW_DEL;
	else act = (char *)FW_ADD;


//	if (negate && remoteIP) {
	if (negate && remoteIP && extPort) {
		va_cmd(IPTABLES, 17, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			 "!", (char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			"-s", (char *)remoteIP,
			(char *)FW_DPORT, extPort, "-j",
			"DNAT", "--to-destination", dstIP);
	} else if (negate && remoteIP) {
		va_cmd(IPTABLES, 15, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			 "!", (char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			"-s", (char *)remoteIP,
			"-j", "DNAT", "--to-destination", dstIP);
//	} else if (negate) {
	} else if (negate && extPort) {
		va_cmd(IPTABLES, 15, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			 "!", (char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			(char *)FW_DPORT, extPort, "-j",
			"DNAT", "--to-destination", dstIP);
//	} else if (remoteIP) {
	} else if (remoteIP && extPort) {
		va_cmd(IPTABLES, 16, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			(char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			"-s", (char *)remoteIP,
			(char *)FW_DPORT, extPort, "-j",
			"DNAT", "--to-destination", dstIP);
	} else if (negate) {
		va_cmd(IPTABLES, 13, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			 "!", (char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			"-j", "DNAT", "--to-destination", dstIP);
	} else if (remoteIP) {
		va_cmd(IPTABLES, 14, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			(char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			"-s", (char *)remoteIP,
			"-j", "DNAT", "--to-destination", dstIP);
//	} else {
	} else if (extPort) {
		va_cmd(IPTABLES, 14, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			(char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			(char *)FW_DPORT, extPort, "-j",
			"DNAT", "--to-destination", dstIP);
	} else {
		va_cmd(IPTABLES, 12, 1, "-t", "nat",
			(char *)act,	(char *)PORT_FW,
			(char *)ARG_I, (char *)ifname,
			"-p", (char *)proto,
			 "-j", "DNAT", "--to-destination", dstIP);
	}

}

/*move from startup.c:iptable_filter()*/
static void iptable_filter( int del, int negate, const char *ifname, const char *proto, const char *remoteIP, const char *intPort)
{
	char *act, *rulenum;

	if(del)
	{
		act = (char *)FW_DEL;

//		if (negate && remoteIP) {
		if (negate && remoteIP && intPort) {
			va_cmd(IPTABLES, 15, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				(char *)FW_DPORT, intPort,
				"-j",(char *)FW_ACCEPT);
		} else if (negate && remoteIP) {
			va_cmd(IPTABLES, 13, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				"-j",(char *)FW_ACCEPT);
//		} else if (negate) {
		} else if (negate && intPort) {
			va_cmd(IPTABLES, 13, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				(char *)FW_DPORT, intPort,
				"-j",(char *)FW_ACCEPT);
//		} else if (remoteIP) {
		} else if (remoteIP && intPort) {
			va_cmd(IPTABLES, 14, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				(char *)FW_DPORT, intPort,
				"-j",(char *)FW_ACCEPT);
		} else if (negate) {
			va_cmd(IPTABLES, 11, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-j",(char *)FW_ACCEPT);
		} else if (remoteIP) {
			va_cmd(IPTABLES, 12, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				"-j",(char *)FW_ACCEPT);
//		} else {
		} else if (intPort) {
			va_cmd(IPTABLES, 12, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				(char *)FW_DPORT, intPort, "-j",
				(char *)FW_ACCEPT);
		} else {
			va_cmd(IPTABLES, 10, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-j", (char *)FW_ACCEPT);
		}
	}else
	{
 		act = (char *)FW_ADD;

//		if (negate && remoteIP) {
		if (negate && remoteIP && intPort) {
			va_cmd(IPTABLES, 15, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				(char *)FW_DPORT, intPort,
				"-j",(char *)FW_ACCEPT);
		} else if (negate && remoteIP) {
			va_cmd(IPTABLES, 13, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				"-j",(char *)FW_ACCEPT);
//		} else if (negate) {
		} else if (negate && intPort) {
			va_cmd(IPTABLES, 13, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				(char *)FW_DPORT, intPort,
				"-j",(char *)FW_ACCEPT);
//		} else if (remoteIP) {
		} else if (remoteIP && intPort) {
			va_cmd(IPTABLES, 14, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				(char *)FW_DPORT, intPort,
				"-j",(char *)FW_ACCEPT);
		} else if (negate) {
			va_cmd(IPTABLES, 11, 1, act,
				(char *)PORT_FW, "!", (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-j",(char *)FW_ACCEPT);
		} else if (remoteIP) {
			va_cmd(IPTABLES, 12, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-s", remoteIP,
				"-j",(char *)FW_ACCEPT);
//		} else {
		} else if (intPort) {
			va_cmd(IPTABLES, 12, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				(char *)FW_DPORT, intPort, "-j",
				(char *)FW_ACCEPT);
		} else {
			va_cmd(IPTABLES, 10, 1, act,
				(char *)PORT_FW, (char *)ARG_I, ifname,
				(char *)ARG_O,
				(char *)LANIF, "-p", (char *)proto,
				"-j", (char *)FW_ACCEPT);
		}
	}
}

/*move from startup.c ==> part of setupFirewall()*/
void portfw_modify( MIB_CE_PORT_FW_T *p, int del)
{
	int negate=0, hasRemote=0, hasLocalPort=0, hasExtPort=0;
	char * proto = 0;
	char intPort[32], extPort[32];
	char ipaddr[32], extra[32], ifname[IFNAMSIZ];

	if(p==NULL) return;

#if 0
	{
		fprintf( stderr,"<portfw_modify>\n" );
		fprintf( stderr,"\taction:%s\n", (del==0)?"ADD":"DEL" );
		fprintf( stderr,"\tifIndex:0x%x\n", p->ifIndex );
		fprintf( stderr,"\tenable:%u\n", p->enable );
		fprintf( stderr,"\tleaseduration:%u\n", p->leaseduration );
		fprintf( stderr,"\tremotehost:%s\n", inet_ntoa(*((struct in_addr *)p->remotehost))  );
		fprintf( stderr,"\texternalport:%u\n", p->externalport );
		fprintf( stderr,"\tinternalclient:%s\n", inet_ntoa(*((struct in_addr *)p->ipAddr)) );
		fprintf( stderr,"\tinternalport:%u\n", p->toPort );
		fprintf( stderr,"\tprotocol:%u\n", p->protoType ); /*PROTO_TCP=1, PROTO_UDP=2*/
		fprintf( stderr,"<end portfw_modify>\n" );
	}
#endif

	if( del==0 ) //add
	{
		char vCh=0;
		mib_get(MIB_PORT_FW_ENABLE, (void *)&vCh);
		if(vCh==0) return;

		if (!p->enable) return;
	}

/*	snprintf(intPort, 12, "%u", p->fromPort);

	if (p->externalport) {
		snprintf(extPort, sizeof(extPort), "%u", p->externalport);
		snprintf(intPort, sizeof(intPort), "%u", p->fromPort);
		snprintf(ipaddr, sizeof(ipaddr), "%s:%s", inet_ntoa(*((struct in_addr *)p->ipAddr)), intPort);
	} else {
		snprintf(intPort, sizeof(extPort), "%u", p->fromPort);
		strncpy(extPort, intPort, sizeof(intPort));
		strncpy(ipaddr, inet_ntoa(*((struct in_addr *)p->ipAddr)), 16);
	}
	*/
    if (p->fromPort)
    {
        if (p->fromPort == p->toPort)
        {
            snprintf(intPort, sizeof(intPort), "%u", p->fromPort);
        }
        else
        {
            /* "%u-%u" is used by port forwarding */
            snprintf(intPort, sizeof(intPort), "%u-%u", p->fromPort, p->toPort);
        }

        snprintf(ipaddr, sizeof(ipaddr), "%s:%s", inet_ntoa(*((struct in_addr *)p->ipAddr)), intPort);

        if (p->fromPort != p->toPort)
        {
            /* "%u:%u" is used by filter */
            snprintf(intPort, sizeof(intPort), "%u:%u", p->fromPort, p->toPort);
        }
        hasLocalPort = 1;
    }
    else
    {
        snprintf(ipaddr, sizeof(ipaddr), "%s", inet_ntoa(*((struct in_addr *)p->ipAddr)));
        hasLocalPort = 0;
    }

	if (p->externalfromport && p->externaltoport && (p->externalfromport != p->externaltoport)) {
		snprintf(extPort, sizeof(extPort), "%u:%u", p->externalfromport, p->externaltoport);
		hasExtPort = 1;
	} else if (p->externalfromport) {
		snprintf(extPort, sizeof(extPort), "%u", p->externalfromport);
		hasExtPort = 1;
	} else if (p->externaltoport) {
		snprintf(extPort, sizeof(extPort), "%u", p->externaltoport);
		hasExtPort = 1;
	} else {
		hasExtPort = 0;
	}
	//printf( "extPort:%s hasExtPort=%d\n",  extPort, hasExtPort);
	//printf( "entry.externalfromport:%d entry.externaltoport=%d\n",  p->externalfromport, p->externaltoport);

	if (ifGetName(p->ifIndex, ifname, sizeof(ifname)))
		negate = 0;
	else {
		strcpy(ifname, LANIF);
		negate = 1;
	}

	if (p->remotehost[0]) {
		snprintf(extra, sizeof(extra), "%s", inet_ntoa(*((struct in_addr *)p->remotehost)));
		hasRemote = 1;
	} else {
		hasRemote = 0;
	}

	//fprintf( stderr, "ipaddr:%s, intPort:%s\n", ipaddr, intPort );
	//check something
	//internalclient can't be zeroip
	if( strncmp(ipaddr,"0.0.0.0", 7)==0 ) return;
	//internalport can't be zero
//	if( strcmp(intPort,"0")==0 ) return;
	//fprintf( stderr, "Pass ipaddr:%s, intPort:%s\n", ipaddr, intPort );

	if (p->protoType == PROTO_TCP || p->protoType == PROTO_UDPTCP)
	{
		// iptables -t nat -A PREROUTING -i ! $LAN_IF -p TCP --dport dstPortRange -j DNAT --to-destination ipaddr
//		iptable_fw( del, negate, ifname, ARG_TCP, hasRemote ? extra : 0, extPort, ipaddr);
		iptable_fw( del, negate, ifname, ARG_TCP, hasRemote ? extra : 0, hasExtPort ? extPort : 0, ipaddr);
		// iptables -I ipfilter 3 -i ! $LAN_IF -o $LAN_IF -p TCP --dport dstPortRange -j RETURN
//		iptable_filter( del, negate, ifname, ARG_TCP, hasRemote ? extra : 0, intPort);
		iptable_filter( del, negate, ifname, ARG_TCP, hasRemote ? extra : 0, hasLocalPort ? intPort : 0);
#ifdef PORT_FORWARD_GENERAL
#ifdef NAT_LOOPBACK
		iptable_fw_natLB( del, p->ifIndex, ARG_TCP, hasExtPort ? extPort : 0, hasLocalPort ? intPort : 0, ipaddr);
#endif
#endif
	}

	if (p->protoType == PROTO_UDP || p->protoType == PROTO_UDPTCP)
	{
//		iptable_fw( del, negate, ifname, ARG_UDP, hasRemote ? extra : 0, extPort, ipaddr);
		iptable_fw( del, negate, ifname, ARG_UDP, hasRemote ? extra : 0, hasExtPort ? extPort : 0, ipaddr);
		// iptables -I ipfilter 3 -i ! $LAN_IF -o $LAN_IF -p UDP --dport dstPortRange -j RETURN
//		iptable_filter( del, negate, ifname, ARG_UDP, hasRemote ? extra : 0, intPort);
		iptable_filter( del, negate, ifname, ARG_UDP, hasRemote ? extra : 0, hasLocalPort ? intPort : 0);
#ifdef PORT_FORWARD_GENERAL
#ifdef NAT_LOOPBACK
		iptable_fw_natLB( del, p->ifIndex, ARG_UDP, hasExtPort ? extPort : 0, hasLocalPort ? intPort : 0, ipaddr);
#endif
#endif
	}
}
#endif // of PORT_FORWARD_GENERAL

/***end port forwarding APIs*******/

#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
int configWizardRedirect(int redirect_flag)
{
	unsigned char value[6] = {0}, ipaddr[16] = {0}, lan_ip_port[32] = {0};

	if (mib_get(MIB_ADSL_LAN_IP, (void *)value) != 0) {
		strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
		ipaddr[15] = '\0';
		sprintf(lan_ip_port, "%s:80", ipaddr);

		va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", "WebRedirect_Wizard");

		if (redirect_flag) {
			//iptables -t nat -A WebRedirect_Wizard -p tcp -d 192.168.0.1 --dport 80 -j RETURN
			va_cmd(IPTABLES, 14, 1, "-t", "nat", "-A", "WebRedirect_Wizard",	"-i", "br+", "-p", "tcp", "-d", ipaddr, "--dport", "80", "-j", (char *)FW_RETURN);

			//iptables -t nat -A WebRedirect_Wizard -p tcp --dport 80 -j DNAT --to-destination 192.168.0.1:80
			va_cmd(IPTABLES, 14, 1, "-t", "nat", "-A", "WebRedirect_Wizard", "-i", "br+", "-p", "tcp", "--dport", "80", "-j", "DNAT", "--to-destination", lan_ip_port);
			va_cmd(IPTABLES, 14, 1, "-t", "nat", "-A", "WebRedirect_Wizard", "-i", "br+", "-p", "tcp", "--dport", "443", "-j", "DNAT", "--to-destination", lan_ip_port);
		}
	}
	return 0;
}

int update_redirect_by_ponStatus(void)
{
	int isConnect = 0, need_redirect = 0;
	rtk_gpon_fsm_status_t state;

	if (rtk_gpon_ponStatus_get(&state) == RT_ERR_OK) {
		if (state > RTK_GPONMAC_FSM_STATE_O1) {
			isConnect = 1;
		}

		unsigned char internet_status = 0;
		mib_get(MIB_USER_TROUBLE_WIZARD_INTERNET_STATUS, (void *)&internet_status);
		if (isConnect == 0 && internet_status) {
			need_redirect = 1;
		}
	}
	else
		need_redirect = 1;

	if (need_redirect == 1) {
		configWizardRedirect(1);
	}
	else {
		configWizardRedirect(0);
	}

	// update dnsmasq
	int pid = -1;
	pid = read_pid("/var/run/systemd.pid");
	if (pid > 0)
		kill(pid, SIGUSR1);

	return 0;
}
#endif

#ifdef TIME_ZONE
// return value:
// 1  : successful
// -1 : failed
int startNTP(void)
{
	int status=1;
	char vChar;
	char sVal[32], sTZ[16], *pStr;
	char sVal2[32]={0};
	FILE *fp;
	unsigned int index = 0, ntpItf;
	unsigned char dst_enabled = 1;
	char ifname[IFNAMSIZ] = {0};

	mib_get( MIB_NTP_ENABLED, (void *)&vChar);
	if (vChar == 1)
	{
		if (mib_get(MIB_NTP_TIMEZONE_DB_INDEX, &index)) {
#ifdef __UCLIBC__
			if ((fp = fopen("/etc/TZ", "w")) != NULL) {
				mib_get(MIB_DST_ENABLED, &dst_enabled);
				fprintf(fp, "%s\n", get_tz_string(index, dst_enabled));
				fclose(fp);
			}
#else
	#ifdef CONFIG_USER_GLIBC_TIMEZONE
			char cmd[128];
			char tz_location[64]={0};
			
			system("rm /var/localtime");
			mib_get(MIB_DST_ENABLED, &dst_enabled);
			if (dst_enabled || !is_tz_dst_exist(index)) {
				snprintf(tz_location,sizeof(tz_location),"%s",get_tz_location(index, FOR_CLI));
				format_tz_location(tz_location);
					
				snprintf(cmd, sizeof(cmd), "ln -s /usr/share/zoneinfo/%s /var/localtime",tz_location);
			} else {
				const char * tz_offset;
				
				tz_offset = get_format_tz_utc_offset(index);
				
				snprintf(cmd, sizeof(cmd), "ln -s /usr/share/zoneinfo/Etc/GMT%s /var/localtime",tz_offset);
			}
			
			//fprintf(stderr,cmd);
			system(cmd);
	#endif
#endif
		}

		mib_get( MIB_NTP_SERVER_ID, (void *)&vChar);
		if (vChar == 0) {
			mib_get(MIB_NTP_SERVER_HOST1, sVal);
		}
		else
		{
			mib_get(MIB_NTP_SERVER_HOST2, (void *)sVal);
			mib_get(MIB_NTP_SERVER_HOST1, (void *)sVal2);
		}

		if(mib_get(MIB_NTP_EXT_ITF, (void *)&ntpItf))
			ifGetName(ntpItf, ifname, sizeof(ifname));

/*ping_zhang:20081223 START:add to support sntp multi server*/
#ifdef SNTP_MULTI_SERVER
		if(sVal2[0])
		{
			if (ifname[0])
				status|=va_cmd(SNTPC, 6, 0, "-s", sVal, "-s", sVal2, "-d", ifname);
			else
				status|=va_cmd(SNTPC, 4, 0, "-s", sVal, "-s", sVal2);
		}
		else
		{
			if (ifname[0])
				status|=va_cmd(SNTPC, 4, 0, "-s", sVal, "-d", ifname);
			else
				status|=va_cmd(SNTPC, 2, 0, "-s", sVal);
		}
#else
		if (ifname[0])
			status|=va_cmd(SNTPC,3,0,sVal,"-d", ifname);
		else
			status|=va_cmd(SNTPC,1,0,sVal);
#endif
/*ping_zhang:20081223 END*/
	}
	return status;
}

// return value:
// 1  : successful
// -1 : failed
int stopNTP(void)
{
	int ntp_pid=0;
	int status=1;

	ntp_pid = read_pid((char *)SNTPC_PID);
	if (ntp_pid >= 1)
	{
		//printf("kill SIGKILL to NTP's pid '%d'\n", ntp_pid);
		// kill it
		status = kill(ntp_pid, SIGTERM);
		if (status != 0)
		{
			printf("Could not kill NTP's pid '%d'\n", ntp_pid);
			return -1;
		}
		// Kaohj -- make sure it has been killed
		while( read_pid((char*)SNTPC_PID) > 0 )
		{
			usleep(30000);
		}
		// Mason Yu. Kill process in real time.
		// We delete vsntp.pid file on vsntp client process's sigtrem hander.
		#if 0
		else {
			unlink(SNTPC_PID);
		}
		#endif
	}
	return 1;
}
#endif //TIME_ZONE

#if defined(CONFIG_FON_GRE) || defined(CONFIG_XFRM)
#ifdef CONFIG_GPON_FEATURE
#define GPON_FLOWID_FILE "/proc/rtk_tr142/wan_info"
#define MAX_GPON_QUEUE_NUM 8
//cxy 2016-12-26: gpon unnumber packets need to assign tx flow id.
int set_smux_gpon_usflow(MIB_CE_ATM_VC_Tp pentry)
{
	FILE*fp;
	char buff[128];
	int wanidx=-1;
	int usflow[MAX_GPON_QUEUE_NUM],gponqueue[MAX_GPON_QUEUE_NUM];
	int usfloworder[MAX_GPON_QUEUE_NUM],tmp;
	char cmd[128],ifname[MAX_NAME_LEN];

#ifdef CONFIG_RTK_L34_ENABLE
	if(pentry == NULL || pentry->rg_wan_idx == -1)
		return -1;
#endif
	if(fp=fopen(GPON_FLOWID_FILE,"r"))
	{
		int i,j;

		for(i=0; i<MAX_GPON_QUEUE_NUM; i++)
		{
			usflow[i] = -1;
			gponqueue[i] = -1;
		}
		while(fgets(buff,sizeof(buff),fp)!=NULL)
		{
			if(strstr(buff,"wanIdx"))
			{
				if(wanidx != -1)//another wan
					break;
				//printf("buff= %s\n",buff);
#ifdef CONFIG_RTK_L34_ENABLE
				if(sscanf(buff,"%*[^=]= %d",&wanidx)==1 && wanidx != pentry->rg_wan_idx)
					wanidx = -1;
#endif
			}
			else if(wanidx!=-1)
			{
				//printf("<%s %d> get wan idx:%d\n",__func__,__LINE__,wanidx);
				//printf("buff= %s\n",buff);
				if(strstr(buff,"usFlowId"))
				{
					sscanf(buff,"%*s = %d %d %d %d %d %d %d %d",
						&usflow[0],&usflow[1],&usflow[2],&usflow[3],
						&usflow[4],&usflow[5],&usflow[6],&usflow[7]);
				}
				else if(strstr(buff,"queueSts"))
				{
					sscanf(buff,"%*s = %d %d %d %d %d %d %d %d",
						&gponqueue[0],&gponqueue[1],&gponqueue[2],&gponqueue[3],
						&gponqueue[4],&gponqueue[5],&gponqueue[6],&gponqueue[7]);
				}
			}
		}
		fclose(fp);
		if(wanidx == -1)
		{
			printf("<%s %d> not find valid wanidx\n",__func__,__LINE__);
			return -1;
		}
		// descending order
		for(i=0; i<MAX_GPON_QUEUE_NUM-1; i++)
		{
			for(j=0; j<MAX_GPON_QUEUE_NUM-i-1; j++)
				if(gponqueue[j] < gponqueue[j+1])
				{
					tmp = gponqueue[j+1];
					gponqueue[j+1] = gponqueue[j];
					gponqueue[j] = tmp;
					
					tmp = usflow[j+1];
					usflow[j+1] = usflow[j];
					usflow[j] = tmp;
				}
		}
		ifGetName(PHY_INTF(pentry->ifIndex),ifname,sizeof(ifname));
		sprintf(cmd,"ethctl setsmux %s %s usflow ",ALIASNAME_NAS0,ifname);
		for(i=0;i<MAX_GPON_QUEUE_NUM;i++)
		{
			snprintf(cmd,sizeof(cmd),"%s%d,",cmd,usflow[i]);
		}
		printf("<%s %d> %s\n",__func__,__LINE__,cmd);
		system(cmd);
		return 0;
	}
	return -1;
}
#endif//CONFIG_GPON_FEATURE
#endif

#ifdef SUPPORT_FON_GRE
int createGreTunnel(char *tnlName, char *ifname, uint32 localAddr, uint32 remoteAddr, int vlan)
{
	char localStr[48];
	char remoteStr[48];

	printf("%s enter!\n", __func__);
	
	inet_ntop(AF_INET, &localAddr, localStr, 48);
	inet_ntop(AF_INET, &remoteAddr, remoteStr, 48);

	va_cmd("/bin/ip", 13, 1, "link", "add", tnlName, "type", "gretap", "remote", remoteStr, 
			"local", localStr, "ttl", "255", "dev", ifname);
	fprintf(stderr, "ip link add %s type gretap remote %s local %s ttl 255 dev %s\n", tnlName, remoteStr, localStr, ifname);

	va_cmd("/bin/ip", 4, 1, "link", "set", tnlName, "up");
	fprintf(stderr, "ip link set %s up\n", tnlName);

	if (vlan != 0)
	{
		char vlanStr[10], vlanDevName[32];

		snprintf(vlanStr, 10, "%d", vlan);
		va_cmd("/bin/vconfig", 3, 1, "add", tnlName, vlanStr);
		fprintf(stderr, "vconfig add %s %d\n", tnlName, vlan);

		snprintf(vlanDevName, 32, "%s.%d", tnlName, vlan);
		
		va_cmd("/bin/ip", 4, 1, "link", "set", vlanDevName, "up");
		fprintf(stderr, "ip link set %s up\n", vlanDevName);
		
		va_cmd("/bin/brctl", 3, 1, "addif", "br0", vlanDevName);
		fprintf(stderr, "brctl addif br0 %s", vlanDevName);
	}
	else
	{
		va_cmd("/bin/brctl", 3, 1, "addif", "br0", tnlName);
		fprintf(stderr, "brctl addif br0 %s", tnlName);
	}
	
	return 0;
}

int deleteGreTunnel(char *tnlName, int vlan)
{
	if (vlan != 0)
	{
		char vlanDevName[32];
		
		snprintf(vlanDevName, 32, "%s.%d", tnlName, vlan);
		
		va_cmd("/bin/brctl", 3, 1, "delif", "br0", vlanDevName);
		fprintf(stderr, "brctl delif br0 %s", vlanDevName);

		va_cmd("/bin/ip", 4, 1, "link", "set", vlanDevName, "down");
		fprintf(stderr, "ip link set %s down\n", vlanDevName);
		
		va_cmd("/bin/vconfig", 2, 1, "rem", vlanDevName);
		fprintf(stderr, "vconfig rem %s\n", vlanDevName);
	}
	else
	{
		va_cmd("/bin/brctl", 3, 1, "delif", "br0", tnlName);
		fprintf(stderr, "brctl delif br0 %s", tnlName);
	}

	va_cmd("/bin/ip", 4, 1, "link", "set", tnlName, "down");
	fprintf(stderr, "ip link set %s down\n", tnlName);

	va_cmd("/bin/ip", 3, 1, "link", "del", tnlName);
	fprintf(stderr, "ip link del %s\n", tnlName);

	return 0;
}

int setupGreFirewall(void)
{
	unsigned char vChar;
	int ori_wlan_idx;
	char tnlName[32], markStr[32];

	snprintf(markStr, 32, "0x%x/0x%x", BYPASS_FWDENGINE_MARK_MASK, BYPASS_FWDENGINE_MARK_MASK);
	
	// iptables -F dhcp_queue
	va_cmd(IPTABLES, 2, 1, "-F", "dhcp_queue");
	// iptables -F FON_GRE_FORWARD
	va_cmd(IPTABLES, 2, 1, "-F", "FON_GRE_FORWARD");
	// iptables -F FON_GRE_INPUT
	va_cmd(IPTABLES, 2, 1, "-F", "FON_GRE_INPUT");
	// iptables -t mangle -F FON_GRE
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "FON_GRE");
	//ebtables -F FON_GRE
	va_cmd(EBTABLES, 2, 1, "-F", "FON_GRE");

	mib_get(MIB_FON_GRE_ENABLE, (void *)&vChar);
	if (0 == vChar)
		return 0;
	
	/* QUEUE DHCP DISCOVERY packet for OPTION-82 process */
	ori_wlan_idx = wlan_idx;
	wlan_idx = 0;
	mib_get(MIB_WLAN_FREE_SSID_GRE_ENABLE, (void *)&vChar);
	if (vChar)
	{
		//iptables -A FON_GRE_FORWARD -i wlan0-vap0 -p tcp --syn -j TCPMSS --set-mss 1418
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, "FON_GRE_FORWARD", (char *)ARG_I, "wlan0-vap0", "-p", "tcp", 
				"--syn", "-j", "TCPMSS", "--set-mss", "1418");
		
		//iptables -t mangle -A FON_GRE -i wlan0-vap0 -j MARK --set-mark 0x700000/0x700000
		va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, "FON_GRE", (char *)ARG_I, "wlan0-vap0", "-j", "MARK", 
				"--set-mark", markStr);
		
		//iptables -A dhcp_queue -i wlan0-vap0 -p udp --dport 67 -j NFQUEUE --queue-num 0
		va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "dhcp_queue", (char *)ARG_I, 
					"wlan0-vap0", "-p", "udp", (char *)FW_DPORT, 
					(char *)PORT_DHCP, "-j", "NFQUEUE", "--queue-num", "0");

		// iptables -A FON_GRE_INPUT -i wlan0-vap0 -p udp --dport 67 -j DROP
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, "FON_GRE_INPUT", (char *)ARG_I, "wlan0-vap0", "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);

		mib_get(MIB_WLAN_FREE_SSID_GRE_NAME, (void *)tnlName);
		snprintf(tnlName, 32, "%s+", tnlName);
		
		//ebtables -A FON_GRE -i wlan0-vap0 -o tnlName+ -j ACCEPT
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, "FON_GRE", "-i", "wlan0-vap0", "-o", tnlName, "-j", "ACCEPT");
		
		//ebtables -A FON_GRE -i tnlName+ -o wlan0-vap0 -j ACCEPT
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, "FON_GRE", "-i", tnlName, "-o", "wlan0-vap0", "-j", "ACCEPT");

		//default rule to drop any packet from wlan0-vap0 or tunnel dev
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", "wlan0-vap0", "-j", "DROP");
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", tnlName, "-j", "DROP");
	}
	mib_get(MIB_WLAN_CLOSED_SSID_GRE_ENABLE, (void *)&vChar);
	if (vChar)
	{
		//iptables -A FON_GRE_FORWARD -i wlan0-vap1 -p tcp --syn -j TCPMSS --set-mss 1418
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, "FON_GRE_FORWARD", (char *)ARG_I, "wlan0-vap1", "-p", "tcp", 
				"--syn", "-j", "TCPMSS", "--set-mss", "1418");
		
		//iptables -t mangle -A FON_GRE -i wlan0-vap1 -j MARK --set-mark 0x700000/0x700000
		va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, "FON_GRE", (char *)ARG_I, "wlan0-vap1", "-j", "MARK", 
				"--set-mark", markStr);
		
		//iptables -A dhcp_queue -i wlan0-vap1 -p udp --dport 67 -j NFQUEUE --queue-num 0
		va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "dhcp_queue", (char *)ARG_I, 
					"wlan0-vap1", "-p", "udp", (char *)FW_DPORT, 
					(char *)PORT_DHCP, "-j", "NFQUEUE", "--queue-num", "0");
		
		// iptables -A FON_GRE_INPUT -i wlan0-vap1 -p udp --dport 67 -j DROP
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, "FON_GRE_INPUT", (char *)ARG_I, "wlan0-vap1", "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);

		mib_get(MIB_WLAN_CLOSED_SSID_GRE_NAME, (void *)tnlName);
		snprintf(tnlName, 32, "%s+", tnlName);
		
		//ebtables -A FON_GRE -i wlan0-vap1 -o tnlName+ -j ACCEPT
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, "FON_GRE", "-i", "wlan0-vap1", "-o", tnlName, "-j", "ACCEPT");
		
		//ebtables -A FON_GRE -i tnlName+ -o wlan0-vap1 -j ACCEPT
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, "FON_GRE", "-i", tnlName, "-o", "wlan0-vap1", "-j", "ACCEPT");

		//default rule to drop any packet from wlan0-vap1 or tunnel dev
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", "wlan0-vap1", "-j", "DROP");
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", tnlName, "-j", "DROP");
	}
	
	wlan_idx = 1;
	mib_get(MIB_WLAN_FREE_SSID_GRE_ENABLE, (void *)&vChar);
	if (vChar)
	{
		//iptables -A FON_GRE_FORWARD -i wlan1-vap0 -p tcp --syn -j TCPMSS --set-mss 1418
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, "FON_GRE_FORWARD", (char *)ARG_I, "wlan1-vap0", "-p", "tcp", 
				"--syn", "-j", "TCPMSS", "--set-mss", "1418");
		
		//iptables -t mangle -A FON_GRE -i wlan1-vap0 -j MARK --set-mark 0x700000/0x700000
		va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, "FON_GRE", (char *)ARG_I, "wlan1-vap0", "-j", "MARK", 
				"--set-mark", markStr);
		
		//iptables -A dhcp_queue -i wlan1-vap0 -p udp --dport 67 -j NFQUEUE --queue-num 0
		va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "dhcp_queue", (char *)ARG_I, 
					"wlan1-vap0", "-p", "udp", (char *)FW_DPORT, 
					(char *)PORT_DHCP, "-j", "NFQUEUE", "--queue-num", "0");
		
		// iptables -A FON_GRE_INPUT -i wlan1-vap0 -p udp --dport 67 -j DROP
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, "FON_GRE_INPUT", (char *)ARG_I, "wlan1-vap0", "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);

		mib_get(MIB_WLAN_FREE_SSID_GRE_NAME, (void *)tnlName);
		snprintf(tnlName, 32, "%s+", tnlName);
		
		//ebtables -A FON_GRE -i wlan1-vap0 -o tnlName+ -j ACCEPT
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, "FON_GRE", "-i", "wlan1-vap0", "-o", tnlName, "-j", "ACCEPT");
		
		//ebtables -A FON_GRE -i tnlName+ -o wlan1-vap0 -j ACCEPT
		va_cmd(EBTABLES, 8, 1, (char *)FW_ADD, "FON_GRE", "-i", tnlName, "-o", "wlan1-vap0", "-j", "ACCEPT");

		//default rule to drop any packet from wlan1-vap0 or tunnel dev
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", "wlan1-vap0", "-j", "DROP");
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", tnlName, "-j", "DROP");
	}
	mib_get(MIB_WLAN_CLOSED_SSID_GRE_ENABLE, (void *)&vChar);
	if (vChar)
	{
		//iptables -A FON_GRE_FORWARD -i wlan1-vap1 -p tcp --syn -j TCPMSS --set-mss 1418
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, "FON_GRE_FORWARD", (char *)ARG_I, "wlan1-vap1", "-p", "tcp", 
				"--syn", "-j", "TCPMSS", "--set-mss", "1418");
		
		//iptables -t mangle -A FON_GRE -i wlan1-vap1 -j MARK --set-mark 0x700000/0x700000
		va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, "FON_GRE", (char *)ARG_I, "wlan1-vap1", "-j", "MARK", 
				"--set-mark", markStr);
		
		//iptables -A dhcp_queue -i wlan1-vap1 -p udp --dport 67 -j NFQUEUE --queue-num 0
		va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, "dhcp_queue", (char *)ARG_I, 
					"wlan1-vap1", "-p", "udp", (char *)FW_DPORT, 
					(char *)PORT_DHCP, "-j", "NFQUEUE", "--queue-num", "0");
		
		// iptables -A FON_GRE_INPUT -i wlan1-vap1 -p udp --dport 67 -j DROP
		va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, "FON_GRE_INPUT", (char *)ARG_I, "wlan1-vap1", "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);

		mib_get(MIB_WLAN_CLOSED_SSID_GRE_NAME, (void *)tnlName);
		snprintf(tnlName, 32, "%s+", tnlName);
		
		//ebtables -A FON_GRE -i wlan1-vap1 -o tnlName+ -j ACCEPT
		va_cmd(EBTABLES, 6, 1, (char *)FW_INSERT, "FON_GRE", "-i", "wlan1-vap1", "-o", tnlName, "-j", "ACCEPT");
		
		//ebtables -A FON_GRE -i tnlName+ -o wlan1-vap1 -j ACCEPT
		va_cmd(EBTABLES, 6, 1, (char *)FW_INSERT, "FON_GRE", "-i", tnlName, "-o", "wlan1-vap1", "-j", "ACCEPT");

		//default rule to drop any packet from wlan1-vap1 or tunnel dev
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", "wlan1-vap1", "-j", "DROP");
		va_cmd(EBTABLES, 6, 1, (char *)FW_ADD, "FON_GRE", "-i", tnlName, "-j", "DROP");
	}
	wlan_idx = ori_wlan_idx;

	return 0;
}

int do_FonGRE(char *ifname, int msg_type)
{
	MIB_CE_ATM_VC_T Entry;
	int totalEntry=0, i;
	int ori_wlan_idx;
	char this_ifname[IFNAMSIZ];
	char tnlName[32];
	struct in_addr localAddr, remoteAddr;
	int vlan;
	unsigned char vChar;
#ifdef CONFIG_GPON_FEATURE
	int pon_mode=0;
	int greTnlCreated = 0;
#endif

	mib_get(MIB_FON_GRE_ENABLE, (void *)&vChar);
	if (0 == vChar)
		return 0;
	
	if (msg_type == RTM_NEWADDR)
	{
		totalEntry = mib_chain_total(MIB_ATM_VC_TBL); /* get chain record size */
		for (i=0; i<totalEntry; i++)
		{
			mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry); /* get the specified chain record */		
			ifGetName(Entry.ifIndex, this_ifname, sizeof(this_ifname));		
		
			if (strcmp(this_ifname, ifname) == 0)
			{
				if (Entry.itfGroup & (1<<PMAP_WLAN0_VAP0))
				{
					//check if gre free ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 0;
					
					mib_get(MIB_WLAN_FREE_SSID_GRE_ENABLE, (void *)&vChar);
					if (1 == vChar)
					{
						mib_get(MIB_WLAN_FREE_SSID_GRE_NAME, (void *)tnlName);

						/* check if tunnel is already created to avoid repetitive operation */
						if (!getInFlags((char *)tnlName, 0))
						{
							mib_get(MIB_WLAN_FREE_SSID_GRE_SERVER, (void *)&remoteAddr);
							mib_get(MIB_WLAN_FREE_SSID_GRE_VLAN, (void *)&vlan);
							
							if (getInAddr( ifname, IP_ADDR, (void *)&localAddr) == 1)
							{
								deleteGreTunnel(tnlName, vlan);
								createGreTunnel(tnlName, ifname, localAddr.s_addr, remoteAddr.s_addr, vlan);
#ifdef CONFIG_GPON_FEATURE
								greTnlCreated = 1;
#endif
							}
						}
					}

					wlan_idx = ori_wlan_idx;
				}
				if (Entry.itfGroup & (1<<PMAP_WLAN1_VAP0))
				{
					//check if gre free ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 1;
					
					mib_get(MIB_WLAN_FREE_SSID_GRE_ENABLE, (void *)&vChar);
					if (1 == vChar)
					{
						mib_get(MIB_WLAN_FREE_SSID_GRE_NAME, (void *)tnlName);
						
						/* check if tunnel is already created to avoid repetitive operation */
						if (!getInFlags((char *)tnlName, 0))
						{
							mib_get(MIB_WLAN_FREE_SSID_GRE_SERVER, (void *)&remoteAddr);
							mib_get(MIB_WLAN_FREE_SSID_GRE_VLAN, (void *)&vlan);
							
							if (getInAddr( ifname, IP_ADDR, (void *)&localAddr) == 1)
							{
								deleteGreTunnel(tnlName, vlan);
								createGreTunnel(tnlName, ifname, localAddr.s_addr, remoteAddr.s_addr, vlan);
#ifdef CONFIG_GPON_FEATURE
								greTnlCreated = 1;
#endif
							}
						}
					}

					wlan_idx = ori_wlan_idx;
				}
				if (Entry.itfGroup & (1<<PMAP_WLAN0_VAP1))
				{
					//check if gre closed ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 0;
					
					mib_get(MIB_WLAN_CLOSED_SSID_GRE_ENABLE, (void *)&vChar);
					if (1 == vChar)
					{
						mib_get(MIB_WLAN_CLOSED_SSID_GRE_NAME, (void *)tnlName);
						
						/* check if tunnel is already created to avoid repetitive operation */
						if (!getInFlags((char *)tnlName, 0))
						{
							mib_get(MIB_WLAN_CLOSED_SSID_GRE_SERVER, (void *)&remoteAddr);
							mib_get(MIB_WLAN_CLOSED_SSID_GRE_VLAN, (void *)&vlan);
							
							if (getInAddr( ifname, IP_ADDR, (void *)&localAddr) == 1)
							{
								deleteGreTunnel(tnlName, vlan);
								createGreTunnel(tnlName, ifname, localAddr.s_addr, remoteAddr.s_addr, vlan);
#ifdef CONFIG_GPON_FEATURE
								greTnlCreated = 1;
#endif
							}
						}
					}

					wlan_idx = ori_wlan_idx;
				}
				if (Entry.itfGroup & (1<<PMAP_WLAN1_VAP1))
				{
					//check if gre closed ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 1;
					
					mib_get(MIB_WLAN_CLOSED_SSID_GRE_ENABLE, (void *)&vChar);
					if (1 == vChar)
					{
						mib_get(MIB_WLAN_CLOSED_SSID_GRE_NAME, (void *)tnlName);
						
						/* check if tunnel is already created to avoid repetitive operation */
						if (!getInFlags((char *)tnlName, 0))
						{
							mib_get(MIB_WLAN_CLOSED_SSID_GRE_SERVER, (void *)&remoteAddr);
							mib_get(MIB_WLAN_CLOSED_SSID_GRE_VLAN, (void *)&vlan);
							
							if (getInAddr( ifname, IP_ADDR, (void *)&localAddr) == 1)
							{
								deleteGreTunnel(tnlName, vlan);
								createGreTunnel(tnlName, ifname, localAddr.s_addr, remoteAddr.s_addr, vlan);
#ifdef CONFIG_GPON_FEATURE
								greTnlCreated = 1;
#endif
							}
						}
					}

					wlan_idx = ori_wlan_idx;
				}
#ifdef CONFIG_RTK_RG_INIT
				FlushRTK_RG_ACL_FON_GRE_RULE();
				AddRTK_RG_ACL_FON_GRE_RULE();
#endif

				setupGreFirewall();

#ifdef CONFIG_GPON_FEATURE
				if (greTnlCreated)
				{
					mib_get(MIB_PON_MODE, &pon_mode);
					if (pon_mode == GPON_MODE)
					{
						set_smux_gpon_usflow(&Entry);
					}
				}
#endif

				break;
			}
		}
	}
	else if (msg_type == RTM_DELADDR)
	{
		totalEntry = mib_chain_total(MIB_ATM_VC_TBL); /* get chain record size */
		for (i=0; i<totalEntry; i++)
		{
			mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry); /* get the specified chain record */		
			ifGetName(Entry.ifIndex, this_ifname, sizeof(this_ifname));		
		
			if (strcmp(this_ifname, ifname) == 0)
			{
				if (Entry.itfGroup & (1<<PMAP_WLAN0_VAP0))
				{
					//check if gre free ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 0;
					
					mib_get(MIB_WLAN_FREE_SSID_GRE_ENABLE, (void *)&vChar);
					if (1 == vChar)
					{
						mib_get(MIB_WLAN_FREE_SSID_GRE_NAME, (void *)tnlName);

						/* check if tunnel is already created to avoid repetitive operation */
						if (getInFlags((char *)tnlName, 0))
						{
							mib_get(MIB_WLAN_FREE_SSID_GRE_VLAN, (void *)&vlan);
							
							deleteGreTunnel(tnlName, vlan);
						}
					}

					wlan_idx = ori_wlan_idx;
				}
				if (Entry.itfGroup & (1<<PMAP_WLAN1_VAP0))
				{
					//check if gre free ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 1;
					
					mib_get(MIB_WLAN_FREE_SSID_GRE_ENABLE, (void *)&vChar);
					if (1 == vChar)
					{
						mib_get(MIB_WLAN_FREE_SSID_GRE_NAME, (void *)tnlName);
						
						/* check if tunnel is already created to avoid repetitive operation */
						if (getInFlags((char *)tnlName, 0))
						{
							mib_get(MIB_WLAN_FREE_SSID_GRE_VLAN, (void *)&vlan);
							
							deleteGreTunnel(tnlName, vlan);
						}
					}

					wlan_idx = ori_wlan_idx;
				}
				if (Entry.itfGroup & (1<<PMAP_WLAN0_VAP1))
				{
					//check if gre closed ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 0;
					
					mib_get(MIB_WLAN_CLOSED_SSID_GRE_ENABLE, (void *)&vChar);
					if (1 == vChar)
					{
						mib_get(MIB_WLAN_CLOSED_SSID_GRE_NAME, (void *)tnlName);
						
						/* check if tunnel is already created to avoid repetitive operation */
						if (getInFlags((char *)tnlName, 0))
						{
							mib_get(MIB_WLAN_CLOSED_SSID_GRE_VLAN, (void *)&vlan);
							
							deleteGreTunnel(tnlName, vlan);
						}
					}

					wlan_idx = ori_wlan_idx;
				}
				if (Entry.itfGroup & (1<<PMAP_WLAN1_VAP1))
				{
					//check if gre closed ssid enabled
					ori_wlan_idx = wlan_idx;
					wlan_idx = 1;
					
					mib_get(MIB_WLAN_CLOSED_SSID_GRE_ENABLE, (void *)&vChar);
					if (1 == vChar)
					{
						mib_get(MIB_WLAN_CLOSED_SSID_GRE_NAME, (void *)tnlName);
						
						/* check if tunnel is already created to avoid repetitive operation */
						if (getInFlags((char *)tnlName, 0))
						{
							mib_get(MIB_WLAN_CLOSED_SSID_GRE_VLAN, (void *)&vlan);
							
							deleteGreTunnel(tnlName, vlan);
						}
					}

					wlan_idx = ori_wlan_idx;
				}
#ifdef CONFIG_RTK_RG_INIT
				FlushRTK_RG_ACL_FON_GRE_RULE();
				AddRTK_RG_ACL_FON_GRE_RULE();
#endif

				setupGreFirewall();
				
				break;
			}
		}
	}
	
	return 0;
}
#endif//SUPPORT_FON_GRE

#ifdef CONFIG_USER_DHCP_SERVER
#ifdef SUPPORT_DHCP_RESERVED_IPADDR
static const char DHCPReservedIPAddrFile[] = "/var/udhcpd/DHCPReservedIPAddr.txt";
static int setupDHCPReservedIPAddr( void )
{
	FILE *fp;

	fp=fopen( DHCPReservedIPAddrFile, "w" );
	if(!fp) return -1;

	{//default pool
		unsigned int j,num;
		num = mib_chain_total( MIB_DHCP_RESERVED_IPADDR_TBL );
		fprintf( fp, "START %u\n", 0 );
		for( j=0;j<num;j++ )
		{
			MIB_DHCP_RESERVED_IPADDR_T entryip;
			if(!mib_chain_get( MIB_DHCP_RESERVED_IPADDR_TBL, j, (void*)&entryip ))
				continue;
			if( entryip.InstanceNum==0 )
			{
				fprintf( fp, "%s\n",  inet_ntoa(*((struct in_addr *)&(entryip.IPAddr))) );
			}
		}
		fprintf( fp, "END\n" );
	}

#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
{
	unsigned int i,numpool;
	numpool = mib_chain_total( MIB_DHCPS_SERVING_POOL_TBL );
	for( i=0; i<numpool;i++ )
	{
		unsigned int j,num;
		DHCPS_SERVING_POOL_T entrypool;

		if( !mib_chain_get( MIB_DHCPS_SERVING_POOL_TBL, i, (void*)&entrypool ) )
			continue;

		//skip disable or relay pools
		if( entrypool.enable==0 || entrypool.localserved==0 )
			continue;

		num = mib_chain_total( MIB_DHCP_RESERVED_IPADDR_TBL );
		fprintf( fp, "START %u\n", entrypool.InstanceNum );
		for( j=0;j<num;j++ )
		{
			MIB_DHCP_RESERVED_IPADDR_T entryip;
			if(!mib_chain_get( MIB_DHCP_RESERVED_IPADDR_TBL, j, (void*)&entryip ))
				continue;
			if( entryip.InstanceNum==entrypool.InstanceNum )
			{
				fprintf( fp, "%s\n",  inet_ntoa(*((struct in_addr *)&(entryip.IPAddr))) );
			}
		}
		fprintf( fp, "END\n" );
	}
}
#endif //_PRMT_X_TELEFONICA_ES_DHCPOPTION_

	fclose(fp);
	return 0;
}

int clearDHCPReservedIPAddrByInstNum(unsigned int instnum)
{
	int j,num;
	num = mib_chain_total( MIB_DHCP_RESERVED_IPADDR_TBL );
	if( num>0 )
	{
		for( j=num-1;j>=0;j-- )
		{
			MIB_DHCP_RESERVED_IPADDR_T entryip;
			if(!mib_chain_get( MIB_DHCP_RESERVED_IPADDR_TBL, j, (void*)&entryip ))
				continue;
			if( entryip.InstanceNum==instnum )
			{
				mib_chain_delete( MIB_DHCP_RESERVED_IPADDR_TBL, j );
			}
		}
	}
	return 0;
}
#endif //SUPPORT_DHCP_RESERVED_IPADDR



// DHCP server configuration
// return value:
// 0  : not active
// 1  : active
// -1 : setup failed
int setupDhcpd(void)
{
	unsigned char value[32], value1[32];
	unsigned int uInt, uLTime;
	DNS_TYPE_T dnsMode;
	FILE *fp=NULL, *fp2=NULL;
	char ipstart[16], ipend[16], vChar, optionStr[254];
#ifdef IMAGENIO_IPTV_SUPPORT
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
	char opchaddr[16]={0}, stbdns1[16]={0}, stbdns2[16]={0};
	unsigned short opchport;
#endif
/*ping_zhang:20090930 END*/
#endif
	char subnet[16], ipaddr[16], ipaddr2[16];
	char dhcpsubnet[16];
	char dns1[16], dns2[16], dns3[16], dhcps[16];
	char domain[MAX_NAME_LEN];
	unsigned int entryNum, i, j;
	MIB_CE_MAC_BASE_DHCP_T Entry;
#ifdef IP_PASSTHROUGH
	int ippt;
	unsigned int ippt_itf;
#endif
	int spc_enable=0, spc_ip=0;
	struct in_addr myip;
	//ql: check if pool is in first IP subnet or in secondary IP subnet.
	int subnetIdx=0;
	struct in_addr lanip1, lanmask1, lanip2, lanmask2, inpoolstart;
	char serverip[16];

/*ping_zhang:20080919 START:add for new telefonica tr069 request: dhcp option*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	DHCPS_SERVING_POOL_T dhcppoolentry;
	int dhcppoolnum;
	unsigned char macempty[6]={0,0,0,0,0,0};
	char *ipempty="0.0.0.0";
#endif
	char macaddr[20];

/*ping_zhang:20080919 END*/

	// check if dhcp server on ?
	// Modified by Mason Yu for dhcpmode
	//if (mib_get(MIB_ADSL_LAN_DHCP, (void *)value) != 0)
	if (mib_get(MIB_DHCP_MODE, (void *)value) != 0)
	{
		uInt = (unsigned int)(*(unsigned char *)value);
		if (uInt != 2 )
		{
			return 0;	// dhcp Server not on
		}
	}

#ifdef CONFIG_SECONDARY_IP
	mib_get(MIB_ADSL_LAN_ENABLE_IP2, (void *)value);
	if (value[0])
		mib_get(MIB_ADSL_LAN_DHCP_POOLUSE, (void *)value);
#else
	value[0] = 0;
#endif

#ifdef DHCPS_POOL_COMPLETE_IP
	mib_get(MIB_DHCP_SUBNET_MASK, (void *)value);
	strncpy(dhcpsubnet, inet_ntoa(*((struct in_addr *)value)), 16);
	dhcpsubnet[15] = '\0';
	// IP Pool start
	mib_get(MIB_DHCP_POOL_START, (void *)value);
	strncpy(ipstart, inet_ntoa(*((struct in_addr *)value)), 16);
	// IP Pool end
	mib_get(MIB_DHCP_POOL_END, (void *)value);
	strncpy(ipend, inet_ntoa(*((struct in_addr *)value)), 16);
#else
	if (value[0] == 0) { // primary LAN
		if (mib_get(MIB_ADSL_LAN_SUBNET, (void *)value) != 0)
		{
			strncpy(dhcpsubnet, inet_ntoa(*((struct in_addr *)value)), 16);
			dhcpsubnet[15] = '\0';
		}
		else
			return -1;

		mib_get(MIB_ADSL_LAN_IP, (void *)value);
	}
	else { // secondary LAN
		if (mib_get(MIB_ADSL_LAN_SUBNET2, (void *)value) != 0)
		{
			strncpy(dhcpsubnet, inet_ntoa(*((struct in_addr *)value)), 16);
			dhcpsubnet[15] = '\0';
		}
		else
			return -1;

		mib_get(MIB_ADSL_LAN_IP2, (void *)value);
	}

	// IP pool start address
	mib_get(MIB_ADSL_LAN_CLIENT_START, (void *)&value[3]);
		strncpy(ipstart, inet_ntoa(*((struct in_addr *)value)), 16);
		ipstart[15] = '\0';
	// IP pool end address
	mib_get(MIB_ADSL_LAN_CLIENT_END, (void *)&value[3]);
		strncpy(ipend, inet_ntoa(*((struct in_addr *)value)), 16);
		ipend[15] = '\0';
	#endif

	// Added by Mason Yu for MAC base assignment. Start
	if ((fp2 = fopen("/var/dhcpdMacBase.txt", "w")) == NULL)
	{
		printf("***** Open file /var/dhcpdMacBase.txt failed !\n");
		goto dhcpConf;
	}

//star: reserve local ip for dhcp server
		MIB_CE_MAC_BASE_DHCP_T entry;

		strcpy(macaddr, "localmac");
		//printf("entry.macAddr = %s\n", entry.macAddr);
		if (mib_get(MIB_ADSL_LAN_IP, (void *)value) != 0)
		{
			//strncpy(entry.ipAddr, inet_ntoa(*((struct in_addr *)value)), 16);
			//entry.ipAddr[15] = '\0';
			fprintf(fp2, "%s: %s\n", macaddr, inet_ntoa(*((struct in_addr *)value)));
		}

#ifdef CONFIG_SECONDARY_IP
		mib_get(MIB_ADSL_LAN_ENABLE_IP2, (void *)value);
		if (value[0] == 1) {
			strcpy(macaddr, "localsecmac");
			//printf("entry.macAddr = %s\n", entry.macAddr);
			if (mib_get(MIB_ADSL_LAN_IP2, (void *)value) != 0)
			{
				//strncpy(entry.ipAddr, inet_ntoa(*((struct in_addr *)value)), 16);
				//entry.ipAddr[15] = '\0';
				fprintf(fp2, "%s: %s\n", macaddr, inet_ntoa(*((struct in_addr *)value)) );
			}

		}

#endif

//star: check mactbl
	struct in_addr poolstart,poolend,macip;
	unsigned long v1;

	inet_aton(ipstart, &poolstart);
	inet_aton(ipend, &poolend);
check_mactbl:
	entryNum = mib_chain_total(MIB_MAC_BASE_DHCP_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_MAC_BASE_DHCP_TBL, i, (void *)&Entry))
		{
  			printf("setupDhcpd:Get chain(MIB_MAC_BASE_DHCP_TBL) record error!\n");
		}
		//inet_aton(Entry.ipAddr, &macip);
		v1 = *((unsigned long *)Entry.ipAddr_Dhcp);
		if( v1 < poolstart.s_addr || v1 > poolend.s_addr )
		//if(macip.s_addr<poolstart.s_addr||macip.s_addr>poolend.s_addr)
		{
			if(mib_chain_delete(MIB_MAC_BASE_DHCP_TBL, i)!=1)
			{
				fclose(fp2);
				printf("setupDhcpd:Delete chain(MIB_MAC_BASE_DHCP_TBL) record error!\n");
				return -1;
			}
			break;
		}
	}
	if((int)i<((int)entryNum-1))
		goto check_mactbl;

	entryNum = mib_chain_total(MIB_MAC_BASE_DHCP_TBL);
	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_MAC_BASE_DHCP_TBL, i, (void *)&Entry))
		{
  			printf("setupDhcpd:Get chain(MIB_MAC_BASE_DHCP_TBL) record error!\n");
		}

		snprintf(macaddr, 18, "%02x-%02x-%02x-%02x-%02x-%02x",
				Entry.macAddr_Dhcp[0], Entry.macAddr_Dhcp[1],
				Entry.macAddr_Dhcp[2], Entry.macAddr_Dhcp[3],
				Entry.macAddr_Dhcp[4], Entry.macAddr_Dhcp[5]);

		for (j=0; j<17; j++){
			if ( macaddr[j] != '-' ) {
				macaddr[j] = tolower(macaddr[j]);
			}
		}

		//fprintf(fp2, "%s: %s\n", Entry.macAddr, Entry.ipAddr);
		fprintf(fp2, "%s: %s\n", macaddr, inet_ntoa(*((struct in_addr *)Entry.ipAddr_Dhcp)) );
	}
	fclose(fp2);
	// Added by Mason Yu for MAC base assignment. End

dhcpConf:

#ifdef SUPPORT_DHCP_RESERVED_IPADDR
	setupDHCPReservedIPAddr();
#endif //SUPPORT_DHCP_RESERVED_IPADDR

#ifdef IMAGENIO_IPTV_SUPPORT
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
	if (mib_get(MIB_OPCH_ADDRESS, (void *)&value[0]) != 0) {
		strncpy(opchaddr, inet_ntoa(*(struct in_addr *)value), 16);
		opchaddr[15] = '\0';
	} else
		return -1;

	if (mib_get(MIB_IMAGENIO_DNS1, (void *)&value[0]) != 0) {
		strncpy(stbdns1, inet_ntoa(*(struct in_addr *)value), 16);
		stbdns1[15] = '\0';
	} else
		return -1;

	mib_get(MIB_IMAGENIO_DNS2, (void *)&myip);
	if ((myip.s_addr != 0xffffffff) && (myip.s_addr != 0)) { // not empty
		strncpy(stbdns2, inet_ntoa(myip), 16);
		stbdns2[15] = '\0';
	}

	if (!mib_get(MIB_OPCH_PORT, (void *)&opchport))
		return -1;
#endif
/*ping_zhang:20090930 END*/
#endif

	// IP max lease time
	if (mib_get(MIB_ADSL_LAN_DHCP_LEASE, (void *)&uLTime) == 0)
		return -1;

	if (mib_get(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)domain) == 0)
		return -1;

	// get DNS mode
	if (mib_get(MIB_ADSL_WAN_DNS_MODE, (void *)value) != 0)
	{
		dnsMode = (DNS_TYPE_T)(*(unsigned char *)value);
	}
	else
		return -1;

	// Commented by Mason Yu for LAN_IP as DNS Server
	if ((fp = fopen(DHCPD_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", DHCPD_CONF);
		return -1;
	}

	if (mib_get(MIB_SPC_ENABLE, (void *)value) != 0)
	{
		if (value[0])
		{
			spc_enable = 1;
			mib_get(MIB_SPC_IPTYPE, (void *)value);
			spc_ip = (unsigned int)(*(unsigned char *)value);
		}
		else
			spc_enable = 0;
	}

/*ping_zhang:20080919 START:add for new telefonica tr069 request: dhcp option*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	fprintf(fp,"poolname default\n");
#endif
/*ping_zhang:20080919 END*/
	//ql: check if pool is in first IP subnet or in secondary IP subnet.
	mib_get(MIB_DHCP_POOL_START, (void *)&inpoolstart);
	mib_get(MIB_ADSL_LAN_IP, (void *)&lanip1);
	mib_get(MIB_ADSL_LAN_SUBNET, (void *)&lanmask1);
	if ((inpoolstart.s_addr & lanmask1.s_addr) == (lanip1.s_addr & lanmask1.s_addr))
		subnetIdx = 0;
	else {
#ifdef CONFIG_SECONDARY_IP
		mib_get(MIB_ADSL_LAN_IP2, (void *)&lanip2);
		mib_get(MIB_ADSL_LAN_SUBNET2, (void *)&lanmask2);
		if ((inpoolstart.s_addr & lanmask2.s_addr) == (lanip2.s_addr & lanmask2.s_addr))
			subnetIdx = 1;
		else
#endif //CONFIG_SECONDARY_IP
			subnetIdx = 0;
	}
	if (0 == subnetIdx)
		snprintf(serverip, 16, "%s", inet_ntoa(lanip1));
	else
		snprintf(serverip, 16, "%s", inet_ntoa(lanip2));

	fprintf(fp, "interface %s\n", LANIF);
	//ql add
	fprintf(fp, "server %s\n", serverip);
	// Mason Yu. For Test
#if 0
	fprintf(fp, "locallyserved no\n" );
	if (mib_get(MIB_ADSL_WAN_DHCPS, (void *)value) != 0)
	{
		if (((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(dhcps, inet_ntoa(*((struct in_addr *)value)), 16);
			dhcps[15] = '\0';
		}
		else
			dhcps[0] = '\0';
	}
	fprintf(fp, "dhcpserver %s\n", dhcps);
#endif

	if (spc_enable)
	{
		if (spc_ip == 0) { // single private ip
			fprintf(fp, "start %s\n", ipstart);
			fprintf(fp, "end %s\n", ipstart);
		}
		//else { // single shared ip
	}
	else
	{
		fprintf(fp, "start %s\n", ipstart);
		fprintf(fp, "end %s\n", ipend);
	}
#if 0
#ifdef IP_BASED_CLIENT_TYPE
	fprintf(fp, "pcstart %s\n", pcipstart);
	fprintf(fp, "pcend %s\n", pcipend);
	fprintf(fp, "cmrstart %s\n", cmripstart);
	fprintf(fp, "cmrend %s\n", cmripend);
	fprintf(fp, "stbstart %s\n", stbipstart);
	fprintf(fp, "stbend %s\n", stbipend);
	fprintf(fp, "phnstart %s\n", phnipstart);
	fprintf(fp, "phnend %s\n", phnipend);
	fprintf(fp, "hgwstart %s\n", hgwipstart);
	fprintf(fp, "hgwend %s\n", hgwipend);
	//ql 20090122 add
	fprintf(fp, "pcopt60 %s\n", pcopt60);
	fprintf(fp, "cmropt60 %s\n", cmropt60);
	fprintf(fp, "stbopt60 %s\n", stbopt60);
	fprintf(fp, "phnopt60 %s\n", phnopt60);
#endif
#endif

#if 0
/*ping_zhang:20090319 START:replace ip range with serving pool of tr069*/
#ifndef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
#ifdef IP_BASED_CLIENT_TYPE
	{
		MIB_CE_IP_RANGE_DHCP_T Entry;
		int i, entrynum=mib_chain_total(MIB_IP_RANGE_DHCP_TBL);

		for (i=0; i<entrynum; i++)
		{
			if (!mib_chain_get(MIB_IP_RANGE_DHCP_TBL, i, (void *)&Entry))
				continue;

			strncpy(devipstart, inet_ntoa(*((struct in_addr *)Entry.startIP)), 16);
			strncpy(devipend, inet_ntoa(*((struct in_addr *)Entry.endIP)), 16);

/*ping_zhang:20090317 START:change len of option60 to 100*/
			//fprintf(fp, "range start=%s:end=%s:option=%s:devicetype=%d\n", devipstart, devipend, Entry.option60,Entry.deviceType);
			//fprintf(fp, "range start=%s:end=%s:option=%s:devicetype=%d:optCode=%d:optStr=%s\n", devipstart, devipend, Entry.option60,Entry.deviceType,Entry.optionCode,Entry.optionStr);
			//fprintf(fp, "range s=%s:e=%s:o=%s:t=%d:oC=%d:oS=%s\n",	devipstart, devipend, Entry.option60,Entry.deviceType,Entry.optionCode,Entry.optionStr);
			fprintf(fp, "range i=%d:s=%s:e=%s:o=%s:t=%d\n",i,devipstart, devipend, Entry.option60,Entry.deviceType);
			fprintf(fp, "range_optRsv i=%d:oC=%d:oS=%s\n",i,Entry.optionCode,Entry.optionStr);
/*ping_zhang:20090317 END*/
		}
	}
#endif
#endif
#endif //#if 0

/*ping_zhang:20090319 END*/
#ifdef IMAGENIO_IPTV_SUPPORT
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
	fprintf(fp, "opchaddr %s\n", opchaddr);
	fprintf(fp, "opchport %d\n", opchport);
	fprintf(fp, "stbdns1 %s\n", stbdns1);
	if (stbdns2[0])
		fprintf(fp, "stbdns2 %s\n", stbdns2);
#endif
/*ping_zhang:20090930 END*/
#endif

#ifdef IP_PASSTHROUGH
	ippt = 0;
	if (mib_get(MIB_IPPT_ITF, (void *)&ippt_itf) != 0)
		if (ippt_itf != DUMMY_IFINDEX) // IP passthrough
			ippt = 1;

	if (ippt)
	{
		fprintf(fp, "ippt yes\n");
		mib_get(MIB_IPPT_LEASE, (void *)&uInt);
		fprintf(fp, "ipptlt %d\n", uInt);
	}
#endif

	fprintf(fp, "opt subnet %s\n", dhcpsubnet);

	// Added by Mason Yu
	if (mib_get(MIB_ADSL_LAN_DHCP_GATEWAY, (void *)value) != 0)
	{
		strncpy(ipaddr2, inet_ntoa(*((struct in_addr *)value)), 16);
		ipaddr2[15] = '\0';
	}
	else{
		fclose(fp);
		return -1;
	}
	fprintf(fp, "opt router %s\n", ipaddr2);

	// Modified by Mason Yu for LAN_IP as DNS Server
#if 0
	if (dnsMode == DNS_AUTO)
	{
		fprintf(fp, "opt dns %s\n", ipaddr);
	}
	else	// DNS_MANUAL
	{
		if (dns1[0])
			fprintf(fp, "opt dns %s\n", dns1);
		if (dns2[0])
			fprintf(fp, "opt dns %s\n", dns2);
		if (dns3[0])
			fprintf(fp, "opt dns %s\n", dns3);
	}
#endif

	// Kaohj
#ifdef DHCPS_DNS_OPTIONS
	mib_get(MIB_DHCP_DNS_OPTION, (void *)value);
	if (value[0] == 0)
	fprintf(fp, "opt dns %s\n", ipaddr2);
	else { // check manual setting
		mib_get(MIB_DHCPS_DNS1, (void *)&myip);
		strncpy(ipaddr, inet_ntoa(myip), 16);
		ipaddr[15] = '\0';
		fprintf(fp, "opt dns %s\n", ipaddr);
		mib_get(MIB_DHCPS_DNS2, (void *)&myip);
		if (myip.s_addr != INADDR_NONE) { // not empty
			strncpy(ipaddr, inet_ntoa(myip), 16);
			ipaddr[15] = '\0';
			fprintf(fp, "opt dns %s\n", ipaddr);
			mib_get(MIB_DHCPS_DNS3, (void *)&myip);
			if (myip.s_addr != INADDR_NONE) { // not empty
				strncpy(ipaddr, inet_ntoa(myip), 16);
				ipaddr[15] = '\0';
				fprintf(fp, "opt dns %s\n", ipaddr);
			}
		}
	}
#else
	fprintf(fp, "opt dns %s\n", ipaddr2);
#endif
	fprintf(fp, "opt lease %u\n", uLTime);
	if (domain[0])
		fprintf(fp, "opt domain %s\n", domain);
	else
		// per TR-068, I-188
		fprintf(fp, "opt domain domain_not_set.invalid\n");

#ifdef _CWMP_MIB_ /*jiunming, for cwmp-tr069*/
{
	//format: opt venspec [enterprise-number] [sub-option code] [sub-option data] ...
	//opt vendspec: dhcp option 125 Vendor-Identifying Vendor-Specific
	//enterprise-number: 3561(ADSL Forum)
	//sub-option code: 4(GatewayManufacturerOUI)
	//sub-option code: 5(GatewaySerialNumber)
	//sub-option code: 6(GatewayProductClass)
	char opt125_sn[65];
	mib_get(MIB_HW_SERIAL_NUMBER, (void *)opt125_sn);
	fprintf( fp, "opt venspec 3561 4 %s 5 %s 6 %s\n", MANUFACTURER_OUI, opt125_sn, PRODUCT_CLASS );
}
#endif

#if !defined(CONFIG_00R0)
	//Mark because project 00R0 will use ntp.local as default NTP Server and it will 
	//Slow the udhcpd init time.
	/* DHCP option 42 */
	mib_get( MIB_NTP_SERVER_ID, (void *)&vChar);
	if (vChar == 0) {
		mib_get(MIB_NTP_SERVER_HOST1, value);
		fprintf(fp, "opt ntpsrv %s\n", value);
	}
	else
	{
		mib_get(MIB_NTP_SERVER_HOST2, (void *)value);
		mib_get(MIB_NTP_SERVER_HOST1, (void *)value1);
		fprintf(fp, "opt ntpsrv %s,%s\n", value,value1);
	}
#endif

	/* DHCP option 66 */
	if (mib_get(MIB_TFTP_SERVER_ADDR, (void *)&optionStr) != 0) { //Length max 254
		fprintf(fp, "opt tftp %s\n", optionStr);
	}

#if !defined(CONFIG_00R0)
	//Mark because project 00R0 will use ntp.local as default NTP Server and it will 
	//Slow the udhcpd init time.
	/* DHCP option 100 */
	if (mib_get(MIB_POSIX_TZ_STRING, (void *)&optionStr) != 0) { //Length max 254
		fprintf(fp, "opt tzstring %s\n", optionStr); 
	}	
#endif
#ifdef CONFIG_USER_RTK_VOIP //single SIP server right now /* DHCP option 120 */

#include <errno.h>
#include <fcntl.h>
#include "voip_manager.h"
#define SIP_PROXY_NUM 2

	voipCfgParam_t *voip_pVoIPCfg = NULL;
	voipCfgPortParam_t *pCfg;
	int count = 0;
	int ret;
	
	int enc[SIP_PROXY_NUM];
	long ipaddrs=0;

	if(voip_pVoIPCfg==NULL){
		if (voip_flash_get(&voip_pVoIPCfg) != 0){
			fclose(fp);
			return -1;
		}
	}
	
	pCfg = &voip_pVoIPCfg->ports[0];
	for( i=0 ; i<MAX_PROXY ; i++ ){
		if( pCfg->proxies[i].addr != NULL ){
			ipaddrs = inet_network(pCfg->proxies[i].addr);
			if (ipaddrs >= 0){
				enc[i] = 1;
				fprintf(fp, "opt sipsrv %d %s\n", enc[i], pCfg->proxies[i].addr);
			}
			else {
				if(strlen(pCfg->proxies[0].addr) > 5) { //Avoid to the NULL string, Iulian Wu
					enc[i] = 0;
					fprintf(fp, "opt sipsrv %d %s\n", enc[i], pCfg->proxies[i].addr);
				}
			}
			break;
		}
	}

#endif

/*write dhcp serving pool config*/
/*ping_zhang:20080919 START:add for new telefonica tr069 request: dhcp option*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	fprintf(fp, "poolend end\n");
	memset(&dhcppoolentry,0,sizeof(DHCPS_SERVING_POOL_T));

/*test code*
	dhcppoolentry.enable=1;
	dhcppoolentry.poolorder=1;
	strcpy(dhcppoolentry.poolname,"poolone");
	strcpy(dhcppoolentry.vendorclass,"MSFT 5.0");
	strcpy(dhcppoolentry.clientid,"");
	strcpy(dhcppoolentry.userclass,"");

	inet_aton("192.168.1.40",((struct in_addr *)(dhcppoolentry.startaddr)));
	inet_aton("192.168.1.50",((struct in_addr *)(dhcppoolentry.endaddr)));
	inet_aton("255.255.255.0",((struct in_addr *)(dhcppoolentry.subnetmask)));
	inet_aton("192.168.1.1",((struct in_addr *)(dhcppoolentry.iprouter)));
	inet_aton("172.29.17.10",((struct in_addr *)(dhcppoolentry.dnsserver1)));
	strcpy(dhcppoolentry.domainname,"poolone.com");
	dhcppoolentry.leasetime=86400;
	dhcppoolentry.dnsservermode=1;
	dhcppoolentry.InstanceNum=1;
	entryNum = mib_chain_total(MIB_DHCPS_SERVING_POOL_TBL);
printf("\nentryNum=%d\n",entryNum);
	if(entryNum==0)
		mib_chain_add(MIB_DHCPS_SERVING_POOL_TBL,(void*)&dhcppoolentry);
****/

	entryNum = mib_chain_total(MIB_DHCPS_SERVING_POOL_TBL);

	for(i=0;i<entryNum;i++){
		memset(&dhcppoolentry,0,sizeof(DHCPS_SERVING_POOL_T));
		if(!mib_chain_get(MIB_DHCPS_SERVING_POOL_TBL,i,(void*)&dhcppoolentry))
			continue;
		if(dhcppoolentry.enable==1){
			strncpy(ipstart, inet_ntoa(*((struct in_addr *)(dhcppoolentry.startaddr))), 16);
			strncpy(ipend, inet_ntoa(*((struct in_addr *)(dhcppoolentry.endaddr))), 16);
			strncpy(dhcpsubnet, inet_ntoa(*((struct in_addr *)(dhcppoolentry.subnetmask))), 16);
			strncpy(ipaddr2, inet_ntoa(*((struct in_addr *)(dhcppoolentry.iprouter))), 16);

			if( dhcppoolentry.localserved ) //check only for locallyserved==true
			{
				if(!strcmp(ipstart,ipempty) ||
					!strcmp(ipend,ipempty) ||
					!strcmp(dhcpsubnet,ipempty))
					continue;
			}


			fprintf(fp, "poolname %s\n",dhcppoolentry.poolname);
			fprintf(fp, "cwmpinstnum %d\n",dhcppoolentry.InstanceNum);
			fprintf(fp, "poolorder %u\n",dhcppoolentry.poolorder);
			fprintf(fp, "interface %s\n", LANIF);
			//ql add
			fprintf(fp, "server %s\n", serverip);
			fprintf(fp, "start %s\n", ipstart);
			fprintf(fp, "end %s\n", ipend);

			fprintf(fp, "sourceinterface %u\n",dhcppoolentry.sourceinterface);
			if(dhcppoolentry.vendorclass[0]!=0){
				fprintf(fp, "vendorclass %s\n",dhcppoolentry.vendorclass);
				fprintf(fp, "vendorclassflag %u\n",dhcppoolentry.vendorclassflag);
				if(dhcppoolentry.vendorclassmode[0]!=0)
					fprintf(fp, "vendorclassmode %s\n",dhcppoolentry.vendorclassmode);
			}
			if(dhcppoolentry.clientid[0]!=0){
				fprintf(fp, "clientid %s\n",dhcppoolentry.clientid);
				fprintf(fp, "clientidflag %u\n",dhcppoolentry.clientidflag);
			}
			if(dhcppoolentry.userclass[0]!=0){
				fprintf(fp, "userclass %s\n",dhcppoolentry.userclass);
				fprintf(fp, "userclassflag %u\n",dhcppoolentry.userclassflag);
			}
			if(memcmp(dhcppoolentry.chaddr,macempty,6)){
				fprintf(fp, "chaddr %02x%02x%02x%02x%02x%02x\n",dhcppoolentry.chaddr[0],dhcppoolentry.chaddr[1],dhcppoolentry.chaddr[2],
					dhcppoolentry.chaddr[3],dhcppoolentry.chaddr[4],dhcppoolentry.chaddr[5]);
				if(memcmp(dhcppoolentry.chaddrmask,macempty,6))
					fprintf(fp, "chaddrmask %02x%02x%02x%02x%02x%02x\n",dhcppoolentry.chaddrmask[0],dhcppoolentry.chaddrmask[1],dhcppoolentry.chaddrmask[2],
						dhcppoolentry.chaddrmask[3],dhcppoolentry.chaddrmask[4],dhcppoolentry.chaddrmask[5]);
				fprintf(fp, "chaddrflag %u\n",dhcppoolentry.chaddrflag);
			}

#ifdef IP_PASSTHROUGH
			ippt = 0;
			if (mib_get(MIB_IPPT_ITF, (void *)&ippt_itf) != 0)
				if (ippt_itf != DUMMY_IFINDEX) // IP passthrough
					ippt = 1;

			if (ippt)
			{
				fprintf(fp, "ippt yes\n");
				mib_get(MIB_IPPT_LEASE, (void *)&uInt);
				fprintf(fp, "ipptlt %d\n", uInt);
			}
#endif
#ifdef IMAGENIO_IPTV_SUPPORT
			//fprintf(fp, "pcopt60 %s\n", pcopt60);
			//fprintf(fp, "cmropt60 %s\n", cmropt60);
			//fprintf(fp, "stbopt60 %s\n", stbopt60);
			//fprintf(fp, "phnopt60 %s\n", phnopt60);
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
			fprintf(fp, "opchaddr %s\n", opchaddr);
			fprintf(fp, "opchport %d\n", opchport);
			fprintf(fp, "stbdns1 %s\n", stbdns1);
			if (stbdns2[0])
				fprintf(fp, "stbdns2 %s\n", stbdns2);
#endif
/*ping_zhang:20090930 END*/
#endif

			if(strcmp(dhcpsubnet,ipempty))
				fprintf(fp, "opt subnet %s\n", dhcpsubnet);
			if(strcmp(ipaddr2,ipempty))
				fprintf(fp, "opt router %s\n", ipaddr2);

			// Kaohj
#ifdef DHCPS_DNS_OPTIONS
			if (dhcppoolentry.dnsservermode == 0)
				fprintf(fp, "opt dns %s\n", ipaddr2);
			else { // check manual setting
				strncpy(ipaddr, inet_ntoa(*((struct in_addr *)(dhcppoolentry.dnsserver1))), 16);
				ipaddr[15] = '\0';
				fprintf(fp, "opt dns %s\n", ipaddr);
				strncpy(ipaddr, inet_ntoa(*((struct in_addr *)(dhcppoolentry.dnsserver2))), 16);
				ipaddr[15] = '\0';
				if(strcmp(ipaddr,ipempty)) { // not empty
					fprintf(fp, "opt dns %s\n", ipaddr);
					strncpy(ipaddr, inet_ntoa(*((struct in_addr *)(dhcppoolentry.dnsserver3))), 16);
					ipaddr[15] = '\0';
					if(strcmp(ipaddr,ipempty)) { // not empty
						fprintf(fp, "opt dns %s\n", ipaddr);
					}
				}
			}
#else
			fprintf(fp, "opt dns %s\n", ipaddr2);
#endif
			fprintf(fp, "opt lease %u\n", (unsigned int)(dhcppoolentry.leasetime));
			if (dhcppoolentry.domainname[0])
				fprintf(fp, "opt domain %s\n", dhcppoolentry.domainname);
			else
				// per TR-068, I-188
				fprintf(fp, "opt domain domain_not_set.invalid\n");

#ifdef _CWMP_MIB_ /*jiunming, for cwmp-tr069*/
		{
			//format: opt venspec [enterprise-number] [sub-option code] [sub-option data] ...
			//opt vendspec: dhcp option 125 Vendor-Identifying Vendor-Specific
			//enterprise-number: 3561(ADSL Forum)
			//sub-option code: 4(GatewayManufacturerOUI)
			//sub-option code: 5(GatewaySerialNumber)
			//sub-option code: 6(GatewayProductClass)
			char opt125_sn[65];
			mib_get(MIB_HW_SERIAL_NUMBER, (void *)opt125_sn);
			fprintf( fp, "opt venspec 3561 4 %s 5 %s 6 %s\n", MANUFACTURER_OUI, opt125_sn, PRODUCT_CLASS );
		}
#endif
			//relayinfo
			if( dhcppoolentry.localserved==0 )
			{
				char dhcpserveripaddr[16];
				fprintf(fp, "locallyserved no\n" );
				strncpy(dhcpserveripaddr, inet_ntoa(*((struct in_addr *)(dhcppoolentry.dhcprelayip))), 16);
				dhcpserveripaddr[15]='\0';
				fprintf(fp, "dhcpserver %s\n", dhcpserveripaddr );
			}

			fprintf(fp, "poolend end\n");

		}
	}
#endif
/*ping_zhang:20080919 END*/

	fclose(fp);

//star: retain the lease file between during dhcdp restart
	if((fp = fopen(DHCPD_LEASE, "r")) == NULL)
	{
		if ((fp = fopen(DHCPD_LEASE, "w")) == NULL)
		{
			printf("Open file %s failed !\n", DHCPD_LEASE);
			return -1;
		}
		fprintf(fp, "\n");
		fclose(fp);
	}
	else
		fclose(fp);

	return 1;
}



// Added by Mason Yu for Dhcp Relay
// return value:
// 0  : not active
// 1  : active
// -1 : setup failed
int startDhcpRelay(void)
{
	unsigned char value[32];
	unsigned int dhcpmode=2;
	char dhcps[16];
	int status=0;


	if (mib_get(MIB_DHCP_MODE, (void *)value) != 0)
	{
		dhcpmode = (unsigned int)(*(unsigned char *)value);
		if (dhcpmode != 1 )
			return 0;	// dhcp Relay not on
	}

	// DHCP Relay is on
	if (dhcpmode == 1) {

		//printf("DHCP Relay is on\n");

		if (mib_get(MIB_ADSL_WAN_DHCPS, (void *)value) != 0)
		{
			if (((struct in_addr *)value)->s_addr != 0)
			{
				strncpy(dhcps, inet_ntoa(*((struct in_addr *)value)), 16);
				dhcps[15] = '\0';
			}
			else
				dhcps[0] = '\0';
		}
		else
			return -1;

		//printf("dhcps = %s\n", dhcps);
#ifndef COMBINE_DHCPD_DHCRELAY
		status=va_cmd("/bin/dhcrelay", 1, 0, dhcps);
#else
		status=va_cmd(DHCPD, 2, 0, "-R", dhcps);
#endif
		status=(status==-1)?-1:1;

		return status;

	}

	return status;

}
#endif // of CONFIG_DHCP_SERVER

int delete_line(const char *filename, int delete_line)
{
	FILE *fileptr1, *fileptr2;
	int ch;
	int temp = 1;
	char *tmpFileName="/var/tmp/replica";

	//open file in read mode
	fileptr1 = fopen(filename, "r");
	fseek(fileptr1, 0, SEEK_SET);

	//open new file in write mode
	fileptr2 = fopen(tmpFileName, "w+");
	while ((ch = getc(fileptr1)) != EOF)
	{
		if (ch == '\n')
			temp++;

		//except the line to be deleted
		if (temp != delete_line)
		{
			//copy all lines in file replica.c
			putc(ch, fileptr2);
		}
	}
	fclose(fileptr1);
	fclose(fileptr2);
	remove(filename);
	//rename the file replica.c to original name
	rename(tmpFileName, filename);
	return 0;
}

#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
int delete_hosts(unsigned char *yiaddr)
{
	FILE *fp;
	int dnsrelaypid=0;
	char temps[256+MAX_NAME_LEN]="", *pwd;
	char tmp1[20]="";
	int tmp_line = 0;

del_again:
	if ((fp = fopen(HOSTS, "r")) == NULL)
	{
		printf("Open file %s failed !\n", HOSTS);
		return 0;
	}

	fseek(fp, 0, SEEK_SET);
	temps[0] = '\0';
	tmp1[0] = '\0';
	tmp_line = 0;

	while (fgets(temps, 256+MAX_NAME_LEN, fp)) {
		tmp_line++;
		pwd = strstr(temps, yiaddr);
		if (pwd) {
			if(sscanf(temps, "%s%*", tmp1) == 1)
			{
				if (!strcmp(tmp1, yiaddr)) {
					fclose(fp);
					delete_line(HOSTS, tmp_line);
					goto del_again;
				}
			}
		}
	}
	fclose(fp);

	dnsrelaypid = read_pid((char*)DNSRELAYPID);
	if(dnsrelaypid > 0)
		kill(dnsrelaypid, SIGHUP);
	return 1;
}

int delete_dsldevice_on_hosts()
{
#if 0
	unsigned char value[32];
	unsigned char IPStr[16]={0};

	struct in_addr inAddr;
	if(!getInAddr(ALIASNAME_BR0, IP_ADDR, (void *)&inAddr)){
		return -1;
	}

	sprintf(IPStr, "%s", inet_ntoa(inAddr));
	delete_hosts(IPStr);
#else
	/* delete entry with deviceName */
	unsigned char value;
	char devName[MAX_NAME_LEN]={0};
	FILE *fp;
	char temps[256+MAX_NAME_LEN]="", *pwd;
	int tmp_line = 0;

	mib_get(MIB_DEVICE_NAME, (void *)devName);
del_again:
	if ((fp = fopen(HOSTS, "r")) == NULL)
	{
		printf("Open file %s failed !\n", HOSTS);
		return 0;
	}

	fseek(fp, 0, SEEK_SET);
	temps[0] = '\0';
	tmp_line = 0;

	while (fgets(temps, 256+MAX_NAME_LEN, fp)) {
		tmp_line++;
		pwd = strstr(temps, devName);
		if (pwd) {
			fclose(fp);
			delete_line(HOSTS, tmp_line);
			goto del_again;
		}
	}
	fclose(fp);
#endif
	return 1;
}

int add_dsldevice_on_hosts()
{
	unsigned char value, value2[32];
	FILE *fp;
	char domain[MAX_NAME_LEN];
	char devName[MAX_NAME_LEN];
	int dnsrelaypid=0;

#ifdef CONFIG_USER_DHCP_SERVER
	mib_get(MIB_DHCP_MODE, (void *)&value);
	if (value== DHCP_SERVER)
	{
		if ((fp = fopen(HOSTS, "a")) == NULL)
		{
			printf("Open file %s failed on DHCP_SERVER mode !\n", HOSTS);
			return -1;
		}
	}
	else
#endif
	{
		if ((fp = fopen(HOSTS, "w")) == NULL)
		{
			printf("Open file %s failed on Not DHCP_SERVER mode !\n", HOSTS);
			return -1;
		}
	}

	struct in_addr inAddr;
	if(!getInAddr(ALIASNAME_BR0, IP_ADDR, (void *)&inAddr)){
		printf("[%s] Get %s address fail !\n", __FUNCTION__, ALIASNAME_BR0);
		return -1;
	}

	fprintf(fp, "%s\t", inet_ntoa(inAddr));
	domain[0] = devName[0] = 0;
	mib_get(MIB_DEVICE_NAME, (void *)devName);

#ifdef CONFIG_USER_DHCP_SERVER
	if (value  == DHCP_SERVER)
	{	// add domain
		mib_get(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)domain);
		if (domain[0])
			fprintf(fp, "%s.%s ", devName, domain);
	}
#endif
	fprintf(fp, "%s\n", devName);
	fclose(fp);

	dnsrelaypid = read_pid((char*)DNSRELAYPID);
	if(dnsrelaypid > 0)
		kill(dnsrelaypid, SIGHUP);

	return 1;
}

// DNS relay server configuration
// return value:
// 1  : successful
// -1 : startup failed
int startDnsRelay(void)
{
	unsigned char value[32];
	FILE *fp;
	DNS_TYPE_T dnsMode;
	char *str;
	unsigned int i, vcTotal, resolvopt;
	char dns1[16], dns2[16], dns3[16];
	char dnsv61[48]={0};
	char dnsv62[48]={0};
	char dnsv63[48]={0};
	char domain[MAX_NAME_LEN];
	FILE *dnsfp=fopen(RESOLV,"w");		/* resolv.conf*/

	fprintf(dnsfp, "nameserver 127.0.0.1\n");
#ifdef CONFIG_IPV6
	fprintf(dnsfp, "nameserver ::1\n");
#endif

	fclose(dnsfp);

#ifdef OLD_DNS_MANUALLY
	if (mib_get(MIB_ADSL_WAN_DNS1, (void *)value) != 0)
	{
		if (((struct in_addr *)value)->s_addr != INADDR_NONE && ((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(dns1, inet_ntoa(*((struct in_addr *)value)), 16);
			dns1[15] = '\0';
		}
		else
			dns1[0] = '\0';
	}
	else
		return -1;

	if (mib_get(MIB_ADSL_WAN_DNS2, (void *)value) != 0)
	{
		if (((struct in_addr *)value)->s_addr != INADDR_NONE && ((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(dns2, inet_ntoa(*((struct in_addr *)value)), 16);
			dns2[15] = '\0';
		}
		else
			dns2[0] = '\0';
	}
	else
		return -1;

	if (mib_get(MIB_ADSL_WAN_DNS3, (void *)value) != 0)
	{
		if (((struct in_addr *)value)->s_addr != INADDR_NONE && ((struct in_addr *)value)->s_addr != 0)
		{
			strncpy(dns3, inet_ntoa(*((struct in_addr *)value)), 16);
			dns3[15] = '\0';
		}
		else
			dns3[0] = '\0';
	}
	else
		return -1;

#ifdef CONFIG_IPV6
	if (mib_get(MIB_ADSL_WAN_DNSV61, (void *)value) != 0)	{
		if (memcmp( ((struct in6_addr *)value)->s6_addr, dnsv61, 16))
		{
			inet_ntop(PF_INET6, &value, dnsv61, 48);
			dnsv61[47] = '\0';
		}
		else
			dnsv61[0] = '\0';
	}
	else
		return -1;

	if (mib_get(MIB_ADSL_WAN_DNSV62, (void *)value) != 0)	{
		if (memcmp( ((struct in6_addr *)value)->s6_addr, dnsv62, 16))
		{
			inet_ntop(PF_INET6, &value, dnsv62, 48);
			dnsv62[47] = '\0';
		}
		else
			dnsv62[0] = '\0';
	}
	else
		return -1;

	if (mib_get(MIB_ADSL_WAN_DNSV63, (void *)value) != 0)	{
		if (memcmp( ((struct in6_addr *)value)->s6_addr, dnsv63, 16))
		{
			inet_ntop(PF_INET6, &value, dnsv63, 48);
			dnsv63[47] = '\0';
		}
		else
			dnsv63[0] = '\0';
	}
	else
		return -1;
#endif

	// get DNS mode
	if (mib_get(MIB_ADSL_WAN_DNS_MODE, (void *)value) != 0)
	{
		dnsMode = (DNS_TYPE_T)(*(unsigned char *)value);
	}
	else
		return -1;

	if ((fp = fopen(MANUAL_RESOLV, "w")) == NULL)
	{
		printf("Open file %s failed !\n", MANUAL_RESOLV);
		return -1;
	}

	if (dns1[0])
		fprintf(fp, "nameserver %s\n", dns1);
	if (dns2[0])
		fprintf(fp, "nameserver %s\n", dns2);
	if (dns3[0])
		fprintf(fp, "nameserver %s\n", dns3);
#ifdef CONFIG_IPV6
	if (dnsv61[0])
		fprintf(fp, "nameserver %s\n", dnsv61);
	if (dnsv62[0])
		fprintf(fp, "nameserver %s\n", dnsv62);
	if (dnsv63[0])
		fprintf(fp, "nameserver %s\n", dnsv63);
#endif
	fclose(fp);

	if (dnsMode == DNS_MANUAL)
	{
		// dnsmasq -h -i LANIF -r MANUAL_RESOLV
		TRACE(STA_INFO, "get DNS from manual\n");
		str=(char *)MANUAL_RESOLV;
	}
	else	// DNS_AUTO
	{
#if 0
		MIB_CE_ATM_VC_T Entry;

		resolvopt = 0;
		vcTotal = mib_chain_total(MIB_ATM_VC_TBL);

		for (i = 0; i < vcTotal; i++)
		{
			/* get the specified chain record */
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				return -1;

			if (Entry.enable == 0)
				continue;

			if ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_PPPOE ||
				(CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_PPPOA)
				resolvopt = 1;
			else if ((DHCP_T)Entry.ipDhcp == DHCP_CLIENT)
				resolvopt = 2;
		}

#ifdef CONFIG_USER_PPPOMODEM
		{
			MIB_WAN_3G_T entry,*p;
			p=&entry;
			if(mib_chain_get( MIB_WAN_3G_TBL, 0, (void*)p))
			{
				if(p->enable) resolvopt = 1;
			}
		}
#endif //CONFIG_USER_PPPOMODEM

		if (resolvopt == 1) // get from PPP
		{
			// dnsmasq -h -i LANIF -r PPP_RESOLV
			TRACE(STA_INFO, "get DNS from PPP\n");
			str=(char *)PPP_RESOLV;
		}
		else if (resolvopt == 2) // get from DHCP client
		{
			// dnsmasq -h -i LANIF -r DNS_RESOLV
			TRACE(STA_INFO, "get DNS from DHCP client\n");
			str=(char *)DNS_RESOLV;
		}
		else	// get from manual
		{
			// dnsmasq -h -i LANIF -r MANUAL_RESOLV
			TRACE(STA_INFO, "get DNS from manual\n");
			str=(char *)MANUAL_RESOLV;
		}
#endif

		fp=fopen(AUTO_RESOLV,"r");
		if(fp)
			fclose(fp);
		else{
			fp=fopen(AUTO_RESOLV,"w");
			if(fp)
				fclose(fp);
		}
		str=(char *)AUTO_RESOLV;
	}
#endif

	// create hosts file
	delete_dsldevice_on_hosts();
	add_dsldevice_on_hosts();

	#if 0
	//va_cmd(DNSRELAY, 4, 0, (char *)ARG_I, (char *)LANIF, "-r", str);
	if (va_cmd(DNSRELAY, 2, 0, "-r", str))
	    return -1;
	#endif

#ifdef OLD_DNS_MANUALLY
//star: for the dns restart
	unlink(RESOLV);

	if (symlink(str, RESOLV)) {
			printf("failed to link %s --> %s\n", str, RESOLV);
			return -1;
	}
#endif

	if (va_cmd(DNSRELAY, 4, 0, "-C", DNSMASQ_CONF, "-r", RESOLV))
	    return -1;

	return 1;
}
#endif


#if defined(CONFIG_USER_PPTP_CLIENT_PPTP) || defined(CONFIG_USER_L2TPD_L2TPD)
/*
 *	generate the ifup_ppp(vpc)x script for WAN interface
 */
static int generate_ifup_script_vpn(unsigned int ifIndex)
{
	int ret;
	FILE *fp;
	char wanif[8], ifup_path[32];

	ret = 0;

	ifGetName(ifIndex, wanif, sizeof(wanif));

	if (PPP_INDEX(ifIndex) != DUMMY_PPP_INDEX) {
		// PPP interface

		snprintf(ifup_path, 32, "/var/ppp/ifup_%s", wanif);
		if (fp=fopen(ifup_path, "w+") ) {
			fprintf(fp, "#!/bin/sh\n\n");

			fclose(fp);
			chmod(ifup_path, 484);
		}
#ifdef CONFIG_IPV6_VPN
		snprintf(ifup_path, 32, "/var/ppp/ifupv6_%s", wanif);
		if (fp=fopen(ifup_path, "w+") ) {
			fprintf(fp, "#!/bin/sh\n\n");

			fclose(fp);
			chmod(ifup_path, 484);
		}
#endif
	}
	else {
		// not supported till now
		return -1;
	}

	return ret;
}

/*
 *	generate the ifdown_ppp(vpn)x script for WAN interface
 */
static int generate_ifdown_script_vpn(unsigned int ifIndex)
{
	int ret;
	FILE *fp;
	char wanif[6], ifdown_path[32];
	char devname[IFNAMSIZ];

	ret = 0;

	if (PPP_INDEX(ifIndex) != DUMMY_PPP_INDEX) {
		// PPP interface
		snprintf(wanif, 6, "ppp%u", PPP_INDEX(ifIndex));
		snprintf(ifdown_path, 32, "/var/ppp/ifdown_%s", wanif);
		if (fp=fopen(ifdown_path, "w+") ) {
			fprintf(fp, "#!/bin/sh\n\n");
			fclose(fp);
			chmod(ifdown_path, 484);
		}
#ifdef CONFIG_IPV6_VPN
		snprintf(ifdown_path, 32, "/var/ppp/ifdownv6_%s", wanif);
		if (fp=fopen(ifdown_path, "w+") ) {
			fprintf(fp, "#!/bin/sh\n\n");
			fclose(fp);
			chmod(ifdown_path, 484);
		}
#endif
	}
	else {
		// not supported till now
		return -1;
	}

	return ret;
}

/*
 *	remove the ifup_ppp(vpc)x script for WAN interface
 */
static int remove_ifup_script_vpn(unsigned int ifIndex)
{
	int ret;
	FILE *fp;
	char wanif[8], ifup_path[32];

	ret = 0;

	ifGetName(ifIndex, wanif, sizeof(wanif));

	if (PPP_INDEX(ifIndex) != DUMMY_PPP_INDEX) {
		// PPP interface

		snprintf(ifup_path, 32, "/var/ppp/ifup_%s", wanif);
		unlink(ifup_path);
#ifdef CONFIG_IPV6_VPN
		snprintf(ifup_path, 32, "/var/ppp/ifupv6_%s", wanif);
		unlink(ifup_path);
#endif
	}
	else {
		// not supported till now
		return -1;
	}

	return ret;
}

/*
 *	remove the ifdown_ppp(vpn)x script for WAN interface
 */
static int remove_ifdown_script_vpn(unsigned int ifIndex)
{
	int ret;
	FILE *fp;
	char wanif[6], ifdown_path[32];
	char devname[IFNAMSIZ];

	ret = 0;

	if (PPP_INDEX(ifIndex) != DUMMY_PPP_INDEX) {
		// PPP interface
		snprintf(wanif, 6, "ppp%u", PPP_INDEX(ifIndex));
		snprintf(ifdown_path, 32, "/var/ppp/ifdown_%s", wanif);
		unlink(ifdown_path);
#ifdef CONFIG_IPV6_VPN
		snprintf(ifdown_path, 32, "/var/ppp/ifdownv6_%s", wanif);
		unlink(ifdown_path);
#endif
	}
	else {
		// not supported till now
		return -1;
	}

	return ret;
}
#endif

#ifdef CONFIG_XFRM
#ifdef CONFIG_GPON_FEATURE
void routeInternetWanCheck(void)
{
	int num_wan, wan_idx, interface_flag;
	MIB_CE_ATM_VC_T entry;
	struct in_addr inAddr;
	char ifname[64];

	printf("[%s %d]\n",__func__, __LINE__);
	num_wan = mib_chain_total(MIB_ATM_VC_TBL);
	for (wan_idx = 0; wan_idx < num_wan; wan_idx++)
	{
		if (mib_chain_get(MIB_ATM_VC_TBL, wan_idx, &entry) == 0)
			continue;

		if ((entry.cmode == CHANNEL_MODE_BRIDGE) || !(entry.applicationtype & X_CT_SRV_INTERNET))
			continue;

		ifGetName(entry.ifIndex, ifname, sizeof(ifname));
		if (!getInFlags(ifname, &interface_flag))
			continue;
		if ((interface_flag & IFF_UP) && getInAddr(ifname, IP_ADDR, &inAddr))
		{
			set_smux_gpon_usflow(&entry);
			//return wan_idx;
		}
	}
}
#endif
#endif

#ifdef CONFIG_XFRM
struct IPSEC_PROP_ST ikeProps[] = {
	{1, "------------------------", 0x00000000},
	{1, "des-md5-group1", 0x01010001},//dhGroup|Encryption|ahAuth|Auth
	{1, "des-sha1-group1", 0x01010002},
	{1, "3des-md5-group2", 0x02020001},
	{1, "3des-sha1-group2", 0x02020002},
    {0} //invalid should be the last one!!!!
};

char *curr_dhArray[] = {
	"null",
	"modp768",
	"modp1024",
	NULL
};

char *curr_encryptAlgm[] = {
	"null_enc",
	"des",
	"3des",
#ifdef CONFIG_CRYPTO_AES
	"aes",
#endif
    NULL
};
static char *setkey_encryptAlgm[] = {
	"null_enc",
	"des-cbc",
	"3des-cbc",
#ifdef CONFIG_CRYPTO_AES
	"aes-cbc",
#endif
    NULL
};

char *curr_authAlgm[] = {
	"non_auth",
	"md5",
	"sha1",
	NULL
};
static char *setkey_authAlgm[] = {
	"non_auth",
	"hmac-md5",
	"hmac-sha1",
	NULL
};
static char *racoon_authAlgm[] = {
	"non_auth",
	"hmac_md5",
	"hmac_sha1",
	NULL
};

/*
return
	0: failed
	1: succeed
*/
static int applyIPsec(MIB_IPSEC_T *pEntry)
{
	char *strVal;
	int intVal;
	char local[20], remote[20], homeAddr[20], gwAddr[20];
	char *saProtocol[2]={"esp", "ah"};
	char *transportMode[2]={"tunnel", "transport"};
	char *ikeMode[2]={"main", "aggressive"};
	char *ikeauthmethod[2]={"pre_shared_key", "rsasig"};
	char *filterProtocol[4]={"any", "tcp", "udp", "icmp"};
	struct in_addr ipAddr;
	char filterPort[10];
	FILE *fpPSK, *fpSetKey, *fpRacoon;
	/*
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	*/
	if ((fpSetKey = fopen(SETKEY_CONF, "a"))== NULL){
		printf("ERROR: open file %s error!\n", PSK_FILE);
		return 0;
	}

	strVal = inet_ntoa(*(struct in_addr*)pEntry->remoteIP);
	strncpy(remote, strVal, 20);

	strVal = inet_ntoa(*(struct in_addr*)pEntry->localIP);
	strncpy(local, strVal, 20);

	strVal = inet_ntoa(*(struct in_addr*)pEntry->remoteTunnel);
	strncpy(gwAddr, strVal, 20);

	strVal = inet_ntoa(*(struct in_addr*)pEntry->localTunnel);
	strncpy(homeAddr, strVal, 20);

	/*
	if (getInAddr(pEntry->localWAN, IP_ADDR, (void *)&ipAddr) == 1){
		strVal = inet_ntoa(ipAddr);
		snprintf(homeAddr, 20, "%s", strVal);
	}else{
		printf("ERROR: can't get address of %s!\n", pEntry->localWAN);
		return 0;
	}

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0; i<entryNum; i++){
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)){
			return -1;
		}
		getDisplayWanName((void *)&Entry, Entry.WanName);
		if(!strncmp(Entry.WanName, pEntry->localWAN, sizeof(pEntry->localWAN)))
			break;
	}

	if(i >= entryNum)
		return -1;
	*/
	if(pEntry->filterPort != 0)
		snprintf(filterPort, 10, "%d", pEntry->filterPort);
	else
		snprintf(filterPort, 10, "%s", "any");

	fprintf(fpSetKey, "flush;\n");
	fprintf(fpSetKey, "spdflush;\n\n");
	if(pEntry->negotiationType == 1){
		//manual mode
		if((pEntry->encapMode&0x1) != 0x0){
			fprintf(fpSetKey, "add %s %s esp 0x%x -m %s ", homeAddr,gwAddr,
				pEntry->espOUTSPI, transportMode[pEntry->transportMode]);

			if(pEntry->espEncrypt != 0)
				fprintf(fpSetKey, "-E %s 0x%s ", setkey_encryptAlgm[pEntry->espEncrypt], pEntry->espEncryptKey);
			if(pEntry->espAuth != 0)
				fprintf(fpSetKey, "-A %s 0x%s ", setkey_authAlgm[pEntry->espAuth], pEntry->espAuthKey);

			fprintf(fpSetKey, ";\n\n");

			fprintf(fpSetKey, "add %s %s esp 0x%x -m %s ", gwAddr,homeAddr,
				pEntry->espINSPI, transportMode[pEntry->transportMode]);

			if(pEntry->espEncrypt != 0)
				fprintf(fpSetKey, "-E %s 0x%s ", setkey_encryptAlgm[pEntry->espEncrypt], pEntry->espEncryptKey);
			if(pEntry->espAuth != 0)
				fprintf(fpSetKey, "-A %s 0x%s ", setkey_authAlgm[pEntry->espAuth], pEntry->espAuthKey);

			fprintf(fpSetKey, ";\n\n");
		}

		if((pEntry->encapMode&0x2) != 0x0){
			fprintf(fpSetKey, "add %s %s ah 0x%x -m %s -A %s 0x%s ;\n\n", homeAddr, gwAddr,
				pEntry->ahOUTSPI, transportMode[pEntry->transportMode], setkey_authAlgm[pEntry->ahAuth], pEntry->ahAuthKey);

			fprintf(fpSetKey, "add %s %s ah 0x%x -m %s -A %s 0x%s ;\n\n", gwAddr, homeAddr,
				pEntry->ahINSPI, transportMode[pEntry->transportMode], setkey_authAlgm[pEntry->ahAuth], pEntry->ahAuthKey);

		}
	}

	//direction: in
	if(pEntry->transportMode == 0){
	fprintf(fpSetKey, "spdadd %s/%d[%s] %s/%d[%s] %s -P in ipsec", remote, pEntry->remoteMask, filterPort,
		local, pEntry->localMask, filterPort, filterProtocol[pEntry->filterProtocol]);
		if((pEntry->encapMode&0x1) != 0x0)
			fprintf(fpSetKey, " esp/tunnel/%s-%s/require", gwAddr, homeAddr);
		if((pEntry->encapMode&0x2) != 0x0)
			fprintf(fpSetKey, " ah/tunnel/%s-%s/require", gwAddr, homeAddr);
	}else{
	    fprintf(fpSetKey, "spdadd %s/32[%s] %s/32[%s] %s -P in ipsec", gwAddr, filterPort,
		    homeAddr, filterPort, filterProtocol[pEntry->filterProtocol]);
		if((pEntry->encapMode&0x1) != 0x0)
			fprintf(fpSetKey, " esp/transport//use");
		if((pEntry->encapMode&0x2) != 0x0)
			fprintf(fpSetKey, " ah/transport//use");
	}

	fprintf(fpSetKey, ";\n\n");

	//direction: out
	if(pEntry->transportMode == 0){
	fprintf(fpSetKey, "spdadd %s/%d[%s] %s/%d[%s] %s -P out ipsec", local, pEntry->localMask, filterPort,
		remote, pEntry->remoteMask, filterPort, filterProtocol[pEntry->filterProtocol]);
		if((pEntry->encapMode&0x1) != 0x0)
			fprintf(fpSetKey, " esp/tunnel/%s-%s/require", homeAddr, gwAddr);
		if((pEntry->encapMode&0x2) != 0x0)
			fprintf(fpSetKey, " ah/tunnel/%s-%s/require", homeAddr, gwAddr);
	}else{
	    fprintf(fpSetKey, "spdadd %s/32[%s] %s/32[%s] %s -P out ipsec", homeAddr, filterPort,
    		gwAddr, filterPort, filterProtocol[pEntry->filterProtocol]);
		if((pEntry->encapMode&0x1) != 0x0)
			fprintf(fpSetKey, " esp/transport//require");
		if((pEntry->encapMode&0x2) != 0x0)
			fprintf(fpSetKey, " ah/transport//require");
	}

	fprintf(fpSetKey, ";\n\n");
	fclose(fpSetKey);

	if(pEntry->negotiationType == 0){
		//ike mode
		if ((0==pEntry->auth_method)&&((fpPSK = fopen(PSK_FILE, "a"))== NULL)){
			printf("ERROR: open file %s error!\n", PSK_FILE);
			return 0;
		}

        if ((fpRacoon = fopen(RACOON_CONF, "w+"))== NULL){
				printf("ERROR: open file %s error!\n", RACOON_CONF);
				return 0;
			}

        if(0==pEntry->auth_method)
        {
            //psk mode
		    fprintf(fpPSK, "%s %s\n", gwAddr, pEntry->psk);
		    fclose(fpPSK);
            fprintf(fpRacoon, "path pre_shared_key \"%s\" ;\n\n", PSK_FILE);
			}
        else{
            //certificate mode
            va_cmd("/bin/mkdir", 2, 1, "-p", CERT_FLOD);
            fprintf(fpRacoon, "path certificate \"%s\" ;\n\n", CERT_FLOD);
		}

// Mason Yu.
#if 0
        //for NAT-Traversal
        fprintf(fpRacoon, "timer {\n");
        fprintf(fpRacoon, "\tnatt_keepalive 10sec ;\n");
        fprintf(fpRacoon, "\t}\n");

        fprintf(fpRacoon, "listen {\n");
        fprintf(fpRacoon, "\tisakmp %s [500] ;\n", homeAddr);
        fprintf(fpRacoon, "\tisakmp_natt %s [4500] ;\n", homeAddr);
        fprintf(fpRacoon, "\t}\n");
        //end
#endif        
	fprintf(fpRacoon, "remote %s {\n", gwAddr);
	fprintf(fpRacoon, "\texchange_mode %s ;\n", ikeMode[pEntry->ikeMode]);
// Mason Yu.
	//fprintf(fpRacoon, "\tnat_traversal on ;\n");    
	fprintf(fpRacoon, "\tlifetime time %d second ;\n", pEntry->ikeAliveTime);
        if(1==pEntry->auth_method){
            //certificate mode
            //ToDo:
            fprintf(fpRacoon, "\tcertificate_type x509 \"cert.pem\" \"privKey.pem\" ;\n");
            fprintf(fpRacoon, "\tverify_cert on ;\n");
            fprintf(fpRacoon, "\tmy_identifier asn1dn ;\n");
            fprintf(fpRacoon, "\tpeers_identifier asn1dn ;\n");
        }
            
	for(intVal = 0; intVal<4; intVal++){
		if(pEntry->ikeProposal[intVal]==0)
			continue;
		fprintf(fpRacoon, "\tproposal {\n");
		fprintf(fpRacoon, "\t\tencryption_algorithm %s ;\n", curr_encryptAlgm[ENCRYPT_INDEX(ikeProps[pEntry->ikeProposal[intVal]].algorithm)]);
		fprintf(fpRacoon, "\t\thash_algorithm %s ;\n", curr_authAlgm[AUTH_INDEX(ikeProps[pEntry->ikeProposal[intVal]].algorithm)]);
		fprintf(fpRacoon, "\t\tauthentication_method %s ;\n", ikeauthmethod[pEntry->auth_method]);
		fprintf(fpRacoon, "\t\tdh_group %d ;\n", DHGROUP_INDEX(ikeProps[pEntry->ikeProposal[intVal]].algorithm));
		fprintf(fpRacoon, "\t}\n\n");
	}
	fprintf(fpRacoon, "}\n\n");

	for(intVal = 1; curr_dhArray[intVal]!=NULL; intVal++){
            int i, flag;
			if(0x0 == (pEntry->pfs_group&(1<<intVal)))
				continue;
			fprintf(fpRacoon, "sainfo address %s/%d[%s] %s address %s/%d[%s] %s{\n", pEntry->transportMode==0?local:homeAddr, pEntry->transportMode==0?pEntry->localMask:32, filterPort,
					filterProtocol[pEntry->filterProtocol], pEntry->transportMode==0?remote:gwAddr, pEntry->transportMode==0?pEntry->localMask:32, filterPort, filterProtocol[pEntry->filterProtocol]);
			fprintf(fpRacoon, "\tpfs_group %s ;\n", curr_dhArray[intVal]);
			fprintf(fpRacoon, "\tlifetime time %d second ;\n", pEntry->saAliveTime);
			fprintf(fpRacoon, "\tencryption_algorithm ");
            flag = 0;
            for(i=0; curr_encryptAlgm[i]!=NULL; i++)
            {
                if(pEntry->encrypt_group&(1<<i))
                {   
                    if(0==flag)
                    {
                        fprintf(fpRacoon, "%s", curr_encryptAlgm[i]);
                        flag = 1;
                    }
                    else
                    {
                        fprintf(fpRacoon, ", %s", curr_encryptAlgm[i]);
                    }
                }
            }
            fprintf(fpRacoon, " ;\n");
			fprintf(fpRacoon, "\tauthentication_algorithm ");
            flag = 0;
            for(i=0; racoon_authAlgm[i]!=NULL; i++)
            {
                if(pEntry->auth_group&(1<<i))
                {   
                    if(0==flag)
                    {
                        fprintf(fpRacoon, "%s", racoon_authAlgm[i]);
                        flag = 1;
                    }
                    else
                    {
                        fprintf(fpRacoon, ", %s", racoon_authAlgm[i]);
                    }
                }
            }
            fprintf(fpRacoon, " ;\n");
			fprintf(fpRacoon, "\tcompression_algorithm deflate ;\n");
			fprintf(fpRacoon, "}\n\n");
		}
		fclose(fpRacoon);

	}
	return 1;
}

void apply_nat_rule(MIB_IPSEC_T *pEntry)
{
	char *strVal;
	char filterPort[10];
	char local[24], remote[24];
	char *argv[20];
	int i = 3;
	
	if(1 == pEntry->transportMode)
	{
		//transportMode do not need it.
		return; 
	}
    
	strVal = inet_ntoa(*(struct in_addr*)pEntry->localIP);
	snprintf(local, 24, "%s/%d", strVal, pEntry->localMask);
	strVal = inet_ntoa(*(struct in_addr*)pEntry->remoteIP);
	snprintf(remote, 24, "%s/%d", strVal, pEntry->remoteMask);
	if(pEntry->filterPort != 0)
		snprintf(filterPort, 10, "%d", pEntry->filterPort);

	argv[1] = "-t";
	argv[2] = "nat";
	if(1 == pEntry->state)
	{
		//for tunnel mode, should disable NAT for SPD.
		argv[i++] = (char *)FW_INSERT;
	}
	else
	{
		argv[i++] = (char *)FW_DEL;
	}
	argv[i++] = (char *)FW_POSTROUTING;
    
	if(0 != pEntry->filterProtocol)
	{
		argv[i++] = "-p";
		if(1 == pEntry->filterProtocol)
			argv[i++] = (char *)ARG_TCP;
		else if(2 == pEntry->filterProtocol)
			argv[i++] = (char *)ARG_UDP;
		else
			argv[i++] = (char *)ARG_ICMP;
	}
        
	//src ip
	argv[i++] = "-s";
	argv[i++] = local;

	// src port
	if ((1 == pEntry->filterProtocol) || (2 == pEntry->filterProtocol)) {
		if(0 != pEntry->filterPort){
			argv[i++] = (char *)FW_SPORT;
			argv[i++] = filterPort;
		}
	}

	// dst ip
	argv[i++] = "-d";
	argv[i++] = remote;
        
	// dst port
	if ((1 == pEntry->filterProtocol) || (2 == pEntry->filterProtocol)) {
		if(0 != pEntry->filterPort){
			argv[i++] = (char *)FW_DPORT;
			argv[i++] = filterPort;
		}
	}
        
	argv[i++] = "-j";
	argv[i++] = (char *)FW_RETURN;
	argv[i++] = NULL;

	//printf("i=%d\n", i);
	TRACE(STA_SCRIPT, "%s %s %s %s %s %s %s ...\n", IPTABLES, argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
	do_cmd(IPTABLES, argv, 1);

    return;
}

void add_ipsec_mangle_rule(void)
{
	unsigned char vChar;
	int ori_wlan_idx;
	char tnlName[32], markStr[32];

	snprintf(markStr, 32, "0x%x/0x%x", BYPASS_FWDENGINE_MARK_MASK, BYPASS_FWDENGINE_MARK_MASK);
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "XFRM");
	
	//iptables -t mangle -A XFRM -p esp -j MARK --set-mark 0x8/0x8
	va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, "XFRM", "-p", "esp", "-j", "MARK", 
			"--set-mark", markStr);
	//iptables -t mangle -A XFRM -p ah -j MARK --set-mark 0x8/0x8
	va_cmd(IPTABLES, 10, 1, "-t", "mangle", (char *)FW_ADD, "XFRM", "-p", "ah", "-j", "MARK", 
			"--set-mark", markStr);
	//iptables -t mangle -A XFRM -p udp --dport 500 --sport 500 -j MARK --set-mark 0x8/0x8
	va_cmd(IPTABLES, 14, 1, "-t", "mangle", (char *)FW_ADD, "XFRM", "-p", (char *)ARG_UDP, "--dport", "500", "--sport",
			"500", "-j", "MARK", "--set-mark", markStr);
}

void del_ipsec_mangle_rule(void)
{
	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "XFRM");
}

void ipsec_take_effect(void)
{

	int entryNum, i, newState;
	MIB_IPSEC_T Entry;
	char gwAddr[20];
	FILE *fp;
	char pid[10];


	va_cmd("/bin/setkey", 1, 1, "-F");
	va_cmd("/bin/setkey", 1, 1, "-FP");

	if(access(RACOON_PID, 0)==0){
		if ((fp = fopen(RACOON_PID, "r"))== NULL){
			printf("ERROR: open file %s error!\n", RACOON_PID);
			return;
		}
		fscanf(fp, "%d\n", &i);
		snprintf(pid, 10, "%d", i);
		va_cmd("/bin/kill", 1, 1,  pid);
		unlink(RACOON_PID);
	}

	if(access(PSK_FILE, 0)==0){
		unlink(PSK_FILE);
	}

	if(access(RACOON_LOG, 0)==0){
		unlink(RACOON_LOG);
	}

	if(access(SETKEY_CONF, 0)==0){
		unlink(SETKEY_CONF);
	}

	if(access(RACOON_CONF, 0)==0){
		unlink(RACOON_CONF);
	}

	del_ipsec_mangle_rule();
#ifdef CONFIG_RTK_L34_ENABLE
	RG_flush_ipsec_trap_rule();
#endif

	entryNum = mib_chain_total(MIB_IPSEC_TBL);
	for(i = 0; i<entryNum; i++){
		mib_chain_get(MIB_IPSEC_TBL, i, (void *)&Entry);

		if(Entry.enable ==1){
			newState = applyIPsec(&Entry);
			if(newState != Entry.state){
				Entry.state = newState;
				mib_chain_update(MIB_IPSEC_TBL, &Entry, i);
			}
		}
        else
        {
            if(0 != Entry.state){
                Entry.state = 0;
				mib_chain_update(MIB_IPSEC_TBL, &Entry, i);
            }
        }

        apply_nat_rule(&Entry);
#ifdef CONFIG_RTK_L34_ENABLE
		if (Entry.state)
			RG_add_ipsec_trap_rule(&Entry);
#endif
	}

	if(access(PSK_FILE, 0)==0)
	{
		va_cmd("/bin/chmod", 2, 1, "600", PSK_FILE);
	}

	if(access(SETKEY_CONF, 0)==0){
		va_cmd("/bin/setkey", 2, 1, "-f", SETKEY_CONF);
		//unlink(SETKEY_CONF);
	}

	if(access(RACOON_CONF, 0)==0){
		va_cmd("/bin/racoon", 4, 1, "-f", RACOON_CONF, "-l", RACOON_LOG);
		//unlink(RACOON_CONF);
		add_ipsec_mangle_rule();
#ifdef CONFIG_GPON_FEATURE
		routeInternetWanCheck();
#endif
	}

	return;
}
#endif

#ifdef CONFIG_USER_PPTP_CLIENT_PPTP
#ifdef CONFIG_USER_PPTPD_PPTPD
void applyPptpAccount(MIB_VPN_ACCOUNT_T *pentry, int enable)
{
	MIB_VPND_T server;
	int total, i;
	struct data_to_pass_st msg;
	char index[5];
	char auth[20];
	char enctype[20];
	char localip[20], peerip[20];

	if (1 == enable) {//add a pptp account

		if (0 == pentry->enable)
			return;

		//get server auth info
		total = mib_chain_total(MIB_VPN_SERVER_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, &server))
				continue;

			if (VPN_PPTP == server.type)
				break;
		}

		if (i >= total) {
			printf("please configure pptp server info first.\n");
			return;
		}

		switch (server.authtype)
		{
		case 1://pap
			strcpy(auth, "pap");
			break;
		case 2://chap
			strcpy(auth, "chap");
			break;
		case 3://chapmsv2
			strcpy(auth, "chapms-v2");
			break;
		default:
			strcpy(auth, "auto");
			break;
		}

		switch (server.enctype)
		{
		case 1://MPPE
			strcpy(enctype, "+MPPE");
			break;
        case 2://MPPC
			strcpy(enctype, "+MPPC");
			break;
		case 3://MPPE&MPPC
			strcpy(enctype, "+BOTH");
			break;
		default:
			strcpy(enctype, "none");
			break;
		}

		snprintf(localip, 20, "%s", inet_ntoa(*(struct in_addr *)&server.localaddr));
		snprintf(peerip, 20, "%s", inet_ntoa(*(struct in_addr *)&server.peeraddr));

		snprintf(msg.data, BUF_SIZE, "spppctl addvpn %s type PPTP auth %s enctype %s username %s password %s localip %s peerip %s",
					pentry->name, auth, enctype, pentry->username, pentry->password, localip, peerip);

		printf("%s: %s\n", __func__, msg.data);

		write_to_pppd(&msg);
	}
	else {
		printf("/bin/spppctl delvpn %s\n", pentry->name);

		va_cmd("/bin/spppctl", 2, 1, "delvpn", pentry->name);
	}
}

void pptpd_take_effect(void)
{
	MIB_VPN_ACCOUNT_T entry;
	unsigned int entrynum, i;
	int enable;

	if ( !mib_get(MIB_PPTP_ENABLE, (void *)&enable) )
		return;

	entrynum = mib_chain_total(MIB_VPN_ACCOUNT_TBL);
	for (i=0; i<entrynum; i++)
	{
		if (!mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&entry))
			continue;
		if(VPN_PPTP == entry.type)
			applyPptpAccount(&entry, 0);
	}

	if (enable) {
		for (i=0; i<entrynum; i++)
		{
			if (!mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&entry))
				continue;
			if(VPN_PPTP == entry.type)
				applyPptpAccount(&entry, 1);
		}
	}
}
#endif

void applyPPtP(MIB_PPTP_T *pentry, int enable, int pptp_index)
{
	struct data_to_pass_st msg;
	char index[5];
	char auth[20];
	char enctype[20];
	// Mason Yu. Add VPN ifIndex
	// unit declarations for ppp  on if_sppp.h
	// (1) 0 ~ 7: pppoe/pppoa, (2) 8: 3G, (3) 9 ~ 10: PPTP, (4) 11 ~12: L2TP
	if (enable == 1) {	// add(persistent) or new(Dial-on-demand)
		switch (pentry->authtype)
		{
		case 1://pap
			strcpy(auth, "pap");
			break;
		case 2://chap
			strcpy(auth, "chap");
			break;
		case 3://chapmsv2
			strcpy(auth, "chapms-v2");
			break;
		default:
			strcpy(auth, "auto");
			break;
		}

		switch (pentry->enctype)
		{
		case 1://MPPE
			strcpy(enctype, "+MPPE");
			break;
        case 2://MPPC
			strcpy(enctype, "+MPPC");
			break;
		case 3://MPPE&MPPC
			strcpy(enctype, "+BOTH");
			break;
		default:
			strcpy(enctype, "none");
			break;
		}

		if (pentry->dgw) {
			snprintf(msg.data, BUF_SIZE,
				"spppctl add %d pptp auth %s username %s password %s server %s gw %d enctype %s", pentry->idx+9,
				auth, pentry->username, pentry->password, pentry->server, pentry->dgw, enctype);
		}
		else {
			snprintf(msg.data, BUF_SIZE,
				"spppctl add %d pptp auth %s username %s password %s server %s enctype %s", pentry->idx+9,
				auth, pentry->username, pentry->password, pentry->server, enctype);
		}

#ifdef CONFIG_IPV6_VPN
		snprintf(msg.data, BUF_SIZE, "%s ipt %u", msg.data, pEntry->IpProtocol - 1);
#endif

		printf("%s: %s\n", __func__, msg.data);

		generate_ifup_script_vpn(pentry->ifIndex);
		generate_ifdown_script_vpn(pentry->ifIndex);

		write_to_pppd(&msg);
#ifdef CONFIG_RTK_L34_ENABLE
		//RG add pptp wan!
		RG_add_pptp_wan(pentry,pptp_index);
#endif
	}
	else {
#ifdef CONFIG_RTK_L34_ENABLE
		//RG del pptp wan!
		if(pentry->rg_wan_idx>0){
			RG_WAN_Interface_Del(pentry->rg_wan_idx);
			pentry->rg_wan_idx=0;
			mib_chain_update(MIB_PPTP_TBL, pentry, pptp_index);
		}
#endif
		snprintf(index, 5, "%d", pentry->idx+9);
		va_cmd("/bin/spppctl", 4, 1, "del", index, "pptp", "0");
		remove_ifup_script_vpn(pentry->ifIndex);
		remove_ifdown_script_vpn(pentry->ifIndex);
	}
}

void pptp_take_effect(void)
{
	MIB_PPTP_T entry;
	unsigned int entrynum, i;
	int enable;

	if ( !mib_get(MIB_PPTP_ENABLE, (void *)&enable) )
		return;

	entrynum = mib_chain_total(MIB_PPTP_TBL);

	for (i=0; i<entrynum; i++)
	{
		if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&entry) )
			return;

		applyPPtP(&entry, 0, i);
	}

	if (enable) {
		for (i=0; i<entrynum; i++)
		{
			if ( !mib_chain_get(MIB_PPTP_TBL, i, (void *)&entry) )
				return;

			applyPPtP(&entry, 1, i);
		}
	}
}
#endif

#ifdef CONFIG_USER_L2TPD_L2TPD
static const char *tunnel_auth[2] = {"none", "challenge"};
#endif
#ifdef CONFIG_USER_L2TPD_LNS
void applyL2tpAccount(MIB_VPN_ACCOUNT_T *pentry, int enable)
{
	MIB_VPND_T server;
	int total, i;
	struct data_to_pass_st msg;
	char index[5];
	char auth[20];
	char enctype[20];
	char localip[20], peerip[20];

	if (1 == enable) {//add a pptp account

		if (0 == pentry->enable)
			return;

		//get server auth info
		total = mib_chain_total(MIB_VPN_SERVER_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_VPN_SERVER_TBL, i, &server))
				continue;

			if (VPN_L2TP == server.type)
				break;
		}

		if (i >= total) {
			printf("please configure l2tp server info first.\n");
			return;
		}

		switch (server.authtype)
		{
		case 1://pap
			strcpy(auth, "pap");
			break;
		case 2://chap
			strcpy(auth, "chap");
			break;
		case 3://chapmsv2
			strcpy(auth, "chapms-v2");
			break;
		default:
			strcpy(auth, "auto");
			break;
		}

		switch (server.enctype)
		{
		case 1://MPPE
			strcpy(enctype, "+MPPE");
			break;
        case 2://MPPC
			strcpy(enctype, "+MPPC");
			break;
		case 3://MPPE&MPPC
			strcpy(enctype, "+BOTH");
			break;
		default:
			strcpy(enctype, "none");
			break;
		}

		snprintf(localip, 20, "%s", inet_ntoa(*(struct in_addr *)&server.localaddr));
		snprintf(peerip, 20, "%s", inet_ntoa(*(struct in_addr *)&server.peeraddr));

		snprintf(msg.data, BUF_SIZE, "spppctl addvpn %s type L2TP auth %s enctype %s username %s password %s localip %s peerip %s",
					pentry->name, auth, enctype, pentry->username, pentry->password, localip, peerip);

		snprintf(msg.data, BUF_SIZE, "%s tunnel_auth %s", msg.data, tunnel_auth[server.tunnel_auth]);
		if (server.tunnel_auth) {
			snprintf(msg.data, BUF_SIZE, "%s secret %s", msg.data, server.tunnel_key);
		}

		printf("%s: %s\n", __func__, msg.data);

		write_to_pppd(&msg);
	}
	else {
		printf("/bin/spppctl delvpn %s\n", pentry->name);

		va_cmd("/bin/spppctl", 2, 1, "delvpn", pentry->name);
	}
}

void l2tpd_take_effect(void)
{
	MIB_VPN_ACCOUNT_T entry;
	unsigned int entrynum, i;
	int enable;

	if ( !mib_get(MIB_L2TP_ENABLE, (void *)&enable) )
		return;

	entrynum = mib_chain_total(MIB_VPN_ACCOUNT_TBL);
	for (i=0; i<entrynum; i++)
	{
		if (!mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&entry))
			continue;
		if(VPN_L2TP == entry.type)
			applyL2tpAccount(&entry, 0);
	}

	if (enable) {
		for (i=0; i<entrynum; i++)
		{
			if (!mib_chain_get(MIB_VPN_ACCOUNT_TBL, i, (void *)&entry))
				continue;
			if(VPN_L2TP == entry.type)
				applyL2tpAccount(&entry, 1);
		}
	}
}
#endif

#ifdef CONFIG_USER_L2TPD_L2TPD
static const char *l2tp_auth[] = {
	"auto", "pap", "chap", "chapms-v2"
};

static const char *l2tp_encryt[] = {
	"none", "+MPPE", "+MPPC", "+BOTH"
};

int applyL2TP(MIB_L2TP_T *pentry, int enable, int l2tp_index)
{
	struct data_to_pass_st msg;
	char index[5];

	// Mason Yu. Add VPN ifIndex
	// unit declarations for ppp  on if_sppp.h
	// (1) 0 ~ 7: pppoe/pppoa, (2) 8: 3G, (3) 9 ~ 10: PPTP, (4) 11 ~12: L2TP
	if (enable == 1) { 	// add(persistent) or new(Dial-on-demand)
		if (pentry->conntype != MANUAL) {
			snprintf(msg.data, BUF_SIZE,
				"spppctl add %d l2tp username %s password %s gw %d mru %d", pentry->idx+11,
				pentry->username, pentry->password, pentry->dgw, pentry->mtu);
		}
		else {
			snprintf(msg.data, BUF_SIZE,
				"spppctl new %d l2tp username %s password %s gw %d mru %d", pentry->idx+11,
				pentry->username, pentry->password, pentry->dgw, pentry->mtu);
		}

		snprintf(msg.data, BUF_SIZE, "%s auth %s", msg.data, l2tp_auth[pentry->authtype]);

		snprintf(msg.data, BUF_SIZE, "%s enctype %s", msg.data, l2tp_encryt[pentry->enctype]);

		snprintf(msg.data, BUF_SIZE, "%s server %s", msg.data, pentry->server);

		snprintf(msg.data, BUF_SIZE, "%s tunnel_auth %s", msg.data, tunnel_auth[pentry->tunnel_auth]);
		if (pentry->tunnel_auth) {
			snprintf(msg.data, BUF_SIZE, "%s secret %s", msg.data, pentry->secret);
		}

		if (pentry->conntype == CONNECT_ON_DEMAND)
			snprintf(msg.data, BUF_SIZE, "%s timeout %d", msg.data, pentry->idletime);

#ifdef CONFIG_IPV6_VPN
		snprintf(msg.data, BUF_SIZE, "%s ipt %u", msg.data, pEntry->IpProtocol - 1);
#endif

		printf("%s: %s\n", __func__, msg.data);

		generate_ifup_script_vpn(pentry->ifIndex);
		generate_ifdown_script_vpn(pentry->ifIndex);
#ifdef CONFIG_RTK_L34_ENABLE
		//RG add pptp wan!
		RG_add_l2tp_wan(pentry,l2tp_index);
#endif

		write_to_pppd(&msg);
	}
	else if (enable == 2) {		// connect(up) for dial on-demand
		snprintf(msg.data, BUF_SIZE, "spppctl up %d", pentry->idx+11);
#ifdef CONFIG_RTK_L34_ENABLE
		//RG add pptp wan!
		RG_add_l2tp_wan(pentry,l2tp_index);
#endif
		write_to_pppd(&msg);
	}
	else if (enable == 3) {		// disconnect(down) for dial on-demand
		snprintf(msg.data, BUF_SIZE, "spppctl down %d", pentry->idx+11);
#ifdef CONFIG_RTK_L34_ENABLE
		//RG add pptp wan!
		RG_add_l2tp_wan(pentry,l2tp_index);
#endif
		write_to_pppd(&msg);
	}
	else {
#ifdef CONFIG_RTK_L34_ENABLE
		//RG del l2tp wan!
		if(pentry->rg_wan_idx>0){
			RG_WAN_Interface_Del(pentry->rg_wan_idx);
			pentry->rg_wan_idx=0;
			mib_chain_update(MIB_L2TP_TBL, pentry, l2tp_index);
		}
#endif
		snprintf(index, 5, "%d", pentry->idx+11);
		va_cmd("/bin/spppctl", 4, 1, "del", index, "l2tp", "0");
		remove_ifup_script_vpn(pentry->ifIndex);
		remove_ifdown_script_vpn(pentry->ifIndex);
	}

	return 1;
}

void l2tp_take_effect(void)
{
	MIB_L2TP_T entry;
	unsigned int entrynum, i;//, j;
	int enable;

	if ( !mib_get(MIB_L2TP_ENABLE, (void *)&enable) )
		return;

	entrynum = mib_chain_total(MIB_L2TP_TBL);

	//delete all firstly
	for (i=0; i<entrynum; i++)
	{
		if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&entry) )
			return;

		applyL2TP(&entry, 0, i);
	}

	if (enable) {
		for (i=0; i<entrynum; i++)
		{
			if ( !mib_chain_get(MIB_L2TP_TBL, i, (void *)&entry) )
				return;

			applyL2TP(&entry, 1, i);
		}
	}
}
#endif

#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
#define SNMPPID  "/var/run/snmpd.pid"

#ifdef CONFIG_USER_SNMPD_SNMPD_V2CTRAP
// Added by Mason Yu for ILMI(PVC) community string
static const char PVCREADSTR[] = "ADSL";
static const char PVCWRITESTR[] = "ADSL";
// Added by Mason Yu for write community string
static const char SNMPCOMMSTR[] = "/var/snmpComStr.conf";
// return value:
// 1  : successful
// -1 : startup failed
int startSnmp(void)
{
	unsigned char value[16];
	unsigned char trapip[16];
	unsigned char commRW[100], commRO[100], enterOID[100];
	FILE 	      *fp;

	// Get SNMP Trap Host IP Address
	if(!mib_get( MIB_SNMP_TRAP_IP,  (void *)value)){
		printf("Can no read MIB_SNMP_TRAP_IP\n");
	}

	if (((struct in_addr *)value)->s_addr != 0)
	{
		strncpy(trapip, inet_ntoa(*((struct in_addr *)value)), 16);
		trapip[15] = '\0';
	}
	else
		trapip[0] = '\0';
	//printf("***** trapip = %s\n", trapip);

	// Get CommunityRO String
	if(!mib_get( MIB_SNMP_COMM_RO,  (void *)commRO)) {
		printf("Can no read MIB_SNMP_COMM_RO\n");
	}
	//printf("*****buffer = %s\n", commRO);


	// Get CommunityRW String
	if(!mib_get( MIB_SNMP_COMM_RW,  (void *)commRW)) {
		printf("Can no read MIB_SNMP_COMM_RW\n");
	}
	//printf("*****commRW = %s\n", commRW);


	// Get Enterprise OID
	if(!mib_get( MIB_SNMP_SYS_OID,  (void *)enterOID)) {
		printf("Can no read MIB_SNMP_SYS_OID\n");
	}
	//printf("*****enterOID = %s\n", enterOID);


	// Write community string to file
	if ((fp = fopen(SNMPCOMMSTR, "w")) == NULL)
	{
		printf("Open file %s failed !\n", SNMPCOMMSTR);
		return -1;
	}

	if (commRO[0])
		fprintf(fp, "readStr %s\n", commRO);
	if (commRW[0])
		fprintf(fp, "writeStr %s\n", commRW);

	// Add ILMI(PVC) community string
	fprintf(fp, "PvcReadStr %s\n", PVCREADSTR);
	fprintf(fp, "PvcWriteStr %s\n", PVCWRITESTR);
	fclose(fp);

	// Mason Yu
	// ZyXEL Remote management does not verify the comm string, so we can limit the comm string as "ADSL"
	if (va_cmd("/bin/snmpd", 8, 0, "-p", "161", "-c", PVCREADSTR, "-th", trapip, "-te", enterOID))
	    return -1;
	return 1;

}
#else
#define SNMP_VERSION_2 1
#define SNMP_VERSION_3 2

static const char NETSNMPCONF[] = "/var/snmpd.conf";

int startSnmp(void)
{
	unsigned char commRW[100], commRO[100];
	FILE *fp;
	char buf[256];
	unsigned char value[16];
	unsigned char trapip[16];
	unsigned char roName[SNMP_STRING_LEN], roPwd[SNMP_STRING_LEN];
	unsigned char roAuth;
	unsigned char rwName[SNMP_STRING_LEN], rwPwd[SNMP_STRING_LEN];
	unsigned char rwAuth, rwPrivacy;
	unsigned char snmpVersion = 1;
	char str_val[256];
	char * auth[3] = {"MD5", "SHA", ""};
	char * privacy[2] = {"DES", "AES"};

	// Get SNMP Trap Host IP Address
	if(!mib_get( MIB_SNMP_TRAP_IP,  (void *)value)){
		printf("Can no read MIB_SNMP_TRAP_IP\n");
	}

	if (((struct in_addr *)value)->s_addr != 0)
	{
		strncpy(trapip, inet_ntoa(*((struct in_addr *)value)), 16);
		trapip[15] = '\0';
	}
	else
		trapip[0] = '\0';
	
	// Get CommunityRO String
	if(!mib_get( MIB_SNMP_COMM_RO,  (void *)commRO)) {
		printf("Can't read MIB_SNMP_COMM_RO\n");
	}
	
	// Get CommunityRW String
	if(!mib_get( MIB_SNMP_COMM_RW,  (void *)commRW)) {
		printf("Can't read MIB_SNMP_COMM_RW\n");
	}

	if(!mib_get( MIB_SNMPV3_RO_NAME, (void*)roName)) {
		printf("Can't read MIB_SNMPV3_RO_NAME\n");
	}

	if(!mib_get( MIB_SNMPV3_RO_PASSWORD, (void*)roPwd)) {
		printf("Can't read MIB_SNMPV3_RO_PASSWORD\n");
	}
	
	if(!mib_get( MIB_SNMPV3_RO_AUTH, (void*)&roAuth)) {
		printf("Can't read MIB_SNMPV3_RO_AUTH\n");
	}
		
	if(!mib_get( MIB_SNMPV3_RW_NAME, (void*)rwName)) {
		printf("Can't read MIB_SNMPV3_RW_NAME\n");
	}
	
	if(!mib_get( MIB_SNMPV3_RW_PASSWORD, (void*)rwPwd)) {
		printf("Can't read MIB_SNMPV3_RW_PASSWORD\n");
	}
	
	if(!mib_get( MIB_SNMPV3_RW_AUTH, (void*)&rwAuth)) {
		printf("Can't read MIB_SNMPV3_RW_AUTH\n");
	}
	
	if(!mib_get( MIB_SNMPV3_RW_PRIVACY, (void*)&rwPrivacy)) {
		printf("Can't read MIB_SNMPV3_RW_PRIVACY\n");
	}

	if(!mib_get( MIB_SNMP_VERSION, (void*)&snmpVersion)) {
		printf("Can't read MIB_SNMP_VERSION\n");
	}
	
	// Write community string to file
	if ((fp = fopen(NETSNMPCONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", NETSNMPCONF);
		return -1;
	}

	if(snmpVersion & SNMP_VERSION_2)
	{
		if (commRO[0] && commRW[0] && (strcmp(commRO,commRW) == 0))
		{
			/*if read-only user name and read-write user name are the same, 
			  the action of net-snmp is ambiguity, so only set read-write user */
			fprintf(fp, "rwcommunity %s\n", commRW);
		}
		else
		{
			if (commRO[0])
				fprintf(fp, "rocommunity %s\n", commRO);
			if (commRW[0])
				fprintf(fp, "rwcommunity %s\n", commRW);
		}
	}

	if(snmpVersion & SNMP_VERSION_3)
	{
		if(roName[0])
		{
			if(roAuth == SNMP_AUTH_NONE)
			{
				fprintf(fp, "createuser %s\n", roName);
				fprintf(fp, "rouser %s noauth\n", roName);
			}
			else if(roPwd[0])
			{
				fprintf(fp, "createuser %s %s \"%s\"\n", roName, auth[roAuth], roPwd);
				fprintf(fp, "rouser %s\n", roName);
			}
		}
		if(rwName[0] && rwPwd[0])
		{
			fprintf(fp, "createuser %s %s \"%s\" %s\n", rwName, auth[rwAuth], rwPwd, privacy[rwPrivacy]);
			fprintf(fp, "rwuser %s\n", rwName);
		}
	}
	
	if(trapip[0])
	{
		fprintf(fp, "trapsink %s:162\n", trapip); /* enable snmpv1 trap */
		fprintf(fp, "trap2sink %s:162\n", trapip); /* enable snmpv2 trap */
	}

	/* config values for net-snmp to set the corresponding oid values */
	if ( mib_get(MIB_SNMP_SYS_DESCR, (void *)str_val)) {			
		fprintf(fp, "sysdescr %s\n", str_val);
	}

	if ( mib_get(MIB_SNMP_SYS_OID, (void *)str_val)) {			
		fprintf(fp, "sysobjectid %s\n", str_val);
	}
	
	if ( mib_get(MIB_SNMP_SYS_CONTACT, (void *)str_val)) {			
		fprintf(fp, "psyscontact %s\n", str_val);
	}

	if ( mib_get(MIB_SNMP_SYS_NAME, (void *)str_val)) {			
		fprintf(fp, "psysname %s\n", str_val);
	}

	if ( mib_get(MIB_SNMP_SYS_LOCATION, (void *)str_val)) {			
		fprintf(fp, "psyslocation %s\n", str_val);
	}
	fclose(fp);

	snprintf(buf, 256, "snmpd -r -L -c %s -p %s", NETSNMPCONF, SNMPPID);
	system(buf);

	return 1;
}
#endif

int restart_snmp(int flag)
{
	unsigned char value[32];
	int snmppid=0;
	int status=0;

	snmppid = read_pid((char*)SNMPPID);

	//printf("\nsnmppid=%d\n",snmppid);

	if(snmppid > 0) {
		kill(snmppid, 9);
		unlink(SNMPPID);
	}

	if(flag==1){
		status = startSnmp();
	}
	return status;
}
#endif

#define CWMPPID  "/var/run/cwmp.pid"
#if defined(CONFIG_USER_CWMP_TR069) || defined(APPLY_CHANGE)
void off_tr069(void)
{
	int cwmppid=0;
	int status;

	cwmppid = read_pid((char*)CWMPPID);

	printf("\ncwmppid=%d\n",cwmppid);

	if(cwmppid > 0)
		kill(cwmppid, 15);

}
#endif

#if defined(IP_ACL)
/*
 *	Return value:
 *	0	: fail
 *	1	: successful
 *	pMsg	: error message
 */
int checkACL(char *pMsg, int idx, unsigned char aclcap)
{
	int i, entryNum;
	MIB_CE_ACL_IP_T Entry;	
	
	// if ACL is disable, it just return success.
	if (!aclcap) return 1;
		
	entryNum = mib_chain_total(MIB_ACL_IP_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ACL_IP_TBL, i, (void *)&Entry))
		{
  			//boaError(wp, 400, "Get chain record error!\n");
			strcpy(pMsg, "Get chain record error!\n");
			return 0;
		}
		
		if (idx != -1) {
			if (idx == i) continue;
		}
			
		if (Entry.Interface == IF_DOMAIN_WAN)
			continue;
		
		if ((Entry.web & 0x02) || (Entry.any==0x01))  // can web access from LAN
			return 1;
	}
	strcpy(pMsg, "Sorry! There must be at lease one IP from LAN that can reach WUI !!\n");
	return 0;
}

int restart_acl(void)
{
	unsigned char aclEnable;
	char msg[100];

	mib_get(MIB_ACL_CAPABILITY, &aclEnable);
	filter_set_acl(0);
	if (aclEnable == 1) {  // ACL Capability is enabled
		if (checkACL(msg, -1, 1)) {
			filter_set_acl(1);
		} else {
			fputs(msg, stderr);
			aclEnable = 0;
			mib_set(MIB_ACL_CAPABILITY, &aclEnable);
		}
	}

	return 0;
}

#ifdef CONFIG_IPV6
void set_v6_acl_service(unsigned char service, char *act, char *strIP, char *protocol, unsigned short port)
{
	char strInputPort[8]="";
	
	snprintf(strInputPort, sizeof(strInputPort), "%d", port);
	if (service & 0x02) {	// can LAN access
		// ip6tables -A inacc -i $LAN_IF -s 2001::100/64 -p UDP --dport 69 -j ACCEPT
		va_cmd(IP6TABLES, 12, 1, act, (char *)FW_ACL,
			(char *)ARG_I, (char *)LANIF, "-s", strIP, "-p", protocol,
			(char *)FW_DPORT, strInputPort, "-j", (char *)FW_ACCEPT);
	}
	if (service & 0x01) {	// can WAN access
		// ip6tables -A inacc -i ! $LAN_IF -s 2001::100/64 -p UDP --dport 69 -j ACCEPT
		va_cmd(IP6TABLES, 13, 1, act, (char *)FW_ACL,
			"!", (char *)ARG_I, (char *)LANIF, "-s", strIP, "-p", protocol,
			(char *)FW_DPORT, strInputPort, "-j", (char *)FW_ACCEPT);
	}
}

void filter_set_v6_acl_service(MIB_CE_V6_ACL_IP_T Entry, int enable, char *strIP)
{
	char *act;
	char strPort[8];
	int ret;


	if (enable)
		act = (char *)FW_ADD;
	else
		act = (char *)FW_DEL;

	// telnet service: bring up by inetd
	#ifdef CONFIG_USER_TELNETD_TELNETD
	set_v6_acl_service(Entry.telnet, act, strIP, (char *)ARG_TCP, 23);
	#endif

	#ifdef CONFIG_USER_FTPD_FTPD
	// ftp service: bring up by inetd
	set_v6_acl_service(Entry.ftp, act, strIP, (char *)ARG_TCP, 21);
	#endif	

	#ifdef CONFIG_USER_TFTPD_TFTPD
	// tftp service: bring up by inetd
	set_v6_acl_service(Entry.tftp, act, strIP, (char *)ARG_UDP, 69);
	#endif

	// HTTP service
	set_v6_acl_service(Entry.web, act, strIP, (char *)ARG_TCP, 80);
	
	#ifdef CONFIG_USER_BOA_WITH_SSL
  	//HTTPS service
	set_v6_acl_service(Entry.https, act, strIP, (char *)ARG_TCP, 443);
	#endif

	#ifdef CONFIG_USER_SSH_DROPBEAR
	// ssh service	
	set_v6_acl_service(Entry.ssh, act, strIP, (char *)ARG_TCP, 22);
	#endif
	
	// ping service
	if (Entry.icmp & 0x02) // can LAN access
	{
		// ip6tables -A aclblock -i $LAN_IF  -s 2001::100/64 -p ICMPV6 -m limit --limit 1/s -j ACCEPT
		va_cmd(IP6TABLES, 14, 1, act, (char *)FW_ACL,
			(char *)ARG_I, (char *)LANIF, "-s", strIP, "-p", "ICMPV6",
			"-m", "limit",
			"--limit", "1/s", "-j", (char *)FW_ACCEPT);
	}
	if (Entry.icmp & 0x01) // can WAN access
	{
		// ip6tables -A aclblock -i ! $LAN_IF  -s 2001::100/64 -p ICMPV6 -m limit --limit 1/s -j ACCEPT		
		va_cmd(IP6TABLES, 15, 1, act, (char *)FW_ACL,
			"!", (char *)ARG_I, (char *)LANIF, "-s", strIP, "-p", "ICMPV6",
			"-m", "limit",
			"--limit", "1/s", "-j", (char *)FW_ACCEPT);
	}	
}

void filter_set_v6_acl(int enable)
{
	int i, total;
	char ssrc[55];
	MIB_CE_V6_ACL_IP_T Entry;
	unsigned char dhcpvalue[32];
	unsigned char vChar;
	int dhcpmode;
	
	// check if ACL Capability is enabled ?
	if (!enable)
		va_cmd(IP6TABLES, 2, 1, "-F", (char *)FW_ACL);
	else {
		// Add policy to aclblock chain
		total = mib_chain_total(MIB_V6_ACL_IP_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_V6_ACL_IP_TBL, i, (void *)&Entry))
			{
				return;
			}

			// Check if this entry is enabled
			if ( Entry.Enabled == 1 ) {
				inet_ntop(PF_INET6, (struct in6_addr *)Entry.sip6Start, ssrc, 48);
				if (Entry.sip6PrefixLen!=0)
					snprintf(ssrc, sizeof(ssrc), "%s/%d", ssrc, Entry.sip6PrefixLen);
		
		        	if ( Entry.Interface == IF_DOMAIN_LAN ) {
					if (Entry.any == 0x01)  	// service = any(0x01)
						va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-i", BRIF, "-s", ssrc, "-j", (char *)FW_RETURN);
					else
						filter_set_v6_acl_service(Entry, enable, ssrc);
				} 
				else
					filter_set_v6_acl_service(Entry, enable, ssrc);
			}
		}

		// (1) allow for LAN
		// allowing DNS request during ACL enabled
		// ip6tables -A aclblock -p udp -i br0 --dport 53 -j RETURN		
		va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "udp", "-i", BRIF,(char*)FW_DPORT, (char *)PORT_DNS, "-j", (char *)FW_RETURN);
		// allowing DHCP request during ACL enabled
		// ip6tables -A aclblock -p udp -i br0 --dport 67 -j RETURN
		va_cmd(IP6TABLES, 10, 1, (char *)FW_ADD, (char *)FW_ACL, "-p", "udp", "-i", BRIF, (char*)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_RETURN);
		
		// ip6tables -A aclblock -i lo -j RETURN
		// Local Out DNS query will refer the /etc/resolv.conf(DNS server is 127.0.0.1). We should accept this DNS query when enable ACL.
		va_cmd(IP6TABLES, 6, 1, (char *)FW_ADD, (char *)FW_ACL, "-i", "lo", "-j", (char *)FW_RETURN);
		
		// (2) allow for WAN
		// (2.1) Added by Mason Yu for dhcp Relay. Open DHCP Relay Port for Incoming Packets.
#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
		if (mib_get(MIB_DHCPV6_MODE, (void *)dhcpvalue) != 0)
		{
			dhcpmode = (unsigned int)(*(unsigned char *)dhcpvalue);
			if (dhcpmode == 1 || dhcpmode == 2 ){
				// ip6tables -A aclblock -i ! br0 -p udp --dport 67 -j ACCEPT
				va_cmd(IP6TABLES, 11, 1, (char *)FW_ADD, (char *)FW_ACL, "!", (char *)ARG_I, BRIF, "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_ACCEPT);
			}
		}
#endif

		// accept related	
		// ip6tables -A aclblock -m state --state ESTABLISHED,RELATED -j ACCEPT
		va_cmd(IP6TABLES, 8, 1, (char *)FW_ADD, (char *)FW_ACL, "-m", "state",
			"--state", "ESTABLISHED,RELATED", "-j", (char *)FW_ACCEPT);

		/* Fix the WAN DHCPv6 get IP FAIL issue */
		// ip6tables -I aclblock -i ! br0 -p udp --dport 546 -j ACCEPT
		va_cmd(IP6TABLES, 11, 1, (char *)FW_INSERT, (char *)FW_ACL, "!", (char *)ARG_I, BRIF, "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCPV6, "-j", (char *)FW_ACCEPT);
			
		// ip6tables -A aclblock -j DROP
		va_cmd(IP6TABLES, 4, 1, (char *)FW_ADD, (char *)FW_ACL, "-j", (char *)FW_DROP);
	}
}

int restart_v6_acl()
{
	unsigned char aclEnable;
	
	mib_get(MIB_V6_ACL_CAPABILITY, (void *)&aclEnable);
	if (aclEnable == 1)  // ACL Capability is enabled
	{
		filter_set_v6_acl(0);
		filter_set_v6_acl(1);
	}
	else
		filter_set_v6_acl(0);

	return 0;
}
#endif
#endif

#ifdef _CWMP_MIB_
void filter_set_cwmp_acl_service(int enable, char *strIP, char *strPort)
{
	char *act;	

	if (enable)
		act = (char *)FW_ADD;
	else
		act = (char *)FW_DEL;	
			
	va_cmd(IPTABLES, 17, 1, ARG_T, "mangle", "-A", (char *)FW_CWMP_ACL,
		"!", (char *)ARG_I, (char *)LANIF, "-s", strIP, "-p", (char *)ARG_TCP,
		(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);				
}

void filter_set_cwmp_acl(int enable)
{
	int i, total;
	struct in_addr src;
	char ssrc[40];
	MIB_CWMP_ACL_IP_T Entry;
	unsigned char dhcpMode;
	unsigned char vChar;
	char strPort[16];
	unsigned int conreq_port=0;
	
	// check if ACL Capability is enabled ?
	if (!enable) {
		va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_CWMP_ACL);		
		// Flush all rule in nat table
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", (char *)FW_CWMP_ACL);
	}
	else {
		// Get TR069 port number
		mib_get(CWMP_CONREQ_PORT, (void *)&conreq_port);
		if(conreq_port==0) conreq_port=7547;
		sprintf(strPort, "%u", conreq_port );
	
		// Add policy to cwmp_aclblock chain
		total = mib_chain_total(MIB_CWMP_ACL_IP_TBL);
		for (i=0; i<total; i++) {
			if (!mib_chain_get(MIB_CWMP_ACL_IP_TBL, i, (void *)&Entry))
			{
				return;
			}

//    		printf("\nstartip=%x,endip=%x\n",*(unsigned long*)Entry.startipAddr,*(unsigned long*)Entry.endipAddr);
						
			src.s_addr = *(unsigned long *)Entry.ipAddr;

			// inet_ntoa is not reentrant, we have to
			// copy the static memory before reuse it
			snprintf(ssrc, sizeof(ssrc), "%s/%hhu", inet_ntoa(src), Entry.maskbit);			
			filter_set_cwmp_acl_service(enable, ssrc, strPort);			
		}
		
		// allow for WAN
		// allow packet with the same mark value(0x1000) from WAN
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_CWMP_ACL,
			"!", (char *)ARG_I, LANIF, "-m", "mark", "--mark", RMACC_MARK, "-j", FW_ACCEPT);		
		
		// iptables -A cwmp_aclblock ! -i br0 -p tcp --dport 7547 -j DROP
		va_cmd(IPTABLES, 11, 1, (char *)FW_ADD, (char *)FW_CWMP_ACL, "!", (char *)ARG_I, (char *)LANIF, 
			"-p", (char *)ARG_TCP, (char *)FW_DPORT, strPort, "-j", (char *)FW_DROP);
	}
}

int restart_cwmp_acl(void)
{
	unsigned char cwmp_aclEnable;
	char msg[100];
	char strPort[16];
	unsigned int conreq_port=0;
	
	mib_get(CWMP_ACL_ENABLE, &cwmp_aclEnable);
	filter_set_cwmp_acl(0);
	if (cwmp_aclEnable == 1)  // CWMP ACL Capability is enabled
		filter_set_cwmp_acl(1);
	else
	{
		/*add a wan port to pass */
		mib_get(CWMP_CONREQ_PORT, (void *)&conreq_port);
		if(conreq_port==0) conreq_port=7547;
		sprintf(strPort, "%u", conreq_port );
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", (char *)FW_ADD, (char *)FW_CWMP_ACL,
			"!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_TCP,
			(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);
	}
	return 0;
}
#endif

#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
//DNSRELAY

int restart_dnsrelay(void)
{
	unsigned char value[32];
	int dnsrelaypid=0, i;

	dnsrelaypid = read_pid((char*)DNSRELAYPID);

	//printf("\ndnsrelaypid=%d\n",dnsrelaypid);

	if(dnsrelaypid > 0)
        {
		kill(dnsrelaypid, SIGTERM);
		// Kaohj -- wait for process termination	
                i = 0;
                while (kill(dnsrelaypid, 0) == 0 && i < 10) {
                        usleep(100000); i++;
			//printf("wait for DNSmasq process termination\n");
                }
                if(i >= 10 )
                        kill(dnsrelaypid, SIGKILL);
	}

	//waitpid(dnsrelaypid, &i, WNOHANG);

	if (startDnsRelay() == -1)
	{
		printf("restart DNS relay failed !\n");
		return -1;
	}
	
	// Mason Yu. We should exit this function until dnsmasq process is created.
	// in order to prevent double run dnsmasq. (dnsmasq: failed to create listening socket: Address already in use)
	while (read_pid((char*)DNSRELAYPID) < 0 ) {
		usleep(10000);
		//printf("wait for dnsmasq process is created\n");
	}
	return 0;

}
#endif

#ifdef CONFIG_USER_DHCP_SERVER
int restart_dhcp(void)
{
	unsigned char value[32];
	unsigned int uInt;
	int dhcpserverpid=0,dhcprelaypid=0;
	int tmp_status, status=0;

	if (mib_get(MIB_DHCP_MODE, (void *)value) != 0)
	{
		uInt = (unsigned int)(*(unsigned char *)value);
//		if (uInt != 0 && uInt !=1 && uInt != 2 )
//			return -1;
	}

	dhcpserverpid = read_pid((char*)DHCPSERVERPID);
	dhcprelaypid = read_pid((char*)DHCPRELAYPID);

	//printf("\ndhcpserverpid=%d,dhcprelaypid=%d\n",dhcpserverpid,dhcprelaypid);

	if(dhcpserverpid > 0)
		kill(dhcpserverpid, 15);
	if(dhcprelaypid > 0)
		kill(dhcprelaypid, 15);
/*ping_zhang:20090319 START:fix 2 dhcp will appear bug*/
	//add to check if old udhcp is exit.
	while(read_pid((char*)DHCPSERVERPID)>0 ||read_pid((char*)DHCPRELAYPID)>0)
	{
		usleep(30000);
	}
/*ping_zhang:20090319 END*/
	if(uInt != DHCP_LAN_SERVER)
	{
		FILE *fp;
		//star:clean the lease file
		if ((fp = fopen(DHCPD_LEASE, "w")) == NULL)
		{
			printf("Open file %s failed !\n", DHCPD_LEASE);
			return -1;
		}
		fprintf(fp, "\n");
		fclose(fp);
	}


	if(uInt == DHCP_LAN_SERVER)
	{
		tmp_status = setupDhcpd();
		if (tmp_status == 1)
		{
			//printf("\nrestart dhcpserver!\n");
#ifdef COMBINE_DHCPD_DHCRELAY
			status = va_cmd(DHCPD, 2, 0, "-S", DHCPD_CONF);
#else
			status = va_cmd(DHCPD, 1, 0, DHCPD_CONF);
#endif

			while(read_pid((char*)DHCPSERVERPID) < 0)
				usleep(250000);

#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
			restart_dnsrelay();
#endif
		}
		else if (tmp_status == -1)
	   	 	status = -1;
		return status;
	}
	else if(uInt == DHCP_LAN_RELAY)
	{
		startDhcpRelay();
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
		restart_dnsrelay();
#endif
		status=(status==-1)?-1:0;

		return status;
	}
	else
	{
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
		restart_dnsrelay();
#endif
		return 0;
	}



	return -1;
}
#endif

//Ip interface
int restart_lanip(void)
{
	char vChar=0;
	unsigned char ipaddr[32]="";
	unsigned char subnet[32]="";
	unsigned char value[6];
	int vInt;
	int status=0;
	FILE * fp;
	
#ifdef IP_PASSTHROUGH
	unsigned char ippt_addr[32]="";
	unsigned int ippt_itf;
	unsigned long myipaddr,hisipaddr;
	int ippt_flag = 0;
#endif

	itfcfg((char *)LANIF, 0);
#ifdef CONFIG_SECONDARY_IP
	itfcfg((char *)LAN_ALIAS, 0);
#endif
#ifdef IP_PASSTHROUGH
	itfcfg((char *)LAN_IPPT, 0);
	if (mib_get(MIB_IPPT_ITF, (void *)&ippt_itf) != 0) {
		if (ippt_itf != DUMMY_IFINDEX) {	// IP passthrough
			fp = fopen ("/tmp/PPPHalfBridge", "r");
			if (fp) {
				fread(&myipaddr, 4, 1, fp);
				// Added by Mason Yu. Access internet fail.
				fclose(fp);
				myipaddr+=1;
				strncpy(ippt_addr, inet_ntoa(*((struct in_addr *)(&myipaddr))), 16);
				ippt_addr[15] = '\0';
				ippt_flag =1;
			}

		}
	}
#endif

#ifdef _CWMP_MIB_
	mib_get(CWMP_LAN_IPIFENABLE, (void *)&vChar);
	if (vChar != 0)
#endif
	{
		if (mib_get(MIB_ADSL_LAN_IP, (void *)value) != 0)
		{
			strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
			ipaddr[15] = '\0';
		}
		if (mib_get(MIB_ADSL_LAN_SUBNET, (void *)value) != 0)
		{
			strncpy(subnet, inet_ntoa(*((struct in_addr *)value)), 16);
			subnet[15] = '\0';
		}

		vInt = 1500;
		snprintf(value, 6, "%d", vInt);
		// set LAN-side MRU
		status|=va_cmd(IFCONFIG, 6, 1, (char*)LANIF, ipaddr, "netmask", subnet, "mtu", value);


#ifdef CONFIG_SECONDARY_IP
		mib_get(MIB_ADSL_LAN_ENABLE_IP2, (void *)value);
		if (value[0] == 1) {
			// ifconfig LANIF LAN_IP netmask LAN_SUBNET
			if (mib_get(MIB_ADSL_LAN_IP2, (void *)value) != 0)
			{
				strncpy(ipaddr, inet_ntoa(*((struct in_addr *)value)), 16);
				ipaddr[15] = '\0';
			}
			if (mib_get(MIB_ADSL_LAN_SUBNET2, (void *)value) != 0)
			{
				strncpy(subnet, inet_ntoa(*((struct in_addr *)value)), 16);
				subnet[15] = '\0';
			}
			snprintf(value, 6, "%d", vInt);
			// set LAN-side MRU
			status|=va_cmd(IFCONFIG, 6, 1, (char*)LAN_ALIAS, ipaddr, "netmask", subnet, "mtu", value);
		}
#endif


#ifdef IP_PASSTHROUGH
               if(ippt_flag)
			   status|=va_cmd(IFCONFIG, 2, 1, (char*)LAN_IPPT,ippt_addr);
#endif

#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_ENABLE)
		RG_reset_LAN();
#endif

#ifdef CONFIG_USER_DHCP_SERVER
		status|=restart_dhcp();
#endif

// Trigger igmpproxy to update interface info.
#ifdef CONFIG_USER_IGMPPROXY
		vInt = read_pid("/var/run/igmp_pid");
		if (vInt >= 1) {
			// Kick to sync the multicast virtual interfaces
			kill(vInt, SIGUSR1);
		}
#endif
		// Kaohj -- Disconnect network connection when LAN IP changes
		usleep(250000);
		cmd_killproc(PID_SHIFT(PID_TELNETD));
		return status;
	}
	return -1;
}

//DHCP

#ifdef EMBED
int getOneDhcpClient(char **ppStart, unsigned long *size, char *ip, char *mac, char *liveTime)
{
	struct dhcpOfferedAddr {
		u_int8_t chaddr[16];
		u_int32_t yiaddr;       /* network order */
		u_int32_t expires;      /* host order */
		u_int32_t interfaceType;
		u_int8_t hostName[64];
#ifdef _PRMT_X_CT_SUPPER_DHCP_LEASE_SC
		int category;
		int isCtcVendor;
		char szVendor[36];
		char szModel[36];
		char szFQDN[64];
#endif
	};

	struct dhcpOfferedAddr entry;

	if ( *size < sizeof(entry) )
		return -1;

	entry = *((struct dhcpOfferedAddr *)*ppStart);
	*ppStart = *ppStart + sizeof(entry);
	*size = *size - sizeof(entry);

	if (entry.expires == 0)
		return 0;
//star: conflict ip addr will not be displayed on web
	if(entry.chaddr[0]==0&&entry.chaddr[1]==0&&entry.chaddr[2]==0&&entry.chaddr[3]==0&&entry.chaddr[4]==0&&entry.chaddr[5]==0)
		return 0;

	strcpy(ip, inet_ntoa(*((struct in_addr *)&entry.yiaddr)) );
	snprintf(mac, 20, "%02x:%02x:%02x:%02x:%02x:%02x",
			entry.chaddr[0],entry.chaddr[1],entry.chaddr[2],entry.chaddr[3],
			entry.chaddr[4], entry.chaddr[5]);

	snprintf(liveTime, 10, "%lu", (unsigned long)ntohl(entry.expires));

	return 1;
}
#endif

int set_dhcp_port_base_filter(int enable)
{
	int j;
	unsigned char value[32];
	unsigned short bitmap;

	if (!enable)
	{
		va_cmd(IPTABLES, 2, 1, "-F", (char *)FW_DHCP_PORT_FILTER);
		return 1;
	}
	else
	{
		if(!mib_get( MIB_DHCP_PORT_FILTER,  (void *)value))
			return 0;

		bitmap = (*(unsigned short *)value);
		for(j = PMAP_ETH0_SW0; j <= PMAP_ETH0_SW3 && j < ELANVIF_NUM; ++j)
		{
			if (bitmap & (0x1 << j))
			{
				#ifdef _LINUX_3_18_
				va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char *)FW_DHCP_PORT_FILTER, "-m", "physdev", "--physdev-in", (char *)ELANVIF[j], "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);
				#else
				// iptables -A dhcp_port_filter -i eth0.2 -p udp --dport 67 -j DROP
				va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_DHCP_PORT_FILTER, (char *)ARG_I, (char *)ELANVIF[j], "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);
				#endif
			}
		}
#ifdef WLAN_SUPPORT
		for(j = PMAP_WLAN0; j <= PMAP_WLAN1_VAP3; ++j)
		{
			if (bitmap & (0x1 << j))
			{
				// iptables -A dhcp_port_filter -i wlan0 -p udp --dport 67 -j DROP
				#ifdef _LINUX_3_18_
				va_cmd(IPTABLES, 12, 1, (char *)FW_ADD, (char *)FW_DHCP_PORT_FILTER, "-m", "physdev", "--physdev-in", wlan[j - PMAP_WLAN0], "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);
				#else
				va_cmd(IPTABLES, 10, 1, (char *)FW_ADD, (char *)FW_DHCP_PORT_FILTER, (char *)ARG_I, wlan[j - PMAP_WLAN0], "-p", "udp", (char *)FW_DPORT, (char *)PORT_DHCP, "-j", (char *)FW_DROP);
				#endif
			}
		}
#endif
		return 1;
	}
}

#ifdef PORT_FORWARD_GENERAL
int delPortForwarding( unsigned int ifindex )
{
	int total,i;

	total = mib_chain_total( MIB_PORT_FW_TBL );
	//for( i=0;i<total;i++ )
	for( i=total-1;i>=0;i-- )
	{
		MIB_CE_PORT_FW_T *c, port_entity;
		c = &port_entity;
		if( !mib_chain_get( MIB_PORT_FW_TBL, i, (void*)c ) )
			continue;

		if(c->ifIndex==ifindex)
			mib_chain_delete( MIB_PORT_FW_TBL, i );
	}
	return 0;
}

int updatePortForwarding( unsigned int old_id, unsigned int new_id )
{
	unsigned int total,i;

	total = mib_chain_total( MIB_PORT_FW_TBL );
	for( i=0;i<total;i++ )
	{
		MIB_CE_PORT_FW_T *c, port_entity;
		c = &port_entity;
		if( !mib_chain_get( MIB_PORT_FW_TBL, i, (void*)c ) )
			continue;

		if(c->ifIndex==old_id)
		{
			c->ifIndex = new_id;
			mib_chain_update( MIB_PORT_FW_TBL, (unsigned char*)c, i );
		}
	}
	return 0;
}
#endif

#ifdef CONFIG_USER_ROUTED_ROUTED
int delRipTable(unsigned int ifindex)
{
	int total,i;
	MIB_CE_RIP_T Entry;

	total=mib_chain_total(MIB_RIP_TBL);

	for(i=total-1;i>=0;i--)	{
		if(!mib_chain_get(MIB_RIP_TBL,i,&Entry))
			continue;
		if (Entry.ifIndex!=ifindex) continue;
		if(mib_chain_delete(MIB_RIP_TBL,i)!=1){
			printf("Delete MIB_RIP_TBL chain record error!\n");
		}
	}

	return 0;
}
#endif

#ifdef ROUTING
int delRoutingTable( unsigned int ifindex )
{
	int total,i;

	total = mib_chain_total( MIB_IP_ROUTE_TBL );
#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_ENABLE)
	Flush_RG_static_route();
#endif
	//for( i=0;i<total;i++ )
	for( i=total-1;i>=0;i-- )
	{
		MIB_CE_IP_ROUTE_T *c, entity;
		c = &entity;
		if( !mib_chain_get( MIB_IP_ROUTE_TBL, i, (void*)c ) )
			continue;

		if(c->ifIndex==ifindex)
			mib_chain_delete( MIB_IP_ROUTE_TBL, i );
	}
	return 0;
}

int updateRoutingTable( unsigned int old_id, unsigned int new_id )
{
	unsigned int total,i;

	total = mib_chain_total( MIB_IP_ROUTE_TBL );
	for( i=0;i<total;i++ )
	{
		MIB_CE_IP_ROUTE_T *c, entity;
		c = &entity;
		if( !mib_chain_get( MIB_IP_ROUTE_TBL, i, (void*)c ) )
			continue;

		if(c->ifIndex==old_id)
		{
			c->ifIndex = new_id;
			mib_chain_update( MIB_IP_ROUTE_TBL, (unsigned char*)c, i );
		}
	}
	return 0;
}

#endif

int delPPPoESession(unsigned int ifindex)
{
	int totalEntry, i;
	MIB_CE_PPPOE_SESSION_T Entry;

	totalEntry = mib_chain_total(MIB_PPPOE_SESSION_TBL);
	for (i = totalEntry - 1; i >= 0; i--) {
		if (!mib_chain_get(MIB_PPPOE_SESSION_TBL, i, &Entry)) {
			printf("Get chain record error!\n");
			continue;
		}

		if (Entry.uifno == ifindex) {
			mib_chain_delete(MIB_PPPOE_SESSION_TBL, i);
		}
	}
	return 0;
}

/*ql:20080926 START: delete MIB_DHCP_CLIENT_OPTION_TBL entry to this pvc*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
int delDhcpcOption( unsigned int ifindex )
{
	MIB_CE_DHCP_OPTION_T entry;
	int i, entrynum;

	entrynum = mib_chain_total(MIB_DHCP_CLIENT_OPTION_TBL);
	for (i=entrynum-1; i>=0; i--)
	{
		if (!mib_chain_get(MIB_DHCP_CLIENT_OPTION_TBL, i, (void *)&entry))
			continue;

		if (entry.ifIndex == ifindex)
			mib_chain_delete(MIB_DHCP_CLIENT_OPTION_TBL, i);
	}
	return 0;
}

void compact_reqoption_order(unsigned int ifIndex)
{
	//int ret=-1;
	int num,i,j;
	int maxorder;
	MIB_CE_DHCP_OPTION_T *p,pentry;
	char *orderflag;

	while(1){
		p=&pentry;
		maxorder=findMaxDHCPReqOptionOrder(ifIndex);
		orderflag=(char*)malloc(maxorder+1);
		if(orderflag==NULL) return;
		memset(orderflag,0,maxorder+1);

		num = mib_chain_total( MIB_DHCP_CLIENT_OPTION_TBL );
		for( i=0; i<num;i++ )
		{

				if( !mib_chain_get( MIB_DHCP_CLIENT_OPTION_TBL, i, (void*)p ))
					continue;
				if(p->usedFor!=eUsedFor_DHCPClient_Req)
					continue;
				if(p->ifIndex != ifIndex)
					continue;
				orderflag[p->order]=1;
		}
		for(j=1;j<=maxorder;j++){
			if(orderflag[j]==0)
				break;
		} //star: there only one 0 in orderflag array
		if(j==(maxorder+1))
		{
			if(orderflag)
			{
				free(orderflag);
				orderflag=NULL;
			}
			break;
		}
		for( i=0; i<num;i++ )
		{

				if( !mib_chain_get( MIB_DHCP_CLIENT_OPTION_TBL, i, (void*)p ))
					continue;
				if(p->usedFor!=eUsedFor_DHCPClient_Req)
					continue;
				if(p->ifIndex != ifIndex)
					continue;
				if(p->order>j){
					(p->order)--;
					mib_chain_update(MIB_DHCP_CLIENT_OPTION_TBL,(void*)p,i);
				}
		}

		if(orderflag)
		{
			free(orderflag);
			orderflag=NULL;
		}
	}

}
#endif

/*ql:20080926 END*/
MIB_CE_ATM_VC_T *getATMVCEntryByIfIndex(unsigned int ifIndex, MIB_CE_ATM_VC_T *p)
{
	unsigned int i,num;

	if( (p==NULL) || (ifIndex==DUMMY_IFINDEX) ) return NULL;

	num = mib_chain_total( MIB_ATM_VC_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_ATM_VC_TBL, i, (void*)p ))
			continue;

		if( p->ifIndex==ifIndex )
		{
			return p;
		}
	}

	return NULL;
}

#ifdef _CWMP_MIB_

unsigned int findMaxConDevInstNum(MEDIA_TYPE_T mType)
{
	unsigned int ret=0, i,num;
	MIB_CE_ATM_VC_T *p,vc_entity;

	num = mib_chain_total( MIB_ATM_VC_TBL );
	for( i=0; i<num;i++ )
	{
		p = &vc_entity;
		if( !mib_chain_get( MIB_ATM_VC_TBL, i, (void*)p ))
			continue;
		if (MEDIA_INDEX(p->ifIndex) != mType)
			continue;
		if( p->ConDevInstNum > ret )
			ret = p->ConDevInstNum;
	}

	return ret;
}

unsigned int findConDevInstNumByPVC(unsigned char vpi, unsigned short vci)
{
	unsigned int ret=0, i,num;
	MIB_CE_ATM_VC_T *p,vc_entity;

	if( (vpi==0) && (vci==0) ) return ret;

	num = mib_chain_total( MIB_ATM_VC_TBL );
	for( i=0; i<num;i++ )
	{
		p = &vc_entity;
		if( !mib_chain_get( MIB_ATM_VC_TBL, i, (void*)p ))
			continue;
		if( p->vpi==vpi && p->vci==vci )
		{
			ret = p->ConDevInstNum;
			break;
		}
	}

	return ret;
}

unsigned int findMaxPPPConInstNum(MEDIA_TYPE_T mType, unsigned int condev_inst)
{
	unsigned int ret=0, i,num;
	MIB_CE_ATM_VC_T *p,vc_entity;

	if(condev_inst==0) return ret;

	num = mib_chain_total( MIB_ATM_VC_TBL );
	for( i=0; i<num;i++ )
	{
		p = &vc_entity;
		if( !mib_chain_get( MIB_ATM_VC_TBL, i, (void*)p ))
			continue;
		if (MEDIA_INDEX(p->ifIndex) != mType)
			continue;
		if( p->ConDevInstNum == condev_inst )
		{
			if( (p->cmode==CHANNEL_MODE_PPPOE) ||
#ifdef PPPOE_PASSTHROUGH
			    ((p->cmode==CHANNEL_MODE_BRIDGE)&&(p->brmode==BRIDGE_PPPOE)) ||
#endif
			    (p->cmode==CHANNEL_MODE_PPPOA) )
			{
				if( p->ConPPPInstNum > ret )
					ret = p->ConPPPInstNum;
			}
		}
	}

	return ret;
}

unsigned int findMaxIPConInstNum(MEDIA_TYPE_T mType, unsigned int condev_inst)
{
	unsigned int ret=0, i,num;
	MIB_CE_ATM_VC_T *p,vc_entity;

	if(condev_inst==0) return ret;

	num = mib_chain_total( MIB_ATM_VC_TBL );
	for( i=0; i<num;i++ )
	{
		p = &vc_entity;
		if( !mib_chain_get( MIB_ATM_VC_TBL, i, (void*)p ))
			continue;
		if (MEDIA_INDEX(p->ifIndex) != mType)
			continue;
		if( p->ConDevInstNum==condev_inst )
		{
			if( (p->cmode == CHANNEL_MODE_IPOE) ||
#ifdef PPPOE_PASSTHROUGH
			    ((p->cmode==CHANNEL_MODE_BRIDGE)&&(p->brmode!=BRIDGE_PPPOE)) ||
#else
			    (p->cmode == CHANNEL_MODE_BRIDGE) ||
#endif
			    (p->cmode == CHANNEL_MODE_RT1483) )
			{
				if( p->ConIPInstNum > ret )
					ret = p->ConIPInstNum;
			}
		}
	}

	return ret;
}


/*start use_fun_call_for_wan_instnum*/
int resetWanInstNum(MIB_CE_ATM_VC_Tp entry)
{
	if(entry==NULL) return -1;

	entry->connDisable=0;/*0: always for web/cli*/
	entry->ConDevInstNum=0;
	entry->ConIPInstNum=0;
	entry->ConPPPInstNum=0;
	return 0;
}

int updateWanInstNum(MIB_CE_ATM_VC_Tp entry)
{
	if(entry==NULL) return -1;

#ifdef CONFIG_00R0
	//Rostelecom PON always use WANConnectionDevice.1.
	entry->ConDevInstNum = 1;
#else
	if(entry->ConDevInstNum==0)
		entry->ConDevInstNum = 1 + findMaxConDevInstNum(MEDIA_INDEX(entry->ifIndex));
#endif

	if( (entry->cmode==CHANNEL_MODE_PPPOE) ||
#ifdef PPPOE_PASSTHROUGH
	    ((entry->cmode==CHANNEL_MODE_BRIDGE)&&(entry->brmode==BRIDGE_PPPOE)) ||
#endif
	    (entry->cmode==CHANNEL_MODE_PPPOA) )
	{
		if(entry->ConPPPInstNum==0)
			entry->ConPPPInstNum = 1 + findMaxPPPConInstNum(MEDIA_INDEX(entry->ifIndex), entry->ConDevInstNum );

		entry->ConIPInstNum = 0;
	}else{
		entry->ConPPPInstNum = 0;

		if(entry->ConIPInstNum==0)
			entry->ConIPInstNum = 1 + findMaxIPConInstNum(MEDIA_INDEX(entry->ifIndex), entry->ConDevInstNum);
	}

	return 0;
}
/*end use_fun_call_for_wan_instnum*/


/*ping_zhang:20080919 START:add for new telefonica tr069 request: dhcp option*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
unsigned int findMaxDHCPOptionInstNum( unsigned int usedFor, unsigned int dhcpConSPInstNum)
{
	unsigned int ret=0, i,num;
	MIB_CE_DHCP_OPTION_T *p,DHCPOption_entity;

	num = mib_chain_total( MIB_DHCP_SERVER_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPOption_entity;
		if( !mib_chain_get( MIB_DHCP_SERVER_OPTION_TBL, i, (void*)p ))
			continue;
		if(p->usedFor!=usedFor || dhcpConSPInstNum != p->dhcpConSPInstNum)
			continue;
		if(p->dhcpOptInstNum>ret)
			ret=p->dhcpOptInstNum;
	}

	return ret;
}

int getDHCPOptionByOptInstNum( unsigned int dhcpOptNum, unsigned int dhcpSPNum, unsigned int usedFor, MIB_CE_DHCP_OPTION_T *p, unsigned int *id )
{
	int ret=-1;
	unsigned int i,num;

	if( (dhcpOptNum==0) || (p==NULL) || (id==NULL) )
		return ret;

	num = mib_chain_total( MIB_DHCP_SERVER_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_DHCP_SERVER_OPTION_TBL, i, (void*)p ) )
			continue;

		if( (p->usedFor==usedFor) && (p->dhcpOptInstNum==dhcpOptNum) && (p->dhcpConSPInstNum==dhcpSPNum) )
		{
			*id = i;
			ret = 0;
			break;
		}
	}
	return ret;
}

int getDHCPClientOptionByOptInstNum( unsigned int dhcpOptNum, unsigned int ifIndex, unsigned int usedFor, MIB_CE_DHCP_OPTION_T *p, unsigned int *id )
{
	int ret=-1;
	unsigned int i,num;

	if( (dhcpOptNum==0) || (p==NULL) || (id==NULL) )
		return ret;

	num = mib_chain_total( MIB_DHCP_CLIENT_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_DHCP_CLIENT_OPTION_TBL, i, (void*)p ) )
			continue;

		if( (p->usedFor==usedFor) && (p->dhcpOptInstNum==dhcpOptNum) &&(p->ifIndex==ifIndex) )
		{
			*id = i;
			ret = 0;
			break;
		}
	}
	return ret;
}

unsigned int findMaxDHCPClientOptionInstNum(int usedFor, unsigned int ifIndex)
{
	unsigned int ret=0, i,num;
	MIB_CE_DHCP_OPTION_T *p,DHCPOption_entity;

	num = mib_chain_total( MIB_DHCP_CLIENT_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPOption_entity;
		if( !mib_chain_get( MIB_DHCP_CLIENT_OPTION_TBL, i, (void*)p ))
			continue;
		if(p->usedFor!=usedFor||p->ifIndex!=ifIndex)
			continue;
		if(p->dhcpOptInstNum>ret)
			ret=p->dhcpOptInstNum;
	}

	return ret;

}

unsigned int findDHCPOptionNum(int usedFor, unsigned int ifIndex)
{
	unsigned int ret=0, i,num;
	MIB_CE_DHCP_OPTION_T *p,DHCPOption_entity;

	num = mib_chain_total( MIB_DHCP_CLIENT_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPOption_entity;
		if( !mib_chain_get( MIB_DHCP_CLIENT_OPTION_TBL, i, (void*)p ))
			continue;
		if(p->usedFor==usedFor && p->ifIndex==ifIndex)
			ret++;
	}

	return ret;

}

unsigned int findMaxDHCPReqOptionOrder(unsigned int ifIndex)
{
	unsigned int ret=0, i,num;
	MIB_CE_DHCP_OPTION_T *p,DHCPSP_entity;

	num = mib_chain_total( MIB_DHCP_CLIENT_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPSP_entity;
		if( !mib_chain_get( MIB_DHCP_CLIENT_OPTION_TBL, i, (void*)p ))
			continue;
		if(p->usedFor!=eUsedFor_DHCPClient_Req)
			continue;
		if(p->ifIndex != ifIndex)
			continue;
		if(p->order>ret)
			ret=p->order;
	}

	return ret;
}

unsigned int findMaxDHCPConSPInsNum(void )
{
	unsigned int ret=0, i,num;
	DHCPS_SERVING_POOL_T *p,DHCPSP_entity;

	num = mib_chain_total( MIB_DHCPS_SERVING_POOL_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPSP_entity;
		if( !mib_chain_get( MIB_DHCPS_SERVING_POOL_TBL, i, (void*)p ))
			continue;
		if(p->InstanceNum>ret)
			ret=p->InstanceNum;
	}

	return ret;
}

unsigned int findMaxDHCPConSPOrder(void )
{
	unsigned int ret=0, i,num;
	DHCPS_SERVING_POOL_T *p,DHCPSP_entity;

	num = mib_chain_total( MIB_DHCPS_SERVING_POOL_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPSP_entity;
		if( !mib_chain_get( MIB_DHCPS_SERVING_POOL_TBL, i, (void*)p ))
			continue;
		if(p->poolorder>ret)
			ret=p->poolorder;
	}

	return ret;
}

int getDHCPConSPByInstNum( unsigned int dhcpspNum,  DHCPS_SERVING_POOL_T *p, unsigned int *id )
{
	int ret=-1;
	unsigned int i,num;

	if( (dhcpspNum==0) || (p==NULL) || (id==NULL) )
		return ret;

	num = mib_chain_total( MIB_DHCPS_SERVING_POOL_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_DHCPS_SERVING_POOL_TBL, i, (void*)p ) )
			continue;

		if( p->InstanceNum==dhcpspNum)
		{
			*id = i;
			ret = 0;
			break;
		}
	}
	return ret;
}

/*ping_zhang:20090319 START:replace ip range with serving pool of tr069*/
void clearOptTbl(unsigned int instnum)
{
	unsigned int  i,num,found;
	MIB_CE_DHCP_OPTION_T *p,DHCPOption_entity;

delOpt:
	num = mib_chain_total( MIB_DHCP_SERVER_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPOption_entity;
		if( !mib_chain_get( MIB_DHCP_SERVER_OPTION_TBL, i, (void*)p ))
			continue;
		if(p->usedFor!=eUsedFor_DHCPServer_ServingPool)
			continue;
		if(p->dhcpConSPInstNum==instnum){
			mib_chain_delete(MIB_DHCP_SERVER_OPTION_TBL,i);
			break;
		}
	}
	if(i<num)
		goto delOpt;
	return;

}

unsigned int getSPDHCPOptEntryNum(unsigned int usedFor, unsigned int instnum)
{
	unsigned int ret=0, i,num;
	MIB_CE_DHCP_OPTION_T *p,DHCPOPT_entity;

	num = mib_chain_total( MIB_DHCP_SERVER_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		p = &DHCPOPT_entity;
		if( !mib_chain_get( MIB_DHCP_SERVER_OPTION_TBL, i, (void*)p ))
			continue;
		if(p->usedFor==usedFor && p->dhcpConSPInstNum==instnum)
			ret++;
	}

	return ret;
}

int getSPDHCPRsvOptEntryByCode(unsigned int instnum, unsigned char optCode, MIB_CE_DHCP_OPTION_T *optEntry ,int *id)
{
	unsigned int ret=-1, i,num;

	if(!optEntry)	return ret;

	num = mib_chain_total( MIB_DHCP_SERVER_OPTION_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_DHCP_SERVER_OPTION_TBL, i, (void*)optEntry ))
			continue;
		if(optEntry->usedFor==eUsedFor_DHCPServer_ServingPool
			&& optEntry->dhcpConSPInstNum==instnum
			&& optEntry->tag==optCode)
		{
			*id = i;
			ret = 0;
			break;
		}
	}

	return ret;
}

void initSPDHCPOptEntry(DHCPS_SERVING_POOL_T *p)
{
	char origstr[GENERAL_LEN];
	char *origstrDomain=origstr;

	if(!p)
		return;

	memset( p, 0, sizeof( DHCPS_SERVING_POOL_T ) );
	p->enable = 1;
	p->poolorder = findMaxDHCPConSPOrder() + 1;
	strncpy(p->vendorclassmode,"Substring",MODE_LEN-1);
	p->vendorclassmode[MODE_LEN-1]=0;
	p->InstanceNum = findMaxDHCPConSPInsNum() +1;
	p->leasetime=86400;
	p->localserved = 1;//default: locallyserved=true;
	memset(p->chaddrmask,0xff,MAC_ADDR_LEN);//default to all 0xff
	mib_get(MIB_ADSL_LAN_DHCP_DOMAIN, (void *)origstrDomain);
	strncpy(p->domainname,origstrDomain,GENERAL_LEN-1);
	p->domainname[GENERAL_LEN-1]=0;
	mib_get(MIB_ADSL_LAN_DHCP_GATEWAY, (void *)p->iprouter);
#ifdef DHCPS_POOL_COMPLETE_IP
	mib_get(MIB_DHCP_SUBNET_MASK, (void *)p->subnetmask);
#else
	mib_get(MIB_ADSL_LAN_SUBNET, (void *)p->subnetmask);
#endif
}
/*ping_zhang:20090319 END*/
#endif
/*ping_zhang:20080919 END*/

MIB_CE_ATM_VC_T *getATMVCByInstNum( unsigned int devnum, unsigned int ipnum, unsigned int pppnum, MIB_CE_ATM_VC_T *p, unsigned int *chainid )
{
	unsigned int i,num;

	if( (p==NULL) || (chainid==NULL) ) return NULL;

	num = mib_chain_total( MIB_ATM_VC_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_ATM_VC_TBL, i, (void*)p ))
			continue;

		if( (p->ConDevInstNum==devnum) &&
		    (p->ConIPInstNum==ipnum) &&
		    (p->ConPPPInstNum==pppnum) )
		{
			*chainid=i;
			return p;
		}
	}

	return NULL;
}

#if defined(IP_QOS) || defined(NEW_IP_QOS_SUPPORT) || defined(CONFIG_USER_IP_QOS_3)
unsigned int findUnusedQueueInstNum(void)
{
	int i,entryNum;
	unsigned int num[MAX_QOS_QUEUE_NUM] = {0};
	MIB_CE_IP_QOS_QUEUE_T entry;

	entryNum = mib_chain_total(MIB_IP_QOS_QUEUE_TBL);
	for(i=0; i<entryNum; i++)
	{
		if( !mib_chain_get( MIB_IP_QOS_QUEUE_TBL, i, (void*)&entry ))
			continue;
		num[entry.QueueInstNum-1] = 1;
	}

	for(i=0; i<MAX_QOS_QUEUE_NUM; i++)
		{
		if(num[i]==0)
			return i+1;
		}
	return 0;
}

unsigned int getQoSQueueNum(void)
{
	unsigned int i,entryNum,num=0;
	MIB_CE_IP_QOS_QUEUE_T entry;

	entryNum = mib_chain_total(MIB_IP_QOS_QUEUE_TBL);
	for(i=0; i<entryNum; i++)
	{
		if( !mib_chain_get( MIB_IP_QOS_QUEUE_TBL, i, (void*)&entry ))
			continue;

		num++;
	}
	return num;
}
#endif

#ifdef _PRMT_TR143_
static const char gUDPEchoServerName[] = "/bin/udpechoserver";
static const char gUDPEchoServerPid[] = "/var/run/udpechoserver.pid";

void UDPEchoConfigSave(struct TR143_UDPEchoConfig *p)
{
	if(p)
	{
		unsigned char itftype;
		mib_get( TR143_UDPECHO_ENABLE, (void *)&p->Enable );
		mib_get( TR143_UDPECHO_SRCIP, (void *)p->SourceIPAddress );
		mib_get( TR143_UDPECHO_PORT, (void *)&p->UDPPort );
		mib_get( TR143_UDPECHO_PLUS, (void *)&p->EchoPlusEnabled );

		mib_get( TR143_UDPECHO_ITFTYPE, (void *)&itftype );
		if(itftype==ITF_WAN)
		{
			int total,i;
			MIB_CE_ATM_VC_T *pEntry, vc_entity;

			p->Interface[0]=0;
			total = mib_chain_total(MIB_ATM_VC_TBL);
			for( i=0; i<total; i++ )
			{
				pEntry = &vc_entity;
				if( !mib_chain_get(MIB_ATM_VC_TBL, i, (void*)pEntry ) )
					continue;
				if(pEntry->TR143UDPEchoItf)
				{
					ifGetName(pEntry->ifIndex, p->Interface, sizeof(p->Interface));
				}
			}
		}else if(itftype<ITF_END)
		{
			strcpy( p->Interface, strItf[itftype] );
			LANDEVNAME2BR0(p->Interface);
		}else
			p->Interface[0]=0;

	}
	return;
}

int UDPEchoConfigStart( struct TR143_UDPEchoConfig *p )
{
	if(!p) return -1;

	if( p->Enable )
	{
		char strPort[16], strAddr[32];
		char *argv[10];
		int  i;

		if(p->UDPPort==0)
		{
			fprintf( stderr, "UDPEchoConfigStart> error p->UDPPort=0\n" );
			return -1;
		}
		sprintf( strPort, "%u", p->UDPPort );
		va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", FW_ADD, (char *)FW_PREROUTING,
			 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
			(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);


		i=0;
		argv[i]=(char *)gUDPEchoServerName;
		i++;
		argv[i]="-port";
		i++;
		argv[i]=strPort;
		i++;
		if( strlen(p->Interface) > 0 )
		{
			argv[i]="-i";
			i++;
			argv[i]=p->Interface;
			i++;
		}
		if( p->SourceIPAddress[0]!=0 ||
			p->SourceIPAddress[1]!=0 ||
			p->SourceIPAddress[2]!=0 ||
			p->SourceIPAddress[3]!=0  )
		{
			struct in_addr *pSIP = (struct in_addr *)p->SourceIPAddress;
			argv[i]="-addr";
			i++;
			sprintf( strAddr, "%s", inet_ntoa( *pSIP ) );
			argv[i]=strAddr;
			i++;
		}
		if( p->EchoPlusEnabled )
		{
			argv[i]="-plus";
			i++;
		}

		argv[i]=NULL;
		do_cmd( gUDPEchoServerName, argv, 0 );

	}

	return 0;
}

int UDPEchoConfigStop( struct TR143_UDPEchoConfig *p )
{
	char strPort[16];
	int pid;
	int status;

	if(!p) return -1;

	sprintf( strPort, "%u", p->UDPPort );
	va_cmd(IPTABLES, 15, 1, ARG_T, "mangle", FW_DEL, (char *)FW_PREROUTING,
		 "!", (char *)ARG_I, (char *)LANIF, "-p", (char *)ARG_UDP,
		(char *)FW_DPORT, strPort, "-j", (char *)"MARK", "--set-mark", RMACC_MARK);


	pid = read_pid((char *)gUDPEchoServerPid);
	if (pid >= 1)
	{
		status = kill(pid, SIGTERM);
		if (status != 0)
		{
			printf("Could not kill UDPEchoServer's pid '%d'\n", pid);
			return -1;
		}
	}

	return 0;
}
#endif //_PRMT_TR143_

#endif //_CWMP_MIB_

int getDisplayWanName(MIB_CE_ATM_VC_T *pEntry, char* name)
{
	MEDIA_TYPE_T mType;

	if(pEntry==NULL || name==NULL)
		return 0;

	name[0] = '\0';
	mType = MEDIA_INDEX(pEntry->ifIndex);
	if (pEntry->cmode == CHANNEL_MODE_PPPOA) { // ppp0, ...
		snprintf(name, 6, "ppp%u", PPP_INDEX(pEntry->ifIndex));
	}
	else if (pEntry->cmode == CHANNEL_MODE_PPPOE) { // ppp0_vc0, ...
		if (mType == MEDIA_ATM)
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			snprintf(name, 16, "ppp%u_%s%u_%u", PPP_INDEX(pEntry->ifIndex), ALIASNAME_VC, VC_MAJOR_INDEX(pEntry->ifIndex), VC_MINOR_INDEX(pEntry->ifIndex));
#else
			snprintf(name, 16, "ppp%u_%s%u", PPP_INDEX(pEntry->ifIndex), ALIASNAME_VC, VC_INDEX(pEntry->ifIndex));
#endif
#ifdef CONFIG_PTMWAN
		else if (mType == MEDIA_PTM)
			snprintf(name, 16, "ppp%u_%s%u", PPP_INDEX(pEntry->ifIndex), ALIASNAME_MWPTM, PTM_INDEX(pEntry->ifIndex));
#endif /*CONFIG_PTMWAN*/
		else if (mType == MEDIA_ETH)
#ifdef CONFIG_RTL_MULTI_ETH_WAN
			snprintf(name, 16, "ppp%u_%s%u", PPP_INDEX(pEntry->ifIndex), ALIASNAME_MWNAS, ETH_INDEX(pEntry->ifIndex));
#else
			snprintf(name, 16, "ppp%u_%s%u", PPP_INDEX(pEntry->ifIndex), ALIASNAME_NAS, ETH_INDEX(pEntry->ifIndex));
#endif
#ifdef WLAN_WISP
		else if (mType == MEDIA_WLAN)
			snprintf(name, 16, "ppp%u_wlan%d-vxd", PPP_INDEX(pEntry->ifIndex), ETH_INDEX(pEntry->ifIndex));
#endif
		else
			return 0;
	}
	else { // vc0 ...
		if (mType == MEDIA_ATM)
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			snprintf(name, 16, "%s%u_%u", ALIASNAME_VC, VC_MAJOR_INDEX(pEntry->ifIndex), VC_MINOR_INDEX(pEntry->ifIndex));
#else
			snprintf(name, 16, "%s%u", ALIASNAME_VC, VC_INDEX(pEntry->ifIndex));
#endif
#ifdef CONFIG_PTMWAN
		else if (mType == MEDIA_PTM)
			snprintf(name, 16, "%s%u", ALIASNAME_MWPTM, PTM_INDEX(pEntry->ifIndex));
#endif /*CONFIG_PTMWAN*/
		else if (mType == MEDIA_ETH)
#ifdef CONFIG_RTL_MULTI_ETH_WAN
			snprintf(name, 16, "%s%u", ALIASNAME_MWNAS, ETH_INDEX(pEntry->ifIndex));
#else
			snprintf(name, 16, "%s%u", ALIASNAME_NAS, ETH_INDEX(pEntry->ifIndex));
#endif
#ifdef WLAN_WISP
		else if (mType == MEDIA_WLAN)
			snprintf(name, 16, "wlan%d-vxd", ETH_INDEX(pEntry->ifIndex));
#endif
		else
			return 0;
	}
	return 1;
}

//jim: to get wan MIB by index... this index is combined index for ppp or vc...
int getWanEntrybyindex(MIB_CE_ATM_VC_T *pEntry, unsigned int ifIndex)
{
	if(pEntry==NULL)
		return -1;
	int mibtotal,i,num=0,totalnum=0;
	MIB_CE_ATM_VC_T Entry;
	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<mibtotal;i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;
		if(Entry.ifIndex == ifIndex)
			break;
	}
	if(i==mibtotal)
		return -1;
	memcpy(pEntry, &Entry, sizeof(Entry));
	return 0;
}

unsigned int getWanIfMapbyMedia(MEDIA_TYPE_T mType)
{
	int mibtotal,i;
	MIB_CE_ATM_VC_T Entry;
	unsigned int ifMap; // high half for PPP bitmap, low half for vc bitmap

	ifMap=0;//init
	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<mibtotal;i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;
		if (Entry.cmode == CHANNEL_MODE_PPPOE)
			ifMap |= (1 << 16) << PPP_INDEX(Entry.ifIndex);	// PPP map
		if(MEDIA_INDEX(Entry.ifIndex) == mType)
			ifMap |= 1 << VC_INDEX(Entry.ifIndex);
	}

	return ifMap;
}

// Kaohj --- first entry
int getWanEntrybyMedia(MIB_CE_ATM_VC_T *pEntry, MEDIA_TYPE_T mType)
{
	if(pEntry==NULL)
		return -1;
	int mibtotal,i,num=0,totalnum=0;
	MIB_CE_ATM_VC_T Entry;
	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<mibtotal;i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;
		if(MEDIA_INDEX(Entry.ifIndex) == mType)
			break;
	}
	if(i==mibtotal)
		return -1;
	memcpy(pEntry, &Entry, sizeof(Entry));
	return i;
}

static const char IF_UP[] = "up";
static const char IF_DOWN[] = "down";
static const char IF_NA[] = "n/a";
#ifdef EMBED
static const char PROC_NET_ATM_BR[] = "/proc/net/atm/br2684";
#ifdef CONFIG_ATM_CLIP
static const char PROC_NET_ATM_CLIP[] = "/proc/net/atm/pvc";
#endif
#ifdef CONFIG_PPP
const char PPP_CONF[] = "/var/ppp/ppp.conf";
const char PPPD_FIFO[] = "/tmp/ppp_serv_fifo";
const char PPPOA_CONF[] = "/var/ppp/pppoa.conf";
const char PPPOE_CONF[] = "/var/ppp/pppoe.conf";
const char PPP_PID[] = "/var/run/spppd.pid";
const char SPPPD[] = "/bin/spppd";
#endif
#endif
#undef FILE_LOCK

void getServiceType(char *serviceTypeStr, int ctype)
{
#ifdef CONFIG_USER_RTK_WAN_CTYPE
		if (ctype == X_CT_SRV_TR069)
			strcpy(serviceTypeStr,"TR069");
		else if (ctype == X_CT_SRV_INTERNET)
			strcpy(serviceTypeStr,"INTERNET");
		else if (ctype == X_CT_SRV_OTHER)
#ifdef CONFIG_00R0
			sprintf(serviceTypeStr,"%s",multilang(LANG_OTHER));
#else
			strcpy(serviceTypeStr,"Other");
#endif
		else if (ctype == X_CT_SRV_VOICE)
			strcpy(serviceTypeStr,"VOICE");
		else if (ctype == X_CT_SRV_INTERNET+X_CT_SRV_TR069)
			strcpy(serviceTypeStr,"INTERNET_TR069");
		else if (ctype == X_CT_SRV_VOICE+X_CT_SRV_TR069)
			strcpy(serviceTypeStr,"VOICE_TR069");
		else if (ctype == X_CT_SRV_VOICE+X_CT_SRV_INTERNET)
			strcpy(serviceTypeStr,"VOICE_INTERNET");
		else if (ctype == X_CT_SRV_VOICE+X_CT_SRV_INTERNET+X_CT_SRV_TR069)
			strcpy(serviceTypeStr,"VOICE_INTERNET_TR069");
		else
			strcpy(serviceTypeStr,"None");
#else
		strcpy(serviceTypeStr,"None");
#endif
}

#ifndef CONFIG_00R0
int getWanStatus(struct wstatus_info *sEntry, int max)
{
	unsigned int data, data2;
	char	buff[256], tmp1[20], tmp2[20], tmp3[20], tmp4[20];
	char	*temp;
	int in_turn=0, vccount=0, ifcount=0;
	int linkState, dslState=0, ethState=0;
#ifdef CONFIG_USER_CMD_CLIENT_SIDE
	int ptmState=0;
#endif
	int i;
	FILE *fp;
#ifdef CONFIG_PPP
#if defined(EMBED)
	int spid;
#endif
#ifdef FILE_LOCK
	struct flock flpoe, flpoa;
	int fdpoe, fdpoa;
#endif
#endif
	Modem_LinkSpeed vLs;
	int entryNum;
	MIB_CE_ATM_VC_T tEntry;
	struct wstatus_info vcEntry[MAX_VC_NUM];

	memset(sEntry, 0, sizeof(struct wstatus_info)*max);
	memset(vcEntry, 0, sizeof(struct wstatus_info)*MAX_VC_NUM);
#if defined(EMBED) && defined(CONFIG_PPP)
	// get spppd pid
	spid = 0;
	if ((fp = fopen(PPP_PID, "r"))) {
		fscanf(fp, "%d\n", &spid);
		fclose(fp);
	}
	else
		printf("spppd pidfile not exists\n");

	if (spid) {
		struct data_to_pass_st msg;
		snprintf(msg.data, BUF_SIZE, "spppctl pppstatus %d", spid);
		TRACE(STA_SCRIPT, "%s\n", msg.data);
		write_to_pppd(&msg);
	}
#endif
	in_turn = 0;
#ifdef EMBED
#ifdef CONFIG_ATM_BR2684
//#ifdef CONFIG_RTL_MULTI_PVC_WAN
#if 1
	if( WAN_MODE & MODE_ATM )
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
				continue;

			if(MEDIA_INDEX(tEntry.ifIndex) != MEDIA_ATM)
				continue;
			//czpinging, skipped the entry with Disabled Admin status
			if(tEntry.enable==0)
				continue;

			vcEntry[vccount].ifIndex = tEntry.ifIndex;
			ifGetName(tEntry.ifIndex, vcEntry[vccount].ifname, IFNAMSIZ);
			ifGetName(PHY_INTF(tEntry.ifIndex), vcEntry[vccount].devname, IFNAMSIZ);
			vcEntry[vccount].tvpi = tEntry.vpi;
			vcEntry[vccount].tvci = tEntry.vci;

			switch(tEntry.encap) {
				case ENCAP_VCMUX:
					strcpy(vcEntry[vccount].encaps, "VCMUX");
					break;
				case ENCAP_LLC:
					strcpy(vcEntry[vccount].encaps, "LLC");
					break;
				default:
					break;
			}

			switch(tEntry.cmode) {
				case CHANNEL_MODE_IPOE:
					strcpy(vcEntry[vccount].protocol, "IPoE");
					break;
				case CHANNEL_MODE_BRIDGE:
					strcpy(vcEntry[vccount].protocol, "Bridged");
					break;
				case CHANNEL_MODE_PPPOE:
					strcpy(vcEntry[vccount].protocol, "PPPoE");
					break;
				case CHANNEL_MODE_PPPOA:
					strcpy(vcEntry[vccount].protocol, "PPPoA");
					break;
				case CHANNEL_MODE_RT1483:
					strcpy(vcEntry[vccount].protocol, "RT1483");
					break;
				case CHANNEL_MODE_RT1577:
					strcpy(vcEntry[vccount].protocol, "RT1577");
					break;
				case CHANNEL_MODE_6RD:
					strcpy(vcEntry[vccount].protocol, "6rd");
					break;
				default:
					break;
			}
			strcpy(vcEntry[vccount].vpivci, "---");
			vccount++;
		}
	}
#else
	if (!(fp=fopen(PROC_NET_ATM_BR, "r")))
		printf("%s not exists.\n", PROC_NET_ATM_BR);
	else {
		while ( fgets(buff, sizeof(buff), fp) != NULL ) {
			if (in_turn==0)
				if(sscanf(buff, "%*s%s", tmp1)!=1) {
					printf("Unsuported pvc configuration format\n");
					break;
				}
				else {
					vccount ++;
					tmp1[strlen(tmp1)-1]='\0';
					strcpy(vcEntry[vccount-1].ifname, tmp1);
					strcpy(vcEntry[vccount-1].devname, tmp1);
				}
			else
				if(sscanf(buff, "%*s%s%*s%s", tmp1, tmp2)!=2) {
					printf("Unsuported pvc configuration format\n");
					break;
				}
				else {
					sscanf(tmp1, "0.%u.%u:", &vcEntry[vccount-1].tvpi, &vcEntry[vccount-1].tvci);
					sscanf(tmp2, "%u,", &data);
					strcpy(vcEntry[vccount-1].protocol, "");
					if (data==1 || data == 4)
						strcpy(vcEntry[vccount-1].encaps, "LLC");
					else if (data==0 || data==3)
						strcpy(vcEntry[vccount-1].encaps, "VCMUX");
					if (data==3 || data==4)
						strcpy(vcEntry[vccount-1].protocol, "rt1483");
					strcpy(vcEntry[vccount-1].vpivci, "---");
				}
			in_turn ^= 0x01;
		}
		fclose(fp);
	}
#endif
#endif
#ifdef CONFIG_ATM_CLIP
	if (!(fp=fopen(PROC_NET_ATM_CLIP, "r")))
		printf("%s not exists.\n", PROC_NET_ATM_CLIP);
	else {
		fgets(buff, sizeof(buff), fp);
		while ( fgets(buff, sizeof(buff), fp) != NULL ) {
			char *p = strstr(buff, "CLIP");
			if (p != NULL) {
				if (sscanf(buff, "%*d%u%u%*d%*d%*s%*d%*s%*s%s%s", &data, &data2, tmp1, tmp2) != 4) {
					printf("Unsuported 1577 configuration format\n");
					break;
				}
				else {
					vccount ++;
					sscanf(tmp1, "Itf:%s", tmp3);
					strcpy(vcEntry[vccount-1].ifname, strtok(tmp3, ","));
					strcpy(vcEntry[vccount-1].devname, vcEntry[vccount-1].ifname);
					sscanf(tmp2, "Encap:%s", tmp4);
					strcpy(vcEntry[vccount-1].encaps, strtok(tmp4, "/"));
					strcpy(vcEntry[vccount-1].protocol, "rt1577");
					vcEntry[vccount-1].tvpi = data;
					vcEntry[vccount-1].tvci = data2;
					strcpy(vcEntry[vccount-1].vpivci, "---");
				}
			}
		}
		fclose(fp);
	}
#endif


#ifdef CONFIG_PTMWAN
	if( WAN_MODE & MODE_PTM )
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
				continue;

			if(MEDIA_INDEX(tEntry.ifIndex) != MEDIA_PTM)
				continue;

			//czpinging, skipped the entry with Disabled Admin status
			if(tEntry.enable==0)
				continue;

			vcEntry[vccount].ifIndex = tEntry.ifIndex;
			ifGetName(tEntry.ifIndex, vcEntry[vccount].ifname, IFNAMSIZ);
			ifGetName(PHY_INTF(tEntry.ifIndex), vcEntry[vccount].devname, IFNAMSIZ);
			strcpy(vcEntry[vccount].encaps, "---");
			switch(tEntry.cmode) {
				case CHANNEL_MODE_IPOE:
					strcpy(vcEntry[vccount].protocol, "IPoE");
					break;
				case CHANNEL_MODE_BRIDGE:
					strcpy(vcEntry[vccount].protocol, "Bridged");
					break;
				case CHANNEL_MODE_PPPOE:
					strcpy(vcEntry[vccount].protocol, "PPPoE");
					break;
				case CHANNEL_MODE_6RD:
					strcpy(vcEntry[vccount].protocol, "6rd");
					break;
				default:
					break;
			}
			strcpy(vcEntry[vccount].vpivci, "---");
			vccount++;
		}
	}
#endif // CONFIG_PTMWAN


#ifdef CONFIG_ETHWAN
	if( WAN_MODE & MODE_Ethernet )
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
				continue;

			if(MEDIA_INDEX(tEntry.ifIndex) != MEDIA_ETH)
				continue;

			//czpinging, skipped the entry with Disabled Admin status
			if(tEntry.enable==0)
				continue;

			vcEntry[vccount].ifIndex = tEntry.ifIndex;
			ifGetName(tEntry.ifIndex, vcEntry[vccount].ifname, IFNAMSIZ);
			ifGetName(PHY_INTF(tEntry.ifIndex), vcEntry[vccount].devname, IFNAMSIZ);
			strcpy(vcEntry[vccount].encaps, "---");
			switch(tEntry.cmode) {
				case CHANNEL_MODE_IPOE:
					strcpy(vcEntry[vccount].protocol, "IPoE");
					break;
				case CHANNEL_MODE_BRIDGE:
					strcpy(vcEntry[vccount].protocol, "Bridged");
					break;
				case CHANNEL_MODE_PPPOE:
					strcpy(vcEntry[vccount].protocol, "PPPoE");
					break;
				case CHANNEL_MODE_6RD:
					strcpy(vcEntry[vccount].protocol, "6rd");
					break;
				default:
					break;
			}
			strcpy(vcEntry[vccount].vpivci, "---");
			vccount++;
		}
	}
#endif // CONFIG_ETHWAN

#ifdef WLAN_WISP
	if( WAN_MODE & MODE_Wlan )
	{
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i=0; i<entryNum; i++)
		{
			if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&tEntry))
				continue;

			if(MEDIA_INDEX(tEntry.ifIndex) != MEDIA_WLAN)
				continue;

			//czpinging, skipped the entry with Disabled Admin status
			if(tEntry.enable==0)
				continue;

			vcEntry[vccount].ifIndex = tEntry.ifIndex;
			ifGetName(tEntry.ifIndex, vcEntry[vccount].ifname, IFNAMSIZ);
			ifGetName(PHY_INTF(tEntry.ifIndex), vcEntry[vccount].devname, IFNAMSIZ);
			strcpy(vcEntry[vccount].encaps, "---");
			switch(tEntry.cmode) {
				case CHANNEL_MODE_IPOE:
					strcpy(vcEntry[vccount].protocol, "IPoE");
					break;
				case CHANNEL_MODE_BRIDGE:
					strcpy(vcEntry[vccount].protocol, "Bridged");
					break;
				case CHANNEL_MODE_PPPOE:
					strcpy(vcEntry[vccount].protocol, "PPPoE");
					break;
				default:
					break;
			}
			strcpy(vcEntry[vccount].vpivci, "---");
			vccount++;
		}
	}
#endif

#ifdef CONFIG_PPP
#ifdef FILE_LOCK
	// file locking
	fdpoe = open(PPPOE_CONF, O_RDWR);
	if (fdpoe != -1) {
		flpoe.l_type = F_RDLCK;
		flpoe.l_whence = SEEK_SET;
		flpoe.l_start = 0;
		flpoe.l_len = 0;
		flpoe.l_pid = getpid();
		if (fcntl(fdpoe, F_SETLKW, &flpoe) == -1)
			printf("pppoe read lock failed\n");
	}
	
	fdpoa = open(PPPOA_CONF, O_RDWR);
	if (fdpoa != -1) {
		flpoa.l_type = F_RDLCK;
		flpoa.l_whence = SEEK_SET;
		flpoa.l_start = 0;
		flpoa.l_len = 0;
		flpoa.l_pid = getpid();
		if (fcntl(fdpoa, F_SETLKW, &flpoa) == -1)
			printf("pppoa read lock failed\n");
	}
#endif

#ifdef CONFIG_DEV_xDSL
	if (!(fp=fopen(PPPOA_CONF, "r")))
		printf("%s not exists.\n", PPPOA_CONF);
	else {
		fgets(buff, sizeof(buff), fp);
		while ( fgets(buff, sizeof(buff), fp) != NULL )
			if (sscanf(buff, "%s%u%u%*s%s%*s%*d%*d%*d%s%s", tmp1, &data, &data2, tmp2, tmp3, tmp4) != 6) {
				printf("Unsuported pppoa configuration format\n");
				break;
			}
			else {
				ifcount++;
				// ifIndex --- ppp index(no vc index)
				sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, tmp1[3]-'0', DUMMY_VC_INDEX);
				strcpy(sEntry[ifcount-1].ifname, tmp1);
				strcpy(sEntry[ifcount-1].encaps, tmp2);
				strcpy(sEntry[ifcount-1].protocol, "PPPoA");
				sEntry[ifcount-1].tvpi = data;
				sEntry[ifcount-1].tvci = data2;
				sprintf(sEntry[ifcount-1].vpivci, "%u/%u", sEntry[ifcount-1].tvpi, sEntry[ifcount-1].tvci);
				strcpy(sEntry[ifcount-1].uptime, tmp3);
				strcpy(sEntry[ifcount-1].totaluptime, tmp4);
			}
		fclose(fp);
	}
#endif

	if (!(fp=fopen(PPPOE_CONF, "r")))
		printf("%s not exists.\n", PPPOE_CONF);
	else {
		fgets(buff, sizeof(buff), fp);
		while ( fgets(buff, sizeof(buff), fp) != NULL )
			if(sscanf(buff, "%s%s%*s%*s%*s%s%s", tmp1, tmp2, tmp3, tmp4) != 4) {
				printf("Unsuported pppoe configuration format\n");
				break;
			}
			else
				for (i=0; i<vccount; i++)
//#ifdef CONFIG_RTL_MULTI_PVC_WAN
					if (strcmp(vcEntry[i].ifname, tmp1) == 0) // based on pppX
//#else
//					if (strcmp(vcEntry[i].devname, tmp2) == 0)
//#endif
					{
						ifcount++;
						// ifIndex --- ppp index + vc index
						if (!strncmp(vcEntry[i].devname, ALIASNAME_VC, strlen(ALIASNAME_VC)))
						{
#ifdef CONFIG_RTL_MULTI_PVC_WAN
							sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, tmp1[strlen(ALIASNAME_PPP)]-'0', ((((vcEntry[i].devname[strlen(ALIASNAME_VC)]-'0') << 4) & 0xf0) | ((vcEntry[i].devname[strlen(ALIASNAME_VC)+2]-'0') & 0x0f)) );
#else
							sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, tmp1[strlen(ALIASNAME_PPP)]-'0', vcEntry[i].devname[strlen(ALIASNAME_VC)]-'0');
#endif
							sEntry[ifcount-1].tvpi = vcEntry[i].tvpi;
							sEntry[ifcount-1].tvci = vcEntry[i].tvci;
							sprintf(sEntry[ifcount-1].vpivci, "%u/%u", sEntry[ifcount-1].tvpi, sEntry[ifcount-1].tvci);
							//printf("***** sEntry[ifcount-1].ifIndex=0x%x\n", sEntry[ifcount-1].ifIndex);
						}
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN) || defined (WLAN_WISP)
						else {
							sEntry[ifcount-1].ifIndex = vcEntry[i].ifIndex;
							strcpy(sEntry[ifcount-1].vpivci, "---");
						}
#endif
						strcpy(sEntry[ifcount-1].ifname, tmp1);
#ifdef CONFIG_RTL_MULTI_PVC_WAN
						strcpy(sEntry[ifcount-1].devname, tmp2);
#else
						strcpy(sEntry[ifcount-1].devname, vcEntry[i].devname);
#endif
						strcpy(sEntry[ifcount-1].encaps, vcEntry[i].encaps);
						strcpy(sEntry[ifcount-1].protocol, "PPPoE");
						strcpy(sEntry[ifcount-1].uptime, tmp3);
						strcpy(sEntry[ifcount-1].totaluptime, tmp4);
						break;
					}
		fclose(fp);
	}
#ifdef FILE_LOCK
	// file unlocking
	if (fdpoe != -1) {
		flpoe.l_type = F_UNLCK;
		if (fcntl(fdpoe, F_SETLK, &flpoe) == -1)
			printf("pppoe read unlock failed\n");
		close(fdpoe);
	}
	if (fdpoa != -1) {
		flpoa.l_type = F_UNLCK;
		if (fcntl(fdpoa, F_SETLK, &flpoa) == -1)
			printf("pppoa read unlock failed\n");
		close(fdpoa);
	}
#endif
#endif

	for (i=0; i<vccount; i++) {
		int j, vcfound=0;
		for (j=0; j<ifcount; j++) {
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			if (strcmp(vcEntry[i].ifname, sEntry[j].ifname) == 0) {	// PPPoE-used device
#else
			if (strcmp(vcEntry[i].devname, sEntry[j].devname) == 0) {	// PPPoE-used device
#endif
				vcfound = 1;
				break;
			}
		}
		if (!vcfound) {	// VC not used for PPPoA/PPPoE, add to list
			ifcount++;
			// ifIndex --- vc index (no ppp index)
			if (!strncmp(vcEntry[i].devname, ALIASNAME_VC, strlen(ALIASNAME_VC))) {
#ifdef CONFIG_RTL_MULTI_PVC_WAN
				sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, DUMMY_PPP_INDEX, ((((vcEntry[i].devname[strlen(ALIASNAME_VC)]-'0') << 4) & 0xf0) | ((vcEntry[i].devname[strlen(ALIASNAME_VC)+2]-'0') & 0x0f)) );
#else
				sEntry[ifcount-1].ifIndex = TO_IFINDEX(MEDIA_ATM, DUMMY_PPP_INDEX, vcEntry[i].ifname[strlen(ALIASNAME_VC)]-'0');
#endif
				sEntry[ifcount-1].tvpi = vcEntry[i].tvpi;
				sEntry[ifcount-1].tvci = vcEntry[i].tvci;
				sprintf(sEntry[ifcount-1].vpivci, "%u/%u", sEntry[ifcount-1].tvpi, sEntry[ifcount-1].tvci);
			}
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN) || defined (WLAN_WISP)
			else {
				sEntry[ifcount-1].ifIndex = vcEntry[i].ifIndex;
				strcpy(sEntry[ifcount-1].vpivci, "---");
			}
#endif
			strcpy(sEntry[ifcount-1].ifname, vcEntry[i].ifname);
			strcpy(sEntry[ifcount-1].devname, vcEntry[i].devname);
			strcpy(sEntry[ifcount-1].encaps, vcEntry[i].encaps);
			strcpy(sEntry[ifcount-1].protocol, vcEntry[i].protocol);
		}
	}

#endif

#ifdef CONFIG_DEV_xDSL
#ifndef CONFIG_USER_CMD_CLIENT_SIDE
	// check for xDSL link
	if (!adsl_drv_get(RLCM_GET_LINK_SPEED, (void *)&vLs, RLCM_GET_LINK_SPEED_SIZE) || vLs.upstreamRate == 0)
		dslState = 0;
	else
		dslState = 1;
#else
	if(get_net_link_status("atm0")==1)
		dslState = 1;
	else
		dslState = 0;

	if(get_net_link_status("ptm0")==1)
		ptmState = 1;
	else
		ptmState = 0;
#endif
#endif
#if defined(CONFIG_ETHWAN) || defined(CONFIG_PTMWAN)
	// todo
	ethState = 1;
#endif

	if (ifcount > max)
		printf("WARNNING! status list overflow(%d).\n", ifcount);


	for (i=0; i<ifcount; i++) {
		struct in_addr inAddr;
		int flags;
		int totalNum, k;
		MIB_CE_ATM_VC_T entry;
		MEDIA_TYPE_T mType;

#ifdef EMBED
		// Kaohj --- interface name to be displayed
		totalNum = mib_chain_total(MIB_ATM_VC_TBL);

		for(k=0; k<totalNum; k++)
		{
			mib_chain_get(MIB_ATM_VC_TBL, k, (void *)&entry);

			if (sEntry[i].ifIndex == entry.ifIndex) {
				getDisplayWanName(&entry, sEntry[i].ifDisplayName);
				sEntry[i].cmode = entry.cmode;
#ifdef CONFIG_IPV6
				sEntry[i].ipver = entry.IpProtocol;
#endif
				sEntry[i].recordNum = k;
#if defined(CONFIG_LUNA)
				sEntry[i].vid = entry.vid;
				getServiceType(sEntry[i].serviceType,entry.applicationtype);				
#endif
				break;
			}

		}
		if (getInAddr( sEntry[i].ifname, IP_ADDR, (void *)&inAddr) == 1)
		{
			temp = inet_ntoa(inAddr);
			if (getInFlags( sEntry[i].ifname, &flags) == 1)
				if ((strcmp(temp, "10.0.0.1") == 0) && flags & IFF_POINTOPOINT)	// IP Passthrough or IP unnumbered
					strcpy(sEntry[i].ipAddr, STR_UNNUMBERED);
				else if (strcmp(temp, "64.64.64.64") == 0)
					strcpy(sEntry[i].ipAddr, "");
				else
					strcpy(sEntry[i].ipAddr, temp);
		}
		else
#endif
			strcpy(sEntry[i].ipAddr, "");

#ifdef EMBED
		if (getInAddr( sEntry[i].ifname, DST_IP_ADDR, (void *)&inAddr) == 1)
		{
			temp = inet_ntoa(inAddr);
			if (strcmp(temp, "10.0.0.2") == 0)
				strcpy(sEntry[i].remoteIp, STR_UNNUMBERED);
			else if (strcmp(temp, "64.64.64.64") == 0)
				strcpy(sEntry[i].remoteIp, "");
			else
				strcpy(sEntry[i].remoteIp, temp);
			if (getInFlags( sEntry[i].ifname, &flags) == 1)
				if (flags & IFF_BROADCAST) {
					unsigned char value[32];
					snprintf(value, 32, "%s.%s", (char *)MER_GWINFO, sEntry[i].ifname);
					if (fp = fopen(value, "r")) {
						fscanf(fp, "%s\n", sEntry[i].remoteIp);
						//strcpy(sEntry[i].protocol, "mer1483");
						fclose(fp);
					}
					else
						strcpy(sEntry[i].remoteIp, "");
				}
		}
		else
#endif
			strcpy(sEntry[i].remoteIp, "");

		if (!strcmp(sEntry[i].protocol, ""))
		{
			//get channel mode
			switch(sEntry[i].cmode) {
			case CHANNEL_MODE_IPOE:
				strcpy(sEntry[i].protocol, "mer1483");
				break;
			case CHANNEL_MODE_BRIDGE:
				strcpy(sEntry[i].protocol, "br1483");
				break;
			case CHANNEL_MODE_RT1483:
				strcpy(sEntry[i].protocol, "rt1483");
				break;
			case CHANNEL_MODE_6RD:
				strcpy(sEntry[i].protocol, "6rd");
				break;
			default:
				break;
			}
		}

		mType = MEDIA_INDEX(sEntry[i].ifIndex);
		if (mType == MEDIA_ATM)
			linkState = dslState;
		#ifdef CONFIG_PTMWAN
		else if (mType == MEDIA_PTM)
#ifndef CONFIG_USER_CMD_CLIENT_SIDE
			linkState = dslState && ethState;//???
#else
			linkState = ptmState;
#endif
		#endif /*CONFIG_PTMWAN*/
		else if (mType == MEDIA_ETH)
			linkState = ethState;
#ifdef WLAN_WISP
		else if (mType == MEDIA_WLAN){
			char wisp_name[16];
			getWispWanName(wisp_name, ETH_INDEX(sEntry[i].ifIndex));
			linkState = get_net_link_status(wisp_name);
		}
#endif
		else
			linkState = 0;
		sEntry[i].link_state = linkState;
		// set status flag
		sEntry[i].strStatus = (char *)IF_DOWN;
		sEntry[i].strStatus_IPv6 = (char *)IF_DOWN;
		sEntry[i].itf_state = 0;
		if (getInFlags( sEntry[i].ifname, &flags) == 1)
		{
			if (flags & IFF_UP) {
				if (linkState) {
					if (sEntry[i].cmode == CHANNEL_MODE_BRIDGE) {
						sEntry[i].strStatus = (char *)IF_UP;
						sEntry[i].strStatus_IPv6 = (char *)IF_UP;
						sEntry[i].itf_state = 1;
					} else {
						if ((sEntry[i].ipver | IPVER_IPV4) &&
								getInAddr(sEntry[i].ifname, IP_ADDR, (void *)&inAddr) == 1) {
							temp = inet_ntoa(inAddr);
							if (strcmp(temp, "64.64.64.64")) {
								sEntry[i].strStatus = (char *)IF_UP;
								sEntry[i].itf_state = 1;
							}
						}

						if (sEntry[i].ipver | IPVER_IPV6) {
							/* always has IPv6 link-local addresses */
							sEntry[i].strStatus_IPv6 = (char *)IF_UP;
 						}
					}
				}
			}
		} else {
			sEntry[i].strStatus = (char *)IF_NA;
			sEntry[i].strStatus_IPv6 = (char *)IF_NA;
			sEntry[i].itf_state = -1;
		}

		if (sEntry[i].cmode == CHANNEL_MODE_PPPOE || sEntry[i].cmode == CHANNEL_MODE_PPPOA) {
			if (sEntry[i].itf_state <= 0) {
				sEntry[i].ipAddr[0] = '\0';
				sEntry[i].remoteIp[0] = '\0';
			}
			if (entry.pppCtype == CONNECT_ON_DEMAND && entry.pppIdleTime != 0)
				sEntry[i].pppDoD = 1;
			else
				sEntry[i].pppDoD = 0;
		}

	}
	return ifcount;
}
#endif //end of CONFIG_00R0

int isValidMedia(unsigned int ifIndex)
{
	MEDIA_TYPE_T mType;

	mType = MEDIA_INDEX(ifIndex);
	if (1
	#ifdef CONFIG_DEV_xDSL
		&& mType!=MEDIA_ATM
	#endif
	#ifdef CONFIG_PTMWAN
		&& mType!=MEDIA_PTM
	#endif /*CONFIG_PTMWAN*/
	#ifdef CONFIG_ETHWAN
		&& mType!=MEDIA_ETH
	#endif
	#ifdef WLAN_WISP
		&& mType!=MEDIA_WLAN
	#endif
	)
		return 0;
	return 1;
}

// Kaohj -- specific for pvc channel
// map: bit map of used interface, ppp index (0~15) is mapped into high 16 bits,
// while vc index (0~15) is mapped into low 16 bits.
// while major vc index (8 ~ 15) is mapped into hight 8 bits of vc index.
// while minor vc index (0 ~ 7) is mapped into low 8 bits of vc index.
// return: interface index, byte1 for PPP index and byte0 for vc index.
//		0xefff(NA_PPP): PPP not available
//		0xffff(NA_VC) : vc not available
unsigned int if_find_index(int cmode, unsigned int map)
{
	int i;
	unsigned int index, vc_idx, ppp_idx;
#ifdef CONFIG_RTL_MULTI_PVC_WAN
	int j;
#endif

	// find the first available vc index (mpoa interface)
	i = 0;
#ifndef CONFIG_RTL_MULTI_PVC_WAN
	for (i=0; i<MAX_VC_NUM; i++)
	{
		if (!((map>>i) & 1))
			break;
	}
#else
	for (i=0; i<MAX_VC_NUM; i++)
	{
		if (!(((map>>8)>>i) & 1))
			break;
	}

	j=0;
	for (j=0; j<MAX_VC_VIRTUAL_NUM; j++)
	{
		if (!((map>>j) & 1))
			break;
	}
#endif

#ifndef CONFIG_RTL_MULTI_PVC_WAN
	if (i != MAX_VC_NUM)
		vc_idx = i;
	else
		return NA_VC;
#else
	// Mason Yu
	//printf("if_find_index: major=%d, minor=%d\n", i, j);
	if (i != MAX_VC_NUM && j != MAX_VC_VIRTUAL_NUM) {
		vc_idx = (((i << 4) & 0xf0) | (j & 0x0f));
	}
	else
		return NA_VC;
#endif

	if (cmode == CHANNEL_MODE_PPPOE || cmode == CHANNEL_MODE_PPPOA)
	{
		// find an available PPP index
		map >>= 16;
		i = 0;
		while (map & 1)
		{
			map >>= 1;
			i++;
		}
		ppp_idx = i;
		if (ppp_idx<=(MAX_PPP_NUM-1))
			index = TO_IFINDEX(MEDIA_ATM, ppp_idx, vc_idx);
		else
			return NA_PPP;

		if (cmode == CHANNEL_MODE_PPPOA)
			index = TO_IFINDEX(MEDIA_ATM, ppp_idx, DUMMY_VC_INDEX);
	}
	else
	{
		// don't care the PPP index
		index = TO_IFINDEX(MEDIA_ATM, DUMMY_PPP_INDEX, vc_idx);
	}
	return index;
}

#ifdef CONFIG_ETHWAN
int init_ethwan_config(MIB_CE_ATM_VC_T *pEntry)
{
	if (!pEntry)
		return 0;
	memset(pEntry, 0, sizeof(MIB_CE_ATM_VC_T));
	pEntry->ifIndex = TO_IFINDEX(MEDIA_ETH, DUMMY_PPP_INDEX, 0);
	pEntry->cmode = CHANNEL_MODE_BRIDGE;
	pEntry->enable = 1;
	return 1;
}
#endif
#ifdef CONFIG_PTMWAN
int init_ptmwan_config(MIB_CE_ATM_VC_T *pEntry)
{
	if (!pEntry)
		return 0;
	memset(pEntry, 0, sizeof(MIB_CE_ATM_VC_T));
	pEntry->ifIndex = TO_IFINDEX(MEDIA_PTM, DUMMY_PPP_INDEX, 0);
	pEntry->cmode = CHANNEL_MODE_BRIDGE;
	pEntry->enable = 1;
	return 1;
}
#endif /*CONFIG_PTMWAN*/

//star: to get ppp index for wanname
static int getpppindex(MIB_CE_ATM_VC_T * pEntry)
{
	int ret = -1;
	int mibtotal, i, num = 0, totalnum = 0;
	unsigned int pppindex, tmpindex;
	MIB_CE_ATM_VC_T Entry;

	if (pEntry->cmode != CHANNEL_MODE_PPPOE && pEntry->cmode != CHANNEL_MODE_PPPOA)
		return ret;

	pppindex = PPP_INDEX(pEntry->ifIndex);
	if (pppindex == DUMMY_PPP_INDEX)
		return ret;

	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < mibtotal; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			continue;
		if (Entry.cmode != CHANNEL_MODE_PPPOE && Entry.cmode != CHANNEL_MODE_PPPOA)
			continue;
		tmpindex = PPP_INDEX(Entry.ifIndex);
		if (tmpindex == DUMMY_PPP_INDEX)
			continue;
		if (Entry.vpi == pEntry->vpi && Entry.vci == pEntry->vci) {
			totalnum++;
			if (tmpindex < pppindex)
				num++;
		}
	}

	if (totalnum > 1)
		ret = num;

	return ret;

}

int setWanName(char *str, int applicationtype)
{
#ifdef CONFIG_USER_RTK_WAN_CTYPE
	str[0] = '\0';
	if (applicationtype | X_CT_SRV_TR069)
		strcat(str, "TR069_");

	if (applicationtype | X_CT_SRV_INTERNET)
		strcat(str, "Internet_");

	if (applicationtype | X_CT_SRV_OTHER)
		strcat(str, "Other_");

	if (applicationtype | X_CT_SRV_VOICE)
		strcat(str, "Voice_");
#else
	strcpy(str, "Internet_");
#endif
}

int generateWanName(MIB_CE_ATM_VC_T * entry, char *wanname)
{
	char vpistr[6];
	char vcistr[6];

	if (entry == NULL || wanname == NULL)
		return -1;
	memset(vpistr, 0, sizeof(vpistr));
	memset(vcistr, 0, sizeof(vcistr));
	setWanName(wanname, entry->applicationtype);
#ifdef CONFIG_USER_RTK_WAN_CTYPE
	wanname[0] = '\0';
	if (entry->applicationtype & X_CT_SRV_TR069)
		strcat(wanname, "TR069_");

	if (entry->applicationtype & X_CT_SRV_INTERNET)
		strcat(wanname, "Internet_");

	if (entry->applicationtype & X_CT_SRV_OTHER)
		strcat(wanname, "Other_");

	if (entry->applicationtype & X_CT_SRV_VOICE)
		strcat(wanname, "Voice_");
#else
	strcpy(wanname, "Internet_");
#endif
	if (entry->cmode == CHANNEL_MODE_BRIDGE)
		strcat(wanname, "B_");
	else
		strcat(wanname, "R_");
	sprintf(vpistr, "%d", entry->vpi);
	sprintf(vcistr, "%d", entry->vci);
	strcat(wanname, vpistr);
	strcat(wanname, "_");
	strcat(wanname, vcistr);
	//star: for multi-ppp in one pvc
	if (entry->cmode == CHANNEL_MODE_PPPOE || entry->cmode == CHANNEL_MODE_PPPOA) {
		char pppindex[6];
		int intindex;
		intindex = getpppindex(entry);
		if (intindex != -1) {
			snprintf(pppindex, 6, "%u", intindex);
			strcat(wanname, "_");
			strcat(wanname, pppindex);
		}
	}

	return 0;
}

int getWanName(MIB_CE_ATM_VC_T * pEntry, char *name)
{
	if (pEntry == NULL || name == NULL)
		return 0;
#ifdef _CWMP_MIB_
	if (*(pEntry->WanName))
		strcpy(name, pEntry->WanName);
	else
#endif
	{			//if not set by ACS. then generate automaticly.
		generateWanName(pEntry, name);
	}
	return 1;
}

#include <linux/atmdev.h>

struct arg{
	unsigned char cmd;
	unsigned int cmd2;
	unsigned int cmd3;
	unsigned int cmd4;
}pass_arg;

#define DEFDATALEN	56
#define PINGCOUNT 3
#define PINGINTERVAL	1	/* second */
#define MAXWAIT		5

static struct sockaddr_in pingaddr;
static int pingsock = -1;
static long ntransmitted = 0, nreceived = 0, nrepeats = 0;
static int myid = 0;
static int finished = 0;

int create_icmp_socket(void)
{
	struct protoent *proto;
	int sock;

	proto = getprotobyname("icmp");
	/* if getprotobyname failed, just silently force
	 * proto->p_proto to have the correct value for "icmp" */
	if ((sock = socket(AF_INET, SOCK_RAW,
			(proto ? proto->p_proto : 1))) < 0) {        /* 1 == ICMP */
		printf("cannot create raw socket\n");
	}

	return sock;
}

int in_cksum(unsigned short *buf, int sz)
{
	int nleft = sz;
	int sum = 0;
	unsigned short *w = buf;
	unsigned short ans = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		*(unsigned char *) (&ans) = *(unsigned char *) w;
		sum += ans;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	ans = ~sum;
	return (ans);
}

static void pingfinal()
{
	finished = 1;
}

static void sendping()
{
	struct icmp *pkt;
	int c;
	char packet[DEFDATALEN + 8];

	pkt = (struct icmp *) packet;
	pkt->icmp_type = ICMP_ECHO;
	pkt->icmp_code = 0;
	pkt->icmp_cksum = 0;
	pkt->icmp_seq = ntransmitted++;
	pkt->icmp_id = myid;
	pkt->icmp_cksum = in_cksum((unsigned short *) pkt, sizeof(packet));

	c = sendto(pingsock, packet, sizeof(packet), 0,
			   (struct sockaddr *) &pingaddr, sizeof(struct sockaddr_in));

	if (c < 0 || c != sizeof(packet)) {
		ntransmitted--;
		finished = 1;
		printf("sock: sendto fail !");
		return;
	}

	signal(SIGALRM, sendping);
	if (ntransmitted < PINGCOUNT) {	/* schedule next in 1s */
		alarm(PINGINTERVAL);
	} else {	/* done, wait for the last ping to come back */
		signal(SIGALRM, pingfinal);
		alarm(MAXWAIT);
	}
}

int utilping(char *str)
{
//	char *submitUrl;
	char tmpBuf[100];
	int c;
	struct hostent *h;
	struct icmp *pkt;
	struct iphdr *iphdr;
	char packet[DEFDATALEN + 8];
	int rcvdseq, ret=0;
	fd_set rset;
	struct timeval tv;

	if (str[0]) {
		if ((pingsock = create_icmp_socket()) < 0) {
			perror("socket");
			snprintf(tmpBuf, 100, "ping: socket create error");
			goto setErr_ping;
		}

		memset(&pingaddr, 0, sizeof(struct sockaddr_in));
		pingaddr.sin_family = AF_INET;

		if ((h = gethostbyname(str)) == NULL) {
			//herror("ping: ");
			//snprintf(tmpBuf, 100, "ping: %s: %s", str, hstrerror(h_errno));
			goto setErr_ping;
		}

		if (h->h_addrtype != AF_INET) {
			//strcpy(tmpBuf, "unknown address type; only AF_INET is currently supported.");
			goto setErr_ping;
		}

		memcpy(&pingaddr.sin_addr, h->h_addr, sizeof(pingaddr.sin_addr));

		printf("PING %s (%s): %d data bytes\n",
		   h->h_name,inet_ntoa(*(struct in_addr *) &pingaddr.sin_addr.s_addr),DEFDATALEN);

		myid = getpid() & 0xFFFF;
		ntransmitted = nreceived = nrepeats = 0;
		finished = 0;
		rcvdseq=ntransmitted-1;
		FD_ZERO(&rset);
		FD_SET(pingsock, &rset);
		/* start the ping's going ... */
		sendping();

		/* listen for replies */
		while (1) {
			struct sockaddr_in from;
			socklen_t fromlen = (socklen_t) sizeof(from);
			int c, hlen, dupflag;

			if (finished)
				break;

			tv.tv_sec = 1;
			tv.tv_usec = 0;

			if (select(pingsock+1, &rset, NULL, NULL, &tv) > 0) {
				if ((c = recvfrom(pingsock, packet, sizeof(packet), 0,
								  (struct sockaddr *) &from, &fromlen)) < 0) {

					printf("sock: recvfrom fail !");
					continue;
				}
			}
			else // timeout or error
				continue;

			if (c < DEFDATALEN+ICMP_MINLEN)
				continue;

			iphdr = (struct iphdr *) packet;
			hlen = iphdr->ihl << 2;
			pkt = (struct icmp *) (packet + hlen);	/* skip ip hdr */
			if (pkt->icmp_id != myid) {
//				printf("not myid\n");
				continue;
			}
			if (pkt->icmp_type == ICMP_ECHOREPLY) {
				++nreceived;
				if (pkt->icmp_seq == rcvdseq) {
					// duplicate
					++nrepeats;
					--nreceived;
					dupflag = 1;
				} else {
					rcvdseq = pkt->icmp_seq;
					dupflag = 0;
					if (nreceived < PINGCOUNT)
					// reply received, send another immediately
						sendping();
				}
				printf("%d bytes from %s: icmp_seq=%u", c,
					   inet_ntoa(*(struct in_addr *) &from.sin_addr.s_addr),
					   pkt->icmp_seq);
				if (dupflag) {
					printf(" (DUP!)");
				}
				printf("\n");
			}
			if (nreceived >= PINGCOUNT) {
				ret = 1;
				break;
			}
		}
		FD_CLR(pingsock, &rset);
		close(pingsock);
		pingsock = -1;
	}
	printf("\n--- ping statistics ---\n");
	printf("%ld packets transmitted, ", ntransmitted);
	printf("%ld packets received\n\n", nreceived);
	if (nrepeats)
		printf("%ld duplicates, ", nrepeats);
	printf("\n");
	return ret;
setErr_ping:
	if (pingsock >= 0) {
		close(pingsock);
		pingsock = -1;
	}
	printf("Ping error!!\n\n");
	return ret;
}

#ifdef CONFIG_DEV_xDSL
int testOAMLoopback(MIB_CE_ATM_VC_Tp pEntry, unsigned char scope, unsigned char type)
{
#ifdef EMBED
#ifdef CONFIG_ATM_REALTEK
	int skfd;
	struct atmif_sioc mysio;
	ATMOAMLBReq lbReq;
	ATMOAMLBState lbState;
	time_t  curTime, preTime = 0;

	if((skfd = socket(PF_ATMPVC, SOCK_DGRAM, 0)) < 0){
		perror("socket open error");
		return -1;
	}

	memset(&lbReq, 0, sizeof(ATMOAMLBReq));
	memset(&lbState, 0, sizeof(ATMOAMLBState));
	lbReq.Scope = scope;
	lbReq.vpi = pEntry->vpi;
	if (type == 5)
		lbReq.vci = pEntry->vci;
	else if (type == 4) {
		if (scope == 0)	// Segment
			lbReq.vci = 3;
		else if (scope == 1)	// End-to-end
			lbReq.vci = 4;
	}
	memset(lbReq.LocID, 0xff, 16);	// Loopback Location ID
	mysio.number = 0;	// ATM interface number
	mysio.arg = (void *)&lbReq;
	// Start the loopback test
	if (ioctl(skfd, ATM_OAM_LB_START, &mysio)<0) {
		perror("ioctl: ATM_OAM_LB_START failed !");
		close(skfd);
		return -1;
	}

	finished = 0;
	time(&preTime);
	// Query the loopback status
	mysio.arg = (void *)&lbState;
	lbState.vpi = pEntry->vpi;
	if (type == 5)
		lbState.vci = pEntry->vci;
	else if (type == 4) {
		if (scope == 0)	// Segment
			lbState.vci = 3;
		else if (scope == 1)	// End-to-end
			lbState.vci = 4;
	}
	lbState.Tag = lbReq.Tag;

	while (1)
	{
		time(&curTime);
		if (curTime - preTime >= MAXWAIT)
		{
			//printf("OAMLB timeout!\n");
			finished = 1;
			break;	// break for timeout
		}

		if (ioctl(skfd, ATM_OAM_LB_STATUS, &mysio)<0) {
			perror("ioctl: ATM_OAM_LB_STATUS failed !");
			mysio.arg = (void *)&lbReq;
			ioctl(skfd, ATM_OAM_LB_STOP, &mysio);
			close(skfd);
			return -1;
		}

		if (lbState.count[0] > 0)
		{
			break;	// break for loopback success
		}
	}

	mysio.arg = (void *)&lbReq;
	// Stop the loopback test
	if (ioctl(skfd, ATM_OAM_LB_STOP, &mysio)<0) {
		perror("ioctl: ATM_OAM_LB_STOP failed !");
		close(skfd);
		return -1;
	}
	close(skfd);

	if (!finished) {
		printf("\n--- Loopback cell received successfully ---\n");
		return 1;	// successful
	}
	else {
		printf("\n--- Loopback failed ---\n");
		return 0;	// failed
	}
#endif // of CONFIG_ATM_REALTEK
#else
	return 1;
#endif
}
#endif	//CONFIG_DEV_xDSL

int defaultGWAddr(char *gwaddr)
{
	char buff[256], ifname[16];
	int flgs;
	//unsigned long int g;
	//struct in_addr gw;
	struct in_addr gw, dest, mask;
	FILE *fp;

	if (!(fp=fopen("/proc/net/route", "r"))) {
		printf("Error: cannot open /proc/net/route - continuing...\n");
		return -1;
	}

	fgets(buff, sizeof(buff), fp);
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		//if (sscanf(buff, "%s%lx%*lx%X%", ifname, &g, &flgs) != 3) {
		if (sscanf(buff, "%s%x%x%x%*d%*d%*d%x", ifname, &dest, &gw, &flgs, &mask) != 5) {
			printf("Unsupported kernel route format\n");
			fclose(fp);
			return -1;
		}
		if(flgs & RTF_UP) {
			// default gateway
			if (dest.s_addr == 0 && mask.s_addr == 0) {
				if (gw.s_addr != 0) {
					sprintf(gwaddr, "%s", inet_ntoa(gw));
					fclose(fp);
					return 0;
				}
				else {
					if (getInAddr(ifname, DST_IP_ADDR, (void *)&gw) == 1)
						if (gw.s_addr != 0) {
							sprintf(gwaddr, "%s", inet_ntoa(gw));
							fclose(fp);
							return 0;
						}
				}
			}
		}
		/*if((g == 0) && (flgs & RTF_UP)) {
			if (getInAddr(ifname, DST_IP_ADDR, (void *)&gw) == 1)
				if (gw.s_addr != 0) {
					sprintf(gwaddr, "%s", inet_ntoa(gw));
					fclose(fp);
					return 0;
				}
		}
		if (sscanf(buff, "%*s%*lx%lx%X%", &g, &flgs) != 2) {
			printf("Unsupported kernel route format\n");
			fclose(fp);
			return -1;
		}
		if(flgs & RTF_UP) {
			gw.s_addr = g;
			if (gw.s_addr != 0) {
				sprintf(gwaddr, "%s", inet_ntoa(gw));
				fclose(fp);
				return 0;
			}
		}*/
	}
	fclose(fp);
	return -1;
}

int pdnsAddr(char *dnsaddr)
{

	FILE *fp;
	char buff[256];
	if ( (fp = fopen("/var/resolv.conf", "r")) == NULL ) {
		printf("Unable to open resolver file\n");
		return -1;
	}

	fgets(buff, sizeof(buff), fp);
	if (sscanf(buff, "nameserver %s", dnsaddr) != 1) {
		printf("Unsupported kernel route format\n");
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 0;
}

int getNameServers(char *buf)
{
	FILE *fp;
	char line[128], addr[64];
	int count = 0;

	buf[0] = '\0';
	if ((fp = fopen(RESOLV, "r")) == NULL)
		return -1;

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (sscanf(line, "nameserver %s", addr) != 1)
			continue;

		if (count == 0) {
			sprintf(buf, "%s", addr);
		} else {
			sprintf(line, ", %s", addr);
			strcat(buf, line);
		}
		count++;
	}

	fclose(fp);

	return 0;
}

int setNameServers(char *buf)
{
	FILE *fp;
	char line[128], *ptr;

	if ((fp = fopen(RESOLV, "w")) == NULL)
		return -1;

	ptr = strtok(buf, ", ");

	do {
		if (snprintf(line, sizeof(line), "nameserver %s\n", ptr) == 0)
			continue;
		fputs(line, fp);
	} while (ptr = strtok(NULL, ", "));

	fclose(fp);

	return 0;
}

#ifdef ACCOUNT_CONFIG
//Jenny, get user account privilege
int getAccPriv(char *user)
{
	int totalEntry, i;
	MIB_CE_ACCOUNT_CONFIG_T Entry;
	char suName[MAX_NAME_LEN], usName[MAX_NAME_LEN];

	mib_get(MIB_SUSER_NAME, (void *)suName);
	mib_get(MIB_USER_NAME, (void *)usName);
	if (strcmp(suName, user) == 0)
		return (int)PRIV_ROOT;
	else if (strcmp(usName, user) == 0)
		return (int)PRIV_USER;
	totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL);
	for (i=0; i<totalEntry; i++)
	{
		if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&Entry))
			continue;
		if (strcmp(Entry.userName, user) == 0)
			return Entry.privilege;
	}
	return -1;
}
#endif

//jim support dsl disconnection when firmware upgrade from Local Side.....
//retrun value: 1-local        0 - wan         -1 - error
static int isAccessFromLocal(unsigned int ip)
{
	unsigned int uLanIp;
	unsigned int uLanMask;
	char secondIpEn;
	unsigned int uSecondIp;
	unsigned int uSecondMask;

	if (!mib_get( MIB_ADSL_LAN_IP, (void *)&uLanIp ))
		return -1;
	if (!mib_get( MIB_ADSL_LAN_SUBNET, (void *)&uLanMask ))
		return -1;

	if ( (ip & uLanMask) == (uLanIp & uLanMask) ) {//in the same subnet with LAN port
		return 1;
	} else {
		if (!mib_get( MIB_ADSL_LAN_ENABLE_IP2, (void *)&secondIpEn ))
			return -1;

		if (secondIpEn == 1) {//second IP is enabled
			if (!mib_get( MIB_ADSL_LAN_IP2, (void *)&uSecondIp))
				return -1;
			if (!mib_get( MIB_ADSL_LAN_SUBNET2, (void *)&uSecondMask))
				return -1;

			if ( (ip & uSecondMask) == (uSecondIp & uSecondMask) )//in the same subnet with LAN port
				return 1;
		}
	}

	return 0;
}

#ifdef ROUTING
// Jenny, for checking duplicated destination address
int checkRoute(MIB_CE_IP_ROUTE_T rtEntry, int idx)
{
	//unsigned char destNet[4];
	unsigned long destID, netMask, nextHop;
	unsigned int totalEntry = mib_chain_total(MIB_IP_ROUTE_TBL); /* get chain record size */
	MIB_CE_IP_ROUTE_T Entry;
	int i;

	/*destNet[0] = rtEntry.destID[0] & rtEntry.netMask[0];
	destNet[1] = rtEntry.destID[1] & rtEntry.netMask[1];
	destNet[2] = rtEntry.destID[2] & rtEntry.netMask[2];
	destNet[3] = rtEntry.destID[3] & rtEntry.netMask[3];*/
	destID = *((unsigned long *)&rtEntry.destID);
	netMask = *((unsigned long *)&rtEntry.netMask);
	nextHop = *((unsigned long *)&rtEntry.nextHop);

	// check if route exists
	for (i=0; i<totalEntry; i++) {
		long pdestID, pnetMask, pnextHop;
		unsigned char pdID[4];
		char *temp;
		if (i == idx)
			continue;
		if (!mib_chain_get(MIB_IP_ROUTE_TBL, i, (void *)&Entry))
			return 0;

		pdID[0] = Entry.destID[0] & Entry.netMask[0];
		pdID[1] = Entry.destID[1] & Entry.netMask[1];
		pdID[2] = Entry.destID[2] & Entry.netMask[2];
		pdID[3] = Entry.destID[3] & Entry.netMask[3];
		temp = inet_ntoa(*((struct in_addr *)pdID));
		pdestID = ntohl(inet_addr(temp));
		pnetMask = *((unsigned long *)&Entry.netMask);
		pnextHop = *((unsigned long *)&Entry.nextHop);
		if (pdestID == destID && pnetMask == netMask && pnextHop == nextHop && rtEntry.ifIndex == Entry.ifIndex)
			return 0;
	}
	return 1;
}
#endif

int isValidIpAddr(char *ipAddr)
{
	long field[4];

	if (sscanf(ipAddr, "%ld.%ld.%ld.%ld", &field[0], &field[1], &field[2], &field[3]) != 4)
		return 0;

	if (field[0] < 1 || field[0] > 223 || field[0] == 127 || field[1] < 0 || field[1] > 255 || field[2] < 0 || field[2] > 255 || field[3] < 0 || field[3] > 254)
		return 0;

	if (inet_addr(ipAddr) == -1)
		return 0;

	return 1;
}

int isValidHostID(char *ip, char *mask)
{
	long hostIp, netMask, hid, mbit;
	int i, bit, bitcount = 0;

	inet_aton(mask, (struct in_addr *)&netMask);
	hostIp = ntohl(inet_addr(ip));
	netMask = ntohl(netMask);

	hid = ~netMask & hostIp;
	if (hid == 0x0)
		return 0;
	mbit = 0;
	while (1) {
		if (netMask & 0x80000000) {
			mbit++;
			netMask <<= 1;
		}
		else
			break;
	}
	mbit = 32 - mbit;
	for (i=0; i<mbit; i++) {
		bit = hid & 1L;
		if (bit)
			bitcount ++;
		hid >>= 1;
	}
	if (bitcount == mbit)
		return 0;
	return 1;
}

int isValidNetmask(char *mask, int checkbyte)
{
	long netMask;
	int i, bit, isChanged = 0;

	netMask = ntohl(inet_addr(mask));

	// Check most byte (must be 255) and least significant bit (must be 0)
	if (checkbyte) {
		bit = (netMask & 0xFF000000L) >> 24;
		if (bit != 255)
			return 0;
	}

//	bit = netMask & 1L;
//	if (bit)
//		return 0;

	// make sure the bit pattern changes from 0 to 1 only once
	for (i=1; i<31; i++) {
		netMask >>= 1;
		bit = netMask & 1L;

		if (bit) {
			if (!isChanged)
				isChanged = 1;
		}
		else {
			if (isChanged)
				return 0;
		}
	}

	return 1;
}

// check whether an IP address is in the same subnet
int isSameSubnet(char *ipAddr1, char *ipAddr2, char *mask)
{
	long netAddr1, netAddr2, netMask;

	netAddr1 = inet_addr(ipAddr1);
	netAddr2 = inet_addr(ipAddr2);
	netMask = inet_addr(mask);

	if ((netAddr1 & netMask) != (netAddr2 & netMask))
		return 0;

	return 1;
}

int isValidMacString(char *MacStr)
{
	int i;

	if(strlen(MacStr) != 17){
		return 0;
	}

	for(i=0;i<17;i++){
		if((i+1)%3 == 0){
			if(MacStr[i] != ':')
				return 0;
		}else{
			if(!((MacStr[i] >= '0' && MacStr[i] <= '9')
				|| (MacStr[i] >= 'a' && MacStr[i] <= 'f')
				|| (MacStr[i] >= 'A' && MacStr[i] <= 'F'))){
				return 0;
			}
		}
	}
	return 1;
}

int isValidMacAddr(unsigned char *macAddr)
{
	// Check for bad, multicast, broadcast, or null address
	if ((macAddr[0] & 1) || (macAddr[0] & macAddr[1] & macAddr[2] & macAddr[3] & macAddr[4] & macAddr[5]) == 0xff
		|| (macAddr[0] | macAddr[1] | macAddr[2] | macAddr[3] | macAddr[4] | macAddr[5]) == 0x00)
		return 0;

	return 1;
}

#ifdef QOS_SPEED_LIMIT_SUPPORT
//return -1 --not existed
int mib_qos_speed_limit_existed(int speed,int prior)
{
	int entryTotalNum=mib_chain_total(MIB_QOS_SPEED_LIMIT);
	int i=0;
	MIB_CE_IP_QOS_SPEEDRANK_T entry;
	for(i=0;i<entryTotalNum;i++)
		{
		    mib_chain_get(MIB_QOS_SPEED_LIMIT, i, &entry);
		   if(entry.speed==speed&&entry.prior==prior)
		   	return entry.index;
		}
	return -1;
}
#endif

//Ethernet
#if defined(ELAN_LINK_MODE)
#include <linux/sockios.h>
struct mii_ioctl_data {
	unsigned short	phy_id;
	unsigned short	reg_num;
	unsigned short	val_in;
	unsigned short	val_out;
};
#endif

int restart_ethernet(int instnum)
{
	char vChar=0;
	int status=0;

#ifdef _CWMP_MIB_
	//eth0 interface enable or disable
	mib_get(CWMP_LAN_ETHIFENABLE, (void *)&vChar);

	if(vChar==0)
	{
		va_cmd(IFCONFIG, 2, 1, ELANIF, "down");
		printf("Disable eth0 interface\n");
		return 0;
	}
	else
	{
#endif
		va_cmd(IFCONFIG, 2, 1, ELANIF, "up");
		printf("Enable eth0 interface\n");

#ifdef ELAN_LINK_MODE_INTRENAL_PHY
		int skfd;
		struct ifreq ifr;
		unsigned char mode;
		//struct mii_ioctl_data *mii = (struct mii_data *)&ifr.ifr_data;
		//MIB_CE_SW_PORT_T Port;
		//int i, k, total;
		struct ethtool_cmd ecmd;


		strcpy(ifr.ifr_name, ELANIF);
		ifr.ifr_data = &ecmd;


		if (!mib_get(MIB_ETH_MODE, (void *)&mode))
			return -1;

		skfd = socket(AF_INET, SOCK_DGRAM, 0);
		if(skfd == -1)
		{
			fprintf(stderr, "Socket Open failed Error\n");
			return -1;
		}
		ecmd.cmd = ETHTOOL_GSET;
		if (ioctl(skfd, SIOCETHTOOL, &ifr) < 0) {
			fprintf(stderr, "ETHTOOL_GSET on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
			status=-1;
			goto error;
		}

		ecmd.autoneg = AUTONEG_DISABLE;
		switch(mode) {
		case LINK_10HALF:
			ecmd.speed = SPEED_10;
			ecmd.duplex = DUPLEX_HALF;
			break;
		case LINK_10FULL:
			ecmd.speed = SPEED_10;
			ecmd.duplex = DUPLEX_FULL;
			break;
		case LINK_100HALF:
			ecmd.speed = SPEED_100;
			ecmd.duplex = DUPLEX_HALF;
			break;
		case LINK_100FULL:
			ecmd.speed = SPEED_100;
			ecmd.duplex = DUPLEX_FULL;
			break;
		default:
			ecmd.autoneg = AUTONEG_ENABLE;
		}

		ecmd.cmd = ETHTOOL_SSET;
		if (ioctl(skfd, SIOCETHTOOL, &ifr) < 0) {
			fprintf(stderr, "ETHTOOL_SSET on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
			status=-1;
			goto error;
		}
		/*
		ecmd.cmd = ETHTOOL_NWAY_RST;
		if (ioctl(skfd, SIOCETHTOOL, &ifr) < 0) {
			fprintf(stderr, "ETHTOOL_SSET on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
			status=-1;
		}
		*/

		error:

		close(skfd);
#endif
		return status;
#ifdef _CWMP_MIB_
	}
#endif

	return -1;

}

#ifdef ELAN_LINK_MODE
// return value:
// 0  : successful
// -1 : failed
int setupLinkMode(void)
{
	int skfd;
	struct ifreq ifr;
	struct mii_ioctl_data *mii = (struct mii_data *)&ifr.ifr_data;
	//MIB_CE_SW_PORT_Tp pPort;
	MIB_CE_SW_PORT_T Port;
	int i, k, total;
	int status=0;

	strcpy(ifr.ifr_name, ELANIF);
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	total = mib_chain_total(MIB_SW_PORT_TBL);

	for (i=0; i<total; i++) {
		if (!mib_chain_get(MIB_SW_PORT_TBL, i, (void *)&Port))
			continue;
		mii->phy_id = i; // phy i
		mii->reg_num = 4; // register 4
		// set NWAY advertisement
		mii->val_in = 0x0401; // enable flow control capability and IEEE802.3
		if (Port.linkMode == LINK_10HALF || Port.linkMode == LINK_AUTO)
			mii->val_in |= (1<<(5+LINK_10HALF));
		if (Port.linkMode == LINK_10FULL || Port.linkMode == LINK_AUTO)
			mii->val_in |= (1<<(5+LINK_10FULL));
		if (Port.linkMode == LINK_100HALF || Port.linkMode == LINK_AUTO)
			mii->val_in |= (1<<(5+LINK_100HALF));
		if (Port.linkMode == LINK_100FULL || Port.linkMode == LINK_AUTO)
			mii->val_in |= (1<<(5+LINK_100FULL));

		if (ioctl(skfd, SIOCSMIIREG, &ifr) < 0) {
			fprintf(stderr, "SIOCSMIIREG on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
			status=-1;
		}

		// restart
		mii->reg_num = 0; // register 0
		mii->val_in = 0x1200; // enable auto-negotiation and restart it
		if (ioctl(skfd, SIOCSMIIREG, &ifr) < 0) {
			fprintf(stderr, "SIOCSMIIREG on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
			status=-1;
		};
	}
	if(skfd!=-1) close(skfd);
	return status;
}
#endif // of ELAN_LINK_MODE

static struct callout_s *callout = NULL;	/* Callout list */
static struct timeval timenow;		/* Current time */

/*
 * timeout - Schedule a timeout.
 *
 * Note that this timeout takes the number of seconds, NOT hz (as in
 * the kernel).
 */
void
timeout_utility(func, arg, time, handle)
    void (*func) __P((void *));
    void *arg;
    int time;
    struct callout_s *handle;
{
    struct callout_s *p, **pp;

    untimeout_utility(handle);

    handle->c_arg = arg;
    handle->c_func = func;
    gettimeofday(&timenow, NULL);
    handle->c_time.tv_sec = timenow.tv_sec + time;
    handle->c_time.tv_usec = timenow.tv_usec;

    /*
     * Find correct place and link it in.
     */
    for (pp = &callout; (p = *pp); pp = &p->c_next)
	if (handle->c_time.tv_sec < p->c_time.tv_sec
	    || (handle->c_time.tv_sec == p->c_time.tv_sec
		&& handle->c_time.tv_usec < p->c_time.tv_usec))
	    break;
    handle->c_next = p;
    *pp = handle;
}


/*
 * untimeout - Unschedule a timeout.
 */
void
untimeout_utility(handle)
struct callout_s *handle;
{
    struct callout_s **copp, *freep;

    /*
     * Find first matching timeout and remove it from the list.
     */
    for (copp = &callout; (freep = *copp); copp = &freep->c_next)
	if (freep == handle) {
	    *copp = freep->c_next;
	    break;
	}
}


/*
 * calltimeout - Call any timeout routines which are now due.
 */
void
calltimeout()
{
    struct callout_s *p;

    while (callout != NULL) {
		p = callout;
		if (gettimeofday(&timenow, NULL) < 0)
		    //fatal("Failed to get time of day: %m");
	    	printf("Failed to get time of day: %m");
		if (!(p->c_time.tv_sec < timenow.tv_sec
		      || (p->c_time.tv_sec == timenow.tv_sec
			  && p->c_time.tv_usec <= timenow.tv_usec)))
	    	break;		/* no, it's not time yet */
		callout = p->c_next;
		(*p->c_func)(p->c_arg);
    }
}

/*
 * timeleft - return the length of time until the next timeout is due.
 */
static struct timeval *
timeleft(tvp)
    struct timeval *tvp;
{
    if (callout == NULL)
	return NULL;

    gettimeofday(&timenow, NULL);
    tvp->tv_sec = callout->c_time.tv_sec - timenow.tv_sec;
    tvp->tv_usec = callout->c_time.tv_usec - timenow.tv_usec;
    if (tvp->tv_usec < 0) {
	tvp->tv_usec += 1000000;
	tvp->tv_sec -= 1;
    }
    if (tvp->tv_sec < 0)
	tvp->tv_sec = tvp->tv_usec = 0;

    return tvp;
}

#ifdef WEB_REDIRECT_BY_MAC
struct callout_s landingPage_ch;
void clearLandingPageRule(void *dummy)
{
	int status=0;
	char ipaddr[16], ip_port[32];
	char tmpbuf[MAX_URL_LEN];
	int  def_port=WEB_REDIR_BY_MAC_PORT;
	unsigned int uLTime;

	va_cmd(IPTABLES, 4, 1, "-t", "nat", "-F", "WebRedirectByMAC");

	ipaddr[0]='\0'; ip_port[0]='\0';

	if (mib_get(MIB_ADSL_LAN_IP, (void *)tmpbuf) != 0)
	{
		strncpy(ipaddr, inet_ntoa(*((struct in_addr *)tmpbuf)), 16);
		ipaddr[15] = '\0';
		sprintf(ip_port,"%s:%d",ipaddr,def_port);
	}

	//iptables -t nat -A WebRedirectByMAC -d 192.168.1.1 -j RETURN
	status|=va_cmd(IPTABLES, 8, 1, "-t", "nat",(char *)FW_ADD,"WebRedirectByMAC",
		"-d", ipaddr, "-j", (char *)FW_RETURN);

	//iptables -t nat -A WebRedirectByMAC -p tcp --dport 80 -j DNAT --to-destination 192.168.1.1:8080
	status|=va_cmd(IPTABLES, 12, 1, "-t", "nat",(char *)FW_ADD,"WebRedirectByMAC",
		"-p", "tcp", "--dport", "80", "-j", "DNAT",
		"--to-destination", ip_port);

	mib_chain_clear(MIB_WEB_REDIR_BY_MAC_TBL);

	//update to the flash
	#if 0
	itfcfg("sar", 0);
	itfcfg(ELANIF, 0);
	#endif
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
	#if 0
	itfcfg("sar", 1);
	itfcfg(ELANIF, 1);
	#endif

        mib_get(MIB_WEB_REDIR_BY_MAC_INTERVAL, (void *)&uLTime);
	TIMEOUT_F(clearLandingPageRule, 0, uLTime, landingPage_ch);
}
#endif

#ifdef CONFIG_NET_IPGRE
struct callout_s gre_ch;
static unsigned long rx_record[MAX_GRE_NUM] = {0, 0};
static unsigned char endpoint_idx[MAX_GRE_NUM] = {1, 1};

void poll_GRE(void *dummy)
{
	struct net_device_stats nds;
	MIB_GRE_T Entry;
	int i, EntryNum;
	char ifname[IFNAMSIZ];	
	
	EntryNum = mib_chain_total(MIB_GRE_TBL);
	for(i=0;i<EntryNum; i++) {
		if(!mib_chain_get(MIB_GRE_TBL, i, (void*)&Entry)||!Entry.enable)
			continue;
		
		strncpy(ifname, (char *)GREIF[i], sizeof(ifname));
		get_net_device_stats(ifname, &nds);
		//printf("%s rx=%u, rx_record[0]=%u, endpoint_idx[0]=%d\n", ifname, nds.rx_packets, rx_record[i], endpoint_idx[i]);
		if (nds.rx_packets == rx_record[i]) {
			//printf("%s rx=%u, rx_record[0]=%u,  create %d endpoint\n", ifname, nds.rx_packets, rx_record[i], Entry.nextEndpointIdx);
			gre_take_effect(Entry.nextEndpointIdx);
		}
		rx_record[i] = nds.rx_packets;
	}
	TIMEOUT_F(poll_GRE, 0, GRE_BACKUP_INTERVAL, gre_ch);
}
#endif

#ifdef DMZ
#ifdef AUTO_DETECT_DMZ
#define AUTO_DMZ_INTERVAL 30
static int getDhcpClientIP(char **ppStart, unsigned long *size, char *ip)
{
	struct dhcpOfferedAddr {
        	u_int8_t chaddr[16];
        	u_int32_t yiaddr;       /* network order */
        	u_int32_t expires;      /* host order */
			u_int32_t interfaceType;
			u_int8_t hostName[64];
#ifdef _PRMT_X_CT_SUPPER_DHCP_LEASE_SC
			int category;
			int isCtcVendor;
			char szVendor[36];
			char szModel[36];
			char szFQDN[64];
#endif
	};
	struct dhcpOfferedAddr entry;
	if (*size < sizeof(entry))
		return -1;
	entry = *((struct dhcpOfferedAddr *)*ppStart);
	*ppStart = *ppStart + sizeof(entry);
	*size = *size - sizeof(entry);
	if (entry.expires == 0)
		return 0;
	if (entry.chaddr[0]==0&&entry.chaddr[1]==0&&entry.chaddr[2]==0&&entry.chaddr[3]==0&&entry.chaddr[4]==0&&entry.chaddr[5]==0)
		return 0;
	strcpy(ip, inet_ntoa(*((struct in_addr *)&entry.yiaddr)));
	return 1;
}

static int Get1stArp(char *dmzIP)
{
	FILE *fp;
	char  buf[256];
	char tmp0[32],tmp1[32],tmp2[32];
	int dmzFlags;
	dmzIP[0] = 0; // "" empty
	fp = fopen("/proc/net/arp", "r");
	if (fp == NULL)
		printf("read arp file fail!\n");
	else {
		fgets(buf, 256, fp);//first line!?
		while(fgets(buf, 256, fp) >0) {
			//sscanf(buf, "%s", dmzIP);
			sscanf(buf,"%s	%*s	0x%x %s %s %s ", dmzIP, &dmzFlags,tmp0,tmp1,tmp2);
			if ((dmzFlags == 0) || (strncmp(tmp2,"br",2)!=0))
				continue;
			return 1;
		}
		fclose(fp);
		return 0;
	}
	return 0;
}

#define _DHCPD_PID_FILE			"/var/run/udhcpd.pid"
struct callout_s autoDMZ_ch;

void poll_autoDMZ(void *dummy)
{
	static char autoDMZ = 0;
	// signal dhcp client to renew
	struct stat status;
	char dhcpIP[40], *buffer = NULL, *ptr;
	FILE *fp;
	char dmz_ip_str[20];


	if (autoDMZ == 0) // search 1st arp
	{
		unsigned long ulDmz, ulDhcp;
		unsigned char ucPreStat;
		int ret;
		char ip[32];
		struct in_addr ipAddr, dmzIp;

		fflush(stdout);
		if (Get1stArp(dmz_ip_str) == 1)
		{
			if (strlen(dmz_ip_str) == 0)
				goto end; // error

			autoDMZ = 1;
			mib_get(MIB_DMZ_ENABLE, (void *)&ucPreStat);
			mib_get(MIB_DMZ_IP, (void *)&dmzIp);
			ulDmz = *((unsigned long *)&dmzIp);
			strncpy(ip, inet_ntoa(dmzIp), 16); // ip -> old dmz
			if (strcmp(ip,dmz_ip_str) == 0)
				goto end; // no changed!

			inet_aton(dmz_ip_str, &ipAddr);
			ip[15] = '\0';
			if (ucPreStat && ulDmz != 0)
				clearDMZ();

			if (mib_set(MIB_DMZ_IP, (void *)&ipAddr) == 0)
				printf("Set DMZ MIB error!\n");

				if (!mib_set(MIB_DMZ_ENABLE, (void *)&autoDMZ))
					printf("Set DMZ Capability error!\n");
				setDMZ(dmz_ip_str);
				goto end;

		}

	}
	else  // check dhcp and then arp
	{
		unsigned long ulDmz, ulDhcp;
		unsigned char ucPreStat;
		struct in_addr ipAddr, dmzIp;
		int ret;
		char ip[32];
		// siganl DHCP server to update lease file
		int pid = read_pid(_DHCPD_PID_FILE);

		if (pid > 0)	// renew
			kill(pid, SIGUSR1);
		usleep(1000);

		if (stat(DHCPD_LEASE, &status) < 0)
			goto end;

		// read DHCP server lease file
		buffer = malloc(status.st_size);
		if (buffer == NULL)
			goto end;
		fp = fopen(DHCPD_LEASE, "r");
		if (fp == NULL)
			goto end;
		fread(buffer, 1, status.st_size, fp);
		fclose(fp);
		ptr = buffer;

		while (1) {
			if (getDhcpClientIP(&ptr, &status.st_size, dhcpIP) == 1)
			{
				mib_get(MIB_DMZ_ENABLE, (void *)&ucPreStat);
				mib_get(MIB_DMZ_IP, (void *)&dmzIp);
				ulDmz = *((unsigned long *)&dmzIp);
				strncpy(ip, inet_ntoa(dmzIp), 16);
				if (strcmp(ip, dhcpIP) ==0 )	//dhcp still in using..
				{
					goto end;
				}
			}
			else // find the 1st arp
			{
				// uses the 1st entry
				if (Get1stArp(dmz_ip_str) == 1)
				{
					if (strlen(dmz_ip_str) == 0)
						goto end; // error

					autoDMZ = 1;
					mib_get(MIB_DMZ_ENABLE, (void *)&ucPreStat);
					mib_get(MIB_DMZ_IP, (void *)&dmzIp);
					ulDmz = *((unsigned long *)&dmzIp);
					strncpy(ip, inet_ntoa(dmzIp), 16); // ip -> old dmz
					inet_aton(dmz_ip_str, &ipAddr);
					ip[15] = '\0';
					if (strcmp(ip,dmz_ip_str) == 0)
						goto end; // still the same one

					if (ucPreStat && ulDmz != 0)
						clearDMZ();

					if (mib_set(MIB_DMZ_IP, (void *)&ipAddr) == 0)
						printf("Set DMZ MIB error!\n");

						if (!mib_set(MIB_DMZ_ENABLE, (void *)&autoDMZ))
							printf("Set DMZ Capability error!\n");
						setDMZ(dmz_ip_str);
						goto end;

				}
				else
				{
					// clear rules
					mib_get(MIB_DMZ_IP, (void *)&dmzIp);
					strncpy(ip, inet_ntoa(dmzIp), 16); // ip -> old dmz
					ip[15] = '\0';

					clearDMZ();
					*((unsigned long *)&dmzIp) = 0;
					autoDMZ = 0;
					mib_set(MIB_DMZ_ENABLE, (void *)&autoDMZ);
					mib_set(MIB_DMZ_IP, (void *)&dmzIp);
				}
				goto end;
			}
		}
	}


end:
	if (buffer)
		free(buffer);
	TIMEOUT_F(poll_autoDMZ, 0, AUTO_DMZ_INTERVAL, autoDMZ_ch);
}
#endif
#endif

void startSNAT_one(MIB_CE_ATM_VC_Tp pEntry)
{
	char wanif[IFNAMSIZ];
	
	ifGetName(pEntry->ifIndex,wanif,sizeof(wanif));
	if (((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_IPOE)  ||
	  ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1483) ||
	  ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_RT1577) )
	{
#ifdef CONFIG_IPV6
		if (pEntry->IpProtocol & IPVER_IPV4) {
#endif
			if ((DHCP_T)pEntry->ipDhcp == DHCP_DISABLED)
			{
				if (pEntry->napt == 1)
				{
					// Setup one NAT Rule for the specfic interface
					startAddressMap(pEntry);
				}
			}
			else
			{
				if (pEntry->napt == 1)
				{	// Enable NAPT
					va_cmd(IPTABLES, 8, 1, "-t", "nat", FW_ADD, "POSTROUTING",
						ARG_O, wanif, "-j", "MASQUERADE");
				}
			}
#ifdef CONFIG_IPV6
		}
#endif
	}
	else if (((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOE) ||
	      ((CHANNEL_MODE_T)pEntry->cmode == CHANNEL_MODE_PPPOA))
	{
#ifdef CONFIG_IPV6
		if (pEntry->IpProtocol & IPVER_IPV4) {
#endif
			if (pEntry->napt == 1)
			{	// Enable NAPT
				va_cmd(IPTABLES, 8, 1, "-t", "nat", FW_ADD, "POSTROUTING",
					"-o", wanif, "-j", "MASQUERADE");
			}
#ifdef CONFIG_IPV6
		}
#endif
	}
}

void startSNAT(void)
{
	int vcTotal, i;
	MIB_CE_ATM_VC_T Entry;

	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < vcTotal; i++)
	{
		/* get the specified chain record */
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return;

		if (Entry.enable == 0)
			continue;

		startSNAT_one(&Entry);
	}
}

// configAll = CONFIGALL,  pEntry = NULL  : restart all WAN connections(include VC, ETHWAN, PTMWAN, VPN, 3g).
//									  It means that we add or modify all VC, ETHWAN, PTMWAN channel.
// configAll = CONFIGONE, pEntry != NULL : restart specified VC, ETHWAN, PTMWAN connection and VPN, 3g connections.
// 									  It means that we add or modify a VC, ETHWAN, PTMWAN channel.
// configAll = CONFIGONE, pEntry = NULL  : restart VPN, 3g connections. It means that we delete a VC, ETHWAN, PTMWAN channel.
void restartWAN(int configAll, MIB_CE_ATM_VC_Tp pEntry)
{
	int dhcrelay_pid, i;
	char vChar;
#ifdef CONFIG_IPV6
	unsigned char tmpBuf[64];
#endif

#ifdef CONFIG_DEV_xDSL
	itfcfg("sar", 0);
#endif

#if !defined(CONFIG_LUNA)
	va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
#else
	//va_cmd("/bin/ethctl", 2, 1, "conntrack", "killall");
#endif
	cleanAllFirewallRule();

#if defined(CONFIG_RTK_L34_ENABLE)
	/* Turn switch WAN port down */
	//RG_wan_phy_force_power_down(1);
#endif

// reconfigure RG LAN interface
#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_ENABLE)
//	Init_RG_ELan(UntagCPort, RoutingWan);
#endif

#if defined(CONFIG_RTK_L34_ENABLE) && defined(CONFIG_USER_RTK_WAN_CTYPE)
{
	int EntryID, ret;
	MIB_CE_ATM_VC_T tmp;
	ret = RG_check_Droute(configAll,pEntry,&EntryID);

	fprintf(stderr, "[%s@%d] RG_check_Droute ret = %d \n", __FUNCTION__, __LINE__, ret);
	if(ret == 3 && configAll != CONFIGCWMP){	
	/*remove, modify D route case!!!!*/
//AUG_PRT("%s-%d EntryID=%d\n",__func__,__LINE__,EntryID);
		if (!mib_chain_get(MIB_ATM_VC_TBL, EntryID, (void*)&tmp))
			return -1;
//AUG_PRT("%s-%d tmp.enable=%d\n",__func__,__LINE__,tmp.enable);
		deleteConnection(CONFIGONE, &tmp);// Modify or remove D route
		startWan(CONFIGONE, &tmp);
	}
//AUG_PRT("%s-%d\n",__func__,__LINE__);
}
#endif
#ifdef CONFIG_USER_BRIDGE_GROUPING
	setup_bridge_grouping(DEL_RULE);
#endif
#ifdef CONFIG_USER_VLAN_ON_LAN
	setup_VLANonLAN(DEL_RULE);
#endif
	if (configAll == CONFIGALL)
		startWan(CONFIGALL, NULL);
	else if (configAll == CONFIGONE)
		startWan(CONFIGONE, pEntry);
	else if (configAll == CONFIGCWMP)
		startWan(CONFIGCWMP, NULL);

#if defined(CONFIG_RTK_L34_ENABLE)
	/* Turn switch WAN port up */
	//RG_wan_phy_force_power_down(0);
#endif

	mib_get(MIB_MPMODE, (void *)&vChar);

#ifdef NEW_PORTMAPPING
	setupnewEth2pvc();
#endif

	// setup IP QoS
#ifdef IP_QOS
	if (vChar&MP_IPQ_MASK) {
		stopIPQ();
		setupIPQ();
	}
#endif

// Mason Yu. SIGRTMIN for DHCP Relay.catch SIGRTMIN(090605)
#ifdef COMBINE_DHCPD_DHCRELAY
	dhcrelay_pid = read_pid("/var/run/udhcpd.pid");
#else
	dhcrelay_pid = read_pid("/var/run/dhcrelay.pid");
#endif
	if (dhcrelay_pid > 0) {
		printf("restartWAN1: kick dhcrelay(%d) to Re-discover all network interface\n", dhcrelay_pid);
		// Mason Yu. catch SIGRTMIN(It is a real time signal and number is 32) for re-sync all interface on DHCP Relay
		// Kaohj -- SIGRTMIN is not constant, so we use 32.
		//kill(dhcrelay_pid, SIGRTMIN);
		kill(dhcrelay_pid, 32);
	}

	// Mason Yu.
#ifdef CONFIG_IPV6
	if (mib_get(MIB_IPV6_LAN_IP_ADDR, (void *)tmpBuf) != 0)
	{
		char cmdBuf[100]={0};
		sprintf(cmdBuf, "%s/%d", tmpBuf, 64);
		va_cmd(IFCONFIG, 3, 1, LANIF, ARG_ADD, cmdBuf);

		//fix two default IPv6 gateway in LAN, Alan
		delOrgLanLinklocalIPv6Address();
	}


#ifdef CONFIG_USER_DHCPV6_ISC_DHCP411
	start_dhcpv6(1);
#endif
#endif

#ifdef IP_QOS
	update_qos_tbl();
#endif

	//ql 20081118 START restart IP QoS
#ifdef NEW_IP_QOS_SUPPORT
	take_qos_effect();
#endif
#ifdef CONFIG_USER_IP_QOS_3
	take_qos_effect_v3();
#endif
#ifdef CONFIG_DEV_xDSL
	itfcfg("sar", 1);
#endif

#ifdef CONFIG_USER_XFRM
	ipsec_take_effect();
#endif

#ifdef CONFIG_USER_PPTP_CLIENT_PPTP
#ifdef CONFIG_USER_PPTPD_PPTPD
	pptpd_take_effect();
#endif
#endif

#ifdef CONFIG_USER_L2TPD_LNS
	l2tpd_take_effect();
#endif
#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_ENABLE)
{
#if defined(CONFIG_IPV6)
	RTK_RG_FLUSH_Route_V6_RA_NS_ACL_FILE();
	RTK_RG_Set_ACL_Route_V6_RA_NS_Filter();	
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE) || defined(CONFIG_FIBER_FEATURE)
		int pon_mode=0, acl_default=0;
		if (mib_get(MIB_PON_MODE, (void *)&pon_mode) != 0)
		{
#ifdef CONFIG_RTL9602C_SERIES
			acl_default = 1;
#endif
			if ((pon_mode != GPON_MODE) || acl_default == 1)
			{
				RG_add_default_Acl_Qos();
			}
		}
#else
		/*use for 8696*/
		RG_add_default_Acl_Qos();
#endif
#ifndef CONFIG_RTL9600_SERIES
		check_port_based_vlan_of_bridge_inet_wan();
#endif
}
#endif
#ifdef CONFIG_USER_VLAN_ON_LAN
	setup_VLANonLAN(ADD_RULE);
#endif
#ifdef CONFIG_USER_BRIDGE_GROUPING
	setup_bridge_grouping(ADD_RULE);
#endif
#ifdef CONFIG_00R0
	printf("Now echo 1 > /proc/rg/wan_dmac2cvid_force_disabled\n");
	system("echo 1 > /proc/rg/wan_dmac2cvid_force_disabled");
#endif
}

/*
 *	Deal with configuration dependency when a WAN channel has been deleted.
 */
void resolveServiceDependency(unsigned int idx)
{
	MIB_CE_ATM_VC_T Entry;
#ifdef CONFIG_USER_IGMPPROXY
	unsigned int igmp_proxy_itf;
	unsigned char igmp_enable;
#endif
#ifdef IP_PASSTHROUGH
	unsigned int ippt_itf;
	unsigned int ippt_lease;
#endif
#ifdef CONFIG_USER_MINIUPNPD
	unsigned char upnpdEnable;
	unsigned int upnpItf;
#endif
	struct data_to_pass_st msg;
	int k;

	/* get the specified chain record */
	if (!mib_chain_get(MIB_ATM_VC_TBL, idx, (void *)&Entry))
	{
		return;
	}

#ifdef CONFIG_USER_IGMPPROXY
	// resolve IGMP proxy dependency
	if(!mib_get( MIB_IGMP_PROXY_ITF,  (void *)&igmp_proxy_itf))
		return;
	if (Entry.ifIndex == igmp_proxy_itf)
	{ // This interface is IGMP proxy interface
		igmp_proxy_itf = DUMMY_IFINDEX;	// set to default
		mib_set(MIB_IGMP_PROXY_ITF, (void *)&igmp_proxy_itf);
		igmp_enable = 0;	// disable IGMP proxy
		mib_set(MIB_IGMP_PROXY, (void *)&igmp_enable);
	}
#endif

#ifdef IP_PASSTHROUGH
	// resolve IP passthrough dependency
	if(!mib_get( MIB_IPPT_ITF,  (void *)&ippt_itf))
		return;
	if (Entry.ifIndex == ippt_itf)
	{ // This interface is IP passthrough interface
		ippt_itf = DUMMY_IFINDEX;	// set to default
		mib_set(MIB_IPPT_ITF, (void *)&ippt_itf);
		ippt_lease = 600;	// default to 10 min.
		mib_set(MIB_IPPT_LEASE, (void *)&ippt_lease);
	}
#endif

#ifdef CONFIG_USER_MINIUPNPD
	if (mib_get(MIB_UPNP_DAEMON, &upnpdEnable) && upnpdEnable) {
		if (mib_get(MIB_UPNP_EXT_ITF, &upnpItf)
				&& upnpItf == Entry.ifIndex) {
			upnpdEnable = 0;
			mib_set(MIB_UPNP_DAEMON, &upnpdEnable);
		}
	}
#endif

#ifdef PORT_FORWARD_GENERAL
	delPortForwarding( Entry.ifIndex );
#endif
#ifdef ROUTING
	delRoutingTable( Entry.ifIndex );
#endif
#ifdef CONFIG_USER_ROUTED_ROUTED
	delRipTable(Entry.ifIndex);
#endif
	delPPPoESession(Entry.ifIndex);
}

#ifdef IP_PASSTHROUGH
static void set_IPPT_LAN_access()
{
	unsigned int ippt_itf;
	unsigned char value[32];

	// IP Passthrough, LAN access
	if (mib_get(MIB_IPPT_ITF, (void *)&ippt_itf) != 0)
		if (ippt_itf != DUMMY_IFINDEX) {	// IP passthrough
			mib_get(MIB_IPPT_LANACC, (void *)value);
			if (value[0] == 0)	// disable LAN access
				// iptables -A FORWARD -i $LAN_IF -o $LAN_IF -j DROP
				va_cmd(IPTABLES, 8, 1, (char *)FW_ADD,
					(char *)FW_FORWARD, (char *)ARG_I,
					(char *)LANIF, (char *)ARG_O,
					(char *)LANIF, "-j", (char *)FW_DROP);
		}
}

static void clean_IPPT_LAN_access()
{
	// iptables -D FORWARD -i $LAN_IF -o $LAN_IF -j DROP
	va_cmd(IPTABLES, 8, 1, (char *)FW_DEL,
		(char *)FW_FORWARD, (char *)ARG_I,
		(char *)LANIF, (char *)ARG_O,
		(char *)LANIF, "-j", (char *)FW_DROP);
}

// Mason Yu
void restartIPPT(struct ippt_para para)
{
	unsigned int entryNum, i, idx, vcIndex, selected;
	MIB_CE_ATM_VC_T Entry;
	FILE *fp;
#ifdef CONFIG_USER_DHCP_SERVER
	int restar_dhcpd_flag=0;
#endif
	int isPPPoE=0;
	unsigned long myipaddr, hisipaddr;
	char pppif[6], globalIP_str[16];
	MEDIA_TYPE_T mType;

	//printf("Take effect for IPPT and old_ippt_itf=%d new_ippt_itf=%d\n", para.old_ippt_itf, para.new_ippt_itf);
	//printf("Take effect for IPPT and old_ippt_lease=%d new_ippt_lease=%d\n", para.old_ippt_lease, para.new_ippt_lease);
	//printf("Take effect for IPPT and old_ippt_lanacc=%d new_ippt_lanacc=%d\n", para.old_ippt_lanacc, para.new_ippt_lanacc);

       	// Stop IPPT
       	// If old_ippt_itf != 255 and new_ippt_itf != old_ippt_itf, it is that the IPPT is enabled. We should clear some configurations.
	if ( para.old_ippt_itf != DUMMY_IFINDEX  && para.new_ippt_itf != para.old_ippt_itf) {
		#ifdef CONFIG_USER_DHCP_SERVER
		// (1)  set restart DHCP server flag with 1.
		restar_dhcpd_flag = 1;  // on restart DHCP server flag
		#endif

		// (2) Delete /tmp/PPPHalfBridge file for DHCP Server
       		fp = fopen("/tmp/PPPHalfBridge", "r");
       		if (fp) {
       			fread(&myipaddr, 4, 1, fp);
	 	       	fread(&hisipaddr, 4, 1, fp);
       			unlink("/tmp/PPPHalfBridge");
       			fclose(fp);
       		}

       		// (3) Delete /tmp/IPoAHalfBridge file for DHCP Server
       		fp = fopen("/tmp/IPoAHalfBridge", "r");
       		if (fp) {
       			fread(&myipaddr, 4, 1, fp);
	 	       	fread(&hisipaddr, 4, 1, fp);
       			unlink("/tmp/IPoAHalfBridge");
       			fclose(fp);
       		}

		// Change Public IP to string
		snprintf(globalIP_str, 16, "%d.%d.%d.%d", (int)(myipaddr>>24)&0xff, (int)(myipaddr>>16)&0xff, (int)(myipaddr>>8)&0xff, (int)(myipaddr)&0xff);

		// (4) Delete LAN IPPT interface
		va_cmd(IFCONFIG, 2, 1, (char*)LAN_IPPT,"down");

		// (5) Delete a public IP's route
               	va_cmd(ROUTE, 5, 1, ARG_DEL, "-host", globalIP_str, "dev", LANIF);

		// (6) Restart the previous IPPT WAN connection.
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < entryNum; i++) {
			/* Retrieve entry */
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
				printf("restartIPPT: cannot get ATM_VC_TBL-1(ch=%d) entry\n", i);
				return;
			}

			/* remove connection on driver*/
			if (para.old_ippt_itf == Entry.ifIndex) {
				stopConnection(&Entry);

				// If this connection is PPPoE/oA, we should kill the old NAPT rule in POSTROUTING chain.
				// When channel mode is rt1483, NAPT rule will be deleted by stopConnection().
				if (Entry.cmode == CHANNEL_MODE_PPPOE || Entry.cmode == CHANNEL_MODE_PPPOA) {
					snprintf(pppif, 6, "ppp%u", PPP_INDEX(Entry.ifIndex));
        				va_cmd(IPTABLES, 10, 1, "-t", "nat", FW_DEL, "POSTROUTING",
			 			"-o", pppif, "-j", "SNAT", "--to-source", globalIP_str);
				}

				if (Entry.cmode == CHANNEL_MODE_PPPOE) {
					isPPPoE = 1;
					vcIndex = VC_INDEX(Entry.ifIndex);
					mType = MEDIA_INDEX(Entry.ifIndex);
					selected = i;
				}

#if defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN)
				addEthWANdev(&Entry);
#endif
				startConnection(&Entry, i);
				startSNAT_one(&Entry);
				break;
			}
		}
	}

	if (isPPPoE) {
		for (i=0; i<entryNum; i++) {
			if (i == selected)
				continue;
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
				printf("restartIPPT: cannot get ATM_VC_TBL-1(ch=%d) entry\n", i);
				return;
			}
			if (mType == MEDIA_INDEX(Entry.ifIndex) && vcIndex == VC_INDEX(Entry.ifIndex)) {	// Jenny, for multisession PPPoE support
				startConnection(&Entry, i);
			}
		}
	}

	// Start New IPPT
	if ( para.new_ippt_itf != DUMMY_IFINDEX && para.new_ippt_itf != para.old_ippt_itf) {
		#ifdef CONFIG_USER_DHCP_SERVER
		// (1)  set restart DHCP server flag with 1.
		restar_dhcpd_flag = 1;  // on restart DHCP server flag
		#endif

		// (2) Config WAN interface and reconnect to DSL.
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < entryNum; i++) {
			/* Retrieve entry */
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
				printf("restartIPPT: cannot get ATM_VC_TBL-2(ch=%d) entry\n", i);
				return;
			}

			if (para.new_ippt_itf == Entry.ifIndex) {
				// If this connection is PPPoE/oA, we should kill the old NAPT rule in POSTROUTING chain.
				// When channel mode is rt1483, NAPT rule will be deleted by stopConnection().
				if ( (Entry.cmode == CHANNEL_MODE_PPPOE || Entry.cmode == CHANNEL_MODE_PPPOA)) {
					snprintf(pppif, 6, "ppp%u", PPP_INDEX(Entry.ifIndex));
					va_cmd(IPTABLES, 8, 1, "-t", "nat", FW_DEL, "POSTROUTING",
			 			"-o", pppif, "-j", "MASQUERADE");
				}
				stopConnection(&Entry);
				if (Entry.cmode == CHANNEL_MODE_PPPOE) {
					isPPPoE = 1;
					vcIndex = VC_INDEX(Entry.ifIndex);
					mType = MEDIA_INDEX(Entry.ifIndex);
					selected = i;
				}
#if defined(CONFIG_RTL_MULTI_ETH_WAN) || defined(CONFIG_PTMWAN)
				addEthWANdev(&Entry);
#endif
				startConnection(&Entry, i);
				startSNAT_one(&Entry);
				break;
			}
		}
		if (isPPPoE) {
			for (i=0; i<entryNum; i++) {
				if (i == selected)
					continue;
				if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry)) {
					printf("restartIPPT: cannot get ATM_VC_TBL-1(ch=%d) entry\n", i);
					return;
				}
				if (mType == MEDIA_INDEX(Entry.ifIndex) && vcIndex == VC_INDEX(Entry.ifIndex)) { // Jenny, for multisession PPPoE support
					startConnection(&Entry, i);
				}
			}
		}

	}  //  if ( new_ippt_itf != 255 )

#ifdef CONFIG_USER_DHCP_SERVER
	// Check IPPT Lease Time
	// Here we just concern about IPPT is enable.
	if ( para.old_ippt_lease != para.new_ippt_lease && para.new_ippt_itf != DUMMY_IFINDEX) {
		restar_dhcpd_flag = 1;  // on restart DHCP server flag
		//printf("change IPPT Lease Time\n");
	}
#endif

	// Check IPPT LAN Access
	if ( para.new_ippt_itf != DUMMY_IFINDEX || para.old_ippt_itf != DUMMY_IFINDEX) {
		if ( para.old_ippt_lanacc == 0 && para.old_ippt_itf != DUMMY_IFINDEX)
			clean_IPPT_LAN_access();
		set_IPPT_LAN_access();
	}

#ifdef CONFIG_USER_DHCP_SERVER
	// Restart DHCP Server
	if ( restar_dhcpd_flag == 1 ) {
		restar_dhcpd_flag = 0;  // off restart DHCP server flag
		restart_dhcp();
	}
#endif

}
#endif

#ifdef DOS_SUPPORT
void setup_dos_protection(void)
{
	unsigned int dos_enable;
	unsigned int dos_syssyn_flood;
	unsigned int dos_sysfin_flood;
	unsigned int dos_sysudp_flood;
	unsigned int dos_sysicmp_flood;
	unsigned int dos_pipsyn_flood;
	unsigned int dos_pipfin_flood;
	unsigned int dos_pipudp_flood;
	unsigned int dos_pipicmp_flood;
	unsigned int dos_block_time;
	unsigned char buffer[256],dosstr[256];
	int dostmpip[4],dostmpmask[4];
	int dosip,dosmask;
	FILE *dosfp;

	if (!mib_get( MIB_DOS_ENABLED,  (void *)&dos_enable)){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get( MIB_DOS_SYSSYN_FLOOD,  (void *)&dos_syssyn_flood)){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get( MIB_DOS_SYSFIN_FLOOD,  (void *)&dos_sysfin_flood)){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get( MIB_DOS_SYSUDP_FLOOD,  (void *)&dos_sysudp_flood)){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get( MIB_DOS_SYSICMP_FLOOD,  (void *)&dos_sysicmp_flood)){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get( MIB_DOS_PIPSYN_FLOOD,  (void *)&dos_pipsyn_flood)){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get( MIB_DOS_PIPFIN_FLOOD,  (void *)&dos_pipfin_flood)){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get( MIB_DOS_PIPUDP_FLOOD,  (void *)&dos_pipudp_flood)){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get( MIB_DOS_PIPICMP_FLOOD,  (void *)&dos_pipicmp_flood)){
		printf("DOS parameter failed!\n");
	}
	if (!mib_get( MIB_DOS_BLOCK_TIME,  (void *)&dos_block_time)){
		printf("DOS parameter failed!\n");
	}

	//get ip
	if(!mib_get( MIB_ADSL_LAN_IP, (void *)buffer))
		return;
	sscanf(inet_ntoa(*((struct in_addr *)buffer)),"%d.%d.%d.%d",&dostmpip[0],&dostmpip[1],&dostmpip[2],&dostmpip[3]);
	dosip= dostmpip[0]<<24 | dostmpip[1]<<16 | dostmpip[2]<<8 | dostmpip[3];
	//get mask
	if(!mib_get( MIB_ADSL_LAN_SUBNET, (void *)buffer))
		return;
	sscanf(inet_ntoa(*((struct in_addr *)buffer)),"%d.%d.%d.%d",&dostmpmask[0],&dostmpmask[1],&dostmpmask[2],&dostmpmask[3]);
	dosmask= dostmpmask[0]<<24 | dostmpmask[1]<<16 | dostmpmask[2]<<8 | dostmpmask[3];
	dosip &= dosmask;
	sprintf(dosstr,"1 %x %x %d %d %d %d %d %d %d %d %d %d\n",
		dosip,dosmask,dos_enable,dos_syssyn_flood,dos_sysfin_flood,dos_sysudp_flood,
		dos_sysicmp_flood,dos_pipsyn_flood,dos_pipfin_flood,dos_pipudp_flood,
		dos_pipicmp_flood,dos_block_time);
	//printf("dosstr:%s\n\n",dosstr);
	dosstr[strlen(dosstr)]=0;
	if (dosfp = fopen("/proc/enable_dos", "w"))
	{
			fprintf(dosfp, "%s",dosstr);
			fclose(dosfp);
	}

#ifdef CONFIG_RTK_L34_ENABLE
	//setup dos
	RTK_RG_DoS_Set(dos_enable);
#endif

}
#endif

int pppdbg_get(int unit)
{
	char buff[256];
	FILE *fp;
	int pppinf, pppdbg = 0;

	if (fp = fopen(PPP_DEBUG_LOG, "r")) {
		while (fgets(buff, sizeof(buff), fp) != NULL) {
			if (sscanf(buff, "%d:%d", &pppinf, &pppdbg) != 2)
				break;
			else {
				if (pppinf == unit)
					break;
			}
		}
		fclose(fp);
	}
	return pppdbg;
}

#ifdef CONFIG_ATM_CLIP
void sendAtmInARPRep(unsigned char update)
{
	int i, totalEntry;
	MIB_CE_ATM_VC_T Entry;
	struct data_to_pass_st msg;
	char wanif[16];
	unsigned long addr;

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	if (update) {	// down --> up
		for (i=0; i<totalEntry; i++) {
			mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry);
			if (Entry.enable == 0)
				continue;
			if (Entry.cmode == CHANNEL_MODE_RT1577) {
				addr = *((unsigned long *)Entry.remoteIpAddr);
				ifGetName(Entry.ifIndex, wanif, sizeof(wanif));
				snprintf(msg.data, BUF_SIZE, "mpoactl set %s inarprep %lu", wanif, addr);
				write_to_mpoad(&msg);
			}
		}
	}
}
#endif

// Magician: Commit immediately
#ifdef COMMIT_IMMEDIATELY
void Commit()
{
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
}
#endif // of #if COMMIT_IMMEDIATELY

#ifdef CONFIG_USER_SAMBA
#ifdef CONFIG_MULTI_SMBD_ACCOUNT
int addSmbPwd(char *username, char *passwd){
	FILE *fp_pass=NULL;
	char cmd[512]={0};

	if(!username || !passwd){
		fprintf(stderr, "[%s:%d] null data\n");
		return 1;
	}

	//fprintf(stderr, "[%s:%d] user:%s, pwd:%s\n", __FUNCTION__, __LINE__, username, passwd);
	fp_pass = fopen("/tmp/.pass", "w"); 
	if(fp_pass == NULL )	 
		return 1;

	fprintf(fp_pass, "%s\n",passwd);
	fprintf(fp_pass, "%s\n",passwd);
	fclose(fp_pass);

#ifdef CONFIG_USER_SMBPASSWD
	sprintf(cmd, "cat /tmp/.pass | smbpasswd -a %s -s", username);
#else
	sprintf(cmd, "cat /tmp/.pass | pdbedit -a %s -t", username);
#endif
	system(cmd);
	return 0;
}

void initSmbPwd(void){
	char userName[MAX_NAME_LEN], userPass[MAX_NAME_LEN];
#ifdef ACCOUNT_CONFIG
	int i;
	MIB_CE_ACCOUNT_CONFIG_T entry;
	unsigned int totalEntry;
#endif
	char cmd[512];

	//flush password account
#ifdef CONFIG_USER_SMBPASSWD
	sprintf(cmd,"echo > %s", SMB_PWD_FILE);
	system(cmd);
#else
	FILE *pp = popen("pdbedit -L", "r");
	char *line=NULL;
	size_t len = 0;
	ssize_t nread;
	char tmpNAME[MAX_NAME_LEN]={0}, tmpUID[6]={0};

	if(!pp){
		printf("[%s:%d] ERROR: Initial samba password fail\n", __FUNCTION__, __LINE__);
		return;
	}

	/**
	 ** pdbedit -L       ==>	admin:0:
	 ** pdbedit -L -w  ==>	admin:0:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:F441F41AA59214CCCC3D4BA5ED1550CC:[U          ]:LCT-00000012:
	**/
       while ((nread = getline(&line, &len, pp)) != -1) {
		if(sscanf(line, "%[^:]:%[^:]:", tmpNAME, tmpUID) == 2){
			sprintf(cmd,"pdbedit -x %s", tmpNAME);
			system(cmd);
		}
       }

	free(line);
	pclose(pp);
#endif

#ifdef ACCOUNT_CONFIG
	totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL); /* get chain record size */
	for (i=0; i<totalEntry; i++) {
		if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&entry)) {
			printf("ERROR: Get account configuration information from MIB database failed.\n");
			return;
		}

		addSmbPwd((char *)entry.userName, (char *)entry.userPassword); 
	}
#endif
	mib_get( MIB_SUSER_NAME, (void *)userName );
	mib_get( MIB_SUSER_PASSWORD, (void *)userPass ); 
	addSmbPwd(userName, userPass);

#ifndef CONFIG_00R0
	// Added by Mason Yu for others user
	mib_get( MIB_SUPER_NAME, (void *)userName );
	mib_get( MIB_SUPER_PASSWORD, (void *)userPass );
	addSmbPwd(userName, userPass);
#endif

	mib_get( MIB_USER_NAME, (void *)userName );
	if (userName[0]) {
		mib_get( MIB_USER_PASSWORD, (void *)userPass );
		addSmbPwd(userName, userPass);
	}

	return;
}

int updateSambaCfg(unsigned char sec_enable)
{
	char userName[MAX_NAME_LEN], userPass[MAX_NAME_LEN];
#ifdef ACCOUNT_CONFIG
	int i;
	MIB_CE_ACCOUNT_CONFIG_T entry;
	unsigned int totalEntry;
#endif

	FILE *fp;
	fp = fopen(SMB_CONF_FILE,"wb+");

	// part 1: write global config
	fprintf(fp,"%s\n","#");
	fprintf(fp,"%s\n","# Realtek Semiconductor Corp.");
	fprintf(fp,"%s\n","#");
	fprintf(fp,"%s\n","# Tony Wu (tonywu@realtek.com)");
	fprintf(fp,"%s\n","# Jan. 10, 2011\n");
	fprintf(fp,"\n");
	fprintf(fp,"%s\n","[global]");
	fprintf(fp,"%s\n","        # netbios name = Realtek");
	fprintf(fp,"%s\n","        # server string = Realtek Samba Server");
	fprintf(fp,"%s\n","        socket options = TCP_NODELAY SO_RCVBUF=131072 SO_SNDBUF=131072 IPTOS_LOWDELAY");
	fprintf(fp,"%s\n","        # unix charset = ISO-8859-1");
	fprintf(fp,"%s\n","        # preferred master = no");
	fprintf(fp,"%s\n","        # domain master = no");
	fprintf(fp,"%s\n","        # local master = yes");
	fprintf(fp,"%s\n","        # os level = 20");
	fprintf(fp,"%s\n","        smb ports = 445");
	fprintf(fp,"%s\n","        disable netbios = yes");
	fprintf(fp,"%s\n","        # guest account = admin");
	fprintf(fp,"%s\n","        deadtime = 15");
	fprintf(fp,"%s\n","        strict sync = no");
	fprintf(fp,"%s\n","        sync always = no");
	fprintf(fp,"%s\n","        dns proxy = no");
	fprintf(fp,"%s\n","        interfaces = lo, br0");
	fprintf(fp,"%s\n","        use sendfile = true");
	fprintf(fp,"%s\n","        write cache size = 8192000");
	fprintf(fp,"%s\n","        min receivefile size = 16384");
	fprintf(fp,"%s\n","        aio read size = 131072");
	fprintf(fp,"%s\n","        aio write size = 131072");
	fprintf(fp,"%s\n","        aio write behind = true");
	fprintf(fp,"%s\n","        read raw = yes");
	fprintf(fp,"%s\n","        write raw = yes");
	fprintf(fp,"%s\n","        getwd cache = yes");
	fprintf(fp,"%s\n","        oplocks = yes");
	fprintf(fp,"%s\n","        max xmit = 32768");
	fprintf(fp,"%s\n","        large readwrite = yes");

	if(!sec_enable){	//share
		fprintf(fp,"%s\n","        syslog = 10");
		fprintf(fp,"%s\n","        security = share");
		fprintf(fp,"%s\n","        # encrypt passwords = true");
		fprintf(fp,"%s\n","        # passdb backend = smbpasswd");
		fprintf(fp,"%s\n","        # usershare allow guests = no");
	}else{	//security
		fprintf(fp,"%s\n","        syslog = 1");
		fprintf(fp,"%s\n","        security = user");
		fprintf(fp,"%s\n","        encrypt passwords = true");
#ifdef CONFIG_USER_SMBPASSWD
		fprintf(fp,"%s\n","        passdb backend = smbpasswd");
		fprintf(fp,"%s\n","        smb passwd file = /var/samba/smbpasswd");
#endif
		fprintf(fp,"%s\n","        usershare allow guests = no");
		fprintf(fp,"%s\n","        map to guest = Bad User");
	}

	fprintf(fp,"\n");	//finish part 1: write global config

	/** part2: write share folder config
	 ** job1: write smb.conf
	 ** job2: fillin smbpasswd file
	**/
	fprintf(fp,"%s\n","[mnt]");
	fprintf(fp,"%s\n","        comment = realtek");
	fprintf(fp,"%s\n","        path = /mnt");
	fprintf(fp,"%s\n","        writable = yes");
	fprintf(fp,"%s\n","        printable = no");

	if(!sec_enable){	//share
		fprintf(fp,"%s\n","        public = yes");
		fprintf(fp,"%s\n","        guest ok = yes");
	}else{	//security
		char add_samba_user_cmd[1024] = "        valid users =";
		char tmpname[MAX_NAME_LEN];

#ifdef ACCOUNT_CONFIG
		totalEntry = mib_chain_total(MIB_ACCOUNT_CONFIG_TBL); /* get chain record size */
		for (i=0; i<totalEntry; i++) {
			if (!mib_chain_get(MIB_ACCOUNT_CONFIG_TBL, i, (void *)&entry)) {
				printf("ERROR: Get account configuration information from MIB database failed.\n");
				goto exit_func;
			}

			sprintf(tmpname, " %s", (char*)entry.userName);
			strncat(add_samba_user_cmd, tmpname, strlen(tmpname));
		}
#endif
		mib_get( MIB_SUSER_NAME, (void *)userName );
		sprintf(tmpname, " %s", userName);
		strncat(add_samba_user_cmd, tmpname, strlen(tmpname));

#ifndef CONFIG_00R0
		// Added by Mason Yu for others user
		mib_get( MIB_SUPER_NAME, (void *)userName );
		sprintf(tmpname, " %s", userName);
		strncat(add_samba_user_cmd, tmpname, strlen(tmpname));
#endif

		mib_get( MIB_USER_NAME, (void *)userName );
		if (userName[0]) {
			sprintf(tmpname, " %s", userName);
			strncat(add_samba_user_cmd, tmpname, strlen(tmpname));
		}

		fprintf(fp,"%s\n",add_samba_user_cmd);
	}	

	// finish part2: write share folder config
exit_func:
	fclose(fp);
	return 0;
}

int initSamba(void)
{
	//char cmd1[512];
	unsigned char smb_security_enable;

#if 0
	sprintf(cmd1,"echo \"telecomadmin:$1$$AwK5ysgOLgsgOriKA.7a./:0:0::/tmp:/bin/sh\" >> /var/passwd.smb");
	system(cmd1);
	sprintf(cmd1,"echo \"adsl:$1$$m9g7v7tSyWPyjvelclu6D1:0:0::/tmp:/bin/sh\" >> /var/passwd.smb");
	system(cmd1);
#endif

	/* it don't need group now */
	//sprintf(cmd1,"touch /var/group");
	//system(cmd1);

	//create samba config file
	mib_get(MIB_SAMBA_SECURITY_ENABLE, &smb_security_enable);
	updateSambaCfg(smb_security_enable);

	/**	init samba password, it should call after create samba config file,
	 **	because pdbedit will check smb.conf file
	 **/
	initSmbPwd();

	return 0;
}
#endif

int startSamba(void)
{
	unsigned char samba_enable;
	char *argv[10],
#ifdef CONFIG_USER_NMBD
	     nbn[MAX_SAMBA_NETBIOS_NAME_LEN], nbn_opt[MAX_SAMBA_NETBIOS_NAME_LEN + 32],
#endif
	     ss[MAX_SAMBA_SERVER_STRING_LEN], ss_opt[MAX_SAMBA_SERVER_STRING_LEN + 32];
	int i = -1, ret = 0;

	mib_get(MIB_SAMBA_ENABLE, &samba_enable);
	if (!samba_enable) {
		return -1;
	}

#ifdef CONFIG_MULTI_SMBD_ACCOUNT
	initSamba();
#endif

#ifdef CONFIG_USER_NMBD
	mib_get(MIB_SAMBA_NETBIOS_NAME, nbn);
#endif
	mib_get(MIB_SAMBA_SERVER_STRING, ss);

	/* set NetBIOS Name and Server String via command line arguments */
#ifdef CONFIG_USER_NMBD
	sprintf(nbn_opt, "--option=netbios name=%s", nbn);
#endif
	sprintf(ss_opt, "--option=server string=%s", ss);

	argv[++i] = "/bin/nmbd";
#ifdef CONFIG_USER_NMBD
	argv[++i] = nbn_opt;
#endif
	argv[++i] = ss_opt;
	argv[++i] = NULL;

#ifdef CONFIG_USER_NMBD
	ret = do_cmd(argv[0], argv, 0);
#endif

	argv[0] = "/bin/smbd";
	ret |= do_cmd(argv[0], argv, 0);

	return ret;
}

int stopSamba(void)
{
	pid_t pid;

#ifdef CONFIG_USER_NMBD
	pid = read_pid(NMBDPID);
	if (pid > 0) {
		/* nmbd is running */
		kill(pid, 9);
		unlink(NMBDPID);
	}
#endif

	pid = read_pid(SMBDPID);
	if (pid > 0) {
		/* smbd is running */
		/* using -pid would kill all processes whose process group ID is pid */
		kill(-pid, 9);
		unlink(SMBDPID);
	}

	return 0;
}
#endif // CONFIG_USER_SAMBA

#ifdef CONFIG_USER_CUPS
int getPrinterList(char *str, size_t size)
{
#if defined(CONFIG_USER_CUPS_1_4_6)
	char strbuf[BUF_SIZE], serverIP[INET_ADDRSTRLEN], lpName[BUF_SIZE];
	FILE *fp;
	int offset;
	struct in_addr inAddr;

	if (getInAddr((char *)LANIF, IP_ADDR, &inAddr) == 1) {
		strncpy(serverIP, inet_ntoa(inAddr), sizeof(serverIP));
	} else {
		getMIB2Str(MIB_ADSL_LAN_IP, serverIP);
	}

	offset = 0;

	sprintf(strbuf, "lpstat -a > /tmp/printer.tmp");
	va_cmd("/bin/sh", 2, 1, "-c", strbuf);
	if ((fp = fopen("/tmp/printer.tmp", "r")) <= 0) {
		return 0;
	}
	
	while (fgets(strbuf, sizeof(strbuf), fp)) {
		/* If no printer online or no printer added, result should be:
		 * lpstat: No destinations added.
		 * lpstat: Connection refused
		 */
		if (!strncmp(strbuf, "lpstat", 6))
			break;

		/* escape blank line */
		if ((strbuf[0] == 0x0d) || (strbuf[0] == 0x0a))
			continue;

		/* lp0 accepting requests since ... */
		sscanf(strbuf, "%s %*s", lpName);
		
		offset += snprintf(str + offset, size - offset,
					"http://%s:631/printers/%s\n",
					serverIP, lpName);
	}

	fclose(fp);
	unlink("/tmp/printer.tmp");

	return offset;
#else
	char strbuf[BUF_SIZE], serverIP[INET_ADDRSTRLEN], *substr, *chr;
	FILE *fp;
	int offset;
	struct in_addr inAddr;

	if (getInAddr((char *)LANIF, IP_ADDR, &inAddr) == 1) {
		strncpy(serverIP, inet_ntoa(inAddr), sizeof(serverIP));
	} else {
		getMIB2Str(MIB_ADSL_LAN_IP, serverIP);
	}

	offset = 0;
	fp = fopen(CUPSDPRINTERCONF, "r");

	while (fgets(strbuf, sizeof(strbuf), fp)) {
		chr = strchr(strbuf, '#');

		/* search the pattern '<DefaultPrinter lp0>' or '<Printer lp0>' */
		if (substr = strstr(strbuf, "Printer ")) {
			if (chr && chr < substr) {
				/* in comment */
				continue;
			}

			/*
			 * the length of "Printer " is 8,
			 * now substr points to the start of the printer name
			 */
			substr += 8;

			if (chr = strchr(substr, '>'))
				*chr = '\0';

			offset += snprintf(str + offset, size - offset,
					"http://%s:631/printers/%s\n",
					serverIP, substr);
		}
	}

	fclose(fp);

	return offset;
#endif
}
#endif // CONFIG_USER_CUPS

#ifdef CONFIG_USER_MINIDLNA
// success: return 1;
// fail: return 0;
int get_dlna_db_dir(char *db_dir)
{
	FILE *fp;
	char buf[256]="";
	int ret=0;

	fp = fopen("/proc/mounts", "r");
	if (fp) {
		while (fgets(buf, sizeof(buf), fp)) {
			if (strstr(buf, "/dev/sd")) {
				sscanf(buf,"%*s %s", db_dir);
				sprintf(db_dir, "%s/minidlna", db_dir);
				//warn("get_db_dir: db_dir=%s\n", db_dir);
				// Format is : /dev/sdb /var/mnt/sdb vfat .....................
				ret = 1;
				break;
			}
		}
		fclose(fp);
	}

	return ret;
}

static void createMiniDLNAconf(char *name, char *directory)
{
	FILE *fp;

	fp = fopen(name, "w");
	if(fp) {
		fputs("port=8200\n",fp);
		fputs("network_interface=br0\n", fp);
		fprintf(fp, "media_dir=/mnt\n");
		fprintf(fp, "db_dir=%s\n",directory);
		fputs("friendly_name=My DLNA Server\n", fp);
		fputs("album_art_names=Cover.jpg/cover.jpg/AlbumArtSmall.jpg/albumartsmall.jpg/AlbumArt.jpg/albumart.jpg/Album.jpg/album.jpg/Folder.jpg/folder.jpg/Thumb.jpg/thumb.jpg\n", fp);
		fputs("inotify=yes\n", fp);
		fputs("enable_tivo=no\n", fp);
		fputs("strict_dlna=no\n", fp);
		fputs("notify_interval=10\n", fp);	// 900
		fputs("serial=12345678\n", fp);
		fputs("model_number=1\n", fp);
		fclose(fp);
	}
	return;
}

void startMiniDLNA(void)
{
	char *argv[10];
	int i = 0, pid = 0;
	//MIB_DMS_T entry,*p;
	unsigned int enable;
	char db_dir[32]="";

	if(read_pid((char *)MINIDLNAPID)>0){
		printf( "%s: already start, line=%d\n", __FUNCTION__, __LINE__ );
		return;
	}

	if (!get_dlna_db_dir(db_dir))
		return;;

	// Mason Yu. use table not chain
	mib_get(MIB_DMS_ENABLE, (void *)&enable);
	if(!enable) {
		printf( "%s: is diaabled, line=%d\n", __FUNCTION__, __LINE__ );
		return;
	}
	createMiniDLNAconf("/var/minidlna.conf", db_dir);

	argv[i++]="/bin/minidlna";
//	argv[i++]="-d";
	argv[i++]="-R";
	argv[i++]="-f";
	argv[i++]="/var/minidlna.conf";
	argv[i]=NULL;
	do_cmd( argv[0], argv, 0 );

	while(read_pid((char*)MINIDLNAPID) < 0)
	{
		//printf("MINIDLNA is not running. Please wait!\n");
		usleep(300000);
	}
}

void stopMiniDLNA(void)
{
	int i, pid = 0;
	pid = read_pid((char *)MINIDLNAPID);
	if(pid <= 0){
		printf( "%s: already stop, line=%d\n", __FUNCTION__, __LINE__ );
		return;
	}

	//killpg(pid, 9);
	kill(pid, 15);
}
#endif

#ifdef CONFIG_IPV6
#ifdef CONFIG_USER_RADVD

// Added by Mason Yu for p2r_test
void init_radvd_conf_mib(void)
{
	unsigned char vChar;

	// MaxRtrAdvIntervalAct
	mib_set(MIB_V6_MAXRTRADVINTERVAL, "");

	// MinRtrAdvIntervalAct
	mib_set(MIB_V6_MINRTRADVINTERVAL, "");

	// AdvCurHopLimitAct
	mib_set(MIB_V6_ADVCURHOPLIMIT, "");

	// AdvDefaultLifetime
	mib_set(MIB_V6_ADVDEFAULTLIFETIME, "");

	// AdvReachableTime
	mib_set(MIB_V6_ADVREACHABLETIME, "");

	// AdvRetransTimer
	mib_set(MIB_V6_ADVRETRANSTIMER, "");

	// AdvLinkMTU
	mib_set(MIB_V6_ADVLINKMTU, "");

	// AdvSendAdvert
	vChar = 1;
	mib_set( MIB_V6_SENDADVERT, (void *)&vChar);      // on

	// AdvManagedFlag
	vChar = 2;
	mib_set( MIB_V6_MANAGEDFLAG, (void *)&vChar );     // ignore

	// AdvOtherConfigFlag
	vChar = 2;
	mib_set( MIB_V6_OTHERCONFIGFLAG, (void *)&vChar ); // ignore

    // RDNSS
	mib_set(MIB_V6_RDNSS1, "");
	mib_set(MIB_V6_RDNSS2, "");

	// Prefix
	vChar = 1;
	mib_set( MIB_V6_PREFIX_MODE, &vChar);

	mib_set(MIB_V6_PREFIX_IP, "3ffe:501:ffff:100::");
	mib_set(MIB_V6_PREFIX_LEN, "64");

	// AdvValidLifetime
	mib_set(MIB_V6_VALIDLIFETIME, "2592000");

	// AdvPreferredLifetime
	mib_set(MIB_V6_PREFERREDLIFETIME, "604800");

	// AdvOnLink
	vChar = 2;
	mib_set( MIB_V6_ONLINK, (void *)&vChar );    // ignore

	// AdvAutonomous
	vChar = 2;
	mib_set( MIB_V6_AUTONOMOUS, (void *)&vChar ); // ignore
}

int setup_radvd_conf()
{
	DNS_V6_INFO_T dnsV6Info={0};
	PREFIX_V6_INFO_T prefixInfo={0};
	FILE *fp;
	unsigned char str[MAX_RADVD_CONF_PREFIX_LEN];
	unsigned char str2[MAX_RADVD_CONF_PREFIX_LEN];
	unsigned char vChar,vChar2;

	get_dnsv6_info(&dnsV6Info);
	get_prefixv6_info(&prefixInfo);

	if ((fp = fopen(RADVD_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", RADVD_CONF);
		return -1;
	}

	fprintf(fp, "interface br0\n");
	fprintf(fp, "{\n");

	// AdvSendAdvert
	if ( !mib_get( MIB_V6_SENDADVERT, (void *)&vChar) )
		printf("Get MIB_V6_SENDADVERT error!");
	if (0 == vChar)
		fprintf(fp, "\tAdvSendAdvert off;\n");
	else if (1 == vChar)
		fprintf(fp, "\tAdvSendAdvert on;\n");

	// MaxRtrAdvIntervalAct
	if ( !mib_get(MIB_V6_MAXRTRADVINTERVAL, (void *)str)) {
		printf("Get MaxRtrAdvIntervalAct mib error!");
	}
	if (str[0]) {
		fprintf(fp, "\tMaxRtrAdvInterval %s;\n", str);
	}

	// MinRtrAdvIntervalAct
	if ( !mib_get(MIB_V6_MINRTRADVINTERVAL, (void *)str)) {
		printf("Get MinRtrAdvIntervalAct mib error!");
	}
	if (str[0]) {
		fprintf(fp, "\tMinRtrAdvInterval %s;\n", str);
	}

	// AdvCurHopLimitAct
	if ( !mib_get(MIB_V6_ADVCURHOPLIMIT, (void *)str)) {
		printf("Get AdvCurHopLimitAct mib error!");
	}
	if (str[0]) {
		fprintf(fp, "\tAdvCurHopLimit %s;\n", str);
	}

	// AdvDefaultLifetime
	if ( !mib_get(MIB_V6_ADVDEFAULTLIFETIME, (void *)str)) {
		printf("Get AdvDefaultLifetime mib error!");
	}
	if (str[0]) {
		fprintf(fp, "\tAdvDefaultLifetime %s;\n", str);
	}

	// AdvReachableTime
	if ( !mib_get(MIB_V6_ADVREACHABLETIME, (void *)str)) {
		printf("Get AdvReachableTime mib error!");
	}
	if (str[0]) {
		fprintf(fp, "\tAdvReachableTime %s;\n", str);
	}

	// AdvRetransTimer
	if ( !mib_get(MIB_V6_ADVRETRANSTIMER, (void *)str)) {
		printf("Get AdvRetransTimer mib error!");
	}
	if (str[0]) {
		fprintf(fp, "\tAdvRetransTimer %s;\n", str);
	}

	// AdvLinkMTU
	if ( !mib_get(MIB_V6_ADVLINKMTU, (void *)str)) {
		printf("Get AdvLinkMTU mib error!");
	}

	if (str[0]) {
		fprintf(fp, "\tAdvLinkMTU %s;\n", str);
	}

	// AdvManagedFlag
	if ( !mib_get( MIB_V6_MANAGEDFLAG, (void *)&vChar) )
		printf("Get MIB_V6_MANAGEDFLAG error!");
	if (0 == vChar)
		fprintf(fp, "\tAdvManagedFlag off;\n");
	else if (1 == vChar)
		fprintf(fp, "\tAdvManagedFlag on;\n");

	// AdvOtherConfigFlag
	if ( !mib_get( MIB_V6_OTHERCONFIGFLAG, (void *)&vChar) )
		printf("Get MIB_V6_OTHERCONFIGFLAG error!");
	if (0 == vChar)
		fprintf(fp, "\tAdvOtherConfigFlag off;\n");
	else if (1 == vChar)
		fprintf(fp, "\tAdvOtherConfigFlag on;\n");

	//NOTE: in radvd.conf
	//      Prefix/clients/route/RDNSS configurations must be given in exactly this order.

	// ULA Prefix
	mib_get (MIB_V6_ULAPREFIX_ENABLE, (void *)&vChar);
	if (vChar!=0) {
		unsigned char validtime[MAX_RADVD_CONF_PREFIX_LEN];
		unsigned char preferedtime[MAX_RADVD_CONF_PREFIX_LEN];

		if ( !mib_get(MIB_V6_ULAPREFIX, (void *)str)       ||
				!mib_get(MIB_V6_ULAPREFIX_LEN, (void *)str2)  ||
				!mib_get(MIB_V6_ULAPREFIX_VALID_TIME, (void *)validtime)  ||
				!mib_get(MIB_V6_ULAPREFIX_PREFER_TIME, (void *)preferedtime)
		   )
		{
			printf("Get ULAPREFIX mib error!");
		}
		else
		{
			unsigned char ip6Addr[IP6_ADDR_LEN];
			unsigned char devAddr[MAC_ADDR_LEN];
			unsigned char meui64[8];
			unsigned char value[64];
			int i;
#ifdef CONFIG_RTK_L34_ENABLE
			struct ipv6_ifaddr ip6_addr[6];
			char cur_ip6_addr[64];
#endif

			fprintf(fp, "\t\n");
			fprintf(fp, "\tprefix %s/%s\n", str, str2);
			fprintf(fp, "\t{\n");
			fprintf(fp, "\t\tAdvOnLink on;\n");
			fprintf(fp, "\t\tAdvAutonomous on;\n");
			fprintf(fp, "\t\tAdvValidLifetime %s;\n",validtime);
			fprintf(fp, "\t\tAdvPreferredLifetime %s;\n",preferedtime);
			fprintf(fp, "\t};\n");

#ifdef CONFIG_RTK_L34_ENABLE
			mib_get(MIB_LAN_IPV6_MODE1, (void *)&vChar);
			if(vChar == 0 ) // LAN IPv6 address mode is auto.
			{
#endif
				inet_pton(PF_INET6, str, (void *)ip6Addr);

				//setup LAN ULA v6 IP address according the ULA prefix + EUI64.
				mib_get(MIB_ELAN_MAC_ADDR, (void *)devAddr);
				mac_meui64(devAddr, meui64);
				for (i=0; i<8; i++)
					ip6Addr[i+8] = meui64[i];
				inet_ntop(PF_INET6, &ip6Addr, value, sizeof(value));
				sprintf(value, "%s/%s", value, str2);
				printf("Set LAN ULA %s\n",value);
#ifdef CONFIG_RTK_L34_ENABLE
			}
			else
			{
				mib_get(MIB_LAN_IPV6_ADDR1, (void *)ip6Addr);
				inet_ntop(PF_INET6, ip6Addr, value, sizeof(value));
				mib_get(MIB_LAN_IPV6_PREFIX_LEN1, (void *)&vChar2);
				sprintf(value, "%s/%d", value, vChar2);
			}

			getifip6(LANIF, IPV6_ADDR_UNICAST, ip6_addr, 6);
			inet_ntop(PF_INET6, &ip6_addr[0].addr, cur_ip6_addr, sizeof(cur_ip6_addr));
			sprintf(cur_ip6_addr, "%s/%d", cur_ip6_addr, ip6_addr[0].prefix_len);

			va_cmd(IFCONFIG, 3, 1, LANIF, "del", cur_ip6_addr);
			va_cmd(IFCONFIG, 3, 1, LANIF, "add", value);
#else
			va_cmd(IFCONFIG, 3, 1, LANIF, "del", value);
			va_cmd(IFCONFIG, 3, 1, LANIF, "add", value);
#endif
		}
	}

	// Prefix
	if(prefixInfo.prefixIP[0] && prefixInfo.prefixLen)
	{
		struct  in6_addr ip6Addr;
		unsigned char devAddr[MAC_ADDR_LEN];
		unsigned char meui64[8];
		unsigned char value[64];
		unsigned char prefixBuf[100]={0};
		int i;

		inet_ntop(PF_INET6,prefixInfo.prefixIP, prefixBuf, sizeof(prefixBuf));
		fprintf(fp, "\t\n");
		fprintf(fp, "\tprefix %s/%d\n", prefixBuf, prefixInfo.prefixLen);
		fprintf(fp, "\t{\n");

		memcpy(ip6Addr.s6_addr,prefixInfo.prefixIP,8);
		mib_get(MIB_ELAN_MAC_ADDR, (void *)devAddr);
		mac_meui64(devAddr, meui64);
		for (i=0; i<8; i++)
			ip6Addr.s6_addr[i+8] = meui64[i];
		inet_ntop(PF_INET6, &ip6Addr, value, sizeof(value));
		sprintf(value, "%s/%d", value, prefixInfo.prefixLen);
		va_cmd(IFCONFIG, 3, 1, LANIF, "del", value);
		va_cmd(IFCONFIG, 3, 1, LANIF, "add", value);

		// AdvOnLink
		if ( !mib_get( MIB_V6_ONLINK, (void *)&vChar) )
			printf("Get MIB_V6_ONLINK error!");
		if (0 == vChar)
			fprintf(fp, "\t\tAdvOnLink off;\n");
		else if (1 == vChar)
			fprintf(fp, "\t\tAdvOnLink on;\n");

		// AdvAutonomous
		if ( !mib_get( MIB_V6_AUTONOMOUS, (void *)&vChar) )
			printf("Get MIB_V6_AUTONOMOUS error!");
		if (0 == vChar)
			fprintf(fp, "\t\tAdvAutonomous off;\n");
		else if (1 == vChar)
			fprintf(fp, "\t\tAdvAutonomous on;\n");

		// AdvValidLifetime
		fprintf(fp, "\t\tAdvValidLifetime %u;\n", prefixInfo.MLTime);			

		// AdvPreferredLifetime
		fprintf(fp, "\t\tAdvPreferredLifetime %u;\n", prefixInfo.PLTime);

		fprintf(fp, "\t\tDeprecatePrefix on;\n");
		fprintf(fp, "\t};\n");
	}

	//set RDNSS according to DNSv6 server setting
	if(strlen(dnsV6Info.nameServer)){
		char *ptr=NULL;
		unsigned char nameServer[IPV6_BUF_SIZE_256];

		memcpy(nameServer,dnsV6Info.nameServer,sizeof(nameServer));

		// Alan, Modify all ',' in string for RADVD CONF format
		//  Replace ',' in string to meet RADVD CONF format
		//	RDNSS ip [ip] [ip] {    list of rdnss specific options
		//	};
		ptr=nameServer;
		while(ptr=strchr(ptr,',')){
			*ptr++=' ';
		}

		fprintf(fp, "\n\tRDNSS %s\n", nameServer);
		fprintf(fp, "\t{\n");
		fprintf(fp, "\t\tAdvRDNSSPreference 8;\n");
		fprintf(fp, "\t\tAdvRDNSSOpen off;\n");
		fprintf(fp, "\t};\n");
	}

	fprintf(fp, "};\n");
	fclose(fp);
	return 0;

}

#endif // of CONFIG_USER_RADVD
#endif // of CONFIG_IPV6

#ifdef CONFIG_USER_DOT1AG_UTILS
//Remvoe entries that WAN interface is not existed anymore.
void arrange_dot1ag_table()
{
	int entryNum = mib_chain_total(MIB_DOT1AG_TBL);
	MIB_CE_DOT1AG_T Entry = {0};
	MIB_CE_ATM_VC_T vc_entry = {0};
	int i;
	char modified = 0;

	for(i = entryNum - 1; i >= 0; i--)
	{
		mib_chain_get(MIB_DOT1AG_TBL, i, (void *)&Entry);

		if(getATMVCEntryByIfIndex(Entry.ifIndex, &vc_entry) == NULL)
		{
			//WAN ifIndex is modified or changed
			mib_chain_delete(MIB_DOT1AG_TBL, i);
			modified = 1;
		}
	}

	if(modified)
		mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
}

static int filter_dot1ag(const struct dirent *dir)
{
	int ret1, ret2;

	if( fnmatch("dot1agd.pid.*", dir->d_name, 0) == 0
		||  fnmatch("dot1ag_ccd.pid.*", dir->d_name, 0) == 0)
		return 1;
	else
		return 0;
}

void stopDot1ag()
{
	struct dirent **list;
	int n = 0;

	n = scandir("/var/run", &list, filter_dot1ag, NULL);

	if(n > 0)
	{
		while(n--)
		{
			int pid;
			FILE *file;
			char fname[128] = {0};

			sprintf(fname, "/var/run/%s", list[n]->d_name);

			pid = read_pid(fname);
			if(pid <= 0)
			{
				fprintf(stderr, "Read PID file failed from: %s.\n", list[n]->d_name);
				fclose(file);
				free(list[n]);
				continue;
			}

			kill(pid, SIGTERM);

			free(list[n]);
		}
		free(list);
		list = NULL;
	}
}

void startDot1ag()
{
	MIB_CE_DOT1AG_T entry = {0};
	int total;
	int i;
	char arg[60] = {0};

	arrange_dot1ag_table();
	stopDot1ag();

	total = mib_chain_total(MIB_DOT1AG_TBL);
	for(i = 0 ; i < total ; i++)
	{
		char *argv[20] = {0};
		int idx = 1;
		char ifname[IFNAMSIZ] = {0};

		mib_chain_get(MIB_DOT1AG_TBL, i, &entry);

		ifGetName(entry.ifIndex, ifname, IFNAMSIZ);

		argv[idx++] = "-i";
		argv[idx++] = ifname;

		do_cmd("/bin/dot1agd", argv, 0);

		if(entry.ccm_enable)
		{
			char str_interval[20] = {0};
			char str_level[4] = {0};
			char str_mep_id[8] = {0};

			argv[idx++] = "-t";
			argv[idx++] = str_interval;
			sprintf(str_interval, "%u", entry.ccm_interval);

			argv[idx++] = "-d";
			argv[idx++] = entry.md_name;

			argv[idx++] = "-l";
			argv[idx++] = str_level;
			sprintf(str_level, "%u", entry.md_level);

			argv[idx++] = "-a";
			argv[idx++] = entry.ma_name;

			argv[idx++] = "-m";
			argv[idx++] = str_mep_id;
			sprintf(str_mep_id, "%u", entry.mep_id);

			do_cmd("/bin/dot1ag_ccd", argv, 0);
		}
	}
}
#endif


#ifdef _CWMP_MIB_
#define NOT_IPV6_IP 1
#define IPV6_IP 2
int set_endpoint(char *newurl, char *url) //star: remove "http://" from url string
{
	register const char *s;
	register size_t i, n;
	newurl[0] = '\0';
	int urltype = NOT_IPV6_IP;
	int isIP=0;
	struct in_addr ina;
	struct in6_addr ina6;
	
	if (!url || !*url)
		return;
	s = strchr(url, ':');
	if (s && s[1] == '/' && s[2] == '/' && s[3] != '[')
		s += 3;
	else if (s && s[1] == '/' && s[2] == '/' && s[3] == '[') {
		s += 4;
		urltype = IPV6_IP;
	}
	else
		s = url;
	
	n = strlen(s);
	if(n>256)
		n=256;

	for (i = 0; i < n; i++)
	{ 
		newurl[i] = s[i];
		// NOT_IPV6_IP
		if (urltype == NOT_IPV6_IP) {
			if (s[i] == '/' || s[i] == ':') 
				break;
		}
	  
		// IPV6_IP
		if (urltype == IPV6_IP) {
			if (s[i] == ']') {
				//printf("This is IPv6 IP format\n");
				break;
			}
		}
	}
	
	newurl[i] = '\0';	
	if(inet_pton(AF_INET6, newurl, &ina6) > 0 || inet_pton(AF_INET, newurl, &ina) > 0)
		isIP = 1;
	
	//printf("newurl=%s, isIP=%d\n", newurl, isIP);
	return isIP;
}

#if defined(IP_QOS) || defined(NEW_IP_QOS_SUPPORT)
/*star:20100305 START add qos rule to set tr069 packets to the first priority queue*/
void setQosfortr069(int mode, char *urlvalue)
{
	char vStr[256+1];
	char acsurl[256+1];
	struct hostent *host;
	struct in_addr acsaddr;
	char classId[]="0x00:0x00";
	char* argv[15];
	char *operationMode[2]={"add","del"};
	char *iptablesopt[2]={(char *)FW_ADD,(char *)FW_DEL};

	{
		set_endpoint(acsurl,urlvalue);
		host=gethostbyname(acsurl);
		if(host==NULL)
			return;
		memcpy((char *) &(acsaddr.s_addr), host->h_addr_list[0], host->h_length);

		{
			char interfaceName[8]={0};
			char DstIp[20]={0};
			int k;
			//iptables -t mangle -A OUTPUT -d 172.21.146.44 -p tcp -j CLASSIFY --set-class 0x00:0x06
			argv[1]="-t";
			argv[2]="mangle";
			argv[3]=iptablesopt[mode];
			argv[4]="OUTPUT";
			argv[5]="-d";
			strcpy(DstIp,inet_ntoa(acsaddr));
			argv[6]=DstIp;
			argv[7]="-p";
			argv[8]="tcp";
			argv[9]="-j";
			argv[10]="CLASSIFY";
			argv[11]="--set-class";
			for (k=0; k<8; k++) {
				if (priomap[k] == 1) {
					classId[8]+=k;
					break;
				}
			}
			argv[12]=classId;
			argv[13]=NULL;
			printf("\niptables");
			int i;
			for(i=1;i<12;i++){
				printf(" %s",argv[i]);
			}
			printf("\n");
			do_cmd("/bin/iptables",argv,1);

			//telefonica requests to add this rule to chain "INPUT" and "FORWARD", for possible tr069 Device in Lan side
			//Magician: Setting this rule in INPUT is useless, this is just for Telefonica's demand.
			//argv[4]="INPUT";
			//do_cmd("/bin/iptables",argv,1);

			argv[4]="FORWARD";
			do_cmd("/bin/iptables",argv,1);
		}
	}
	return;

}

static int SetcwmpQosflag = 0;

void setTr069QosFlag(int var)
{
	SetcwmpQosflag = var;
}

int getTr069QosFlag(void)
{
	return SetcwmpQosflag;
}
#endif

static const char OLDACSFILE[] = "/var/oldacs";
static const char OLDACSFILE_FOR_WAN[] = "/var/oldacs_wan";  // Magician: Used for TR-069 WAN interface.

void storeOldACS(void)
{
	FILE* fp;
	char acsurl[256+1]={0};

#if defined(CONFIG_00R0) && defined(CONFIG_TR142_MODULE) && !defined(CONFIG_E8B)
	mib_get(RS_CWMP_USED_ACS_URL, acsurl);
#else
	mib_get(CWMP_ACS_URL, acsurl);
#endif
	if(strlen(acsurl))
	{
		if(fp = fopen(OLDACSFILE, "w"))
		{
			fprintf(fp, "%s",acsurl);
			fclose(fp);
		}

		if(fp = fopen(OLDACSFILE_FOR_WAN, "w"))
		{
			fprintf(fp, "%s",acsurl);
			fclose(fp);
		}
	}
}

int getOldACS(char *acsurl)
{
	FILE* fp;

	acsurl[0]=0;
	fp=fopen(OLDACSFILE,"r");
	if(fp){
		fscanf(fp,"%s",acsurl);
		fclose(fp);
		unlink(OLDACSFILE);
	}
	if(strlen(acsurl))
		return 1;
	else
		return 0;
}
/*star:20100305 END*/

int getOldACSforWAN(char *acsurl)
{
	FILE* fp;

	acsurl[0]=0;
	fp=fopen(OLDACSFILE_FOR_WAN,"r");
	if(fp){
		fscanf(fp,"%s",acsurl);
		fclose(fp);
		unlink(OLDACSFILE_FOR_WAN);
	}
	if(strlen(acsurl))
		return 1;
	else
		return 0;
}
#endif

char *trim_white_space(char *str)
{
	char *end;

	// Trim leading space
	while (isspace(*str)) str++;

	if(*str == 0)  // All spaces?
		return str;

	// Trim trailing space
	end = str + strlen(str) - 1;
	while (end >= str && isspace(*end)) end--;

	// Write new null terminator
	*(end+1) = 0;

	return str;
}

// Magician: This function is for checking the validation of whole config file.
/*
 *	Check the validation of config file.
 *	Return:
 *		0 for failed or Config file type (CONFIG_DATA_T) if successful.
 */
int checkConfigFile(const char *config_file)
{
	FILE *fp;
	char strbuf[1024], *pbuf, *pstr;
	int linenum = 1, len;
	char inChain = 0, isEnd = 0;
	int chainLevel=0;
	int status;

	if(!(fp = fopen(config_file, "r")))
	{
		printf("Open config file failed: %s\n", strerror(errno));
		return 0;
	}

	len = strlen(fgets(strbuf, 1024, fp));
	if(strbuf[len-1] != '\n')
	{
		printf("Miss a newline at line %d!\n", linenum);
		fclose(fp);
		return 0;
	}

	if(strbuf[len-2] == '\r' && strbuf[len-1] == '\n') // Remove the CRLF(0x0D0A) which is at the last of a line.
		strbuf[len-2] = 0;
	else if(strbuf[len-1] == '\n') // Remove the LF(0x0A) which is at the last of a line.
		strbuf[len-1] = 0;

	pbuf = trim_white_space(strbuf);
	if(!strcmp(pbuf, CONFIG_HEADER))
		status = CURRENT_SETTING;
	else if(!strcmp(pbuf, CONFIG_HEADER_HS))
		status = HW_SETTING;
	else
	{
		printf("Invalid header: %s\n", strbuf);
		fclose(fp);
		return 0;
	}

	while(fgets(strbuf, 1024, fp))
	{
		linenum++;  // The counter of current line on handling.
		len = strlen(strbuf);
		//printf("%d: %d: %s", linenum, len, strbuf);

		if(strbuf[len-1] != '\n')   // If this line does not end with a newline, return an error.
		{
			printf("Miss a newline at line %d!\n", linenum);
			status = 0;
			break;
		}

		if(strbuf[len-2] == '\r' && strbuf[len-1] == '\n') // Remove the CRLF(0x0D0A) which is at the last of a line.
			strbuf[len-2] = 0;
		else if(strbuf[len-1] == '\n') // Remove the LF(0x0A) which is at the last of a line.
			strbuf[len-1] = 0;

		if(isEnd)  // It should be at the end of this config file.
		{
			printf("Invalid end of config file at line %d\n", linenum);
			status = 0;
			break;
		}

		pbuf = trim_white_space(strbuf);
		if (strlen(pbuf)==0)
			continue;

		if(!strncmp(pbuf, "<Value Name=\"", 13))  // Check the validation of common <Value Name.... line.
		{
			if(!(pstr = strchr(pbuf+13, '\"')))
			{
				printf("Invalid format at line %d: %s\n", linenum, strbuf);
				status = 0;
				break;
			}

			pstr++;

			if(strncmp(pstr, " Value=\"", 8))
			{
				printf("Invalid format at line %d: %s\n", linenum, strbuf);
				status = 0;
				break;
			}

			if(!(pstr = strchr(pstr+8, '\"')))
			{
				printf("Invalid format at line %d: %s\n", linenum, strbuf);
				status = 0;
				break;
			}

			pstr++;

			if(strncmp(pstr, "/>", 2))
			{
				printf("Invalid format at line %d: %s\n", linenum, strbuf);
				status = 0;
				break;
			}

			pstr += 2;

			if(*pstr != '\0')
			{
				printf("Invalid format at line %d: %s\n", linenum, strbuf);
				status = 0;
				break;
			}

			continue;
		}

		if(!strncmp(pbuf, "<chain chainName=\"", 18))  // Enter in a chain table.
		{
			if(!(pstr = strchr(pbuf+18, '\"')))
			{
				printf("Invalid format at line %d: %s\n", linenum, strbuf);
				status = 0;
				break;
			}

			pstr++;

			if(*pstr != '>')
			{
				printf("Invalid format at line %d: %s\n", linenum, strbuf);
				status = 0;
				break;
			}

			pstr++;

			if(*pstr != '\0')
			{
				printf("Invalid format at line %d: %s\n", linenum, strbuf);
				status = 0;
				break;
			}

			chainLevel++;
			continue;
		}

		if(!strncmp(pbuf, "</chain>", 8))  // Leave from a chain table.
		{
			if (chainLevel <=0)
			{
				printf("Invalid structure at line %d: %s\n", linenum, strbuf);
				status = 0;
				break;
			}

			chainLevel--;
			continue;
		}

		if(!strcmp(pbuf, CONFIG_TRAILER) || !strcmp(pbuf, CONFIG_TRAILER_HS))  // Reach the end of config file.
		{
			isEnd = 1;
			continue;
		}

		printf("Unknown format at line %d: %s\n", linenum, strbuf);
		status = 0;
		break;
	}

	fclose(fp);
	return status;
}

// Magician: This function is used for memory changing watch. You can put it anywhere to detect if memory usage changes.
// Be sure you assign all these functions in THE SAME process.
#if DEBUG_MEMORY_CHANGE
char last_memsize[128], last_file[32], last_func[32]; // Use to indicate last position where you put ShowMemChange().
int last_line;  // Use to indicate last position where you put ShowMemChange().
int ShowMemChange(char *file, char *func, int line)
{
	FILE *fp, *logfp;
	char buf[128];
	int i;
	char isPntOnChg = 1;  // Print message when memory size changed.
	char isLog = 1;  // Log results in /tmp/memlog
	char status = 0;  // return 1 when memory usage does change or otherwise return 0.

	if(isLog && !(logfp = fopen("/tmp/memlog", "a")))
	{
		perror("/tmp/memlog");
		return -1;
	}

	sprintf(buf, "/proc/%d/status", getpid());

	if(fp = fopen(buf, "r"))
	{
		for( i = 0; i < 11; i++ )
			fgets(buf, 128, fp);

		fclose(fp);

		if(!isPntOnChg || strcmp(buf, last_memsize))
		{
			putchar('\n');

			if(isPntOnChg)
			{
				printf("===== Last Memory size info (%s:%s:%d) =====\n", last_file, last_func, last_line);
				printf(last_memsize);

				if(isLog)
				{
					fprintf(logfp, "===== Last Memory size info (%s:%s:%d) =====\n", last_file, last_func, last_line);
					fprintf(logfp, last_memsize);
				}
			}

			printf("===== Memory size info (%s:%s:%d) =====\n", file, func, line);
			printf(buf);

			if(isLog)
			{
				fprintf(logfp, "===== Memory size info (%s:%s:%d) =====\n", file, func, line);
				fprintf(logfp, "%s\n", buf);
			}

			status = 1;
		}

		strncpy(last_memsize, buf, 128);
		strncpy(last_file, file, 32);
		strncpy(last_func, func, 32);
		last_line = line;
	}

	fclose(logfp);
	return status;
}
#endif
//Kevin:Check whether to enable/disable upstream ip fastpath
void UpdateIpFastpathStatus(void)
{
    /* If any one of the folllowing functions is enabled,
           we have to disable upstream ip fastpath.
               (1) IP Qos
               (2) URL blocking
               (3) Domain blocking
           Otherwise, keep ip fastpath up/downstream both to enhance throughput.
       */

	unsigned char mode=0, fastpath_us_disabled = 0, fastbridge_disabled = 0;

#if defined(IP_QOS) && !defined(CONFIG_RTL_ADV_FAST_PATH) && (defined(CONFIG_RTL867X_IPTABLES_FAST_PATH) || defined(CONFIG_RTK_FASTPATH))
	mib_get(MIB_MPMODE, (void *)&mode);
	if (mode & MP_IPQ_MASK)
	{
		//printf("(%s)IP Qos V1!\n",__func__);
		system("/bin/echo 1 > /proc/FastPath");
		fastpath_us_disabled = 1;
	}
#endif

#ifdef CONFIG_USER_IP_QOS_3
	unsigned int qosEnable;
	unsigned int qosRuleNum, carRuleNum;
	unsigned char totalBandWidthEn;

	qosEnable = getQosEnable();
	qosRuleNum = getQosRuleNum();
	mib_get(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&totalBandWidthEn);
	carRuleNum = mib_chain_total(MIB_IP_QOS_TC_TBL);
	if ((qosEnable && qosRuleNum)) {
#if !defined(CONFIG_RTL_ADV_FAST_PATH) && (defined(CONFIG_RTL867X_IPTABLES_FAST_PATH) || defined(CONFIG_RTK_FASTPATH))
		system("/bin/echo 1 > /proc/FastPath");
		fastpath_us_disabled = 1;
#endif

#ifdef CONFIG_RTK_FASTBRIDGE
		system("/bin/echo 0 > /proc/realtek/fastbridge");
		fastbridge_disabled = 1;
#endif
#ifdef CONFIG_RTL8672_BRIDGE_FASTPATH
		system("/bin/echo 0 > /proc/fastbridge");
		fastbridge_disabled = 1;
#endif
	}
	else if (totalBandWidthEn || carRuleNum) {
#if defined(CONFIG_RTL867X_IPTABLES_FAST_PATH) || defined(CONFIG_RTK_FASTPATH)
		system("/bin/echo 1 > /proc/FastPath");
		fastpath_us_disabled = 1;
#endif
	}
#endif /* CONFIG_USER_IP_QOS_3 */

#if defined(CONFIG_RTL867X_IPTABLES_FAST_PATH) || defined(CONFIG_RTK_FASTPATH)
#if defined(URL_BLOCKING_SUPPORT) || defined(URL_ALLOWING_SUPPORT)
	mode=0;
	mib_get(MIB_URL_CAPABILITY, (void *)&mode);
	if (mode)
	{
		//printf("(%s)URL blocking!\n",__func__);
		system("/bin/echo 1 > /proc/FastPath");
		fastpath_us_disabled = 1;
	}
#endif

#ifdef DOMAIN_BLOCKING_SUPPORT
	mode=0;
	mib_get(MIB_DOMAINBLK_CAPABILITY, (void *)&mode);
	if (mode)
	{
		//printf("(%s)Domain blocking!\n",__func__);
		system("/bin/echo 1 > /proc/FastPath");
		fastpath_us_disabled = 1;
	}
#endif

	//printf("(%s)none!\n",__func__);
	if (fastpath_us_disabled == 0)
		system("/bin/echo 2 > /proc/FastPath");
#endif /* defined(CONFIG_RTL867X_IPTABLES_FAST_PATH) || defined(CONFIG_RTK_FASTPATH) */
	if (fastbridge_disabled == 0) {
#ifdef CONFIG_RTK_FASTBRIDGE
		system("/bin/echo 1 > /proc/realtek/fastbridge");
#endif
#ifdef CONFIG_RTL8672_BRIDGE_FASTPATH
		system("/bin/echo 1 > /proc/fastbridge");
#endif
	}
}

#ifdef RESERVE_KEY_SETTING
/*
 * flag: reserved for future use.
 */
static int reserve_critical_setting(int flag)
{
	//first backup current setting
	mib_backup(CONFIG_MIB_ALL);

	// restore current to default
#ifdef CONFIG_USER_XMLCONFIG
	va_cmd(shell_name, 2, 1, "/etc/scripts/config_xmlconfig.sh", "-d");
#else
	mib_sys_to_default(CURRENT_SETTING);
#endif
	//now retrieve the key parameters.
#if 0
#ifdef _CWMP_MIB_
	mib_retrive_table(CWMP_ACS_URL);
	mib_retrive_table(CWMP_ACS_USERNAME);
	mib_retrive_table(CWMP_ACS_PASSWORD);
#endif
	mib_retrive_table(MIB_BRCTL_AGEINGTIME);
	mib_retrive_table(MIB_BRCTL_STP);
	mib_retrive_chain(MIB_ATM_VC_TBL);
#ifdef ROUTING
	mib_retrive_chain(MIB_IP_ROUTE_TBL);
#endif
#endif
	mib_retrive_table(MIB_LOID);
	mib_retrive_table(MIB_LOID_PASSWD);
#if defined(CONFIG_GPON_FEATURE)
	mib_retrive_table(MIB_GPON_PLOAM_PASSWD);
#endif
#if defined(CONFIG_EPON_FEATURE)
	mib_retrive_chain(MIB_EPON_LLID_TBL);
#endif

	return 0;
}
#endif

static char *get_name(char *name, char *p)
{
	while (isspace(*p))
		p++;
	while (*p) {
		if (isspace(*p))
			break;
		if (*p == ':') {	/* could be an alias */
			char *dot = p, *dotname = name;
			*name++ = *p++;
			while (isdigit(*p))
				*name++ = *p++;
			if (*p != ':') {	/* it wasn't, backup */
				p = dot;
				name = dotname;
			}
			if (*p == '\0')
				return NULL;
			p++;
			break;
		}
		*name++ = *p++;
	}
	*name = '\0';

	return p;
}

static int procnetdev_version(char *buf)
{
	if (strstr(buf, "compressed"))
		return 2;
	if (strstr(buf, "bytes"))
		return 1;
	return 0;
}

static const char *const ss_fmt[] = {
	"%n%lu%lu%lu%lu%lu%n%n%n%lu%lu%lu%lu%lu%lu",
	"%lu%lu%lu%lu%lu%lu%n%n%lu%lu%lu%lu%lu%lu%lu",
	"%lu%lu%lu%lu%lu%lu%lu%lu%lu%lu%lu%lu%lu%lu%lu%lu"
};

static void get_dev_fields(char *bp, struct net_device_stats *nds, int procnetdev_vsn)
{
	memset(nds, 0, sizeof(*nds));

	sscanf(bp, ss_fmt[procnetdev_vsn],
		   &nds->rx_bytes, /* missing for 0 */
		   &nds->rx_packets,
		   &nds->rx_errors,
		   &nds->rx_dropped,
		   &nds->rx_fifo_errors,
		   &nds->rx_frame_errors,
		   &nds->rx_compressed, /* missing for <= 1 */
		   &nds->multicast, /* missing for <= 1 */
		   &nds->tx_bytes, /* missing for 0 */
		   &nds->tx_packets,
		   &nds->tx_errors,
		   &nds->tx_dropped,
		   &nds->tx_fifo_errors,
		   &nds->collisions,
		   &nds->tx_carrier_errors,
		   &nds->tx_compressed /* missing for <= 1 */
		   );

	if (procnetdev_vsn <= 1) {
		if (procnetdev_vsn == 0) {
			nds->rx_bytes = 0;
			nds->tx_bytes = 0;
		}
		nds->multicast = 0;
		nds->rx_compressed = 0;
		nds->tx_compressed = 0;
	}
}

/**
 * list_net_device_with_flags - list network devices with the specified flags
 * @flags: input argument, the network device flags
 * @nr_names: input argument, number of elements in @names
 * @names: output argument, constant pointer to the array of network device names
 *
 * Returns the number of resulted elements in @names for success
 * or negative errno values for failure.
 */
int list_net_device_with_flags(short flags, int nr_names,
				char (* const names)[IFNAMSIZ])
{
	FILE *fh;
	char buf[512];
	struct ifreq ifr;
	int nr_result, skfd;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (skfd < 0)
		goto out;

	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh)
		goto out_close_skfd;
	fgets(buf, sizeof(buf), fh);	/* eat line */
	fgets(buf, sizeof(buf), fh);

	nr_result = 0;
	while (fgets(buf, sizeof(buf), fh) && nr_result < nr_names) {
		char name[128];

		get_name(name, buf);

		strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
		if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
			goto out_close_fh;

		if (ifr.ifr_flags & flags) {
			strncpy(names[nr_result++], name, ARRAY_SIZE(names[0]));
		}
	}

	if (ferror(fh))
		goto out_close_fh;

	fclose(fh);
	close(skfd);

	return nr_result;

out_close_fh:
	fclose(fh);
out_close_skfd:
	close(skfd);
out:
	warn("%s():%d", __FUNCTION__, __LINE__);

	return -errno;
}

/**
 * get_net_device_stats - get the statistics of the specified network device
 * @ifname: input argument, constant pointer to the network device name
 * @nds: output argument, pointer to network device statistics
 *
 * Returns 0 if not found, 1 if found
 * or negative errno values for failure.
 *
 */
int get_net_device_stats(const char *ifname, struct net_device_stats *nds)
{
	FILE *fh;
	char buf[512];
	int procnetdev_vsn, found;

	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh)
		goto out;
	fgets(buf, sizeof(buf), fh);	/* eat line */
	fgets(buf, sizeof(buf), fh);

	procnetdev_vsn = procnetdev_version(buf);

	memset(nds, 0, sizeof(*nds));
	found = 0;
	while (fgets(buf, sizeof(buf), fh)) {
		char *s, name[128];
		int n;

		s = get_name(name, buf);

		if (!strcmp(name, ifname)) {
			get_dev_fields(s, nds, procnetdev_vsn);
			found = 1;
			break;
		}
	}

	if (ferror(fh))
		goto out_close_fh;

	fclose(fh);

	return found;

out_close_fh:
	fclose(fh);
out:
	warn("%s():%d %s", __FUNCTION__, __LINE__, _PATH_PROCNET_DEV);

	return -errno;
}

/**
 * ethtool_gstats - get the statistics of the specified network device using ethtool
 * @ifname: input argument, constant pointer to the network device name
 *
 * Returns NULL if an error occurs, non-NULL otherwise
 */
struct ethtool_stats * ethtool_gstats(const char *ifname)
{
	struct ifreq ifr;
	int fd, err;
	struct ethtool_drvinfo drvinfo;
	struct ethtool_stats *stats = NULL;
	unsigned int n_stats;

	/* Setup our control structures. */
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname);

	/* Open control socket. */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("Cannot get control socket");
		goto out;
	}

	drvinfo.cmd = ETHTOOL_GDRVINFO;
	ifr.ifr_data = (caddr_t)&drvinfo;
	err = ioctl(fd, SIOCETHTOOL, &ifr);
	if (err < 0) {
		perror("Cannot get driver information");
		goto out_close_fd;
	}

	n_stats = drvinfo.n_stats;
	if (n_stats < 1) {
		fprintf(stderr, "no stats available\n");
		goto out_close_fd;
	}

	stats = calloc(1, n_stats * sizeof(uint64_t) + sizeof(struct ethtool_stats));
	if (!stats) {
		fprintf(stderr, "no memory available\n");
		goto out_close_fd;
	}

	stats->cmd = ETHTOOL_GSTATS;
	stats->n_stats = n_stats;
	ifr.ifr_data = (caddr_t)stats;
	err = ioctl(fd, SIOCETHTOOL, &ifr);
	if (err < 0) {
		perror("Cannot get stats information");
		free(stats);
		stats = NULL;
		goto out_close_fd;
	}

out_close_fd:
	close(fd);
out:
	return stats;
}

// Kaohj
/*
 *	Get the link status about device.
 *	Return:
 *		-1 on error
 *		0 on link down
 *		1 on link up
 */
int get_net_link_status(const char *ifname)
{
	#if 0
	struct ifreq ifr;
	struct ethtool_value edata;
	#else
	FILE *fp;
	char buff[32];
	int num;
	#endif
	int ret;
#ifdef CONFIG_DEV_xDSL
	Modem_LinkSpeed vLs;
#endif

#ifdef CONFIG_DEV_xDSL
	if (!strcmp(ifname, ALIASNAME_DSL0)) {
       		if ( adsl_drv_get(RLCM_GET_LINK_SPEED, (void *)&vLs,
			RLCM_GET_LINK_SPEED_SIZE) && vLs.upstreamRate != 0)
			ret = 1;
		else
			ret = 0;
		return ret;
	}
#endif
#ifdef WLAN_WISP
	bss_info pBss;
	if(!strncmp(ifname, "wlan", 4)){
		getWlBssInfo(ifname, &pBss);
		if (pBss.state == STATE_CONNECTED)
			return 1;
		else
			return 0;
	}
#endif
	#if 0
	strcpy(ifr.ifr_name, ifname);
	edata.cmd = ETHTOOL_GLINK;
	ifr.ifr_data = (caddr_t)&edata;

	ret = do_ioctl(SIOCETHTOOL, &ifr);
	if (ret == 0)
		ret = edata.data;
	#else
	sprintf(buff, "/sys/class/net/%s/carrier", ifname);
	if (!(fp = fopen(buff, "r"))) {
		printf("Error: cannot open %s\n", buff);
		return -1;
	}
	num = fscanf(fp, "%d", &ret);
	fclose(fp);
	if (num <= 0)
		return -1;
	#endif
	return ret;
}

/*
 *	Get the link information about device.
 *	Return:
 *		-1 on error
 *		0 on success
 */
int get_net_link_info(const char *ifname, struct net_link_info *info)
{
	struct ifreq ifr;
	struct ethtool_cmd ecmd;
	int ret;

	memset(info, 0, sizeof(struct net_link_info));
	strcpy(ifr.ifr_name, ifname);
	ecmd.cmd = ETHTOOL_GSET;
	ifr.ifr_data = (caddr_t)&ecmd;

	ret = do_ioctl(SIOCETHTOOL, &ifr);
	if (ret == 0) {
		info->supported = ecmd.supported; // ports, link modes, auto-negotiation
		info->advertising = ecmd.advertising; // link modes, pause frame use, auto-negotiation
		info->speed = ecmd.speed; // 10Mb, 100Mb, gigabit
		info->duplex = ecmd.duplex; // Half, Full, Unknown
		info->phy_address = ecmd.phy_address;
		info->transceiver = ecmd.transceiver;
		info->autoneg = ecmd.autoneg;
	}
	return ret;
}

/*
 *	ifname:
 *		"nas0", "ptm0", "eth0.2", "eth0.3" ...
 *	Return:
 *		>=	0 shapingRate
 *		-1	on error
 */
int get_dev_ShapingRate(const char *ifname)
{
	int ret=-1;
#if CONFIG_HWNAT
	ret = hwnat_itf_get_ShapingRate(ifname);
#endif
	return ret;
}

/*
 *	ifname:
 *		"nas0", "ptm0", "eth0.2", "eth0.3" ...
 *	Rate:
 *		-1	no shaping
 *		>=0	rate in bps
 *	Return:
 *		>= 0 on success
 *		-1 on error
 */
int set_dev_ShapingRate(const char *ifname, int rate)
{
	int ret=-1;
#if CONFIG_HWNAT
	ret = hwnat_itf_set_ShapingRate(ifname, rate);
#endif
	return ret;
}

/*
 *	ifname:
 *		"nas0", "ptm0", "eth0.2", "eth0.3" ...
 *	Return:
 *		>=0	Burst size in bytes
 *		-1	on error
 */
int get_dev_ShapingBurstSize(const char *ifname)
{
	int ret=-1;
#if CONFIG_HWNAT
	ret = hwnat_itf_get_ShapingBurstSize(ifname);
#endif
	return ret;
}

/*
 *	ifname:
 *		"nas0", "ptm0", "eth0.2", "eth0.3" ...
 *	bsize:
 		Burst size in bytes
 *	Return:
 *		>=0	on success
 *		-1	on error
 */
int set_dev_ShapingBurstSize(const char *ifname, unsigned int bsize)
{
	int ret=-1;
#if CONFIG_HWNAT
	ret = hwnat_itf_set_ShapingBurstSize(ifname, bsize);
#endif
	return ret;
}

// MEM_UTILITY, CPU_UTILITY
enum enum_mem_info {
	DEV_MEM_TOTAL,
	DEV_MEM_FREE,
	DEV_MEM_USAGE_RATIO
};

struct dev_stats {
        unsigned int    user;    // user (application) usage
        unsigned int    nice;    // user usage with "niced" priority
        unsigned int    system;  // system (kernel) level usage
        unsigned int    idle;    // CPU idle and no disk I/O outstanding
        unsigned int    iowait;  // CPU idle but with outstanding disk I/O
        unsigned int    irq;     // Interrupt requests
        unsigned int    softirq; // Soft interrupt requests
        unsigned int    steal;   // Invol wait, hypervisor svcing other virtual CPU
        unsigned int    total;
};

int getdata(FILE *fp, struct dev_stats *st)
{
	if (fseek(fp, 0, SEEK_SET) < 0)
		return(-1);
	fscanf(fp, "cpu %u %u %u %u %u %u %u %u",
		&st->user, &st->nice, &st->system, &st->idle,
		&st->iowait, &st->irq, &st->softirq, &st->steal);
	
	st->total = st->user + st->nice  + st->system + st->idle + st->iowait +
		st->irq  + st->steal + st->softirq;
	return(0);
}

int get_cpu_usage(void)
{
	FILE *fp;
    struct dev_stats    st;
    unsigned int    curtotal, curidle, curusage;
	static unsigned int totalCpuTime=0, idleCpuTime=0;
    unsigned int    usepercent;

	if( (fp = fopen("/proc/stat","r")) == NULL){
		printf("%s: getCpuTotal Failed to open file\n", __FUNCTION__);
		return -1;
	}

	getdata(fp, &st);
	
	curtotal = st.total - totalCpuTime;
	curidle = st.idle - idleCpuTime;
	curusage = curtotal - curidle;

	totalCpuTime = st.total;
	idleCpuTime = st.idle;
	
	if (curusage > 0 && curtotal > 0)
		usepercent = curusage * 100 / curtotal;
	else
		usepercent = 0;

	fclose(fp);
	return usepercent;
}

static int get_mem_info(int info)
{
	FILE* fs = NULL;
	char line[128];
	int total = 0, free = 0, rate = 0, buffers = 0, cache = 0, use = 0;
	char *pToken = NULL,*pLast;
	int ret = 0, count = 0;
	fs = fopen( "/proc/meminfo" , "r");
	if ( fs == NULL )
		return 0;
	else
	{
		while (count < 4)
		{
			fgets(line, 128, fs);
			strtok_r(line, ":", &pLast);
			if (0 == strcasecmp(line, "MemTotal")) {
				total = atoi(pLast);
				count++;
			} else if (0 == strcasecmp(line, "MemFree")) {
				free = atoi(pLast);
				count++;
			} else if (0 == strcasecmp(line, "Buffers")) {
				buffers = atoi(pLast);
				count++;
			} else if (0 == strcasecmp(line, "Cached")) {
				cache = atoi(pLast);
				count++;
			}
		}

		switch(info) {
		case DEV_MEM_TOTAL:
			ret = total;
			break;
		case DEV_MEM_FREE:
			ret = free + buffers + cache;
			break;
		case DEV_MEM_USAGE_RATIO:
			ret = (total > 0) ? (100*(total-buffers-cache-free))/total : 0;
			break;
		}

		fclose(fs);
		return (int)(ret);
	}
}

int get_mem_free(void)
{
	int m = get_mem_info(DEV_MEM_FREE);
	return m;
}

int get_mem_total(void)
{
	int m = get_mem_info(DEV_MEM_TOTAL);
	return m;
}

int get_mem_usage(void)
{
	int m = get_mem_info(DEV_MEM_USAGE_RATIO);
	return m;
}

int GetWanMode(void)
{
	unsigned int wanmode = 0;

	if(!mib_get(MIB_WAN_MODE, (void *)&wanmode))
		fprintf(stderr, "Get mib value MIB_WAN_MODE failed!\n");

	return (wanmode & WAN_MODE_MASK);
}

int isInterfaceMatch(unsigned int ifindex)
{
	if(WAN_MODE & MODE_ATM && MEDIA_ATM == MEDIA_INDEX(ifindex))
		return 1;
	else if(WAN_MODE & MODE_Ethernet && MEDIA_ETH == MEDIA_INDEX(ifindex))
		return 1;
	else if(WAN_MODE & MODE_PTM && MEDIA_PTM == MEDIA_INDEX(ifindex))
		return 1;

#ifdef WLAN_WISP
	else if(WAN_MODE & MODE_Wlan && MEDIA_WLAN == MEDIA_INDEX(ifindex))
		return 1;
#endif
	return 0;
}

int isDefaultRouteWan(MIB_CE_ATM_VC_Tp pEntry)
{
	#ifdef DEFAULT_GATEWAY_V2
	unsigned int dgw;
	#endif
	
	#ifdef DEFAULT_GATEWAY_V2
	if (mib_get(MIB_ADSL_WAN_DGW_ITF, (void *)&dgw) != 0)
	{
		if ((dgw == pEntry->ifIndex)
		#ifdef AUTO_PPPOE_ROUTE
		 || (dgw == DGW_AUTO)
		#endif
		)
			return 1;
	}
	#else
	#ifdef CONFIG_USER_RTK_WAN_CTYPE
	if((pEntry->applicationtype & (X_CT_SRV_INTERNET)) && pEntry->cmode != CHANNEL_MODE_BRIDGE)
	#else
	if(pEntry->dgw && pEntry->cmode != CHANNEL_MODE_BRIDGE)
	#endif
		return 1;
	#endif
	return 0;
}

/*
 * flag: reserved for future use
 */
int reset_cs_to_default(int flag)
{

#ifdef CONFIG_00R0
	//Request to send DHCP release before factory default
	doReleaseWanConneciton();
#endif

#ifdef RESERVE_KEY_SETTING
	reserve_critical_setting(flag);
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
#else
	return mib_flash_to_default(CURRENT_SETTING);
#endif
	return 1;
}

#ifdef CONFIG_TR_064
int GetTR064Status(void)
{
	unsigned char tr064_st = 0;

	if(!mib_get(MIB_TR064_ENABLED, (void *)&tr064_st))
		fprintf(stderr, "Get mib value MIB_TR064_ENABLED failed!\n");

	return tr064_st;
}
#endif

// Mason Yu. Support ddns status file.
#ifdef CONFIG_USER_DDNS
int stop_all_ddns()
{
	unsigned int entryNum, i;
	MIB_CE_DDNS_T Entry;
	char pidName[128];
	int pid = 0;

	entryNum = mib_chain_total(MIB_DDNS_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_DDNS_TBL, i, (void *)&Entry))
		{
			printf("stop_all_ddns:Get chain record error!\n");
			continue;
		}

		snprintf(pidName, 128, "%s.%s.%s.%s", (char *)DDNSC_PID, Entry.provider, Entry.interface, Entry.hostname);
		pid = read_pid((char *)pidName);
		if (pid > 0) {
			kill(pid, SIGKILL);
			unlink(pidName);
		}
	}
}

int restart_ddns(void)
{
	// Mason Yu.  create DDNS thread dynamically
	//printf("restart_ddns\n");
	// Mason Yu. Specify IP Address
#ifdef CONFIG_IPV6
	va_cmd("/bin/updateddctrl", 2, 1, "all", "3");
#else
	va_cmd("/bin/updateddctrl", 2, 1, "all", "1");
#endif

	return 0;
}

void remove_ddns_status(void)
{
	unsigned int entryNum, i;
	MIB_CE_DDNS_T Entry;
	FILE *fp;
	unsigned char filename[100];

	entryNum = mib_chain_total(MIB_DDNS_TBL);
	for (i=0; i<entryNum; i++) {

		if (!mib_chain_get(MIB_DDNS_TBL, i, (void *)&Entry))
		{
  			printf("remove_ddns_status:Get chain record error!\n");
			continue;
		}

		// Check all variables that updatedd need
		if ( strlen(Entry.username) == 0 ) {
			printf("remove_ddns_status: username/email is NULL!!\n");
			continue;
		}

		if ( strlen(Entry.password) == 0 ) {
			printf("remove_ddns_status: password/key is NULL!!\n");
			continue;
		}

		if ( strlen(Entry.hostname) == 0 ) {
			printf("remove_ddns_status: Hostname is NULL!!\n");
			continue;
		}

		if ( strlen(Entry.interface) == 0 ) {
			printf("remove_ddns_status: Interface is NULL!!\n");
			continue;
		}

		if ( Entry.Enabled != 1 ) {
			printf("remove_ddns_status: The account is disabled!!\n");
			continue;
		}

		if ( strcmp(Entry.provider, "dyndns") == 0 || strcmp(Entry.provider, "tzo") == 0 || strcmp(Entry.provider, "noip") == 0) {
			// open a status file
			sprintf(filename, "/var/%s.%s.%s.txt", Entry.provider, Entry.username, Entry.password);
			if((fp=fopen(filename,"r")) ==NULL)
				continue;
			fclose(fp);
			unlink(filename);
		}else {
			//sprintf(account, "%s:%s", Entry.email, Entry.key);
			printf("remove_ddns_status: Not support this provider\n");
			syslog(LOG_INFO, "remove_ddns_status: Not support this provider %s\n", Entry.provider);
			continue;
		}
	}
}
#endif

#ifdef CONFIG_USER_VLAN_ON_LAN
int setup_VLANonLAN(int mode)
{
	char ethVlan[24], vidStr[8];
	int i, status = 0;
	unsigned short vid;
	MIB_CE_SW_PORT_T sw_entry;

	if (mode == ADD_RULE) {
		status |= va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N",
				 (char *)BR_VLAN_ON_LAN);
		status |= va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P",
				 (char *)BR_VLAN_ON_LAN, (char *)FW_RETURN);
		status |= va_cmd(EBTABLES, 6, 1, "-t", "broute", (char *)FW_ADD, "BROUTING",
			   "-j", (char *)BR_VLAN_ON_LAN);
	} else if (mode == DEL_RULE) {
		status |= va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F",
				 (char *)BR_VLAN_ON_LAN);
		status |= va_cmd(EBTABLES, 6, 1, "-t", "broute", "-D", "BROUTING",
			   "-j", (char *)BR_VLAN_ON_LAN);
		status |= va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X",
			   (char *)BR_VLAN_ON_LAN);
	}

	for (i = 0; i < ELANVIF_NUM; i++) {
		if (!mib_chain_get(MIB_SW_PORT_TBL, i, &sw_entry))
			return -1;
		if (!sw_entry.vlan_on_lan_enabled)
			continue;
		snprintf(vidStr, sizeof(vidStr), "%u", sw_entry.vid);
		snprintf(ethVlan, sizeof(ethVlan), "%s.%u", ELANVIF[i],
			 sw_entry.vid);

		if (mode == ADD_RULE) {
			// (1) use vconfig to config vlan
			// vconfig add eth0.2 3
			status |= va_cmd("/bin/vconfig", 3, 1, "add",
				   ELANVIF[i], vidStr);

			// (2) use ifconfig to up interface
			// ifconfig eth0.2.3 up
			status |= va_cmd(IFCONFIG, 2, 1, (char *)ethVlan, "up");

			// (3) use brctl to add eth0.2.3 into br0 bridge
			// brctl addif br0 eth0.2.3 
			status |= va_cmd(BRCTL, 3, 1, "addif", (char *)BRIF, ethVlan);

			// (4) set drop rule on BROUTING, then tag packet can go to bridge WAN via eth0.2.3 
			// ebtables -t broute -A vlan_on_lan -i eth0.2 -p 0x8100 --vlan-id 3 -j DROP
			status |= va_cmd(EBTABLES, 12, 1, "-t", "broute",
				   (char *)FW_ADD, (char *)BR_VLAN_ON_LAN, "-i",
				   ELANVIF[i], "-p", "0x8100",
				   "--vlan-id", vidStr, "-j", "DROP");
		} else if (mode == DEL_RULE) {
			// (1) use brctl to del eth0.2.3 from br0 bridge
			// brctl delif br0 eth0.2.3 
			status |= va_cmd(BRCTL, 3, 1, "delif", (char *)BRIF, ethVlan);

			// (2) use vconfig to remove vlan
			// vconfig rem eth0.2.3
			status |= va_cmd("/bin/vconfig", 2, 1, "rem", ethVlan);
		}
	}

	return status;
}
#endif

// ysleu: To show diagshell RG DHCP request information.
#if defined(CONFIG_LUNA) && defined(CONFIG_RTK_L34_ENABLE)
void write_to_dhcpc_info(unsigned long requested_ip,unsigned long subnet_mask,unsigned long gw_addr)
{
	int fh;
	int mark;
	char buff[64];

	fh = open("/var/udhcpc/udhcpc.info", O_RDWR|O_CREAT|O_TRUNC, S_IXUSR);

	if (fh == -1) {
		printf("Create udhcpc script file %s error!\n");
		return;
	}

 	WRITE_DHCPC_FILE(fh, "\n##########################################\n");
	WRITE_DHCPC_FILE(fh, "DHCP Request Information:\n");
	sprintf(buff, "[ip_addr]:%d.%d.%d.%d\n",(requested_ip>>24)&0xff,(requested_ip>>16)&0xff,(requested_ip>>8)&0xff,(requested_ip>>0)&0xff);
	WRITE_DHCPC_FILE(fh, buff);
	sprintf(buff, "[ip_network_mask]:%d.%d.%d.%d\n",(subnet_mask>>24)&0xff,(subnet_mask>>16)&0xff,(subnet_mask>>8)&0xff,(subnet_mask>>0)&0xff);
	WRITE_DHCPC_FILE(fh, buff);
	sprintf(buff, "[gateway_ipv4_addr]:%d.%d.%d.%d\n",(gw_addr>>24)&0xff,(gw_addr>>16)&0xff,(gw_addr>>8)&0xff,(gw_addr>>0)&0xff);
	WRITE_DHCPC_FILE(fh, buff);
 	WRITE_DHCPC_FILE(fh, "##########################################\n");

	close(fh);
}
#endif

int update_hosts(char *hostname, struct addrinfo *servinfo)
{
	if(!hostname || !servinfo) return 0;

	struct addrinfo *p;
	struct sockaddr_in *h;
	struct sockaddr_in6 *h6;
	struct in_addr ina;
	struct in6_addr ina6;

	FILE *fp;
	int isV4Existed = 0, isV6Existed = 0;
	char line[80], ip[64];

	if(inet_pton(AF_INET6, hostname, &ina6) > 0 || inet_pton(AF_INET, hostname, &ina) > 0)
		return 0;

	fp = fopen(HOSTS, "a+");
	while(fgets(line, 80, fp) != NULL)
	{
		char *ip,*name,*saveptr1;
		ip = strtok_r(line," \n", &saveptr1);
		name = strtok_r(NULL," \n", &saveptr1);

		if(name && !strcmp(name, hostname))
		{
			if(strchr(ip, '.') != NULL)
				isV4Existed = 1;
			else
				isV6Existed = 1;
		}
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if(p->ai_family == AF_INET)
		{
			h = (struct sockaddr_in *) p->ai_addr;
			strcpy(ip, inet_ntoa(h->sin_addr));

			if(!isV4Existed)
				fprintf(fp, "%-15s %s\n", ip, hostname);
		}
		else if(p->ai_family == AF_INET6)
		{
			h6 = (struct sockaddr_in6 *) p->ai_addr;
			inet_ntop(AF_INET6, &h6->sin6_addr, ip, sizeof(ip));

			if(!isV6Existed)
	 			fprintf(fp, "%-15s %s\n", ip, hostname);
		}
	}

	fclose(fp);
	return 1;
}

struct getaddrinfo_arg {
	char *hostname;
	IP_PROTOCOL IPVer;
	unsigned char end;
};

static void *getaddrinfo_routine(void *a)
{
	struct getaddrinfo_arg *parg = a;
	struct addrinfo hints, *servinfo = NULL;
	char *hostname = NULL;
	int rv;

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

	pthread_cleanup_push((void (*)(void *))freeaddrinfo, servinfo);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = parg->IPVer == IPVER_IPV4 ? AF_INET : parg->IPVer == IPVER_IPV6 ? AF_INET6 : AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	hostname = strdup(parg->hostname);

	if ((rv = getaddrinfo(hostname, NULL, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s, servinfo: %p\n", gai_strerror(rv), servinfo);
	}

	free(hostname);

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	parg->end = 1;

	pthread_cleanup_pop(0);

	return servinfo;
}

// IMPORTANGE NOTE: caller of this API must use freeaddrinfo() to free the returned memory.
struct addrinfo *hostname_to_ip(char *hostname, IP_PROTOCOL IPVer)
{
	pthread_t tid;
	struct getaddrinfo_arg arg = { hostname, IPVer, 0 };
	struct addrinfo *servinfo = NULL;
	int times = 0, rv;

	pthread_create(&tid, NULL, getaddrinfo_routine, &arg);

	/* wait 2 seconds at most */
	while (1) {
		if (arg.end)
			goto intime;

		if (++times >= 200)
			break;

		/* sleep for 0.01 seconds */
		usleep(10000);
	}

timeout:
	rv = pthread_cancel(tid);
	if (rv == ESRCH) {
		/*  No thread with the ID tid could be found */
		goto intime;
	}

	pthread_detach(tid);

	return NULL;
intime:
	pthread_join(tid, (void **)&servinfo);

	update_hosts(hostname, servinfo);

	return servinfo;
}

//return 1: registered, return 0: failed.
int check_user_is_registered()
{
#if defined(_PRMT_X_CT_COM_USERINFO_) && defined(E8B_NEW_DIAGNOSE)
	unsigned int regStatus = 99;
	unsigned int regResult = 99;
	unsigned char no_pop;

	mib_get(PROVINCE_NO_POPUP_REG_PAGE, &no_pop);
	mib_get(CWMP_USERINFO_STATUS, &regStatus);
	mib_get(CWMP_USERINFO_RESULT, &regResult);

	if(no_pop)
		return 1;

	if(regResult != 0 && regResult != 1)
	return 0;

	if(regStatus != 0 && regStatus != 5)
		return 0;

	return 1;
#else
	return 0;
#endif
}

int check_vlan_conflict(MIB_CE_ATM_VC_T *pEntry, int idx, char *err_msg)
{
	int i;
	MIB_CE_ATM_VC_T chk_entry = {0};
	int total = mib_chain_total(MIB_ATM_VC_TBL);

	// Accept duplicated vid if WAN interface is disabled
	if(pEntry->enable == 0)
		return 0;

	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(MIB_ATM_VC_TBL, i, &chk_entry) == 0)
			continue;

		// Do not check it self
		if(i == idx)
			continue;

		// Accept duplicated vid if WAN interface is disabled
		if(chk_entry.enable == 0)
			continue;

		// Accept duplicated vid if different media type
		if(MEDIA_INDEX(pEntry->ifIndex) != MEDIA_INDEX(chk_entry.ifIndex))
			continue;

		// Both are bridging or both are routing
		if((pEntry->cmode == CHANNEL_MODE_BRIDGE && chk_entry.cmode == CHANNEL_MODE_BRIDGE)
			|| ((pEntry->cmode != CHANNEL_MODE_BRIDGE && pEntry->cmode != CHANNEL_MODE_PPPOE) && (chk_entry.cmode != CHANNEL_MODE_BRIDGE && chk_entry.cmode != CHANNEL_MODE_PPPOE)))
		{
			// Both enable vlan tag and have the same vid,
			// or both disable vlan tag
			if((pEntry->vlan == 1 && chk_entry.vlan == 1 && pEntry->vid == chk_entry.vid)
				|| (pEntry->vlan == 0 && chk_entry.vlan == 0))
			{
				// If both media type are ATM, check vpi/vci, too.
				if(MEDIA_INDEX(pEntry->ifIndex) != MEDIA_ATM)
					goto conflict;
				else if(pEntry->vpi == chk_entry.vpi && pEntry->vci == chk_entry.vci)
					goto conflict;
			}
		}
	}

	return 0;

conflict:
	{
		char ifname[IFNAMSIZ] = {0};
		ifGetName(PHY_INTF(chk_entry.ifIndex), ifname, IFNAMSIZ);
		sprintf(err_msg, "VLAN id %d is conflicted with %s\n", pEntry->vid, ifname);
	}
	return -1;
}


#if defined(CONFIG_IPV6) && defined(DUAL_STACK_LITE)
int query_aftr(char *aftr,  char *aftr_dst)
{
	struct addrinfo hints, *res, *p;
	int status;
	char ipstr[INET6_ADDRSTRLEN];
	void *addr=NULL;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;

	printf("aftr=[%s]\n",aftr);
	if ((status = getaddrinfo(aftr, NULL, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo:%s %s\n", aftr, gai_strerror(status));
		return 2;
	}

	printf("IP addresses for [%s]:", aftr);

	p = res;
	if (p &&p->ai_family == AF_INET6) { // IPv6
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
		addr = &(ipv6->sin6_addr);
		memcpy(aftr_dst,addr,sizeof(struct in6_addr));
		
		// convert the IP to a string and print it:
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		printf("[%s]\n",ipstr);
	}


	freeaddrinfo(res);

	return 0;
}
#endif

#ifdef CONFIG_USER_Y1731
const char Y1731_CONF[] = "/var/eoam.xml";
const char Y1731_MEG_CONF[] = "/var/eoam_meg.xml";
//#define OAMDPID "/var/run/oamd.pid"
const char OAMDPID[] = "/var/run/oamd.pid";
const char BR_Y1731[] = "y1731";
/**
 * Buffer for multicast DA class 1
 */
unsigned char y1731_da1[6];

/**
 * Buffer for multicast DA class 2
 */
unsigned char y1731_da2[6];


static void oam_create_multicast_da(int level, unsigned char *addr, int class) {
    unsigned char da[6];

    da[0] = 0x01;
    da[1] = 0x80;
    da[2] = 0xC2;
    da[3] = 0x0;
    da[4] = 0x0;

    switch (class) {
    case 1:
        da[5] = 0x30;
        da[5] += level;
        break;
    case 2:
        da[5] = 0x38;
        da[5] += level;        
        break;
    default:
        da[5] = 0x0;
        printf("Invalid multiclass address class\n");
    }
    memcpy(addr, &da, sizeof(da));
}

/**
 * Function that creates the class 1 multicast destination address.
 * 01-80-C2-00-00-3x where x is 0-7
 *
 * @param level What is the current MEG level
 * @param addr buffer for the addr
 *
 * @note Be sure that addr is long enough to hold the multicast addr.
 */
void oam_create_multicast_da1(int level, unsigned char *addr) {
    oam_create_multicast_da(level, addr, 1);
}

/**
 * Function that creates the class 2 multicast destination address.
 * 01-80-C2-00-00-3y where y is 8-F
 *
 * @param level What is the current MEG level
 * @param addr buffer for the addr
 *
 * @note Be sure that addr is long enough to hold the multicast addr.
 */
void oam_create_multicast_da2(int level, unsigned char *addr) {
    oam_create_multicast_da(level, addr, 2);
}

int Y1731_setup(void) {
	char mode = 0, ccmEnable = 0, pulses = 0, level;
	FILE *fp;
	unsigned short myid, vid;
	char megid[Y1731_MEGID_LEN];
	char incl_iface[Y1731_INCL_IFACE_LEN];
	char loglevel[8];
	unsigned char ccm_interval;
	char ifname[IFNAMSIZ] = {0};
	unsigned int oamExtItf;
	char oamExtItf_ifname[16];
	
	mib_get(Y1731_MODE, (void *)&mode);
	if (0==mode)
		return -1;

	fp = fopen( Y1731_CONF, "w");
	if (NULL==fp)
		return -1;

	if (mib_get(Y1731_MEGLEVEL, (void *)&level) &&
		mib_get(Y1731_MYID, (void *)&myid) &&
		mib_get(Y1731_VLANID, (void *)&vid) &&
		mib_get(Y1731_MEGID, (void *)megid) &&
		mib_get(Y1731_INCL_IFACE, (void *)incl_iface) &&
		//mib_get(Y1731_WAN_INTERFACE, (void *)&oamExtItf_ifname) &&
		mib_get(Y1731_LOGLEVEL, (void *)loglevel) &&
		mib_get(Y1731_ENABLE_CCM, (void *)&ccmEnable) &&
		mib_get(Y1731_CCM_INTERVAL, (void *)&ccm_interval)
		) {
			char *ccm_prec, *ccm_period;

			switch (ccm_interval) {
			case 1: ccm_prec = "ms"; ccm_period = "3"; break;
			case 2: ccm_prec = "ms"; ccm_period = "10"; break;
			case 3: ccm_prec = "ms"; ccm_period = "100"; break;
			case 4: ccm_prec = "s";  ccm_period = "1"; break;
			case 5: ccm_prec = "s";  ccm_period = "10"; break;
			case 6: ccm_prec = "min";  ccm_period = "1"; break;
			case 7: ccm_prec = "min";  ccm_period = "10"; break;
			default:
				ccm_prec = "s";  ccm_period = "1"; break;
			}
		
			if(ccmEnable)
				pulses = 5;
			else
				pulses = 0;
			
		fprintf(fp, "<?xml version=\"1.0\"?>\n"
			"<me>\n"
			"<meg name=\"testmeg\" id=\"%s\" level=\"%d\" max=\"16\" />\n"
			"<myid name=\"wel-36\" id=\"%d\"/>\n"
			"<vid name=\"vlan\" id=\"%d\"/>\n"
			"<config log=\"stderr\" loglevel=\"%s\" pulses=\"%d\" dynamic=\"on\" />\n"
			"<ccd use=\"off\" ip=\"127.0.0.1\" port=\"8080\" />\n"
			"<ccm precision=\"%s\" period=\"%s\" one-way-loss-measurement=\"on\" />\n"
			"<lt ttl=\"10\" />\n"
			"<ais precision=\"s\" period=\"1\" />\n"
			"<lck precision=\"s\" period=\"1\" />\n"
			"<mcc oui=\"00:22:68\" />\n"
			"<lm used=\"off\" precision=\"ms\" period=\"100\" warning=\"10\" error=\"20\" samples=\"30\" th_warning=\"40\" th_error=\"50\"/>\n"
			"<dm used=\"off\" optional_fields=\"on\" precision=\"ms\" period=\"100\" warning=\"10\" error=\"20\" samples=\"30\" th_warning=\"40\" th_error=\"50\" />\n"
			"<include ifs=\"%s\" />\n"
			"</me>\n", megid, level, myid, vid, loglevel, pulses, ccm_prec, ccm_period, incl_iface);
		fclose(fp);
	} else {
		goto ERROR_00;
	}

	fp = fopen(Y1731_MEG_CONF, "w");
	if (NULL==fp)
		return -1;

	fprintf(fp, "<?xml version=\"1.0\"?>\n"
		"<participants>\n"
		//"<meg>\n"
		//"<mep name=\"aravis\" id=\"1\">\n"
		//"<mac>%s</mac>\n"
		//"<ifname>%s</ifname>\n"
		//"</mep>\n"
		//"</meg>\n"
		//"</participants>\n", macaddr, oamExtItf_ifname);
		"</participants>\n");
	fclose(fp);

	return 0;
ERROR_00:
	fclose(fp);
	return -1;
}

#if 0
int set_BROUTE_Rule_Y1731(int mode)
{	
	int status = 0;	
	char ifname[IFNAMSIZ] = {0};
	int vcTotal, i;
	MIB_CE_ATM_VC_T Entry;	
	char level;
	char macaddr[20];
	
	if (mode == ADD_RULE) {
		status |= va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N",
				 (char *)BR_Y1731);
		status |= va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P",
				 (char *)BR_Y1731, (char *)FW_RETURN);
		status |= va_cmd(EBTABLES, 6, 1, "-t", "broute", "-A", "BROUTING",
			   "-j", (char *)BR_Y1731);
			   
		
		if (!mib_get(Y1731_MEGLEVEL, (void *)&level)){
			printf("Get Y1731_MEGLEVEL failed\n");
			return -1;
		}
		oam_create_multicast_da1(level, y1731_da1);
		oam_create_multicast_da2(level, y1731_da2);
	
		vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < vcTotal; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				return -1;
			if (Entry.enable == 0)
				continue;
			if ((CHANNEL_MODE_T)Entry.cmode != CHANNEL_MODE_BRIDGE)
				continue;
			#ifdef CONFIG_ETHWAN			
			if(( MEDIA_INDEX(Entry.ifIndex)==MEDIA_ETH ) || ( MEDIA_INDEX(Entry.ifIndex)==MEDIA_PTM ))					
			#endif
			{			
				snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
					y1731_da1[0], y1731_da1[1], y1731_da1[2], y1731_da1[3], y1731_da1[4], y1731_da1[5]);
				//eg.: ebtables -t broute -A BROUTING -d 01:80:c2:00:00:32 -j DROP
				va_cmd(EBTABLES, 8, 1, "-t", "broute", "-A", BR_Y1731, "-d", macaddr, "-j", "DROP");
					
				snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
					y1731_da2[0], y1731_da2[1], y1731_da2[2], y1731_da2[3], y1731_da2[4], y1731_da2[5]);
				//eg.: ebtables -t broute -A BROUTING -d 01:80:c2:00:00:38 -j DROP
				va_cmd(EBTABLES, 8, 1, "-t", "broute", "-A", BR_Y1731, "-d", macaddr, "-j", "DROP");			
				break;
			}
		}
	} else if (mode == DEL_RULE) {
		status |= va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F",
				 (char *)BR_Y1731);
		status |= va_cmd(EBTABLES, 6, 1, "-t", "broute", "-D", "BROUTING",
			   "-j", (char *)BR_Y1731);
		status |= va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X",
				(char *)BR_Y1731);
	}	
	return status;
}
#endif

void Y1731_stop(void) {
	pid_t pid;
	pid = read_pid((char *)OAMDPID);
	if (pid > 0)
	{
		//set_BROUTE_Rule_Y1731(DEL_RULE);		
		kill(pid, 15);
		//unlink(OAMDPID);
		while(read_pid((char*)OAMDPID) > 0)
			usleep(250000);
	}
}

int Y1731_start(int dowait) {
	pid_t pid;
	char ccmEnable = 0;
	
	Y1731_stop();
	if (Y1731_setup())
		return -1;
	
	//set_BROUTE_Rule_Y1731(ADD_RULE);
	mib_get(Y1731_ENABLE_CCM, (void *)&ccmEnable);	
	if (ccmEnable) {		
		va_cmd("/bin/oamd", 4, dowait,
			"-c", Y1731_CONF,
			"-m", Y1731_MEG_CONF);
	}
	else {
		va_cmd("/bin/oamd", 5, dowait,		
			"-c", Y1731_CONF,
			"-m", Y1731_MEG_CONF,
			"-x");  // Not send CCM			
	}	
	pid = read_pid((char *)OAMDPID);
	return (pid>0) ? 0 : -1;
}

#endif

#ifdef CONFIG_USER_8023AH
#define PID_FILE_8023AH "/var/run/oamlinkd.pid"

void EFM_8023AH_stop(void) {
	pid_t pid;
	pid = read_pid((char *)PID_FILE_8023AH);
	if (pid > 0)
	{
		kill(pid, 9);
		unlink(PID_FILE_8023AH);
	}
}

int EFM_8023AH_start(int dowait) {
	pid_t pid;
	char Enable = 0, Active = 0;
	unsigned int efm_8023ah_ext_if;
	char ifname[IFNAMSIZ] = {0};
	
	EFM_8023AH_stop();	
	
	mib_get(EFM_8023AH_MODE, (void *)&Enable);
	if (Enable == 0)
		return -1;	

	mib_get(EFM_8023AH_WAN_INTERFACE, (void *)ifname);
	mib_get(EFM_8023AH_ACTIVE, (void *)&Active);
	if (Active == 0)  // passive
		va_cmd("/bin/oamlinkd", 2, dowait, "-i", ifname);
	if (Active == 1)  //Active
		va_cmd("/bin/oamlinkd", 4, dowait, "-m", "1", "-i", ifname);

	pid = read_pid((char *)PID_FILE_8023AH);
	return (pid>0) ? 0 : -1;
}

#endif

#ifdef CONFIG_USB_SUPPORT
int getUsbDeviceLabel(const char *device, char *label, char *type)
{
        FILE *fp;
        char command[100], buff[256], tmpBuf[256], *pStr;
        int i;

        snprintf(command, 100, "/bin/blkid %s > /tmp/blkid_file", device);
        system(command);


        if ((fp = fopen("/tmp/blkid_file", "r")) == NULL) {
                printf("open /tmp/blkid_file failed.\n");
                return 0;
        }
        else {
                if ( fgets(buff, sizeof(buff), fp) != NULL )
                {
                        memcpy(tmpBuf, buff, 256);
                        if ((pStr = strstr(tmpBuf, "LABEL=")) != NULL) {
                                pStr += strlen("LABEL=");

                                /* LABEL="LISI" */
                                pStr++;
                                i = 0;
                                while (pStr[i] != '"')
                                        i++;
                                pStr[i] = '\0';

                                strcpy(label, pStr);
                        }
                        else
                                strcpy(label, "Unknown");

                        memcpy(tmpBuf, buff, 256);
                        if ((pStr = strstr(tmpBuf, "TYPE=")) != NULL) {
                                pStr += strlen("TYPE=");

                                pStr++;
                                i=0;
                                while (pStr[i] != '"')
                                        i++;
                                pStr[i] = '\0';
                                strcpy(type, pStr);
                        }
                        else
                                strcpy(type, "Unknown");
                }

                fclose(fp);
                unlink("/tmp/blkid_file");
        }
        return 1;
}

static int usb_filter(const struct dirent *dirent)
{
        const char *name = dirent->d_name;
        struct stat statbuff;
        char path[32];

        if((strlen(name) == 4) && (name[0] == 's') && (name[1] == 'd')
           && (name[2] >= 'a') && (name[2] <= 'z')
           && (name[3] >= '0') && (name[3] <= '9'))
                return 1;
        else if((strlen(name) == 3) &&(name[0] == 's') && (name[1] == 'r')
                && (name[2] >= '0') && (name[2] <= '9'))
                return 1;
        else if ((strlen(name) == 3) &&(name[0] == 's') && (name[1] == 'd')
                && (name[2] >= 'a') && (name[2] <= 'z')) {
                sprintf(path, "/mnt/%s", name);
                if (stat(path, (struct stat *)&statbuff) == -1)
                        return 0;
                else if (0 == statbuff.st_blocks)
                        return 0;
                return 1;
        }

        return 0;
}

int umountUSBDevie(void)
{
        int errcode = 1;
        struct dirent **namelist;
        char device[100];
        int i, n;

        n = scandir("/mnt", &namelist, usb_filter, alphasort);

        if (n <= 0)
                return 1;

        for (i = 0; i < n; i++)
        {
                if (namelist[i]->d_name[0] == 's')
                {
                        snprintf(device, 100, "/dev/%s", namelist[i]->d_name);
                        va_cmd("/bin/umount", 1, 1, (char *)device);
                }

                free(namelist[i]);
        }
        free(namelist);

        return 1;
}


void getUSBDeviceInfo(int *disk_sum, struct usb_info * disk1,struct usb_info * disk2)
{
        struct dirent **namelist;
        char tmpLabel[100], tmpFSType[10];
        char device[2][20], type[2][100], mounton[2][512];
        int  usb_storage_num[2];
        char usb_storage_dev[2][10];
        char usb_storage_serial[2][100];
        char invert=0;
        char tmpstr1[100], tmpstr2[100];
        char usb1_serial[100], usb2_serial[100];
        char usb1_dev[10], usb2_dev[10];
        char cmd[100];
        char line[512] = {0};
        FILE *pf = NULL;
        int i, n, diskSum=0;
        unsigned long used[2],avail[2];

        n = scandir("/mnt", &namelist, usb_filter, alphasort);
        memset(type, 0, sizeof(type));
        memset(mounton,0,sizeof(mounton));
        memset(used,0,sizeof(used));
        memset(avail,0,sizeof(avail));

        /* no match */
        if (n > 0)
        {
                for (i=0; i<n; i++) {
                        if ((namelist[i]->d_name[0] == 's') && (diskSum < 2)) {
                                if (strlen(namelist[i]->d_name) == 3)
                                        namelist[i]->d_name[3] = '\0';
                                else
                                        namelist[i]->d_name[4] = '\0';
                                snprintf(device[diskSum], 20, "/dev/%s", namelist[i]->d_name);
                                memset(tmpLabel, 0, sizeof(tmpLabel));
                                memset(tmpFSType, 0, sizeof(tmpFSType));
                                getUsbDeviceLabel(device[diskSum], tmpLabel, tmpFSType);
                                snprintf(type[diskSum], 100, "%s", tmpFSType);
                                snprintf(cmd,sizeof(cmd),"df %s > /tmp/usbinfo",device[diskSum]);
                                system(cmd);
                                pf=fopen("/tmp/usbinfo","r");
                                if(pf)
                                {
                                        fgets(line,sizeof(line),pf);
                                        if(fgets(line,sizeof(line),pf))
                                        {
                                                sscanf(line,"%*s %*d %d %d %*s %s\n",&used[diskSum],&avail[diskSum],mounton[diskSum]);
                                        }
                                        fclose(pf);
                                        unlink("/tmp/usbinfo");
                                }
                                diskSum++;
                        }
                        free(namelist[i]);
                }

                free(namelist);
        }
        *disk_sum = diskSum;

        usb_storage_num[0] = usb_storage_num[1] = -1;
        memset(usb_storage_dev, 0, sizeof(usb_storage_dev));
        memset(usb_storage_serial, 0, sizeof(usb_storage_serial));

        snprintf(cmd, 100, "ls /proc/scsi/usb-storage/  > /tmp/storage.tmp 2>&1");
        system(cmd);

        pf = fopen("/tmp/storage.tmp", "r");
        if(pf) {
                i = 0;
                while(fgets(line, sizeof(line), pf)!=NULL) {
                        //printf("%s:%s", __func__, line);
                        sscanf(line, "%d", &usb_storage_num[i]);

                        i++;
                        if (i >= 2)
                                break;
                }
                //printf("usb_storage_num %d %d\n", usb_storage_num[0], usb_storage_num[1]);
                fclose(pf);
        }

        for (i=0; i<2; i++)
        {
                if (usb_storage_num[i] != -1) {
                        snprintf(cmd, 100, "ls /sys/class/scsi_device/%d:0:0:0/device/block/ > /tmp/storage.tmp 2>&1",
                                        usb_storage_num[i]);
                        system(cmd);

                        pf = fopen("/tmp/storage.tmp", "r");
                        if(pf) {
                                fgets(line, sizeof(line), pf);
                                //printf("%s\n", line);
                                sscanf(line, "%s", usb_storage_dev[i]);
                                fclose(pf);
                        }

                        snprintf(cmd, 100, "cat /proc/scsi/usb-storage/%d > /tmp/storage.tmp 2>&1", usb_storage_num[i]);
                        system(cmd);

                        pf = fopen("/tmp/storage.tmp", "r");
                        if(pf) {
                                while(fgets(line, sizeof(line), pf)!=NULL) {
                                        //printf("%s\n", line);
                                        sscanf(line, "%s %*s %s", tmpstr1, tmpstr2);
                                        if (strstr(tmpstr1, "Serial")) {
                                                strcpy(usb_storage_serial[i], tmpstr2);
                                                //printf("%s:storage_serial[%d]:%s\n", __func__, i, usb_storage_serial[i]);
                                                break;
                                        }
                                }
                                fclose(pf);
                        }
                }
        }

        memset(usb1_serial, 0, sizeof(usb1_serial));
        memset(usb2_serial, 0, sizeof(usb2_serial));
        memset(usb1_dev, 0, sizeof(usb1_dev));
        memset(usb2_dev, 0, sizeof(usb2_dev));

        pf = fopen("/sys/bus/usb/devices/2-1/serial", "r");
        if (!pf)
                pf = fopen("/sys/bus/usb/devices/2-2/serial", "r");
        if (pf) {
                fgets(line, sizeof(line), pf);
                sscanf(line, "%s", usb1_serial);
                //printf("%s:usb2_serial:%s", __func__, usb1_serial);
                fclose(pf);
        }

        pf = fopen("/sys/bus/usb/devices/1-2/serial", "r");
        if (!pf)
                pf = fopen("/sys/bus/usb/devices/1-1/serial", "r");
        if (pf) {
                fgets(line, sizeof(line), pf);
                sscanf(line, "%s", usb2_serial);
                //printf("%s:usb1_serial:%s\n", __func__, usb2_serial);
                fclose(pf);
        }

        unlink("/tmp/storage.tmp");

        if (usb1_serial[0]) {//usb1 running(USB2.0)
                for (i=0; i<2; i++) {
                        if (usb_storage_serial[i][0] && !strcmp(usb_storage_serial[i], usb1_serial)) {
                                strcpy(usb1_dev, usb_storage_dev[i]);
                                break;
                        }
                }
                //debug
                if (i >= 2)
                        printf("unknown usb1 dev name\n");
                else {
                        if (strstr(device[0], usb1_dev))
                                invert = 0;
                        else if (strstr(device[1], usb1_dev))
                                invert = 1;
                }
        }

        if (usb2_serial[0]) {//usb2 running(USB3.0)
                for (i=0; i<2; i++) {
                        if (usb_storage_serial[i][0] && !strcmp(usb_storage_serial[i], usb2_serial)) {
                                strcpy(usb2_dev, usb_storage_dev[i]);
                                break;
                        }
                }
                //debug
                if (i >= 2)
                        printf("unknown usb2 dev name\n");
                else {
                        if (strstr(device[0], usb2_dev))
                                invert = 1;
                        else if (strstr(device[1], usb2_dev))
                                invert = 0;
                }
        }

        if(usb1_dev[0])
        {
                strcpy(disk1->disk_type, usb1_dev);
                strcpy(disk1->disk_status, "Mounted");
                if(invert)
                {
                        sprintf(disk1->disk_fs, "%s", type[1]);
                        disk1->disk_used=used[1];
                        disk1->disk_available=avail[1];
                        strcpy(disk1->disk_mounton, mounton[1]);
                }
                else
                {
                        sprintf(disk1->disk_fs, "%s", type[0]);
                        disk1->disk_used=used[0];
                        disk1->disk_available=avail[0];
                        strcpy(disk1->disk_mounton,  mounton[0]);
                }
        }
        else
        {
                strcpy(disk1->disk_type, "No Device");
                strcpy(disk1->disk_status, "Disconnect");
                strcpy(disk1->disk_fs,"");
                strcpy(disk1->disk_mounton,"");
                disk1->disk_used=0;
                disk1->disk_available=0;
        }

        if(usb2_dev[0])
        {
                strcpy(disk2->disk_type, usb2_dev);
                strcpy(disk2->disk_status, "Mounted");
                if(invert)
                {
                        sprintf(disk2->disk_fs, "%s", type[0]);
                        disk2->disk_used=used[0];
                        disk2->disk_available=avail[0];
                        strcpy(disk2->disk_mounton, mounton[0]);
                }
                else
                {
                        sprintf(disk2->disk_fs, "%s", type[1]);
                        disk2->disk_used=used[1];
                        disk2->disk_available=avail[1];
                        strcpy(disk2->disk_mounton, mounton[1]);
                }
        }
        else
        {
                strcpy(disk2->disk_type, "No Device");
                strcpy(disk2->disk_status, "Disconnect");
                strcpy(disk2->disk_fs,"");
                strcpy(disk2->disk_mounton,"");
                disk2->disk_used=0;
                disk2->disk_available=0;
        }

}
#endif//end of CONFIG_USB_SUPPORT

#if defined(CONFIG_00R0) && defined(USER_WEB_WIZARD)
void set_internet_status(unsigned char internet_status)
{
	unsigned char mib_internet_status;

	mib_get(MIB_USER_TROUBLE_WIZARD_INTERNET_STATUS, (void *)&mib_internet_status);

	if((mib_internet_status == INTERNET_NOT_READY )&& (internet_status == INTERNET_CONNECTED)){
		printf("Save internet status to CONNECTED into MIB!");
		mib_set(MIB_USER_TROUBLE_WIZARD_INTERNET_STATUS, (void *)&internet_status);
		return internet_status;
	}
}

unsigned char get_internet_status()
{
	unsigned char internet_status;

	mib_get(MIB_USER_TROUBLE_WIZARD_INTERNET_STATUS, (void *)&internet_status);
	//printf("Get internet status : %d!\n",internet_status);
	return internet_status;

}
#endif

#if defined(CONFIG_00R0) && defined(CONFIG_RTK_HOST_SPEEDUP)
#define HOST_SPEEDUP "/proc/HostSpeedUP-Info"

/*
 * FUNCTION:ADD_HOST_SPEEDUP
 *
 * Set the IP and Port of local in/out network traffics, which you don't want to pass through netfilter.
 *
 * INPUT:
 *	srcip:
 *		source ip.
 *	sport:
 *		source port.
 *	dstip:
 *		destination ip.
 *	dport:
 *		destination port.
 *
 *	RETURN:
 *	0: Sucess
 *	-1: Failure
 */

int add_host_speedup(struct in_addr srcip, unsigned short sport, struct in_addr dstip, unsigned short dport)
{

	int ret=0;
	FILE *fp=NULL;
	char str_srcip[64] = {0};
	char str_dstip[64] = {0};
	if(srcip.s_addr==0 || dstip.s_addr==0){
		printf("srcip or dstip equal to 0 is not accept!\n");
		ret = -1;
		return ret;
	}
	inet_ntop(AF_INET, (const void *) &srcip, str_srcip, 64);
	inet_ntop(AF_INET, (const void *) &dstip, str_dstip, 64);
//printf("%s-%d sip=%s, sport=%d, dip=%s, dport=%d\n",__func__,__LINE__, str_srcip,sport,str_dstip,dport);
	fp = fopen(HOST_SPEEDUP, "w");
	if(fp)
	{
		fprintf(fp, "add sip %s sport %d dip %s dport %d\n",str_srcip,sport,str_dstip,dport);
		fclose(fp);
		ret = 0;
	}else{
		fprintf(stderr, "open %s fail!\n",HOST_SPEEDUP);
		ret = -1;
	}

	return ret;
}

/*
 * FUNCTION:DEL_HOST_SPEEDUP
 *
 * Delete the track of IP and Port for local in/out network traffics, which you don't want to pass through netfilter.
 *
 * INPUT:
 *	srcip:
 *		source ip.
 *	sport:
 *		source port.
 *	dstip:
 *		destination ip.
 *	dport:
 *		destination port.
 *
 *	RETURN:
 *	0: Sucess
 *	-1: Failure
 */

int del_host_speedup(struct in_addr srcip, unsigned short sport, struct in_addr dstip, unsigned short dport)
{

	int ret=0;
	FILE *fp=NULL;
	char str_srcip[64] = {0};
	char str_dstip[64] = {0};
	inet_ntop(AF_INET, (const void *) &srcip, str_srcip, 64);
	inet_ntop(AF_INET, (const void *) &dstip, str_dstip, 64);
//printf("%s-%d sip=%s, sport=%d, dip=%s, dport=%d\n",__func__,__LINE__, str_srcip,sport,str_srcip,dport);
	fp = fopen(HOST_SPEEDUP, "w");
	if(fp)
	{
		fprintf(fp, "del info\n");
		fclose(fp);
		ret = 0;
	}else{
		fprintf(stderr, "open %s fail!\n",HOST_SPEEDUP);
		ret = -1;
	}

	return ret;
}


#endif /*CONFIG_RTK_HOST_SPEEDUP*/

#if defined(CONFIG_IPV6)
/************************************************
* Propose: delOrgLanLinklocalIPv6Address
*
*    delete the original Link local IPv6 address
*      e.q: ifconfig br0 del fe80::2e0:4cff:fe86:5338/64 >/dev/null 2>&1
*
*    When modify the function, please also modify _delOrgLanLinklocalIPv6Address()
*    in src/linux/msgparser.c
* Parameter:
*	None
* Return:
*     None
* Author:
*     Alan
*************************************************/
void delOrgLanLinklocalIPv6Address(void)
{
	unsigned char value[64];
	struct in6_addr ip6Addr, targetIp;
	unsigned char devAddr[MAC_ADDR_LEN];
	unsigned char *p = &targetIp.s6_addr[0];
	int i=0;
	char cmdBuf[256]={0};

	if (mib_get(MIB_ELAN_MAC_ADDR, (void *)devAddr) != 0)
	{
		ipv6_linklocal_eui64(p, devAddr);
		inet_ntop(PF_INET6, &targetIp, value, sizeof(value));
		sprintf(cmdBuf, "%s %s %s %s/%d %s %s", IFCONFIG, LANIF, ARG_DEL, value, 64, hideErrMsg1, hideErrMsg2);
		//use system to prevent error message
		system(cmdBuf);
		//va_cmd(IFCONFIG, 5, 1, LANIF, ARG_DEL, value, hideErrMsg1, hideErrMsg2);
	}
}

/************************************************
* Propose: setLanLinkLocalIPv6Address(void)
*    set the Link local IPv6 address
*      e.q: ifconfig br0 add fe80::1/64 >/dev/null 2>&1
*
*    When modify the function, please also modify _setLanLinkLocalIPv6Address()
*    in src/linux/msgparser.c
* Parameter:
*	None
* Return:
*     None
* Author:
*     Alan
*************************************************/
void setLanLinkLocalIPv6Address(void)
{
	unsigned char value[64];

	if (mib_get(MIB_IPV6_LAN_IP_ADDR, (void *)value) != 0)
	{
		char cmdBuf[256]={0};
		sprintf(cmdBuf, "%s %s %s %s/%d %s %s", IFCONFIG, LANIF, ARG_ADD, value, 64, hideErrMsg1, hideErrMsg2);
		//use system to prevent error message
		system(cmdBuf);
		//va_cmd(IFCONFIG, 5, 1, LANIF, ARG_ADD, cmdBuf, hideErrMsg1, hideErrMsg2);
	}
}

/************************************************
* Propose: radvdRunningMode(void)
*    Get radvd running mode
* Parameter:
*	None
* Return:
*      0  :  RADVD_RUNNING_MODE_DISABLE
*      1  :  RADVD_RUNNING_MODE_STATIC
*      2  :  RADVD_RUNNING_MODE_DELEGATION
* Author:
*     Alan
*************************************************/
int radvdRunningMode(void)
{
	int vcTotal, i;
	MIB_CE_ATM_VC_T Entry;
	
	vcTotal = mib_chain_total(MIB_ATM_VC_TBL);
	
	for (i = 0; i < vcTotal; i++)
	{
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
			return -1;

		if (Entry.enable == 0)
			continue;

		if ((CHANNEL_MODE_T)Entry.cmode == CHANNEL_MODE_BRIDGE)
			continue;

		if (Entry.IpProtocol == IPVER_IPV4)
			continue;

		if (isDefaultRouteWan(&Entry) && ((Entry.Ipv6DhcpRequest & 0x2)==0x2)){
			return RADVD_RUNNING_MODE_DELEGATION;
		}
	}

	return RADVD_RUNNING_MODE_DISABLE;
}

/************************************************
* Propose: setLinklocalIPv6Address
*
*    set thel Link local IPv6 address
*      e.q: ifconfig br0 add fe80::2e0:4cff:fe86:5338/64 >/dev/null 2>&1
*
* Parameter:
*	char* ifname              interface name
* Return:
*     None
* Author:
*     Alan
*************************************************/
void setLinklocalIPv6Address(char* ifname)
{
	unsigned char value[64];
	struct in6_addr ip6Addr, targetIp;
	unsigned char devAddr[MAC_ADDR_LEN];
	unsigned char *p = &targetIp.s6_addr[0];
	int i=0;
	char cmdBuf[256]={0};

	if (getMacAddr(ifname, devAddr) != 0)
	{
		ipv6_linklocal_eui64(p, devAddr);
		inet_ntop(PF_INET6, &targetIp, value, sizeof(value));
		sprintf(cmdBuf, "%s %s %s %s/%d %s %s", IFCONFIG, ifname, ARG_ADD, value, 64, hideErrMsg1, hideErrMsg2);
		//use system to prevent error message
		system(cmdBuf);
		//va_cmd(IFCONFIG, 5, 1, ifname, ARG_ADD, value, hideErrMsg1, hideErrMsg2);
	}
}

#endif /* #if defined(CONFIG_IPV6) */


/************************************************
* Propose: checkProcess()
*    Check process exist or not
* Parameter:
*	char* pidfile      pid file path
* Return:
*      1  :  exist
*      0  :  not exist
*      -1:  parameter error
* Author:
*     Alan
*************************************************/
int checkProcess(char* pidfile)
{
	char command[256] = {0};
	char output[256] = {0};
	int pid;

	if(pidfile==NULL || strlen(pidfile)==0){
		fprintf(stderr, "Error: [%s %s:%d] pid file is empty!!\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

	pid = read_pid(pidfile);
	if(pid>0){
		return 1;
	}

	return 0;
}

/************************************************
* Propose: waitProcessTerminate()
*    wait process until process does not exist
* Parameter:
*	char* pidfile                    pid file path
*     unsigned int timeout        0: wait infinite, otherise wait timeout miliseconds(mutiple of 10)
* Return:
*      0  : success
*      -1: parameter error
* Author:
*     Alan
*************************************************/
int waitProcessTerminate(char* pidfile, unsigned int timeout)
{
	clock_t time_end;

	if(pidfile==NULL || strlen(pidfile)==0){
		fprintf(stderr, "Error: [%s %s:%d] pid file is empty!!\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}
	
	time_end = clock() + (timeout/10*10);
	while(checkProcess(pidfile)==1){
		if(timeout!=0){
			if(clock() > time_end){
				break;
			}
		}
		usleep(10);
	}
	return 0;
}

/************************************************
* Propose: getMacAddr
*
*    get MAC address
*
* Parameter:
*	char* ifname                     interface name
*     unsigned char* macaddr      mac addr
* Return:
*     -1 : fail
*       0 : success
* Author:
*     Alan
*************************************************/
int getMacAddr(char* ifname, unsigned char* macaddr)
{
	int s;
	struct ifreq ifr;

	if(ifname==NULL || strlen(ifname)==0 || macaddr==NULL){
		printf("%s: parameter error\n", __FUNCTION__);
        return -1;
	}
	
    s = socket(PF_PACKET, SOCK_DGRAM, 0);

    if (s < 0) {
        printf("%s: socket error\n", __FUNCTION__);
        return -1;
    }

	memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if (ioctl(s, SIOCGIFHWADDR, &ifr) < 0) {
        printf("%s: Interface %s not found\n", __FUNCTION__, ifname);
		close(s);
        return -1;
    }
	close(s);

	if (ifr.ifr_hwaddr.sa_family!=ARPHRD_ETHER) {
    	printf("%s: not an Ethernet interface\n", __FUNCTION__);
		return -1;
	}
	memcpy(macaddr, ifr.ifr_hwaddr.sa_data, MAC_ADDR_LEN);

	//printf("%s: %02X:%02X:%02X:%02X:%02X:%02X\n", __FUNCTION__,
    //	macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
    return 0;
}


#ifdef CONFIG_EPON_FEATURE	
int config_oam_vlancfg()
{
	char cfg_type, manu_mode, manu_pri;
	unsigned short manu_vid;
	char cmd[128];
	int status = -1;
	int i;
	
	if(!mib_get(MIB_VLAN_CFG_TYPE, (void *)&cfg_type))
	{
		printf("MIB_VLAN_CFG_TYPE get failed!\n");
		return status;
	}

	if(!mib_get(MIB_VLAN_MANU_MODE, (void *)&manu_mode))
	{
		printf("MIB_VLAN_MANU_MODE get failed!\n");
		return status;
	}
	
	if(cfg_type == VLAN_AUTO)
	{
		snprintf(cmd, sizeof(cmd), "oamcli set ctc statusVlan 0");
		system(cmd);
		printf("%s\n",cmd);
	}
	else
	{
		snprintf(cmd, sizeof(cmd), "oamcli set ctc statusVlan 1");
		system(cmd);
		printf("%s\n",cmd);
	}
	
	if((cfg_type == VLAN_MANUAL) && (manu_mode == VLAN_MANU_TAG))
	{
		if(!mib_get(MIB_VLAN_MANU_TAG_VID, (void *)&manu_vid))
		{
			printf("MIB_VLAN_MANU_TAG_VID get failed!\n");
			return status;
		}
		if(!mib_get(MIB_VLAN_MANU_TAG_PRI, (void *)&manu_pri))
		{
			printf("MIB_VLAN_MANU_TAG_PRI get failed!\n");
			return status;
		}

		for(i=0; i<ELANVIF_NUM; i++)
		{
			snprintf(cmd, sizeof(cmd), "oamcli set ctc vlan %d 1 %d/%d", i, manu_vid, manu_pri);
			status = system(cmd);
			printf("%s\n",cmd);
		}
	}
	else
	{
		for(i=0; i<ELANVIF_NUM; i++)
		{
			snprintf(cmd, sizeof(cmd), "oamcli set ctc vlan %d 0",i);
			status = system(cmd);
			printf("%s\n",cmd);
		}
	}

	return status;
}
#endif

#if defined(CONFIG_PON_LINKCHANGE_EVENT) || defined(CONFIG_00R0)
int doReleaseWanConneciton()
{
	unsigned int entryNum=0, i=0;
	char ifname[IFNAMSIZ]={0};
	char sVal[32]={0};
	MIB_CE_ATM_VC_T entry={0};

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry)){
  			printf("Get chain record error!\n");
			return -1;
		}

		if (entry.enable == 0)
			continue;

		ifGetName(entry.ifIndex,ifname,sizeof(ifname));

		if(entry.ipDhcp == DHCP_CLIENT ){ //DHCPC
			int dhcpc_pid;
			printf("Release DHCPClient ifname=%s\n",ifname);
			// DHCP Client
			snprintf(sVal, 32, "%s.%s", (char*)DHCPC_PID, ifname);
			dhcpc_pid = read_pid((char*)sVal);
			if (dhcpc_pid > 0) {
				kill(dhcpc_pid, SIGUSR2); // force release
			}
		}
		else if(entry.cmode == CHANNEL_MODE_PPPOE){
			char *p_index= &ifname[3];
			unsigned int index= atoi(p_index);

			printf("Release PPPoE client, ifname=%s, pppoe index=%d\n",ifname,index);
			sprintf(sVal,"/bin/spppctl down %d\n", index);
			system(sVal);
		}
	}
	sleep(1);
	return 0;
}

int doRecoverWanConneciton()
{
	unsigned int entryNum=0, i=0;
	char ifname[IFNAMSIZ]={0};
	char sVal[32]={0};
	MIB_CE_ATM_VC_T entry={0};

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&entry)){
			printf("Get chain record error!\n");
			return -1;
	}

		if (entry.enable == 0)
			continue;

		ifGetName(entry.ifIndex,ifname,sizeof(ifname));

		if(entry.ipDhcp == DHCP_CLIENT ){ //DHCPC
			int dhcpc_pid;
			printf("Renew DHCPClient ifname=%s\n",ifname);
			// DHCP Client
			snprintf(sVal, 32, "%s.%s", (char*)DHCPC_PID, ifname);
			dhcpc_pid = read_pid((char*)sVal);
			if (dhcpc_pid > 0) {
				kill(dhcpc_pid, SIGUSR1); // force renew
			}
		}
		else if(entry.cmode == CHANNEL_MODE_PPPOE){
			char *p_index= &ifname[3];
			unsigned int index= atoi(p_index);

			printf("Restart PPPoE client, ifname=%s, pppoe index=%d\n",ifname,index);
			sprintf(sVal,"/bin/spppctl up %d\n", index);
			system(sVal);
		}
	}
	sleep(1);
	return 0;
}
#endif



int getifIndexByWanName(const char *name)
{
	int index=0;
	int cnt=mib_chain_total(MIB_ATM_VC_TBL);
	MIB_CE_ATM_VC_T pvcEntry;
	for(index=0;index<cnt;index++)
		{
			char itfname[256]={0};
			if(!mib_chain_get(MIB_ATM_VC_TBL,index,&pvcEntry))
				continue;
			getWanName(&pvcEntry, itfname);
			if(!strncmp(name,itfname,strlen(name)))
				return pvcEntry.ifIndex;
		}
     return -1;
}

#if defined(CONFIG_EPON_FEATURE)||defined(CONFIG_GPON_FEATURE)
//cxy 2016-8-22: led_mode: 0:led off   1:led on 2:led state restore
#include "rtk/pon_led.h"
int set_pon_led_state(int led_mode)
{
	unsigned int pon_mode=0;
	rtk_pon_led_pon_mode_t led_pon_mode=PON_LED_PON_MODE_END;

	mib_get(MIB_PON_MODE,&pon_mode);
	if(pon_mode==EPON_MODE)
		led_pon_mode=PON_LED_PON_MODE_EPON;
	else if(pon_mode==GPON_MODE)
		led_pon_mode=PON_LED_PON_MODE_GPON;
	else
		return -1;

	switch(led_mode)
	{
		case 0://pon led state led off
			rtk_pon_led_status_set(led_pon_mode, PON_LED_FORCE_OFF);
			break;
		case 1://pon led state led on
			rtk_pon_led_status_set(led_pon_mode, PON_LED_FORCE_ON);
			break;
		case 2://lpon led state restore
			rtk_pon_led_status_set(led_pon_mode, PON_LED_STATE_RESTORE);
			break;
		default:
			break;
	}

	return 0;
}

//cxy 2016-8-29:control los led. led_mode: 0:led off   1:led on 2:led state restore
int set_los_led_state(int led_mode)
{
	switch(led_mode)
	{
		case 0:
			rtk_pon_led_status_set(PON_LED_PON_MODE_OTHERS, PON_LED_FORCE_OFF);//LOS led off
			break;
		case 1:
			rtk_pon_led_status_set(PON_LED_PON_MODE_OTHERS, PON_LED_FORCE_ON);
			break;
		case 2:
			rtk_pon_led_status_set(PON_LED_PON_MODE_OTHERS, PON_LED_STATE_RESTORE);
			break;
		default:
			break;
	}
	return 0;
}
#endif

/*
 *	Convert the mib data mib_buf, from mib_get(), to a string representation.
 *	Return:
 		string
 */
int mib_to_string(char *string, const void *mib, TYPE_T type, int size)
{
	char tmp[16];
	int array_size, i, ret = 0;

	switch (type) {
	case IA_T:
		if (inet_ntop(AF_INET, mib, string, INET_ADDRSTRLEN) == NULL) {
			string[0] = '\0';
			ret = -1;
		}
		break;
#ifdef CONFIG_IPV6
	case IA6_T:
		if (inet_ntop(AF_INET6, mib, string, INET6_ADDRSTRLEN) == NULL) {
			string[0] = '\0';
			ret = -1;
		}
		break;
#endif
	case BYTE_T:
		sprintf(string, "%hhu", *(unsigned char *)mib);
		break;

	case WORD_T:
		sprintf(string, "%hu", *(unsigned short *)mib);
		break;

	case DWORD_T:
		sprintf(string, "%u", *(unsigned int *)mib);
		break;

	case INTEGER_T:
		sprintf(string, "%d", *(int *)mib);
		break;

	case BYTE5_T:
	case BYTE6_T:
	case BYTE13_T:
		string[0] = '\0';
		for (i = 0; i < size; i++) {
			sprintf(tmp, "%02x", ((unsigned char *)mib)[i]);
			strcat(string, tmp);
		}
		break;

	case BYTE_ARRAY_T:
		string[0] = '\0';
		for (i = 0; i < size; i++) {
			sprintf(tmp, "%02x", ((unsigned char *)mib)[i]);
			if (i != (size - 1))
				strcat(tmp, ",");
			strcat(string, tmp);
		}
		break;

	case WORD_ARRAY_T:
		array_size = size / sizeof(short);

		string[0] = '\0';
		for (i = 0; i < array_size; i++) {
			sprintf(tmp, "%04x", ((unsigned short *)mib)[i]);

			if (i != (array_size - 1))
				strcat(tmp, ",");

			strcat(string, tmp);
		}
		break;

	case INT_ARRAY_T:
		array_size = size / sizeof(int);

		string[0] = '\0';
		for (i = 0; i < array_size; i++) {
			sprintf(tmp, "%08x", ((unsigned int *)mib)[i]);

			if (i != (array_size - 1))
				strcat(tmp, ",");

			strcat(string, tmp);
		}
		break;
	case STRING_T:
		strcpy(string, mib);
		break;

	default:
		ret = -1;
		printf("%s: Unknown data type %d!\n", __FUNCTION__, type);
	}

	return ret;
}

#ifdef RTK_SMART_ROAMING
#define READ_BUF_SIZE        50
extern pid_t find_pid_by_name( char* pidName)
{
	DIR *dir;
	struct dirent *next;
	pid_t pid;
	
	if ( strcmp(pidName, "init")==0)
		return 1;
	
	dir = opendir("/proc");
	if (!dir) {
		printf("Cannot open /proc");
		return 0;
	}

	while ((next = readdir(dir)) != NULL) {
		FILE *status;
		char filename[READ_BUF_SIZE];
		char buffer[READ_BUF_SIZE];
		char name[READ_BUF_SIZE];

		/* Must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, "..") == 0)
			continue;

		/* If it isn't a number, we don't want it */
		if (!isdigit(*next->d_name))
			continue;

		sprintf(filename, "/proc/%s/status", next->d_name);
		if (! (status = fopen(filename, "r")) ) {
			continue;
		}
		if (fgets(buffer, READ_BUF_SIZE-1, status) == NULL) {
			fclose(status);
			continue;
		}
		fclose(status);

		/* Buffer should contain a string like "Name:   binary_name" */
		sscanf(buffer, "%*s %s", name);
		if (strcmp(name, pidName) == 0) {
		//	pidList=xrealloc( pidList, sizeof(pid_t) * (i+2));
			pid=(pid_t)strtol(next->d_name, NULL, 0);
			closedir(dir);
			return pid;
		}
	}
	closedir(dir);
	return 0;
}
#endif

/*
 *	Remove comma and blank in the string
 */
static char *to_hex_string(char *string)
{
	char *tostr;
	int i, k;
	size_t len = strlen(string);

	tostr = malloc(len + 1);
	if (tostr == NULL)
		return NULL;

	for (i = 0, k = 0; i < len; i++) {
		if (string[i] != ',' && string[i] != ' ')
			tostr[k++] = string[i];
	}
	tostr[k] = '\0';

	return tostr;
}

static int string_to_hex(char *string, unsigned char *key, int len)
{
	char tmpBuf[4];
	int idx, ii = 0;

	if (string == NULL || key == NULL)
		return 0;

	for (idx = 0; idx < len; idx += 2) {
		tmpBuf[0] = string[idx];
		tmpBuf[1] = string[idx + 1];
		tmpBuf[2] = 0;

		if (!isxdigit(tmpBuf[0]) || !isxdigit(tmpBuf[1]))
			return 0;

		key[ii++] = strtoul(tmpBuf, NULL, 16);
	}

	return 1;
}

static int string_to_short(char *string, unsigned short *key, int len)
{
	char tmpBuf[5];
	int idx, ii = 0, j;

	if (string == NULL || key == NULL)
		return 0;

	for (idx = 0; idx < len; idx += 4) {
		for (j = 0; j < 4; j++) {
			tmpBuf[j] = string[idx + j];
			if (!isxdigit(tmpBuf[j]))
				return 0;
		}

		tmpBuf[4] = 0;

		key[ii++] = strtoul(tmpBuf, NULL, 16);
	}

	return 1;
}

static int string_to_integer(char *string, unsigned int *key, int len)
{
	char tmpBuf[9];
	int idx, ii = 0, j;

	if (string == NULL || key == NULL)
		return 0;

	for (idx = 0; idx < len; idx += 8) {
		for (j = 0; j < 8; j++) {
			tmpBuf[j] = string[idx + j];
			if (!isxdigit(tmpBuf[j]))
				return 0;
		}

		tmpBuf[8] = 0;

		key[ii++] = strtoul(tmpBuf, NULL, 16);
	}

	return 1;
}

int string_to_mib(void *mib, const char *string, TYPE_T type, int size)
{
	struct in_addr *ipAddr;
#ifdef CONFIG_IPV6
	struct in6_addr *ip6Addr;
#endif
	unsigned char *vChar;
	unsigned short *vShort;
	unsigned int *vUInt;
	int *vInt;
	int ret;

	ret = 0;
	switch (type) {
	case IA_T:
		if (!strlen(string)) {
			string = "0.0.0.0";
		}
		ipAddr = mib;
		if (inet_pton(AF_INET, string, ipAddr) == 0)
			ret = -1;
		break;
#ifdef CONFIG_IPV6
	case IA6_T:
		if (!strlen(string)) {
			string = "::";
		}
		ip6Addr = mib;
		if (inet_pton(AF_INET6, string, ip6Addr) == 0)
			ret = -1;
		break;
#endif
	case BYTE_T:
		vChar = mib;
		*vChar = strtoul(string, NULL, 0);
		break;
	case WORD_T:
		vShort = mib;
		*vShort = strtoul(string, NULL, 0);
		break;
	case DWORD_T:
		vUInt = mib;
		*vUInt = strtoul(string, NULL, 0);
		break;
	case INTEGER_T:
		vInt = mib;
		*vInt = strtol(string, NULL, 0);
		break;
	case BYTE_ARRAY_T:
		string = to_hex_string((char *)string);	// remove comma and blank
		string_to_hex((char *)string, mib, size * 2);
		free((char *)string);
		break;
	case BYTE5_T:
	case BYTE6_T:
	case BYTE13_T:
		string_to_hex((char *)string, mib, size * 2);
		break;
	case STRING_T:
		strncpy(mib, string, size);
		break;
	case WORD_ARRAY_T:
		string = to_hex_string((char *)string);	// remove comma and blank
		string_to_short((char *)string, mib, size * 2);
		free((char *)string);
		break;
	case INT_ARRAY_T:
		string = to_hex_string((char *)string);	// remove comma and blank
		string_to_integer((char *)string, mib, size * 2);
		free((char *)string);
		break;
	default:
		printf("Unknown data type !\n");
		break;
	}

	return ret;
}

int before_upload(const char *fname)
{
	int ret = 1;

#ifdef CONFIG_USER_XMLCONFIG
	if (ret = va_cmd(shell_name, 3, 1, "/etc/scripts/config_xmlconfig.sh", "-s", fname)) {
		fprintf(stderr, "exec /etc/scripts/config_xmlconfig.sh error!\n");
	}
#else
#ifdef CONFIG_USE_XML
	if (ret = va_cmd("/bin/saveconfig", 4, 1, "-f", fname, "-t", "xml")) {
		fprintf(stderr, "exec /bin/saveconfig error!\n");
	}
#else
	if (ret = va_cmd("/bin/saveconfig", 4, 1, "-f", fname, "-t", "raw")) {
		fprintf(stderr, "exec /bin/saveconfig error!\n");
	}
#endif
#endif

	return ret;
}

int after_download(const char *fname)
{
	int ret = 1;

#ifdef CONFIG_USER_XMLCONFIG
	if (ret = va_cmd(shell_name, 3, 1, "/etc/scripts/config_xmlconfig.sh", "-l", fname)) {
		fprintf(stderr, "exec /etc/scripts/config_xmlconfig.sh error!\n");
	}
#else
#ifdef CONFIG_USE_XML
	if (ret = va_cmd("/bin/loadconfig", 4, 1, "-f", fname, "-t", "xml")) {
		fprintf(stderr, "exec /bin/loadconfig error!\n");
	}
#else
	if (ret = va_cmd("/bin/loadconfig", 4, 1, "-f", fname, "-t", "raw")) {
		fprintf(stderr, "exec /bin/loadconfig error!\n");
	}
#endif
#endif

	return ret;
}

#ifdef CONFIG_00R0
void reSync_reStartVoIP()
{
	printf("Now calling voip_flash_server_update()\n");
	voip_flash_server_update();
	system("echo x > /var/run/solar_control.fifo");
}

int checkAndModifyWanServiceType(int serv_type_to_check, int change_type, int index)
{
	unsigned int totalEntry;
	MIB_CE_ATM_VC_T Entry;
	int i,ret;

	//if change_type is WAN_CHANGETYPE_ADD, just need to search ATM_VC_TBL found the one has the same applicationtype
	//but if change_type is WAN_CHANGETYPE_MODIFY, need to search ATM_VC_TBL found the one has the same applicationtype
	//but not with the same index.

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL); 
	
	for (i=0; i<totalEntry; i++) {
		if (mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
			if((change_type==WAN_CHANGETYPE_MODIFY) && (i==index))
				continue;

			if(Entry.applicationtype & serv_type_to_check){
				printf("Wan %d, Entry.applicationtype changed from 0x%x to",i,Entry.applicationtype);
				Entry.applicationtype &= ~serv_type_to_check;
				if(Entry.applicationtype ==0 )
					Entry.applicationtype = X_CT_SRV_OTHER ;	
			
				printf(" 0x%x\n",Entry.applicationtype);
				mib_chain_update(MIB_ATM_VC_TBL, (void *)&Entry, i);

				if(serv_type_to_check & X_CT_SRV_VOICE ){
					//If the modify service type is VOICE, need to sync share memory for VoIP used
					//and restart voip, so need special handle, return 1
					return 1; //Means voip need to do flash re-sync and restart
				}
			}
		}
	}
}

/* record the singal 1490 timer, use for wizard's requirement  */
#define SIGNAL_1490_END_TIMER "/var/setSignal1490_end_timer"
void setSignal1490_end_timer() //Record the time for receieve the SIGNAL1490 signals
{
	struct sysinfo signal_1490_info;
	sysinfo(&signal_1490_info);
	//printf(" setSignal1490_end_timer signal_1490_info.uptime = %d \n", signal_1490_info.uptime);
	int ret=0;
	char buf[64];
	sprintf(buf, "echo %ld > %s", signal_1490_info.uptime, SIGNAL_1490_END_TIMER);
	system(buf);
}

long getSignal1490_end_timer()//Report the time when receieved the SIGNAL1490 signals
{
	FILE *fp;
	int signal1490_timer=0;

	if(!(fp = fopen(SIGNAL_1490_END_TIMER, "r")))
		return 0;

	fscanf(fp, "%d\n", &signal1490_timer);
	fclose(fp);
	//printf(" getSignal1490_end_timer signal1490_timer = %d \n", signal1490_timer);
	
	return signal1490_timer;
}
#endif

#if defined(CONFIG_RTL9607C) || defined(CONFIG_RTL8686)
typedef enum
{
	UNI_PORT_NONE=0,
	UNI_PORT_FE,
	UNI_PORT_GE
}UniPortCapabilityType_t;

void setupUniPortCapability()
{
	FILE *fp;
	char line[64];
	fp=fopen(PROC_UNI_CAPABILITY,"r");
	char *delim = ";";
	char * pch;
	char cmd[128];
	int portIdx=0;
	rtk_port_phy_ability_t ability;
	while(fgets(line, 64, fp)){
		if(line[0]=='#'){
			continue;
		}
		fclose(fp);
		printf ("%s:%d line is %s\n", __FUNCTION__, __LINE__, line);
		pch = strtok(line,delim);
		while (pch != NULL)
		{
	  		printf ("%s:%d pch is %s\n",__FUNCTION__, __LINE__,pch);
			if(atoi(pch) == UNI_PORT_NONE){
				ability.Half_10 = 0;
				ability.Full_10 = 0;
				ability.Half_100 = 0;
				ability.Full_100 = 0;
				ability.Half_1000 = 0;
				ability.Full_1000 = 0;
				rtk_port_phyAutoNegoAbility_set(portIdx, &ability);
				printf ("%s:%d pch is %s, port %d, UNI_PORT_NONE\n",__FUNCTION__, __LINE__,pch, portIdx);
				//sprintf(cmd, "diag port set auto-nego port %d ability", portIdx);
			}
			else if(atoi(pch) == UNI_PORT_FE){
				ability.Half_10 = 1;
				ability.Full_10 = 1;
				ability.Half_100 = 1;
				ability.Full_100 = 1;
				ability.Half_1000 = 0;
				ability.Full_1000 = 0;
				rtk_port_phyAutoNegoAbility_set(portIdx, &ability);
				printf ("%s:%d pch is %s, port %d, UNI_PORT_FE\n",__FUNCTION__, __LINE__,pch, portIdx);
				//sprintf(cmd, "diag port set auto-nego port %d ability 10h 10f 100h 100f flow-control asy-flow-control", portIdx);
			}
			else { //UNI_PORT_GE
				ability.Half_10 = 1;
				ability.Full_10 = 1;
				ability.Half_100 = 1;
				ability.Full_100 = 1;
				ability.Half_1000 = 1;
				ability.Full_1000 = 1;
				rtk_port_phyAutoNegoAbility_set(portIdx, &ability);
				printf ("%s:%d pch is %s, port %d, UNI_PORT_GE\n",__FUNCTION__, __LINE__,pch, portIdx);
				//sprintf(cmd, "diag port set auto-nego port %d ability 10h 10f 100h 100f 1000f flow-control asy-flow-control", portIdx);
			}
			
	  		pch = strtok (NULL, delim);
			portIdx++;
		} 
		break;
	}
}
#endif

#if 1
/* operations for bsd flock(), also used by the kernel implementation */
#define LOCK_SH		1	/* shared lock */
#define LOCK_EX		2	/* exclusive lock */
#define LOCK_NB		4	/* or'd with one of the above to prevent
				   blocking */
#define LOCK_UN		8	/* remove lock */

#define LOCK_MAND	32	/* This is a mandatory flock ... */
#define LOCK_READ	64	/* which allows concurrent read operations */
#define LOCK_WRITE	128	/* which allows concurrent write operations */
#define LOCK_RW		192	/* which allows concurrent read & write ops */

int unlock_file_by_flock(int lockfd)
{
	while (flock(lockfd, LOCK_UN) == -1 && errno==EINTR) {
		usleep(1000);
		printf("pkt loss write unlock failed by flock. errno=%d\n", errno);
	}
	close(lockfd);
	return 0;
}

int lock_file_by_flock(const char *filename, int wait)
{
	int lockfd;

	if ((lockfd = open(filename, O_RDWR|O_CREAT)) == -1) {
		perror("open shm lockfile");
		return lockfd;
	}

	if(wait)
	{
		 while (flock(lockfd, LOCK_EX) == -1 && errno==EINTR) {  //wait util lock can been use.
		 	usleep(1000);
		 	printf("file %s write lock failed by flock. errno=%d\n", filename, errno);
		 }		
	}
	else
	{
		if(flock(lockfd, LOCK_EX|LOCK_NB) == -1)
		{
			close(lockfd);  //if failed to lock it. close lockfd.
			printf("fail to lock file %s\n", filename);
			return -1;
		}
	}

	return lockfd;
}
#endif

#if defined(CONFIG_RTL9600_SERIES) && defined(CONFIG_RTK_L34_ENABLE)
#define INTERNAL_VID_START	2000
#define INTERNAL_VID_END	3000
int mib_internal_vid_list[] = {MIB_LAN_VLAN_ID1, MIB_LAN_VLAN_ID2, MIB_UNTAG_WAN_VLAN_ID, MIB_FWD_CPU_VLAN_ID, MIB_FWD_PROTO_BLOCK_VLAN_ID, MIB_FWD_BIND_INTERNET_VLAN_ID, MIB_FWD_BIND_OTHER_VLAN_ID, 0};
int checkVlanConfictWithMib(int internalVid, int mibId)
{
	unsigned int atmVcEntryNum, i;
	MIB_CE_ATM_VC_T atmVcEntry;
	unsigned int pbNum, k;
#if defined(CONFIG_00R0) && defined(CONFIG_RTK_L34_ENABLE)
	MIB_CE_PORT_BINDING_T pbEntry;
#endif
	struct v_pair *vid_pair;
	

	if(!internalVid)
		return 0;
	
	/* WAN config or MVLAN */
	atmVcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<atmVcEntryNum; i++) 
	{
		if (mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&atmVcEntry))
		{
			if (atmVcEntry.enable == 0)
				continue;

			if(mibId==MIB_FWD_BIND_OTHER_VLAN_ID) 
			{
				if(atmVcEntry.vlan && (atmVcEntry.vid >= internalVid && atmVcEntry.vid <= (internalVid+DEFAULT_BIND_LAN_OFFSET))){
					return 1;
				}

				if(atmVcEntry.mVid >= internalVid && atmVcEntry.mVid <= (internalVid+DEFAULT_BIND_LAN_OFFSET)){
					return 1;
				}
			}
			else
			{
				if(atmVcEntry.vlan && atmVcEntry.vid == internalVid){
					return 1;
				}

				if(atmVcEntry.mVid == internalVid){
					return 1;
				}
			}
		}
	}

#if defined(CONFIG_00R0) && defined(CONFIG_RTK_L34_ENABLE)
	/* VLAN binding */
	pbNum = mib_chain_total(MIB_PORT_BINDING_TBL);
	for (i=0; i<pbNum; i++)
	{
		if (mib_chain_get(MIB_PORT_BINDING_TBL, i, (void*)&pbEntry))
		{
			if(pbEntry.pb_mode != (unsigned char)VLAN_BASED_MODE)
				continue;

			vid_pair = (struct v_pair *)&pbEntry.pb_vlan0_a;
			for (k=0; k<4; k++)
			{			
				//Be sure the content of vlan-mapping exsit!
				if ((vid_pair[k].vid_b==internalVid || vid_pair[k].vid_a==internalVid) && vid_pair[k].vid_a!=0)
				{
					return 1;
				}
			}
		}
	}
#endif

	return 0;
}

int checkVlanConfictWithInternal(int internalVid, int mibId)
{
	int i, vlan_id;

	if(!internalVid)
		return 0;
	
	for(i=0 ; mib_internal_vid_list[i] ; i++)
	{
		if(mib_internal_vid_list[i]==mibId)
			continue;
	
		if(mib_get(mib_internal_vid_list[i], (void *)&vlan_id) != 0)
		{
			if(mib_internal_vid_list[i]==MIB_FWD_BIND_OTHER_VLAN_ID) {
				if(internalVid >= vlan_id && internalVid <= (vlan_id+DEFAULT_BIND_LAN_OFFSET)){
					return 1;
				}
			} else if(mibId==MIB_FWD_BIND_OTHER_VLAN_ID) {
				if(vlan_id >= internalVid && vlan_id <= (internalVid+DEFAULT_BIND_LAN_OFFSET)){
					return 1;
				}
			} else {
				if(vlan_id == internalVid){
					return 1;
				}
			}
		}
	}

	return 0;
}

int decideInternalVlan(int mibId)
{
	int vlan_id;

	for(vlan_id=INTERNAL_VID_START ; vlan_id<=INTERNAL_VID_END ; vlan_id++)
	{
		if(!checkVlanConfictWithMib(vlan_id, mibId) && !checkVlanConfictWithInternal(vlan_id, mibId))
			return vlan_id;
	}

	return -1;
}

void restartup_RG(void) 
{
	va_cmd(STARTUP, 1, 1, "RG");
}

void checkVlanConfict(void)
{
	int conflictHappend = 0;
	int i, vlan_id, new_vlan_id;
	int bind_other_vlan_id;

	for(i=0 ; mib_internal_vid_list[i] ; i++)
	{
		if(mib_get(mib_internal_vid_list[i], (void *)&vlan_id) != 0)
		{
			if(checkVlanConfictWithMib(vlan_id, mib_internal_vid_list[i]) || checkVlanConfictWithInternal(vlan_id, mib_internal_vid_list[i]))
			{
				new_vlan_id = decideInternalVlan(mib_internal_vid_list[i]);				
				if(new_vlan_id != -1) {
					mib_set(mib_internal_vid_list[i], (void *)&new_vlan_id);
					conflictHappend = 1;
				}
			}
		}
	}

	if(conflictHappend)
	{
		AUG_PRT(" NOTE: VLAN ID conflict happend !!\n");
		restartup_RG();
	}
}

void restartOMCI(void)
{
#ifdef CONFIG_EPON_FEATURE
	unsigned int pon_mode=0;
#endif

#ifdef CONFIG_GPON_FEATURE
	mib_get(MIB_PON_MODE, &pon_mode);
	if(pon_mode == GPON_MODE)
	{
		va_cmd(OMCICLI, 2, 1, "debug", "reactive");
	}
#endif
}
#endif

#ifdef CONFIG_RTK_DEV_AP
/* there is no vwlan_idx in boa */
int SetWlan_idx(char * wlan_iface_name)
{
	int idx;
	idx = atoi(&wlan_iface_name[4]);
	if (idx >= NUM_WLAN_INTERFACE) {
		printf("invalid wlan interface index number!\n");
		return 0;
	}
	wlan_idx = idx;
	
	return 1;		
}

#define READ_BUF_SIZE	50
pid_t find_pid_by_name( char* pidName)
{
	DIR *dir;
	struct dirent *next;
	pid_t pid;
	
	if ( strcmp(pidName, "init")==0)
		return 1;
	
	dir = opendir("/proc");
	if (!dir) {
		printf("Cannot open /proc");
		return 0;
	}

	while ((next = readdir(dir)) != NULL) {
		FILE *status;
		char filename[READ_BUF_SIZE];
		char buffer[READ_BUF_SIZE];
		char name[READ_BUF_SIZE];

		/* Must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, "..") == 0)
			continue;

		/* If it isn't a number, we don't want it */
		if (!isdigit(*next->d_name))
			continue;

		sprintf(filename, "/proc/%s/status", next->d_name);
		if (! (status = fopen(filename, "r")) ) {
			continue;
		}
		if (fgets(buffer, READ_BUF_SIZE-1, status) == NULL) {
			fclose(status);
			continue;
		}
		fclose(status);

		/* Buffer should contain a string like "Name:   binary_name" */
		sscanf(buffer, "%*s %s", name);
		if (strcmp(name, pidName) == 0) {
		//	pidList=xrealloc( pidList, sizeof(pid_t) * (i+2));
			pid=(pid_t)strtol(next->d_name, NULL, 0);
			closedir(dir);
			return pid;
		}
	}
	closedir(dir);
	return 0;
}

int isFileExist(char *file_name)
{
	struct stat status;

	if ( stat(file_name, &status) < 0)
		return 0;

	return 1;
}

int if_readlist_proc(char *target, char *key, char *exclude)
{
	FILE *fh;
	char buf[512];
	char *s, name[16], tmp_str[16];
	int iface_num=0;
	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh) {
		return 0;
	}
	fgets(buf, sizeof buf, fh);	/* eat line */
	fgets(buf, sizeof buf, fh);
	while (fgets(buf, sizeof buf, fh)) {
		s = get_name(name, buf);

		#ifdef CONFIG_8021Q_VLAN_SUPPORTED
		if(strstr(name, "."))
			continue;
		#endif
		
		if(strstr(name, key)){
			iface_num++;
			if(target[0]==0x0){
				sprintf(target, "%s", name);
			}else{
				sprintf(tmp_str, " %s", name);
				strcat(target, tmp_str);
			}
		}
		//printf("iface name=%s, key=%s\n", name, key);
	}
	
	fclose(fh);
	return iface_num;
}
#endif

void checkIGMPMLDProxySnooping(int isIGMPenable, int isMLDenable, int isIGMPProxyEnable, int isMLDProxyEnable)
{
	AUG_PRT("isIGMPenable=%d, isMLDenable=%d, isIGMPProxyEnable=%d, isMLDProxyEnable=%d\n", isIGMPenable, isMLDenable, isIGMPProxyEnable, isMLDProxyEnable);
	if(isIGMPProxyEnable && isMLDProxyEnable)
	{
		if(isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 1 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 1 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 1 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 1 > /proc/rg/mld_trap_to_PS");
		}
	}
	else if(!isIGMPProxyEnable && isMLDProxyEnable)
	{
		if(isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 1 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 2 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 2 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 1 > /proc/rg/mld_trap_to_PS");
		}
	}
	else if(isIGMPProxyEnable && !isMLDProxyEnable)
	{
		if(isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 1 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 1 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 1 > /proc/rg/mcast_protocol");
			system("/bin/echo 1 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
	}
	else if(!isIGMPProxyEnable && !isMLDProxyEnable)
	{
		if(isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 1 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && isMLDenable)
		{
			system("/bin/echo 1 > /proc/rg/igmpSnooping");
			system("/bin/echo 2 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
		else if (!isIGMPenable && !isMLDenable)
		{
			system("/bin/echo 0 > /proc/rg/igmpSnooping");
			system("/bin/echo 0 > /proc/rg/mcast_protocol");
			system("/bin/echo 0 > /proc/rg/igmp_trap_to_PS");
			system("/bin/echo 0 > /proc/rg/mld_trap_to_PS");
		}
	}
}

