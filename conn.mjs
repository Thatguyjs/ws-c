// A small script for debugging HTTP server output

import net from "node:net";


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
});
