<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--ϵͳĬ��ģ��-->
<HTML>
<HEAD>
<TITLE>�˿ڹ���-����������</TITLE>
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

var links = new Array();
with(links){<%listWanif("rt");%>}

function on_chkclick(index)
{
	if(index < 0 || index >= links.length)
		return;
	links[index].select = !links[index].select;
	if(links[index].select)form.portnum.value = parseInt(form.portnum.value) + 1;
	else form.portnum.value = parseInt(form.portnum.value) - 1;
}
/*
function on_sel()
{
	with(form)
	{
		for(var i = 0; i <lstrc.rows.length; i++)
		{
			lstrc.rows[i].cells[0].children[0].click();
		}
	}
}
*/
/********************************************************************
**          on document load
********************************************************************/
function on_init()
{
	sji_docinit(document);
	/*
	if(lstrc.rows){while(lstrc.rows.length > 0) lstrc.deleteRow(0);}
	for(var i = 0; i < links.length; i++)
	{
		var row = lstrc.insertRow(i);

		row.nowrap = true;
		row.vAlign = "top";
		row.align = "center";

		var cell = row.insertCell(0);
		cell.innerHTML = "<input type=\"checkbox\" onClick=\"on_chkclick(" + i + ");\">";
		cell = row.insertCell(1);
		cell.innerHTML = links[i].displayname(1);
	}
	*/

/*
	if ( <%checkWrite("IPv6Show");%> )
	{
		if (document.getElementById)  // DOM3 = IE5, NS6
		{
			document.getElementById('ipprotbl').style.display = 'block';
		}
		else {
			if (document.layers == false) // IE4
			{
				document.all.ipprotbl.style.display = 'block';
			}
		}

		protocolChange();
	}
*/
}

function refresh()
{
	window.location.reload(true);
}

function protocolChange()
{
	// If protocol is IPv4 only.
	if(document.forms[0].IpProtocolType.value == 1){
		if (document.getElementById)  // DOM3 = IE5, NS6
		{
			document.getElementById('ip4tbl').style.display = 'block';
			document.getElementById('ip6tbl').style.display = 'none';
			document.getElementById('ip4protoType').style.display = 'block';
			document.getElementById('ip6protoType').style.display = 'none';
		}
		else {
			if (document.layers == false) // IE4
			{
				document.all.ip4tbl.style.display = 'block';
				document.all.ip6tbl.style.display = 'none';
				document.all.ip4protoType.style.display = 'block';
				document.all.ip6protoType.style.display = 'none';
			}
		}
	}
	// If protocol is IPv6 only.
	else if(document.forms[0].IpProtocolType.value == 2){
		if (document.getElementById)  // DOM3 = IE5, NS6
		{
			document.getElementById('ip4tbl').style.display = 'none';
			document.getElementById('ip6tbl').style.display = 'block';
			document.getElementById('ip4protoType').style.display = 'none';
			document.getElementById('ip6protoType').style.display = 'block';
		}
		else {
			if (document.layers == false) // IE4
			{
				document.all.ip4tbl.style.display = 'none';
				document.all.ip6tbl.style.display = 'block';
				document.all.ip4protoType.style.display = 'none';
				document.all.ip6protoType.style.display = 'block';
			}
		}
	}
}

// Mason Yu:20110524 ipv6 setting. START

/********************************************************************
**          on document submit
********************************************************************/
function on_submit()
{
	with ( document.forms[0] )
	{
		if(filterName.value.length <= 0)
		{
			filterName.focus();
			alert("������������Ϊ�գ��������������!");
			return;
		}
		if(sji_checkstrnor(filterName.value, 1, 22) == false)
		{
			filterName.focus();
			alert("�����������������������������!");
			return;
		}

	    if(sipStart.value.length==0 && sipEnd.value.length)
	    {
			sipStart.focus();
			alert("ԴIP������ַ�������ԴIP��ʼ��ַ��");
			return;
	    }
	    if(dipStart.value.length==0 && dipEnd.value.length)
		{
			dipStart.focus();
			alert("Ŀ��IP������ַ�������Ŀ��IP��ʼ��ַ��");
			return;
		}
		
	    if(smask.value.length && sipStart.value.length==0)
	    {
			sipStart.focus();
			alert("Դ��������������ԴIP��ַ��")
			return;
	    }
	    if(dmask.value.length && dipStart.value.length==0)
	    {
			dipStart.focus();
			alert("Ŀ����������������Ŀ��IP��ַ��");
			return;
	    }
		
	    if(sportStart.value.length==0 && sportEnd.value.length)
	    {
			sportStart.focus();
			alert("����Դ�˿ڱ��������ʼԴ�˿ڣ�");
			return;
	    }
	    if(sportStart.value.length==0 && sportEnd.value.length)
	    {
			sportStart.focus();
			alert("����Ŀ�Ķ˿ڱ��������ʼĿ�Ķ˿ڣ�");
			return;
	    }

		if(sipStart.value.length == 0)
		{
			sipStart.focus();
			alert("ԴIP��ַ(��ʼ) Ϊ��Ч��ַ�����������룡");
			return;
		}
		if((!isGlobalIpv6Address(sipStart.value)) && sji_checkvip(sipStart.value) == false)
		{
			sipStart.focus();
			alert("ԴIP��ַ(��ʼ)\"" + sipStart.value + "\"Ϊ��Ч��ַ�����������룡");
			return;
		}

		if(isGlobalIpv6Address(sipStart.value))
		{	
			IpProtocolType.value = 2;
 			protoTypeV6.value = protoType.value;
			sip6Start.value = sipStart.value;
			sip6End.value = sipEnd.value;
			dip6Start.value = dipStart.value;
			dip6End.value = dipEnd.value;
			protoType.value = 5;
			sipStart.value = "";
			sipEnd.value = "";
			dipStart.value = "";
			dipEnd.value = "";
		}
		else
		{
			IpProtocolType.value = 1;
		}

		//////////
		if((protoType.value ==0 || protoType.value ==4 ) && (protoTypeV6.value ==0 || protoTypeV6.value ==4 ) && (sportStart.value || sportEnd.value || dportStart.value || dportEnd.value))
		{
		  sportStart.focus();
		  alert("�˿ںű�����TCP, UDPһ��ѡ��");
		  return;
		}
		
		if(document.forms[0].IpProtocolType.value == 1){
			if(sipEnd.value.length == 0 || sji_checkvip(sipEnd.value) == false)
			{
				sipEnd.focus();
				alert("ԴIP��ַ(����)\"" + sipEnd.value + "\"Ϊ��Ч��ַ�����������룡");
				return;
			}
			if(sipStart.value.length != 0 && sipEnd.value.length != 0 && sji_ipcmp(sipStart.value, sipEnd.value) > 0)
			{
				sipEnd.focus();
				alert("ԴIP��ʼ��ַ���ܴ��ڽ�����ַ�����������������ַ��");
				return;
			}
			
			if(sipStart.value.length != 0 && sipEnd.value.length==0 && sji_checkmask(smask.value) == false)
			{
				smask.focus();
				alert("Դ��������\"" + smask.value + "\"Ϊ��Ч���룬���������룡");
				return;
			}
			if(sipStart.value.length != 0 && sipEnd.value.length != 0 && smask.value.length !=0)
			{
				smask.focus();
				alert("�趨ԴIP��Χʱ��������Դ�������룡");
				return;
			}
			
		}
		
		if(sportStart.value.length != 0 && sji_checkdigitrange(sportStart.value, 1, 65535) == false)
		{
			sportStart.focus();
			alert("Դ�˿�(��ʼ)\"" + sportStart.value + "\"Ϊ��Ч�˿ڣ����������룡");
			return;
		}
		if(sportEnd.value.length != 0 && sji_checkdigitrange(sportEnd.value, 1, 65535) == false)
		{
			sportEnd.focus();
			alert("Դ�˿�(����)\"" + sportStart.value + "\"Ϊ��Ч�˿ڣ����������룡");
			return;
		}
		if(sportStart.value.length != 0 && sportEnd.value.length != 0 && (parseInt(sportStart.value) > parseInt(sportEnd.value)))
		{
			sportEnd.focus();
			alert("Դ��ʼ�˿ڲ��ܴ��ڽ����˿ڣ���������������˿ڣ�");
			return;
		}
		///////////////

		if(document.forms[0].IpProtocolType.value == 1){
			if(dipStart.value.length == 0 || sji_checkvip(dipStart.value) == false)
			{
				dipStart.focus();
				alert("Ŀ��IP��ַ(��ʼ)\"" + sipStart.value + "\"Ϊ��Ч��ַ�����������룡");
				return;
			}
			if(dipEnd.value.length != 0 && sji_checkvip(dipEnd.value) == false)
			{
				dipEnd.focus();
				alert("Ŀ��IP��ַ(����)\"" + dipEnd.value + "\"Ϊ��Ч��ַ�����������룡");
				return;
			}
			if(dipStart.value.length != 0 && dipEnd.value.length != 0 && sji_ipcmp(dipStart.value, dipEnd.value) > 0)
			{
				dipEnd.focus();
				alert("Ŀ��IP��ʼ��ַ���ܴ��ڽ�����ַ�����������������ַ��");
				return;
			}
			
			if(dipStart.value.length != 0 &&  dipEnd.value.length==0 && sji_checkmask(dmask.value) == false)
			{
				dmask.focus();
				alert("Ŀ����������\"" + dmask.value + "\"Ϊ��Ч���룬���������룡");
				return;
			}
			if(dipStart.value.length != 0 && dipEnd.value.length != 0 && dmask.value.length !=0)
			{
				dmask.focus();
				alert("�趨Ŀ��IP��Χʱ��������Ŀ���������룡");
				return;
			}
			
		}
		if(dportStart.value.length != 0 && sji_checkdigitrange(dportStart.value, 1, 65535) == false)
		{
			dportStart.focus();
			alert("Ŀ�Ķ˿�(��ʼ)\"" + dportStart.value + "\"Ϊ��Ч�˿ڣ����������룡");
			return;
		}
		if(dportEnd.value.length != 0 && sji_checkdigitrange(dportEnd.value, 1, 65535) == false)
		{
			dportEnd.focus();
			alert("Ŀ�Ķ˿�(����)\"" + dportStart.value + "\"Ϊ��Ч�˿ڣ����������룡");
			return;
		}
		if(dportStart.value.length != 0 && dportEnd.value.length != 0 && (parseInt(dportStart.value) > parseInt(dportEnd.value)))
		{
			dportEnd.focus();
			alert("Ŀ����ʼ�˿ڲ��ܴ��ڽ����˿ڣ���������������˿ڣ�");
			return;
		}
		/*ql:20080717 START: must assign at least one term.*/
		if ((protoType.selectedIndex==0 && sipStart.value.length==0 && dipStart.value.length==0 &&
			sportStart.value.length==0 && dportEnd.value.length==0) &&
			(protoTypeV6.selectedIndex==0 && sip6Start.value.length==0 && dip6Start.value.length==0 &&
			sportStart.value.length==0 && dportEnd.value.length==0))
		{
			alert("���趨���˹���!");
			return;
		}
		/*ql:20080717 END*/

		var sif = "";
		for(var i in links)
		{
			if(!links[i].select)continue;
			if(sif.length != 0) sif += ";" + links[i].displayname();
			else sif += links[i].displayname();
		}

		ifname.value = sif;

		if ( <%checkWrite("IPv6Show");%> ) {
			if(document.forms[0].IpProtocolType.value == 0) {
				alert("��ָ��IPЭ��汾��");
				return;
			}

			//If this is IPv6 rule.
			if(document.forms[0].IpProtocolType.value == 2){
				if(sip6Start.value != ""){
					if (! isGlobalIpv6Address(sip6Start.value) ){
						alert("��Ч��ԴIPv6��ʼ��ַ!"); //Invalid Source IPv6 Start address!
						return;
					}
					if ( sip6PrefixLen.value != "" ) {
						var prefixlen= getDigit(sip6PrefixLen.value, 1);
						if (prefixlen > 128 || prefixlen <= 0) {
							alert("��Ч��ԴIPv6ǰ׺����!"); //Invalid Source IPv6 prefix length!
							return;
						}
					}
				}

				if(sip6End.value != ""){
					if (! isGlobalIpv6Address(sip6End.value) ){
						alert("��Ч��ԴIPv6������ַ!"); //Invalid Source IPv6 End address!
						return;
					}
				}

				if(dip6Start.value != ""){
					if (! isGlobalIpv6Address(dip6Start.value) ){
						alert("��Ч��Ŀ��IPv6��ʼ��ַ!"); //Invalid Destination IPv6 Start address!
						return;
					}
					if ( dip6PrefixLen.value != "" ) {
						var prefixlen= getDigit(dip6PrefixLen.value, 1);
						if (prefixlen > 128 || prefixlen <= 0) {
							alert("��Ч��Ŀ��IPv6ǰ׺����!"); //Invalid destination IPv6 prefix length!
							return;
						}
					}
				}

				if(dip6End.value != ""){
					if (! isGlobalIpv6Address(dip6End.value) ){
						alert("��Ч��Ŀ��IPv6������ַ!"); //Invalid Destination IPv6 End address!
						return;
					}
				}
			}
		}
		submit();
	}
}
</SCRIPT>
</HEAD>

<!-------------------------------------------------------------------------------------->
<!--��ҳ����-->
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onLoad="on_init();">
<blockquote>
	<DIV align="left" style="padding-left:20px; padding-top:0px">
		<form id="form" action="/boaform/admin/formPortFilterWhiteOut" method="post">
			<div align="left">
				<b>IP�����</b><br>

				<br>
				<table cellSpacing="1" cellPadding="0" border="0">
				   <tr>
					  <td width="130px">��������:</td>
					  <td><input type="text" size="22" name="filterName" style="width:150px"></td>
				   </tr>

				   <tr>
					  <td width="130px">Э��:</td>
					  <td><select name="protoType" size="1" style="width:150px">
							<option value="5" selected>&nbsp;</option>
							<option value="1">TCP/UDP</option>
							<option value="2">TCP</option>
							<option value="3">UDP</option>
							<option value="4">ICMP</option>
						 </select></td>
				   </tr>		
				
				   <tr>
					  <td width="130px">ԴIP��ַ:</td>
					  <td><input type="text" size="16" name="sipStart" style="width:150px"></td>
				   </tr>
				   <tr>
					  <td width="130px">����ԴIP��ַ:</td>
					  <td><input type="text" size="16" name="sipEnd" style="width:150px"></td>
				   </tr>
				   <tr>
					  <td width="130px">Դ�˿�:</td>
					  <td><input type="text" size="6" name="sportStart" style="width:150px"></td>
				   </tr>
   				   <tr>
					  <td width="130px">����Դ�˿�:</td>
					  <td><input type="text" size="6" name="sportEnd" style="width:150px"></td>
				   </tr>
   				   <tr>
					  <td width="130px">Ŀ��IP��ַ:</td>
					  <td><input type="text" size="16" name="dipStart" style="width:150px"></td>
				   </tr>
   				   <tr>
					  <td width="130px">����Ŀ��IP��ַ:</td>
					  <td><input type="text" size="16" name="dipEnd" style="width:150px"></td>
				   </tr>
				   <tr>
					  <td width="130px">Ŀ�Ķ˿�:</td>
					  <td><input type="text" size="6" name="dportStart" style="width:150px"></td>
				   </tr>
				   <tr>
					  <td width="130px">����Ŀ�Ķ˿�:</td>
					  <td><input type="text" size="6" name="dportEnd" style="width:150px"></td>
				   </tr>
				</table>
				</div>


			<hr align="left" class="sep" size="1" width="90%">
			<INPUT type="button" class="btnsaveup" value="ȷ��" onClick="on_submit();">
			<input type="reset" onclick="refresh()" class="BtnCnl" value="ȡ��">
			<input type="hidden" name="action" value="ad">
			<input type="hidden" name="portnum" value="0">
			<input type="hidden" name="ifname" value="">
			<input type="hidden" name="IpProtocolType" value="">
			<input type="hidden" name="protoTypeV6" value="">
			<input type="hidden" name="sip6Start" value="">
			<input type="hidden" name="sip6End" value="">
			<input type="hidden" name="sip6PrefixLen" value="">
			<input type="hidden" name="dip6Start" value="">
			<input type="hidden" name="dip6End" value="">
			<input type="hidden" name="dip6PrefixLen" value="">
			<input type="hidden" name="smask" value="">
			<input type="hidden" name="dmask" value="">
			<input type="hidden" name="sip6PrefixLen" value="">
			<input type="hidden" name="dip6PrefixLen" value="">
			<input type="hidden" name="submit-url" value="/secu_portfilter_cfg.asp">
		</form>
	</DIV>
	</blockquote>
</body>
<%addHttpNoCache();%>
</html>