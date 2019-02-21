<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_PARENTAL_CONTROL); %></title>
<link rel="stylesheet" type="text/css" href="common_style.css" />
<script type="text/javascript" src="share.js">
</script>
<SCRIPT language="javascript" src="/common.js"></SCRIPT>
<script>
function skip () { this.blur(); }
function addClick(obj)
{
  var ls, le;
  var ls1, le1;
  var ls2, le2;
  var ls3, le3;
  
  ls = getDigit(document.formParentCtrlAdd.ipstart.value,4);
  le = getDigit(document.formParentCtrlAdd.ipend.value,4); 
  
  ls1 = getDigit(document.formParentCtrlAdd.ipstart.value,1);
  le1 = getDigit(document.formParentCtrlAdd.ipend.value,1);  
  
  ls2 = getDigit(document.formParentCtrlAdd.ipstart.value,2);
  le2 = getDigit(document.formParentCtrlAdd.ipend.value,2); 
  
  ls3 = getDigit(document.formParentCtrlAdd.ipstart.value,3);
  le3 = getDigit(document.formParentCtrlAdd.ipend.value,3);  
	
  if (document.formParentCtrlAdd.usrname.value=="" )
  {
  	alert("<% multilang(LANG_INPUT_USERNAME); %>");
	document.formParentCtrlAdd.usrname.focus();
	return false;
  }
  
  if (document.formParentCtrlAdd.specPC[0].checked) 
  {
     if (!checkHostIP(document.formParentCtrlAdd.ipstart, 1))
     {	
	   document.formParentCtrlAdd.ipstart.focus();
	   return false;
     }
     
     if (!checkHostIP(document.formParentCtrlAdd.ipend, 1))
     {	
	   document.formParentCtrlAdd.ipend.focus();
	   return false;
     }
     
     if ( ls1 != le1 || ls2 != le2 || ls3 != le3 ) 
     {
  	  alert("<% multilang(LANG_LOCAL_START_IP_DOMAIN_IS_DIFFERENT_FROM_END_IP); %>");
  	  document.formParentCtrlAdd.ipstart.focus();
  	  return false;
     } 
   		
     if ( le <= ls ) 
     {
	  alert("<% multilang(LANG_INVALID_IP_RANGE); %>");
	  document.formParentCtrlAdd.ipstart.focus();
	  return false;
     } 
  }	 

  if (document.formParentCtrlAdd.specPC[1].checked) 
  {  
    if (document.formParentCtrlAdd.mac.value=="" )
  {
  	alert("<% multilang(LANG_INPUT_MAC_ADDRESS); %>");
	document.formParentCtrlAdd.mac.focus();
	return false;
  }
  var str = document.formParentCtrlAdd.mac.value;
  if ( str.length < 12) {
	alert("<% multilang(LANG_INPUT_MAC_ADDRESS_IS_NOT_COMPLETE_IT_SHOULD_BE_12_DIGITS_IN_HEX); %>");
	document.formParentCtrlAdd.mac.focus();
	return false;
  }

  for (var i=0; i<str.length; i++) {
    if ( (str.charAt(i) >= '0' && str.charAt(i) <= '9') ||
			(str.charAt(i) >= 'a' && str.charAt(i) <= 'f') ||
			(str.charAt(i) >= 'A' && str.charAt(i) <= 'F') )
			continue;

	alert("<% multilang(LANG_INVALID_MAC_ADDRESS_IT_SHOULD_BE_IN_HEX_NUMBER_0_9_OR_A_F); %>");
	document.formParentCtrlAdd.mac.focus();
	return false;
  }
  }
  
  if ((document.formParentCtrlAdd.starthr.value=="" )
  	|| (document.formParentCtrlAdd.startmin.value=="" )
  	||(document.formParentCtrlAdd.endhr.value=="" )
  	||(document.formParentCtrlAdd.endmin.value=="" )){
  	alert("<% multilang(LANG_INVALID_VALUE_OF_TIME_VALUE_PLEASE_INPUT_CORRENT_TIME_FORMAT_00_00_23_59); %>");
  	document.formParentCtrlAdd.starthr.focus();
	return false;
  }
	num1 = parseInt(document.formParentCtrlAdd.starthr.value,10);
	num2 = parseInt(document.formParentCtrlAdd.startmin.value,10);
	num3 = parseInt(document.formParentCtrlAdd.endhr.value,10);
	num4 = parseInt(document.formParentCtrlAdd.endmin.value,10);
  if ((num1 > 23)||(num3 > 23) )   {
  	alert("<% multilang(LANG_INVALID_VALUE_OF_TIME_VALUE_PLEASE_INPUT_CORRENT_TIME_FORMAT_00_00_23_59); %>");
  	document.formParentCtrlAdd.starthr.focus();
	return false;
  }

  
  if ( num1 > num3 )   {
  	alert("<% multilang(LANG_END_TIME_SHOULD_BE_LARGER); %>");
  	document.formParentCtrlAdd.starthr.focus();
	return false;
  }
   if (( num1 == num3 ) && (num2 >= num4))   {
  	alert("<% multilang(LANG_END_TIME_SHOULD_BE_LARGER); %>");
  	document.formParentCtrlAdd.starthr.focus();
	return false;
  } 
  
  if ((num2 > 59)||(num4 > 59) )   {
  	alert("<% multilang(LANG_INVALID_VALUE_OF_TIME_VALUE_PLEASE_INPUT_CORRENT_TIME_FORMAT_EX_12_50); %>");
  	document.formParentCtrlAdd.starthr.focus();
	return false;
  }

  obj.isclick = 1;
  postTableEncrypt(document.formParentCtrlAdd.postSecurityFlag, document.formParentCtrlAdd);
	
  return true;
}

function disableDelButton()
{
  if (verifyBrowser() != "ns") {
	disableButton(document.formParentCtrlDel.deleteSelFilterMac);
	disableButton(document.formParentCtrlDel.deleteAllFilterMac);
  }
}

function updateState()
{
//  if (document.formParentCtrlAdd.enabled.checked) {
  if (document.formParentCtrlAdd.specPC[0].checked) {
 	enableTextField(document.formParentCtrlAdd.ipstart);
	enableTextField(document.formParentCtrlAdd.ipend);
	disableTextField(document.formParentCtrlAdd.mac);
  }
  else {
 	enableTextField(document.formParentCtrlAdd.mac);
	disableTextField(document.formParentCtrlAdd.ipstart);
	disableTextField(document.formParentCtrlAdd.ipend);
  }
}

function deleteClick(obj)
{
	if ( !confirm('<% multilang(LANG_CONFIRM_DELETE_ONE_ENTRY); %>') ) {
		return false;
	}
	else{
		obj.isclick = 1;
		postTableEncrypt(document.formParentCtrlDel.postSecurityFlag, document.formParentCtrlDel);
		return true;
	}
}
        
function deleteAllClick(obj)
{
	if ( !confirm('Do you really want to delete the all entries?') ) {
		return false;
	}
	else{
		obj.isclick = 1;
		postTableEncrypt(document.formParentCtrlDel.postSecurityFlag, document.formParentCtrlDel);
		return true;
	}
}

function on_submit(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.formParentCtrl.postSecurityFlag, document.formParentCtrl);
	return true;
}

</script>
</head>

<body>
<blockquote>
<h2 class="page_title"><% multilang(LANG_PARENTAL_CONTROL); %></h2>

<table>
<tr><td><font size=2>
 <% multilang(LANG_ENTRIES_IN_THIS_TABLE_ARE_USED_TO_RESTRICT_ACCESS_TO_INTERNET_FROM_YOUR_LOCAL_PCS_DEVICES_BY_MAC_ADDRESS_AND_TIME_INTERVAL_USE_OF_SUCH_FILTERS_CAN_BE_HELPFUL_FOR_PARENTS_TO_CONTROL_CHILDREN_S_USAGE_OF_INTERNET); %>
</font></td></tr>
</table>

<table>
<form action=/boaform/admin/formParentCtrl method=POST name="formParentCtrl">
	<tr>
		<th><% multilang(LANG_PARENTAL_CONTROL); %>:</th>
		<td>
			<input type="radio" value="0" name="parental_ctrl_on" <% checkWrite("parental-ctrl-on-0"); %> ><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
			<input type="radio" value="1" name="parental_ctrl_on" <% checkWrite("parental-ctrl-on-1"); %> ><% multilang(LANG_ENABLE); %>&nbsp;&nbsp;
		</td>
		<td><input type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="parentalCtrlSet" onClick="return on_submit(this)"></td>
		<td><input type="hidden" value="/parental-ctrl.asp" name="submit-url"></td>
		<td><input type="hidden" name="postSecurityFlag" value=""></td>
	</tr>
</form>
</table>

<form action=/boaform/admin/formParentCtrl method=POST name="formParentCtrlAdd">
<table>
<tr><td colspan="2"><hr size=1 noshade align=top></td></tr>


<tr>
	<th><% multilang(LANG_USER); %><% multilang(LANG_NAME); %>:&nbsp;&nbsp;  </th>
	<td><input type="text" name="usrname" size="32" maxlength="31">&nbsp;&nbsp;</td>
</tr>
<tr>
	<th><% multilang(LANG_SPECIFIED_PC); %>:</th>
	<td>
		<input type="radio" value="0" name="specPC" checked onClick="updateState()"><% multilang(LANG_IP_ADDRESS); %>&nbsp;&nbsp;
		<input type="radio" value="1" name="specPC"  onClick="updateState()"><% multilang(LANG_MAC_ADDRESS); %>&nbsp;&nbsp;	     
	 </td>
</tr>
<tr>
	<th><% multilang(LANG_IP_ADDRESS); %>: </th>
	<td>
		<input type="text" name="ipstart" size="15" maxlength="15" value= >--
		<input type="text" name="ipend" size="15" maxlength="15" value= >
	 </td>
</tr>
<tr>
	<th><% multilang(LANG_MAC_ADDRESS); %>:&nbsp;&nbsp;  </th>
	<td><input type="text" name="mac" size="15" maxlength="12">(ex. 00e086710502)&nbsp;&nbsp;   </td>
</tr>
<table>
<tr>
	<th></th>
	<td align='center' width='60' ><font size="2"><% multilang(LANG_SUN); %></font></td>
	<td align='center' width='60' ><font size="2"><% multilang(LANG_MON); %></font></td>
	<td align='center' width='60' ><font size="2"><% multilang(LANG_TUE); %></font></td>
	<td align='center' width='60' ><font size="2"><% multilang(LANG_WED); %></font></td>
	<td align='center' width='60' ><font size="2"><% multilang(LANG_THU); %></font></td>
	<td align='center' width='60' ><font size="2"><% multilang(LANG_FRI); %></font></td>
	<td align='center' width='60' ><font size="2"><% multilang(LANG_SAT); %></font></td>
</tr>
<tr>
	<th><% multilang(LANG_CONTROLLED_DAYS); %></th>
	<td align='center'><input type='checkbox' name='Sun'></td>
	<td align='center'><input type='checkbox' name='Mon'></td>
	<td align='center'><input type='checkbox' name='Tue'></td>
	<td align='center'><input type='checkbox' name='Wed'></td>
	<td align='center'><input type='checkbox' name='Thu'></td>
	<td align='center'><input type='checkbox' name='Fri'></td>
	<td align='center'><input type='checkbox' name='Sat'></td>
</tr>
</table>
</table>
<table>
<tr>
	<th><% multilang(LANG_START_BLOCKING_TIME); %>:&nbsp;&nbsp; </th>
	<td>
		<input type="text" name="starthr" size="2" maxlength="2">:
		<input type="text" name="startmin" size="2" maxlength="2">
	 </td>
</tr>
<tr>
	<th><% multilang(LANG_END_BLOCKING_TIME); %>:&nbsp;&nbsp; </th>
	<td>
		<input type="text" name="endhr" size="2" maxlength="2">:
		<input type="text" name="endmin" size="2" maxlength="2">
	 </td>
</tr>
<tr>
<td><input type="submit" value="<% multilang(LANG_ADD); %>" name="addfilterMac" onClick="return addClick(this)">&nbsp;&nbsp;
        <!--<input type="reset" value="<% multilang(LANG_RESET); %>" name="reset">-->
<input type="hidden" value="/parental-ctrl.asp" name="submit-url">
<input type="hidden" name="postSecurityFlag" value="">
</td>
</tr>
  <script> updateState(); </script>
</form>
</table>

<br>
<form action=/boaform/admin/formParentCtrl method=POST name="formParentCtrlDel">
  <table>
  <tr><font size=2><b><% multilang(LANG_CURRENT_PARENT_CONTROL_TABLE); %>:</b></font></tr>
  <% parentControlList(); %>
  </table>
  <br>
  <input type="submit" value="<% multilang(LANG_DELETE_SELECTED); %>" name="deleteSelFilterMac" onClick="return deleteClick(this)">&nbsp;&nbsp;
  <input type="submit" value="<% multilang(LANG_DELETE_ALL); %>" name="deleteAllFilterMac" onClick="return deleteAllClick(this)">&nbsp;&nbsp;&nbsp;
  <input type="hidden" value="/parental-ctrl.asp" name="submit-url">
  <input type="hidden" name="postSecurityFlag" value="">
 <script>
 	<% checkWrite("parentCtrlNum"); %>
 </script>
</form>

</blockquote>
</body>
</html>
