<html>
<! Copyright (c) Realtek Semiconductor Corp., 2008. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_IP_PRECEDENCE_PRIORITY_SETTINGS); %></title>
<link rel="stylesheet" type="text/css" href="common_style.css" />
<script type="text/javascript" src="share.js">
</script>
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
<BODY>
<blockquote>
<h2 class="page_title"><% multilang(LANG_IP_PRECEDENCE_PRIORITY_SETTINGS); %></h2>

<table>
<tr><td><font size=2>
<% multilang(LANG_THIS_PAGE_IS_USED_TO_CONFIG_IP_PRECEDENCE_PRIORITY); %>
</font></td></tr>
<tr><td><hr size=1 noshade align=top></td></tr>
</table>
<form action=/boaform/formIPQoS method=POST name=qos_set1p>
<table border="0" width=500>
<tr><font size=2><% multilang(LANG_IP_PRECEDENCE_RULE); %>:</font></tr>
<% settingpred(); %>
</table>	
<input type="hidden" value="/qos_pred.asp" name="submit-url">
<td><input type="submit" value="<% multilang(LANG_MODIFY); %>" name=setpred onClick="return on_submit(this)"></td>
<input type="button" value="<% multilang(LANG_CLOSE); %>" name="close" onClick="javascript: window.close();"></p>
<input type="hidden" name="postSecurityFlag" value="">
</form>
</blockquote>
</body>
</html>

