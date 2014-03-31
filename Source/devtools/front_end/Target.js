/*
 * Copyright 2014 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**
 * @constructor
 * @extends {Protocol.Agents}
 * @param {!InspectorBackendClass.Connection} connection
 * @param {function(!WebInspector.Target)=} callback
 */
WebInspector.Target = function(connection, callback)
{
    Protocol.Agents.call(this, connection.agentsMap());
    this._connection = connection;
    this.isMainFrontend = false;

    this.pageAgent().canScreencast(this._initializeCapability.bind(this, "canScreencast", null));

    if (WebInspector.experimentsSettings.powerProfiler.isEnabled())
        this.powerAgent().canProfilePower(this._initializeCapability.bind(this, "canProfilePower", null));

    this.workerAgent().canInspectWorkers(this._initializeCapability.bind(this, "isMainFrontend", this._loadedWithCapabilities.bind(this, callback)));
}

WebInspector.Target.prototype = {

    _initializeCapability: function(name, callback, error, result)
    {
        this[name] = result;
        if (!Capabilities[name])
            Capabilities[name] = result;
        if (callback)
            callback();
    },

    /**
     * @param {function(!WebInspector.Target)=} callback
     */
    _loadedWithCapabilities: function(callback)
    {
        this.consoleModel = new WebInspector.ConsoleModel(this);
        //This and similar lines are needed for compatibility
        if (!WebInspector.console)
            WebInspector.console = this.consoleModel;

        this.networkManager = new WebInspector.NetworkManager(this);
        if (!WebInspector.networkManager)
            WebInspector.networkManager = this.networkManager;

        this.resourceTreeModel = new WebInspector.ResourceTreeModel(this);
        if (!WebInspector.resourceTreeModel)
            WebInspector.resourceTreeModel = this.resourceTreeModel;

        this.debuggerModel = new WebInspector.DebuggerModel(this);
        if (!WebInspector.debuggerModel)
            WebInspector.debuggerModel = this.debuggerModel;

        this.runtimeModel = new WebInspector.RuntimeModel(this);
        if (!WebInspector.runtimeModel)
            WebInspector.runtimeModel = this.runtimeModel;

        this.domModel = new WebInspector.DOMModel(this);
        if (!WebInspector.domModel)
            WebInspector.domModel = this.domModel;

        this.cssModel = new WebInspector.CSSStyleModel(this);
        if (!WebInspector.cssModel)
            WebInspector.cssModel = this.cssModel;

        this.workerManager = new WebInspector.WorkerManager(this, this.isMainFrontend);
        if (!WebInspector.workerManager)
            WebInspector.workerManager = this.workerManager;

        if (this.canProfilePower)
            WebInspector.powerProfiler = new WebInspector.PowerProfiler();

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
        return !this.isMainFrontend;
    },

    __proto__: Protocol.Agents.prototype
}

/**
 * @constructor
 * @param {!WebInspector.Target} target
 */
WebInspector.TargetAware = function(target)
{
    this._target = target;
}

WebInspector.TargetAware.prototype = {
    /**
     * @return {!WebInspector.Target}
     */
    target: function()
    {
        return this._target;
    }
}

/**
 * @constructor
 */
WebInspector.TargetManager = function()
{
    /** @type {!Array.<!WebInspector.Target>} */
    this._targets = [];
    this._observers = [];
}

WebInspector.TargetManager.Events = {
    TargetAdded: "TargetAdded",
}

WebInspector.TargetManager.prototype = {

    /**
     * @param {!WebInspector.TargetManager.Observer} targetObserver
     */
    observeTargets: function(targetObserver)
    {
        WebInspector.targetManager.targets().forEach(targetObserver.targetAdded.bind(targetObserver));
        if (this._targets.length)
            targetObserver.activeTargetChanged(this._targets[0]);
        this._observers.push(targetObserver);
    },

    /**
     * @param {!InspectorBackendClass.Connection} connection
     * @param {function(!WebInspector.Target)=} callback
     */
    createTarget: function(connection, callback)
    {
        var target = new WebInspector.Target(connection, callbackWrapper.bind(this));

        /**
         * @this {WebInspector.TargetManager}
         * @param newTarget
         */
        function callbackWrapper(newTarget)
        {
            if (callback)
                callback(newTarget);

            this._targets.push(newTarget);
            var copy = this._observers;
            for (var i = 0; i < copy.length; ++i) {
                copy[i].targetAdded(newTarget);
                if (this._targets.length === 1)
                    copy[i].activeTargetChanged(newTarget);
            }
        }
    },

    /**
     * @return {!Array.<!WebInspector.Target>}
     */
    targets: function()
    {
        return this._targets;
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

    /**
     * @param {?WebInspector.Target} target
     */
    activeTargetChanged: function(target) { }
}

/**
 * @type {!WebInspector.TargetManager}
 */
WebInspector.targetManager;
