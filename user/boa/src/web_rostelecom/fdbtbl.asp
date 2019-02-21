<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_BRIDGE_FORWARDING_DATABASE); %></title>
<link rel="stylesheet" type="text/css" href="common_style.css" />
</head>

<body>
<blockquote>
<h2 class="page_title"><% multilang(LANG_BRIDGE_FORWARDING_DATABASE); %></h2>

<table>
  <tr><td><font size=2>
  <% multilang(LANG_PAGE_DESC_MAC_TABLE_INFO); %>
  </font></td></tr>
  <tr><td><hr size=1 noshade align=top></td></tr>
</table>


<form action=/boaform/admin/formRefleshFdbTbl method=POST name="formFdbTbl">
<table border='1'>
<tr bgcolor=#7f7f7f> <th class="table_item"><% multilang(LANG_PORT); %></th>
<th><% multilang(LANG_MAC_ADDRESS); %></th>
<th><% multilang(LANG_IS_LOCAL); %>?</th>
<th><% multilang(LANG_AGEING_TIMER); %></th></tr>
<% bridgeFdbList(); %>
</table>

<input type="hidden" value="/admin/fdbtbl.asp" name="submit-url">
  <p><input type="submit" value="<% multilang(LANG_REFRESH); %>" name="refresh">&nbsp;&nbsp;
  <input type="button" value="<% multilang(LANG_CLOSE); %>" name="close" onClick="javascript: window.close();"></p>
</form>
</blockquote>
</body>

</html>
