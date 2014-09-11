self.addEventListener('fetch', function(event) {
    var url = event.request.url;
    if (url.indexOf('dummy?test') == -1) {
      return;
    }
    event.respondWith(new Promise(function(resolve) {
        var headers = [];
        event.request.headers.forEach(function(value, key) {
            headers.push([key, value]);
          });
        event.request.text()
          .then(function(result) {
              resolve(new Response(JSON.stringify({
                  method: event.request.method,
                  headers: headers,
                  body: result
                })));
            })
      }));
  });
