/*
 *      System routines for IP QoS
 *
 */

#include "debug.h"
#include "utility.h"
#include <signal.h>
#include <string.h>
#include <netdb.h>

#define QOS_SETUP_DEBUG   1// for qos debug
#define MAX_PORT(val1, val2) (val1>val2?val1:val2)
#define MIN_PORT(val1, val2) (val1<val2?val1:val2)
const char QOS_IP_CHAIN[] =  "qos_ip_rules";
const char QOS_EB_CHAIN[] =  "qos_eb_rules";
const char QOS_RTP_CHAIN[] =  "qos_rtp_rules";
const char CAR_FILTER_CHAIN[] =  "car_filter";
const char CAR_TRAFFIC_CHAIN[] =  "car_traffic_qos";
static unsigned int qos_setup_debug = 3;


#ifdef QOS_SETUP_DEBUG
#define QOS_SETUP_PRINT_FUNCTION                    \
    do{if(qos_setup_debug&0x1) fprintf(stderr,"%s: %s  %d\n", __FILE__, __FUNCTION__,__LINE__);}while(0);

#define QOS_SETUP_PRINT(fmt, args...)        \
    do{if(qos_setup_debug&0x2) fprintf(stderr,fmt, ##args);}while(0)
#else
#define QOS_SETUP_PRINT_FUNCTION do{}while(0);
#define QOS_SETUP_PRINT(fmt, args...) do{}while(0)
#endif

#define RTP_RULE_NUM_MAX     256

#define QOS_WAN_ENABLE		0x01
#define QOS_WAN_VLAN_ENABLE	0x02
#define QOS_WAN_BRIDGE		0x04
#define QOS_WAN_TR069		0x08

#define QOS_APP_NONE		0
#define QOS_APP_VOIP		1
#define QOS_APP_TR069		2

// QoS definitions, and pass these information to linux kernel
#define QOS_8021P_OFFSET 		0	
#define QOS_8021P_MASK		(0x7<<QOS_8021P_OFFSET)

#define QOS_SWQID_OFFSET		3
#define QOS_SWQID_MASK		(0xF<<QOS_SWQID_OFFSET)

#define QOS_1P_ENABLE		(1<<8)
#define QOS_ENTRY_OFFSET 	9
#define QOS_ENTRY_MASK		(0x3F<<QOS_ENTRY_OFFSET)

#define SW_VLAN_ENABLE		(1<<7)
#define QOS_VLAN_ID_OFFSET 	20	
#define QOS_VLAN_ID_MASK		(0xFFF<<QOS_VLAN_ID_OFFSET)
#define QOS_MARK_MASK		QOS_VLAN_ID_MASK|QOS_ENTRY_MASK|QOS_1P_ENABLE|SW_VLAN_ENABLE|QOS_SWQID_MASK|QOS_8021P_MASK
#ifdef QOS_SUPPORT_RTP
#define RTP_PORT_FILE	"/proc/net/ip_qos_rtp"
#endif

static char* proto2str[] = {
    [0]" ",
    [1]"-p TCP",
    [2]"-p UDP",
	[3]"-p ICMP",
};

static char* proto2str2layer[] = {
    [0]" ",
    [1]"--ip-proto 6",
    [2]"--ip-proto 17",
	[3]"--ip-proto 1",
};

#ifdef CONFIG_IPV6
static char* proto2str_v6[] = {
    [0]" ",
    [1]"-p TCP",
    [2]"-p UDP",
	[3]"-p ICMPV6",
};

static char* proto2str2layer_v6[] = {
    [0]" ",
    [1]"--ip6-proto 6",
    [2]"--ip6-proto 17",
	[3]"--ip6-proto 58",
};
#endif

static unsigned int current_uprate;
unsigned int getUpLinkRate(void);


#ifdef CONFIG_HWNAT
#include "hwnat_ioctl.h"

static int fill_ip_family_common_rule(struct rtl867x_hwnat_qos_rule *qos_rule, MIB_CE_IP_QOS_Tp qEntry){
	//printf("qEntry->sip %x\n", *((struct in_addr *)qEntry->sip));
	//printf("qEntry->dip %x\n", *((struct in_addr *)qEntry->dip));
	qos_rule->Format_IP_SIP = (((struct in_addr *)qEntry->sip)->s_addr);
	if(qEntry->smaskbit != 0)
		qos_rule->Format_IP_SIP_M = ~((1<<(32-qEntry->smaskbit)) - 1);
	qos_rule->Format_IP_DIP = (((struct in_addr *)qEntry->dip)->s_addr);
	if(qEntry->dmaskbit != 0)
		qos_rule->Format_IP_DIP_M = ~((1<<(32-qEntry->dmaskbit)) - 1);
	return 0;
}

static int fill_ip_rule(struct rtl867x_hwnat_qos_rule *qos_rule, MIB_CE_IP_QOS_Tp qEntry){
	fill_ip_family_common_rule(qos_rule, qEntry);
	// protocol
	if (qEntry->protoType != PROTO_NONE) {//not none
		if((qEntry->flags&EXC_PROTOCOL) != EXC_PROTOCOL){//EXC_PROTOCOL didn't set, we don't support
			qos_rule->Format_IP_Proto_M = 0xff;
			if (qEntry->protoType == PROTO_TCP)
				qos_rule->Format_IP_Proto = 6;
			else if (qEntry->protoType == PROTO_UDP)
				qos_rule->Format_IP_Proto = 17;
			else if (qEntry->protoType == PROTO_ICMP)
				qos_rule->Format_IP_Proto = 1;
			else
				qos_rule->Format_IP_Proto_M = 0x0;
		}
	}
	return 0;
}

static int fill_tcp_rule(struct rtl867x_hwnat_qos_rule *qos_rule, MIB_CE_IP_QOS_Tp qEntry){
	fill_ip_family_common_rule(qos_rule, qEntry);
	qos_rule->Format_TCP_SPORT_Sta = qEntry->sPort;
	qos_rule->Format_TCP_SPORT_End = qEntry->sPortRangeMax;
	qos_rule->Format_TCP_DPORT_Sta = qEntry->dPort;
	qos_rule->Format_TCP_DPORT_End = qEntry->dPortRangeMax;
	return 0;
}

static int fill_udp_rule(struct rtl867x_hwnat_qos_rule *qos_rule, MIB_CE_IP_QOS_Tp qEntry){
	fill_ip_family_common_rule(qos_rule, qEntry);
	qos_rule->Format_UDP_SPORT_Sta = qEntry->sPort;
	qos_rule->Format_UDP_SPORT_End = qEntry->sPortRangeMax;
	qos_rule->Format_UDP_DPORT_Sta = qEntry->dPort;
	qos_rule->Format_UDP_DPORT_End = qEntry->dPortRangeMax;
	return 0;
}

static int hwnat_qos_translate_rule(MIB_CE_IP_QOS_Tp qEntry){
	hwnat_ioctl_cmd hifr;
	struct hwnat_ioctl_qos_cmd hiqc;
	struct rtl867x_hwnat_qos_rule *qos_rule;
	MIB_CE_ATM_VC_T pvcEntry;

	if ( qEntry->outif != DUMMY_IFINDEX ) {
		if( (MEDIA_INDEX(qEntry->outif) != MEDIA_ETH)
		  #ifdef CONFIG_PTMWAN
		   && (MEDIA_INDEX(qEntry->outif) != MEDIA_PTM)
		  #endif /*CONFIG_PTMWAN*/
		   )
			return -1;
	}
	memset(&hiqc, 0, sizeof(hiqc));
	hifr.type = HWNAT_IOCTL_QOS_TYPE;
	hifr.data = &hiqc;
	hiqc.type = QOSRULE_CMD;
	qos_rule = &(hiqc.u.qos_rule.qos_rule_pattern);
	//type decide
	//Warning! now we don't support exclude....
	//and we only try IP TYPE now...
	qos_rule->rule_type = RTL867x_IPQos_Format_IP;//default
	
	switch(qEntry->ipqos_rule_type)
	{
		case IPQOS_PORT_BASE:
			strncpy(qos_rule->Format_SRCPort_NETIFNAME, ELANVIF[qEntry->phyPort-1], strlen(ELANVIF[qEntry->phyPort-1]));
			//printf("qos_rule->Format_SRCPort_NETIFNAME=%s, ALIASNAME_ELAN0=%s\n", qos_rule->Format_SRCPort_NETIFNAME, ALIASNAME_ELAN0);
			qos_rule->rule_type = RTL867x_IPQos_Format_srcPort;
			break;
		case IPQOS_MAC_BASE:
		case IPQOS_ETHERTYPE_BASE:
			qos_rule->rule_type = RTL867x_IPQos_Format_Ethernet;
			break;
		case IPQOS_IP_PROTO_BASE:
			if (qEntry->protoType == PROTO_TCP)
				qos_rule->rule_type = RTL867x_IPQos_Format_TCP;
			else if (qEntry->protoType == PROTO_UDP)
				qos_rule->rule_type = RTL867x_IPQos_Format_UDP;
			else //if (qEntry->protoType == PROTO_ICMP)
				qos_rule->rule_type = RTL867x_IPQos_Format_IP;
			break;
		case IPQOS_8021P_BASE:
			qos_rule->rule_type = RTL867x_IPQos_Format_8021p;
			break;
		default:
			printf("Error! No HW QoS rule type matched, should not go here!\n");
			break;
	}

RULE_TYPE_OK:

	//remark dscp
/*DSCP modify Boyce 2014-06-06*/
#if 1 //defined(CONFIG_RTL_FTTDP_SUPPORT)
                hiqc.u.qos_rule.qos_rule_remark_dscp = (qEntry->m_dscp>>2) + (qEntry->m_dscp&0x1);
#else
#ifdef QOS_DSCP
	if(qEntry->dscp==0) {
#endif
		// Kaohj -- use DSCP target to set IP-preced
		if (qEntry->m_ipprio) {
			hiqc.u.qos_rule.qos_rule_remark_dscp = (qEntry->m_ipprio-1) << 3;
		}
		if (qEntry->m_iptos != 0xff) {
			//seems we need to shift left 2 bits to fit DSCP's format?
			hiqc.u.qos_rule.qos_rule_remark_dscp |= ((qEntry->m_iptos)>>2);
		}
#ifdef QOS_DSCP
	}
	else if (qEntry->dscp == 1) {
		if (qEntry->m_ipprio != 0 || qEntry->m_iptos != 0) {
			hiqc.u.qos_rule.qos_rule_remark_dscp = qEntry->m_ipprio << 3;
			hiqc.u.qos_rule.qos_rule_remark_dscp |= qEntry->m_iptos << 1;
		}
	}
#endif
        
#endif

	//802.1p remark
	if(qEntry->m_1p != 0){
		hiqc.u.qos_rule.qos_rule_remark_8021p = qEntry->m_1p;	// 802.1p
	}

#if defined(CONFIG_RTL8685) || defined(CONFIG_RTL8685S) || defined(CONFIG_RTL8685SB)
	//vid remark
	if(qEntry->m_vid !=0){
		hiqc.u.qos_rule.qos_rule_vid = qEntry->m_vid;	// VLAN ID
	}
#endif
	//q_index
	hiqc.u.qos_rule.qos_rule_queue_index = qEntry->prior;  //(1, 2, 3, 4)
	//fill the rule
	switch(qos_rule->rule_type){
		case RTL867x_IPQos_Format_srcPort:
			break;
		case RTL867x_IPQos_Format_Ethernet:			
			{
				uint8 maskMac[MAC_ADDR_LEN]={0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
				memcpy(qos_rule->Format_Ethernet_SMAC.octet, qEntry->smac, MAC_ADDR_LEN);
				memcpy(qos_rule->Format_Ethernet_DMAC.octet, qEntry->dmac, MAC_ADDR_LEN);
				if (memcmp(qEntry->smac, EMPTY_MAC, MAC_ADDR_LEN))
					memcpy(qos_rule->Format_Ethernet_SMAC_M.octet, maskMac, MAC_ADDR_LEN);
				if (memcmp(qEntry->dmac, EMPTY_MAC, MAC_ADDR_LEN))
					memcpy(qos_rule->Format_Ethernet_DMAC_M.octet, maskMac, MAC_ADDR_LEN);

                                if(0 != qEntry->ethType) {
					qos_rule->Format_Ethernet_TYPE = qEntry->ethType;
                                        qos_rule->Format_Ethernet_TYPE_M = 0xffff;
			
			}
           }
			break;
                        
		case RTL867x_IPQos_Format_IP:
			fill_ip_rule(qos_rule, qEntry);
                        /*DSCP modify Boyce 2014-06-06
                        qEntry->qosDscp  
                        b'0=0 don't care dscp rule 
                        b'0=1 also compare dscp value  
                        */
                                if(qEntry->qosDscp & 0x1)
                                {
                                        qos_rule->Format_IP_TOS =qEntry->qosDscp-1;
                                        qos_rule->Format_IP_TOS_M= 0xfC;  //only mask dscpField
                                }else
                                {
                                        qos_rule->Format_IP_TOS_M= 0x0;
                                }
			break;

		case RTL867x_IPQos_Format_TCP:
			fill_tcp_rule(qos_rule, qEntry);
                        /*DSCP modify Boyce 2014-06-06
                        qEntry->qosDscp  
                        b'0=0 don't care dscp rule
                        b'0=1 also compare dscp value  
                        */
                                if(qEntry->qosDscp & 0x1)
                                {
                                        qos_rule->Format_IP_TOS =qEntry->qosDscp-1;
                                        qos_rule->Format_IP_TOS_M= 0xfC;  //only mask dscpField
                                }else
                                {
                                        qos_rule->Format_IP_TOS_M= 0x0;
                                }
			break;

		case RTL867x_IPQos_Format_UDP:
			fill_udp_rule(qos_rule, qEntry);
                        /*DSCP modify Boyce 2014-06-06
                        qEntry->qosDscp  
                        b'0=0 don't care dscp rule 
                        b'0=1 also compare dscp value  
                        */
                                if(qEntry->qosDscp & 0x1)
                                {
                                        qos_rule->Format_IP_TOS =qEntry->qosDscp-1;
                                        qos_rule->Format_IP_TOS_M= 0xfC;  //only mask dscpField
                                }else
                                {
                                        qos_rule->Format_IP_TOS_M= 0x0;
                                }
			break;
                        
		// Added by Mason Yu. for 802.1p match
		case RTL867x_IPQos_Format_8021p:
			//printf("***** set hwnat rule for 801.1p match(%d)\n", (qEntry->vlan1p-1));
			qos_rule->Format_8021P_PRIORITY = qEntry->vlan1p-1;
			qos_rule->Format_8021P_PRIORITY_M = 0x07;
			break;
		default:
			break;
	}
	send_to_hwnat(hifr);
	return 0;
}

static int hwnat_qos_queue_setup(void){
	hwnat_ioctl_cmd hifr;
	struct hwnat_ioctl_qos_cmd hiqc;
	struct rtl867x_hwnat_qos_queue *qos_queue;
	unsigned char policy=0;
	int qEntryNum=0, j=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	unsigned int total_bandwidth = 0;

	mib_get(MIB_QOS_POLICY, (void*)&policy);
	if((qEntryNum=mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		return 0;

	total_bandwidth = getUpLinkRate();

	memset(&hiqc, 0, sizeof(hiqc));
	hifr.type = HWNAT_IOCTL_QOS_TYPE;
	hifr.data = &hiqc;
	hiqc.type = OUTPUTQ_CMD;
	qos_queue = &(hiqc.u.qos_queue);
	qos_queue->action = QDISC_ENABLE;
	if(policy == PLY_PRIO){
		qos_queue->sp_num = 4;
	}else if(policy == PLY_WRR){
		qos_queue->wrr_num = 4;
		for(j=0; j<qEntryNum; j++){
			if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, j, (void*)&qEntry))
				continue;
			if ( !qEntry.enable)
				continue;				

			qos_queue->weight[j] = qEntry.weight;
			qos_queue->ceil[j] = (total_bandwidth*1024);
		}
	}
	//qos_queue->bandwidth = -1;
	qos_queue->default_queue = 3;
	send_to_hwnat(hifr);
	return 0;
}

static int hwnat_qos_bandwidth_setup(int total_tandwidth){
	hwnat_ioctl_cmd hifr;
	struct hwnat_ioctl_qos_cmd hiqc;

	memset(&hiqc, 0, sizeof(hiqc));
	hifr.type = HWNAT_IOCTL_QOS_TYPE;
	hifr.data = &hiqc;
	hiqc.type = OUTPUTQ_BWCTL_CMD;
	hiqc.qos_total_bandwidth = total_tandwidth;
	send_to_hwnat(hifr);
	return 0;
}

static int hwnat_qos_queue_stop(void){
	hwnat_ioctl_cmd hifr;
	struct hwnat_ioctl_qos_cmd hiqc;
	struct rtl867x_hwnat_qos_queue *qos_queue;

	memset(&hiqc, 0, sizeof(hiqc));
	hifr.type = HWNAT_IOCTL_QOS_TYPE;
	hifr.data = &hiqc;
	hiqc.type = OUTPUTQ_CMD;
	qos_queue = &(hiqc.u.qos_queue);
	qos_queue->action = QDISC_DISABLE;
	send_to_hwnat(hifr);
	return 0;
}

#else
static int hwnat_qos_default_rule(void){
	return 0;
}
static int hwnat_qos_translate_rule(MIB_CE_IP_QOS_Tp qEntry){
	return 0;
}
static int hwnat_qos_queue_setup(void){
	return 0;
}
static int hwnat_qos_queue_stop(void){
	return 0;
}
static int hwnat_qos_bandwidth_setup(int total_tandwidth){
	return 0;
}
#endif

int findQueueMinWeigh()
{
	int nBytes=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, i;

	if((qEntryNum=mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		return nBytes;
	for(i=0;i<qEntryNum; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i, (void*)&qEntry))
			continue;
		if ( !qEntry.enable)
			continue;

		if(nBytes==0)
			nBytes = qEntry.weight;

		if(qEntry.weight < nBytes)
			nBytes = qEntry.weight;
	}
	return nBytes;
}

#ifdef CONFIG_00R0
unsigned int get_next_QoSclass_instnum()
{
	unsigned int num = 0, instNum = 0, i = 0, MAXinstNum = 0, used = 0;
	MIB_CE_IP_QOS_T *p, qos_entity;

	p = &qos_entity;
	num = mib_chain_total(MIB_IP_QOS_TBL);

	// find MAX instnum
	for (i = 0; i < num; i++) {
		if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)p))
			continue;

		if (p->InstanceNum > MAXinstNum) {
			MAXinstNum = p->InstanceNum;
		}
	}

	// find available instnum
	for (instNum = 1; instNum <= MAXinstNum; instNum++) {
		used = 0;
		for (i = 0; i < num; i++) {
			if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)p))
				continue;

			if (p->InstanceNum == instNum) {
				used = 1;
				break;
			}
		}

		if (used == 0) {
			fprintf(stderr, "[%s:%s@%d] get %d \n", __FILE__, __FUNCTION__, __LINE__, instNum);
			return instNum;
		}
	}

	if (used == 1) {
		fprintf(stderr, "[%s:%s@%d] get %d \n", __FILE__, __FUNCTION__, __LINE__, (MAXinstNum + 1));
		return (MAXinstNum + 1);
	}
	else {
		fprintf(stderr, "[%s:%s@%d] get %d \n", __FILE__, __FUNCTION__, __LINE__, (num + 1));
		return (num + 1);
	}
}
#endif

int ifSumWeighIs100()
{
	int sum=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, i;

	if((qEntryNum=mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		return 0;
	for(i=0;i<qEntryNum; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i, (void*)&qEntry))
			continue;
		if ( !qEntry.enable)
			continue;
		sum += qEntry.weight;
	}

	if (sum == 100)
		return 1;
	else
		return 0;
}

int isQueneEnableForRule(unsigned char prior)
{
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, j;

	if((qEntryNum=mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		return 0;
	for(j=0;j<qEntryNum; j++)
	{
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, j, (void*)&qEntry))
			continue;
		if (j == (prior-1))
		{
			if (qEntry.enable)
				return 1;
			break;
		}
	}
	return 0;
}
int SumWeightOfWRR()
{
	int sum=0;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, i;

	if((qEntryNum=mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		return 0;
	for(i=0;i<qEntryNum; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, i, (void*)&qEntry))
			continue;
		if(!qEntry.enable)
			continue;
		sum += qEntry.weight;
	}

	return sum;
}

int set_wrr_class_qdisc()
{
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, minWeigh, j;
#ifdef QOS_SETUP_IFB
	char *devname="ifb0";
#else
	char *devname="imq0";
#endif
	char s_classid[16], s_handle[16], s_qlen[16];
	int totalWeight = SumWeightOfWRR();
	if((qEntryNum=mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		return 0;
	for(j=0;j<qEntryNum; j++)
	{
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, j, (void*)&qEntry))
			continue;
		if ( !qEntry.enable)
			continue;

		snprintf(s_classid, 16, "1:%d00", j+1);
		snprintf(s_handle, 16, "%d00:", j+1);
		minWeigh = findQueueMinWeigh();
		snprintf(s_qlen, 16, "%d", 30*qEntry.weight/totalWeight);
		//printf("set_wrr_class_qdisc: s_qlen=%s\n", s_qlen);

		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
		"parent", s_classid, "handle", s_handle, "pfifo", "limit", s_qlen);

	}
	return 1;
}

//get state about qos from pvc interface, 0: false, 1: true
#ifdef BR_ROUTE_ONEPVC
static unsigned int get_pvc_qos_state(unsigned int ifIndex, unsigned char cmode)
#else
static unsigned int get_pvc_qos_state(unsigned int ifIndex)
#endif
{
	MIB_CE_ATM_VC_T entry;
	int i, entryNum = 0;
	unsigned int ret = 0;

	// Mason Yu.
	if ( ifIndex == DUMMY_IFINDEX )
		return QOS_WAN_VLAN_ENABLE;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i = 0; i< entryNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
			continue;

#ifdef BR_ROUTE_ONEPVC
		if (entry.cmode != cmode)
			continue;
#endif
		if(entry.ifIndex == ifIndex)
		{
			if(entry.enableIpQos)
				ret |= QOS_WAN_ENABLE;
			if(entry.vlan)
				ret |= QOS_WAN_VLAN_ENABLE;
#ifndef BR_ROUTE_ONEPVC
			if (entry.cmode == CHANNEL_MODE_BRIDGE)
				ret |= QOS_WAN_BRIDGE;
#endif
			// Mason Yu.
#if 0
			/*ql:20080825 START: for tr069 pvc, just set priority according to wanport*/
			if (entry.applicationtype&X_CT_SRV_TR069)
			//if (entry.applicationtype == APPTYPE_TR069)//tr069
				ret |= QOS_WAN_TR069;
			/*ql:20080825 END*/
#endif
			break;
		}
	}

	return ret;
}

/****************************************
* getUpLinkRate:
* DESC: get upstream link rate.
****************************************/
unsigned int getWanUpLinkRate(void)
{
	unsigned int total_bandwidth = 102400;//default to be 100Mbps

#ifdef CONFIG_DEV_xDSL
	Modem_LinkSpeed vLs;
	unsigned char ret = 0;

	ret = adsl_drv_get(RLCM_GET_LINK_SPEED, (void *)&vLs, RLCM_GET_LINK_SPEED_SIZE);
	if (ret) {
		if(0 != vLs.upstreamRate)
			total_bandwidth = vLs.upstreamRate;
	}else
#endif
	{
		// Ethernet upLink: 100Mbps
		total_bandwidth = 102400;
	}

	mib_set(MIB_QOS_UPRATE, (void *)&total_bandwidth);
	return ((total_bandwidth*93)/100);

}


unsigned int getUpLinkRate()
{
	unsigned int total_bandwidth = 1024;//default to be 1Mbps
	unsigned char policy = 0;

	if(mib_get(MIB_TOTAL_BANDWIDTH_LIMIT_EN, &policy))
	{
		if(policy ==0 ){

			/* Use auto selection */
			return getWanUpLinkRate();
			
		}else{
			//  Because we can not distinguish total bandwidth. It is for total bandwidth limit. Let user to input total bandwidth.
			if(mib_get(MIB_TOTAL_BANDWIDTH, &total_bandwidth))
			{
				return total_bandwidth;
			}
		}
	}
	else
		return 1024;

	return 1024;
}

// QoS Enable

#ifdef BR_ROUTE_ONEPVC
static int isPvcQosEnable(MIB_CE_ATM_VC_Tp brgEntry)
{
	MIB_CE_ATM_VC_T vcEntry;
	int i, entryNum;

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0; i<entryNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry)||!vcEntry.br_route_flag)
			continue;

		if ((vcEntry.vpi==brgEntry->vpi) && (vcEntry.vci==brgEntry->vci))
		{
			if (vcEntry.cmode == CHANNEL_MODE_IPOE)
			{
				if (vcEntry.enableIpQos)
					return 1;
				else
					return 0;
			}
		}
	}

	return 0;
}
#endif


int getQosEnable()
{
	unsigned char qosEnable;

	mib_get(MIB_QOS_ENABLE_QOS, (void*)&qosEnable);
	return (int)qosEnable;
}

int getQosRuleNum()
{
	MIB_CE_IP_QOS_T entry;
	int i, ruleNum;
	int sum=0;

	ruleNum = mib_chain_total(MIB_IP_QOS_TBL);
	if (0 == ruleNum)
		return 0;

	for (i=0; i<ruleNum; i++)
	{
		if (!mib_chain_get(MIB_IP_QOS_TBL, i, (void *)&entry) || !entry.enable)
			continue;

		sum++;
	}

	return(sum);
}

static int isQosMacRule(MIB_CE_IP_QOS_Tp pEntry)
{
	if (pEntry->protoType || pEntry->sip || pEntry->dip || pEntry->sPort ||
		pEntry->dPort
//#ifdef QOS_DIFFSERV
		|| pEntry->qosDscp
//#endif
		)
		return 0;
	return 1;
}

static int mac_hex2string(unsigned char *hex, char *str)
{
	snprintf(str, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
		hex[0], hex[1],	hex[2], hex[3],	hex[4], hex[5]);
	return 0;
}

static int setupCarChain(unsigned int enable)
{
	QOS_SETUP_PRINT_FUNCTION

	if (enable)
	{
		va_cmd(IPTABLES, 2, 1, "-N", CAR_FILTER_CHAIN);
		va_cmd(IPTABLES, 5, 1, "-I", "FORWARD", "1", "-j", CAR_FILTER_CHAIN);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", CAR_TRAFFIC_CHAIN);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", CAR_TRAFFIC_CHAIN);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", "pvc_mark");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", "pvc_mark");

		#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 2, 1, "-N", CAR_FILTER_CHAIN);
		va_cmd(IP6TABLES, 5, 1, "-I", "FORWARD", "1", "-j", CAR_FILTER_CHAIN);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", CAR_TRAFFIC_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", CAR_TRAFFIC_CHAIN);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", "pvc_mark");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", "pvc_mark");
		#endif
	} else {
		va_cmd(IPTABLES, 2, 1, "-F", CAR_FILTER_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", CAR_TRAFFIC_CHAIN);

		va_cmd(IPTABLES, 4, 1, "-D", "FORWARD", "-j", CAR_FILTER_CHAIN);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", "pvc_mark");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", CAR_TRAFFIC_CHAIN);

		va_cmd(IPTABLES, 2, 1, "-X", CAR_FILTER_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", "pvc_mark");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", CAR_TRAFFIC_CHAIN);

		#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 2, 1, "-F", CAR_FILTER_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", CAR_TRAFFIC_CHAIN);

		va_cmd(IP6TABLES, 4, 1, "-D", "FORWARD", "-j", CAR_FILTER_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", "pvc_mark");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", CAR_TRAFFIC_CHAIN);

		va_cmd(IP6TABLES, 2, 1, "-X", CAR_FILTER_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", "pvc_mark");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", CAR_TRAFFIC_CHAIN);
		#endif
	}

	return 1;
}

/*
 *                              HTB(root qdisc, handle 10:)
 *                               |
 *                              HTB(root class, classid 10:1)
 *            ___________________|_____________________
 *            |         |        |          |          |
 *           HTB       HTB      HTB        HTB        HTB
 *(subclass id 10:10 rate Xkbit)........       (sub class id 10:N0 rate Ykbit)
 */
static int setupCarQdisc(unsigned int enable)
{
	MIB_CE_ATM_VC_T vcEntry;
	int i, vcEntryNum;
	char ifname[IFNAMSIZ];
	char s_rate[16], s_ceil[16];
	unsigned char totalBandWidthEn;
	unsigned int bandwidth;
	unsigned int rate, ceil;

	QOS_SETUP_PRINT_FUNCTION;

	mib_get(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&totalBandWidthEn);

	if (totalBandWidthEn) {
		mib_get(MIB_TOTAL_BANDWIDTH, &bandwidth);
	}
	else
		bandwidth = getUpLinkRate();

	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<vcEntryNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry)||!vcEntry.enableIpQos)
			continue;

		ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));

		if (!enable) {
			va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", ifname, "root");
			continue;
		}
#ifdef BR_ROUTE_ONEPVC
		if (vcEntry.br_route_flag)
		{
			if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
			{//check if route itf has enabled qos
				if (isPvcQosEnable(&vcEntry))
					continue;
			}
		}
#endif
		//patch: actual bandwidth maybe a little greater than configured limit value, so I minish 7% of the configured limit value ahead.
		if (totalBandWidthEn)
			//ceil = bandwidth/100 * 93;
			ceil = bandwidth*93/100;
		else
			ceil = bandwidth;

		// delete default prio qdisc
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", ifname, "root");
		//tc qdisc add dev $DEV root handle 1: htb default 2 r2q 1		
		va_cmd(TC, 12, 1, "qdisc", (char *)ARG_ADD, "dev", ifname,
			"root", "handle", "1:", "htb", "default", "2", "r2q", "1");

		// root class
		snprintf(s_rate, 16, "%dKbit", ceil);
		//tc class add dev $DEV parent 1: classid 1:1 htb rate $RATE ceil $CEIL
		va_cmd(TC, 17, 1, "class", (char *)ARG_ADD, "dev", ifname,
			"parent", "1:", "classid", "1:1", "htb", "rate", s_rate, "ceil", s_rate,
			"mpu", "64", "overhead", "4");

		//patch with above
		rate = (ceil>10)?10:ceil;

		// default class
		snprintf(s_rate, 16, "%dKbit", rate);
		snprintf(s_ceil, 16, "%dKbit", ceil);
		//tc class add dev $DEV parent 1:1 classid 1:2 htb rate $RATE ceil $CEIL
		va_cmd(TC, 17, 1, "class", (char *)ARG_ADD, "dev", ifname,
			"parent", "1:1", "classid", "1:2", "htb", "rate",
			s_rate, "ceil", s_ceil, "mpu", "64", "overhead", "4");

		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", ifname,
			"parent", "1:2", "handle", "2:", "pfifo", "limit", "100");
	}

	return 0;
}

static int setupCarRule_one(MIB_CE_IP_TC_Tp entry)
{
	char ifname[IFNAMSIZ]={0};
	char* tc_act = NULL, *fw_act=NULL;
	char* proto1 = NULL, *proto2 = NULL;
	char wanPort[16]={0};
	char  saddr[55], daddr[55], sport[16], dport[16];
	char tmpstr[48];
	int upLinkRate=0, childRate=0;
	int mark;
	char *iptables_cmd=NULL, *tc_protocol=NULL;
	DOCMDINIT;

	QOS_SETUP_PRINT_FUNCTION;

	tc_act = (char*)ARG_ADD;
	fw_act = (char*)FW_ADD;

	if(NULL == entry) {
		printf("Null traffic contolling rule!\n");
		return 1;
	}

#ifndef CONFIG_RTK_L34_ENABLE //In RG platform,no specify interface
	ifGetName(entry->ifIndex, ifname, sizeof(ifname));
	//wan interface
	if (ifname[0] != '\0')
		snprintf(wanPort, 16, "-o %s", ifname);
#endif

#ifdef CONFIG_IPV6
	// This is a IPv4 rule
	if ( entry->IpProtocol == IPVER_IPV4 ) {
		//source address and netmask
		snprintf(tmpstr, 16, "%s", inet_ntoa(*((struct in_addr *)entry->srcip)));
		if (strcmp(tmpstr, ARG_0x4)) {
			if(0 != entry->smaskbits) {
				snprintf(saddr, 24, "-s %s/%d", tmpstr, entry->smaskbits);
			} else {
				snprintf(saddr, 24, "-s %s", tmpstr);
			}
		}
		else {//if not specify the source ip
			saddr[0] = '\0';
		}
	}
	// This is a IPv6 rule
	else if ( entry->IpProtocol == IPVER_IPV6 ) {
		//source address and netmask
		inet_ntop(PF_INET6, (struct in6_addr *)entry->sip6, tmpstr, sizeof(tmpstr));
		if (strcmp(tmpstr, "::")) {
			if(0 != entry->sip6PrefixLen) {
				snprintf(saddr, 55, "-s %s/%d", tmpstr, entry->sip6PrefixLen);
			} else {
				snprintf(saddr, 55, "-s %s", tmpstr);
			}
		}
		else {//if not specify the source ip
			saddr[0] = '\0';
		}
	}
#else
	//source address and netmask
	snprintf(tmpstr, 16, "%s", inet_ntoa(*((struct in_addr *)entry->srcip)));
	if (strcmp(tmpstr, ARG_0x4)) {
		if(0 != entry->smaskbits) {
			snprintf(saddr, 24, "-s %s/%d", tmpstr, entry->smaskbits);
		} else {
			snprintf(saddr, 24, "-s %s", tmpstr);
		}
	}
	else {//if not specify the source ip
		saddr[0] = '\0';
	}
#endif

#ifdef CONFIG_IPV6
	// This is a IPv4 rule
	if ( entry->IpProtocol == IPVER_IPV4 ) {
		//destination address and netmask
		snprintf(tmpstr, 16, "%s", inet_ntoa(*((struct in_addr *)entry->dstip)));
		if (strcmp(tmpstr, ARG_0x4)) {
			if(0 != entry->dmaskbits) {
				snprintf(daddr, 24, "-d %s/%d", tmpstr, entry->dmaskbits);
			} else {
				snprintf(daddr, 24, "-d %s", tmpstr);
			}
		} else {//if not specify the dest ip
			daddr[0] = '\0';
		}
	}
	// This is a IPv6 rule
	else if ( entry->IpProtocol == IPVER_IPV6 ) {
		//destination address and netmask
		inet_ntop(PF_INET6, (struct in6_addr *)entry->dip6, tmpstr, sizeof(tmpstr));
		if (strcmp(tmpstr, "::")) {
			if(0 != entry->dip6PrefixLen) {
				snprintf(daddr, 55, "-d %s/%d", tmpstr, entry->dip6PrefixLen);
			} else {
				snprintf(daddr, 55, "-d %s", tmpstr);
			}
		} else {//if not specify the dest ip
			daddr[0] = '\0';
		}
	}
#else
	//destination address and netmask
	snprintf(tmpstr, 16, "%s", inet_ntoa(*((struct in_addr *)entry->dstip)));
	if (strcmp(tmpstr, ARG_0x4)) {
		if(0 != entry->dmaskbits) {
			snprintf(daddr, 24, "-d %s/%d", tmpstr, entry->dmaskbits);
		} else {
			snprintf(daddr, 24, "-d %s", tmpstr);
		}
	} else {//if not specify the dest ip
		daddr[0] = '\0';
	}
#endif

	//source port
	if(0 != entry->sport) {
		snprintf(sport, 16, "--sport %d", entry->sport);
	} else {
		sport[0] = '\0';
	}

	//destination port
	if(0 != entry->dport) {
		snprintf(dport, 16, "--dport %d", entry->dport);
	} else {
		dport[0] = '\0';
	}

	//protocol
	//if (((0 != entry->sport) || (0 != entry->dport)) &&
	//	(entry->protoType < 2))
	//	entry->protoType = 4;

	if(entry->protoType>4)//wrong protocol index
	{
		printf("Wrong protocol\n");
		return 1;
	} else {
		switch(entry->protoType)
		{
			case PROTO_NONE://NONE
				proto1 = " ";
				break;
			case PROTO_ICMP://ICMP
				#ifdef CONFIG_IPV6
				// This is a IPv4 rule
				if ( entry->IpProtocol == IPVER_IPV4 )
					proto1 = "-p ICMP";
				// This is a IPv6 rule
				else if ( entry->IpProtocol == IPVER_IPV6 )
					proto1 = "-p ICMPV6";
				#else
				proto1 = "-p ICMP";
				#endif
				break;
			case PROTO_TCP://TCP
				proto1 = "-p TCP";
				break;
			case PROTO_UDP://UDP
				proto1 = "-p UDP";
				break;
			case PROTO_UDPTCP://TCP/UDP
				proto1 = "-p TCP";
				proto2 = "-p UDP";
				break;
		}
	}

#ifdef CONFIG_IPV6
		// This is a IPv4 rule
		if ( entry->IpProtocol == IPVER_IPV4 ) {
			iptables_cmd = "/bin/iptables";
			tc_protocol = "protocol ip";
		}
		// This is a IPv6 rule
		else {
			iptables_cmd = "/bin/ip6tables";
			tc_protocol = "protocol ipv6";
		}
#else
		// This is a IPv4 rule
		iptables_cmd = "/bin/iptables";
		tc_protocol = "protocol ip";
#endif

	upLinkRate = entry->limitSpeed;
	if(0 != upLinkRate)
	{
		//get mark
		mark = (entry->entryid<<12);

		//patch: true bandwidth will be a little greater than limit value, so I minish 7% of set limit value ahead.
		int ceil;
		//ceil = upLinkRate/100 * 93;
		ceil = upLinkRate*93/100;

		childRate = (10>ceil)?ceil:10;
		//childRate = ceil;

		DOCMDARGVS(TC, DOWAIT, "class %s dev %s parent 1:1 classid 1:%d0 htb rate %dkbit ceil %dkbit mpu 64 overhead 4",
			tc_act, ifname, entry->entryid, childRate, ceil);

		DOCMDARGVS(TC, DOWAIT, "qdisc %s dev %s parent 1:%d0 handle %d1: pfifo limit 100",
			tc_act, ifname, entry->entryid, entry->entryid);

		//DOCMDARGVS(TC, DOWAIT, "filter %s dev %s parent 1: protocol ip prio 0 handle 0x%x fw flowid 1:%d0",
		//	tc_act, ifname, mark, entry->entryid);
		DOCMDARGVS(TC, DOWAIT, "filter %s dev %s parent 1: %s prio 0 handle 0x%x fw flowid 1:%d0",
			tc_act, ifname, tc_protocol, mark, entry->entryid);

		DOCMDARGVS(iptables_cmd, DOWAIT,  "-t mangle %s %s %s %s %s %s %s %s -j MARK --set-mark 0x%x",
			fw_act, CAR_TRAFFIC_CHAIN, wanPort, proto1, saddr, daddr, sport, dport, mark);

		/*TCP/UDP?*/
		if(proto2)//setup the other protocol
		{
			DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle %s %s %s %s %s %s %s %s -j MARK --set-mark 0x%x",
				fw_act, CAR_TRAFFIC_CHAIN, wanPort, proto2, saddr, daddr, sport, dport, mark);
		}
	}
	else
	{//if uprate=0, forbid traffic matching the rules
		DOCMDARGVS(iptables_cmd, DOWAIT, "-t filter %s %s %s %s %s %s %s %s -j DROP",
			fw_act, CAR_FILTER_CHAIN, wanPort, proto1, saddr, daddr, sport, dport);

		/*TCP/UDP again*/
		if(proto2)
		{
			DOCMDARGVS(iptables_cmd, DOWAIT, "-t filter %s %s %s %s %s %s %s %s -j DROP",
				fw_act, CAR_FILTER_CHAIN, wanPort, proto2, saddr, daddr, sport, dport);
		}
	}

#ifdef CONFIG_RTK_L34_ENABLE
	RTK_RG_QoS_Car_Rule_Set(entry);
#endif
	return 0;
}

static int setupCarRule()
{
	int entry_num, i;
	MIB_CE_IP_TC_T  entry;

	entry_num = mib_chain_total(MIB_IP_QOS_TC_TBL);

	for(i=0; i<entry_num; i++)
	{
		if(!mib_chain_get(MIB_IP_QOS_TC_TBL, i, (void*)&entry))
			continue;

		if (setupCarRule_one(&entry))
			return 1;
	}

	return 0;
}

/*********************************************************************
 * NAME:    setup_qos_rules
 * DESC:    Using iptables to setup rules for queue, including packet
 *          marking(dscp, 802.1p). This function doesn't affect traffic
 *          controlling, just for priority and weighted round robin
 *          queue. Suppose that the mib setting is well-formated and right.
 * ARGS:    none
 * RETURN:  0 success, others fail
 *********************************************************************/
static int setup_qos_rules(unsigned char policy)
{
	MIB_CE_IP_QOS_T entry;
	MIB_CE_ATM_VC_T vcEntry;
	int i, j, EntryNum = 0, vcEntryNum = 0;
	int order;
	unsigned char enableDscpMark=1;
#ifdef QOS_SUPPORT_RTP
	FILE *fp;
	char buff[100];
	unsigned int rtp_dip;
	unsigned int rtp_dpt;
	unsigned int rtpCnt=0, rtpRuleCnt=0;
	struct rtp_struct {
		unsigned int dip;
		unsigned int dport;
	} __PACK__;
	struct rtp_rule_st {
		char daddr[48];
		char dport[48];
	} __PACK__;

	struct rtp_struct rtp_entry[RTP_RULE_NUM_MAX];
	struct rtp_rule_st rtp_rule_entry[RTP_RULE_NUM_MAX];
#endif

	DOCMDINIT

	QOS_SETUP_PRINT_FUNCTION
/*
	if(PLY_WRR==policy)//weighted round robin
	{
		if (!ifSumWeighIs100())
			return 1;
	}
*/
	//get current RTP stream info
#ifdef QOS_SUPPORT_RTP
	if (!(fp=fopen(RTP_PORT_FILE, "r"))) {
		printf("no RTP connection!\n");
	} else {
		while ( fgets(buff, sizeof(buff), fp) != NULL ) {
			if(sscanf(buff, "dip=%d dport=%d", &rtp_dip, &rtp_dpt)!=2) {
				printf("Unsuported rtp format\n");
				break;
			}
			else {
				rtpCnt++;
				rtp_entry[rtpCnt-1].dip = rtp_dip;
				rtp_entry[rtpCnt-1].dport = rtp_dpt;
			}
		}
		fclose(fp);
	}
#endif
	//mib_get(MIB_QOS_ENABLE_DSCP_MARK, (void *)&enableDscpMark);

	EntryNum = mib_chain_total(MIB_IP_QOS_TBL);

	for(order =1 ; order <(EntryNum+1); order++)
	{
		char ifname[IFNAMSIZ];
#ifdef _LINUX_3_18_
		char phyPort[48]={0};
#else
		char phyPort[16]={0};
#endif
		char phyPort1[16]={0};
		char wanPort[16]={0};
		char sport[48]={0}, dport[48]={0}, saddr[75]={0}, daddr[75]={0}, strmark[48]={0}, strEthType[48]={0};
		char tmpstr[48];
		char *proto1=NULL, *proto2 = NULL, *eth_proto = NULL;
		char *iptables_cmd=NULL, *tc_protocol=NULL;
		char dscp[24] = {0};
		unsigned int mark=0;
		char smac[48]={0}, dmac[48]={0};
		//ql
		unsigned int wan_qos_state = 0;
		unsigned int wan_vlan_enable = 0;
		/*ql:20080825 START: for tr069 pvc, just set priority according to wanport*/
		unsigned int wan_tr069=0;
		/*ql:20080825 END*/
		unsigned int ebtable=0;
		unsigned short vid=0, qid=0;
		char dhcpopt_cmd[80]={0};

		for(i=0; i<EntryNum; i++)
		{
			if(!mib_chain_get(MIB_IP_QOS_TBL, i, (void*)&entry)||!entry.enable)
				continue;
			if(order != entry.QoSOrder)
				continue;
			break;
		}
		if(i>=EntryNum || !isQueneEnableForRule(entry.prior))
			continue;

#ifndef CONFIG_RTK_L34_ENABLE //Temperorily mark for LUNA platform, ebtable rule make let pc could not ping gateway.
#ifdef BR_ROUTE_ONEPVC
		wan_qos_state = get_pvc_qos_state(entry.outif, entry.cmode);
#else
		wan_qos_state = get_pvc_qos_state(entry.outif);
#endif
		wan_vlan_enable = wan_qos_state&QOS_WAN_VLAN_ENABLE;
		/*ql:20080825 START: for tr069 pvc, just set priority according to wanport*/
		wan_tr069 = wan_qos_state&QOS_WAN_TR069;
		/*ql:20080825 END*/
#ifdef BR_ROUTE_ONEPVC
		if ((entry.outif != DUMMY_IFINDEX) && (entry.cmode == CHANNEL_MODE_BRIDGE))
			ebtable = 1;
#else
		ebtable = wan_qos_state&QOS_WAN_BRIDGE;
#endif
		// If this rule is for All WAN interface(DUMMY_IFINDEX), use ebtables to set rule.
	    if ( entry.outif == DUMMY_IFINDEX ) {
			ebtable = 1;
		}

		//interface
		if ( entry.outif != DUMMY_IFINDEX ) {
			ifGetName(entry.outif, ifname, sizeof(ifname));

			//wan port
			snprintf(wanPort, 16, "-o %s", ifname);
		}

		if (memcmp(entry.smac, EMPTY_MAC, MAC_ADDR_LEN)) {
			mac_hex2string(entry.smac, tmpstr);
			snprintf(smac, 48, "-s %s", tmpstr);
		}
		else
			smac[0] = '\0';

		if (memcmp(entry.dmac, EMPTY_MAC, MAC_ADDR_LEN)) {
			mac_hex2string(entry.dmac, tmpstr);
			snprintf(dmac, 48, "-d %s", tmpstr);
		}
		else
			dmac[0] = '\0';

		if ((smac[0] != '\0') || (dmac[0] != '\0'))
			ebtable = 1;

		//phy port, 0: none, range:1-4
		if (entry.phyPort>=1 && entry.phyPort<=ELANVIF_NUM)
		{
			if(ebtable)
			{	
				snprintf(phyPort, 16, "-i %s", ELANVIF[entry.phyPort-1]);
			}else{
				#ifdef _LINUX_3_18_
				snprintf(phyPort, 48, "-m physdev --physdev-in %s", ELANVIF[entry.phyPort-1]);
				#else
				snprintf(phyPort, 16, "-i %s", ELANVIF[entry.phyPort-1]);
				#endif
			}
		}
		else {
			if (ebtable || (0 != entry.ethType))
			{
				/*
					Consider that 
						1. (WAN = Any)
						2. (WAN = Specific WAN) && (EtherType base)
				*/
				
				phyPort[0] = '\0';
			}else{
				#ifdef CONFIG_PPPOE_PROXY_IF_NAME
				/*ql:20080805 START: qos enable for ppp proxy*/
				snprintf(phyPort, 16, "-i br+");
				/*ql: 20080805 END*/
				#else
				snprintf(phyPort, 16, "-i br0");
				#endif
			}
		}

//#ifdef QOS_DIFFSERV
		//dscp match
		if(0 != entry.qosDscp)
		{
			#ifdef CONFIG_IPV6
			if (ebtable) {
				// This is a IPv4 rule
				if ( entry.IpProtocol == IPVER_IPV4 )
					snprintf(dscp, 24, "--ip-tos 0x%x", (entry.qosDscp-1)&0xFF);
				// This is a IPv6 rule
				else if ( entry.IpProtocol == IPVER_IPV6 )
					snprintf(dscp, 24, "--ip6-tclass 0x%x", (entry.qosDscp-1)&0xFF);
			}
			#else
			if (ebtable)
				snprintf(dscp, 24, "--ip-tos 0x%x", (entry.qosDscp-1)&0xFF);
			#endif
			else
				snprintf(dscp, 24, "-m dscp --dscp 0x%x", (entry.qosDscp-1)>>2);
		}else{
		    dscp[0]='\0';
		}
//#endif

		//protocol
#ifdef QOS_SUPPORT_RTP
		if(entry.protoType>5)//invalid protocol index
			return 1;
#else
		if(entry.protoType>PROTO_UDPTCP)//invalid protocol index
			return 1;
#endif

		//source port (range)
		if((PROTO_NONE == entry.protoType)||(PROTO_ICMP == entry.protoType)||0 == entry.sPort)
		{
			//if protocol is icmp or none or port not set, ignore the port
		    	sport[0] = '\0';
		}
		else
		{
			#ifdef CONFIG_IPV6
			// This is a IPv4 rule
			if ( entry.IpProtocol == IPVER_IPV4 && ebtable) {
					if(0 != entry.sPortRangeMax)
						snprintf(sport, 48, "--ip-source-port %d:%d", MIN_PORT(entry.sPort, entry.sPortRangeMax),
							MAX_PORT(entry.sPort, entry.sPortRangeMax));
					else
						snprintf(sport, 48, "--ip-source-port %d", entry.sPort);
			}
			// This is a IPv6 rule
			else if ( entry.IpProtocol == IPVER_IPV6 && ebtable) {
					if(0 != entry.sPortRangeMax)
						snprintf(sport, 48, "--ip6-source-port %d:%d", MIN_PORT(entry.sPort, entry.sPortRangeMax),
							MAX_PORT(entry.sPort, entry.sPortRangeMax));
					else
						snprintf(sport, 48, "--ip6-source-port %d", entry.sPort);
			}
			#else
			if (ebtable) {
				if(0 != entry.sPortRangeMax)
					snprintf(sport, 48, "--ip-source-port %d:%d", MIN_PORT(entry.sPort, entry.sPortRangeMax),
						MAX_PORT(entry.sPort, entry.sPortRangeMax));
				else
					snprintf(sport, 48, "--ip-source-port %d", entry.sPort);
			}
			#endif
			else {
				if(0 != entry.sPortRangeMax)
					snprintf(sport, 48, "--sport %d:%d", MIN_PORT(entry.sPort, entry.sPortRangeMax),
						MAX_PORT(entry.sPort, entry.sPortRangeMax));
				else
					snprintf(sport, 48, "--sport %d", entry.sPort);
			}
		}

#ifdef CONFIG_IPV6
		// This is a IPv4 rule
		if ( entry.IpProtocol == IPVER_IPV4 ) {
			//source address
			snprintf(tmpstr, 16, "%s", inet_ntoa(*((struct in_addr *)entry.sip)));
			if (strcmp(tmpstr, ARG_0x4)) {
				if (ebtable) {
					if(0 != entry.smaskbit)
						snprintf(saddr, 48, "--ip-source %s/%d",
							tmpstr, entry.smaskbit );
					else
						snprintf(saddr, 48, "--ip-source %s", tmpstr);
				}
				else {
					if(0 != entry.smaskbit)
						snprintf(saddr, 48, "-s %s/%d",
							tmpstr, entry.smaskbit );
					else
						snprintf(saddr, 48, "-s %s", tmpstr);
				}
			}
			else
				saddr[0]='\0';
		}
		// This is a IPv6 rule
		else if ( entry.IpProtocol == IPVER_IPV6 ) {
			//source address
			inet_ntop(PF_INET6, (struct in6_addr *)entry.sip6, tmpstr, sizeof(tmpstr));
			if (strcmp(tmpstr, "::")) {
				if (ebtable) {
					if(0 != entry.sip6PrefixLen)
						snprintf(saddr, 75, "--ip6-source %s/%d",
							tmpstr, entry.sip6PrefixLen );
					else
						snprintf(saddr, 75, "--ip6-source %s", tmpstr);
				}
				else {
					if(0 != entry.sip6PrefixLen)
						snprintf(saddr, 75, "-s %s/%d",
							tmpstr, entry.sip6PrefixLen );
					else
						snprintf(saddr, 75, "-s %s", tmpstr);
				}
			}
			else
				saddr[0]='\0';
		}
#else
		//source address
		snprintf(tmpstr, 16, "%s", inet_ntoa(*((struct in_addr *)entry.sip)));
		if (strcmp(tmpstr, ARG_0x4)) {
			if (ebtable) {
				if(0 != entry.smaskbit)
					snprintf(saddr, 48, "--ip-source %s/%d",
						tmpstr, entry.smaskbit );
				else
					snprintf(saddr, 48, "--ip-source %s", tmpstr);
			}
			else {
				if(0 != entry.smaskbit)
					snprintf(saddr, 48, "-s %s/%d",
						tmpstr, entry.smaskbit );
				else
					snprintf(saddr, 48, "-s %s", tmpstr);
			}
		}
		else
			saddr[0]='\0';
#endif

#ifdef QOS_SUPPORT_RTP
		if (entry.protoType == 5)//RTP
		{
			if (0 == rtpCnt)//no rtp stream found
				continue;

			if (ebtable)
				continue;//don't process such rule
			else
				proto1 = "-p UDP";

			rtpRuleCnt = 0;
/*star:20090407 START only parse rtp src port*/
			snprintf(tmpstr, 16, "%s", inet_ntoa(*((struct in_addr *)entry.dip)));
			if (strcmp(tmpstr, ARG_0x4)) {
				for (j=0; j<rtpCnt; j++) {
					rtpRuleCnt++;

					if(0 != entry.dmaskbit)
						snprintf(rtp_rule_entry[rtpRuleCnt-1].daddr, 26, "-d %s/%d",
							tmpstr, entry.dmaskbit );
					else
						snprintf(rtp_rule_entry[rtpRuleCnt-1].daddr, 26, "-d %s", tmpstr);

					snprintf(rtp_rule_entry[rtpRuleCnt-1].dport, 24, "--sport %d", rtp_entry[j].dport);
				}
				if (rtpRuleCnt==0)//not found .
					continue;
			} else {
				for (j=0; j<rtpCnt; j++) {
					rtpRuleCnt++;
					//snprintf(rtp_rule_entry[rtpRuleCnt-1].daddr, 26, "-d %s", inet_ntoa(*((struct in_addr*)(&rtp_entry[j].dip))));
					rtp_rule_entry[rtpRuleCnt-1].daddr[0]=0;
					snprintf(rtp_rule_entry[rtpRuleCnt-1].dport, 24, "--sport %d", rtp_entry[j].dport);
				}
			}
		}
		else
		{
#endif
			if(entry.protoType == PROTO_UDPTCP)//add by penggenyao, support two protos at the same time
			{
				#ifdef CONFIG_IPV6
				if (ebtable) {
					// This is a IPv4 rule
					if ( entry.IpProtocol == IPVER_IPV4 ) {
						proto1 = "--ip-proto 6";
						proto2 = "--ip-proto 17";
					}
					// This is a IPv6 rule
					else if ( entry.IpProtocol == IPVER_IPV6 ) {
						proto1 = "--ip6-proto 6";
						proto2 = "--ip6-proto 17";
					}
				}
				#else
				if (ebtable) {
					proto1 = "--ip-proto 6";
					proto2 = "--ip-proto 17";
				}
				#endif
				else {
				    proto1 = "-p TCP";
				    proto2 = "-p UDP";
				}
			}
			else {
				#ifdef CONFIG_IPV6
				if (ebtable) {
					// This is a IPv4 rule
					if ( entry.IpProtocol == IPVER_IPV4 )
						proto1 = proto2str2layer[entry.protoType];
					// This is a IPv6 rule
					else if ( entry.IpProtocol == IPVER_IPV6 )
						proto1 = proto2str2layer_v6[entry.protoType];
				}
				else {
					// This is a IPv4 rule
					if ( entry.IpProtocol == IPVER_IPV4 )
						proto1 = proto2str[entry.protoType];
					// This is a IPv6 rule
					else if ( entry.IpProtocol == IPVER_IPV6 )
						proto1 = proto2str_v6[entry.protoType];
				}
				#else
				if (ebtable)
					proto1 = proto2str2layer[entry.protoType];
				else
					proto1 = proto2str[entry.protoType];
				#endif

			}

			//dest port (range)
			if((PROTO_NONE == entry.protoType)||(PROTO_ICMP == entry.protoType)||0 == entry.dPort)
			{//if protocol is icmp or none or port not set, ignore the port
			    dport[0] = '\0';
			}
			else
			{
				#ifdef CONFIG_IPV6
				// This is a IPv4 rule
				if ( entry.IpProtocol == IPVER_IPV4 && ebtable) {
						if(0 != entry.dPortRangeMax)
							snprintf(dport, 48, "--ip-destination-port %d:%d", MIN_PORT(entry.dPort, entry.dPortRangeMax),
								MAX_PORT(entry.dPort, entry.dPortRangeMax));
						else
							snprintf(dport, 48, "--ip-destination-port %d", entry.dPort);
				}
				// This is a IPv6 rule
				else if ( entry.IpProtocol == IPVER_IPV6 && ebtable) {
						if(0 != entry.dPortRangeMax)
							snprintf(dport, 48, "--ip6-destination-port %d:%d", MIN_PORT(entry.dPort, entry.dPortRangeMax),
								MAX_PORT(entry.dPort, entry.dPortRangeMax));
						else
							snprintf(dport, 48, "--ip6-destination-port %d", entry.dPort);
				}
				#else
				if (ebtable) {
					if(0 != entry.dPortRangeMax)
						snprintf(dport, 48, "--ip-destination-port %d:%d", MIN_PORT(entry.dPort, entry.dPortRangeMax),
							MAX_PORT(entry.dPort, entry.dPortRangeMax));
					else
						snprintf(dport, 48, "--ip-destination-port %d", entry.dPort);
				}
				#endif
				else {
				    if(0 != entry.dPortRangeMax)
						snprintf(dport, 48, "--dport %d:%d", MIN_PORT(entry.dPort, entry.dPortRangeMax),
							MAX_PORT(entry.dPort, entry.dPortRangeMax));
				    else
						snprintf(dport, 48, "--dport %d", entry.dPort);
				}
			}

#ifdef CONFIG_IPV6
			// This is a IPv4 rule
			if ( entry.IpProtocol == IPVER_IPV4 ) {
				//dest address
				snprintf(tmpstr, 16, "%s", inet_ntoa(*((struct in_addr *)entry.dip)));
				if (strcmp(tmpstr, ARG_0x4)) {
					if (ebtable) {
						if(0 != entry.dmaskbit)
							snprintf(daddr, 48, "--ip-destination %s/%d",
								tmpstr, entry.dmaskbit );
						else
							snprintf(daddr, 48, "--ip-destination %s", tmpstr);
					} else {
						if(0 != entry.dmaskbit)
							snprintf(daddr, 48, "-d %s/%d",
								tmpstr, entry.dmaskbit );
						else
							snprintf(daddr, 48, "-d %s", tmpstr);
					}
				}
				else
					daddr[0]='\0';
			}
			// This is a IPv6 rule
			else if ( entry.IpProtocol == IPVER_IPV6 ){
				//dest address
				inet_ntop(PF_INET6, (struct in6_addr *)entry.dip6, tmpstr, sizeof(tmpstr));
				if (strcmp(tmpstr, "::")) {
					if (ebtable) {
						if(0 != entry.dip6PrefixLen)
							snprintf(daddr, 75, "--ip6-destination %s/%d",
								tmpstr, entry.dip6PrefixLen );
						else
							snprintf(daddr, 75, "--ip6-destination %s", tmpstr);
					}
					else {
						if(0 != entry.dip6PrefixLen)
							snprintf(daddr, 75, "-d %s/%d",
								tmpstr, entry.dip6PrefixLen );
						else
							snprintf(daddr, 75, "-d %s", tmpstr);
					}
				}
				else {
					daddr[0]='\0';
				}
			}
#else
			//dest address
			snprintf(tmpstr, 16, "%s", inet_ntoa(*((struct in_addr *)entry.dip)));
			if (strcmp(tmpstr, ARG_0x4)) {
				if (ebtable) {
					if(0 != entry.dmaskbit)
						snprintf(daddr, 48, "--ip-destination %s/%d",
							tmpstr, entry.dmaskbit );
					else
						snprintf(daddr, 48, "--ip-destination %s", tmpstr);
				} else {
					if(0 != entry.dmaskbit)
						snprintf(daddr, 48, "-d %s/%d",
							tmpstr, entry.dmaskbit );
					else
						snprintf(daddr, 48, "-d %s", tmpstr);
				}
			}
			else
				daddr[0]='\0';
#endif

#ifdef QOS_SUPPORT_RTP
		}
#endif

		//lan 802.1p mark, 0-2 bit(match)
		if(0 != entry.vlan1p) {
			// Mason Yu
			ebtable=1;
			phyPort[0] = '\0';						
			snprintf(strmark, 48, "--vlan-prio %d", (entry.vlan1p-1)&0xff);
		} else
			strmark[0] = '\0';

		if (ebtable) {
			if(strmark[0] != '\0')//vlan 802.1p priority, use 802.1Q ethernet protocol
			{
				eth_proto = "-p 0x8100";
			}
			else {//use ipv4 for ethernet protocol
#ifdef CONFIG_IPV6
		if(entry.ipqos_rule_type==IPQOS_IP_PROTO_BASE)
		{
				// This is a IPv4 rule
				if ( entry.IpProtocol == IPVER_IPV4 )
					eth_proto = "-p 0x0800";
				// This is a IPv6 rule
				else if ( entry.IpProtocol == IPVER_IPV6 )
					eth_proto = "-p 0x86dd";
		}
		else  if(entry.ipqos_rule_type==IPQOS_DHCPOPT_BASE)
			eth_proto = "-p 0x0800";
				else
					eth_proto = " ";
#else
				if (!isQosMacRule(&entry))
					eth_proto = "-p 0x0800";
				else
					eth_proto = " ";
#endif
			}
		}
		
		// Match Ethernet Type		
		if(0 != entry.ethType) {			
			ebtable=1;									
			snprintf(strEthType, 48, "-p 0x%x", entry.ethType);
			eth_proto = strEthType;
		} else
			strEthType[0] = '\0';

#ifdef CONFIG_BRIDGE_EBT_DHCP_OPT
		if(entry.ipqos_rule_type==IPQOS_DHCPOPT_BASE)
		{
			char duid_mac[20]={0};
			// duid mac
			snprintf(duid_mac, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
					entry.duid_mac[0], entry.duid_mac[1],
					entry.duid_mac[2], entry.duid_mac[3],
					entry.duid_mac[4], entry.duid_mac[5]);
			switch(entry.dhcpopt_type)
			{
				case IPQOS_DHCPOPT_60:
					//EX:ebtables -A FORWARD --dhcp-opt60 ABC -j DROP
					sprintf(dhcpopt_cmd,"--dhcp-opt60 \"%s\"",entry.opt60_vendorclass);
					break;
				case IPQOS_DHCPOPT_61:
					switch(entry.opt61_duid_type)
					{
						case 0:  //llt , EX: ebtables -A FORWARD --dhcp-opt61 1234/llt/1/1234/00:12:34:56:12:13 -j DROP
							sprintf(dhcpopt_cmd,"--dhcp-opt61 %d/llt/%d/%d/%s",entry.opt61_iaid,entry.duid_hw_type,entry.duid_time,duid_mac);
							break;
						case 1:  //en , EX: ebtables -A FORWARD --dhcp-opt61 1234/en/1234/ABCDE -j DROP
							sprintf(dhcpopt_cmd,"--dhcp-opt61 %d/en/%d/%s",entry.opt61_iaid,entry.duid_ent_num,entry.duid_ent_id);
							break;
						case 2:  //ll, EX: ebtables -A FORWARD --dhcp-opt61 1234/ll/1/00:12:34:56:12:13 -j DROP
							sprintf(dhcpopt_cmd,"--dhcp-opt61 %d/ll/%d/%s",entry.opt61_iaid,entry.duid_hw_type,duid_mac);
							break;
					}
					break;
				case IPQOS_DHCPOPT_125:
					//EX: ebtables -A FORWARD --dhcp-opt125 1234//DEF// -j DROP
					sprintf(dhcpopt_cmd,"--dhcp-opt125 %d/%s/%s/%s/%s",
							entry.opt125_ent_num,
							entry.opt125_manufacturer,
							entry.opt125_product_class,
							entry.opt125_model,
							entry.opt125_serial);
					break;
				default:
					break;
			}
			eth_proto = "-p 0x0800";
		}
		printf("dhcpopt_cmd:[%s]\n",dhcpopt_cmd);
#endif // CONFIG_BRIDGE_EBT_DHCP_OPT

		//wan 802.1p mark 0-2 bit: 802.1p, 8-15 bit: entry id
		// We can not use 0 to as tc handle. So we use (i+1).

		//======================================================================
		//
		// For port based/protocol based/Source IP based ....etc VLAN function.
		//
		// Using Mark to let driver know how to insert 802.1Q header.
		// The rule is as following design.
		//
		// VLAN ID  	[31:20](12 bits): value should be 1~4095
		// LAN Mark 	[19:15]( 5 bits): LAN MARK used for port-mapping
		// Qos Entry 	[14:9]( 6 bits): Qos Entry ID
		// Qos Enable  	[8]( 1 bit) : Qos Enable Indicator (802.1p remarking)
		// VLAN Enable  [7]( 1 bit) : VLAN ID Enable Indicator
		// SW QID	[6:3]( 4 bits): Queue ID (1~4)
		// 802.1p     	[2:0]( 3 bits): 802.1p value. (0~7)
		//
		//        28      24      20      16      12      8       4       0   
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		// |    VLAN ID[31:20]     |LAN Mark |Qos EntryID|Q|V| SW QID| p   |
		// |       (12bits)        | 5bits   | [14:9](6b)|1|1| [6:3] |[2:0]|
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		//======================================================================

		//vid, if VLAN ID is configured(value>0), 
		//     then set it into bit [31:20] and turn on bit 7(VLAN enable)
		if(entry.m_vid>0)
		{
			vid= entry.m_vid;
			mark |= (vid) <<QOS_VLAN_ID_OFFSET | SW_VLAN_ENABLE;
		}

#if defined(CONFIG_RTL_ADV_FAST_PATH) || defined(SUPPORT_BYPASS_RG_FWDENGINE)
		if(entry.prior>0)
		{
			qid= entry.prior;
			mark |= (qid) <<QOS_SWQID_OFFSET;
		}
#endif
		
		if((0 != entry.m_1p)) {
			mark |= (entry.m_1p-1)&QOS_8021P_MASK;
			//ql 20090106 START: set bit 16 of nfmark when tag 802.1p
			mark |= (((i+1)<<QOS_ENTRY_OFFSET) |QOS_1P_ENABLE);
		}
		else {
			mark |= ((i+1)<<QOS_ENTRY_OFFSET);
		}

#ifdef CONFIG_IPV6
		// This is a IPv6 rule
		if ( entry.IpProtocol == IPVER_IPV6 ){
			iptables_cmd = "/bin/ip6tables";
			tc_protocol = "protocol all";
		}
		else  {  // This is a IPv4 rule or no specify protocol. (EX: LAN Port)
			iptables_cmd = "/bin/iptables";
			tc_protocol = "protocol all";
		}

#else
		// This is a IPv4 rule
		iptables_cmd = "/bin/iptables";
		tc_protocol = "protocol all";
#endif

#ifdef QOS_SUPPORT_RTP
		if (entry.protoType != 5)//RTP
		{
#endif
			if(ebtable){

				// 2014/3/4 Because DHCP Option 60 may contain space, and original way DOCMDARGVS 
				//	        will seperate argument by space, it's not workable. So, change this way 
				//	        by using system function call.

				/* Set DSCP value */
				if( enableDscpMark && (0 != entry.m_dscp))
				{
					char cmd_buf[200]={0};
					sprintf(cmd_buf,"%s -t %s -A %s %s %s %s %s %s %s %s %s %s %s %s %s -j ftos --set-ftos 0x%x", 
							EBTABLES, "broute", QOS_EB_CHAIN, phyPort, eth_proto, (proto1==NULL)?" ":proto1, smac, dmac, saddr, sport, daddr, dport, dscp, strmark, dhcpopt_cmd, (entry.m_dscp-1)&0xFF);
					system(cmd_buf);
					if(proto2)
					{
						sprintf(cmd_buf,"%s -t %s -A %s %s %s %s %s %s %s %s %s %s %s %s %s -j ftos --set-ftos 0x%x", 
								EBTABLES, "broute", QOS_EB_CHAIN, phyPort, eth_proto, proto2, smac, dmac, saddr, sport, daddr, dport, dscp, strmark, dhcpopt_cmd, (entry.m_dscp-1)&0xFF);
						system(cmd_buf);
					}
				}

				/* Set Mark value */
				if(entry.ipqos_rule_type==IPQOS_ETHERTYPE_BASE)
				{
					DOCMDARGVS(EBTABLES, DOWAIT, "-t %s -A %s %s -j mark --mark-or 0x%x --mark-target ACCEPT", "broute",
							QOS_EB_CHAIN, eth_proto, mark);
				
				}
				else
				{
					// 2014/3/4 Because DHCP Option 60 may contain space, and original way DOCMDARGVS 
					//			will seperate argument by space, it's not workable. So, change this way 
					//			by using system function call.
					// 2014/3/4 --mark-target ACCEPT, if match, ACCEPT it, not use default CONTINUE, because the rule may
					//			match for multiple rules, and the mark value may not be correct.		
					char cmd_buf[200]={0};
					DOCMDARGVS(EBTABLES, DOWAIT, "-t %s -A %s %s %s %s %s %s %s %s %s %s %s %s %s -j mark --mark-or 0x%x --mark-target ACCEPT", "broute",
							QOS_EB_CHAIN, phyPort, eth_proto, (proto1==NULL)?" ":proto1, smac, dmac, saddr, sport, daddr, dport, dscp, strmark, dhcpopt_cmd, mark);
					
					if(proto2)
					{
						DOCMDARGVS(EBTABLES, DOWAIT, "-t %s -A %s %s %s %s %s %s %s %s %s %s %s %s %s -j mark --mark-or 0x%x --mark-target ACCEPT", "broute",
							QOS_EB_CHAIN, phyPort, eth_proto, proto2, smac, dmac, saddr, sport, daddr, dport, dscp, strmark, dhcpopt_cmd, mark);
					}
					
				}

			}else{

				/*ql:20080825 START: for tr069 pvc, just set priority according to wanport*/
				if (wan_tr069)
				{
					DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle -A qos_rule_output %s -j MARK --set-mark 0x%x/0x%x",
						wanPort, mark, QOS_MARK_MASK);
				} else {
				/*ql:20080825 END*/
					//forward chain
					DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle -A %s %s %s %s %s %s %s %s %s %s -j MARK --set-mark 0x%x/0x%x",
						QOS_IP_CHAIN, phyPort, wanPort, (proto1==NULL)?"":proto1, saddr, sport, daddr, dport, dscp, strmark, mark, QOS_MARK_MASK);

					if(proto2) /*OK, Here it comes again. TCP/UDP*/
					{
						//forward chain
						DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle -A %s %s %s %s %s %s %s %s %s %s -j MARK --set-mark 0x%x/0x%x",
							QOS_IP_CHAIN, phyPort, wanPort, proto2, saddr, sport, daddr, dport, dscp, strmark, mark, QOS_MARK_MASK);
					}
				/*ql:20080825 START: for tr069 pvc, just set priority according to wanport*/
				}
				/*ql:20080825 END*/

				//set dscp
				if( enableDscpMark && (0 != entry.m_dscp))	
				{
					//forward chain
					DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle -A %s %s %s %s %s %s %s %s %s %s -j DSCP --set-dscp 0x%x",
						QOS_IP_CHAIN, phyPort, wanPort, (proto1==NULL)?"":proto1, saddr, sport, daddr, dport, dscp, strmark, (entry.m_dscp-1)>>2);

					if(proto2)/* TCP/UDP, bored, why not set twice, any other wise idea? */
					{
						//forward chain
						DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle -A %s %s %s %s %s %s %s %s %s %s -j DSCP --set-dscp 0x%x",
							QOS_IP_CHAIN, phyPort, wanPort, proto2, saddr, sport, daddr, dport, dscp, strmark, (entry.m_dscp-1)>>2);
					}
				}
				
				
				/*--------------------------------------------------------*/
				/*  Iptables' behavior : Packets traverse a chain until they 
					hit ACCEPT, DROP or RETURN. They do not stop on a match 
					unless that match contains a terminating action. 
					
					Add a return rule after packet is matched and marked  */
				/*--------------------------------------------------------*/
				if( enableDscpMark && (0 != entry.m_dscp))	
				{
					dscp[0]='\0';
					snprintf(dscp, 24, "-m dscp --dscp 0x%x", (entry.m_dscp-1)>>2);
				
					//forward chain
					DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle -A %s %s %s %s %s %s %s %s %s %s -j RETURN",
						QOS_IP_CHAIN, phyPort, wanPort, (proto1==NULL)?"":proto1, saddr, sport, daddr, dport, dscp, strmark);

					if(proto2)/* TCP/UDP, bored, why not set twice, any other wise idea? */
					{
						//forward chain
						DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle -A %s %s %s %s %s %s %s %s %s %s -j RETURN",
							QOS_IP_CHAIN, phyPort, wanPort, proto2, saddr, sport, daddr, dport, dscp, strmark);
					}
				}
				else
				{					
					if (wan_tr069)
					{
						/*ql:20080825 START: for tr069 pvc, just set priority according to wanport*/
						DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle -A qos_rule_output %s -j RETURN",
							wanPort);
					} else {
						//forward chain
						DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle -A %s %s %s %s %s %s %s %s %s %s -j RETURN",
							QOS_IP_CHAIN, phyPort, wanPort, (proto1==NULL)?"":proto1, saddr, sport, daddr, dport, dscp, strmark);

						if(proto2) /*OK, Here it comes again. TCP/UDP*/
						{
							//forward chain
							DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle -A %s %s %s %s %s %s %s %s %s %s -j RETURN",
								QOS_IP_CHAIN, phyPort, wanPort, proto2, saddr, sport, daddr, dport, dscp, strmark);
						}
					}			
				}	

			}

#ifdef QOS_SUPPORT_RTP
		} else {
			if (!ebtable) {
				for (j=0; j<rtpRuleCnt; j++) {
					DOCMDARGVS(iptables_cmd, DOWAIT, "-t mangle -A %s %s %s %s %s %s %s %s %s %s -j MARK --set-mark 0x%x/0x%x",
						QOS_RTP_CHAIN, phyPort, wanPort, (proto1==NULL)?"":proto1, saddr, sport, rtp_rule_entry[j].daddr, rtp_rule_entry[j].dport, dscp, strmark, mark, QOS_MARK_MASK);
				}
			}
		}
#endif

		vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);

#ifdef QOS_SETUP_IFB
		if(PLY_PRIO==policy)//priority queue
		{
			ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
			DOCMDARGVS(TC, DOWAIT, "filter add dev ifb0 parent 2: prio %d %s handle 0x%x/0x%x fw flowid 2:%d",
					entry.prior, tc_protocol, mark, QOS_MARK_MASK, entry.prior);
			/* 
				Consider QoS SP mode. 
				Some packets will be dropped in VC/PTM/ETHWAN queue.
				Create a SP qdisc in VC/PTM/ETHWAN queue to avoid this problem.	
			*/
			for(j=0; j<vcEntryNum; j++)
			{
				if(!mib_chain_get(MIB_ATM_VC_TBL, j, &vcEntry) || !vcEntry.enableIpQos)
					continue;
				
				// Use "action mirred egress redirect" to do 2 layer QoS. One is on ifb0, another is on physical device(nas0_0).
				// It is that if the packet match the rule, it will demonstrate is the following sequence
				// (1) Packet is classified as class 1:%d and redirected to ifb0(the first QoS).
				// (2) Packet will come back to physical device(nas0_0) (the second QoS).
				ifGetName(PHY_INTF(vcEntry.ifIndex), ifname, sizeof(ifname));
				DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio %d %s handle 0x%x/0x%x fw flowid 1:%d action mirred egress redirect dev ifb0",
					ifname, entry.prior, tc_protocol, mark, QOS_MARK_MASK, entry.prior);
			}
		}
		else if(PLY_WRR==policy)//weighted round robin
		{
			DOCMDARGVS(TC, DOWAIT, "filter add dev ifb0 parent 1: prio 1 %s handle 0x%x/0x%x fw flowid 1:%d00",
				tc_protocol, mark, QOS_MARK_MASK, entry.prior);
		}
#elif defined(QOS_SETUP_IMQ)
		if(PLY_PRIO==policy)//priority queue
		{

			ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
			DOCMDARGVS(TC, DOWAIT, "filter add dev imq0 parent 2: prio %d %s handle 0x%x/0x%x fw flowid 2:%d",
					entry.prior, tc_protocol, mark, QOS_MARK_MASK, entry.prior);

			{
				/*
					If in bridge mode, 
					need redirect packet to imq0 by tc filter rule
				*/
				for(j=0; j<vcEntryNum; j++)
				{
					if(!mib_chain_get(MIB_ATM_VC_TBL, j, &vcEntry) || !vcEntry.enableIpQos)
						continue;
						
					if (vcEntry.cmode != CHANNEL_MODE_BRIDGE)
						continue;

					ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
						DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio %d %s handle 0x%x/0x%x fw flowid 2:%d action mirred egress redirect dev imq0",
						ifname, entry.prior, tc_protocol, mark, QOS_MARK_MASK, entry.prior);
				}
			}

			#if 1
			{
				/* 	
					Consider route mode, QoS SP mode. 
					Some packets will be dropped in VC/PTM/ETHWAN queue.
					Create a SP qdisc in VC/PTM/ETHWAN queue to avoid this problem.				
				*/
				for(j=0; j<vcEntryNum; j++)
				{
					if(!mib_chain_get(MIB_ATM_VC_TBL, j, &vcEntry) || !vcEntry.enableIpQos)
						continue;
						
					if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
						continue;

					ifGetName(PHY_INTF(vcEntry.ifIndex), ifname, sizeof(ifname));
					DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio %d %s handle 0x%x/0x%x fw flowid 1:%d",
						ifname, entry.prior, tc_protocol, mark, QOS_MARK_MASK, entry.prior);
				}
			}
			#endif
		}
		else if(PLY_WRR==policy)//weighted round robin
		{
			DOCMDARGVS(TC, DOWAIT, "filter add dev imq0 parent 1: prio 1 %s handle 0x%x/0x%x fw flowid 1:%d00",
				tc_protocol, mark, QOS_MARK_MASK, entry.prior);

			// Mason Yu.
			// If the vc is bridge mode , set tc action mirred and redirect to img0
			{				
				for(j=0; j<vcEntryNum; j++)
				{
					if(!mib_chain_get(MIB_ATM_VC_TBL, j, &vcEntry) || !vcEntry.enableIpQos)
						continue;

					if (vcEntry.cmode != CHANNEL_MODE_BRIDGE)
						continue;	
						
					ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));					
					DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio 1 %s handle 0x%x/0x%x fw flowid 1:1 action mirred egress redirect dev imq0",
						ifname, tc_protocol, mark, QOS_MARK_MASK);					
				}
			}
		}
#else
		if(PLY_PRIO==policy)//priority queue
		{
			//DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio %d protocol ip handle 0x%x/0x%x fw flowid 1:%d",
			//	ifname, entry.prior, mark, QOS_MARK_MASK, entry.prior);
			for(j=0; j<vcEntryNum; j++)
			{
				if(!mib_chain_get(MIB_ATM_VC_TBL, j, &vcEntry))
					continue;

				ifGetName(PHY_INTF(vcEntry.ifIndex), ifname, sizeof(ifname));
				DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio %d %s handle 0x%x/0x%x fw flowid 1:%d",
					ifname, entry.prior, tc_protocol, mark, QOS_MARK_MASK, entry.prior);
			}
		}
		else if(PLY_WRR==policy)//weighted round robin
		{
			for(j=0; j<vcEntryNum; j++)
			{
				if(!mib_chain_get(MIB_ATM_VC_TBL, j, &vcEntry))
					continue;

				ifGetName(PHY_INTF(vcEntry.ifIndex), ifname, sizeof(ifname));

				DOCMDARGVS(TC, DOWAIT, "filter add dev %s parent 1: prio 1 %s handle 0x%x/0x%x fw flowid 1:%d00",
					ifname, tc_protocol, mark, QOS_MARK_MASK, entry.prior);
			}
		}
#endif
		//if ( (wan_qos_state&QOS_WAN_BRIDGE) != QOS_WAN_BRIDGE) {
			// hwnat qos
			hwnat_qos_translate_rule(&entry);
		//}
#endif 
#ifdef CONFIG_RTK_L34_ENABLE
		RTK_RG_QoS_Rule_Set(&entry);
#endif
	}

    return 0;
}

static int setupQosRuleChain(unsigned int enable)
{
	QOS_SETUP_PRINT_FUNCTION;

	if (enable)
	{
		va_cmd(EBTABLES, 4, 1, "-t", "broute", "-N", QOS_EB_CHAIN);
		va_cmd(EBTABLES, 5, 1, "-t", "broute", "-P", QOS_EB_CHAIN, "RETURN");
		va_cmd(EBTABLES, 6, 1, "-t", "broute", "-A", "BROUTING", "-j", QOS_EB_CHAIN);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", QOS_IP_CHAIN);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", QOS_IP_CHAIN);

		//for rtp qos
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", QOS_RTP_CHAIN);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", QOS_RTP_CHAIN);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", "pvc_mark");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", "pvc_mark");

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", "qos_rule_output");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "OUTPUT", "-j", "qos_rule_output");

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", "pvc_mark_output");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "OUTPUT", "-j", "pvc_mark_output");

#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", QOS_IP_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", QOS_IP_CHAIN);

		//for rtp qos
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", QOS_RTP_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", QOS_RTP_CHAIN);

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", "pvc_mark");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "FORWARD", "-j", "pvc_mark");

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", "qos_rule_output");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "OUTPUT", "-j", "qos_rule_output");

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", "pvc_mark_output");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "OUTPUT", "-j", "pvc_mark_output");
#endif
	} else {
		va_cmd(EBTABLES, 4, 1, "-t", "broute", "-F", QOS_EB_CHAIN);
		va_cmd(EBTABLES, 6, 1, "-t", "broute", "-D", "BROUTING", "-j", QOS_EB_CHAIN);
		va_cmd(EBTABLES, 4, 1, "-t", "broute", "-X", QOS_EB_CHAIN);

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", QOS_IP_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", QOS_RTP_CHAIN);	//for rtp qos
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark_output");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "qos_rule_output");

		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", "pvc_mark");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", QOS_IP_CHAIN);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", QOS_RTP_CHAIN);
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "OUTPUT", "-j", "pvc_mark_output");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "OUTPUT", "-j", "qos_rule_output");

		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", "pvc_mark");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", QOS_IP_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", QOS_RTP_CHAIN);
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", "pvc_mark_output");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", "qos_rule_output");

#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", QOS_IP_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", QOS_RTP_CHAIN);	//for rtp qos
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "pvc_mark_output");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "qos_rule_output");

		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", "pvc_mark");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", QOS_IP_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "FORWARD", "-j", QOS_RTP_CHAIN);
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "OUTPUT", "-j", "pvc_mark_output");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "OUTPUT", "-j", "qos_rule_output");

		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", "pvc_mark");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", QOS_IP_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", QOS_RTP_CHAIN);
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", "pvc_mark_output");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", "qos_rule_output");
#endif
	}

	return 1;
}

static int delete_ALL_WAN_default_qdisc()
{
	MIB_CE_ATM_VC_T vcEntry;
	int i, vcEntryNum;
	
	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<vcEntryNum; i++)
	{
		char devname[IFNAMSIZ];
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry))
			continue;

		ifGetName(PHY_INTF(vcEntry.ifIndex), devname, sizeof(devname));
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", devname, "root");
	}
	return 0;
}

static int setup_prio_queue(int enable)
{
	// Mason Yu. Use device name not imq0
#ifdef QOS_SETUP_IFB
	char *imq0_devname="ifb0";
#else
	char *imq0_devname="imq0";
#endif
	
	//char devname[IFNAMSIZ];
	MIB_CE_ATM_VC_T vcEntry;
	int i, vcEntryNum;
	char s_rate[16], s_ceil[16];
	char tc_cmd[100];
	char tc_cmd2[200];
	unsigned int total_bandwidth = 0;

	QOS_SETUP_PRINT_FUNCTION;

	if(!enable){
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", imq0_devname, "root");
		delete_ALL_WAN_default_qdisc();
		// disable IPQoS
		__dev_setupIPQoS(0);
		hwnat_qos_bandwidth_setup(0);
		return 0;
	}

	//tc qdisc add dev $DEV root handle 1: htb default 400
	va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"root", "handle", "1:", "htb", "default", "100");
	
	//set total_bandwidth limit in imq0, and let packet be mirred to imq0 by tc filter or iptables rule	
	total_bandwidth = getUpLinkRate();
	hwnat_qos_bandwidth_setup(total_bandwidth*1024);
	snprintf(s_rate, 16, "%dKbit", total_bandwidth);
	snprintf(s_ceil, 16, "%dKbit", total_bandwidth);
	va_cmd(TC, 13, 1, "class", "add", "dev", imq0_devname,
		"parent", "1:", "classid", "1:100", "htb", "rate", s_rate, "ceil", s_ceil);

	snprintf(tc_cmd, 100, "tc qdisc add dev %s parent 1:100 handle 2: prio bands 4 priomap %s", imq0_devname, "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3");
	TRACE(STA_SCRIPT,"%s\n",tc_cmd);
	system(tc_cmd);
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"parent", "2:1", "handle", "3:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"parent", "2:2", "handle", "4:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"parent", "2:3", "handle", "5:", "pfifo", "limit", "100");
	va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"parent", "2:4", "handle", "6:", "pfifo", "limit", "100");
	
	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<vcEntryNum; i++)
	{
		char devname[IFNAMSIZ];
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry)||!vcEntry.enableIpQos)
			continue;

		//if (vcEntry.cmode != CHANNEL_MODE_BRIDGE)
		//	continue;

		ifGetName(PHY_INTF(vcEntry.ifIndex), devname, sizeof(devname));

		// delete default prio qdisc
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", devname, "root");
		snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 4 priomap %s", devname, "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3");
		TRACE(STA_SCRIPT,"%s\n",tc_cmd);
		system(tc_cmd);
		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
			"parent", "1:1", "handle", "2:", "pfifo", "limit", "100");
		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
			"parent", "1:2", "handle", "3:", "pfifo", "limit", "100");
		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
			"parent", "1:3", "handle", "4:", "pfifo", "limit", "100");
		va_cmd(TC, 11, 1, "qdisc", (char *)ARG_ADD, "dev", devname,
			"parent", "1:4", "handle", "5:", "pfifo", "limit", "100");
#ifdef QOS_SETUP_IFB	
		
		// tc filter add dev nas0_0 parent 1: protocol ip u32 match u32 0 0 flowid 1:4 caction mirred egress redirect dev ifb0
		//Use "action mirred egress redirect" to do 2 layer QoS. One is on ifb0, another is on physical device(nas0_0).
		// This rule is for not match the other filters on the physical device(nas0_0) and will match this rule .
		// Match packets will demonstrate is the following sequence
		// (1) Packet is classified as class 1:4 and redirected to ifb0(the first QoS).
		// (2) Packet will come back to physical device(nas0_0)(the second QoS).
		snprintf(tc_cmd2, 200, "tc filter add dev %s parent 1: protocol ip u32 match u32 0 0 flowid 1:4 action mirred egress redirect dev %s", devname, imq0_devname);		
		TRACE(STA_SCRIPT,"%s\n",tc_cmd2);
		system(tc_cmd2);
#endif
	}
	// enable IPQoS
	__dev_setupIPQoS(1);
	
	return 0;
}

/*
 *	action:
 *	0: Add
 *	1: Change
 */
static int setup_htb_class(int action, unsigned int upRate)
{
#ifdef QOS_SETUP_IFB
	char *devname="ifb0";
#else
	char *devname="imq0";
#endif
	char s_rate[16], s_ceil[16], s_classid[16], s_quantum[16];
	const char *pAction;
	int j;
	int rate, ceil, quantum;
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int qEntryNum, minWeigh;
	int totalWeight = SumWeightOfWRR();

	if (action == 0)
		pAction = ARG_ADD;
	else
		pAction = ARG_CHANGE;
	snprintf(s_rate, 16, "%dKbit", upRate);
	snprintf(s_ceil, 16, "%dKbit", upRate);
	//tc class add dev $DEV parent 1: classid 1:1 htb rate $RATE ceil $CEIL
	va_cmd(TC, 13, 1, "class", pAction, "dev", devname,
		"parent", "1:", "classid", "1:1", "htb", "rate", s_rate, "ceil", s_ceil);

	//mib_get(MIB_QOS_ENABLE_FORCE_WEIGHT, (void *)&enableForceWeight);

	if((qEntryNum=mib_chain_total(MIB_IP_QOS_QUEUE_TBL)) <=0)
		return 0;
	for(j=0;j<qEntryNum; j++)
	{
		if(!mib_chain_get(MIB_IP_QOS_QUEUE_TBL, j, (void*)&qEntry))
			continue;
		if ( !qEntry.enable)
			continue;

		snprintf(s_classid, 16, "1:%d00", j+1);
		minWeigh = findQueueMinWeigh();
		// Quantum should be bigger then MTU (1500) so you can send the maximum packet in 1 turn and smaller then 60000
		quantum = (1500*qEntry.weight)/minWeigh;
		if (quantum > 60000)
			quantum = 60000;
		snprintf(s_quantum, 16, "%d", quantum);
		//printf("s_quantum=%s\n", s_quantum);
		rate = qEntry.weight;
		//if enableForceWeight, then send rate should not larger than wrr proportion
		ceil = upRate;
		snprintf(s_ceil, 16, "%dKbit", ceil);
		if (rate > ceil)
		{
			rate = ceil*qEntry.weight/totalWeight;
		}
		snprintf(s_rate, 16, "%dKbit", rate);

		//tc class add dev $DEV parent 10:1 classid 10:$SUBID htb rate $RATE ceil $RATE prio $PRIO
		va_cmd(TC, 15, 1, "class", pAction, "dev", devname,
			"parent", "1:1", "classid", s_classid, "htb", "rate",
			s_rate, "ceil", s_ceil, "quantum", s_quantum);
	}

}

static int setup_wrr_queue(enable)
{
	int j;
#ifdef QOS_SETUP_IFB
	char *imq0_devname="ifb0";
#else
	char *imq0_devname="imq0";
#endif
	char s_rate[16], s_ceil[16], s_classid[16], s_quantum[16];
	unsigned int total_bandwidth = 0;

	MIB_CE_ATM_VC_T vcEntry;
	int i, vcEntryNum;
	char tc_cmd[100];
	char tc_cmd2[200];
	char devname2[IFNAMSIZ];

	QOS_SETUP_PRINT_FUNCTION;

	if (!enable) {
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", imq0_devname, "root");
		delete_ALL_WAN_default_qdisc();
		// disable IPQoS
		__dev_setupIPQoS(0);	
		return 0;
	}

//	if (!ifSumWeighIs100())
//		return 0;
	total_bandwidth = getUpLinkRate();
	current_uprate = total_bandwidth;
	hwnat_qos_bandwidth_setup(total_bandwidth*1024);

	snprintf(s_rate, 16, "%dKbit", total_bandwidth);
	snprintf(s_ceil, 16, "%dKbit", total_bandwidth);
	//tc qdisc add dev $DEV root handle 1: htb default 400
	va_cmd(TC, 10, 1, "qdisc", (char *)ARG_ADD, "dev", imq0_devname,
		"root", "handle", "1:", "htb", "default", "400");

	setup_htb_class(0, total_bandwidth);
	//set queue len
	set_wrr_class_qdisc();
	
#ifdef QOS_SETUP_IFB	
	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<vcEntryNum; i++)
	{
		char devname[IFNAMSIZ];
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry)||!vcEntry.enableIpQos)
			continue;

		ifGetName(vcEntry.ifIndex, devname, sizeof(devname));

		// delete default prio qdisc
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", devname, "root");
		snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 4 priomap %s", devname, "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3");
		TRACE(STA_SCRIPT,"%s\n",tc_cmd);
		system(tc_cmd);		
	
		// tc filter add dev nas0_0 parent 1: protocol ip u32 match u32 0 0 action mirred egress redirect dev ifb0
		snprintf(tc_cmd2, 200, "tc filter add dev %s parent 1: protocol ip u32 match u32 0 0 action mirred egress redirect dev %s", devname, imq0_devname);
		TRACE(STA_SCRIPT,"%s\n",tc_cmd2);
		system(tc_cmd2);
	}
#endif


#ifdef QOS_SETUP_IMQ
	// Mason Yu.
	// If the vc is bridge mode , set root qdisc for tc action mirred
	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<vcEntryNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry)||!vcEntry.enableIpQos)
			continue;

		if (vcEntry.cmode != CHANNEL_MODE_BRIDGE)
			continue;

		ifGetName(vcEntry.ifIndex, devname2, sizeof(devname2));

		// delete default prio qdisc
		va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", devname2, "root");
		snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio bands 4 priomap %s", devname2, "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3");
		TRACE(STA_SCRIPT,"%s\n",tc_cmd);
		system(tc_cmd);
	}
#endif
	// enable IPQoS
	__dev_setupIPQoS(1);	
	return 0;
}

static int setupQdisc(int enable)
{
	unsigned char policy;

	QOS_SETUP_PRINT_FUNCTION;
	mib_get(MIB_QOS_POLICY, (void*)&policy);

#ifdef QOS_SETUP_IFB
	// ifconfig ifb0 up	
	va_cmd(IFCONFIG, 2, 1, "ifb0", "up");	
#endif	
	if (policy == PLY_PRIO)
		setup_prio_queue(enable);
	else
		setup_wrr_queue(enable);
	return 0;
}

static int setupDefaultQdisc(int enable)
{
	MIB_CE_ATM_VC_T vcEntry;
	int i, vcEntryNum;
	char tc_cmd[100];
	
	QOS_SETUP_PRINT_FUNCTION;

	vcEntryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for(i=0;i<vcEntryNum; i++)
	{
		char devname[IFNAMSIZ];
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry))
			continue;

		ifGetName(PHY_INTF(vcEntry.ifIndex), devname, sizeof(devname));

		if (!enable) {
			va_cmd(TC, 5, 1, "qdisc", (char *)ARG_DEL, "dev", devname, "root");
			continue;
		}

		snprintf(tc_cmd, 100, "tc qdisc add dev %s root handle 1: prio", devname);
		TRACE(STA_SCRIPT,"%s\n",tc_cmd);
		system(tc_cmd);
	}	
	return 0;
}

static int setupQRule()
{
	unsigned char policy;

	QOS_SETUP_PRINT_FUNCTION;
	mib_get(MIB_QOS_POLICY, (void*)&policy);
	setup_qos_rules(policy);
	return 0;
}

int enableIMQ(int enable)
{
	MIB_CE_ATM_VC_T vcEntry;
	int i, entryNum;
	char ifname[IFNAMSIZ];

	QOS_SETUP_PRINT_FUNCTION;

	if (!enable) {
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-F", "IMQ_CHAIN");
		va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", "IMQ_CHAIN");
		va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-X", "IMQ_CHAIN");
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-F", "IMQ_CHAIN");
		va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-D", "POSTROUTING", "-j", "IMQ_CHAIN");
		va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-X", "IMQ_CHAIN");
#endif
		va_cmd(IFCONFIG, 2, 1, "imq0", "down");
		return 0;
	}

	va_cmd(IFCONFIG, 3, 1, "imq0", "txqueuelen", "100");
	va_cmd(IFCONFIG, 2, 1, "imq0", "up");

	va_cmd(IPTABLES, 4, 1, "-t", "mangle", "-N", "IMQ_CHAIN");
	va_cmd(IPTABLES, 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", "IMQ_CHAIN");
#ifdef CONFIG_IPV6
	va_cmd(IP6TABLES, 4, 1, "-t", "mangle", "-N", "IMQ_CHAIN");
	va_cmd(IP6TABLES, 6, 1, "-t", "mangle", "-A", "POSTROUTING", "-j", "IMQ_CHAIN");
#endif

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);

	for(i=0; i<entryNum; i++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&vcEntry) || !vcEntry.enableIpQos)
			continue;

		if (vcEntry.cmode == CHANNEL_MODE_BRIDGE)
			continue;

		ifGetName(vcEntry.ifIndex, ifname, sizeof(ifname));
		va_cmd(IPTABLES, 10, 1, "-t", "mangle", "-A", "IMQ_CHAIN",
			"-o", ifname, "-j", "IMQ", "--todev", "0");
#ifdef CONFIG_IPV6
		va_cmd(IP6TABLES, 10, 1, "-t", "mangle", "-A", "IMQ_CHAIN",
			"-o", ifname, "-j", "IMQ", "--todev", "0");
#endif
	}

	return 0;
}

/*
 * Make sure we have 4 QoS Queues.
 */
static void checkQosQueue()
{
	MIB_CE_IP_QOS_QUEUE_T qEntry;
	int i;

	if(mib_chain_total(MIB_IP_QOS_QUEUE_TBL) >= 4)
		return;

	// Flush table
	mib_chain_clear(MIB_IP_QOS_QUEUE_TBL);

	// Add MAX_QOS_QUEUE_NUM of default Qos Queues
	for (i=0; i<4; i++) {
		qEntry.enable = 0;
		qEntry.weight = 10*(4-i);
		qEntry.QueueInstNum = i+1;
		mib_chain_add(MIB_IP_QOS_QUEUE_TBL, &qEntry);
	}
	mib_update(CURRENT_SETTING, CONFIG_MIB_ALL);
}


int setupIPQ(void)
{
	unsigned int qosEnable;
	unsigned int qosRuleNum, carRuleNum;
	unsigned char totalBandWidthEn;

	QOS_SETUP_PRINT_FUNCTION;
	checkQosQueue();
	UpdateIpFastpathStatus(); // need to disable upstream IPQoS for IPQoS

	qosEnable = getQosEnable();
	qosRuleNum = getQosRuleNum();
	mib_get(MIB_TOTAL_BANDWIDTH_LIMIT_EN, (void *)&totalBandWidthEn);
	carRuleNum = mib_chain_total(MIB_IP_QOS_TC_TBL);

	//printf("[%s %d]qosEnable=%d, totalBandWidthEn=%d, qosRuleNum=%d, carRuleNum=%d\n", __func__, __LINE__, qosEnable, totalBandWidthEn, qosRuleNum, carRuleNum);

	// hwnat qos
	hwnat_qos_queue_setup();
//	hwnat_wanif_rule();
	setWanIF1PMark();
	
	// Set qdisc to prio for all WAN interface. It need delete first when the WAN interface want to be added another qdisc(see the "delete default prio qdisc" string on this file).
	setupDefaultQdisc(1);
	if (qosEnable && qosRuleNum) { // ip qos
		setupQosRuleChain(1);
#ifdef CONFIG_RTK_L34_ENABLE
		RTK_RG_QoS_Queue_Set();
#endif
		setupQdisc(1);
		setupQRule();
#if defined(QOS_SETUP_IMQ)
		enableIMQ(1);
#endif
	}
#ifdef CONFIG_RTK_L34_ENABLE  //Apollo HW have rate limit and QoS marking/rules co-exist capability.
	if (totalBandWidthEn || carRuleNum) { // traffic shapping
#else
	else if (totalBandWidthEn || carRuleNum) { // traffic shapping
#endif
		setupCarChain(1);
		setupCarQdisc(1);
		setupCarRule();
	}

#ifdef CONFIG_RTK_L34_ENABLE
#ifdef CONFIG_00R0
		if (qosEnable) { //Enable IPQos workaround , 
			RG_del_IPQos_WorkAround();
			RG_add_IPQos_WorkAround();
		}
		else { //disable IpQos workaround
			RG_del_IPQos_WorkAround();
		}
#endif


	if (totalBandWidthEn){
		unsigned int bandwidth = getUpLinkRate();
		printf("Config RG QoS total bandwidth %d Kbps\n",bandwidth);
		RTK_RG_QoS_TotalBandwidth_Set(bandwidth);
	}
#endif

	//setupUserIPQoSRule(1);
}

int stopIPQ(void)
{
	setupQosRuleChain(0);
	setupQdisc(0);
	setupDefaultQdisc(0);
#if defined(QOS_SETUP_IMQ)
	enableIMQ(0);
#endif
	setupCarChain(0);
	setupCarQdisc(0);
	// hwnat qos
	hwnat_qos_queue_stop();
	hwnat_qos_bandwidth_setup(0);

#ifdef CONFIG_RTK_L34_ENABLE
	FlushRTK_RG_QoS_Rules();
#ifdef CONFIG_00R0
	FlushRTK_RG_QoS_TS_Rules();
#endif
	printf("Reset RG QoS total bandwidth to unlimit.\n");
	RTK_RG_QoS_TotalBandwidth_Set(0);
#ifdef QOS_TRAFFIC_SHAPING_BY_SSID
	reset_ssid_traffic_shwping_rule();
#endif
#endif
#ifdef CONFIG_TR142_MODULE
	RTK_RG_QoS_Queue_Remove();
#endif

	return 0;
}

void take_qos_effect_v3(void)
{
	stopIPQ();
	setupIPQ();
}


#ifdef _PRMT_X_CT_COM_QOS_
#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
const char *ct_typename[CT_TYPE_NUM + 1] = {
	"", "SMAC", "8021P", "SIP", "DIP", "SPORT", "DPORT", "TOS", "DSCP",
	    "WANInterface", "LANInterface",
};

const char *LANString[4] = {
	"InternetGatewayDevice.LANDevice.1.LANEthernetInterfaceConfig.1",	//eth0_sw0
	"InternetGatewayDevice.LANDevice.1.LANEthernetInterfaceConfig.2",	//eth0_sw1
	"InternetGatewayDevice.LANDevice.1.LANEthernetInterfaceConfig.3",	//eth0_sw2
	"InternetGatewayDevice.LANDevice.1.LANEthernetInterfaceConfig.4",	//eth0_sw3
};

static int findcttype(MIB_CE_IP_QOS_T *p, unsigned char typenum)
{
	int i;

	for (i = 0; i < CT_TYPE_NUM; i++) {
		if (p->cttypemap[i] == typenum)
			return i;
	}

	return -1;
}

static int gettypeinst(MIB_CE_IP_QOS_T *p)
{
	int i;

	for (i = 0; i < CT_TYPE_NUM; i++) {
		if (p->cttypemap[i] == 0 || p->cttypemap[i] == 0xf)
			return i;
	}

	return 0;
}

static unsigned int getIPTVIndex()
{
	MIB_CE_ATM_VC_T entry;
	int i, total;

	total = mib_chain_total(MIB_ATM_VC_TBL);
	for (i = 0; i < total; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, &entry))
			continue;
		if ((entry.applicationtype == X_CT_SRV_OTHER || entry.applicationtype == X_CT_SRV_INTERNET) && entry.cmode == CHANNEL_MODE_BRIDGE && entry.enable == 1)	//OTHER or INTERNET
			return entry.ifIndex;
	}

	return DUMMY_IFINDEX;
}


int updatecttypevalue(MIB_CE_IP_QOS_T *p)
{
	unsigned char i;
	unsigned char typenum = 0;
	struct in_addr addr;
	int intvalue;
	int typeinst;
	int tmpinst;

	if (p->modeTr69 != MODEIPTV && p->modeTr69 != MODEOTHER)
		return -1;

	for (i = 1; i <= CT_TYPE_NUM; i++) {
		tmpinst = findcttype(p, i);
		switch (i) {
		case 1:	//SMAC
			break;
		case 2:	//8021P
			if (p->vlan1p > 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
			} else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		case 3:	//SIP
			if (p->sip[0] != 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
			} else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		case 4:	//DIP
			if (p->dip[0] != 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
			} else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		case 5:	//SPORT
			if (p->sPort != 0 || p->sPortRangeMax != 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
			} else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		case 6:	//DPORT
			if (p->dPort != 0 || p->dPortRangeMax != 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
			} else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		case 7:	//TOS
			break;
		case 8:	//DSCP
			if (p->qosDscp != 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
			} else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		case 9:	//WANInterface
			break;
		case 10:	//LANInterface
			if (p->phyPort > 0 || p->minphyPort > 0) {
				if (tmpinst < 0) {	//web add this type
					typeinst = gettypeinst(p);
					p->cttypemap[typeinst] = i;
				}
			} else {
				if (tmpinst >= 0)	//web del this type
					p->cttypemap[tmpinst] = 0;
			}
			break;
		default:
			break;
		}
	}
	return 0;
}

static void calculate_ip(struct in_addr *ip, unsigned char mask, int maxflag, struct in_addr *result)
{
	struct in_addr mask_ip = {0};
	int i;

	for(i = 0 ; i < mask ;i++)
		mask_ip.s_addr |= (1 << (32-i));

	memcpy(result, ip, sizeof(struct in_addr));
	result->s_addr &= mask_ip.s_addr;

	if(maxflag)
	{
		mask_ip.s_addr = 0;
		for(i = 0 ; i < (32 - mask) ;i++)
			mask_ip.s_addr |= (1 << i);

		result->s_addr |= mask_ip.s_addr;
	}
}

int getcttypevalue(char *typevalue, MIB_CE_IP_QOS_T *p, int typeinst,
		   int maxflag)
{
	unsigned char typenum;

	typenum = p->cttypemap[typeinst];
	if (typenum <= 0 || typenum > CT_TYPE_NUM)
		return -1;

	switch (typenum) {
	case 1:		//SMAC
		mac_hex2string(p->smac, typevalue);
		return 0;
	case 2:		//8021P
		sprintf(typevalue, "%hhu", p->vlan1p - 1);
		return 0;
	case 3: 	//SIP
		{
			struct in_addr ip_result = {0};
			calculate_ip((struct in_addr *)p->sip, p->smaskbit, maxflag, &ip_result);
			strcpy(typevalue, inet_ntoa(ip_result));
			return 0;
		}
	case 4: 	//DIP
		{
			struct in_addr ip_result = {0};
			calculate_ip((struct in_addr *)p->dip, p->dmaskbit, maxflag, &ip_result);
			strcpy(typevalue, inet_ntoa(ip_result));
			return 0;
		}

	case 5:		//SPORT
		sprintf(typevalue, "%hu",
			maxflag ? p->sPortRangeMax : p->sPort);
		return 0;
	case 6:		//DPORT
		sprintf(typevalue, "%hu",
			maxflag ? p->dPortRangeMax : p->dPort);
		return 0;
	case 7:		//TOS
		sprintf(typevalue, "%hhu", p->tos);
		return 0;
	case 8:		//DSCP
		sprintf(typevalue, "%hhu", (p->qosDscp - 1) >> 2);
		return 0;
	case 9:		//WANInterface
		return 0;
	case 10:		//LANInterface
		if (maxflag == 1) {
			if (p->phyPort > 0 && p->phyPort < 5) {
				strcpy(typevalue, LANString[p->phyPort - 1]);
				return 0;
			} else
				return -1;
		} else {
			if (p->minphyPort > 0 && p->minphyPort < 5) {
				strcpy(typevalue, LANString[p->minphyPort - 1]);
				return 0;
			} else
				return -1;
		}
	default:
		return -1;
	}
}

int setcttypevalue(char *typevalue, MIB_CE_IP_QOS_T * p, int typeinst,
		   int maxflag)
{
	unsigned char typenum;
	int i;

	typenum = p->cttypemap[typeinst];
	if (typenum <= 0 || typenum > CT_TYPE_NUM)
		return -1;

	switch (typenum) {
	case 1:		//SMAC
		for(i = 0; i < ETH_ALEN; i++) {
			p->smac[i] = hex(typevalue[i * 3]) * 16 + hex(typevalue[i * 3 + 1]);
		}
		return 0;
	case 2:		//8021P
		sscanf(typevalue, "%hhu", &p->vlan1p);
		p->vlan1p = p->vlan1p + 1;
		return 0;
	case 3:		//SIP
		if (!inet_aton(typevalue, (struct in_addr *)p->sip))
			return -1;
		p->smaskbit = 32;
		return 0;
	case 4:		//DIP
		if (!inet_aton(typevalue, (struct in_addr *)p->dip))
			return -1;
		p->smaskbit = 32;
		return 0;
	case 5:		//SPORT
		sscanf(typevalue, "%hu", maxflag ? &p->sPortRangeMax : &p->sPort);
		return 0;
	case 6:		//DPORT
		sscanf(typevalue, "%hu", maxflag ? &p->dPortRangeMax : &p->dPort);
		return 0;
	case 7:		//TOS
		sscanf(typevalue, "%hhu", &p->tos);
		return 0;
	case 8:		//DSCP
		sscanf(typevalue, "%hhu", &p->qosDscp);
		p->qosDscp = p->qosDscp >> 2 + 1;
		return 0;
	case 9:		//WANInterface
		return 0;
	case 10:		//LANInterface
		for (i = 0; i < 4; i++) {
			if (strstr(typevalue, LANString[i]) != 0)
				break;
		}
		if (i <= 4) {
			if (maxflag == 1)
				p->phyPort = i + 1;
			else
				p->minphyPort = i + 1;
		} else
			return -1;
		return 0;
	default:
		return 0;
	}
}

int getcttype(char *typename, MIB_CE_IP_QOS_T *p, int typeinst)
{
	unsigned char typenum;

	typenum = p->cttypemap[typeinst];
	if (typenum <= 0 || typenum > CT_TYPE_NUM)
		return -1;

	strcpy(typename, ct_typename[typenum]);
	return 0;
}

int setcttype(char *typename, MIB_CE_IP_QOS_T *p, int typeinst)
{
	int i;

	for (i = 1; i <= CT_TYPE_NUM; i++) {
		if (!strcmp(typename, ct_typename[i])) {
			delcttypevalue(p, typeinst);
			p->cttypemap[typeinst] = i;

			return 0;
		}
	}

	return -1;
}

int delcttypevalue(MIB_CE_IP_QOS_T *p, int typeinst)
{
	unsigned char typenum;

	typenum = p->cttypemap[typeinst];
	if (typenum <= 0 || (typenum > CT_TYPE_NUM && typenum != 0xf))
		return -1;
	p->cttypemap[typeinst] = 0;

	switch (typenum) {
	case 1:		//SMAC
		memset(p->smac, 0, sizeof(p->smac));
		return 0;
	case 2:		//8021P
		p->vlan1p = 0;
		return 0;
	case 3:		//SIP
		memset(p->sip, 0, sizeof(p->sip));
		return 0;
	case 4:		//DIP
		memset(p->dip, 0, sizeof(p->dip));
		return 0;
	case 5:		//SPORT
		p->sPort = 0;
		p->sPortRangeMax = 0;
		return 0;
	case 6:		//DPORT
		p->dPort = 0;
		p->dPortRangeMax = 0;
		return 0;
	case 7:		//TOS
		p->tos = 0;
		return 0;
	case 8:		//DSCP
		p->qosDscp = 0;
		return 0;
	case 9:		//WANInterface
		return 0;
	case 10:		//LANInterface
		p->phyPort = 0;
		p->minphyPort = 0;
		return 0;
	default:
		return 0;
	}
}


int setMIBforQosMode(unsigned char modeflag)
{
	MIB_CE_IP_QOS_T qos_entity, *p = &qos_entity;	
	MIB_CE_IP_QOS_APP_T qosApp_entity, *p2 = &qosApp_entity;	
	int if_iptv = DUMMY_IFINDEX;
	
	
	if ( modeflag == OTHERMODE )
		return 0;
		
	/* INTERNET and TR069 must exist */
	if ((modeflag & (INTERNETMODE | TR069MODE)) !=
		   (INTERNETMODE | TR069MODE)) {
			printf("Error: INTERNET and TR069 must exist\n");
			return ERR_9007;
	}

	if_iptv = getIPTVIndex();
	if ((modeflag & IPTVMODE) && if_iptv == DUMMY_IFINDEX) {
		printf("Error: IPTV must exist for BR_ROUTE_ONEPVC\n");
		return ERR_9002;
	}
	
	/* delete old IP QoS classification rules */
	mib_chain_clear(MIB_IP_QOS_TBL);
	/* delete old IP QoS APP rules */
	mib_chain_clear(MIB_IP_QOS_APP_TBL);

	if (modeflag & TR069MODE) {
		char vStr[256 + 1];
		char acsurl[256 + 1];
		struct hostent *host;
		
		memset(p2, 0, sizeof(MIB_CE_IP_QOS_APP_T));

		p2->prior = 1;
		if (modeflag & IPTVMODE)
			p2->prior++;
		if (modeflag & VOIPMODE)
			p2->prior++;		
		p2->appName = QOS_APP_TR069;
		mib_chain_add(MIB_IP_QOS_APP_TBL, p2);		
	}
	if (modeflag & IPTVMODE) {
		memset(p, 0, sizeof(MIB_CE_IP_QOS_T));
		p->outif = getIPTVIndex();
#ifdef BR_ROUTE_ONEPVC
		p->cmode = CHANNEL_MODE_BRIDGE;
#endif
#ifdef CONFIG_IPV6
		p->IpProtocol = IPVER_IPV4;	/* IPv4 */
#endif
		p->prior = 1;
		if (modeflag & VOIPMODE)
			p->prior++;
#ifdef _PRMT_X_CT_COM_QOS_
		p->modeTr69 = MODEIPTV;
#endif
		p->enable = 1;
		p->InstanceNum = 1;
		sprintf(p->RuleName, "%s", "rule_iptv");
#ifdef _PRMT_X_CT_COM_QOS_
		p->cttypemap[0] = 10;	//LANInterface
#endif
		p->phyPort = 2;
#ifdef _PRMT_X_CT_COM_QOS_
		p->minphyPort = 2;
#endif
		p->protoType = PROTO_UDPTCP;
		p->m_dscp = 0;
		p->m_1p = 0;

		mib_chain_add(MIB_IP_QOS_TBL, p);		
	}
	if (modeflag & VOIPMODE) {
		memset(p2, 0, sizeof(MIB_CE_IP_QOS_APP_T));

		p2->prior = 1;				
		p2->appName = QOS_APP_VOIP;
		mib_chain_add(MIB_IP_QOS_APP_TBL, p2);		
	}

	mib_set(CTQOS_MODE, &modeflag);

	unsigned char vUChar;
	unsigned int vUInt;
	/* InternetGatewayDevice.X_CT-COM_UplinkQoS.Enable = false */
	//vUChar = 0;
	//mib_set(MIB_QOS_ENABLE_QOS, &vUChar);

	/* InternetGatewayDevice.X_CT-COM_UplinkQoS.Bandwidth = 0 */
	vUInt = 0;
	mib_set(MIB_TOTAL_BANDWIDTH, &vUInt);
	vUChar = 0;
	mib_set(MIB_TOTAL_BANDWIDTH_LIMIT_EN, &vUChar);

	/* InternetGatewayDevice.X_CT-COM_UplinkQoS.Plan = priority */
	vUChar = 0;
	mib_set(MIB_QOS_POLICY, &vUChar);

	/* InternetGatewayDevice.X_CT-COM_UplinkQoS.EnableForceWeight = 0 */
	vUChar = 0;
	mib_set(MIB_QOS_ENABLE_FORCE_WEIGHT, &vUChar);

	/* InternetGatewayDevice.X_CT-COM_UplinkQoS.EnableDSCPMark = 0 */
	vUChar = 0;
	mib_set(MIB_QOS_ENABLE_DSCP_MARK, &vUChar);

	/* InternetGatewayDevice.X_CT-COM_UplinkQoS.Enable802-1_P = 0 */
	vUChar = 0;
	mib_set(MIB_QOS_ENABLE_1P, &vUChar);
}
#endif
