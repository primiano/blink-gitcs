// Adapter for testharness.js-style tests with Service Workers

function service_worker_test(url, description) {
    var t = async_test(description);
    t.step(function() {
        var scope = 'nonexistent';
        service_worker_unregister_and_register(t, url, scope, onRegistered);

        function onRegistered(worker) {
            var messageChannel = new MessageChannel();
            messageChannel.port1.onmessage = t.step_func(onMessage);
            worker.postMessage({port:messageChannel.port2}, [messageChannel.port2]);
        }

        function onMessage(e) {
            assert_equals(e.data, 'pass');
            service_worker_unregister_and_done(t, scope);
        }
    });
}

function service_worker_unregister_and_register(test, url, scope, onregister) {
    var options = scope ? { scope: scope } : {};
    return navigator.serviceWorker.unregister(scope).then(
        // FIXME: Wrap this with test.step_func once testharness.js is updated.
        function() {
            return navigator.serviceWorker.register(url, options);
        },
        unreached_rejection(test, 'Unregister should not fail')
    ).then(
        test.step_func(onregister),
        unreached_rejection(test, 'Registration should not fail')
    );
}

function service_worker_unregister_and_done(test, scope) {
    return navigator.serviceWorker.unregister(scope).then(
        test.done.bind(test),
        unreached_rejection(test, 'Unregister should not fail'));
}

// Rejection-specific helper that provides more details
function unreached_rejection(test, prefix) {
    return test.step_func(function(reason) {
        assert_unreached(prefix + ': ' + reason.name);
    });
}

// FIXME: Clean up the iframe when the test completes.
function with_iframe(url, f) {
    return new Promise(function(resolve, reject) {
        var frame = document.createElement('iframe');
        frame.src = url;
        frame.onload = function() {
            if (f) {
              f(frame);
            }
            resolve(frame);
        };
        document.body.appendChild(frame);
    });
}

function normalizeURL(url) {
  return new URL(url, document.location).toString().replace(/#.*$/, '');
}
