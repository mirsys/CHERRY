<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_ADSL_CONNECTION_DIAGNOSTICS); %></title>
<link rel="stylesheet" type="text/css" href="common_style.css" />
</head>
<script type="text/javascript" src="share.js">
</script>
<script>
var initInf;

function itfSelected()
{
	initInf = document.diagtest.wan_if.value;
}
</script>

<body>
<blockquote>
<h2 class="page_title"><% multilang(LANG_ADSL_CONNECTION_DIAGNOSTICS); %></h2>

<form action=/boaform/formDiagTest method=POST name=diagtest>
<table>
	<tr><td><font size=2>
	  <% multilang(LANG_THE_DEVICE_IS_CAPABLE_OF_TESTING_YOUR_CONNECTION_THE_INDIVIDUAL_TESTS_ARE_LISTED_BELOW_IF_A_TEST_DISPLAYS_A_FAIL_STATUS_CLICK_GO_BUTTON_AGAIN_TO_MAKE_SURE_THE_FAIL_STATUS_IS_CONSISTENT); %>
	</font></td></tr>
	<tr><td><hr size=1 noshade align=top></td></tr>
</table>

<table>
  <tr>
    <td><font size=2><% multilang(LANG_SELECT_THE_ADSL_CONNECTION); %>: 
		<select name="wan_if"  onChange="itfSelected()">
		<% if_wan_list("adsl"); %>
		</select>
    </td>
    <td><input type=submit value="<% multilang(LANG_GO); %>" name="start"></td>
  </tr>
</table>
<p>
<!-- Nic and switch are always linked!
<table width=400 border=0>
	<% lanTest(); %>
</table>
-->
<p>
<table>
	<% adslTest(); %>
</table>
<p>
<table>
	<% internetTest(); %>
</table>
  <br>
<input type=hidden value="/diag-test.asp" name="submit-url">
</form>
<script>
	<% initPage("diagTest"); %>
</script>
</blockquote>
</body>

</html>
