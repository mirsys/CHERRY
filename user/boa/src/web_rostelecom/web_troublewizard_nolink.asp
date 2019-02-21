﻿<VWS_FUNCTION (void*)SendWebMetaStr();>
<VWS_FUNCTION (void*)SendWebCssStr();>
<title>Html Wizard</title>

<SCRIPT>
#if defined(CONFIG_ADSLUP) && defined(CONFIG_MULTI_ETHUP)
	<VWS_FUNCTION (void*) vmsGetPhytype(); >
#elif defined(CONFIG_ADSLUP)
	var wanphytype = 0;
#else
	var wanphytype = 1;
#endif
</SCRIPT>
</head>

<body>
	<div class="data_common data_common_notitle">
		<table>
			<tr class="data_prompt_info">
				<th colspan="2" style="color:red; font-size:20px;">
<script>
#if defined(CONFIG_MULTI_ETHUP)
				if(wanphytype == 1)
					document.write("<% multilang(LANG_ETHERNET_LINE_IS_NOT_DETECTED); %>");
#endif
#if defined(CONFIG_ADSLUP) && defined(CONFIG_MULTI_ETHUP)
				else
#endif
#if defined(CONFIG_ADSLUP)
				if(wanphytype == 0)
					document.write("<% multilang(LANG_ADSL_LINE_IS_NOT_DETECTED); %>");
#endif
</script>
				</th>
			</tr>
			<tr>
				<td colspan="2" class="data_prompt_td_info">
<script>
#if defined(CONFIG_MULTI_ETHUP)
				if(wanphytype == 1)
					document.write("<% multilang(LANG_PLEASE_CHECK_THE_CABLE_CONNECTION_ETHERNET_AS_SHOWN_BELOW_IF_THE_PROBLEM_PERSISTS_CONTACT_TECHNICAL_SUPPORT_OF_JSC_ROSTELECOM); %>");
#endif
#if defined(CONFIG_ADSLUP) && defined(CONFIG_MULTI_ETHUP)
				else
#endif
#if defined(CONFIG_ADSLUP)
				if(wanphytype == 0)
					document.write("<% multilang(LANG_PLEASE_CHECK_THE_CABLE_CONNECTION_ADSL_AS_SHOWN_BELOW_IF_THE_PROBLEM_PERSISTS_CONTACT_TECHNICAL_SUPPORT_OF_JSC_ROSTELECOM); %>");
#endif
</script>
				</th>
			</tr>
		</table>
		<table>
			<tr>
				<th width="15%"><% multilang(LANG_DEVICE_TYPE); %></th>
				<td>
				<VWS_SCREEN (char*)xscrnRoseModelName[];>
				</td>
				<td rowspan="5" style="text-align:center;">
#ifdef CONFIG_VENDOR_TPLINK				
				    <VWS_FUNCTION (void*)getAdslNoConnJpg();>
#else
				    <VWS_FUNCTION (void*)getAdslConnJpg();>
#endif
				</td>
			</tr>
			<tr>
				<th><% multilang(LANG_FIRMWARE_VERSION); %></th>
				<td>
				<VWS_SCREEN (char*)xscrnHwVersion[];>
				</td>
			</tr>
			<tr>
				<th>Software version</th>
				<td>
				<VWS_SCREEN (char*)xscrnAppVersion[];>
				</td>
			</tr>
			<tr>
				<th><% multilang(LANG_MAC_ADDRESS); %></th>
				<td>
#ifdef CONFIG_RESERVE_DEFAULT_MAC
				<VWS_SCREEN (char*)xscrnRoseReserveMAC[];>
#else
				<VWS_SCREEN (char*)xscrnRoseMAC[];>
#endif
				</td>
			</tr>
			<tr>
				<th><% multilang(LANG_SERIAL_NUMBER); %></th>
				<td>
				<VWS_SCREEN (char*)xscrnRoseSerial[];>
				</td>
			</tr>
		</table>
	</div>
	<br>
#ifdef CONFIG_VENDOR_TPLINK
	<form action="form2RoseTroubleWizardNoLink.cgi" method=POST name="RoseTroubleWizardNoLink">
	<div class="adsl clearfix btn_center">
		<input class="link_bg" type="submit" name="next" value="Continue">
		<input class="link_bg" type="button" name="close" value="Manual configuration" onClick="top.location.href='index.htm';" >
		<input type="hidden" value="Send" name="submit.htm?rose_troublewizard_nolink.htm">
	</div>
	</form>
#else
		<input class="link_bg" type="button" name="close" value="Manual configuration" onClick="top.location.href='index.htm';" >
#endif
</body>

</html>
