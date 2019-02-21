<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html" charset="utf-8">
<title><% multilang(LANG_IGMP_PROXY); %><% multilang(LANG_CONFIGURATION); %></title>
<link rel="stylesheet" type="text/css" href="common_style.css"/>
<SCRIPT language="javascript" src="/common.js"></SCRIPT>
<SCRIPT>
function proxySelection()
{
	if (document.igmp.proxy[0].checked) {
		document.igmp.proxy_if.disabled = true;
	}
	else {
		document.igmp.proxy_if.disabled = false;
	}
}

function on_submit()
{
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}
</SCRIPT>
</head>

<body>
<blockquote>
<h2 class="page_title"><% multilang(LANG_IGMP_PROXY); %><% multilang(LANG_CONFIGURATION); %></h2>

<form action=/boaform/formIgmproxy method=POST name="igmp">
<table>
  <tr><td><font size=2>
    <% multilang(LANG_IGMP_PROXY_ENABLES_THE_SYSTEM_TO_ISSUE_IGMP_HOST_MESSAGES_ON_BEHALF_OF_HOSTS_THAT_THE_SYSTEM_DISCOVERED_THROUGH_STANDARD_IGMP_INTERFACES_THE_SYSTEM_ACTS_AS_A_PROXY_FOR_ITS_HOSTS_WHEN_YOU_ENABLE_IT_BY_DOING_THE_FOLLOWS); %>:
    <br>. <% multilang(LANG_ENABLE_IGMP_PROXY_ON_WAN_INTERFACE_UPSTREAM_WHICH_CONNECTS_TO_A_ROUTER_RUNNING_IGMP); %>
    <br>. <% multilang(LANG_ENABLE_IGMP_ON_LAN_INTERFACE_DOWNSTREAM_WHICH_CONNECTS_TO_ITS_HOSTS); %>
  </font></td></tr>
  <tr><td><hr size=1 noshade align=top></td></tr>
</table>

  <tr>
      <td class="table_item"><% multilang(LANG_IGMP_PROXY); %>:</td>
      <td width><font size=2>
      	<input type="radio" value="0" name="proxy" <% checkWrite("igmpProxy0"); %> onClick="proxySelection()"><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
     	<input type="radio" value="1" name="proxy" <% checkWrite("igmpProxy1"); %> onClick="proxySelection()"><% multilang(LANG_ENABLE); %>
      </td>
  </tr>
  <tr>
      <td class="table_item"><% multilang(LANG_PROXY_INTERFACE); %>:</td>
      <td>
      	<select name="proxy_if" <% checkWrite("igmpProxy0d"); %>>
          <% if_wan_list("rt"); %>
      	</select>
      </td>
      <td><input type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" onClick="return on_submit()">&nbsp;&nbsp;</td>
  </tr>
</table>
      <input type="hidden" value="/igmproxy.asp" name="submit-url">
      <input type="hidden" name="postSecurityFlag" value="">
<script>
	ifIdx = <% getInfo("igmp-proxy-itf"); %>;
	if (ifIdx != 255)
		document.igmp.proxy_if.value = ifIdx;
	else
		document.igmp.proxy_if.selectedIndex = 0;
	
</script>
</form>
</blockquote>
</body>

</html>
