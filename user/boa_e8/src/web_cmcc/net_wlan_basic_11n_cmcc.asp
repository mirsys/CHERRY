<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<HTML>
<HEAD>
<TITLE>中国移动-WLAN配置</TITLE>
<META http-equiv=pragma content=no-cache>
<META http-equiv=cache-control content="no-cache, must-revalidate">
<META http-equiv=content-type content="text/html; charset=gbk">
<META http-equiv=content-script-type content=text/javascript>
<!--系统公共css-->
<STYLE type=text/css>
@import url(/style/default.css);
</STYLE>
<!--系统公共脚本-->
<SCRIPT language="javascript" src="common.js"></SCRIPT>
<script type="text/javascript" src="share.js"></script>
<SCRIPT language="javascript" type="text/javascript">

var cgi_wlCurrentChannel = "11"; // 当前信道
var cgi_wlBssid = "wlBssid"; // wlBssid值
var enbl = true; // 取wlEnbl值；true 表示启用无线, false 则不启用。
var ssid = "CMCC-one"; // SSID值
var txPower = "100"; // 发送功率
//var hiddenSSID = true; // 取消广播的开关, false 表示启用广播，true 表示不启用广播
var wlanMode = 1; // WLAN模式
var wlBandMode = 0;
var ssid_2g = "<% checkWrite("2G_ssid"); %>";
var ssid_5g = "<% checkWrite("5G_ssid"); %>";
var defaultBand = <% checkWrite("band"); %>;
var _Band2G5GSupport = new Array();
var regDomain; //, defaultChan;
var _wlanEnabled = new Array();
var _band = new Array();
var _ssid = new Array();
var _bssid = new Array();
var _chan = new Array();
var _chanwid = new Array();
var _ctlband = new Array();
var _txRate = new Array();
var _txpower = new Array();
var _txpower_high = new Array();
var _hiddenSSID = new Array();
var _wlCurrentChannel = new Array();
var _auto = new Array();
var _rf_used = new Array();
var _shortGI0 = new Array();
var wlan_num = <% checkWrite("wlan_num"); %>;
var wlan_support_8812e = <% checkWrite("wlan_support_8812e") %>;
var dfs_enable=<% checkWrite("dfs_enable"); %>;
var wlan_sta_control = 0;
var wlan_module_enable = <% checkWrite("wlan_module_enable"); %>;
var wlan_rate_prior_enable = <% checkWrite("wlan_rate_prior_enable"); %>;
var wlan_txpower_high_enable = <% checkWrite("wlan_txpower_high_enable"); %>;
var _wlan_rate_prior = new Array();
var _wlan_11k = new Array();
var _wlan_11v = new Array();
var _wlan_beacon_interval = new Array();
var _wlan_dtim_period = new Array();
var channel_list_5g_dfs =[[],
					  [36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165],
					  [36,40,44,48,52,56,60,64,149,153,157,161],
					  [36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140],
					  [36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140],
					  [36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140],
					  [36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140],
					  [36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140],
					  [34,38,42,46],
					  [36,40,44,48],
					  [36,40,44,48,52,56,60,64],
					  [56,60,64,100,104,108,112,116,136,140,149,153,157,161,165],
					  [36,40,44,48,52,56,60,64,132,136,140,149,153,157,161,165],
					  [36,40,44,48,52,56,60,64,149,153,157,161,165],
    			      [36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165],
    			      [36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165],
    			      [36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,144,149,153,157,161,165,169,173,177]
					  ];


var channel_list_5g =[[],
					  [36,40,44,48,149,153,157,161,165],
					  [36,40,44,48,149,153,157,161],
					  [36,40,44,48],
					  [36,40,44,48],
					  [36,40,44,48],
					  [36,40,44,48],
					  [36,40,44,48],
					  [34,38,42,46],
					  [36,40,44,48],
					  [36,40,44,48],
					  [56,60,64,149,153,157,161,165],
					  [36,40,44,48,149,153,157,161,165],
					  [36,40,44,48,149,153,157,161,165],
    			      [36,40,44,48,149,153,157,161,165],
    			      [36,40,44,48,149,153,157,161,165],
    			      [36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,144,149,153,157,161,165,169,173,177]
					  ];
var user_mode = <% checkWrite("user_mode"); %>;
<% init_wlan_page(); %>
// <% checkWrite("wl_txRate"); %>
<% checkWrite("wl_chno"); %>
/********************************************************************
**          on document load
********************************************************************/
function on_init()
{
	with (document.wlanSetup) {
		/* Tsai: the initial value */
		wlanEnabled.checked = wlan_module_enable;
		ssid.value = "<% getInfo("ssid"); %>";
		if(ssid.value.substr(0,5) == "CMCC-")
			ssid.value = ssid.value.substr(5);
		txpower.selectedIndex = _txpower[0];
		hiddenSSID.checked = _hiddenSSID[0];
		elements.wlan_idx.value = 0;
		chanwid.selectedIndex = _chanwid[0];
		shortGI0[_shortGI0[0]==0?1:0].checked = true;
		/* Tsai: show or hide elements */
		wlSecCbClick(wlanEnabled);

		if(ssid_2g.substr(0,5) == "CMCC-")
			ssid_2g = ssid_2g.substr(5);
		if(ssid_5g.substr(0,5) == "CMCC-")
			ssid_5g = ssid_5g.substr(5);
	}
}

/********************************************************************
**          on document update
********************************************************************/
function wlSecCbClick(cb) 
{
	
	var status = cb.checked;
	with (document.wlanSetup) {
		wlSecAll.style.display = status ? "block" : "none";
		if(status)
			advanced.style.display = wlSecInfo.style.display;
		else
			advanced.style.display = "none";
	}
	/*
	var band_value = parseInt(document.wlanSetup.band.value, 10) + 1;

	with (document.wlanSetup) {
		ssid.disabled = !status;
		hiddenSSID.disabled = !status;

		wlSecInfo.style.display = status ? "block" : "none";
		optionfor11n.style.display = (status && (band_value & 8)) ? "block" : "none";
		adminWlinfo.style.display = status ? "block" : "none";
		advanced.style.display = status ? "block" : "none";
  	}
	*/
		
}

function validateNum(str)
{
  for (var i=0; i<str.length; i++) {
        if ( !(str.charAt(i) >='0' && str.charAt(i) <= '9')) {
                alert("无效的值。它应该在十进制数字 (0-9)");
                return false;
        }
  }
  return true;
}

/********************************************************************
**          on document submit
********************************************************************/
function on_submit() 
{
	var str, band, basicRate = 0, operRate = 0, num;


		if (document.wlanSetup.wlanEnabled.checked) {
			str = document.wlanSetup.ssid.value;

			//if (str.length == 0) {
			//	alert("SSID不能为空.");
			//	return;
			//}

			if (str.length > 27) {
				alert('SSID "CMCC-' + str + '" 不能大于32个字符。');
				return;
			}

			if (isIncludeInvalidChar(str)) {
				alert("SSID 含有非法字符，请重新输入!");
				return;
			}

			if ( validateNum(document.wlanSetup.beaconInterval.value) == 0 ) {
				document.wlanSetup.beaconInterval.focus();
				return;
			}

			num = parseInt(document.wlanSetup.beaconInterval.value);
			if (document.wlanSetup.beaconInterval.value=="" || num < 20 || num > 1024) {
				alert("Beacon信标帧发送间隔应介于20~1024之间");
				document.wlanSetup.beaconInterval.focus();
				return;
			}

			if ( validateNum(document.wlanSetup.dtimPeriod.value) == 0 ) {
				document.wlanSetup.dtimPeriod.focus();
				return;
			}

			num = parseInt(document.wlanSetup.dtimPeriod.value);
			if (document.wlanSetup.dtimPeriod.value=="" || num < 1 || num > 255) {
				alert("DTIM间隔隔应介于1~255之间");
				document.wlanSetup.dtimPeriod.focus();
				return;
			}


			band = parseInt(document.wlanSetup.band.value, 10) + 1;

			/* band:
			   bit 0 == 802.11b, bit 1 == 802.11g,
			   bit 2 == 802.11a, bit 3 == 802.11n
			*/

			/* basicRate, operRate:
			   bit 0 == 1M
			   bit 1 == 2M
			   bit 2 == 5M
			   bit 3 == 11M
			   bit 4 == 6M
			   bit 5 == 9M
			   bit 6 == 12M
			   bit 7 == 18M
			   bit 8 == 24M
			   bit 9 == 36M
			   bit 10 == 48M
			   bit 11 == 54M
			*/ 

			/* 802.11b */
			if (band & 1) {
				operRate |= 0xf;
				basicRate |= 0xf;
			}

			/* 802.11g, 802.11a */
			if ((band & 2) || (band & 4)) {
				operRate |= 0xff0;
				if  (!(band & 1))
					basicRate |= 0xff0;
			}

			/* 802.11n */
			if (band & 8) {
				if (!(band & 3))
					operRate |= 0xfff;

				if (band & 2)
					basicRate = 0x1f0;
				else
					basicRate = 0xf;
			}

			operRate |= basicRate;
			document.wlanSetup.basicrates.value = basicRate;
			document.wlanSetup.operrates.value = operRate;
		}
		document.wlanSetup.submit();

}

function on_adv() 
{
	var wlan_idx = document.wlanSetup.elements["wlan_idx"].value;
	var loc;
	if(wlan_num>1){
		if(wlan_sta_control==1)
			loc = 'boaform/admin/formWlanRedirect?redirect-url=/net_wlan_adv.asp&wlan_idx=0';
		else
		//loc = '/net_wlan_adv.asp?wlan_idx='+wlan_idx;
		loc = 'boaform/admin/formWlanRedirect?redirect-url=/net_wlan_adv.asp&wlan_idx='+wlan_idx;
	}
	else
		loc = '/net_wlan_adv.asp';
	var code = 'location.assign("' + loc + '")';

	eval(code);
}

function on_dot11r() 
{
	var wlan_idx = document.wlanSetup.elements["wlan_idx"].value;
	var loc;
	if(wlan_num>1)
		loc = 'boaform/admin/formWlanRedirect?redirect-url=/net_wlan_ft.asp&wlan_idx='+wlan_idx;
	else
		loc = '/net_wlan_ft.asp';
	var code = 'location.assign("' + loc + '")';

	eval(code);
}

function get_rate(curRate)
{
	var rate_mask = new Array(31,1,1,1,1,2,2,2,2,2,2,2,2,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8);
	var rate_name = new Array("Auto","1M","2M","5.5M","11M","6M","9M","12M","18M","24M","36M","48M","54M", "MCS0", "MCS1",
		"MCS2", "MCS3", "MCS4", "MCS5", "MCS6", "MCS7", "MCS8", "MCS9", "MCS10", "MCS11", "MCS12", "MCS13", "MCS14", "MCS15");
	vht_rate_name=["NSS1-MCS0","NSS1-MCS1","NSS1-MCS2","NSS1-MCS3","NSS1-MCS4",
		"NSS1-MCS5","NSS1-MCS6","NSS1-MCS7","NSS1-MCS8","NSS1-MCS9",
		"NSS2-MCS0","NSS2-MCS1","NSS2-MCS2","NSS2-MCS3","NSS2-MCS4",
		"NSS2-MCS5","NSS2-MCS6","NSS2-MCS7","NSS2-MCS8","NSS2-MCS9"];
	var rate, mask,idx=0, i;
	var band = parseInt(document.wlanSetup.band.value, 10) + 1;
	var wlan_idx = document.wlanSetup.elements["wlan_idx"].value;
	var rf_num = _rf_used[wlan_idx];
	var auto = _auto[wlan_idx];
	
	// band:
	//   bit 0 == 802.11b, bit 1 == 802.11g,
	//  bit 2 == 802.11a, bit 3 == 802.11n
	//

	mask = 0;
	if (auto)
		curRate = 0;
	if (band & 1)
		mask |= 1;
	if ((band & 2) || (band & 4))
		mask |= 2;
	if (band & 8) {
		if (rf_num == 2)
			mask |= 12;	
		else
			mask |= 4;
	}
	if(band & 64){
		mask |= 16;
	}

	document.wlanSetup.txRate.length = 0;
	
	for (i = 0; i < rate_mask.length; i++) {
		if (rate_mask[i] & mask) {
			rate = (i == 0) ? 0 : 1 << (i - 1);
			document.wlanSetup.txRate.options[idx++] = new Option(rate_name[i], i, false, curRate == rate);
		}
	}
	if(band & 64){
		for (i=0; i<vht_rate_name.length; i++) {
			rate = (((1 << 31)>>>0) + i);
			document.wlanSetup.txRate.options[idx++] = new Option(vht_rate_name[i], (i+30), false, curRate == rate);
		}
	}

}

function updatePage(byHand)
{
	var band = parseInt(document.wlanSetup.band.value, 10) + 1;

	showhide("optionfor11n", (band >= 8) && document.wlanSetup.wlanEnabled.checked);

	updateChannel(byHand);
}

function updateChannel(byHand)
{
	var idx_value= document.wlanSetup.band.selectedIndex;
	var band_value= parseInt(document.wlanSetup.band.value, 10) + 1; 

	/* Tsai: change channel by hand ? */
	if (byHand) {
		wlan_channel = parseInt(document.wlanSetup.chan.value, 10);
	} else {
		wlan_channel = _chan[document.wlanSetup.elements["wlan_idx"].value];
	}

	showcontrolsideband_updated(band_value, idx_value);
	showchannelbound_updated(band_value, byHand);

	updateChannelBound();

	/* Tsai: update wlan_channel because chan may be changed */
	wlan_channel = parseInt(document.wlanSetup.chan.value, 10);

	if (document.wlanSetup.chanwid.selectedIndex == 0 || wlan_channel == 0)
		document.wlanSetup.ctlband.disabled = true;
	else
		document.wlanSetup.ctlband.disabled = false;

	if(document.wlanSetup.chan.disabled == true)
		document.wlanSetup.ctlband.disabled = true;
}

function updateChannel2()
{
	var idx_value= document.wlanSetup.band.selectedIndex;
	var band_value= parseInt(document.wlanSetup.band.value, 10) + 1; 

	wlan_channel = parseInt(document.wlanSetup.chan.value, 10);

	showcontrolsideband_updated(band_value, idx_value);

	updateChannelBound();

	/* Tsai: update wlan_channel because chan may be changed */
	wlan_channel = parseInt(document.wlanSetup.chan.value, 10);

	if (document.wlanSetup.chanwid.selectedIndex == 0 || wlan_channel == 0)
		document.wlanSetup.ctlband.disabled = true;
	else
		document.wlanSetup.ctlband.disabled = false;

	if(document.wlanSetup.chan.disabled == true)
		document.wlanSetup.ctlband.disabled = true;
}

function updateChan_selectedIndex()
{
	var chan_number_idx=document.wlanSetup.chan.selectedIndex;
	var chan_number= document.wlanSetup.chan.options[chan_number_idx].value;
	var band_value= parseInt(document.wlanSetup.band.value, 10);

	wlan_channel = chan_number;
	if(chan_number == 0)
		disableTextField(document.wlanSetup.ctlband);	
	else{
		if(document.wlanSetup.chanwid.selectedIndex == "0")
 			disableTextField(document.wlanSetup.ctlband);	
		else if(document.wlanSetup.chanwid.selectedIndex == "2" && (band_value == 75 || band_value == 71|| band_value ==63))
 			disableTextField(document.wlanSetup.ctlband);
 		else
			enableTextField(document.wlanSetup.ctlband);		
	}

	if((wlan_support_8812e==1) && (chan_number > 14)) //8812
		disableTextField(document.wlanSetup.ctlband);	
}

function updateChannelBound()
{
	var band = parseInt(document.wlanSetup.band.value, 10) + 1;
	var bound = document.wlanSetup.chanwid.selectedIndex;	/* 0:20Mhz, 1:40Mhz */
	var adjust;
	var idx_value= document.wlanSetup.band.selectedIndex;

	/* band:
	   bit 0 == 802.11b, bit 1 == 802.11g,
	   bit 2 == 802.11a, bit 3 == 802.11n
	*/

	/* Tsai: 802.11n */
	adjust = (band & 8) ? bound : 0;

	if (band & 4 || (band == 8 && idx_value == 1) || (band  & 64)) {
		/* Tsai: 802.11a and 802.11n and 802.11ac */
		showChannel5G(adjust);
		document.wlanSetup.chan.disabled = false;
	} else {
		/* Tsai: 802.11b, 802.11g and 802.11n */
		adjust = (adjust != 0) ? 1 : 0;
		showChannel2G(band, adjust);
		document.wlanSetup.chan.disabled = false;
	}
}

function showChannel5G(bound_40)
{
	var chan, idx = 0;

	/* Tsai: remove all options under chan */
	document.wlanSetup.chan.length = 0;

	if (dfs_enable == 1)		
			document.wlanSetup.chan.options[idx++] = new Option("Auto(DFS)", 0, false, 0 == wlan_channel);
	else
			document.wlanSetup.chan.options[idx++] = new Option("Auto", 0, false, 0 == wlan_channel);


	if(wlan_support_8812e ==1)
	{
		var bound = document.wlanSetup.chanwid.selectedIndex;
		var channel_list;
		var chan, chan_pair, i, ii, iii, iiii, found; 

		if(regDomain==8) //MKK1
		{
			channel_list = channel_list_5g_dfs[regDomain];
			
			if(bound <= 2){
				for(i=0; i<channel_list.length; i++){
					chan = channel_list[i];
					document.wlanSetup.chan.options[idx] = new Option(chan, chan, false, chan == wlan_channel);
					idx++;
				}
			}
			
		}
		else{

			channel_list = channel_list_5g_dfs[regDomain];
			
			for (i=0; i<channel_list.length; i++) {
				
					chan = channel_list[i];
								
					if(regDomain != 16 && regDomain != 11)
						if((dfs_enable == 0) && (chan >= 52) && (chan <= 144))
							continue;

               		if(regDomain == 11)
                		if((dfs_enable == 0) && (chan >= 100) && (chan <= 140))
                    		continue;

					if(regDomain != 16){
						if(bound==1){
							for(ii=0; ii<channel_list_5g_dfs[16].length; ii++){
								if(chan == channel_list_5g_dfs[16][ii])
									break;
							}
							if(ii%2==0)
								chan_pair = channel_list_5g_dfs[16][ii+1];
							else
								chan_pair = channel_list_5g_dfs[16][ii-1];
							
							found = 0;
							for(ii=0; ii<channel_list.length; ii++){
								if(chan_pair == channel_list[ii]){
									found = 1;
									break;
								}
							}
							if(found == 0)
								chan=0;
						}
						else if(bound==2){
							for(ii=0; ii<channel_list_5g_dfs[16].length; ii++){
								if(chan == channel_list_5g_dfs[16][ii])
									break;
							}
							for(iii=(ii-(ii%4)); iii<((ii-(ii%4)+3)) ; iii++)
							{
								found = 0;
								chan_pair = channel_list_5g_dfs[16][iii];
								for(iiii=0; iiii<channel_list.length; iiii++){
									if(chan_pair == channel_list[iiii]){
										found = 1;
										break;
									}
								}
								if(found==0){
									chan=0;
									break;
								}
							}

							
						}
					}
					
					if(chan!=0){
						document.wlanSetup.chan.options[idx] = new Option(chan, chan, false, chan == wlan_channel);
						idx++;
					}
			}
		}
	/*
		var bound = document.wlanSetup.chanwid.selectedIndex;
		var channel_list;
		if(dfs_enable == 1) // use dfs channel
			channel_list = channel_list_5g_dfs[regDomain];
		else
			channel_list = channel_list_5g[regDomain];

		for (chan = 0; chan < channel_list.length; idx ++, chan ++) {
			if(bound != 0 && channel_list[chan] == 165) // channel 165 only exist 20M channel bandwidth
				break;
			document.wlanSetup.chan.options[idx] = new Option(channel_list[chan], channel_list[chan], false, false);
			if (channel_list[chan] == defaultChan) {
				document.wlanSetup.chan.selectedIndex = idx;
				defChanIdx = idx;
			}
		}
	*/
	/*
		var bound = document.wlanSetup.chanwid.selectedIndex;
		var inc_scale;
		var chan_end;
		
		inc_scale = 4;
		chan_str = 36;

		if (dfs_enable == 1)
			chan_end = 64;
		else
			chan_end = 48;

		for (chan = chan_str; chan <= chan_end; idx ++, chan += inc_scale) {
			document.wlanSetup.chan.options[idx] = new Option(chan, chan, false, false);
			if (chan == defaultChan) {
				document.wlanSetup.chan.selectedIndex = idx;
				defChanIdx = idx;
			}
		}
		if (dfs_enable == 1)
		{
			chan_str = 149;
			chan_end = 161;
			var bound = document.wlanSetup.chanwid.selectedIndex;
			if(bound == 0) // 20MHz
				chan_end = 165;
		
			for (chan = chan_str; chan <= chan_end; idx ++, chan += inc_scale) {
				document.wlanSetup.chan.options[idx] = new Option(chan, chan, false, false);
				if (chan == defaultChan) {
					document.wlanSetup.chan.selectedIndex = idx;
					defChanIdx = idx;
				}
			}
		}
	*/
	}
	else
	if (regDomain == 6) {	// MKK
		for (chan = 34; chan <= 64; chan += 2) {
			if ((chan == 50) || (chan == 54) || (chan == 58) || (chan == 62)) {
				continue;
			}

			document.wlanSetup.chan.options[idx++] = new Option(chan, chan, false, chan == wlan_channel);
		}
	} else {
		var start, end;

		if (bound_40 == 1) {
			var sideBand = document.wlanSetup.ctlband.selectedIndex;

			if (sideBand == 0) {	// upper
				start = 40;
				end = 64;
			} else {		// lower
				start = 36;
				end = 60;
			}
		} else {
			start = 36;
			end = 64;
		}

		for (chan = start; chan <= end; chan += (4 * (bound_40 + 1))) {
			document.wlanSetup.chan.options[idx++] = new Option(chan, chan, false, chan == wlan_channel);
		}
	}
}

function showChannel2G(band, bound_40)
{
	var start = 0, end = 0, idx = 0;

	/* band:
	   bit 0 == 802.11b, bit 1 == 802.11g,
	   bit 2 == 802.11a, bit 3 == 802.11n
	*/

	if (regDomain == 1 || regDomain == 2) {
		start = 1;
		end = 11;
	} else if (regDomain == 3) {
		start = 1;
		end = 13;
	} else if (regDomain == 4) {
		start = 1;
		end = 13;
	} else if (regDomain == 5) {
		start = 10;
		end = 13;
	} else if (regDomain == 6) {
		start = 1;
		end = 14;
	} else if (regDomain == 7) {
		start = 3;
		end = 13;
	} else if (regDomain == 8 || regDomain == 9 || regDomain == 10) {
		start = 1;
		end = 14;
	} else if (regDomain == 11) {
		start = 1;
		end = 11;
	} else if (regDomain == 12) {
		start = 1;
		end = 13;
	} else if (regDomain == 13) {
		start = 1;
		end = 13;
	} else if (regDomain == 14) {
		start = 1;
		end = 14;
	} else if (regDomain == 15) {
		start = 1;
		end = 13;
	} else if (regDomain == 16) {
		start = 1;
		end = 14;
	} else {		//wrong regDomain ?
		start = 1;
		end = 11;
	}

	/* Tsai: 802.11n */
	if (band & 8) {
		if (bound_40 == 1) {
			var sideBand = document.wlanSetup.ctlband.selectedIndex;

			if (regDomain == 5) {
				if (sideBand == 0) {	//upper
					start = 13;
					end = 13;
				} else {		//lower
					start = 10;
					end = 10;
				}
			} else {
				if(sideBand == 0){  //upper
					start = start+4;
					if(end == 14)
						end = 13			
					
				}else{ //lower
					if(end == 14)
						end = 13;
					end = end-4;
				}
			}
		}
	}

	/* Tsai: remove all options under chan */
	document.wlanSetup.chan.length = 0;

	document.wlanSetup.chan.options[idx++] = new Option("Auto", 0, false, 0 == wlan_channel);

	for (chan = start; chan <= end; chan++) {
		document.wlanSetup.chan.options[idx++] = new Option(chan, chan, false, chan == wlan_channel);
	}
}

function updateBand(band)
{
	var idx=0;
	var select_idx = document.wlanSetup.elements["wlan_idx"].value;
	var Band2G5GSupport = _Band2G5GSupport[select_idx];

	document.wlanSetup.band.length = 0;

	if(Band2G5GSupport == 2 || wlBandMode == 1) // 2:PHYBAND_5G 1:BANDMODESIGNLE
	{
		document.wlanSetup.band.options[idx++] = new Option("5 GHz (A)", "3", false, false);
        document.wlanSetup.band.options[idx++] = new Option("5 GHz (N)", "7", false, false);
        document.wlanSetup.band.options[idx++] = new Option("5 GHz (A+N)", "11", false, false);

		if(wlan_support_8812e==1){
			document.wlanSetup.band.options[idx++] = new Option("5 GHz (AC)", "63", false, false); //8812
			document.wlanSetup.band.options[idx++] = new Option("5 GHz (N+AC)", "71", false, false); //8812
			document.wlanSetup.band.options[idx++] = new Option("5 GHz (A+N+AC)", "75", false, false); //8812
		}
	}

	if(Band2G5GSupport == 1 || wlBandMode == 1) // 1:PHYBAND_2G 1:BANDMODESIGNLE
	{
		document.wlanSetup.band.options[idx++] = new Option("2.4 GHz (B)", "0", false, false);
		document.wlanSetup.band.options[idx++] = new Option("2.4 GHz (G)", "1", false, false);
        document.wlanSetup.band.options[idx++] = new Option("2.4 GHz (B+G)", "2", false, false);
        document.wlanSetup.band.options[idx++] = new Option("2.4 GHz (N)", "7", false, false);
        document.wlanSetup.band.options[idx++] = new Option("2.4 GHz (G+N)", "9", false, false);
        document.wlanSetup.band.options[idx++] = new Option("2.4 GHz (B+G+N)", "10", false, false);
	}


	for (idx = 0; idx < document.wlanSetup.band.length; idx++) {
		if (document.wlanSetup.band.options[idx].value == band) {
			/* Tsai: 802.11n */
			if (band == 7) {
				var c = document.wlanSetup.band.options[idx].text.charAt(0);

				if ((Band2G5GSupport == 2 && c == "5")		//Band2G5GSupport=2:PHYBAND_5G
					|| (Band2G5GSupport == 1 && c == "2")) {		//Band2G5GSupport=1:PHYBAND_2G
					document.wlanSetup.band.selectedIndex = idx;
					break;
				}
			} else {
				document.wlanSetup.band.selectedIndex = idx;
				break;
			}
		}
	}
}

function Set_SSIDbyBand()
{
	var c;

	with (document.wlanSetup) {
		c = band.options[band.selectedIndex].text.charAt(0);

		if (c == "5")
			ssid.value = ssid_5g;
		else if (c == "2")
			ssid.value = ssid_2g;
	}
}

function showcontrolsideband_updated(band, idx_value)
{
	var idx=0;
	var i;
  	var controlsideband_idx = document.wlanSetup.ctlband.selectedIndex;
	var controlsideband_str = document.wlanSetup.ctlband.options[controlsideband_idx].value;

	document.wlanSetup.ctlband.length = 0;

  	if(wlan_support_8812e ==1 && ((band==8 && idx_value==1) || band ==12 || band==64 || band==72 || band ==76))
	{
		document.wlanSetup.ctlband.options[idx++] = new Option("Auto", "0", false, false);
		document.wlanSetup.ctlband.options[idx++] = new Option("Auto", "1", false, false);
  	}
  	else
	{
		document.wlanSetup.ctlband.options[idx++] = new Option("Upper", "0", false, false);
		document.wlanSetup.ctlband.options[idx++] = new Option("Lower", "1", false, false);
  	}
  	document.wlanSetup.ctlband.length = idx;
 
	//document.wlanSetup.ctlband.selectedIndex = controlsideband_idx;

	for (i=0; i<idx; i++) {
		if(controlsideband_str == document.wlanSetup.ctlband.options[i].value)
			document.wlanSetup.ctlband.selectedIndex = i;
	}
}


function showchannelbound_updated(band, byHand)
{
	var idx=0;
	var i;
	var channelbound_idx;
//	var channelbound_str = document.wlanSetup.chanwid.options[channelbound_idx].value;
	var idx_value = document.wlanSetup.band.selectedIndex;

	if(byHand)
		channelbound_idx = document.wlanSetup.chanwid.selectedIndex;
	else
		channelbound_idx = _chanwid[document.wlanSetup.elements["wlan_idx"].value]; 

	

	document.wlanSetup.chanwid.length = 0;

	document.wlanSetup.chanwid.options[idx++] = new Option("20MHz", "0", false, false);
	document.wlanSetup.chanwid.options[idx++] = new Option("40MHz", "1", false, false);

	if (band & 4 || (band == 8 && idx_value == 1) || (band & 64)) {
		if(band & 64)
			document.wlanSetup.chanwid.options[idx++] = new Option("80MHz", "2", false, false);
	}
	else{
		document.wlanSetup.chanwid.options[idx++] = new Option("20/40MHz", "2", false, false);
	}
	
 	document.wlanSetup.chanwid.length = idx;
 
	if(channelbound_idx > (idx-1))
		document.wlanSetup.chanwid.selectedIndex = idx - 1;
	else
		document.wlanSetup.chanwid.selectedIndex = channelbound_idx;
/*	
	for (i=0; i<idx; i++) {
		if(channelbound_str == document.wlanSetup.chanwid.options[i].value)
			document.wlanSetup.chanwid.selectedIndex = i;
	}
*/

}

function wlRatePriorCbClick(cb) 
{
	
	var status = cb.checked;
	var idx = document.wlanSetup.elements["wlan_idx"].value;

	with (document.wlanSetup) {
		band.disabled = status? true : false;
		chanwid.disabled = status? true : false;
		if(status){
			if(_Band2G5GSupport[idx]==2)
				updateBand(63); //ac
			else
				updateBand(7);
			get_rate(_txRate[idx]);
			chanwid.selectedIndex = 2;
			updateChannel(true);
		}
		else{
			updateBand(_band[idx]);
			get_rate(_txRate[idx]);
			updateChannel(false);
		}
  	}
		
}

function wlDot11kChange()
{
	if(document.wlanSetup.elements.namedItem("dot11vEnabled")!= null){
		with (document.wlanSetup) {
			dot11v.style.display = dot11kEnabled[0].checked? "" : "none";
		}
	}
}


function showtxpower_updated(band, value, value_high)
{
	var idx=0;
	var idx_value = document.wlanSetup.band.selectedIndex;
	var i;

	document.wlanSetup.txpower.length = 0;
	if(wlan_txpower_high_enable){
		if (band & 4 || (band == 8 && idx_value == 1) || (band & 64)){
			document.wlanSetup.txpower.options[idx++] = new Option("100%", "0", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("70%", "1", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("50%", "2", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("35%", "3", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("15%", "4", false, false);
		}
		else{
			document.wlanSetup.txpower.options[idx++] = new Option("200%", "5", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("100%", "0", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("70%", "1", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("50%", "2", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("35%", "3", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("15%", "4", false, false);
			if(value == 0 && value_high == 1){
				value = 5;
			}
		}
	}
	else{
		document.wlanSetup.txpower.options[idx++] = new Option("100%", "0", false, false);
		document.wlanSetup.txpower.options[idx++] = new Option("70%", "1", false, false);
		document.wlanSetup.txpower.options[idx++] = new Option("50%", "2", false, false);
		document.wlanSetup.txpower.options[idx++] = new Option("35%", "3", false, false);
		document.wlanSetup.txpower.options[idx++] = new Option("15%", "4", false, false);
	}
	document.wlanSetup.txpower.length = idx;
	
	for (i=0; i<idx; i++) {
		if(value == document.wlanSetup.txpower.options[i].value)
			document.wlanSetup.txpower.selectedIndex = i;
	}
}

function wlEnableSSIDClick(cb) 
{
	
	var status = cb.checked;

	var band_value = parseInt(document.wlanSetup.band.value, 10) + 1;

	with (document.wlanSetup) {
		wlSecInfo.style.display = status ? "block" : "none";
		optionfor11n.style.display = (status && (band_value >= 8)) ? "block" : "none";
		adminWlinfo.style.display = status ? "block" : "none";

		if(wlanEnabled.checked==false)
			advanced.style.display = "none";
		else
			advanced.style.display = status ? "block" : "none";

	}
}

function BandSelected(index)
{
//	document.wlanSetup.wlanEnabled.checked = _wlanEnabled[index];
	document.wlanSetup.band.value = _band[index];
	document.wlanSetup.elements["wlan_idx"].value = index;
	document.wlanSetup.wlSsidIdx.length = 0;
	if(index==0)
		document.wlanSetup.wlSsidIdx.options[0] = new Option("SSID1", "0", false, false);
	else
		document.wlanSetup.wlSsidIdx.options[0] = new Option("SSID5", "0", false, false);
	document.wlanSetup.wlSsidIdx.length = 1;
	if(wlan_sta_control==1 && index==1)
		document.wlanSetup.ssid.disabled = true;
	else
		document.wlanSetup.ssid.disabled = false;
	document.wlanSetup.ssid.value = _ssid[index];
	if(document.wlanSetup.ssid.value.substr(0,5) == "CMCC-")
		document.wlanSetup.ssid.value = document.wlanSetup.ssid.value.substr(5);
	document.getElementById('wlBssid').innerHTML = _bssid[index];
	//alert(_bssid[index]);
	document.getElementById('cur_wlChannel').innerHTML = '当前信道: '+ _wlCurrentChannel[index];
	//document.wlanSetup.txpower.selectedIndex = _txpower[index];
	document.wlanSetup.hiddenSSID.checked = _hiddenSSID[index];
	document.getElementsByName('select_2g5g')[index].checked = true;
//	document.wlanSetup.chan.value = _chan[index];
	document.wlanSetup.chanwid.selectedIndex = _chanwid[index];
	document.wlanSetup.ctlband.selectedIndex = _ctlband[index];
	document.wlanSetup.shortGI0[_shortGI0[index]==0?1:0].checked = true; 
	if(document.wlanSetup.elements.namedItem("dot11kEnabled")!= null)	
		document.wlanSetup.dot11kEnabled[_wlan_11k[index]==0?1:0].checked = true;
	if(document.wlanSetup.elements.namedItem("dot11vEnabled")!= null){
		document.wlanSetup.dot11vEnabled[_wlan_11v[index]==0?1:0].checked = true;
		document.getElementById("dot11v").style.display = _wlan_11k[index]? "" : "none";
	}
	document.wlanSetup.enableSSID.checked = _wlanEnabled[index]? true: false;
	
	txrate = _txRate[index];
	auto = _auto[index];
	updateBand(_band[index]);
	get_rate(txrate);
	updatePage(false);
	Set_SSIDbyBand();
	//wlan_idx = index;

	showtxpower_updated(_band[index], _txpower[index], _txpower_high[index]);

	var status = _wlanEnabled[index];
	var band_value = parseInt(document.wlanSetup.band.value, 10) + 1;

	with (document.wlanSetup) {
		wlSecInfo.style.display = status ? "block" : "none";
		optionfor11n.style.display = (status && (band_value >= 8)) ? "block" : "none";
		adminWlinfo.style.display = status ? "block" : "none";

		if(wlanEnabled.checked==false)
			advanced.style.display = "none";
		else
			advanced.style.display = status ? "block" : "none";

		wlRatePrior.style.display = wlan_rate_prior_enable ? "block" : "none";
		wlanRatePrior.checked = _wlan_rate_prior[index];
		if(wlan_rate_prior_enable == 1 && _wlan_rate_prior[index]==1){
			band.disabled = true;
			if(_Band2G5GSupport[index]==2)
				updateBand(63); //ac
			else
				updateBand(7);
			get_rate(txrate);
			chanwid.disabled = true;
			chanwid.selectedIndex = 2;
			updateChannel(true);
		}
		else{
			band.disabled = false;
			chanwid.disabled = false;
		}
		wlBand.style.display = user_mode ? "" : "none";
		ShortGI.style.display = user_mode ? "" : "none";
		beaconInterval.value = _wlan_beacon_interval[index];
		dtimPeriod.value = _wlan_dtim_period[index];
  	}
}

</script>
</head>
   
<!-------------------------------------------------------------------------------------->
<!--主页代码-->

<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onload="BandSelected(0)">
	<blockquote>
	<DIV align="left" style="padding-left:20px; padding-top:10px">
	<form action=/boaform/admin/formWlanSetup method="post" name="wlanSetup">
	<b>无线设置 -- 基本</b><br>
	<br>
    本页配置无线LAN 口的基本特性。 包括启用或禁用无线LAN口、从工作站的AP扫描搜索中 隐藏SSID、 设置无线网络名(即SSID)。<br>
	<br>
    点击"保存/应用"，无线设置的基本配置生效。<br>
	<br>
	<table border="0" cellpadding="4" cellspacing="0">
		<tr>
			<td valign="middle" align="center" width="30" height="30">
				<input type='checkbox' name='wlanEnabled' onClick='wlSecCbClick(this);' value="ON"></td>
			<td>启用无线</td>
		</tr>
	</table>
	<div id='wlSecAll'>
	<div id='wlSecInfoDualband'>
	<% checkWrite("wlan_sta_control"); %>
	<table border="0" cellpadding="5" cellspacing="0">
		<tr>
		<td width=100% colspan="2">
			<% checkWrite("wlbandchoose"); %>
		</td>
		</tr>
	</table>
	</div>
	<table border="0" cellpadding="4" cellspacing="0"  width=400>
		<tr>
			<td width="45%">SSID 使能:</td>
			<td valign="middle" width="30" height="30">
				<input type='checkbox' name='enableSSID' onClick='wlEnableSSIDClick(this);' value="ON"></td>
		</tr>
		<tr>
			<td width="45%">SSID 索引:</td>
			<td colspan="2"><select name="wlSsidIdx">
				<option value="0">SSID1</option>
			</select></td>
		</tr>
	</table>
	<div id='wlSecInfo'>
	<table border="0" cellpadding="4" cellspacing="0">
		<tr id="wlRatePrior" style="display:none">
			<td valign="middle" align="center" width="30" height="30">
				<input type='checkbox' name='wlanRatePrior' onClick='wlRatePriorCbClick(this);' value="ON"></td>
			<td>启用速率优先</td>
		</tr>
	</table>
	<table border="0" cellpadding="4" cellspacing="0" width=400>
		<tr id="wlBand" style="display: none"> 
			<td width="45%">模式:</td>
			<td colspan="2">
				<select name=band size="1" onChange="updatePage(true); Set_SSIDbyBand(); get_rate(txrate);">
					<% checkWrite("wlband"); %>
				</select>
				<!--<SCRIPT>updateBand(defaultBand);</SCRIPT>-->
			</td>
		</tr>
		<tr>
			<td width="45%">SSID:</td>
			<td colspan="2">
				CMCC-<input type='text' name='ssid' maxlength="27" value=""></td>
		</tr>
		<tr>
			<td>BSSID:</td>
			<td id='wlBssid' colspan="2">
				<script language="javascript">
					document.writeln(cgi_wlBssid);
				</script>
			</td>
		</tr>
	</table>
	</div>
	<div  id="optionfor11n" style="display:none">
	<table border="0" cellpadding="4" cellspacing="0" width=400>
		<tr>
			<td width="45%">频带宽度:</td>
			<td colspan="2"><select size="1" name="chanwid" onChange="updateChannel2()">
				<% checkWrite("wlchanwid"); %>
				</select>
			</td>
		</tr>
		<tr>
			<td>控制边带:</td>
			<td colspan="2"><select size="1" name="ctlband" onChange="updateChannelBound()">
				<% checkWrite("wlctlband"); %>      		
				</select>
			</td>
		</tr> 
	</table>
	</div>
	<div id="adminWlinfo"  style="display:none">
	<table cellpadding="4" cellspacing="0" width=400>
		<tr>
			<td width="45%">信道:</td>
			<td><select name='chan' onChange="updateChan_selectedIndex()">
			</select></td>
			<td id='cur_wlChannel' width="26%">当前信道:
				<script language="javascript">
					document.writeln(cgi_wlCurrentChannel);
				</script>
			</td>
		</tr>
		<tr> 
			<td>速率:</td>
			<td colspan="2"><select name="txRate">
				<!--<SCRIPT>
					get_rate(txrate);
				</SCRIPT> -->
			</select></td> 
		</tr> 
		<tr>
			<td>Beacon信标帧发送间隔:</td>
			<td>
				<input type='text' name="beaconInterval" size="10" maxlength="4">
			</td>
			<td>(20-1024)</td>
		</tr>
		<tr>
			<td>DTIM间隔:</td>
			<td>
				<input type='text' name="dtimPeriod" size="5" maxlength="3">
			</td>
			<td>(1-255)</td>
		</tr>
		<tr> 
			<td>发送功率:</td>
			<td colspan="2"><select name="txpower" size="1">
				<% checkWrite("txpower"); %> 
			</select></td> 
		</tr>
		<tr>
			<td>取消广播:</td>
			<td valign="middle" width="30" height="30">
				<input type='checkbox' name='hiddenSSID' value="ON"></td>
		</tr>
		<tr id="ShortGI" style="display: none">
		<td>SGI:</td>
		<td> 
		<input type="radio" name="shortGI0" value="on">ON
		<input type="radio" name="shortGI0" value="off">OFF
		</td>
		</tr>
		<% ShowDot11r(); %>
		<% ShowDot11k_v(); %>
		<tr> 
			<td>&nbsp;</td>
			<td colspan="2">&nbsp;</td>
		</tr>
	</table>          
	</div>                  
	</div>
	<table width="295" border="0" cellpadding="4" cellspacing="0">
		<tr>
			<td><input type="hidden" value="/net_wlan_basic_11n_cmcc.asp" name="submit-url"></td>    
			<td width="162"><input class="btnsaveup" type='button' onClick='on_submit()' value='保存/应用'></td>
			<td><input class="btnaddup" type='button' onClick='on_adv()' name="advanced" value='高级'></td>
			<td><input type="hidden" name="wlan_idx" value=0></td>
			<td><input type="hidden" name="basicrates" value=0></td>
			<td><input type="hidden" name="operrates" value=0></td>
			
		</tr>
		<script>
				<% initPage("wle8basic"); %>
				on_init();
				//updatePage(); 
				//BandSelected(0);  
		</script>
	</table>
	</form>
	</DIV>
</blockquote>
</body>
<%addHttpNoCache();%>
</html>
