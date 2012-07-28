var event_input = "input";
var event_kill = "kill";
var event_exec = "exec";
var event_exit = "exit";
var event_output = "output";
var stream_stdout = "stdout";
var stream_stderr = "stderr";

var ticket_counter = 0;

function DefaultVal(val, def) {
	return (val ? val : def);
}

/* BrowserDetect came from http://www.quirksmode.org/js/detect.html */

var BrowserDetect = {
	init: function () {
		this.browser = this.searchString(this.dataBrowser) || "An unknown browser";
		this.version = this.searchVersion(navigator.userAgent)
			|| this.searchVersion(navigator.appVersion)
			|| "an unknown version";
		this.OS = this.searchString(this.dataOS) || "an unknown OS";
	},
	searchString: function (data) {
		for (var i = 0; i < data.length; i++) {
			var dataString = data[i].string;
			var dataProp = data[i].prop;
			this.versionSearchString = data[i].versionSearch || data[i].identity;
			if (dataString) {
				if (dataString.indexOf(data[i].subString) != -1)
					return data[i].identity;
			}
			else if (dataProp)
				return data[i].identity;
		}
	},
	searchVersion: function (dataString) {
		var index = dataString.indexOf(this.versionSearchString);
		if (index == -1) return;
		return parseFloat(dataString.substring(index+this.versionSearchString.length+1));
	},
	dataBrowser: [
		{
			string: navigator.userAgent,
			subString: "Chrome",
			identity: "Chrome"
		},
		{ 	string: navigator.userAgent,
			subString: "OmniWeb",
			versionSearch: "OmniWeb/",
			identity: "OmniWeb"
		},
		{
			string: navigator.vendor,
			subString: "Apple",
			identity: "Safari",
			versionSearch: "Version"
		},
		{
			prop: window.opera,
			identity: "Opera",
			versionSearch: "Version"
		},
		{
			string: navigator.vendor,
			subString: "iCab",
			identity: "iCab"
		},
		{
			string: navigator.vendor,
			subString: "KDE",
			identity: "Konqueror"
		},
		{
			string: navigator.userAgent,
			subString: "Firefox",
			identity: "Firefox"
		},
		{
			string: navigator.vendor,
			subString: "Camino",
			identity: "Camino"
		},
		{		// for newer Netscapes (6+)
			string: navigator.userAgent,
			subString: "Netscape",
			identity: "Netscape"
		},
		{
			string: navigator.userAgent,
			subString: "MSIE",
			identity: "Explorer",
			versionSearch: "MSIE"
		},
		{
			string: navigator.userAgent,
			subString: "Gecko",
			identity: "Mozilla",
			versionSearch: "rv"
		},
		{	// for older Netscapes (4-)
			string: navigator.userAgent,
			subString: "Mozilla",
			identity: "Netscape",
			versionSearch: "Mozilla"
		}
	],
	dataOS : [
		{
			string: navigator.platform,
			subString: "Win",
			identity: "Windows"
		},
		{
			string: navigator.platform,
			subString: "Mac",
			identity: "Mac"
		},
		{
			string: navigator.userAgent,
			subString: "iPhone",
			identity: "iPhone/iPod"
		},
		{
			string: navigator.platform,
			subString: "Linux",
			identity: "Linux"
		}
	]
};
BrowserDetect.init();

function get_appropriate_ws_url()
{
	var pcol;
	var u = document.URL;

	/*
	 * We open the websocket encrypted if this page came on an
	 * https:// url itself, otherwise unencrypted
	 */

	if (u.substring(0, 5) == "https") {
		pcol = "wss://";
		u = u.substr(8);
	} else {
		pcol = "ws://";
		if (u.substring(0, 4) == "http")
			u = u.substr(7);
	}

	u = u.split('/');

	return pcol + u[0];
}

/* webarg-api protocol */

function WebargProcess(path, args, onMessage, onInterrupt, onError, quitSignal, killTimeout) {
	var socket;
	var callbacks = [];

	this.pid = 0;
	this.finished = false;
	this.exit_code = 0;

	var ws_url = get_appropriate_ws_url();
	var protocol_name = "webarg-api";

	if (BrowserDetect.browser == "Firefox")
		socket = new MozWebSocket(ws_url, protocol_name);
	else
		socket = new WebSocket(ws_url, protocol_name);

	var sendMessage = function(body, attachment) {
		var str = JSON.stringify(body) + "\0" + DefaultVal(attachment, "");
		socket.send(str);
	}

	this.input = function(text, callback) {
		var body = {
			event: event_input,
			ticket: ++ticket_counter
		};
		if (callback)
			callbacks[ticket_counter] = callback;
		sendMessage(body, text);
		return ticket_counter;
	}

	this.kill = function(signal, callback) {
		var body = {
			event: event_kill,
			params: {
				signal: signal,
			},
			ticket: ++ticket_counter
		};
		if (callback)
			callbacks[ticket_counter] = callback;
		sendMessage(body);
		return ticket_counter;
	}

	socket.onopen = function() {
		var body = {
			event: event_exec,
			params: {
				path: path,
				args: args,
				quit_signal: DefaultVal(quitSignal, 0),
				kill_timeout: DefaultVal(killTimeout, 0)
			}
		};
		sendMessage(body);
	}

	socket.onclose = function() {
		if (!this.finished && onInterrupt)
			onInterrupt();
	}

	socket.onerror = function(evt) {
		if (onError)
			onError(evt);
	}

	socket.onmessage = function(msg) {
		var str = msg.data;
		var bodyStr = str;
		var attachment = "";
		for (var i = 0; i < str.length; i++) {
			if (str.charCodeAt(i) == 0) {
				bodyStr = str.substring(0, i);
				attachment = str.substring(i + 1);
				break;
			}
		}

		var body;
		var callback;

		try {
			body = JSON.parse(bodyStr);

			var event = body.event;
			var params = body.params;
			if (event == event_exec)
				this.pid = params.pid;
			else if (event == "exit") {
				this.finished = true;
				this.exit_code = params.code;
				socket.close();
			}
		
			var ticket = body.ticket;
			if (ticket in callbacks) {
				callback = callbacks[ticket];
				delete callbacks[ticket];
			}
		} catch (e) {
		}

		if (callback) {
			callback(body, attachment);
		}
		else if (onMessage)
			onMessage(body, attachment);
	}
}

