<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title>DMZ <% multilang(LANG_CONFIGURATION); %></title>
<link rel="stylesheet" type="text/css" href="common_style.css" />
<SCRIPT language="javascript" src="common.js"></SCRIPT>
<script type="text/javascript" src="share.js">
</script>
<script>
function skip () { this.blur(); }
function saveClick(obj)
{
//  if (!document.formDMZ.enabled.checked)
  if (document.formDMZ.dmzcap[0].checked){
  	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
 	return true;      
  	}
/*  if ( validateKey( document.formDMZ.ip.value ) == 0 ) {
	alert("Invalid IP address value. It should be the decimal number (0-9).");
	document.formDMZ.ip.focus();
	return false;
  }
  if( IsLoopBackIP( document.formDMZ.ip.value)==1 ) {
	alert("Invalid IP address value.");
	document.formDMZ.ip.focus();
	return false;
  }
  if ( !checkDigitRange(document.formDMZ.ip.value,1,0,223) ) {
      	alert('Invalid IP address range in 1st digit. It should be 0-223.');
	document.formDMZ.ip.focus();
	return false;
  }
  if ( !checkDigitRange(document.formDMZ.ip.value,2,0,255) ) {
      	alert('Invalid IP address range in 2nd digit. It should be 0-255.');
	document.formDMZ.ip.focus();
	return false;
  }
  if ( !checkDigitRange(document.formDMZ.ip.value,3,0,255) ) {
      	alert('Invalid IP address range in 3rd digit. It should be 0-255.');
	document.formDMZ.ip.focus();
	return false;
  }
  if ( !checkDigitRange(document.formDMZ.ip.value,4,1,254) ) {
      	alert('Invalid IP address range in 4th digit. It should be 1-254.');
	document.formDMZ.ip.focus();
	return false;
  }*/
  if (!checkHostIP(document.formDMZ.ip, 1))
	return false;

  obj.isclick = 1;
  postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);

  return true;
}

function updateState()
{
//  if (document.formDMZ.enabled.checked) {
  if (document.formDMZ.dmzcap[1].checked) {
 	enableTextField(document.formDMZ.ip);
  }
  else {
 	disableTextField(document.formDMZ.ip);
  }
}


</script>
</head>

<body>
<blockquote>
<h2 class="page_title">DMZ <% multilang(LANG_CONFIGURATION); %></h2>

<table>
	<tr><td><font size=2>
	<% multilang(LANG_A_DEMILITARIZED_ZONE_IS_USED_TO_PROVIDE_INTERNET_SERVICES_WITHOUT_SACRIFICING_UNAUTHORIZED_ACCESS_TO_ITS_LOCAL_PRIVATE_NETWORK_TYPICALLY_THE_DMZ_HOST_CONTAINS_DEVICES_ACCESSIBLE_TO_INTERNET_TRAFFIC_SUCH_AS_WEB_HTTP_SERVERS_FTP_SERVERS_SMTP_E_MAIL_SERVERS_AND_DNS_SERVERS); %>
	</font></td></tr>
	<tr><td><hr size=1 noshade align=top></td></tr>
</table>

<form action=/boaform/formDMZ method=POST name="formDMZ">
	<table>
	<tr><th><% multilang(LANG_DMZ_HOST); %>:</th>
	      <td><font size=2>
		<input type="radio" value="0" name="dmzcap" <% checkWrite("dmz-cap0"); %> onClick="updateState()"><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
		<input type="radio" value="1" name="dmzcap" <% checkWrite("dmz-cap1"); %> onClick="updateState()"><% multilang(LANG_ENABLE); %>&nbsp;&nbsp;
	      </font></td>
	</td></tr>
	<tr>
		<th><% multilang(LANG_DMZ_HOST); %> <% multilang(LANG_IP_ADDRESS); %>: </th>
		<td><input type="text" name="ip" size="15" maxlength="15" value=<% getInfo("dmzHost"); %> ></td>
	</tr>
	</table>
	<br>
	<input type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="save" onClick="return saveClick(this)">
	<input type="hidden" value="/fw-dmz.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
	<script> updateState(); </script>
</form>
</blockquote>
</body>
</html>
