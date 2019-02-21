<%SendWebHeadStr(); %>
<TITLE>IP QoS <% multilang(LANG_TRAFFIC_SHAPING); %></TITLE>
<SCRIPT language="javascript" type="text/javascript">

var protos = new Array("", "TCP", "UDP", "ICMP", "TCP/UDP", "RTP");
<!--var protos = new Array("", "ICMP", "TCP", "UDP", "TCP/UDP", "RTP");-->
var traffictlRules = new Array();
var totalBandwidth = 1000;
var totalBandWidthEn = 0;
<% initTraffictlPage(); %>

function on_chkdel(index) {
	if(index<0 || index>=traffictlRules.length)
		return;
	traffictlRules[index].select = !traffictlRules[index].select;
}

/********************************************************************
**          on document load
********************************************************************/
function on_init_page(){
	if (totalBandWidthEn == 0) {//to apply
		if (document.getElementById)  // DOM3 = IE5, NS6
		{
			document.getElementById('apply').style.display = 'block';
			document.getElementById('cancel').style.display = 'none';
		} else {
			document.all.apply.style.display = 'block';
			document.all.cancel.style.display = 'none';
		}
	} else {//to cancel
		if (document.getElementById)  // DOM3 = IE5, NS6
		{
			document.getElementById('apply').style.display = 'none';
			document.getElementById('cancel').style.display = 'block';
		} else {
			document.all.apply.style.display = 'none';
			document.all.cancel.style.display = 'block';
		}
	}
	
	with(document.forms[0]) {
		totalbandwidth.value = totalBandwidth;
		if(traffictl_tbl.rows){
			while(traffictl_tbl.rows.length > 1) 
				traffictl_tbl.deleteRow(1);
		}
	
		for(var i = 0; i < traffictlRules.length; i++)
		{
			var row = traffictl_tbl.insertRow(i + 1);
		
			row.nowrap = true;
			row.vAlign = "center";
			row.align = "left";

			var cell_index=0;
			var cell = row.insertCell(cell_index++);
			cell.innerHTML = traffictlRules[i].id;
			
			cell = row.insertCell(cell_index++);
			cell.innerHTML = protos[traffictlRules[i].proto];
			cell = row.insertCell(cell_index++);
			if (traffictlRules[i].sport == "0")
				cell.innerHTML = "<br>";
			else
				cell.innerHTML = traffictlRules[i].sport;
			cell = row.insertCell(cell_index++);
			if (traffictlRules[i].dport == "0")
				cell.innerHTML = "<br>";
			else
				cell.innerHTML = traffictlRules[i].dport;
		
			cell = row.insertCell(cell_index++);
			// For IPv4 and IPv6 
			if ( <% checkWrite("IPv6Show"); %> ) {		
				// For IPv4
				if ( traffictlRules[i].IpProtocolType == "1 ") {
					if (traffictlRules[i].srcip == "0.0.0.0")
						cell.innerHTML = "<br>";
					else
						cell.innerHTML = traffictlRules[i].srcip;
				}
				// For IPv6
				else if ( traffictlRules[i].IpProtocolType == "2" ) {
					if (traffictlRules[i].sip6 == "::")
						cell.innerHTML = "<br>";
					else {							
						cell.innerHTML = traffictlRules[i].sip6;											
					}
				}				
			}
			// For IPv4
			else {			
				if (traffictlRules[i].srcip == "0.0.0.0")
					cell.innerHTML = "<br>";
				else
					cell.innerHTML = traffictlRules[i].srcip;				
			}
			
			cell = row.insertCell(cell_index++);				
			// For IPv4 and IPv6 
			if ( <% checkWrite("IPv6Show"); %> ) {		
				// For IPv4
				if ( traffictlRules[i].IpProtocolType == "1 ") {
					if (traffictlRules[i].dstip == "0.0.0.0")
						cell.innerHTML = "<br>";
					else
						cell.innerHTML = traffictlRules[i].dstip;
				}
				// For IPv6
				else if ( traffictlRules[i].IpProtocolType == "2" ) {
					if (traffictlRules[i].dip6 == "::")
						cell.innerHTML = "<br>";
					else {							
						cell.innerHTML = traffictlRules[i].dip6;											
					}
				}				
			}
			// For IPv4
			else {			
				if (traffictlRules[i].dstip == "0.0.0.0")
					cell.innerHTML = "<br>";
				else
					cell.innerHTML = traffictlRules[i].dstip;				
			}
			
			if ( <% checkWrite("TrafficShapingByVid"); %> )
			{
			    cell = row.insertCell(cell_index++);
				if( traffictlRules[i].vlanID!=0)
					cell.innerHTML = traffictlRules[i].vlanID;
			}			
			
			if ( <% checkWrite("TrafficShapingBySsid"); %> )
			{
			    cell = row.insertCell(cell_index++);
				cell.innerHTML = traffictlRules[i].ssid;
			}			
			
			
			cell = row.insertCell(cell_index++);
			cell.innerHTML = traffictlRules[i].rate;
			cell = row.insertCell(cell_index++);
			cell.align = "center";
			cell.innerHTML = "<input type=\"checkbox\" onClick=\"on_chkdel(" + i + ");\">";
			
			cell = row.insertCell(cell_index++);
			if ( <% checkWrite("IPv6Show"); %> ) {			
				if (traffictlRules[i].IpProtocolType == "1")
					cell.innerHTML = "IPv4";
				else if (traffictlRules[i].IpProtocolType == "2")
					cell.innerHTML = "IPv6";
			}
			else {				
				cell.innerHTML = "IPv4";		
			}			
			
			//direction
			cell = row.insertCell(cell_index++);	
			if (traffictlRules[i].direction =="0")
				cell.innerHTML = "UPStream";
			else
				cell.innerHTML = "DownStream";			
			if ( <% checkWrite("LUNAShow"); %> )
				; //If is LUNA platform, not show WAN Interface field.
			else{
				cell = row.insertCell(cell_index++);
				cell.innerHTML = traffictlRules[i].inf;
			}
		} // for
	} // with
}


function on_apply_bandwidth(){
	
	with(document.forms[0]) {
		var sbmtstr = "";
		var bandwidth = -1;
		bandwidth = parseInt(totalbandwidth.value);
		if(bandwidth<0 || bandwidth >Number.MAX_VALUE)
			return;
		sbmtstr = "applybandwidth&bandwidth="+bandwidth;
		lst.value = sbmtstr;
		postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
		submit();
	}
}

function on_cancel_bandwidth(){
	with(document.forms[0]) {
		var sbmtstr = "";
		sbmtstr = "cancelbandwidth";
		lst.value = sbmtstr;
		postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
		submit();
	}
}

function on_submit(){
	var sbmtstr = "applysetting#id=";
	var firstFound = true;
	for(var i=0; i<traffictlRules.length; i++)
	{
		if(traffictlRules[i].select)
		{
			if(!firstFound)
				sbmtstr += "|";
			else
				firstFound = false;
			sbmtstr += traffictlRules[i].id;
		}
	}
	document.forms[0].lst.value = sbmtstr;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	document.forms[0].submit();
}
</SCRIPT>
</head>

<body onLoad="on_init_page();">
<div class="intro_main ">
	<p class="intro_title">IP QoS <% multilang(LANG_TRAFFIC_SHAPING); %></p>
</div>

<form id="form" action="/boaform/admin/formQosTraffictl" method="post">
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th><% multilang(LANG_TOTAL_BANDWIDTH_LIMIT); %>:</th>
			<td>
				<input type="text" id="totalbandwidth" value="1005"><% multilang(LANG_KB); %>
			</td>
		<tr>
	</table>
</div>

<div class="data_vertical data_common_notitle">
	 <div class="data_common ">
		<table id="traffictl_tbl" >
			   <tr>
					<th><% multilang(LANG_ID); %></th>
					<th><% multilang(LANG_PROTOCOL); %></th>
					<th><% multilang(LANG_SOURCE_PORT); %></th>
					<th><% multilang(LANG_DESTINATION_PORT); %></th>
					<th><% multilang(LANG_SOURCE); %> IP</th>
					<th><% multilang(LANG_DESTINATION); %> IP</th>
					<th style="display:<% check_display("vlanID"); %>"><% multilang(LANG_VLAN_ID); %></th>						
					<th style="display:<% check_display("ssid"); %>"><% multilang(LANG_SSID); %></th>						
					<th><% multilang(LANG_RATE); %>(kb/s)</th>
					<th><% multilang(LANG_DELETE); %></th>
					<th>IP <% multilang(LANG_VERSION); %></th>
	                <th><% multilang(LANG_DIRECTION); %></th>
					<th><div id='wan_interface'  style="display:<% check_display("wan_interface"); %>" ><% multilang(LANG_WAN_INTERFACE); %></div></th>
				</tr>
		</table>
	</div>
</div>
<div class="btn_ctl">
	<table>
		<tr>
			<td><input class="link_bg" type="button" onClick="location.href='net_qos_traffictl_edit.asp';" value="<% multilang(LANG_ADD); %>"></td>
			<td><input class="link_bg" type="button" onClick="on_submit();" value="<% multilang(LANG_APPLY_CHANGES); %>"></td>
			<td>
				<div  id="apply"  style="display:none">
					<input class="link_bg" type="button" onClick="on_apply_bandwidth();" value="<% multilang(LANG_APPLY_TOTAL_BANDWIDTH_LIMIT); %>">
				</div>
				<div  id="cancel" style="display:none">
					<input class="link_bg" type="button" onClick="on_cancel_bandwidth();" value="<% multilang(LANG_CANCEL_TOTAL_BANDWIDTH_LIMIT); %>">
				</div>
			</td>
		</tr>
	</table>
</div>
<input type="hidden" id="lst" name="lst" value="">
<input type="hidden" name="submit-url" value="/net_qos_traffictl.asp">
<input type="hidden" name="postSecurityFlag" value="">
</form>
</body>
</html>
