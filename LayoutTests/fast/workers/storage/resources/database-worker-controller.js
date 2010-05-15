var databaseWorker = new Worker('resources/database-worker.js');

databaseWorker.onerror = function(event) {
    log("Caught an error from the worker!");
    log(event);
    for (var i in event)
        log("event[" + i + "]: " + event[i]);
};

databaseWorker.onmessage = function(event) {
    if (event.data.indexOf('log:') == 0)
        log(event.data.substring(4));
    else if (event.data == 'notifyDone') {
        if (window.layoutTestController)
            layoutTestController.notifyDone();
    } else
        throw new Error("Unrecognized message: " + event);
};

function log(message)
{
    document.getElementById("console").innerText += message + "\n";
}
