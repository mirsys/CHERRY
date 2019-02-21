<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title>IPv6 IP/Port <% multilang(LANG_FILTERING); %></title>
<link rel="stylesheet" type="text/css" href="common_style.css" />
<script type="text/javascript" src="share.js">
</script>
<script>
function skip () { this.blur(); }

function protocolSelection()
{
	if ( document.formFilterAdd.protocol.selectedIndex == 2 )
	{
		document.formFilterAdd.sportStart.disabled = true;
		document.formFilterAdd.sportEnd.disabled = true;
		document.formFilterAdd.dportStart.disabled = true;
		document.formFilterAdd.dportEnd.disabled = true;
	}
	else
	{
		document.formFilterAdd.sportStart.disabled = false;
		document.formFilterAdd.sportEnd.disabled = false;
		document.formFilterAdd.dportStart.disabled = false;
		document.formFilterAdd.dportEnd.disabled = false;
	}
}

function addClick()
{
	if (document.formFilterAdd.sip6Start.value == "" && document.formFilterAdd.sip6PrefixLen.value == ""
		&& document.formFilterAdd.dip6Start.value == "" && document.formFilterAdd.dip6PrefixLen.value == ""
		&& document.formFilterAdd.sfromPort.value == "" && document.formFilterAdd.dfromPort.value == "") {		
		alert('<% multilang(LANG_FILTER_RULES_CAN_NOT_BE_EMPTY); %>');
		document.formFilterAdd.sip6Start.focus();
		return false;
	}

	with ( document.forms[1] )	{
	if(sip6Start.value != ""){
		if (! isGlobalIpv6Address(sip6Start.value) ){			
			alert('<% multilang(LANG_INVALID_SOURCE_IPV6_START_ADDRESS); %>');
			document.formFilterAdd.sip6Start.focus();
			return false;
		}
		if ( sip6PrefixLen.value != "" ) {
			if ( validateKey( document.formFilterAdd.sip6PrefixLen.value ) == 0 ) {				
				alert('<% multilang(LANG_INVALID_SOURCE_IPV6_PREFIX_LENGTH_IT_MUST_BE_0_9); %>');
				document.formFilterAdd.sip6PrefixLen.focus();
				return false;
			}

			var prefixlen= getDigit(sip6PrefixLen.value, 1);
			if (prefixlen > 64 || prefixlen < 0) {				
				alert('<% multilang(LANG_INVALID_SOURCE_IPV6_PREFIX_LENGTH_IT_MUST_BE_0_64); %>');
				document.formFilterAdd.sip6PrefixLen.focus();
				return false;
			}
		}
	}

	if(sip6End.value != ""){
		if (! isGlobalIpv6Address(sip6End.value) ){			
			alert('<% multilang(LANG_INVALID_SOURCE_IPV6_END_ADDRESS); %>');
			document.formFilterAdd.sip6End.focus();
			return false;
		}
	}

	if(dip6Start.value != ""){
		if (! isGlobalIpv6Address(dip6Start.value) ){			
			alert('<% multilang(LANG_INVALID_DESTINATION_IPV6_START_ADDRESS); %>');
			document.formFilterAdd.dip6Start.focus();
			return false;
		}
		if ( dip6PrefixLen.value != "" ) {
			if ( validateKey( document.formFilterAdd.dip6PrefixLen.value ) == 0 ) {				
				alert('<% multilang(LANG_INVALID_DESTINATION_IPV6_PREFIX_LENGTH_IT_MUST_BE_0_9); %>');
				document.formFilterAdd.dip6PrefixLen.focus();
				return false;
			}

			var prefixlen= getDigit(dip6PrefixLen.value, 1);
			if (prefixlen > 64 || prefixlen < 0) {				
				alert('<% multilang(LANG_INVALID_DESTINATION_IPV6_PREFIX_LENGTH_IT_MUST_BE_0_64); %>');
				document.formFilterAdd.dip6PrefixLen.focus();
				return false;
			}
		}
	}

	if(dip6End.value != ""){
		if (! isGlobalIpv6Address(dip6End.value) ){			
			alert('<% multilang(LANG_INVALID_DESTINATION_IPV6_END_ADDRESS); %>');
			document.formFilterAdd.dip6End.focus();
			return false;
		}
	}

	if ( document.formFilterAdd.sfromPort.value!="" ) {
		if ( validateKey( document.formFilterAdd.sfromPort.value ) == 0 ) {			
			alert('<% multilang(LANG_INVALID_SOURCE_PORT); %>');
			document.formFilterAdd.sfromPort.focus();
			return false;
		}

		d1 = getDigit(document.formFilterAdd.sfromPort.value, 1);
		if (d1 > 65535 || d1 < 1) {			
			alert('<% multilang(LANG_INVALID_SOURCE_PORT_NUMBER); %>');
			document.formFilterAdd.sfromPort.focus();
			return false;
		}

		if ( document.formFilterAdd.stoPort.value!="" ) {
			if ( validateKey( document.formFilterAdd.stoPort.value ) == 0 ) {				
				alert('<% multilang(LANG_INVALID_SOURCE_PORT); %>');
				document.formFilterAdd.stoPort.focus();
				return false;
			}

			d1 = getDigit(document.formFilterAdd.stoPort.value, 1);
			if (d1 > 65535 || d1 < 1) {				
				alert('<% multilang(LANG_INVALID_SOURCE_PORT_NUMBER); %>');
				document.formFilterAdd.stoPort.focus();
				return false;
			}
		}
	}

	if ( document.formFilterAdd.dfromPort.value!="" ) {
		if ( validateKey( document.formFilterAdd.dfromPort.value ) == 0 ) {
			alert('<% multilang(LANG_INVALID_DESTINATION_PORT); %>');
			document.formFilterAdd.dfromPort.focus();
			return false;
		}

		d1 = getDigit(document.formFilterAdd.dfromPort.value, 1);
		if (d1 > 65535 || d1 < 1) {
			alert('<% multilang(LANG_INVALID_DESTINATION_PORT_NUMBER); %>');
			document.formFilterAdd.dfromPort.focus();
			return false;
		}

		if ( document.formFilterAdd.dtoPort.value!="" ) {
			if ( validateKey( document.formFilterAdd.dtoPort.value ) == 0 ) {
				alert('<% multilang(LANG_INVALID_DESTINATION_PORT); %>');
				document.formFilterAdd.dtoPort.focus();
				return false;
			}

			d1 = getDigit(document.formFilterAdd.dtoPort.value, 1);
			if (d1 > 65535 || d1 < 1) {
				alert('<% multilang(LANG_INVALID_DESTINATION_PORT_NUMBER); %>');
				document.formFilterAdd.dtoPort.focus();
				return false;
			}
		}
	}
	}
	return true;
}

function disableDelButton()
{
  if (verifyBrowser() != "ns") {
	disableButton(document.formFilterDel.deleteSelFilterIpPort);
	disableButton(document.formFilterDel.deleteAllFilterIpPort);
  }
}

</script>
</head>

<body>
<blockquote>
<h2 class="page_title">IPv6 IP/Port <% multilang(LANG_FILTERING); %></h2>

<table>
<tr><td><font size=2>
 <% multilang(LANG_PAGE_DESC_DATA_PACKET_FILTER_TABLE); %>
</font></td></tr>

<tr><td><hr size=1 noshade align=top></td></tr>
</table>

<table>

<form action=/boaform/admin/formFilterV6 method=POST name="formFilterDefault">
<tr><th><% multilang(LANG_DEFAULT_ACTION); %>&nbsp;&nbsp;</th>
<td><input type="radio" name="outAct" value=0 <% checkWrite("v6_ipf_out_act0"); %>><% multilang(LANG_DENY); %>&nbsp;&nbsp;
   	<input type="radio" name="outAct" value=1 <% checkWrite("v6_ipf_out_act1"); %>><% multilang(LANG_ALLOW); %>&nbsp;&nbsp;</td>
<td><input type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="setDefaultAction">&nbsp;&nbsp;
	<input type="hidden" value="/admin/fw-ipportfilter-v6_rg.asp" name="submit-url">
</td></tr>
</form>
</table>
<table>
<tr><td><hr size=1 noshade align=top></td></tr>
<form action=/boaform/admin/formFilterV6 method=POST name="formFilterAdd">
<tr>
	<th>
	<!--<b><% multilang(LANG_DIRECTION); %>: </b>
		<select name=dir>
			<option select value=0><% multilang(LANG_OUTGOING); %></option>
			<option value=1><% multilang(LANG_INCOMING); %></option>
		</select>&nbsp;&nbsp; -->
	<% multilang(LANG_PROTOCOL); %>: 
		<select name="protocol" onChange="protocolSelection()">
			<option select value=1>TCP</option>
			<option value=2>UDP</option>
			<option value=3>ICMPV6</option>
		</select>&nbsp;&nbsp;</th>
	<th><% multilang(LANG_RULE_ACTION); %>
   		<input type="radio" name="filterMode" value="Deny" checked>&nbsp;<% multilang(LANG_DENY); %>
   		<input type="radio" name="filterMode" value="Allow">&nbsp;&nbsp;<% multilang(LANG_ALLOW); %>
	</th>
</tr>
<table>

<table>
   <tr>
	  <th><% multilang(LANG_SOURCE); %> <% multilang(LANG_IP_ADDRESS); %>:</th>
	  <td><input type="text" size="16" name="sip6Start" style="width:150px"> - <input type="text" size="16" name="sip6End" style="width:150px"></td>
   </tr>

   <tr>
	  <th><% multilang(LANG_SOURCE); %> <% multilang(LANG_PREFIX_LENGTH); %>:</th>
	  <td><input type="text" size="16" name="sip6PrefixLen" style="width:150px"></td>
   </tr>
   <tr>
	  <th><% multilang(LANG_DESTINATION); %> <% multilang(LANG_IP_ADDRESS); %>:</th>
	  <td><input type="text" size="16" name="dip6Start" style="width:150px"> - <input type="text" size="16" name="dip6End" style="width:150px"></td>
   </tr>
   <tr>
	  <th><% multilang(LANG_DESTINATION); %> <% multilang(LANG_PREFIX_LENGTH); %>:</th>
	  <td><input type="text" size="16" name="dip6PrefixLen" style="width:150px"></td>
   </tr>
   <tr>
	  <th><% multilang(LANG_SOURCE); %> <% multilang(LANG_PORT); %>:</th>
	  <td><input type="text" size="6" name="sfromPort" style="width:150px"> - <input type="text" size="6" name="stoPort" style="width:150px"></td>
   </tr>
   <tr>
	  <th><% multilang(LANG_DESTINATION); %> <% multilang(LANG_PORT); %>:</th>
	  <td><input type="text" size="6" name="dfromPort" style="width:150px"> - <input type="text" size="6" name="dtoPort" style="width:150px"></td>
   </tr>
</table>

<tr>
	<td>
	<input type="submit" value="<% multilang(LANG_ADD); %>" name="addFilterIpPort" onClick="return addClick()">
	<input type="hidden" value="/fw-ipportfilter-v6_rg.asp" name="submit-url">
	</td>
</tr>
<tr><td><hr size=1 noshade align=top></td></tr>
</form>
</table>

<form action=/boaform/admin/formFilterV6 method=POST name="formFilterDel">
	<table>
	<tr><font size=2><b><% multilang(LANG_CURRENT_FILTER_TABLE); %>:</b></font></tr>
	<% ipPortFilterListV6(); %>
	</table>
	<br>
	<input type="submit" value="<% multilang(LANG_DELETE_SELECTED); %>" name="deleteSelFilterIpPort" onClick="return deleteClick()">&nbsp;&nbsp;
	<input type="submit" value="<% multilang(LANG_DELETE_ALL); %>" name="deleteAllFilterIpPort" onClick="return deleteAllClick()">&nbsp;&nbsp;&nbsp;
	<input type="hidden" value="/admin/fw-ipportfilter-v6_rg.asp" name="submit-url">
	<script>
		<% checkWrite("ipFilterNumV6"); %>
	</script>
</form>

</blockquote>
</body>
</html>
