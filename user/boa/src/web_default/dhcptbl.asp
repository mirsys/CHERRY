<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_ACTIVE_DHCP_CLIENTS); %></title>
<link rel="stylesheet" type="text/css" href="common_style.css" />
<SCRIPT language="javascript" src="/common.js"></SCRIPT>
<SCRIPT>
function on_submit(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}
</SCRIPT>
</head>

<body>
<blockquote>
<h2 class="page_title"><% multilang(LANG_ACTIVE_DHCP_CLIENTS); %></h2>

<table>
  <tr><td><font size=2>
  <% multilang(LANG_THIS_TABLE_SHOWS_THE_ASSIGNED_IP_ADDRESS_MAC_ADDRESS_AND_TIME_EXPIRED_FOR_EACH_DHCP_LEASED_CLIENT); %>
  </font></td></tr>
  <tr><td><hr size=1 noshade align=top></td></tr>
</table>


<form action=/boaform/formReflashClientTbl method=POST name="formClientTbl">
<table border='1'>
<tr bgcolor=#7f7f7f> <th><% multilang(LANG_IP_ADDRESS); %></th>
<th><% multilang(LANG_MAC_ADDRESS); %></th>
<th><% multilang(LANG_EXPIRED_TIME_SEC); %></th></tr>
<% dhcpClientList(); %>
</table>

<input type="hidden" value="/dhcptbl.asp" name="submit-url">
  <p><input type="submit" value="<% multilang(LANG_REFRESH); %>" name="refresh" onClick="return on_submit(this)">&nbsp;&nbsp;
  <input type="button" value="<% multilang(LANG_CLOSE); %>" name="close" onClick="javascript: window.close();">
  <input type="hidden" name="postSecurityFlag" value="">
  </p>
</form>
</blockquote>
</body>

</html>
