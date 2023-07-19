// A small script for debugging HTTP server output

import net from "node:net";


const stats = {
	requests: 0,
	responses: 0,
	statuses: {}
};


const paths = [
	'/',
	'/index.html',
	'/some/very/long_path/that_doesnt_lead/anywhere.txt',
	'/main.js'
];

function random_path() {
	return paths[Math.floor(Math.random() * 4)];
}

async function get_data(sk) {
	return new Promise(res => {
		sk.once('data', chunk => {
			stats.responses++;

			const status_start = chunk.indexOf(' ') + 1;
			const status = chunk.subarray(status_start, chunk.indexOf(' ', status_start)).toString();

			if(!(status in stats.statuses))
				stats.statuses[status] = 0;

			stats.statuses[status]++;
			res();
		});
	});
}

const start = Date.now();

for(let i = 0; i < 100; i++) {
	const conn = net.connect(8080, "127.0.0.1", async () => {
		for(let j = 0; j < 1000; j++) {
			conn.write(`GET ${random_path()} HTTP/1.1\r\nDummy-Header: somevalue\r\nAnother-Header: ANOTHER, VALUE\r\n\r\n`);
			stats.requests++;
			await get_data(conn);

			if(stats.responses === 100 * 1000)
				finish();
		}
	});
}

function finish() {
	const end = Date.now();
	console.log(`Completed in ${end - start} ms`);
	console.log(stats);
}


/*
const conn = net.connect(8080, "127.0.0.1", () => {
	conn.write("GET / HTTP/1.1\r\nDummy-Header: dummyvalue\r\n\r\n");

	let total = Buffer.alloc(1024, 0);
	let index = 0;

	conn.on('data', buf => {
		for(let i = 0; i < buf.length; i++) {
			console.log(String.fromCharCode(buf[i]), buf[i]);
		}

		buf.copy(total, index);
		index += buf.length;
	});

	conn.on('end', () => {
		console.log("Disconnected :(");
	});

	setTimeout(() => {
		conn.write("GET / HTTP/1.1\r\nAnother-Header: anothervalue\r\n\r\n");
	}, 5 * 1000);
});
*/
