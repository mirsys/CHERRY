<html>
<! Copyright (c) Realtek Semiconductor Corp., 2007. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_FAST_BSS_TRANSITION_802_11R); %></title>
<link rel="stylesheet" type="text/css" href="common_style.css" />
<SCRIPT language="javascript" src="/common.js"></SCRIPT>
<script type="text/javascript" src="share.js"></script>

<script>
var ssid_num;
var _dotIEEE80211r=new Array();
var _encrypt=new Array();
var _ft_enable=new Array();
var _ft_mdid=new Array();
var _ft_over_ds=new Array();
var _ft_res_request=new Array();
var _ft_r0key_timeout=new Array();
var _ft_reasoc_timeout=new Array();
var _ft_r0kh_id=new Array();
var _ft_push=new Array();
var _ft_kh_num=new Array();

function SSIDSelected(index, form)
{
	if (ssid_num == 0)
		return;

	document.formFt.ftSSID.value = form.ftSSID.value;
	document.formFtKhAdd.ftSSID.value = form.ftSSID.value;
	document.formFtKhDel.ftSSID.value = form.ftSSID.value;

	document.formFt.security_method.value = _encrypt[index];
	if ((_encrypt[index]==4) || (_encrypt[index]==6)) { //WPA2,WPA2-MIXED
		_dotIEEE80211r[index] = true;
		enableRadioGroup(document.formFt.ft_enable);
	} else { //NONE,WEP
		_dotIEEE80211r[index] = false;
		disableRadioGroup(document.formFt.ft_enable);
	}

	enable_80211r_setting(index);
	load_80211r_setting(index);
}

function enable_80211r_setting(index)
{
	if ((_dotIEEE80211r[index]==false) || (document.formFt.ft_enable[0].checked==true)) {
		document.formFt.ft_mdid.disabled = true;
		disableRadioGroup(document.formFt.ft_over_ds);
		disableRadioGroup(document.formFt.ft_res_request);
		document.formFt.ft_r0key_timeout.disabled = true;
		document.formFt.ft_reasoc_timeout.disabled = true;
		document.formFt.ft_r0kh_id.disabled = true;
		disableRadioGroup(document.formFt.ft_push);
	} else {
		document.formFt.ft_mdid.disabled = false;
		enableRadioGroup(document.formFt.ft_over_ds);
		enableRadioGroup(document.formFt.ft_res_request);
		document.formFt.ft_r0key_timeout.disabled = false;
		document.formFt.ft_reasoc_timeout.disabled = false;
		document.formFt.ft_r0kh_id.disabled = false;
		enableRadioGroup(document.formFt.ft_push);
	}
}

function load_80211r_setting(index)
{
	document.formFt.ft_enable[_ft_enable[index]].checked = true;
	document.formFt.ft_mdid.value = _ft_mdid[index];
	document.formFt.ft_over_ds[_ft_over_ds[index]].checked = true;
	document.formFt.ft_res_request[_ft_res_request[index]].checked = true;
	document.formFt.ft_r0key_timeout.value = _ft_r0key_timeout[index];
	document.formFt.ft_reasoc_timeout.value = _ft_reasoc_timeout[index];
	document.formFt.ft_r0kh_id.value = _ft_r0kh_id[index];
	document.formFt.ft_push[_ft_push[index]].checked = true;
	enable_80211r_setting(index);
}

function check_80211r_setting()
{
	var i, len, val;
	
	//check MDID
	len = document.formFt.ft_mdid.value.length;
	if (len != 4) {
		alert("<% multilang(LANG_INVALID_MDID_2_OCTET_IDENTIFIER_AS_A_HEX_STRING); %>");
		document.formFt.ft_mdid.focus();
		return false;
	}
	for (i=0; i<len; i++) {
		if (is_hex(document.formFt.ft_mdid.value.charAt(i)) == false) {
			alert("<% multilang(LANG_INVALID_HEX_NUMBER); %>");
			document.formFt.ft_mdid.focus();
			return false;
		}
	}
	
	//check Key expiration timeout
	val = parseInt(document.formFt.ft_r0key_timeout.value);
	if (val>65535)
	{
		alert("<% multilang(LANG_INVALID_RANGE_OF_KEY_EXPIRATION_TIMEOUT_0_OR_1_65535_MINUTES); %>");
		document.formFt.ft_r0key_timeout.focus();
		return false;
	}
	
	// check Reassociation timeout
	val = parseInt(document.formFt.ft_reasoc_timeout.value);
	if ((val!=0) && (val<1000 || val>65535))
	{
		alert("<% multilang(LANG_INVALID_RANGE_OF_REASSOCIATION_TIMEOUT_0_OR_1000_65535); %>");
		document.formFt.ft_reasoc_timeout.focus();
		return false;
	}
	
	// check NAS identifier
	len = document.formFt.ft_r0kh_id.value.length;
	if (len<1 || len>48)
	{
		alert("<% multilang(LANG_INVALID_NAS_IDENTIFIER_1_48_CHARACTERS); %>");
		document.formFt.ft_r0kh_id.focus();
		return false;
	}
	document.formFt.ftSaveConfig.isclick = 1;
	postTableEncrypt(document.formFt.postSecurityFlag, document.formFt);
	return true;
}

function is_hex(ch)
{
	if ((ch>='0' && ch<='9') || (ch>='a' && ch<='f') || (ch>='A' && ch<='F'))
		return true;
	else
		return false;
}

function check_kh_setting(index)
{
	var len;
	
	// check if exceed max number of KH entry
	if (_ft_kh_num[index] >= <% checkWrite("11r_ftkh_num") %>) {
		alert("<% multilang(LANG_EXCEED_MAX_NUMBER_OF_KH_ENTRY_FOR_SELECTED_SSID); %>");
		return false;
	}
	
	// check MAC address
	if (!checkMac(document.formFtKhAdd.kh_mac, 1))
		return false;

	// check NAS ID (1~48 characters)
	len = document.formFtKhAdd.kh_nas_id.value.length;
	if (len<1 || len>48)
	{
		alert("<% multilang(LANG_INVALID_NAS_IDENTIFIER_1_48_CHARACTERS); %>");
		document.formFtKhAdd.kh_nas_id.focus();
		return false;
	}
	for (i=0; i<len; i++) {
		if (document.formFtKhAdd.kh_nas_id.value.charAt(i) == ' ') {
			alert("<% multilang(LANG_WHITE_SPACE_IS_NOT_ALLOWED_IN_NAS_IDENTIFIER_FIELD_); %>");
			document.formFtKhAdd.kh_nas_id.focus();
			return false;
		}
	}

	// check AES wrap/unwrap key (16 octets)
	len = document.formFtKhAdd.kh_kek.value.length;
	if (len<1 || len>32)
	{
		alert("<% multilang(LANG_INVALID_KEY_LENGTH_SHOULD_BE_16_OCTETS_OR_1_32_CHARACTERS); %>");
		document.formFtKhAdd.kh_nas_id.focus();
		return false;
	}
	document.formFtKhAdd.ftAddKH.isclick = 1;
	postTableEncrypt(document.formFtKhAdd.postSecurityFlag, document.formFtKhAdd);
	return true;
}

function deleteClick(obj)
{
	if ( !confirm('<% multilang(LANG_CONFIRM_DELETE_ONE_ENTRY); %>') ) {
		return false;
	}
	else{
		obj.isclick = 1;
		postTableEncrypt(document.formFtKhDel.postSecurityFlag, document.formFtKhDel);
		return true;
	}
}
        
function deleteAllClick(obj)
{
	if ( !confirm('Do you really want to delete the all entries?') ) {
		return false;
	}
	else{
		obj.isclick = 1;
		postTableEncrypt(document.formFtKhDel.postSecurityFlag, document.formFtKhDel);
		return true;
	}
}
</script>
</head>

<body onload="SSIDSelected(0, formFt);">
<blockquote>
<h2 class="page_title"><% multilang(LANG_FAST_BSS_TRANSITION_802_11R); %></h2>

<!-- Page description -->
<table>
	<tr>
		<td><font size=2>
		<% multilang(LANG_THIS_PAGE_ALLOWS_YOU_TO_CHANGE_THE_SETTING_FOR_FAST_BSS_TRANSITION_802_11R); %>
		</font>
		</td>
	</tr>
	<tr><td><hr size=1 noshade align=top></td></tr>
</table>

<div id="wlan_dot11r_table" style="display:none">
<!-- 802.11r driver configuration -->
<form action=/boaform/formFt method=POST name="formFt">
<!-- select SSID -->
<table>
		<tr>
			<th><% multilang(LANG_SSID); %> <% multilang(LANG_TYPE); %>:</th>
			<td>
				<select name=ftSSID onChange="SSIDSelected( this.selectedIndex, this.form )"><% SSID_select(); %></select>
			</td>
		</tr>
		<tr>
			<th><% multilang(LANG_ENCRYPTION); %>:</th>
			<td>
				<select size="1" id="security_method" name="security_method" disabled>
					<option value=0>None</option>
					<option value=1>WEP</option>
					<option value=4>WPA2</option>
					<option value=6>WPA2 Mixed</option>
				</select>
			</td>
		</tr>
</table>

<!-- ----------------------------------------------------- -->
<table>
	<tr><td><hr size=1 noshade align=top></td></tr>
</table>

<!-- mib settings -->
<table>
	<!-- Enable/Disable Fast BSS Transition -->
	<tr id="show_ft_enable">
		<th class="bgblue">
			<% multilang(LANG_IEEE_802_11R); %>:
		</th>
		<td class="bggrey">
			<input type="radio" name="ft_enable" value=0 onClick="enable_80211r_setting()" >Disable
			<input type="radio" name="ft_enable" value=1 onClick="enable_80211r_setting()" >Enable
		</td>
	</tr>

	<!-- Mobility Domain ID -->
	<tr id="show_ft_mdid">
		<th bgcolor="#FFFFFF" class="bgblue">
			<% multilang(LANG_MOBILITY_DOMAIN_ID); %>:
		</th>
		<td class="bggrey">
			<input type="text" name="ft_mdid" size="4" maxlength="4" value="A1B2">
		</td>
	</tr>

	<!-- Support over DS -->
	<tr id="show_ft_over_ds">
		<th class="bgblue">
			<% multilang(LANG_SUPPORT_OVER_DS); %>:
		</th>
		<td class="bggrey">
			<input type="radio" name="ft_over_ds" value=0 >Disable
			<input type="radio" name="ft_over_ds" value=1 >Enable
		</td>
	</tr>

	<!-- Support resource request -->
	<tr id="show_ft_res_request" style="display:none">
		<th class="bgblue">
			<% multilang(LANG_SUPPORT_RESOURCE_REQUEST); %>:
		</th>
		<td class="bggrey">
			<input type="radio" name="ft_res_request" value=0 >Disable
			<input type="radio" name="ft_res_request" value=1 >Enable
		</td>
	</tr>

	<!-- Key expiration timeout -->
	<tr id="show_ft_r0key_timeout">
		<th bgcolor="#FFFFFF" class="bgblue">
			<% multilang(LANG_KEY_EXPIRATION_TIMEOUT); %>:
		</th>
		<td class="bggrey">
			<input type="text" name="ft_r0key_timeout" size="12" maxlength="10" value="10000">
			<font size="1">&nbsp;(1..65535 minutes, 0:disable)</font>
		</td>
	</tr>

	<!-- Reassociation timeout -->
	<tr id="show_ft_reasoc_timeout">
		<th bgcolor="#FFFFFF" class="bgblue">
			<% multilang(LANG_REASSOCIATION_TIMEOUT); %>:
		</th>
		<td class="bggrey">
			<input type="text" name="ft_reasoc_timeout" size="12" maxlength="10" value="1000">
			<font size="1">&nbsp;(1000..65535 seconds, 0:disable)</font>
		</td>
	</tr>

	<!-- NAS identifier (R0KH-ID) -->
	<tr id="show_ft_r0kh_id">
		<th bgcolor="#FFFFFF" class="bgblue">
			<% multilang(LANG_NAS_IDENTIFIER); %>:
		</th>
		<td class="bggrey">
			<input type="text" name="ft_r0kh_id" size="30" maxlength="48" value="www.realtek.com.tw">
			<font size="1">&nbsp;(1~48 characters)</font>
		</td>
	</tr>

	<!-- Enable Key-Push, this is for FT-Daemon -->
	<tr id="show_ft_push">
		<th class="bgblue">
			<% multilang(LANG_SUPPORT_KEY_PUSH); %>:
		</th>
		<td class="bggrey">
			<input type="radio" name="ft_push" value=0 >Disable
			<input type="radio" name="ft_push" value=1 >Enable
		</td>
	</tr>

	<tr>
		<td colspan="2" height="40">
			<input type="hidden" name="wlan_idx" value=<% checkWrite("wlan_idx"); %>>
			<input type="hidden" name="submit-url" value="/wlft.asp">
			<input type="submit" name="ftSaveConfig" value="<% multilang(LANG_APPLY_CHANGES); %>" onclick="return check_80211r_setting()">
			<input type="hidden" name="postSecurityFlag" value="">
		</td>
	</tr>
</table>
</form>

<!-- ----------------------------------------------------- -->
<table>
	<tr><td><hr size=1 noshade align=top></td></tr>
</table>

<!-- Add R0KH/R1KH entry -->
<form action=/boaform/formFt method=POST name="formFtKhAdd">
<table>
	<tr>
		<th colspan="2"><% multilang(LANG_KEY_HOLDER_CONFIGURATION); %>:</th>
	</tr>

	<tr>
		<th><% multilang(LANG_SSID); %> <% multilang(LANG_TYPE); %>:</th>
		<td>
			<select name=ftSSID onChange="SSIDSelected( this.selectedIndex, this.form )"><% SSID_select(); %></select>
		</td>
	</tr>
</table>

<table>
	<tr>
		<th><% multilang(LANG_MAC_ADDRESS); %>:</th>
		<td><input type="text" name="kh_mac" size="10" maxlength="12" value="">
			<font size=1>&nbsp(ex: 00E086710502)</font></td>
	</tr>

	<tr>
		<th><% multilang(LANG_NAS_IDENTIFIER); %>:</th>
		<td nowrap><input type="text" name="kh_nas_id" size="34" maxlength="48" value="">
			<font size=1>&nbsp(1~48 characters)</font></td>
	</tr>

	<tr>
		<th><% multilang(LANG_ENCRYPTION_KEY); %>:</th>
		<td nowrap><input type="text" name="kh_kek" size="34" maxlength="32" value="">
			<font size=1>&nbsp;(16 octets or passphrase)</font></td>
	</tr>

	<tr>
		<td colspan="2" height="40">
		<input type="hidden" name="submit-url" value="/admin/wlft.asp">
		<input type="hidden" name="wlan_idx" value=<% checkWrite("wlan_idx"); %>>
		<input type="submit" name="ftAddKH" value="<% multilang(LANG_ADD); %>"
			onClick="return check_kh_setting(ftSSID.selectedIndex)">&nbsp;&nbsp;
		<input type="reset" name="reset" value="<% multilang(LANG_RESET); %>">&nbsp;&nbsp;
		<input type="hidden" name="postSecurityFlag" value="">
		</td>
	</tr>
</table>
</form>

<!-- ----------------------------------------------------- -->
<table>
	<tr><td><hr size=1 noshade align=top></td></tr>
</table>

<!-- Delete R0KH/R1KH entry -->
<form action=/boaform/formFt method=POST name="formFtKhDel">
<table border=0 width="700">
	<tr>
		<th colspan="4"><% multilang(LANG_CURRENT_KEY_HOLDER_INFORMATION); %>:</th>
	</tr>
	<% wlFtKhList(); %>

	<tr>
		<tdcolspan="4" height="40">
		<input type="hidden" name="submit-url" value="/admin/wlft.asp">
		<input type="hidden" name="wlan_idx" value=<% checkWrite("wlan_idx"); %>>
		<input type="hidden" name=ftSSID>
		<input type="submit" name="ftDelSelKh" value="<% multilang(LANG_DELETE_SELECTED); %>"
			onClick="return deleteClick(this)">&nbsp;&nbsp;
		<input type="submit" name="ftDelAllKh" value="<% multilang(LANG_DELETE_ALL); %>"
			onClick="return deleteAllClick(this)">&nbsp;&nbsp;
		<input type="hidden" name="postSecurityFlag" value="">
		</td>
	</tr>
</table>
</form>
</div>

<script>
<% initPage("wlft"); %>
</script>
</blockquote>
</body>
</html>
