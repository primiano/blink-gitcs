/*
 * Copyright 2014 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**
 * @constructor
 * @extends {Protocol.Agents}
 * @param {string} name
 * @param {!InspectorBackendClass.Connection} connection
 * @param {function(!WebInspector.Target)=} callback
 */
WebInspector.Target = function(name, connection, callback)
{
    Protocol.Agents.call(this, connection.agentsMap());
    /** @type {!WeakReference.<!WebInspector.Target>} */
    this._weakReference = new WeakReference(this);
    this._name = name;
    this._connection = connection;
    this._id = WebInspector.Target._nextId++;

    /** @type {!Object.<string, boolean>} */
    this._capabilities = {};
    this.pageAgent().canScreencast(this._initializeCapability.bind(this, WebInspector.Target.Capabilities.canScreencast, null));
    this.pageAgent().hasTouchInputs(this._initializeCapability.bind(this, WebInspector.Target.Capabilities.hasTouchInputs, null));
    if (WebInspector.experimentsSettings.timelinePowerProfiler.isEnabled())
        this.powerAgent().canProfilePower(this._initializeCapability.bind(this, WebInspector.Target.Capabilities.canProfilePower, null));
    this.workerAgent().canInspectWorkers(this._initializeCapability.bind(this, WebInspector.Target.Capabilities.canInspectWorkers, this._loadedWithCapabilities.bind(this, callback)));

    /** @type {!WebInspector.Lock} */
    this.profilingLock = new WebInspector.Lock();
}

/**
 * @enum {string}
 */
WebInspector.Target.Capabilities = {
    canScreencast: "canScreencast",
    hasTouchInputs: "hasTouchInputs",
    canProfilePower: "canProfilePower",
    canInspectWorkers: "canInspectWorkers"
}

WebInspector.Target._nextId = 1;

WebInspector.Target.prototype = {

    /**
     * @return {number}
     */
    id: function()
    {
        return this._id;
    },

    /**
     *
     * @return {string}
     */
    name: function()
    {
        return this._name;
    },

    /**
     * @return {!WeakReference.<!WebInspector.Target>}
     */
    weakReference: function()
    {
       return this._weakReference;
    },

    /**
     * @param {string} name
     * @param {function()|null} callback
     * @param {?Protocol.Error} error
     * @param {boolean} result
     */
    _initializeCapability: function(name, callback, error, result)
    {
        this._capabilities[name] = result;
        if (callback)
            callback();
    },

    /**
     * @param {string} capability
     * @return {boolean}
     */
    hasCapability: function(capability)
    {
        return !!this._capabilities[capability];
    },

    /**
     * @param {function(!WebInspector.Target)=} callback
     */
    _loadedWithCapabilities: function(callback)
    {
        /** @type {!WebInspector.ConsoleModel} */
        this.consoleModel = new WebInspector.ConsoleModel(this);
        // This and similar lines are needed for compatibility.
        if (!WebInspector.consoleModel)
            WebInspector.consoleModel = this.consoleModel;

        /** @type {!WebInspector.NetworkManager} */
        this.networkManager = new WebInspector.NetworkManager(this);
        if (!WebInspector.networkManager)
            WebInspector.networkManager = this.networkManager;

        /** @type {!WebInspector.ResourceTreeModel} */
        this.resourceTreeModel = new WebInspector.ResourceTreeModel(this);
        if (!WebInspector.resourceTreeModel)
            WebInspector.resourceTreeModel = this.resourceTreeModel;

        /** @type {!WebInspector.NetworkLog} */
        this.networkLog = new WebInspector.NetworkLog(this);
        if (!WebInspector.networkLog)
            WebInspector.networkLog = this.networkLog;

        /** @type {!WebInspector.DebuggerModel} */
        this.debuggerModel = new WebInspector.DebuggerModel(this);
        if (!WebInspector.debuggerModel)
            WebInspector.debuggerModel = this.debuggerModel;

        /** @type {!WebInspector.RuntimeModel} */
        this.runtimeModel = new WebInspector.RuntimeModel(this);
        if (!WebInspector.runtimeModel)
            WebInspector.runtimeModel = this.runtimeModel;

        /** @type {!WebInspector.DOMModel} */
        this.domModel = new WebInspector.DOMModel(this);

        /** @type {!WebInspector.CSSStyleModel} */
        this.cssModel = new WebInspector.CSSStyleModel(this);
        if (!WebInspector.cssModel)
            WebInspector.cssModel = this.cssModel;

        /** @type {!WebInspector.WorkerManager} */
        this.workerManager = new WebInspector.WorkerManager(this, this.hasCapability(WebInspector.Target.Capabilities.canInspectWorkers));
        if (!WebInspector.workerManager)
            WebInspector.workerManager = this.workerManager;

        if (this.hasCapability(WebInspector.Target.Capabilities.canProfilePower))
            WebInspector.powerProfiler = new WebInspector.PowerProfiler();

        /** @type {!WebInspector.TimelineManager} */
        this.timelineManager = new WebInspector.TimelineManager(this);
        if (!WebInspector.timelineManager)
            WebInspector.timelineManager = this.timelineManager;

        /** @type {!WebInspector.DatabaseModel} */
        this.databaseModel = new WebInspector.DatabaseModel(this);
        if (!WebInspector.databaseModel)
            WebInspector.databaseModel = this.databaseModel;

        /** @type {!WebInspector.DOMStorageModel} */
        this.domStorageModel = new WebInspector.DOMStorageModel(this);
        if (!WebInspector.domStorageModel)
            WebInspector.domStorageModel = this.domStorageModel;

        /** @type {!WebInspector.CPUProfilerModel} */
        this.cpuProfilerModel = new WebInspector.CPUProfilerModel(this);
        if (!WebInspector.cpuProfilerModel)
            WebInspector.cpuProfilerModel = this.cpuProfilerModel;

        /** @type {!WebInspector.HeapProfilerModel} */
        this.heapProfilerModel = new WebInspector.HeapProfilerModel(this);

        if (callback)
            callback(this);
    },

    /**
     * @override
     * @param {string} domain
     * @param {!Object} dispatcher
     */
    registerDispatcher: function(domain, dispatcher)
    {
        this._connection.registerDispatcher(domain, dispatcher);
    },

    /**
     * @return {boolean}
     */
    isWorkerTarget: function()
    {
        return !this.hasCapability(WebInspector.Target.Capabilities.canInspectWorkers);
    },

    /**
     * @return {boolean}
     */
    isMobile: function()
    {
        // FIXME: either add a separate capability or rename canScreencast to isMobile.
        return this.hasCapability(WebInspector.Target.Capabilities.canScreencast);
    },

    dispose: function()
    {
        this._weakReference.clear();
        this.debuggerModel.dispose();
        this.networkManager.dispose();
        this.cpuProfilerModel.dispose();
    },

    __proto__: Protocol.Agents.prototype
}

/**
 * @constructor
 * @extends {WebInspector.Object}
 * @param {!WebInspector.Target} target
 */
WebInspector.SDKObject = function(target)
{
    WebInspector.Object.call(this);
    this._target = target;
}

WebInspector.SDKObject.prototype = {
    /**
     * @return {!WebInspector.Target}
     */
    target: function()
    {
        return this._target;
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 */
WebInspector.TargetManager = function()
{
    /** @type {!Array.<!WebInspector.Target>} */
    this._targets = [];
    /** @type {!Array.<!WebInspector.TargetManager.Observer>} */
    this._observers = [];
}

WebInspector.TargetManager.prototype = {

    /**
     * @param {!WebInspector.TargetManager.Observer} targetObserver
     */
    observeTargets: function(targetObserver)
    {
        this.targets().forEach(targetObserver.targetAdded.bind(targetObserver));
        this._observers.push(targetObserver);
    },

    /**
     * @param {!WebInspector.TargetManager.Observer} targetObserver
     */
    unobserveTargets: function(targetObserver)
    {
        this._observers.remove(targetObserver);
    },

    /**
     * @param {string} name
     * @param {!InspectorBackendClass.Connection} connection
     * @param {function(!WebInspector.Target)=} callback
     */
    createTarget: function(name, connection, callback)
    {
        var target = new WebInspector.Target(name, connection, callbackWrapper.bind(this));

        /**
         * @this {WebInspector.TargetManager}
         * @param {!WebInspector.Target} newTarget
         */
        function callbackWrapper(newTarget)
        {
            this.addTarget(newTarget);
            if (callback)
                callback(newTarget);
        }
    },

    /**
     * @param {!WebInspector.Target} newTarget
     */
    addTarget: function(newTarget)
    {
        this._targets.push(newTarget);
        var copy = this._observers;
        for (var i = 0; i < copy.length; ++i)
            copy[i].targetAdded(newTarget);
    },

    /**
     * @param {!WebInspector.Target} target
     */
    removeTarget: function(target)
    {
        this._targets.remove(target);
        target.dispose();
        var copy = this._observers.slice();
        for (var i = 0; i < copy.length; ++i)
            copy[i].targetRemoved(target);
    },

    /**
     * @return {!Array.<!WebInspector.Target>}
     */
    targets: function()
    {
        return this._targets.slice();
    },

    /**
     * @return {?WebInspector.Target}
     */
    activeTarget: function()
    {
        return this._targets[0];
    }
}

/**
 * @interface
 */
WebInspector.TargetManager.Observer = function()
{
}

WebInspector.TargetManager.Observer.prototype = {
    /**
     * @param {!WebInspector.Target} target
     */
    targetAdded: function(target) { },

    /**
     * @param {!WebInspector.Target} target
     */
    targetRemoved: function(target) { },
}

/**
 * @type {!WebInspector.TargetManager}
 */
WebInspector.targetManager = new WebInspector.TargetManager();
