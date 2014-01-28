/*
 * Copyright (C) 2007, 2008 Apple Inc.  All rights reserved.
 * Copyright (C) 2009 Joseph Pecoraro
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @param {!WebInspector.InspectorView} inspectorView
 */
WebInspector.Drawer = function(inspectorView)
{
    this._inspectorView = inspectorView;

    this.element = this._inspectorView.devtoolsElement().createChild("div", "drawer");
    this.element.style.flexBasis = 0;

    this._savedHeight = 200; // Default.

    this._drawerContentsElement = this.element.createChild("div");
    this._drawerContentsElement.id = "drawer-contents";

    this._toggleDrawerButton = new WebInspector.StatusBarButton(WebInspector.UIString("Show drawer."), "console-status-bar-item");
    this._toggleDrawerButton.addEventListener("click", this.toggle, this);

    this._tabbedPane = new WebInspector.TabbedPane();
    this._tabbedPane.closeableTabs = false;
    this._tabbedPane.markAsRoot();
    this._tabbedPane.setRetainTabOrder(true, WebInspector.moduleManager.orderComparator(WebInspector.Drawer.ViewFactory, "name", "order"));

    this._tabbedPane.addEventListener(WebInspector.TabbedPane.EventTypes.TabClosed, this._updateTabStrip, this);
    this._tabbedPane.addEventListener(WebInspector.TabbedPane.EventTypes.TabSelected, this._tabSelected, this);
    WebInspector.installDragHandle(this._tabbedPane.headerElement(), this._startStatusBarDragging.bind(this), this._statusBarDragging.bind(this), this._endStatusBarDragging.bind(this), "ns-resize");
    this._tabbedPane.element.createChild("div", "drawer-resizer");
    this._showDrawerOnLoadSetting = WebInspector.settings.createSetting("WebInspector.Drawer.showOnLoad", false);
    this._lastSelectedViewSetting = WebInspector.settings.createSetting("WebInspector.Drawer.lastSelectedView", "console");
    this._initialize();
}

WebInspector.Drawer.prototype = {
    _initialize: function()
    {
        this._viewFactories = {};
        var extensions = WebInspector.moduleManager.extensions(WebInspector.Drawer.ViewFactory);

        for (var i = 0; i < extensions.length; ++i) {
            var descriptor = extensions[i].descriptor();
            var id = descriptor["name"];
            var title = WebInspector.UIString(descriptor["title"]);
            var settingName = descriptor["setting"];
            var setting = settingName ? /** @type {!WebInspector.Setting|undefined} */ (WebInspector.settings[settingName]) : null;

            this._viewFactories[id] = extensions[i];

            if (setting) {
                setting.addChangeListener(this._toggleSettingBasedView.bind(this, id, title, setting));
                if (setting.get())
                    this._tabbedPane.appendTab(id, title, new WebInspector.View());
            } else {
                this._tabbedPane.appendTab(id, title, new WebInspector.View());
            }
        }
    },

    /**
     * @param {string} id
     * @param {string} title
     * @param {!WebInspector.Setting} setting
     */
    _toggleSettingBasedView: function(id, title, setting)
    {
        this._tabbedPane.closeTab(id);
        if (setting.get())
            this._tabbedPane.appendTab(id, title, new WebInspector.View());
    },

    /**
     * @return {!Element}
     */
    toggleButtonElement: function()
    {
        return this._toggleDrawerButton.element;
    },

    _constrainHeight: function(height)
    {
        return Number.constrain(height, Preferences.minConsoleHeight, this._inspectorView.devtoolsElement().offsetHeight - Preferences.minConsoleHeight);
    },

    /**
     * @return {boolean}
     */
    isHiding: function()
    {
        return this._isHiding;
    },

    /**
     * @param {string} tabId
     * @param {string} title
     * @param {!WebInspector.View} view
     */
    _addView: function(tabId, title, view)
    {
        if (!this._tabbedPane.hasTab(tabId)) {
            this._tabbedPane.appendTab(tabId, title, view,  undefined, false);
        } else {
            this._tabbedPane.changeTabTitle(tabId, title);
            this._tabbedPane.changeTabView(tabId, view);
        }
    },

    /**
     * @param {string} id
     */
    closeView: function(id)
    {
        this._tabbedPane.closeTab(id);
    },

    /**
     * @param {string} id
     * @param {boolean=} immediately
     */
    showView: function(id, immediately)
    {
        if (!this._toggleDrawerButton.enabled())
            return;
        var viewFactory = this._viewFactory(id);
        if (viewFactory)
            this._tabbedPane.changeTabView(id, viewFactory.createView());
        this._innerShow(immediately);
        this._tabbedPane.selectTab(id, true);
        // In case this id is already selected, anyways persist it as the last saved value.
        this._lastSelectedViewSetting.set(id);
        this._updateTabStrip();
    },

    /**
     * @param {string} id
     * @param {string} title
     * @param {!WebInspector.View} view
     */
    showCloseableView: function(id, title, view)
    {
        if (!this._toggleDrawerButton.enabled())
            return;
        if (!this._tabbedPane.hasTab(id)) {
            this._tabbedPane.appendTab(id, title, view, undefined, false, true);
        } else {
            this._tabbedPane.changeTabView(id, view);
            this._tabbedPane.changeTabTitle(id, title);
        }
        this._innerShow();
        this._tabbedPane.selectTab(id, true);
        this._updateTabStrip();
    },

    /**
     * @param {boolean=} immediately
     */
    show: function(immediately)
    {
        this.showView(this._lastSelectedViewSetting.get(), immediately);
    },

    showOnLoadIfNecessary: function()
    {
        if (this._showDrawerOnLoadSetting.get())
            this.showView(this._lastSelectedViewSetting.get(), true);
    },

    /**
     * @param {boolean=} immediately
     */
    _innerShow: function(immediately)
    {
        this._immediatelyFinishAnimation();

        if (this._toggleDrawerButton.toggled)
            return;
        this._showDrawerOnLoadSetting.set(true);
        this._toggleDrawerButton.toggled = true;
        this._toggleDrawerButton.title = WebInspector.UIString("Hide drawer.");

        document.body.classList.add("drawer-visible");
        this._tabbedPane.show(this._drawerContentsElement);

        var height = this._constrainHeight(this._savedHeight);
        // While loading, window may be zero height. Do not corrupt desired drawer height in this case.
        // FIXME: making Drawer a view and placing it inside SplitView eliminates the need for this.
        if (window.innerHeight == 0)
            height = this._savedHeight;
        var animations = [
            {element: this.element, start: {"flex-basis": 23}, end: {"flex-basis": height}},
        ];

        /**
         * @param {boolean} finished
         * @this {WebInspector.Drawer}
         */
        function animationCallback(finished)
        {
            if (this._inspectorView.currentPanel())
                this._inspectorView.currentPanel().doResize();
            if (!finished)
                return;
            this._updateTabStrip();
            if (this._visibleView()) {
                // Get console content back
                this._tabbedPane.changeTabView(this._tabbedPane.selectedTabId, this._visibleView());
                this._visibleView().focus();
            }
            delete this._currentAnimation;
        }

        this._currentAnimation = WebInspector.animateStyle(animations, this._animationDuration(immediately), animationCallback.bind(this));

        if (immediately)
            this._currentAnimation.forceComplete();
    },

    /**
     * @param {boolean=} immediately
     */
    hide: function(immediately)
    {
        this._immediatelyFinishAnimation();

        if (!this._toggleDrawerButton.toggled)
            return;
        this._showDrawerOnLoadSetting.set(false);
        this._toggleDrawerButton.toggled = false;
        this._toggleDrawerButton.title = WebInspector.UIString("Show console.");

        this._isHiding = true;
        this._savedHeight = this.element.offsetHeight;

        WebInspector.restoreFocusFromElement(this.element);

        // Temporarily set properties and classes to mimic the post-animation values so panels
        // like Elements in their updateStatusBarItems call will size things to fit the final location.
        document.body.classList.remove("drawer-visible");
        this._inspectorView.currentPanel().statusBarResized();
        document.body.classList.add("drawer-visible");

        var animations = [
            {element: this.element, start: {"flex-basis": this.element.offsetHeight }, end: {"flex-basis": 23}},
        ];

        /**
         * @param {boolean} finished
         * @this {WebInspector.Drawer}
         */
        function animationCallback(finished)
        {
            var panel = this._inspectorView.currentPanel();
            if (!finished) {
                panel.doResize();
                return;
            }
            this._tabbedPane.detach();
            this._drawerContentsElement.removeChildren();
            document.body.classList.remove("drawer-visible");
            panel.doResize();
            delete this._currentAnimation;
            delete this._isHiding;
        }

        this._currentAnimation = WebInspector.animateStyle(animations, this._animationDuration(immediately), animationCallback.bind(this));

        if (immediately)
            this._currentAnimation.forceComplete();
    },

    resize: function()
    {
        if (!this._toggleDrawerButton.toggled)
            return;

        this._visibleView().storeScrollPositions();
        var height = this._constrainHeight(this.element.offsetHeight);
        this.element.style.flexBasis = height + "px";
        this._tabbedPane.doResize();
    },

    _immediatelyFinishAnimation: function()
    {
        if (this._currentAnimation)
            this._currentAnimation.forceComplete();
    },

    /**
     * @param {boolean=} immediately
     * @return {number}
     */
    _animationDuration: function(immediately)
    {
        return immediately ? 0 : 50;
    },

    /**
     * @return {boolean}
     */
    _startStatusBarDragging: function(event)
    {
        if (!this._toggleDrawerButton.toggled || event.target !== this._tabbedPane.headerElement())
            return false;

        this._visibleView().storeScrollPositions();
        this._statusBarDragOffset = event.pageY - this.element.totalOffsetTop();
        return true;
    },

    _statusBarDragging: function(event)
    {
        var height = window.innerHeight - event.pageY + this._statusBarDragOffset;
        height = Number.constrain(height, Preferences.minConsoleHeight, this._inspectorView.devtoolsElement().offsetHeight - Preferences.minConsoleHeight);

        this.element.style.flexBasis = height + "px";
        if (this._inspectorView.currentPanel())
            this._inspectorView.currentPanel().doResize();
        this._tabbedPane.doResize();

        event.consume(true);
    },

    _endStatusBarDragging: function(event)
    {
        this._savedHeight = this.element.offsetHeight;
        delete this._statusBarDragOffset;

        event.consume();
    },

    /**
     * @return {!WebInspector.View} view
     */
    _visibleView: function()
    {
        return this._tabbedPane.visibleView;
    },

    _updateTabStrip: function()
    {
        this._tabbedPane.onResize();
        this._tabbedPane.doResize();
    },

    /**
     * @param {!WebInspector.Event} event
     */
    _tabSelected: function(event)
    {
        var tabId = this._tabbedPane.selectedTabId;
        if (event.data["isUserGesture"] && !this._tabbedPane.isTabCloseable(tabId))
            this._lastSelectedViewSetting.set(tabId);
        var viewFactory = this._viewFactory(tabId);
        if (viewFactory)
            this._tabbedPane.changeTabView(tabId, viewFactory.createView());
    },

    toggle: function()
    {
        if (this._toggleDrawerButton.toggled)
            this.hide();
        else
            this.show();
    },

    /**
     * @return {boolean}
     */
    visible: function()
    {
        return this._toggleDrawerButton.toggled;
    },

    /**
     * @return {string}
     */
    selectedViewId: function()
    {
        return this._tabbedPane.selectedTabId;
    },

    /**
     * @return {?WebInspector.Drawer.ViewFactory}
     */
    _viewFactory: function(id)
    {
        return this._viewFactories[id] ? /** @type {!WebInspector.Drawer.ViewFactory} */ (this._viewFactories[id].instance()) : null;
    }
}

/**
 * @interface
 */
WebInspector.Drawer.ViewFactory = function()
{
}

WebInspector.Drawer.ViewFactory.prototype = {
    /**
     * @return {!WebInspector.View}
     */
    createView: function() {}
}

/**
 * @constructor
 * @implements {WebInspector.Drawer.ViewFactory}
 * @param {function(new:T)} constructor
 * @template T
 */
WebInspector.Drawer.SingletonViewFactory = function(constructor)
{
    this._constructor = constructor;
}

WebInspector.Drawer.SingletonViewFactory.prototype = {
    /**
     * @return {!WebInspector.View}
     */
    createView: function()
    {
        if (!this._instance)
            this._instance = /** @type {!WebInspector.View} */(new this._constructor());
        return this._instance;
    }
}
