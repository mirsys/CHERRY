<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
<title>智能插件信息</title>
<meta http-equiv=pragma content=no-cache>
<meta http-equiv=cache-control content="no-cache, must-revalidate">
<meta http-equiv=content-type content="text/html; charset=gbk">
<style type=text/css>
@import url(/style/default.css);
</style>
<script language="javascript" src="common.js"></script>
<script language="javascript" type="text/javascript">
var rcs = new Array();
<% pluginModuleList(); %>

function on_init()
{
	if (lstrc.rows) {
		while (lstrc.rows.length > 1)
			lstrc.deleteRow(1);
	}
	for (var i = 0; i < rcs.length; i++) {
		var row = lstrc.insertRow(i + 1);

		row.nowrap = true;
		row.vAlign = "top";

		var cell = row.insertCell(0);
		cell.innerHTML = rcs[i][0];
		cell = row.insertCell(1);
		cell.innerHTML = rcs[i][1];
	}
}
</script>
</head>
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onLoad="on_init();">
	<blockquote>
		<div align="left" style="padding-left:20px;"><br>
			<div align="left"><b>插件信息</b></div>
			<br>
			<table class="flat" id="lstrc" border="1" cellpadding="0" cellspacing="1" width="90%">
			   <tr class="hdb" align="center">
				  <td>插件名称</td>
				  <td>运行状态</td>
			   </tr>
			</table>
			
		</div>
	</blockquote>
</body>
<%addHttpNoCache();%>
</html>
