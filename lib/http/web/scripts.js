function updateContent() {
	var jqxhr = $.getJSON( "status.json", function( data ) {
		$.each( data, function( key, val ) {
			$("#" + key).html(val);
		});
	})
	jqxhr.always(function() {
		setTimeout(updateContent, 1000);
	});
}
function ledChangeTransitionParameters() {
	$("#trpar").slideUp("fast", function() {
		$("#trpar").empty();
		switch($("#tra").val()) {
			case "0":
				$("#trpar").append(
					"<div class=\"form-group\">"+
						"<label class=\"control-label col-sm-3\" for=\"trtime\">Transition time:</label>"+
						"<div class=\"col-sm-9\">"+
							"<input id=\"trtime\" class=\"form-control\" type=\"number\" name=\"trtime\" value=\"1000\">"+
						"</div>"+
					"</div>");
				break;
			case "1":
				$("#trpar").append(
					"<div class=\"form-group\">"+
						"<label class=\"control-label col-sm-3\" for=\"trres1\">Reserved:</label>"+
						"<div class=\"col-sm-9\">"+
							"<input id=\"trres1\" class=\"form-control\" type=\"text\" name=\"trres1\" placeholder=\"Reserved\">"+
						"</div>"+
					"</div>");
				break;
		}
		$("#trpar").slideDown("fast");
	});
}
function ledChangeAnimationParameters() {
	$("#anipar").slideUp("fast", function() {
		$("#anipar").empty();
		switch($("#ani").val()) {
			case "0":
				$("#anipar").append(
					"<div class=\"form-group\">"+
						"<label class=\"control-label col-sm-3\" for=\"ancol\">Color:</label>"+
						"<div class=\"col-sm-9\">"+
							"<input id=\"ancol\" class=\"form-control\" type=\"color\" name=\"ancol\" value=\"#ffffff\">"+
						"</div>"+
					"</div>");
				break;
			case "1":
				$("#anipar").append(
					"<div class=\"form-group\">"+
						"<label class=\"control-label col-sm-3\" for=\"ancol\">First Color:</label>"+
						"<div class=\"col-sm-9\">"+
							"<input id=\"ancol\" class=\"form-control\" type=\"color\" name=\"ancol\" value=\"#ffd555\">"+
						"</div>"+
					"</div>"+
					"<div class=\"form-group\">"+
						"<label class=\"control-label col-sm-3\" for=\"ancol1\">Second Color:</label>"+
						"<div class=\"col-sm-9\">"+
							"<input id=\"ancol1\" class=\"form-control\" type=\"color\" name=\"ancol1\" value=\"#ffffff\">"+
						"</div>"+
					"</div>"+
					"<div class=\"form-group\">"+
						"<label class=\"control-label col-sm-3\" for=\"angle\">Angle:</label>"+
						"<div class=\"col-sm-9\">"+
							"<input id=\"angle\" class=\"form-control\" type=\"number\" name=\"angle\" min=\"0\" max=\"359\" value=\"90\">"+
						"</div>"+
					"</div>");
				break;
			case "2":
				$("#anipar").append(
					"<div class=\"form-group\">"+
						"<label class=\"control-label col-sm-3\" for=\"anpal\">Palette:</label>"+
						"<div class=\"col-sm-9\">"+
							"<select class=\"form-control\" id=\"anpal\" name=\"anpal\" value=\"#ffd555\">"+
								"<option value=\"0\">Rainbow</option>"+
								"<option value=\"1\">Sky</option>"+
								"<option value=\"2\">Lava</option>"+
								"<option value=\"3\">Heat</option>"+
								"<option value=\"4\">Ocean</option>"+
								"<option value=\"5\">Forest</option>"+
							"</select>"+
						"</div>"+
					"</div>");
				break;
			case "3":
				$("#anipar").append(
					"<div class=\"form-group\">"+
						"<label class=\"control-label col-sm-3\" for=\"anres1\">Reserved:</label>"+
						"<div class=\"col-sm-9\">"+
							"<input id=\"anres1\" class=\"form-control\" type=\"text\" name=\"anres1\" placeholder=\"Reserved\">"+
						"</div>"+
					"</div>");
				break;
		}
		$("#anipar").slideDown("fast");
	});
}
function statusOnLoad() {
	setTimeout(updateContent, 2000);
}
function ledOnLoad() {
	$("#tra").change(ledChangeTransitionParameters);
	ledChangeTransitionParameters();
	$("#ani").change(ledChangeAnimationParameters);
	ledChangeAnimationParameters();
}
function wifiOnLoad() {
	
}
function mqttOnLoad() {
	
}
function loadStatus() {
	$("#status").load("status.html", function(response, status, xhr) {
		if(status == "error") {
			loadStatus();
		} else {
			statusOnLoad();
		}
	});
}
function loadLed() {
	$("#led").load("led.html", function(response, status, xhr) {
		if(status == "error") {
			loadLed();
		} else {
			ledOnLoad();
		}
	});
}
function loadWifi() {
	$("#wifi").load("wifi.html", function(response, status, xhr) {
		if(status == "error") {
			loadWifi();
		} else {
			wifiOnLoad();
		}
	});
}
function loadMqtt() {
	$("#mqtt").load("mqtt.html", function(response, status, xhr) {
		if(status == "error") {
			loadMqtt();
		} else {
			mqttOnLoad();
		}
	});
}
function indexOnLoad() {
	loadStatus();
	loadLed();
	loadWifi();
	loadMqtt();
}
