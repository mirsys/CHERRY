<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_REMOTE_ACCESS); %><% multilang(LANG_CONFIGURATION); %></title>
<link rel="stylesheet" type="text/css" href="common_style.css" />
<script type="text/javascript" src="share.js">
</script>

<script>
function addClick()
{
	dTelnet = getDigit(document.acc.w_telnet_port.value, 1);
	dFtp = getDigit(document.acc.w_ftp_port.value, 1);
	dWeb = getDigit(document.acc.w_web_port.value, 1);
	if (dTelnet == dFtp || dTelnet == dWeb) {
		alert("<% multilang(LANG_DUPLICATED_PORT_NUMBER); %>");
		document.acc.w_telnet_port.focus();
		return false;
	}
	if (dFtp == dWeb) {
		alert("<% multilang(LANG_DUPLICATED_PORT_NUMBER); %>");
		document.acc.w_ftp_port.focus();
		return false;
	}

	if (document.acc.w_telnet.checked) {
		if (document.acc.w_telnet_port.value=="") {
			alert("<% multilang(LANG_PORT_RANGE_CANNOT_BE_EMPTY_YOU_SHOULD_SET_A_VALUE_BETWEEN_1_65535); %>");
			document.acc.w_telnet_port.focus();
			return false;
		}
		if ( validateKey( document.acc.w_telnet_port.value ) == 0 ) {
			alert("<% multilang(LANG_INVALID_PORT_NUMBER_IT_SHOULD_BE_THE_DECIMAL_NUMBER_0_9); %>");
			document.acc.w_telnet_port.focus();
			return false;
		}
		//d1 = getDigit(document.acc.w_telnet_port.value, 1);
		//if (d1 > 65535 || d1 < 1) {
		if (dTelnet > 65535 || dTelnet < 1) {
			alert("<% multilang(LANG_INVALID_PORT_NUMBER_YOU_SHOULD_SET_A_VALUE_BETWEEN_1_65535); %>");
			document.acc.w_telnet_port.focus();
			return false;
		}
  }

	if (document.acc.w_ftp.checked) {
		if (document.acc.w_ftp_port.value=="") {
			alert("<% multilang(LANG_PORT_RANGE_CANNOT_BE_EMPTY_YOU_SHOULD_SET_A_VALUE_BETWEEN_1_65535); %>");
			document.acc.w_ftp_port.focus();
			return false;
		}
		if ( validateKey( document.acc.w_ftp_port.value ) == 0 ) {
			alert("<% multilang(LANG_INVALID_PORT_NUMBER_IT_SHOULD_BE_THE_DECIMAL_NUMBER_0_9); %>");
			document.acc.w_ftp_port.focus();
			return false;
		}
		//d1 = getDigit(document.acc.w_ftp_port.value, 1);
		//if (d1 > 65535 || d1 < 1) {
		if (dFtp > 65535 || dFtp < 1) {
			alert("<% multilang(LANG_INVALID_PORT_NUMBER_YOU_SHOULD_SET_A_VALUE_BETWEEN_1_65535); %>");
			document.acc.w_ftp_port.focus();
			return false;
		}
	}

	if (document.acc.w_web.checked) {
		if (document.acc.w_web_port.value=="") {
			alert("<% multilang(LANG_PORT_RANGE_CANNOT_BE_EMPTY_YOU_SHOULD_SET_A_VALUE_BETWEEN_1_65535); %>");
			document.acc.w_web_port.focus();
			return false;
		}
		if ( validateKey( document.acc.w_web_port.value ) == 0 ) {
			alert("<% multilang(LANG_INVALID_PORT_NUMBER_IT_SHOULD_BE_THE_DECIMAL_NUMBER_0_9); %>");
			document.acc.w_web_port.focus();
			return false;
		}
		//d1 = getDigit(document.acc.w_web_port.value, 1);
		//if (d1 > 65535 || d1 < 1) {
		if (dWeb > 65535 || dWeb < 1) {
			alert("<% multilang(LANG_INVALID_PORT_NUMBER_YOU_SHOULD_SET_A_VALUE_BETWEEN_1_65535); %>");
			document.acc.w_web_port.focus();
			return false;
		}
	}

	return true;

}

</script>
</head>

<body>
<blockquote>
<h2 class="page_title"><% multilang(LANG_REMOTE_ACCESS); %><% multilang(LANG_CONFIGURATION); %></h2>

<table>
<tr><td colspan=4><font size=2>
 <% multilang(LANG_THIS_PAGE_IS_USED_TO_ENABLE_DISABLE_MANAGEMENT_SERVICES_FOR_THE_LAN_AND_WAN); %>
</font></td></tr>

<tr><td><hr size=1 noshade align=top></td></tr>
</table>
<form action=/boaform/admin/formSAC method=POST name=acc>
<table>
<tr>
	<td width=150 align=left class="table_item"><% multilang(LANG_SERVICE); %><% multilang(LANG_NAME); %></td>
	<td width=80 align=center class="table_item"><% multilang(LANG_LAN); %></td>
	<td width=80 align=center class="table_item"><% multilang(LANG_WAN); %></td>
	<td width=80 align=center class="table_item"><% multilang(LANG_WAN_PORT); %></td>
</tr>
<% rmtaccItem(); %>
</table>
<br>
<tr>
	<td><input type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="set" onClick="return addClick()"></td>
	<td><input type="hidden" value="/admin/rmtacc.asp" name="submit-url"></td>
</tr>
<script>
	<% accPost(); %>
</script>
</form>
</blockquote>
</body>
</html>
