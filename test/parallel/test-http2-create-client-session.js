'use strict';

const common = require('../common');
const assert = require('assert');
const h2 = require('http2');
const body =
  '<html><head></head><body><h1>this is some data</h2></body></html>';

const server = h2.createServer();

// we use the lower-level API here
server.on('stream', common.mustCall(onStream));

function onStream(stream) {
  stream.respond({
    'content-type': 'text/html',
    ':status': 200
  });
  stream.end(body);
}

server.listen(0);

server.on('listening', common.mustCall(function() {

  const client = h2.connect(`http://localhost:${this.address().port}`);

  client.once('connect', common.mustCall(() => {

    const headers = { ':path': '/' };

    const req = client.request(headers);

    req.on('response', common.mustCall(function(headers) {
      assert.strictEqual(headers[':status'], '200', 'status code is set');
      assert.strictEqual(headers['content-type'], 'text/html',
                         'content type is set');
      assert(headers['date'], 'there is a date');
    }));

    let data = '';
    req.setEncoding('utf8');
    req.on('data', (d) => data += d);
    req.on('end', common.mustCall(function() {
      assert.strictEqual(body, data);
      server.close();
      client.socket.destroy();
    }));
    req.end();
  }));

}));
