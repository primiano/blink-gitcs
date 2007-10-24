/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

WebInspector.ImagePanel = function(resource)
{
    WebInspector.ResourcePanel.call(this, resource);

    this.element.addStyleClass("image");

    var container = document.createElement("div");
    container.className = "image";
    this.element.appendChild(container);

    this.imagePreviewElement = document.createElement("img");
    this.imagePreviewElement.setAttribute("src", this.resource.url);

    container.appendChild(this.imagePreviewElement);

    container = document.createElement("div");
    container.className = "info";
    this.element.appendChild(container);

    var imageNameElement = document.createElement("h1");
    imageNameElement.className = "title";
    imageNameElement.textContent = this.resource.displayName;
    container.appendChild(imageNameElement);

    var infoListElement = document.createElement("dl");
    infoListElement.className = "infoList";

    var imageProperties = [
        { name: "Dimensions", value: this.imagePreviewElement.naturalWidth + " \u00D7 " + this.imagePreviewElement.height },
        { name: "File size", value: (this.resource.contentLength / 1024).toPrecision(2) + "KB" },
        { name: "MIME type", value: this.resource.mimeType }
    ];

    var listHTML = '';
    for (var i = 0; i < imageProperties.length; ++i)
        listHTML += "<dt>" + imageProperties[i].name + "</dt><dd>" + imageProperties[i].value + "</dd>";

    infoListElement.innerHTML = listHTML;
    container.appendChild(infoListElement);
}

WebInspector.ImagePanel.prototype = {
    
}

WebInspector.ImagePanel.prototype.__proto__ = WebInspector.ResourcePanel.prototype;
