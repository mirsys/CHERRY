<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_LOGOUT); %></title>
<SCRIPT language="javascript" src="/common.js"></SCRIPT>
<SCRIPT>
function confirmlogout(obj)
{
   if ( !confirm('<% multilang(LANG_DO_YOU_CONFIRM_TO_LOGOUT); %>') ) {
	return false;
  }
  else {
 	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
  }
}
</SCRIPT>
</head>

<body>
<blockquote>
<h2><font color="#0000FF"><% multilang(LANG_LOGOUT); %></font></h2>

<form action=/boaform/admin/formLogout method=POST name="cmlogout">
<table border=0 width="500" cellspacing=4 cellpadding=0>
  <tr><td><font size=2>
    <% multilang(LANG_THIS_PAGE_IS_USED_TO_LOGOUT_FROM_THE_DEVICE); %>
  </font></td></tr>
  <tr><td><hr size=1 noshade align=top></td></tr>
</table>
  <br>
      <input type="submit" value="<% multilang(LANG_LOGOUT); %>" name="save" onclick="return confirmlogout(this)">&nbsp;&nbsp;
      <input type="hidden" value="/admin/logout.asp" name="submit-url">
      <input type="hidden" name="postSecurityFlag" value="">
 </form>
</blockquote>
</body>

</html>

