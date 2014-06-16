self.onmessage = function(e) {
    var message = e.data;
    if ('port' in message) {
        port = message.port;
        doNextFetchTest(port);
    }
};

var testTargets = [
    'other.html',
    'http://',
    'http://www.example.com/foo'
];

function doNextFetchTest(port) {
    if (testTargets.length == 0) {
        port.postMessage('quit');
        // Destroying the execution context while fetch is happening should not cause a crash.
        fetch('dummy.html').then(function() {}).catch(function() {});
        self.close();
        return;
    }
    var target = testTargets.shift();
    fetch(target)
    .then(function() {
        port.postMessage('Resolved: ' + target);
        doNextFetchTest(port);
    }).catch(function(e) {
        port.postMessage('Rejected: ' + target + ' : '+ e.message);
        doNextFetchTest(port);
    });
};
