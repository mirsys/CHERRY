<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_ATM_SETTINGS); %></title>
<link rel="stylesheet" type="text/css" href="common_style.css" />
<SCRIPT language="javascript" src="/common.js"></SCRIPT>
<script type="text/javascript" src="share.js">
</script>
<script>

function addClick(obj)
{
	var digit;
	
	if(document.formAtm.vpi.value.length == 0)
	{
		obj.isclick = 1;
		postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
		return true;
	}
	
	<% checkWrite("vcMax"); %>
	
	digit = getDigit(document.formAtm.vpi.value, 1);
	if ( validateKey(document.formAtm.vpi.value) == 0 ||
		(digit > 255 || digit < 0) ) {
		alert("<% multilang(LANG_INVALID_VPI_VALUE_YOU_SHOULD_SET_A_VALUE_BETWEEN_0_255); %>");
		document.formAtm.vpi.focus();
		return false;
	}
	
	digit = getDigit(document.formAtm.vci.value, 1);
	if ( validateKey(document.formAtm.vci.value) == 0 ||
		(digit > 65535 || digit < 0) ) {
		alert("<% multilang(LANG_INVALID_VCI_VALUE_YOU_SHOULD_SET_A_VALUE_BETWEEN_0_65535); %>");
		document.formAtm.vci.focus();
		return false;
	}
	
	digit = getDigit(document.formAtm.pcr.value, 1);
	if ( validateKey(document.formAtm.pcr.value) == 0 ||
		(digit > 6000 || digit < 1) ) {
		alert("Invalid PCR value! You should set a value between 1-6000.");
		document.formAtm.pcr.focus();
		return false;
	}
	
	digit = getDigit(document.formAtm.cdvt.value, 1);
	if ( validateKey(document.formAtm.cdvt.value) == 0 ||
		(digit > 4294967295 || digit < 0) ) {
		alert("Invalid CDVT value! You should set a value between 0-4294967295.");
		document.formAtm.cdvt.focus();
		return false;
	}
	
	if (( document.formAtm.qos.selectedIndex == 2 ) ||
		( document.formAtm.qos.selectedIndex == 3 )) {
	
		digit = getDigit(document.formAtm.scr.value, 1);
		if ( validateKey(document.formAtm.scr.value) == 0 ||
			(digit > 6000 || digit < 1) ) {
			alert("Invalid SCR value! You should set a value between 1-6000.");
			document.formAtm.scr.focus();
			return false;
		}
		
		digit = getDigit(document.formAtm.mbs.value, 1);
		if ( validateKey(document.formAtm.mbs.value) == 0 ||
			(digit > 65535 || digit < 0) ) {
			alert("Invalid MBS value! You should set a value between 0-65535.");
			document.formAtm.mbs.focus();
			return false;
		}
	}

	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

function resetClicked()
{
	document.formAtm.qos.selectedIndex = 0;
	disableTextField(document.formAtm.scr);
	disableTextField(document.formAtm.mbs);
}

function qosSelection()
{
	if (( document.formAtm.qos.selectedIndex == 2 ) ||
		( document.formAtm.qos.selectedIndex == 3 )) {
		enableTextField(document.formAtm.scr);
		enableTextField(document.formAtm.mbs);
	} else {
		disableTextField(document.formAtm.scr);
		disableTextField(document.formAtm.mbs);
	}
}

function clearAll()
{
	document.formAtm.vpi.value = "";
	document.formAtm.vci.value = "";
	document.formAtm.qos.selectedIndex = 0;
	document.formAtm.pcr.value = "";
	document.formAtm.cdvt.value = "";
	document.formAtm.scr.value = "";
	document.formAtm.mbs.value = "";
}

function postVC(vpi,vci,qos,pcr,cdvt,scr,mbs)
{
	clearAll();
	document.formAtm.vpi.value = vpi;
	document.formAtm.vci.value = vci;
	document.formAtm.qos.selectedIndex = qos;
	document.formAtm.pcr.value = pcr;
	document.formAtm.cdvt.value = cdvt;
	
	if (qos == 2 || qos == 3) {
		enableTextField(document.formAtm.scr);
		enableTextField(document.formAtm.mbs);
		document.formAtm.scr.value = scr;
		document.formAtm.mbs.value = mbs;
	}
	else {
		disableTextField(document.formAtm.scr);
		disableTextField(document.formAtm.mbs);
	}
}

</script>

</head>
<BODY>
<blockquote>
<h2 class="page_title"><% multilang(LANG_ATM_SETTINGS); %></h2>

<table>
<tr><td><font size=2>
    <% multilang(LANG_PAGE_DESC_ATM_SETTING); %>
</font></td></tr>
<tr><td><hr size=1 noshade align=top></td></tr>
</table>

<form action=/boaform/formWanAtm method=POST name="formAtm">
<table>
<tr><td class="table_item">
	<p>VPI:  <input type="text" name="vpi" size="5" maxlength="5" disabled>&nbsp;&nbsp;
	VCI:  <input type="text" name="vci" size="5" maxlength="5" disabled>&nbsp;&nbsp;
	QoS:  <select size="1" name="qos" onChange="qosSelection()">
		<option selected value="0">UBR</option>
		<option value="1">CBR</option>
		<option value="2">nrt-VBR</option>
		<option value="3">rt-VBR</option>
		</select>
	
	<p>PCR:  <input type="text" name="pcr" size="5" maxlength="5">&nbsp;&nbsp;
	CDVT:  <input type="text" name="cdvt" size="10" maxlength="10">&nbsp;&nbsp;
	SCR:  <input type="text" name="scr" size="5" maxlength="5">&nbsp;&nbsp;
	MBS:  <input type="text" name="mbs" size="5" maxlength="5"></font>
	
	<p><input type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="changeAtmVc" onClick="return addClick(this)">&nbsp;&nbsp;
		<input type="reset" value="<% multilang(LANG_UNDO); %>" name="reset" onClick="resetClicked()">
		<input type="hidden" value="/wanatm.asp" name="submit-url">
		<input type="hidden" name="postSecurityFlag" value="">
	</p>
</td><tr>
</table>
<br>
<table>
	<tr><font size=2><b><% multilang(LANG_CURRENT_ATM_VC_TABLE); %>:</b></font></tr>
	<% atmVcList(); %>
</table>
<SCRIPT>
	qosSelection();
</SCRIPT>

</form>
</blockquote>
</body>
</html>
