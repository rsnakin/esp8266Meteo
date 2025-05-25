var __VERSION = '1.21';

var LOADING = false;
var HOME_MODE = true;
var LOGS_MODE = false;
var SPIFFS_MODE = false;

var loadInterval = 1000;
var homeInterval;
var logsInterval;
var spiffsInterval;
var currentLOG = '/system.log';

var dialogElem;
var closeDialorButton;

document.addEventListener("DOMContentLoaded", ready);

var  loadLevels = {
	1: {
			url:'/bs.010.css',
			rel:'stylesheet',
			type:'text/css'
	},
	2: {
			url:'/bs.025.css',
			rel:'stylesheet',
			type:'text/css'
	},
	3:{
			url:'/u.01.css',
			rel:'stylesheet',
			type:'text/css'
	},
	4:{
			url:'/u.02.css',
			rel:'stylesheet',
			type:'text/css'
	},
	5:{
			url:'/styles.css',
			rel:'stylesheet',
			type:'text/css'
	},
	6:{
			url:'/icon.png',
			rel:'icon',
			type:'image/png'
	},
	7:{
		type:'startMain'
	}
};
var loadLevel = 1;
var loadProgress;
function ready() {
	loadProgress = parseInt( loadLevel * 100 / 7);
	let loadEnd = 100 - loadProgress;
	let simPr = '%';
	if(loadProgress == 100) {
		simPr = '';
		loadEnd = '';
	}
	if(loadLevel > 1) {
		document.body.innerHTML = '<table width="100%" cellspacing="0" cellpadding="0"><tr style="border:1px solid#000077;"><td width="' + loadProgress 
		+ '%" style="background:#00aaff;text-align:center"><b>' + loadProgress + '%</b></td>'
		+ '<td width="' + loadEnd + '%" style="background:#ccc;text-align:center"><b>' + loadEnd + simPr + '</b></td></tr>'
		+ '<tr><td colspan="2" style="text-align:center"><b>esp8266MeteoWebApp V:' + __VERSION + '</b></tr><table>';
		loadProgress = '&#9606;&#9606;&#9606' + loadProgress;
	}
	if(loadLevels[loadLevel]['type'] == 'startMain') {
		ajax.get('/index_body.html?123', {}, function(data) {
			document.body.innerHTML = data;
			main();
		});
		return(true);
	}
	setTimeout(function(){
		__loadData(loadLevels[loadLevel]['url'], loadLevel, loadLevels[loadLevel]['rel'], loadLevels[loadLevel]['type']);
		loadLevel ++;
		ready();
	}, loadInterval);
}
function main() {
	dialogElem = document.getElementById('dialog');
	closeDialorButton = document.getElementById('closeDialoButton');
	closeDialorButton.addEventListener("click", () => {
		dialogElem.close();
	});
	let aboutDialog = document.getElementById('__about');
	aboutDialog.addEventListener("click", () => {
		showDialog('esp8266MeteoWebApp V:' + __VERSION);
	});
	let contentDIV = document.getElementById('content');
	let tdHeader = document.getElementById('tdHeader');
	let contentDIVHeight = window.innerHeight - tdHeader.offsetHeight - 2;
	contentDIVStyle = 'overflow:auto;height:' + contentDIVHeight + 'px;';
	contentDIV.setAttribute("style", contentDIVStyle);
	var reloadButton = document.getElementById('reload');
	reloadButton.onclick = function(){
		if(LOADING) { return(false); }
		if(HOME_MODE) {
			if(homeInterval) {
				clearInterval(homeInterval);
			}
			homeUpData();
			homeInterval = setInterval(() => {
				homeUpData();
				houseMouseOverListenerOn();
			}, 60000);
			return(true);
		}
		if(LOGS_MODE) {
			if(logsInterval) {
				clearInterval(logsInterval);
			}
			logsLoad(currentLOG, true);
			return(true);
		}
		if(SPIFFS_MODE) {
			if(spiffsInterval) {
				clearInterval(spiffsInterval);
			}
			loadSPIFFS(true);			
		}
	};
	var homeButton = document.getElementById('__home');
	homeButton.setAttribute("style", "color: rgb(255,0,0)");
	var logsButton = document.getElementById('__logs');
	var spiffsButton = document.getElementById('__spiffs');
	homeButton.onclick = function(){
		if(HOME_MODE || LOADING) { return(false); }
		HOME_MODE = true;
		LOGS_MODE = false;
		SPIFFS_MODE = false;
		logsButton.removeAttribute("style");
		spiffsButton.removeAttribute("style");
		homeButton.setAttribute("style", "color: rgb(255,0,0)");
		if(homeInterval) {
			clearInterval(homeInterval);
		}
		if(logsInterval) {
			clearInterval(logsInterval);
		}
		if(spiffsInterval) {
			clearInterval(spiffsInterval);
		}
		homeFirstLoad();
	};
	logsButton.onclick = function(){
		if(LOGS_MODE || LOADING) { return(false); }
		HOME_MODE = false;
		LOGS_MODE = true;
		SPIFFS_MODE = false;
		currentLOG = '/system.log';
		homeButton.removeAttribute("style");
		spiffsButton.removeAttribute("style");
		logsButton.setAttribute("style", "color: rgb(255,0,0)");
		if(homeInterval) {
			clearInterval(homeInterval);
		}
		if(logsInterval) {
			clearInterval(logsInterval);
		}
		if(spiffsInterval) {
			clearInterval(spiffsInterval);
		}
		logsLoad(currentLOG, true);
	};
	spiffsButton.onclick = function(){
		if(SPIFFS_MODE || LOADING) { return(false); }
		HOME_MODE = false;
		LOGS_MODE = false;
		SPIFFS_MODE = true;
		homeButton.removeAttribute("style");
		logsButton.removeAttribute("style");
		spiffsButton.setAttribute("style", "color: rgb(255,0,0)");
		if(homeInterval) {
			clearInterval(homeInterval);
		}
		if(logsInterval) {
			clearInterval(logsInterval);
		}
		if(spiffsInterval) {
			clearInterval(spiffsInterval);
		}
		loadSPIFFS(true);
	};
	homeFirstLoad();
}
function showDialog(msg) {
	document.getElementById('dialogContent').innerText = msg;
	dialogElem = document.getElementById('dialog');
	dialogElem.showModal();
}
function __loadData(dataURL, dataId, rel, type) {
	var cssId = dataId;
    var head  = document.getElementsByTagName('head')[0];
    var link  = document.createElement('link');
    link.id   = dataId;
	link.rel  = rel;
	link.type = type;
    link.href = dataURL;
    link.media = 'all';
    head.appendChild(link);
}
function logsLoad(log, startInterval) {
	loaderOn();
	let logSimbols = {
		SM:"&#8627;",
		UF:"&#10003;",
		RM:"&#10005;",
		RS:"&#9736;",
		IN:"&#128959;"
	};
    let options = {};
    if(log) {
        options = {'log': log};
        currentLOG = log;
    }
	let contentDIV = document.getElementById('content');
	let logHTML = '<table class="table"><tbody>';
	ajax.get('/logs_list', {}, function(data) {
        let obj;
		try {
			obj = JSON.parse(data);
		} catch (error) {
			console.error(error);
			showDialog(error);
		}
		if(!obj) {
			loaderOff();
			return(false);
		}
		logHTML += '<tr><td class="tdLogSelect" colspan="5"><select id="selectLog"><optgroup label="Select log:">';
		logHTML += '<option value="/system.log">/system.log</option>';
		for (i in obj) {
            if(obj[i]['END'] == 'NULL') { continue; }
            let selected = '';
            if(log == obj[i]['LN']) { selected = 'selected'; }
            logHTML += '<option value="' + obj[i]['LN'] + '" ' + selected + '>' + obj[i]['LN'] + '</option>';
        }
        logHTML += '</optgroup></select></td></tr>';
		ajax.get('/get_log', options, function(data) {
			data = "[" + data + "]";
			let obj;
			try {
				obj = JSON.parse(data);
			} catch (error) {
				console.error(error);
				showDialog(error);
			}
			if(!obj) {
				loaderOff();
				return(false);
			}
			let startTime;
			let n = 0;
			for (i in obj) {
				if(obj[i]['A'] == 'SL') {
					startTime = obj[i]['T'];
				}
				n ++;
			}
			n --;
			for (let i = n; i >= 0; i--) {
				if(obj[i]['A'] == 'SL') {
					logHTML += '<tr class="trLog"><td class="firstLogTD_SL">&#9872;</td><td class="logBody">' + timeConverter(startTime)
					+ '</td><td class="logBody" >Started log</td><td class="logBody">' + obj[i]['D']['LF']
					+ '</td><td class="logBody" >Build: ' + obj[i]['D']['B'] + '</td></tr>';
					continue;
				}
				let uTime = parseInt(startTime) + parseInt(obj[i]['T']);
				logHTML += '<tr class="trLog"><td class="firstLogTD_' + obj[i]['A'] + '">'
				+ logSimbols[obj[i]['A']] + '</td><td class="logBody">' + timeConverter(uTime) +'</td>';
				if(obj[i]['A'] == 'SM') {
					let command;
					if(obj[i]['D']['C'] == 'm') { command = '/meteo';    }
					if(obj[i]['D']['C'] == 'i') { command = '/info';     }
					if(obj[i]['D']['C'] == 'h') { command = '/help';     }
					if(obj[i]['D']['C'] == 'l') { command = '/location'; }
					logHTML += '<td class="logBody">Sent message</td>';
					logHTML += '<td class="logBody">User: <font color="#2192fc">' + obj[i]['D']['U'] + '</font> Command: <font color="#2192fc">'
					+ command +  '</font></td>';
					logHTML += '<td class="logBody">Run time: <font color="#2192fc">' + obj[i]['D']['RT'] + '</font></td>';
				}
				if(obj[i]['A'] == 'UF') {
					logHTML += '<td class="logBody">Uploaded file</td>';
					logHTML += '<td class="logBody">File: <font color="#2192fc" class="text-uppercase">/' + obj[i]['D']['FL'] + '</font></td>';
					logHTML += '<td class="logBody">Size: <font color="#2192fc">' + obj[i]['D']['SZ'] + '</font></td>';
				}
				if(obj[i]['A'] == 'RM') {
					logHTML += '<td class="logBody">Removed file</td>';
					logHTML += '<td class="logBody">File: <font color="#2192fc" class="text-uppercase">' + obj[i]['D']['FL'] + '</font></td>';
					logHTML += '<td class="logBody"></td>';
				}
				if(obj[i]['A'] == 'RS') {
					logHTML += '<td class="logBody">Readed sensor</td>';
					logHTML += '<td class="logBody">Sensor: <font color="#2192fc">' + obj[i]['D']['SN'] + '</font></td>';
					logHTML += '<td class="logBody">Run time: <font color="#2192fc">' + obj[i]['D']['RT'] + '</font></td>';
				}
				logHTML += '</tr>';
			}
			logHTML += '</tbody></table>';
			contentDIV.innerHTML = logHTML;
			let log_selector = document.getElementById("selectLog");
			log_selector.onchange="";
			log_selector.onchange = function(){
				clearInterval(logsInterval);
				logsLoad(selectLog.value, true);
			};
			if(startInterval) {
				logsInterval = setInterval(() => {
					logsLoad(log, false);
				}, 60000);
			}
			loaderOff();
		});
	});
}
function timeConverter(UNIX_timestamp){
    var a = new Date(UNIX_timestamp * 1000);
    var months = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
    var year = a.getFullYear();
    var month = months[a.getMonth()];
    var date = a.getDate();
    var hour = a.getHours();
    if(parseInt(hour) < 10) { hour = '0' + hour;}
    var min = a.getMinutes();
    if(parseInt(min) < 10) { min = '0' + min;}
    var sec = a.getSeconds();
    if(parseInt(sec) < 10) { sec = '0' + sec;}
    var time = date + ' ' + month + ' ' + year + ' ' + hour + ':' + min + ':' + sec ;
    return time;
}
function homeFirstLoad() {
    loaderOn();
	let fields = {};
	fields = {
		DS18B20_t:'Temperature DS18B20',
		BMP180_t:'Temperature BMP180 GY-68',
		DHT11_t:'Temperature DHT11',
		BMP180_p:'Pressure BMP180 GY-68',
		DHT11_h:'Humidity DHT11',
		version:'Version',
		ip:'IP',
		volts:'Voltage',
		freq:'CPU frequency',
		uptime:'Uptime',
		time:'Time',
		start_time:'Start time',
		read_time:'Sensor read time',
		read_counter:'Readig couner',
		msgs:'Messages'
	};
    ajax.get('/all', {}, function(data) {
		let obj;
		try {
			obj = JSON.parse(data);
		} catch (error) {
			console.error(error);
			showDialog(error);
			loaderOff();
			return;
		}
		if(!obj) {
			loaderOff();
			showDialog('Object not defined!');
			return(false);
		}
		let contentDIV = document.getElementById('content');
		let homeHTML = '<table class="table" style="border-bottom:0px solid #fff;"><tbody>';
        for (key in fields) {
            //element.innerHTML = obj[key];
			homeHTML += '<tr class="homeMouseOver" style="border-top:1px solid rgb(92,172,252);">'
            + '<td class="tdTitle">' + fields[key] + '&nbsp;&nbsp;&nbsp;&nbsp;:</td>'
            + '<td class="tdValue" id="' + key + '">' + obj[key] +'</td></tr>'
            loaderOff();
        }
		homeHTML += '</tbody></table>';
		contentDIV.innerHTML = homeHTML;
		houseMouseOverListenerOn();
		homeInterval = setInterval(() => {
			homeUpData();
			houseMouseOverListenerOn();
		}, 60000);
		loaderOff();
    });	
}
function homeUpData() {
    loaderOn();
    ajax.get('/all', {}, function(data) {
		let obj;
		try {
			obj = JSON.parse(data);
		} catch (error) {
			console.error(error);
			showDialog(error);
			loaderOff();
			return(false);
		}
		if(!obj) {
			loaderOff();
			showDialog('Object not defined!');
			return(false);
		}
        for (key in obj) {
            let element =  document.getElementById(key);
            element.innerHTML = obj[key];
        }
		loaderOff();
    });
}
function loaderOn() {
	let reloadButton = document.getElementById('reload');
	reloadButton.innerHTML = '<img src="/loader.gif" style="margin-bottom:2px;width:16px;height:16px">';
	LOADING = true;
}
function loaderOff() {
	let reloadButton = document.getElementById('reload');
	reloadButton.innerHTML = '<img src="/reload.png" style="margin-bottom:3px;width:16px;height:16px">';
	LOADING = false;
}
function houseMouseOverListenerOn() {
	let homeMouseOver = document.getElementsByClassName('homeMouseOver');
	for(let i in homeMouseOver) {
		if (typeof homeMouseOver[i] === 'object') {
			homeMouseOver[i].addEventListener("mouseover", (event) => {
				let _element = homeMouseOver[i].getElementsByClassName('tdTitle');
				for(let n in _element) {
					if (typeof _element[n] === 'object') {
						_element[n].classList.add("tdTitleMOver");
						_element[n].classList.remove("tdTitle");
					}
				}
				_element = homeMouseOver[i].getElementsByClassName('tdValue');
				for(let n in _element) {
					if (typeof _element[n] === 'object') {
						_element[n].classList.add("tdValueMOver");
						_element[n].classList.remove("tdValue");
						
					}
				}
			});
			homeMouseOver[i].addEventListener("mouseout", (event) => {
				let _element = homeMouseOver[i].getElementsByClassName('tdTitleMOver');
				for(let n in _element) {
					if (typeof _element[n] === 'object') {
						_element[n].classList.add("tdTitle");
						_element[n].classList.remove("tdTitleMOver");
					}
				}
				_element = homeMouseOver[i].getElementsByClassName('tdValueMOver');
				for(let n in _element) {
					if (typeof _element[n] === 'object') {
						_element[n].classList.add("tdValue");
						_element[n].classList.remove("tdValueMOver");
					}
				}
			});
		}
	}
}
function loadSPIFFS(startInterval) {
	loaderOn();
	let spiffsHTML = '<table class="table"><tbody><tr>';
	spiffsHTML += '<td style="text-align:right;width:50%;" colspan="2">'
	+ '<input type="text" id="selectFileInput" readonly placeholder="Select file"></td>'
	+ '<td style="width: 50%;"><button class="btn btn-primary" type="button" id="selectFileButton">Select file</button>'
	+ '<button class="btn btn-primary" type="button" id="uploadFileButton">Upload</button></td></tr>';
	ajax.get('/disk_usage', {}, function(data) {
		let obj;
		try {
			obj = JSON.parse(data);
		} catch (error) {
			console.error(error);
			showDialog(error);
		}
		if(obj) {
			let totalSpace = obj['TU'];
			let usedSpace = obj['UU'];
			let usedPr = parseInt(usedSpace * 100 / totalSpace);
			let freePr = 100 - usedPr;
			spiffsHTML += '<tr><td class="diskUsageTable" colspan="3"><table class="table" style="margin-bottom:0px;"><tbody>';
			spiffsHTML += '<tr><td style="padding:3px;text-align:center;font-weight:bold;background:rgb(33, 77, 252);'
			+ 'color:rgb(255,255,255);" width="' + usedPr + '%">' + usedPr + '%</td>'
			+ '<td style="padding:3px;text-align:center;font-weight:bold;color:rgb(255,255,255);background:rgb(17,190,2);"'
			+ ' width="' + freePr + '%">' + freePr + '%</td></tr>'
			+ '<tr><td class="duInfo" colspan="2">Total: ' +   obj['T'] + ' Used: ' + obj['U'] + '</td></tr>'
            + '</tbody></table></td></tr>';
			ajax.get('/list', {dir: '/'}, function(data) {
				let obj;
				try {
					obj = JSON.parse(data);
				} catch (error) {
					console.error(error);
					showDialog(error);
				}
				if(obj) { 
					obj.sort(compare);
					for (i in obj) {
						let deleteFile = '';
						if(obj[i]['name'] != 'system.log') {
							deleteFile = '&nbsp;<span class="d-inline-block" title="Delete" style="cursor:pointer;" '
							+ 'onclick="removeFile(\''+ obj[i]['name'] + '\')">&#10060;</span>';
						}
						spiffsHTML += '<tr class="trSPIFFS"><td class="text-end tdSPIFFS1 text-uppercase">' + obj[i]['name'] + '</td>'
                        + '<td class="text-end tdSPIFFS2">' + obj[i]['size'] + '</td>'
                        + '<td class="tdSPIFFS3"><a href="/' + obj[i]['name'] + '" title="Download" style="text-decoration:none;">'
						+ '&#128190;</a>' + deleteFile + '</td></tr>';
					}
				}
				spiffsHTML += '</tbody></table>';
				spiffsHTML += '<div style="display:none;height:0px;width:0px;"><form method="POST" '
				+ 'action="/edit" id="uploadForm"><input type="file" id="upload_file"></form></div>';
				let contentDIV = document.getElementById('content');
				contentDIV.innerHTML = spiffsHTML;
				let selectFileButton = document.getElementById("selectFileButton");
				let uploadFileButton = document.getElementById("uploadFileButton");
				let inputFile = document.getElementById("upload_file");
				let selectFileInput = document.getElementById("selectFileInput");
				let uploadForm = document.getElementById("uploadForm");
				selectFileButton.onclick = function(){
					inputFile.click();
				};
				inputFile.onchange = function(){
					if(spiffsInterval) {
						clearInterval(spiffsInterval);
					}
					selectFileInput.value = inputFile.value.match(/[^\\/]*$/)[0];
				};
				uploadFileButton.onclick = function(){
					if(!inputFile.value){
						showDialog('Select file!');
						return;
					}
					loaderOn();
					let formData = new FormData();
					for (const file of inputFile.files) {
						formData.append("files", file);
					}
					fetch("/edit", {
						method: "post",
						body: formData,
						mode: "no-cors"
					}).catch((error) => ("Something went wrong!", error));
					uploadForm.reset();
					selectFileInput.value = '';
					if(spiffsInterval) {
						clearInterval(spiffsInterval);
					}
					loadSPIFFS(true);
				};
				if(startInterval) {
					spiffsInterval = setInterval(() => {
						loadSPIFFS(false);
					}, 60000);
				}
				loaderOff();
			});
		}
		loaderOff();
	});
}
function compare(a, b) {
    if(a.name < b.name) {
      return -1;
    }
    if(a.name > b.name) {
      return 1;
    }
    return 0;
}
function removeFile(fileName) {
    if(confirm("Delete \"" + fileName +"\"?")) {
		loaderOn();
		const formData = new FormData();
        formData.append('data', '/' + fileName);
        fetch('/edit', {method: 'DELETE', body:formData }).then(() => loadSPIFFS(false));
    }

}
