/*
 *      Adsl Driver Interface
 *      Authors: Dick Tam	<dicktam@realtek.com.tw>
 *
 */

#ifndef _INCLUDE_ADSL_DRV_H
#define _INCLUDE_ADSL_DRV_H

#define ADSL_DRV_RETURN_LEN	512

typedef enum {
	ADSL_GET_SNR_DS,
	ADSL_GET_SNR_US,
	ADSL_GET_VERSION,
	ADSL_GET_MODE,
	ADSL_GET_STATE,
	ADSL_GET_POWER_DS,
	ADSL_GET_POWER_US,
	ADSL_GET_ATTRATE_DS,
	ADSL_GET_ATTRATE_US,
	ADSL_GET_RATE_DS,
	ADSL_GET_RATE_US,
	ADSL_GET_LATENCY,
	//ADSL_GET_LATENCY_DS,
	//ADSL_GET_LATENCY_US,
	ADSL_GET_LPATT_DS,
	ADSL_GET_LPATT_US,
	ADSL_GET_TRELLIS,
	ADSL_GET_POWER_LEVEL,
	ADSL_GET_K_DS,
	ADSL_GET_K_US,
	ADSL_GET_R_DS,
	ADSL_GET_R_US,
	ADSL_GET_S_DS,
	ADSL_GET_S_US,
	ADSL_GET_D_DS,
	ADSL_GET_D_US,
	ADSL_GET_DELAY_DS,
	ADSL_GET_DELAY_US,
	
	/* order is important here. */
	ADSL_GET_CRC_DS,
	ADSL_GET_FEC_DS,
	ADSL_GET_ES_DS,
	ADSL_GET_SES_DS,
	ADSL_GET_UAS_DS,
	ADSL_GET_LOS_DS,
	
	ADSL_GET_CRC_US,
	ADSL_GET_FEC_US,
	ADSL_GET_ES_US,
	ADSL_GET_SES_US,
	ADSL_GET_UAS_US,
	ADSL_GET_LOS_US,
	
	ADSL_GET_FULL_INIT,
	ADSL_GET_FAILED_FULL_INIT,
	ADSL_GET_LAST_LINK_DS,
	ADSL_GET_LAST_LINK_US,
	ADSL_GET_TX_FRAMES,
	ADSL_GET_RX_FRAMES,
	ADSL_GET_SYNCHRONIZED_TIME,
	ADSL_GET_SYNCHRONIZED_NUMBER,
	
//#ifdef CONFIG_VDSL
	DSL_GET_TRELLIS_DS,
	DSL_GET_TRELLIS_US,	
	DSL_GET_PHYR_DS,
	DSL_GET_PHYR_US,
	DSL_GET_N_DS,
	DSL_GET_N_US,
	DSL_GET_L_DS,
	DSL_GET_L_US,
	DSL_GET_INP_DS,
	DSL_GET_INP_US,
	DSL_GET_TPS,
//#endif /*CONFIG_VDSL*/

//#ifdef CONFIG_DSL_VTUO
	DSL_GET_PMD_MODE,
	DSL_GET_LINK_TYPE,
	DSL_GET_LIMIT_MASK,
	DSL_GET_US0_MASK,
	DSL_GET_UPBOKLE_DS,
	DSL_GET_UPBOKLE_US,
	DSL_GET_ACTUALCE,

	DSL_GET_SIGNAL_ATN_DS,
	DSL_GET_SIGNAL_ATN_US,
	DSL_GET_LINE_ATN_DS,
	DSL_GET_LINE_ATN_US,
	DSL_GET_RX_POWER_DS,
	DSL_GET_RX_POWER_US,
	DSL_GET_PMSPARA_I_DS,
	DSL_GET_PMSPARA_I_US,
	DSL_GET_DATA_LPID_DS,
	DSL_GET_DATA_LPID_US,
	DSL_GET_PTM_STATUS,
	DSL_GET_RA_MODE_DS,
	DSL_GET_RA_MODE_US,
	DSL_GET_RETX_OHRATE_DS,
	DSL_GET_RETX_OHRATE_US,
	DSL_GET_RETX_FRAM_TYPE_DS,
	DSL_GET_RETX_FRAM_TYPE_US,
	DSL_GET_RETX_ACTINP_REIN_DS,
	DSL_GET_RETX_ACTINP_REIN_US,
	DSL_GET_RETX_H_DS,
	DSL_GET_RETX_H_US,
//#endif /*CONFIG_DSL_VTUO*/
	DSL_GET_VECTOR_MODE,
	ADSL_GET_LOOP_LENGTH_METER,
	ADSL_GET_ENDNO
} ADSL_GET_ID;
        
typedef struct {
	char *cmd;
	ADSL_GET_ID id;
} adsl_drv_get_cmd;

typedef struct {
	char *cmd;
	char *value;
} adsl_drv_set_cmd;


extern int getAdslDrvInfo(char *name, char *buff, int len);
extern int getAdslInfo(ADSL_GET_ID id, char *buf, int len);
#ifdef CONFIG_USER_XDSL_SLAVE
extern int getAdslSlvDrvInfo(char *name, char *buff, int len);
extern int getAdslSlvInfo(int id, char *buf, int len);
extern char adsl_slv_drv_get(unsigned int id, void *rValue, unsigned int len);
extern char dsl_slv_msg(unsigned int id, int msg, int *pval);
extern char dsl_slv_msg_set_array(int msg, int *pval);
extern char dsl_slv_msg_set(int msg, int val);
extern char dsl_slv_msg_get_array(int msg, int *pval);
extern char dsl_slv_msg_get(int msg, int *pval);
#endif /*CONFIG_USER_XDSL_SLAVE*/


#endif
