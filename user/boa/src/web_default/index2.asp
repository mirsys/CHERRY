<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_FIRMWARE_UPGRADE); %></title>
<link rel="stylesheet" type="text/css" href="common_style.css"/>
<script>
function sendClicked(obj)
{
	if (document.password.binary.value=="") {		
		alert('<% multilang(LANG_SELECTED_FILE_CANNOT_BE_EMPTY); %>');
		document.password.binary.focus();
		return false;
	}		
	
	return confirm('<% multilang(LANG_DO_YOU_REALLY_WANT_TO_UPGRADE_THE_FIRMWARE); %>');
}

</script>

</head>
<BODY>
<blockquote>
<h2 class="page_title"><% multilang(LANG_FIRMWARE_UPGRADE); %></h2>
<form action=/boaform/admin/formUpload method=POST enctype="multipart/form-data" name="password">
<table>
 <tr><td><font size=2>
 <% multilang(LANG_PAGE_DESC_UPGRADE_FIRMWARE); %>
 </font></td></tr>
 <tr><td><hr size=1 noshade align=top></td></tr>
</table>
<table>
  <tr>
      <td><font size=2><input type="file" value="<% multilang(LANG_CHOOSE_FILE); %>" name="binary" size=20></td>
  </tr>
  </table>
    <p> <input type="submit" value="<% multilang(LANG_UPGRADE); %>" name="send" onclick="return sendClicked()">&nbsp;&nbsp;
	<input type="reset" value="<% multilang(LANG_RESET); %>" name="reset">
    </p>
 </form>
 </blockquote>
</body>
</html>
