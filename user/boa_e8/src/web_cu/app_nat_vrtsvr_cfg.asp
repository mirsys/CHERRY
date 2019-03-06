<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--ϵͳĬ��ģ��-->
<HTML>
<HEAD>
<TITLE>�������������</TITLE>
<META http-equiv=pragma content=no-cache>
<META http-equiv=cache-control content="no-cache, must-revalidate">
<META http-equiv=content-type content="text/html; charset=gbk">
<META http-equiv=content-script-type content=text/javascript>
<!--ϵͳ����css-->
<STYLE type=text/css>
@import url(/style/default.css);
</STYLE>
<!--ϵͳ�����ű�-->
<SCRIPT language="javascript" src="common.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript">

var protos = new Array("TCP/UDP", "TCP", "UDP");
var root = new Array( 
	new it_mlo("Domain Name Server",
		new Array(53, 53, 2)),
	
	new it_mlo("FTP Server",
		new Array(21, 21, 1)),
		
	new it_mlo("IPSEC", 
		new Array(500, 500, 2)), 
	
	new it_mlo("Mail POP3",
		new Array(110, 110, 1)),

	new it_mlo("Mail SMTP",
		new Array(25, 25, 1)),
	
	new it_mlo("PPTP",
		new Array(1723, 1723, 1)),
	
	new it_mlo("Real Player 8 Plus",
		new Array(7070, 7070, 2)),
	
	new it_mlo("Secure Shell Server",
		new Array(22, 22, 1)),

	new it_mlo("Secure Web Server HTTPS",
		new Array(443, 443, 1)),
	
	new it_mlo("SNMP",
		new Array(161, 161, 2)),

	new it_mlo("SNMP Trap",
		new Array(162, 162, 2)),
	
	new it_mlo("Telnet Server",   
		new Array(23, 23, 1)),
	
	new it_mlo("TFTP",   
		new Array(69, 69, 2)),
		
	new it_mlo("Web Server HTTP",
		new Array(80, 80, 1))
		);
		
var rcs = new Array();
with(rcs){<% virtualSvrList(); %>}

function on_chkclick(index)
{
	if(index < 0 || index >= rcs.length)
		return; 
	rcs[index].select = !rcs[index].select;
}

/********************************************************************
**          on document load
********************************************************************/
function on_init()
{
	sji_docinit(document);
	
	for(var i = 0; i < root.length; i++)
	{
		form.srvName.options.add(new Option(root[i].name, root[i].name));
	}
	
	if(lstrc.rows){while(lstrc.rows.length > 1) lstrc.deleteRow(1);}
	for(var i = 0; i < rcs.length; i++)
	{
		var row = lstrc.insertRow(i + 1);
		
		row.nowrap = true;
		row.vAlign = "top";
		row.align = "center";
		var cell = row.insertCell(0);
		cell.innerHTML = protos[rcs[i].protoType];
		cell = row.insertCell(1);
		if(rcs[i].remotehost[0] == '0')
			cell.innerHTML = "";
		else
			cell.innerHTML = rcs[i].remotehost;
		
		cell = row.insertCell(2);
		cell.innerHTML = rcs[i].wanStartPort;
		cell = row.insertCell(3);
		cell.innerHTML = rcs[i].wanEndPort;
		cell = row.insertCell(4);
		cell.innerHTML = rcs[i].serverIp;
		cell = row.insertCell(5);
		cell.innerHTML = rcs[i].svrName;
		cell = row.insertCell(6);
		if(rcs[i].enable == "1")
			cell.innerHTML = "����";
		else
			cell.innerHTML = "�ر�";
		cell = row.insertCell(7);
		cell.align = "center";
		cell.innerHTML = "<input type=\"checkbox\" name=\"rml"
				+ i + "\" value=\"ON\" onClick=\"on_chkclick(" + i + ");\">";
	}
}

/********************************************************************
**          on document submit
********************************************************************/
function on_submit_add() 
{
	with ( document.forms[0] ) 
	{
		if(form.radiosrv[1].checked == true && form.srvName.selectedIndex == 0)
		{
			form.srvName.focus();
			alert("��ѡ��һ������");
			return;
		}
		if(form.radiosrv[0].checked == true && sji_checkstrnor(form.cusSrvName.value, 1, 60) == false)
		{
			form.cusSrvName.focus();
			alert("�Զ����������" + form.cusSrvName.value + "Ϊ��Ч���ƣ����������룡");
			return;
		}

		if((form.remotehost.value== "0.0.0.0") || (form.remotehost.value.length > 0 && sji_checkvip(form.remotehost.value) == false))
		{
			form.remotehost.focus();
			alert("ԴIP��ַ��" + form.remotehost.value + "Ϊ��ЧIP�����������룡");
			return;
		}

		if(sji_checkdigitrange(form.wanStartPort.value, 1, 65535) == false)
		{
			form.wanStartPort.focus();
			alert("��ʼ�˿ںţ�" + form.wanStartPort.value + "Ϊ��Ч�˿ڣ����������룡");
			return;
		}
		if(sji_checkdigitrange(form.wanEndPort.value, parseInt(form.wanStartPort.value, 10), 65535) == false)
		{
			form.wanEndPort.focus();
			alert("�����˿ںţ�" + form.wanEndPort.value + "Ϊ��Ч�˿ڣ����������룡");
			return;
		}
		if(sji_checkvip(serverIp.value) == false)
		{
			form.serverIp.focus();
			alert("�ڲ�������" + form.serverIp.value + "Ϊ��ЧIP�����������룡");
			return;
		}
	
		form.action.value = "add";
		form.submit();
		}
}

function on_submit_delete() 
{
	with ( document.forms[0] ) 
	{
		form.bcdata.value = sji_encode(rcs, "select");
		form.action.value = "delete";
		form.submit();
	}
}

function on_mode()
{
	if (form.radiosrv[0].checked == true)
	{
		form.srvName.disabled = true;
	}
	else
	{
		form.srvName.disabled = false;
	}
	//reset_all();
	form.srvName.selectedIndex = 0;
}

function on_change(index)
{
	if(index < 0 || index > root.length)return false;
	if(index == 0)
	{
		form.wanStartPort.value="";
		form.wanEndPort.value="";
		form.protoType.value="0";
		return;
	}
	index--;
	
	with (form) 
	{
		form.wanStartPort.value = root[index].childs[0][0];
		form.wanEndPort.value = root[index].childs[0][1];
		form.protoType.value = root[index].childs[0][2];
		form.cusSrvName.value = root[index].name;
	}
}

function on_extif()
{
	if(form.ext_if=="" || form.ext_if=="0"){
		document.getElementById('div_Vrtsrv').style.display = 'none';
	}
	else{
		document.getElementById('div_Vrtsrv').style.display = 'block';
	}
}

</SCRIPT>
</HEAD>

<!-------------------------------------------------------------------------------------->
<!--��ҳ����-->
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onLoad="on_init();">
	<blockquote>
		<DIV align="left" style="padding-left:20px; padding-top:5px">
			<form id="form" name="form" action="/boaform/admin/formVrtsrv" method="post">
				<br>
				<table cellpadding="0" cellspacing="1">
					<tr>
						<td align="left">�ӿڣ�</td>
						<td><select name="ext_if" onclick="on_extif()"> <% if_wan_list("rt"); %> </select></td>
					</tr>
					<tr>
						<td align="left"><input type="radio" name="radiosrv" onclick="on_mode()" value="">�Զ���:</td>
						<td></td>
					</tr>
					<tr>
						<td align="left"><input type="radio" name="radiosrv" onclick="on_mode()" value="auto" checked>ѡ��Ӧ�ã�</td>
						<td>
							<select name="srvName" size="1" onChange="on_change(this.selectedIndex);" style="width:180px ">
								<option value="none">ѡ��...</option>
							</select>
						</td>
					</tr>
					<tr>
						<td align="left">Э�飺</td>
						<td>
							<select name="protoType">
								<option value="1" selected>
								TCP
								<option value="2">
								UDP
								<option value="0">
								TCP/UDP
							 </select>
						</td>
					</tr>
					<tr>
						<td align="left">ԴIP��ַ��</td>
						<td>
							<input type="text" size="15" name="remotehost">
						</td>
					</tr>
					<tr>
						<td align="left">��ʼ�˿ںţ�</td>
						<td>
							<input type="text" size="12" name="wanStartPort">
						</td>
					</tr>
					<tr>
						<td align="left">�����˿ںţ�</td>
						<td>
							<input type="text" size="12" name="wanEndPort">
						</td>
					</tr>
					<tr>
						<td align="left">�ڲ�������</td>
						<td>
							<input type="text" size="15" name="serverIp">
						</td>
					</tr>
					<tr>
						<td align="left">ӳ�����֣�</td>
						<td>
							<input type="text" size="15" name="cusSrvName">
						</td>
					</tr>
				</table>
				<DIV id="div_Vrtsrv" name="div_Vrtsrv" style="display:block;">				
				<table class="flat" id="lstrc" border="1" cellpadding="0" cellspacing="1">
				   <tr class="hdb" align="center" nowrap>
					  <td width="60px">Э��</td>
					  <td width="120px">ԴIP��ַ</td>
					  <td width="90px">��ʼ�˿ں�</td>
					  <td width="90px">�����˿ں�</td>
					  <td width="120px">�ڲ�����</td>
					  <td width="120px">ӳ������</td>
					  <td width="60px">ʹ��</td>
					  <td width="40px">ɾ��</td>
				   </tr>
				</table>
				</DIV>
				<br>
				<input type="button" class="btnaddup" onClick="on_submit_add();" value="����">
				<input type="button" class="btndeleup" onClick="on_submit_delete();" value="ɾ��">
				<input type="hidden" id="action" name="action" value="delete">
				<input type="hidden" name="bcdata" value="le">
				<input type="hidden" name="submit-url" value="">
			</form>
		</DIV>
	</blockquote>
<script>
	document.form.ext_if.selectedIndex = 0;
		
	on_extif();
</script>
</body>
<%addHttpNoCache();%>
</html>