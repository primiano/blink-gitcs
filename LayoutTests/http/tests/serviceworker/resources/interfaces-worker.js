importScripts('interfaces.js');
importScripts('worker-test-helpers.js');

test(function() {
    var EVENT_HANDLER = 'object';

    verifyInterface('ServiceWorkerGlobalScope',
                    self,
                    {
                        scope: 'string',
                        clients: 'object',

                        onactivate: EVENT_HANDLER,
                        onfetch: EVENT_HANDLER,
                        oninstall: EVENT_HANDLER,
                        onmessage: EVENT_HANDLER
                    });

    verifyInterface('ServiceWorkerClients',
                    self.clients,
                    {
                        getServiced: 'function'
                    });

    verifyInterface('Client');
    // FIXME: Get an instance and test it, or ensure property exists on prototype.

}, 'Interfaces and attributes in ServiceWorkerGlobalScope');
