<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_INTERFACE_STATISITCS); %></title>
<link rel="stylesheet" type="text/css" href="common_style.css"/>
<script>
function resetClick() {
	with ( document.forms[0] ) {
		reset.value = 1;
		submit();
	}
}
</script>
</head>
<body>
<blockquote>
<h2 class="page_title"><% multilang(LANG_INTERFACE_STATISITCS); %></h2>

<table>
  <tr><td><font size=2>
 <% multilang(LANG_PAGE_DESC_PACKET_STATISTICS_INFO); %>
  </font></td></tr>
  <tr><td><hr size=1 noshade align=top></td></tr>
</table>
<form action=/boaform/admin/formStats method=POST name="formStats">
<table style="border:2px solid;">
	<% pktStatsList(); %>
</table>
  <br>
  <br><br>
  <input type="hidden" value="/admin/stats.asp" name="submit-url">
  <input type="submit" value="<% multilang(LANG_REFRESH); %>" name="refresh">
  <input type="hidden" value="0" name="reset">
  <input type="button" onClick="resetClick()" value="<% multilang(LANG_RESET_STATISTICS); %>">
</form>
</blockquote>
</body>

</html>
