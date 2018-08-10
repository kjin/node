'use strict';

const common = require('../common');
if (!common.hasCrypto)
  common.skip('missing crypto');
const assert = require('assert');
const http2 = require('http2');

const msecs = common.platformTimeout(1);
const server = http2.createServer();

server.on('request', (req, res) => {
  res.setTimeout(msecs, common.mustCall(() => {
    res.end();
  }));
  // Note: The timeout is reset in the call to res.end() above. The 'timeout'
  // event listener might be called more than once if it takes more than 1ms
  // to close, so use the `once` event listener here. (This is more likely to
  // happen in Node 8 than 10.)
  res.once('timeout', common.mustCall());
  res.on('finish', common.mustCall(() => {
    res.setTimeout(msecs, common.mustNotCall());
    process.nextTick(() => {
      assert.doesNotThrow(
        () => res.setTimeout(msecs, common.mustNotCall())
      );
      server.close();
    });
  }));
});

server.listen(0, common.mustCall(() => {
  const port = server.address().port;
  const client = http2.connect(`http://localhost:${port}`);
  const req = client.request({
    ':path': '/',
    ':method': 'GET',
    ':scheme': 'http',
    ':authority': `localhost:${port}`
  });
  req.on('end', common.mustCall(() => {
    client.close();
  }));
  req.resume();
  req.end();
}));
