<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<HTML>
<HEAD>
<TITLE>中国电信</TITLE>
<META http-equiv=pragma content=no-cache>
<META http-equiv=cache-control content="no-cache, must-revalidate">
<META http-equiv=content-type content="text/html; charset=gbk">
<META http-equiv=content-script-type content=text/javascript>
<!--系统公共css-->
<STYLE type=text/css>
@import url(/style/default.css);
</STYLE>
<!--系统公共脚本-->
<SCRIPT language="javascript" src="common.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript">
<% gettopstyle(); %>
/********************************************************************
**          on document load
********************************************************************/
function on_init()
{
	if(top_style == 0)
	{
		top.topFrame.location.href ="top.asp";
	}
	else if(top_style == 1)
	{
		top.topFrame.location.href ="top_awifi.asp";
	}
}

</SCRIPT>
</HEAD>

<!-------------------------------------------------------------------------------------->
<!--主页代码-->
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onLoad="on_init();">
<table border="0" cellpadding="0" cellspacing="0" width="100%" height="100%">
  <tr valign="top">
    <td bgcolor="#E7E7E7" width="100%" valign="top">
		<table border="0" cellpadding="0" cellspacing="0" width="100%" id="lstmenu">
		<tr><td></td></tr> 
		</table>
	</td>
    <td width="8px" background="image/UI_04.gif">&nbsp;</td>
  </tr>
</table>
</body>
</HTML>
