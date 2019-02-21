<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_3G_SETTINGS); %></title>
<link rel="stylesheet" type="text/css" href="common_style.css" />
<SCRIPT language="javascript" src="/common.js"></SCRIPT>
<script type="text/javascript" src="share.js">
</script>
<script>
function checkDialNumber(str)
{
	for (var i=0; i<str.length; i++) 
	{
		if( (str.charAt(i) >= '0' && str.charAt(i) <= '9') ||
			(str.charAt(i) == '*') || (str.charAt(i) == '#') )
			continue;
		return 0;
	}
	return 1;
}
function authchange()
{
	if(document.form3gconf.auth.selectedIndex==3)
	{
		document.form3gconf.username.disabled=true;
		document.form3gconf.password.disabled=true;
	}else{
		document.form3gconf.username.disabled=false;
		document.form3gconf.password.disabled=false;
	}
}
function ctypechange()
{
	if(document.form3gconf.ctype.selectedIndex==1)
	{
		document.form3gconf.idletime.disabled=false;
	}else{
		document.form3gconf.idletime.disabled=true;
	}
}
function backupenable(enable) //paula, 3g backup PPP
{
	if(enable==1)
	{
		document.form3gconf.backup_timer.disabled=false;
		document.form3gconf.droute[0].disabled = true;
		document.form3gconf.droute[1].disabled = true;
		document.form3gconf.droute[0].checked = false;
		document.form3gconf.droute[1].checked = true;
		//document.form3gconf.droute.value=0;
		document.form3gconf.ctype.disabled = true;
		document.form3gconf.ctype.selectedIndex = 0;
	}else{
		document.form3gconf.backup_timer.disabled=true;
		document.form3gconf.droute[0].disabled = false;
		document.form3gconf.droute[1].disabled = false;
		document.form3gconf.droute[0].checked = false;
		document.form3gconf.droute[1].checked = true;
		document.form3gconf.ctype.disabled = false;
	}
}
function applyclick()
{
	/*pin code*/
	if( !((document.form3gconf.pin.value.length==0) ||
	      ((document.form3gconf.pin.value.length==4)&&
	        checkDigit(document.form3gconf.pin.value))) )
	{
		alert("<% multilang(LANG_INVALID_PIN_CODE_VALUE); %>");
		/*document.form3gconf.pin.value=document.form3gconf.pin.defaultValue;*/
		document.form3gconf.pin.focus();
		return false;
	}
	
	/*apn*/
	if( (includeSpace(document.form3gconf.apn.value)==1) || 
		(checkString(document.form3gconf.apn.value)==0) )
	{
		alert("<% multilang(LANG_INVALID_APN_VALUE_APN_MUST_BE_A_STRING_WITHOUT_ANY_SPACE); %>");
		/*document.form3gconf.apn.value=document.form3gconf.apn.defaultValue;*/
		document.form3gconf.apn.focus();
		return false;
	}

	/*dial*/
	if( checkDialNumber(document.form3gconf.dial.value)==0 )
	{
		alert("<% multilang(LANG_INVALID_DIAL_NUMBER_VALUE_DIAL_NUMBER_MUST_BE_A_PHONE_NUMBER); %>");
		/*document.form3gconf.dial.value=document.form3gconf.dial.defaultValue;*/
		document.form3gconf.dial.focus();
		return false;
	}

	/*auth*/
	if( document.form3gconf.auth.selectedIndex!=3 )
	{
		/*username*/
		if (document.form3gconf.username.value=="") {
			alert("<% multilang(LANG_USER_NAME_CANNOT_BE_EMPTY); %>");
			document.form3gconf.username.focus();
			return false;
		}
		if (includeSpace(document.form3gconf.username.value)) {
			alert("<% multilang(LANG_CANNOT_ACCEPT_SPACE_CHARACTER_IN_USER_NAME); %>");
			document.form3gconf.username.focus();
			return false;
		}
		if (checkString(document.form3gconf.username.value) == 0) {
			alert("<% multilang(LANG_INVALID_USER_NAME); %>");
			document.form3gconf.username.focus();
			return false;
		}
		
		/*password*/
		if (document.form3gconf.password.value=="") {
			alert("<% multilang(LANG_PASSWORD_CANNOT_BE_EMPTY); %>");
			document.form3gconf.password.focus();
			return false;
		}
		if (includeSpace(document.form3gconf.password.value)) {
			alert("<% multilang(LANG_CANNOT_ACCEPT_SPACE_CHARACTER_IN_PASSWORD); %>");
			document.form3gconf.password.focus();
			return false;
		}
		if (checkString(document.form3gconf.password.value) == 0) {
			alert("<% multilang(LANG_INVALID_PASSWORD); %>");
			document.form3gconf.password.focus();
			return false;
		}
	}

	/*ctype*/	
	if (document.form3gconf.ctype.selectedIndex == 1) 
	{
		/*idletime*/
		if( checkDigit(document.form3gconf.idletime.value)==0 ||
		    document.form3gconf.idletime.value<=0 ) 
		{
			alert("<% multilang(LANG_INVALID_IDLE_TIME); %>");
			document.form3gconf.idletime.focus();
			return false;
		}
	}

	/*mtu*/
	if( checkDigit(document.form3gconf.mtu.value)==0 ||
	    document.form3gconf.mtu.value<65 || 
	    document.form3gconf.mtu.value>1500 )
	{
		alert("<% multilang(LANG_INVALID_MTU_VALUE_YOU_SHOULD_SET_A_VALUE_BETWEEN_65_1500); %>");
		/*document.form3gconf.mtu.value=document.form3gconf.mtu.defaultValue;*/
		document.form3gconf.mtu.focus();
		return false;
	}
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}
</script>
</head>

<body>
<blockquote>
<h2 class="page_title"><% multilang(LANG_3G_SETTINGS); %></h2>
<form action=/boaform/admin/form3GConf method=POST name="form3gconf">
<table>
  <tr><td><font size=2>
    <% multilang(LANG_PAGE_DESC_3G_CONFIGURE); %>
  </font></td></tr>
  <tr><td><hr size=1 noshade align=top></td></tr>
</table>
<table>
  <tr>
      <th>3G <% multilang(LANG_WAN); %>:</th>
      <td>
	      <input type="radio" name="enable3g" value=0 <% fm3g_checkWrite("fm3g-enable-dis"); %> ><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
	      <input type="radio" name="enable3g" value=1 <% fm3g_checkWrite("fm3g-enable-en"); %> ><% multilang(LANG_ENABLE); %>
      </td>
  </tr>
<!--ISP-related-->
  <tr>
      <th><% multilang(LANG_PIN_CODE); %>:</th>
      <td><input type="text" name="pin" size="32" maxlength="4" value="<% fm3g_checkWrite("fm3g-pin"); %>"></td>
  </tr>
  <tr>
      <th><% multilang(LANG_APN); %>:</th>
      <td><input type="text" name="apn" size="32" maxlength="63" value="<% fm3g_checkWrite("fm3g-apn"); %>"></td>
  </tr>
  <tr>
      <th><% multilang(LANG_DIAL_NUMBER); %>:</th>
      <td><input type="text" name="dial" size="32" maxlength="16" value="<% fm3g_checkWrite("fm3g-dial"); %>"></td>
  </tr>
<!--end ISP-related-->
<!--PPP-related-->
  <tr>
      <th><% multilang(LANG_AUTHENTICATION); %>:</th>
      <td>
	<select size=1 name="auth" onChange="authchange()">
		<option <% fm3g_checkWrite("fm3g-auth-auto"); %> value=0><% multilang(LANG_AUTO); %></option>
		<option <% fm3g_checkWrite("fm3g-auth-pap"); %> value=1>PAP</option>
		<option <% fm3g_checkWrite("fm3g-auth-chap"); %> value=2>CHAP</option>
		<option <% fm3g_checkWrite("fm3g-auth-none"); %> value=3><% multilang(LANG_NONE); %></option>
	</select>
      </td>
  </tr>
  <tr>
      <th><% multilang(LANG_USER); %><% multilang(LANG_NAME); %>:</th>
      <td><input type="text" name="username" size="32" maxlength="63" value="<% fm3g_checkWrite("fm3g-username"); %>" <% fm3g_checkWrite("fm3g-username-dis"); %>></td>
  </tr>
  <tr>
      <th><% multilang(LANG_PASSWORD); %>:</th>
      <td><input type="text" name="password" size="32" maxlength="29" value="<% fm3g_checkWrite("fm3g-password"); %>" <% fm3g_checkWrite("fm3g-password-dis"); %>></td>
  </tr>
  <tr>
      <th><% multilang(LANG_CONNECTION_TYPE); %>:</th>
      <td>
	<select size=1 name="ctype" onChange="ctypechange()" <% fm3g_checkWrite("fm3g-ctype-bu"); %> > <!-- paula, 3g backup PPP-->
		<option <% fm3g_checkWrite("fm3g-ctype-cont"); %> value=0><% multilang(LANG_CONTINUOUS); %></option>
		<option <% fm3g_checkWrite("fm3g-ctype-demand"); %> value=1><% multilang(LANG_CONNECT_ON_DEMAND); %></option>
		<option <% fm3g_checkWrite("fm3g-ctype-manual"); %> value=2><% multilang(LANG_MANUAL); %></option>
	</select>
      </td>
  </tr>
  <tr>
      <th><% multilang(LANG_IDLE_TIME_MIN); %>:</th>
      <td><input type="text" name="idletime" size="32" maxlength="3" value="<% fm3g_checkWrite("fm3g-idletime"); %>" <% fm3g_checkWrite("fm3g-idletime-dis"); %>></td>
  </tr>
<!--end PPP-related-->
<!--Network-related-->
  <tr>
      <th><% multilang(LANG_NAPT); %>:</th>
      <td>
	      <input type="radio" name="napt" value=0 <% fm3g_checkWrite("fm3g-napt-dis"); %> ><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
	      <input type="radio" name="napt" value=1 <% fm3g_checkWrite("fm3g-napt-en"); %> ><% multilang(LANG_ENABLE); %>
      </td>
  </tr>
  <tr>
      <th><% multilang(LANG_DEFAULT_ROUTE); %>:</th>
      <td> 
	  <!-- 3g backup PPP, disable default route, paula-->
	      <input type="radio" name="droute" value=0 <% fm3g_checkWrite("fm3g-droute-dis"); %> <% fm3g_checkWrite("fm3g-droute-bu"); %> ><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
	      <input type="radio" name="droute" value=1 <% fm3g_checkWrite("fm3g-droute-en"); %> <% fm3g_checkWrite("fm3g-droute-bu"); %> ><% multilang(LANG_ENABLE); %>
      <!-- end 3g backup PPP disable default route-->
	  </td>
  </tr>
  <tr>
      <th><% multilang(LANG_MTU); %>:</th>
      <td><input type="text" name="mtu" size="32" maxlength="4" value="<% fm3g_checkWrite("fm3g-mtu"); %>"></td>
  </tr>
  <!--3g backup PPP, paula-->
  <tr>
      <th><% multilang(LANG_BACKUP_FOR_ADSL); %>:</th>
      <td>
	      <input type="radio" name="backup" value=0 onClick="backupenable(this.value)" <% fm3g_checkWrite("fm3g-backup-dis"); %> ><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
	      <input type="radio" name="backup" value=1 onClick="backupenable(this.value)" <% fm3g_checkWrite("fm3g-backup-en"); %> ><% multilang(LANG_ENABLE); %>
      </td>
  </tr>
  <tr>
      <th><% multilang(LANG_BACKUP_TIMER_SEC); %>:</th>
      <td><input type="text" name="backup_timer" size="32" maxlength="4" value="<% fm3g_checkWrite("fm3g-backup_timer"); %>"  <% fm3g_checkWrite("fm3g-backup_timer-dis"); %> ></td>
  </tr>
  <!--end 3g backup PPP -->
<!--end Network-related-->
</table>
<br>
      <input type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>"  onClick="return applyclick()">&nbsp;&nbsp;
      <input type="reset" value="<% multilang(LANG_UNDO); %>" name="reset" onClick="window.location.reload()">
      <input type="hidden" value="/wan3gconf.asp" name="submit-url">
      <input type="hidden" name="postSecurityFlag" value="">
</form>
</blockquote>
</body>
</html>
