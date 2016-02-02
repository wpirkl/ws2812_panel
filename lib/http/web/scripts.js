
function updateContent() {
	var jqxhr = $.getJSON( "status.json", function( data ) {
		$.each( data, function( key, val ) {
			$("#" + key).html(val);
		});
		setTimeout(updateContent, 1000);
	})
}

function ledChangeParameters() {
	$("#params").slideUp("fast", function() {
		$("#params").empty();
		switch($("#animation").val()) {
			case "0":
				$("#params").append(
					"<div class=\"form-group\">"+
						"<label class=\"control-label col-sm-3\" for=\"color\">Color:</label>"+
						"<div class=\"col-sm-9\">"+
							"<input id=\"color\" class=\"form-control\" type=\"color\" name=\"color\" value=\"#ffffff\">"+
						"</div>"+
					"</div>");
				break;
			case "1":
				$("#params").append(
					"<div class=\"form-group\">"+
						"<label class=\"control-label col-sm-3\" for=\"res1\">Reserved:</label>"+
						"<div class=\"col-sm-9\">"+
							"<input id=\"res1\" class=\"form-control\" type=\"text\" name=\"res1\" placeholder=\"Reserved\">"+
						"</div>"+
					"</div>");
				break;
		}
		$("#params").slideDown("fast");
	});
}

function statusOnLoad() {
	setTimeout(updateContent, 2000);
}

function ledOnLoad() {
	$("#animation").change(ledChangeParameters);
	ledChangeParameters();
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
