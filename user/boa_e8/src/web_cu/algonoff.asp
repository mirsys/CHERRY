<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<HTML>
<HEAD>
<TITLE>ALG On-Off</TITLE>
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
function checkChange(cb)
{
	if(cb.checked==true){
		cb.value = 1;
	}
	else{
		cb.value = 0;
	}
}

</SCRIPT>
</head>

<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000">
<blockquote>
<DIV align="left" style="padding-left:20px; padding-top:5px">

<table  width=500>
<tr><td colspan=4></td></tr>
<form action=/boaform/formALGOnOff method=POST name=algof>
<table>
<% checkWrite("GetAlgType"); %>	
<tr>
	<td></td>
	<td></td>
</tr>
<tr>
  <td> <input type="hidden" value="/algonoff.asp" name="submit-url"></td>
  <td ><input  class="btnsaveup" type=submit value="保存/应用" name=apply></td>
  <td></td>
</tr>
</table>
</form>
</table>
</DIV>
</blockquote>
</body>
</html>
