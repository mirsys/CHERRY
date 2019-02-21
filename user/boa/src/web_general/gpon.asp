<% SendWebHeadStr();%>
<title><% multilang(LANG_GPON_SETTINGS); %></title>

<script>
function applyclick(obj)
{
	/* LOID */
	if (document.formgponconf.fmgpon_loid.value=="") {		
		alert('<% multilang(LANG_LOID_CANNOT_BE_EMPTY); %>');
		document.formgponconf.fmgpon_loid.focus();
		return false;
	}
	/* LOID Password */
	if (document.formgponconf.fmgpon_loid_password.value=="") {		
		alert('<% multilang(LANG_LOID_PASSWORD_CANNOT_BE_EMPTY); %>');
		document.formgponconf.fmgpon_loid_password.focus();
		return false;
	}
	if (includeSpace(document.formgponconf.fmgpon_loid_password.value)) {		
		alert('<% multilang(LANG_CANNOT_ACCEPT_SPACE_CHARACTER_IN_LOID_PASSWORD); %>');
		document.formgponconf.fmgpon_loid_password.focus();
		return false;
	}
	if (checkString(document.formgponconf.fmgpon_loid_password.value) == 0) {		
		alert('<% multilang(LANG_INVALID_LOID_PASSWORD); %>');
		document.formgponconf.fmgpon_loid_password.focus();
		return false;
	}
	/* PLOAM Password */
	if (document.formgponconf.fmgpon_ploam_password.value=="") {		
		alert('<% multilang(LANG_PLOAM_PASSWORD_CANNOT_BE_EMPTY); %>');
		document.formgponconf.fmgpon_ploam_password.focus();
		return false;
	}
	if (includeSpace(document.formgponconf.fmgpon_ploam_password.value)) {		
		alert('<% multilang(LANG_CANNOT_ACCEPT_SPACE_CHARACTER_IN_PLOAM_PASSWORD); %>');
		document.formgponconf.fmgpon_ploam_password.focus();
		return false;
	}
	if (checkString(document.formgponconf.fmgpon_ploam_password.value) == 0) {		
		alert('<% multilang(LANG_INVALID_PLOAM_PASSWORD); %>');
		document.formgponconf.fmgpon_ploam_password.focus();
		return false;
	}
	if( document.formgponconf.fmgpon_ploam_password.value.length>10 )
	{		
		alert('<% multilang(LANG_PLOAM_PASSWORD_SHOULD_BE_10_CHARACTERS); %>');
		document.formgponconf.fmgpon_ploam_password.focus();
		return false;
	}
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}
</script>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_GPON_SETTINGS); %></p>
	<p class="intro_content"><% multilang(LANG_THIS_PAGE_IS_USED_TO_CONFIGURE_THE_PARAMETERS_FOR_YOUR_GPON_NETWORK_ACCESS); %></p>
</div>
<form action=/boaform/admin/formgponConf method=POST name="formgponconf">
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th width="40%"><% multilang(LANG_LOID); %>:</th>
			<td><input type="text" name="fmgpon_loid" size="20" maxlength="20" value="<% fmgpon_checkWrite("fmgpon_loid"); %>"></td>
		</tr>
		<tr>
			<th width="40%"><% multilang(LANG_LOID_PASSWORD); %>:</th>
			<td><input type="text" name="fmgpon_loid_password" size="20" maxlength="12" value="<% fmgpon_checkWrite("fmgpon_loid_password"); %>"></td>
		</tr>
		<tr>
			<th width="40%"><% multilang(LANG_PLOAM_PASSWORD); %>:</th>
			<td><input type="text" name="fmgpon_ploam_password" size="20" maxlength="10" value="<% fmgpon_checkWrite("fmgpon_ploam_password"); %>" ></td>
		</tr>
		<tr>
			<th><% multilang(LANG_SERIAL_NUMBER); %>:</th>
			<td><% fmgpon_checkWrite("fmgpon_sn"); %></td>
		</tr>
		<% showOMCI_OLT_mode(); %>
	</table>
</div>
<div class="btn_ctl clearfix">
      <input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="apply" onClick="return applyclick(this)">&nbsp;&nbsp;
      <input type="hidden" value="/gpon.asp" name="submit-url">
      <input type="hidden" name="postSecurityFlag" value="">
</div>
</form>
<br><br>
</body>
</html>
