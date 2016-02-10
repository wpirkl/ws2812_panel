
function updateContent() {
	var jqxhr = $.getJSON( "status.json", function( data ) {
		$.each( data, function( key, val ) {
			$("#" + key).html(val);
		});
		setTimeout(updateContent, 1000);
	})
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
							"<input id=\"trtime\" class=\"form-control\" type=\"number\" name=\"trtime\" value=\"25\">"+
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

function indexOnLoad() {
	$("#status").load("status.html", statusOnLoad);
	$("#led").load("led.html", ledOnLoad);
	$("#wifi").load("wifi.html", wifiOnLoad);
	$("#mqtt").load("mqtt.html", mqttOnLoad);
}
