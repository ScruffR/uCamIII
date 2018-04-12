// Run this like:
// node imageReceiver.js

var fs = require('fs');
var path = require('path');
var net = require('net');

// For this simple test, just create dat files in the "out" directory in the directory
// where imageReceiver.js lives.
var outputDir = path.join(__dirname, "out");  

var dataPort = 8124; // this is the port to listen on for data from the device

// Output files in the out directory are of the form 00001.jpg. lastNum is used 
// to speed up scanning for the next unique file.
var lastNum = 0;

// Create the out directory if it does not exist
try {
	fs.mkdirSync(outputDir);
}
catch(e) {
}

// Start a TCP Server. This is what receives data from the Particle device
// https://gist.github.com/creationix/707146
net.createServer(function (socket) {
	console.log('data connection started from ' + socket.remoteAddress);
	
	// The server sends a 8-bit byte value for each sample. Javascript doesn't really like
	// binary values, so we use setEncoding to read each byte of a data as 2 hex digits instead.
	socket.setEncoding('hex');
	
	var outPath = getUniqueOutputPath();
	
	let writeStream = fs.createWriteStream(outPath);
	
	socket.on('data', function (data) {
		// We received data on this connection.
		// console.log("got data " + (data.length / 2));
		writeStream.write(data, 'hex');
	});
	socket.on('end', function () {
		console.log('transmission complete, saved to ' + outPath);
		writeStream.end();
	});
}).listen(dataPort);


function formatName(num) {
	var s = num.toString();
	
	while(s.length < 5) {
		s = '0' + s;
	}
	return s + '.jpg';
}

function getUniqueOutputPath() {
	for(var ii = lastNum + 1; ii < 99999; ii++) {
		var outPath = path.join(outputDir, formatName(ii));
		try {
			fs.statSync(outPath);
		}
		catch(e) {
			// File does not exist, use this one
			lastNum = ii;
			return outPath;
		}
	}
	lastNum = 0;
	return "00000.jpg";
}
