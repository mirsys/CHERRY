﻿<VWS_FUNCTION (void*)SendWebMetaStr();>
<VWS_FUNCTION (void*)SendWebCssStr();>
<title>Html Wizard</title>

<SCRIPT>

var wtime=6;
var id;
function stop()
{
	clearTimeout(id); 
}

function start() 
{ 
	wtime--;
	if (wtime >= 0)
	{ 
		id=setTimeout("start()",1000);
	}
	if (wtime == 0)
	{ 
		document.RoseHijackWizardWait1.submit();
	}
}
</SCRIPT>
</head>

<body onload="start();" onunload="stop();">
<form action="form2RoseHijackWizardWait1.cgi" method=POST name="RoseHijackWizardWait1">
	<div class="data_common data_common_notitle">
		<table>
			<tr class="data_wait_info">
				<td colspan="2" style="padding: 25px;">
				<% multilang(LANG_ASE_WAIT_YOU_ARE_TRYING_TO_ESTABLISH_A_CONNECTION_TO_THE_INTERNET); %>
				<br>
				<% multilang(LANG_PROCEDURE_TAKES_NO_MORE_THAN_12_MINUTES); %>
				</td>
			</tr>
		</table>
	</div>
	<input type="hidden" value="Send" name="submit.htm?rose_hijackwizard_wait1.htm">
</form>

</body>

</html>

