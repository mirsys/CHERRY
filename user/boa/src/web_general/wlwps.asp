<% SendWebHeadStr();%>
<title><% multilang(LANG_WI_FI_PROTECTED_SETUP); %></title>

<style>
.on {display:on}
.off {display:none}
</style>
<script>
var wps20;
var wps_version_configurable;
var isClient;
var isConfig;
var hidden_ssid=<% getInfo("hidden_ssid"); %>;
var wpa_cipher=<% getInfo("wpa_cipher"); %>;
var wpa2_cipher=<% getInfo("wpa2_cipher"); %>;
var encrypt=<% getInfo("encrypt"); %>;						
var enable1x=<% getInfo("enable1X"); %>;
var wpa_auth=<% getInfo("wpaAuth"); %>;
var mode=<% getInfo("wlanMode"); %>;
var is_adhoc=<% getInfo("networkType"); %>;
var autolockdown_stat=<% getInfo("lockdown_stat"); %>;
var wpsUseVersion=<% getInfo("wpsUseVersion"); %>;
var warn_msg1='WPS was disabled automatically because wireless mode setting could not be supported. ' +
				'You need go to Wireless/Basic page to modify settings to enable WPS.';
var warn_msg2='WPS was disabled automatically because Radius Authentication could not be supported. ' +
				'You need go to Wireless/Security page to modify settings to enable WPS.';
var warn_msg3="PIN number was generated. You have to click \'Apply Changes\' button to make change effectively.";
var warn_msg4='WPS was disabled automatically because broadcast SSID is disabled. ' +
				'You need go to Wireless/Advanced Settings page to modify settings to enable WPS.';
var warn_msg5='WPS was disabled automatically because Encryption type could not be WEP. ' +
				'You need go to Wireless/Security page to modify settings to enable WPS.';
var warn_msg6='WPS was disabled automatically because WPA only or TKIP only is not supported. ' +
				'You need go to Wireless/Security page to modify settings to enable WPS.';
var disable_all=0;

function noenter(e)
{
	if (window.event)	//IE
		return !(e.keyCode == 13);
	else if (e.which)	//Netscape/Firefox/Opera
		return !(e.which == 13);
}

function triggerPBCClicked(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
  	return true;
}

// WPS2DOTX , for brute force attack unlock
function unlockdownClicked(obj)
{	
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

function triggerPINClicked(obj)
{
	if(saveChangesWPS(document.formWsc))
	{
		obj.isclick = 1;
		postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
		return true;
	}
	else
		return false;
}

function compute_pin_checksum(val)
{
	var accum = 0;	
	var code = parseInt(val)*10;

	accum += 3 * (parseInt(code / 10000000) % 10); 
	accum += 1 * (parseInt(code / 1000000) % 10); 
	accum += 3 * (parseInt(code / 100000) % 10); 
	accum += 1 * (parseInt(code / 10000) % 10);
	accum += 3 * (parseInt(code / 1000) % 10);
	accum += 1 * (parseInt(code / 100) % 10);
	accum += 3 * (parseInt(code / 10) % 10); 
	accum += 1 * (parseInt(code / 1) % 10);	
	var digit = (parseInt(accum) % 10);
	return ((10 - digit) % 10);
}

function genPinClicked()
{
	var num_str="1";
	var rand_no;
	var num;

	while (num_str.length != 7) {
		rand_no = Math.random()*1000000000;	
		num = parseInt(rand_no, 10);
		num = num%10000000;
		num_str = num.toString();
	}
	
	num = num*10 + compute_pin_checksum(num);
	num = parseInt(num, 10);	
	document.formWsc.elements["localPin"].value = num; 
	alert(warn_msg3);
}

function validate_pin_code(code)
{
	var accum=0;

	accum += 3 * (parseInt(code / 10000000) % 10); 
	accum += 1 * (parseInt(code / 1000000) % 10); 
	accum += 3 * (parseInt(code / 100000) % 10); 
	accum += 1 * (parseInt(code / 10000) % 10);
	accum += 3 * (parseInt(code / 1000) % 10);
	accum += 1 * (parseInt(code / 100) % 10);
	accum += 3 * (parseInt(code / 10) % 10); 
	accum += 1 * (parseInt(code / 1) % 10);
	return (0 == (accum % 10));	
}

function check_pin_code(str)
{
	var i;
	var code_len;
		
	code_len = str.length;
	if (code_len != 8 && code_len != 4)
		return 1;

	for (i=0; i<code_len; i++) {
		if ((str.charAt(i) < '0') || (str.charAt(i) > '9'))
			return 2;
	}

	if (code_len == 8) {
		var code = parseInt(str, 10);
		if (!validate_pin_code(code))
			return 3;
		else
			return 0;
	}
	else
		return 0;
}

function setPinClicked(form)
{
	var ret;
	var str = form.elements["peerPin"].value;
	form.elements["peerPin"].value = str.replace(/[(\s)-]/g, "");

	ret = check_pin_code(form.elements["peerPin"].value);
	if (ret == 1) {
		alert('<% multilang(LANG_INVALID_ENROLLEE_PIN_LENGTH_THE_DEVICE_PIN_IS_USUALLY_FOUR_OR_EIGHT_DIGITS_LONG); %>');
		form.peerPin.focus();		
		return false;
	}
	else if (ret == 2) {
		alert('<% multilang(LANG_INVALID_ENROLLEE_PIN_ENROLLEE_PIN_MUST_BE_NUMERIC_DIGITS); %>');
		form.peerPin.focus();		
		return false;
	}
	else if (ret == 3) {
		if ( !confirm('<% multilang(LANG_CHECKSUM_FAILED_USE_PIN_ANYWAY); %>') ) {
			form.peerPin.focus();
			return false;
  		}
	}	
	form.setPIN.isclick = 1;
	postTableEncrypt(form.postSecurityFlag, form);
	return true;
}


function checkWPSstate(form)
{
	if (autolockdown_stat && !form.elements["disableWPS"].checked) {
		enableButton(form.elements["unlockautolockdown"]);		
	}else{
		disableButton(form.elements["unlockautolockdown"]);
	}  
	if (disable_all || form.elements["wlanDisabled"].value == "ON") {
		disableCheckBox(form.elements["disableWPS"]);
		disableButton(form.elements["save"]);
		disableButton(form.elements["reset"]);  
	}
	if (disable_all || form.elements["disableWPS"].checked || form.elements["wlanDisabled"].value == "ON") {	 	
	 	disableTextField(form.elements["localPin"]);	 	
	 	disableTextField(form.elements["peerPin"]);
	 	disableButton(form.elements["setPIN"]);
		disableButton(form.elements["triggerPIN"]);	 	
		disableButton(form.elements["triggerPBC"]);
		disableButton(form.elements["genPIN"]);
  	}
	else {
		enableTextField(form.elements["localPin"]);
		enableTextField(form.elements["peerPin"]);
	 	enableButton(form.elements["setPIN"]);
		enableButton(form.elements["genPIN"]);
		enableButton(form.elements["triggerPIN"]);		
		enableButton(form.elements["triggerPBC"]);
	}
	disableRadioGroup(form.elements["config"]);	 
	return true;
}

function saveChangesWPS(form)
{
	var wps_enc_status;
	var str = form.elements["localPin"].value;
	form.elements["localPin"].value = str.replace(/[(\s)-]/g, "");
	
	ret = check_pin_code(form.elements["localPin"].value);
	if (ret == 1) {
		alert('<% multilang(LANG_INVALID_PIN_LENGTH_THE_DEVICE_PIN_IS_USUALLY_FOUR_OR_EIGHT_DIGITS_LONG); %>');
		form.localPin.focus();
		return false;
	}
	else if (ret == 2) {
		alert('<% multilang(LANG_INVALID_PIN_THE_DEVICE_PIN_MUST_BE_NUMERIC_DIGITS); %>');
		form.localPin.focus();		
		return false;
	}
	else if (ret == 3) {
		alert('<% multilang(LANG_INVALID_PIN_CHECKSUM_ERRO); %>');
		form.localPin.focus();		
		return false;
	}  	

	if (wps20 && wpsUseVersion != 0)
	{
	    wps_enc_status = check_wps_enc(encrypt, enable1x, wpa_auth, hidden_ssid, wpa_cipher, wpa2_cipher, wps20);
	}
	else
	{
		wps_enc_status = check_wps_enc(encrypt, enable1x, wpa_auth, hidden_ssid, wpa_cipher, wpa2_cipher, wpsUseVersion);
	}
	if (wps_enc_status) {
		alert('<% multilang(LANG_WPS2_0_DOES_NOT_SUPPORT_THIS_ENCRYPTION_MODE); %>');
		return false;
	}
	form.save.isclick = 1;
	postTableEncrypt(form.postSecurityFlag, form);
   	return true;
}

function updatewpsUseVersionIndex()
{
	wpsUseVersion=document.formWsc.wpsUseVersion.selectedIndex;
}
</script>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_WI_FI_PROTECTED_SETUP); %></p>
	<p class="intro_content"><% multilang(LANG_PAGE_DESC_WPS_SETTING); %></p>
</div>

<form action=/boaform/formWsc method=POST name="formWsc">
<div class="data_common data_common_notitle">
	<table>
		<input type=hidden name="wlanDisabled" value=<% wlanStatus(); %>>
		<script>
			<% checkWrite("wlanMode"); %>  
			<% checkWrite("wpsVer"); %>
			<% checkWrite("wpsVerConfig"); %>
			if (mode == 0 || mode == 3)
			{
				if (wps20)
				{
					disable_all = check_wps_enc(encrypt, enable1x, wpa_auth, hidden_ssid, wpa_cipher, wpa2_cipher, wpsUseVersion);
				}
				else
				{
					disable_all = check_wps_enc(encrypt, enable1x, wpa_auth, hidden_ssid, wpa_cipher, wpa2_cipher, wps20);
				}
			} else {
				disable_all = check_wps_wlanmode(mode, is_adhoc);
			}
		</script>
		<tr>
			<th width="100%" colspan=3>
				<input type="checkbox" name="disableWPS" value="ON" <% checkWrite("wscDisable"); %> ONCLICK="checkWPSstate(document.formWsc)">&nbsp;&nbsp;<% multilang(LANG_DISABLE_WPS); %>
			</th>
		</tr>
		<script>
			if (isClient) {
				document.write("</table>\n");
				document.write("<span id = \"hide_div\" class = \"off\">\n");
				document.write("<table border=\"0\" width=500>\n");
			}
		</script>
		<tr>
			<th width="40%">WPS <% multilang(LANG_STATUS); %>:</th>  
			<td width="60%">
				<input type="radio" name="config" value="on" <% checkWrite("wscConfig-1"); %>><% multilang(LANG_CONFIGURED); %>&nbsp;&nbsp;
				<input type="radio" name="config" value="off" <% checkWrite("wscConfig-0"); %>><% multilang(LANG_UNCONFIGURED); %>
			</td>  
		</tr>
		<script>
			if (isClient) {
				document.write("</table>\n");
				document.write("</span>\n");
				document.write("<table border=\"0\" width=500>\n");
			}
		</script>
		<script>
			if (wps20 && wps_version_configurable)
				document.write("<tr>");
			else
				document.write("<tr style=\"display: none;\">");
		</script>
			<th width="40%">WPS <% multilang(LANG_VERSION); %>:</th>  
			<td width="60%">
				<select size="1" name="wpsUseVersion" onChange="updatewpsUseVersionIndex()"><% checkWrite("wpsUseVersion"); %></select>
				<SCRIPT>document.formWsc.wpsUseVersion.selectedIndex = wpsUseVersion;</SCRIPT>
			</td>  
		</tr>
	<!-- for brute force attack unlock --> 
		<tr <% checkWrite("wlan_qtn_hidden_function"); %>>
			<th width="40%"><% multilang(LANG_AUTO_LOCK_DOWN_STATE); %>:</th>
			<td width="60%">  	
			<script>
				if (autolockdown_stat) {
					document.write("<% multilang(LANG_LOCKED); %>");
				}else{
					document.write("<% multilang(LANG_UNLOCKED); %>");  
				}
			</script> 
			&nbsp;&nbsp;
	  		<input type="submit" value="<% multilang(LANG_UNLOCK); %>" name="unlockautolockdown" onClick="return unlockdownClicked(this)" class="inner_btn">
			</td>  
		</tr>
		<tr>
			<th width="40%"><% multilang(LANG_SELF_PIN_NUMBER); %>:</th>
			<td width="60%"><input type="text" name="localPin" size="12" maxlength="10" onkeypress="return noenter(event)" value=<% getInfo("wscLoocalPin"); %>>
				&nbsp;&nbsp;<input type="button" value="<% multilang(LANG_REGENERATE_PIN); %>" name="genPIN" onClick="return genPinClicked()" class="inner_btn">
			</td>
		</tr>

		<script>
		  if (!isClient) {
			document.write("</table>\n");
			document.write("<span id = \"hide_div\" class = \"off\">\n");
			document.write("<table>\n");
		  }
		</script>
		<tr>
			<th width="40%">PIN <% multilang(LANG_CONFIGURATION); %>:</th> 
			<td width="60%">
				<input type="submit" value="<% multilang(LANG_START_PIN); %>" name="triggerPIN" onClick="return triggerPINClicked(this)" class="inner_btn">
			</td>  
		</tr>
		<script>
		  if (!isClient) {
			document.write("</table>\n");
			document.write("</span>\n");
			document.write("<table border=\"0\" width=500>\n");
		  }
		</script>
		<tr>
			<th width="40%"><% multilang(LANG_PUSH_BUTTON); %> <% multilang(LANG_CONFIGURATION); %>:</th> 
			<td width="60%">
				<input type="submit" value="<% multilang(LANG_START_PBC); %>" name="triggerPBC" onClick="return triggerPBCClicked(this)" class="inner_btn"></td>
			</td>  
		</tr>
		<input type="hidden" value="/wlwps.asp" name="submit-url">
		<tr>
			<td width="100%" colspan="2">
				<input type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="save" onClick="return saveChangesWPS(document.formWsc)" class="inner_btn">&nbsp;&nbsp;
				<input type="reset" value="<% multilang(LANG_RESET); %>" name="reset" class="inner_btn">
			</td>
		</tr>
		<script>
			if (disable_all) {
				 document.write("<tr><td colspan=\"2\" height=\"55\"><font size=2><em>");
				if (disable_all == 1)     
					document.write(warn_msg1);
				else if(disable_all == 2)
					document.write(warn_msg2);
				else if(disable_all == 4)
					document.write(warn_msg4);
				else if(disable_all == 5)
					document.write(warn_msg5);
				else if(disable_all == 6)
					document.write(warn_msg6);   	
				document.write("</td></tr>"); 	   	
			}
		</script>   
	</table>
</div>
			
<script>
    <% checkWrite("wscConfig-A"); %>
	if (isClient || !isConfig)
		document.write("<span id = \"hide_div\" class = \"off\">\n");
</script>

<div class="column">
	<div class="column_title">
		<div class="column_title_left"></div>
			<p><% multilang(LANG_CURRENT_KEY_INFO); %></p>
		<div class="column_title_right"></div>
	</div>
	<div class="data_common data_vertical">
		<table>
			<tr>
				<th width="30%"><% multilang(LANG_AUTHENTICATION); %></th>
				<th width="20%"><% multilang(LANG_ENCRYPTION); %></th>
				<th colspan=2><% multilang(LANG_KEY); %></th>
			</tr>
			<tr>
				<td width="30%">
					<% checkWrite("wps_auth"); %>
				</td>
				<td width="20%">
					<% checkWrite("wps_enc"); %>
				</td>
				<td width="30%">
					<% getInfo("wps_key"); %>
				</td>
			</tr>
		</table>
	</div>
</div>

<script>
  if (isClient || !isConfig) {
	document.write("</span>\n");
  }      
  if (isClient)
	document.write("<span id = \"hide_div\" class = \"off\">\n");
</script>

<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th width="40%"><% multilang(LANG_CLIENT_PIN_NUMBER); %>:</th>
			<td width="60%"><input type="text" name="peerPin" size="12" maxlength="10" onkeypress="return noenter(event)" value="">&nbsp;&nbsp;
				<input type="submit" value="<% multilang(LANG_START_PIN); %>" name="setPIN" onClick="return setPinClicked(document.formWsc)" class="inner_btn">
				<input type="hidden" name="wlan_idx" value=<% checkWrite("wlan_idx"); %>>
				<input type="hidden" name="postSecurityFlag" value="">
			</td>
		</tr>
	</table>
</div>
<script>
	if (isClient)
		document.write("</span>\n");
	checkWPSstate(document.formWsc);
</script>
  
</form>
<br><br>
</body>

</html>
