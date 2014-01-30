/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE INC.
 * OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.View}
 * @param {boolean} isVertical
 * @param {string=} sidebarSizeSettingName
 * @param {number=} defaultSidebarWidth
 * @param {number=} defaultSidebarHeight
 */
WebInspector.SplitView = function(isVertical, sidebarSizeSettingName, defaultSidebarWidth, defaultSidebarHeight)
{
    WebInspector.View.call(this);

    this.registerRequiredCSS("splitView.css");
    this.element.classList.add("split-view");

    this._firstElement = this.element.createChild("div", "split-view-contents scroll-target split-view-contents-first vbox");
    this._secondElement = this.element.createChild("div", "split-view-contents scroll-target split-view-contents-second vbox");

    this._resizerElement = this.element.createChild("div", "split-view-resizer");
    this._onDragStartBound = this._onDragStart.bind(this);
    this._resizerElements = [];

    this._resizable = true;

    this._savedSidebarWidth = defaultSidebarWidth || 200;
    this._savedSidebarHeight = defaultSidebarHeight || this._savedSidebarWidth;

    if (0 < this._savedSidebarWidth && this._savedSidebarWidth < 1 &&
        0 < this._savedSidebarHeight && this._savedSidebarHeight < 1)
        this._useFraction = true;
    
    this._sidebarSizeSettingName = sidebarSizeSettingName;

    this.setSecondIsSidebar(true);

    this._innerSetVertical(isVertical);

    // Should be called after isVertical has the right value.
    this.installResizer(this._resizerElement);
}

WebInspector.SplitView.Events = {
    SidebarSizeChanged: "SidebarSizeChanged"
}

WebInspector.SplitView.prototype = {
    /**
     * @return {boolean}
     */
    isVertical: function()
    {
        return this._isVertical;
    },

    /**
     * @param {boolean} isVertical
     */
    setVertical: function(isVertical)
    {
        if (this._isVertical === isVertical)
            return;

        this._innerSetVertical(isVertical);

        if (this.isShowing())
            this._updateLayout();

        for (var i = 0; i < this._resizerElements.length; ++i)
            this._resizerElements[i].style.setProperty("cursor", this._isVertical ? "ew-resize" : "ns-resize");
    },

    /**
     * @param {boolean} isVertical
     */
    _innerSetVertical: function(isVertical)
    {
        this.element.classList.remove(this._isVertical ? "hbox" : "vbox");
        this._isVertical = isVertical;
        this.element.classList.add(this._isVertical ? "hbox" : "vbox");
        delete this._resizerElementSize;
        this._sidebarSize = -1;
    },
  
    /**
     * @param {boolean=} animate
     * @param {boolean=} fromOnResize
     */
    _updateLayout: function(animate, fromOnResize)
    {
        delete this._totalSize; // Lazy update.
        this._innerSetSidebarSize(this._lastSidebarSize(), false, animate, fromOnResize);
    },

    /**
     * @param {!WebInspector.View} view
     */
    setFirstView: function(view)
    {
        if (this._firstView)
            this._firstView.detach();
        this._firstView = view;
        view.show(this._firstElement);
    },

    /**
     * @param {!WebInspector.View} view
     */
    setSecondView: function(view)
    {
        if (this._secondView)
            this._secondView.detach();
        this._secondView = view;
        view.show(this._secondElement);
    },

    /**
     * @param {!WebInspector.View} view
     */
    setMainView: function(view)
    {
        if (this.isSidebarSecond())
            this.setFirstView(view);
        else
            this.setSecondView(view);
    },

    /**
     * @param {!WebInspector.View} view
     */
    setSidebarView: function(view)
    {
        if (this.isSidebarSecond())
            this.setSecondView(view);
        else
            this.setFirstView(view);
    },

    /**
     * @return {!Element}
     */
    firstElement: function()
    {
        return this._firstElement;
    },

    /**
     * @return {!Element}
     */
    secondElement: function()
    {
        return this._secondElement;
    },

    /**
     * @return {!Element}
     */
    mainElement: function()
    {
        return this.isSidebarSecond() ? this.firstElement() : this.secondElement();
    },

    /**
     * @return {!WebInspector.View}
     */
    mainView: function()
    {
        return this.isSidebarSecond() ? this._firstView : this._secondView;
    },

    /**
     * @return {!Element}
     */
    sidebarElement: function()
    {
        return this.isSidebarSecond() ? this.secondElement() : this.firstElement();
    },

    /**
     * @return {!WebInspector.View}
     */
    sidebarView: function()
    {
        return this.isSidebarSecond() ? this._secondView : this._firstView;
    },

    /**
     * @return {boolean}
     */
    isSidebarSecond: function()
    {
        return this._secondIsSidebar;
    },

    /**
     * @param {boolean} secondIsSidebar
     */
    setSecondIsSidebar: function(secondIsSidebar)
    {
        this.sidebarElement().classList.remove("split-view-sidebar");
        this.mainElement().classList.remove("split-view-main");
        this._secondIsSidebar = secondIsSidebar;
        this.sidebarElement().classList.add("split-view-sidebar");
        this.mainElement().classList.add("split-view-main");
    },

    /**
     * @return {!Element}
     */
    resizerElement: function()
    {
        return this._resizerElement;
    },

    showOnlyFirst: function()
    {
        this._showOnly(this._firstElement, this._secondElement);
        if (this._firstView)
            this._firstView.show(this._firstElement);
        if (this._secondView)
            this._secondView.detach();
        this.doResize();
    },

    showOnlySecond: function()
    {
        this._showOnly(this._secondElement, this._firstElement);
        if (this._firstView)
            this._firstView.detach();
        if (this._secondView)
            this._secondView.show(this._secondElement);
        this.doResize();
    },

    /**
     * @param {!Element} sideA
     * @param {!Element} sideB
     */
    _showOnly: function(sideA, sideB)
    {
        this._cancelAnimation();
        sideA.classList.remove("hidden");
        sideA.classList.add("maximized");
        sideB.classList.add("hidden");
        sideB.classList.remove("maximized");
        this._removeAllLayoutProperties();

        this._isShowingOne = true;
        this._sidebarSize = -1;
        this.setResizable(false);
    },

    _removeAllLayoutProperties: function()
    {
        this.sidebarElement().style.removeProperty("flexBasis");

        this._resizerElement.style.removeProperty("left");
        this._resizerElement.style.removeProperty("right");
        this._resizerElement.style.removeProperty("top");
        this._resizerElement.style.removeProperty("bottom");

        this._resizerElement.style.removeProperty("margin-left");
        this._resizerElement.style.removeProperty("margin-right");
        this._resizerElement.style.removeProperty("margin-top");
        this._resizerElement.style.removeProperty("margin-bottom");
    },

    /**
     * @param {boolean=} animate
     */
    showBoth: function(animate)
    {
        if (!this._isShowingOne)
            animate = false;

        this._cancelAnimation();
        this._firstElement.classList.remove("hidden");
        this._firstElement.classList.remove("maximized");
        this._secondElement.classList.remove("hidden");
        this._secondElement.classList.remove("maximized");

        if (this._firstView)
            this._firstView.show(this._firstElement);
        if (this._secondView)
            this._secondView.show(this._secondElement);

        this._isShowingOne = false;
        this._sidebarSize = -1;
        this.setResizable(true);
        this._updateLayout(animate);
    },

    /**
     * @param {boolean} resizable
     */
    setResizable: function(resizable)
    {
        this._resizable = resizable;
        this._resizerElement.enableStyleClass("hidden", !resizable);
    },

    /**
     * @param {number} size
     * @param {boolean=} ignoreConstraints
     */
    setSidebarSize: function(size, ignoreConstraints)
    {
        this._innerSetSidebarSize(size, ignoreConstraints);
        this._saveSidebarSize();
    },

    /**
     * @return {number}
     */
    sidebarSize: function()
    {
        return Math.max(0, this._sidebarSize);
    },

    /**
     * @return {number}
     */
    totalSize: function()
    {
        if (!this._totalSize)
            this._totalSize = this._isVertical ? this.element.offsetWidth : this.element.offsetHeight;
        return this._totalSize;
    },

    /**
     * @param {number} size
     * @param {boolean=} ignoreConstraints
     * @param {boolean=} animate
     * @param {boolean=} fromOnResize
     */
    _innerSetSidebarSize: function(size, ignoreConstraints, animate, fromOnResize)
    {
        if (this._isShowingOne) {
            this._sidebarSize = size;
            return;
        }

        if (!ignoreConstraints)
            size = this._applyConstraints(size);
        if (this._sidebarSize === size)
            return;

        if (size < 0) {
            // Never apply bad values, fix it upon onResize instead.
            return;
        }

        this._removeAllLayoutProperties();

        var sizeValue;
        if (this._useFraction)
            sizeValue = (size / this.totalSize()) * 100 + "%";
        else
            sizeValue = size + "px";

        this.sidebarElement().style.flexBasis = sizeValue;

        if (!this._resizerElementSize)
            this._resizerElementSize = this._isVertical ? this._resizerElement.offsetWidth : this._resizerElement.offsetHeight;

        var animatedMarginPropertyName;
        // Position resizer.
        if (this._isVertical) {
            if (this._secondIsSidebar) {
                animatedMarginPropertyName = "margin-right";
                this._resizerElement.style.right = sizeValue;
                this._resizerElement.style.marginRight = -this._resizerElementSize / 2 + "px";
            } else {
                animatedMarginPropertyName = "margin-left";
                this._resizerElement.style.left = sizeValue;
                this._resizerElement.style.marginLeft = -this._resizerElementSize / 2 + "px";
            }
        } else {
            if (this._secondIsSidebar) {
                animatedMarginPropertyName = "margin-bottom";
                this._resizerElement.style.bottom = sizeValue;
                this._resizerElement.style.marginBottom = -this._resizerElementSize / 2 + "px";
            } else {
                animatedMarginPropertyName = "margin-top";
                this._resizerElement.style.top = sizeValue;
                this._resizerElement.style.marginTop = -this._resizerElementSize / 2 + "px";
            }
        }

        this._sidebarSize = size;

        if (animate) {
            this._animate(animatedMarginPropertyName, size);
        } else {
            // No need to recalculate this._sidebarSize and this._totalSize again.
            if (!fromOnResize)
                this.doResize();
            this.dispatchEventToListeners(WebInspector.SplitView.Events.SidebarSizeChanged, this.sidebarSize());
        }
    },

    /**
     * @param {string} animatedMarginPropertyName
     * @param {number} size
     */
    _animate: function(animatedMarginPropertyName, size)
    {
        var animationTime = 50;

        var mainView = this.mainView();
        var sidebarView = this.sidebarView();

        // This order of things is important.
        // 1. Resize main element early and force layout.
        this.element.style.setProperty(animatedMarginPropertyName, "-" + size + "px");
        suppressUnused(this._firstElement.offsetWidth);
        suppressUnused(this._secondElement.offsetWidth);

        // 2. Issue onresize to the sidebar element, its size won't change.
        if (sidebarView) {
            sidebarView.onResize();
            sidebarView.doResize();
        }

        // 3. Configure and run animation
        this.element.style.setProperty("transition", animatedMarginPropertyName + " " + animationTime + "ms linear");

        var boundAnimationFrame;
        var startTime;
        /**
         * @this {WebInspector.SplitView}
         */
        function animationFrame()
        {
            delete this._animationFrameHandle;

            if (!startTime) {
                // Kick animation on first frame.
                this.element.style.setProperty(animatedMarginPropertyName, "0");
                startTime = window.performance.now();
            } else if (window.performance.now() < startTime + animationTime) {
                // Process regular animation frame.
                if (mainView) {
                     mainView.onResize();
                     mainView.doResize();
                }
            } else {
                // Complete animation.
                this._cancelAnimation();
                if (mainView) {
                     mainView.onResize();
                     mainView.doResize();
                }
                this.dispatchEventToListeners(WebInspector.SplitView.Events.SidebarSizeChanged, this.sidebarSize());
                return;
            }
            this._animationFrameHandle = window.requestAnimationFrame(boundAnimationFrame);
        }
        boundAnimationFrame = animationFrame.bind(this);
        this._animationFrameHandle = window.requestAnimationFrame(boundAnimationFrame);
    },

    _cancelAnimation: function()
    {
        if (this._animationFrameHandle)
            window.cancelAnimationFrame(this._animationFrameHandle);
        delete this._animationFrameHandle;
        this.element.style.removeProperty("margin-top");
        this.element.style.removeProperty("margin-right");
        this.element.style.removeProperty("margin-bottom");
        this.element.style.removeProperty("margin-left");
        this.element.style.removeProperty("transition");
    },

    /**
     * @param {number=} minWidth
     * @param {number=} minHeight
     */
    setSidebarElementConstraints: function(minWidth, minHeight)
    {
        if (typeof minWidth === "number")
            this._minimumSidebarWidth = minWidth;
        if (typeof minHeight === "number")
            this._minimumSidebarHeight = minHeight;
    },

    /**
     * @param {number=} minWidth
     * @param {number=} minHeight
     */
    setMainElementConstraints: function(minWidth, minHeight)
    {
        if (typeof minWidth === "number")
            this._minimumMainWidth = minWidth;
        if (typeof minHeight === "number")
            this._minimumMainHeight = minHeight;
    },

    /**
     * @param {number} sidebarSize
     * @return {number}
     */
    _applyConstraints: function(sidebarSize)
    {
        const minPadding = 20;
        var totalSize = this.totalSize();
        var minimumSiderbarSizeContraint = this.isVertical() ? this._minimumSidebarWidth : this._minimumSidebarHeight;
        var from = minimumSiderbarSizeContraint || 0;
        var fromInPercents = false;
        if (from && from < 1) {
            fromInPercents = true;
            from = Math.round(totalSize * from);
        }
        if (typeof minimumSiderbarSizeContraint !== "number")
            from = Math.max(from, minPadding);

        var minimumMainSizeConstraint = this.isVertical() ? this._minimumMainWidth : this._minimumMainHeight;
        var minMainSize = minimumMainSizeConstraint || 0;
        var toInPercents = false;
        if (minMainSize && minMainSize < 1) {
            toInPercents = true;
            minMainSize = Math.round(totalSize * minMainSize);
        }
        if (typeof minimumMainSizeConstraint !== "number")
            minMainSize = Math.max(minMainSize, minPadding);

        var to = totalSize - minMainSize;
        if (from <= to)
            return Number.constrain(sidebarSize, from, to);

        // Respect fixed constraints over percents. This will, for example, shrink
        // the sidebar to its minimum size when possible.
        if (!fromInPercents && !toInPercents)
            return -1;
        if (toInPercents && sidebarSize >= from && from < totalSize)
            return from;
        if (fromInPercents && sidebarSize <= to && to < totalSize)
            return to;

        return -1;
    },

    wasShown: function()
    {
        this._updateLayout();
    },

    onResize: function()
    {
        this._updateLayout(false, true);
    },

    /**
     * @param {!MouseEvent} event
     * @return {boolean}
     */
    _startResizerDragging: function(event)
    {
        if (!this._resizable)
            return false;

        this._saveSidebarSizeRecursively();
        this._dragOffset = (this._secondIsSidebar ? this.totalSize() - this._sidebarSize : this._sidebarSize) - (this._isVertical ? event.pageX : event.pageY);
        return true;
    },

    /**
     * @param {!MouseEvent} event
     */
    _resizerDragging: function(event)
    {
        var newOffset = (this._isVertical ? event.pageX : event.pageY) + this._dragOffset;
        var newSize = (this._secondIsSidebar ? this.totalSize() - newOffset : newOffset);
        this.setSidebarSize(newSize);
        event.preventDefault();
    },

    /**
     * @param {!MouseEvent} event
     */
    _endResizerDragging: function(event)
    {
        delete this._dragOffset;
        this._saveSidebarSizeRecursively();
    },

    _saveSidebarSizeRecursively: function()
    {
        /** @this {WebInspector.View} */
        function doSaveSidebarSizeRecursively()
        {
            if (this._saveSidebarSize)
                this._saveSidebarSize();
            this._callOnVisibleChildren(doSaveSidebarSizeRecursively);
        }
        this._saveSidebarSize();
        this._callOnVisibleChildren(doSaveSidebarSizeRecursively);
    },

    hideDefaultResizer: function()
    {
        this.element.classList.add("split-view-no-resizer");
    },

    /**
     * @param {!Element} resizerElement
     */
    installResizer: function(resizerElement)
    {
        resizerElement.addEventListener("mousedown", this._onDragStartBound, false);
        resizerElement.style.setProperty("cursor", this._isVertical ? "ew-resize" : "ns-resize");
        if (this._resizerElements.indexOf(resizerElement) === -1)
            this._resizerElements.push(resizerElement);
    },

    /**
     * @param {!Element} resizerElement
     */
    uninstallResizer: function(resizerElement)
    {
        resizerElement.removeEventListener("mousedown", this._onDragStartBound, false);
        resizerElement.style.removeProperty("cursor");
        this._resizerElements.remove(resizerElement);
    },

    /**
     * @param {?Event} event
     */
    _onDragStart: function(event)
    {
        // Only handle drags of the nodes specified.
        if (this._resizerElements.indexOf(event.target) === -1)
            return;
        WebInspector.elementDragStart(this._startResizerDragging.bind(this), this._resizerDragging.bind(this), this._endResizerDragging.bind(this), this._isVertical ? "ew-resize" : "ns-resize", event);
    },

    /**
     * @return {?WebInspector.Setting}
     */
    _sizeSetting: function()
    {
        if (!this._sidebarSizeSettingName)
            return null;

        var settingName = this._sidebarSizeSettingName + (this._isVertical ? "" : "H");
        if (!WebInspector.settings[settingName])
            WebInspector.settings[settingName] = WebInspector.settings.createSetting(settingName, undefined);

        return WebInspector.settings[settingName];
    },

    /**
     * @return {number}
     */
    _lastSidebarSize: function()
    {
        var sizeSetting = this._sizeSetting();
        var size = sizeSetting ? sizeSetting.get() : 0;
        if (!size)
             size = this._isVertical ? this._savedSidebarWidth : this._savedSidebarHeight;
        if (this._useFraction)
            size *= this.totalSize();
        return size;
    },

    _saveSidebarSize: function()
    {
        var size = this._sidebarSize;
        if (size < 0)
            return;

        if (this._useFraction)
            size /= this.totalSize();

        if (this._isVertical)
            this._savedSidebarWidth = size;
        else
            this._savedSidebarHeight = size;

        var sizeSetting = this._sizeSetting();
        if (sizeSetting)
            sizeSetting.set(size);
    },

    __proto__: WebInspector.View.prototype
}
