var configKeys = ["DATE_FORMAT"];

Pebble.addEventListener("ready",
  function(e) {
  	//console.log("Yes, it's running!!!");
	SendDateFormat();
  }
);

Pebble.addEventListener("showConfiguration",
  function(e) {
	var url = 'http://59dub.leighmurray.com/settings.htm';
	//url += GetJSONConfig();
	//url = encodeURI(url);
	//console.log(url);
	Pebble.openURL(url);
  }
);


Pebble.addEventListener("webviewclosed",
  function(e) {
  	var stopStr = "";
    //console.log("Configuration window returned: " + e.response);
    if (!e.response) {
      return;
    }

    var configuration = JSON.parse(e.response);

    for (var i = 0; i < configuration.length; i++) {
      var returnedObject = configuration[i];
		SetConfig(returnedObject.name, returnedObject.value);
      }
    SendDateFormat();
  }
);

function SendDateFormat () {
	var format = GetConfig("DATE_FORMAT");
	if (format !== null) {
		SendAppMessage("set_date_format", Number(format));
	} else {
		console.log("no date format found");
	}
}

function SendAppMessage (header, content) {
	// TODO! Make headers/functions into int (use config for names)
	// so more space available for content
	var maxLength = 47;
	maxLength -= header.length;
	if (content.length > maxLength) {
		console.log("Warning! SendAppMessage trimming content to " + maxLength + ". Orig length:" + content.length);
		content = content.substr(0, maxLength);
	}
	console.log("Sending Header: " + header + " with content: " + content);
	var transactionId = Pebble.sendAppMessage(
    	{
    		"0":header,
    		"1":content
    	},
  		function(e) {
    		console.log("Successfully delivered message with transactionId=" + e.data.transactionId);
    	},
  		function(e) {
    		console.log("Unable to deliver message with transactionId=" + e.data.transactionId + " Error is: " + e.message);
  		}
	);
}

function GetConfig (key) {
	return window.localStorage.getItem(key);
}

function SetConfig (key, value) {
	console.log("Setting Key:" + key + " - Value:" + value);
	window.localStorage.setItem(key, value);
}

function GetJSONConfig () {

	var tempConfig = {};
	for (var i=0; i<configKeys.length; i++) {
		var configKey = configKeys[i];
		tempConfig[configKey] = GetConfig(configKey);
	}
	var jsonConfig = JSON.stringify(tempConfig);
	return jsonConfig;
}
