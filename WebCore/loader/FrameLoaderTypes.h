/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef FrameLoaderTypes_h
#define FrameLoaderTypes_h

namespace WebCore {

    enum FrameState {
        FrameStateProvisional,
        // This state indicates we are ready to commit to a page,
        // which means the view will transition to use the new data source.
        FrameStateCommittedPage,
        FrameStateComplete
    };

    enum PolicyAction {
        PolicyUse,
        PolicyDownload,
        PolicyIgnore,
    };

    // NOTE: Keep in sync with WebKit/mac/WebView/WebFramePrivate.h and WebKit/win/Interfaces/IWebFramePrivate.idl
    enum FrameLoadType {
        FrameLoadTypeStandard,
        FrameLoadTypeBack,
        FrameLoadTypeForward,
        FrameLoadTypeIndexedBackForward, // a multi-item hop in the backforward list
        FrameLoadTypeReload,
        // Skipped value: 'FrameLoadTypeReloadAllowingStaleData', still present in mac/win public API. Ready to be reused
        FrameLoadTypeSame = FrameLoadTypeReload + 2, // user loads same URL again (but not reload button)
        FrameLoadTypeRedirectWithLockedBackForwardList, // FIXME: Merge "lockBackForwardList", "lockHistory", "quickRedirect" and "clientRedirect" into a single concept of redirect.
        FrameLoadTypeReplace,
        FrameLoadTypeReloadFromOrigin,
        FrameLoadTypeBackWMLDeckNotAccessible
    };

    enum NavigationType {
        NavigationTypeLinkClicked,
        NavigationTypeFormSubmitted,
        NavigationTypeBackForward,
        NavigationTypeReload,
        NavigationTypeFormResubmitted,
        NavigationTypeOther
    };

    enum DatabasePolicy {
        DatabasePolicyStop,    // The database thread should be stopped and database connections closed.
        DatabasePolicyContinue
    };

    enum ObjectContentType {
        ObjectContentNone,
        ObjectContentImage,
        ObjectContentFrame,
        ObjectContentNetscapePlugin,
        ObjectContentOtherPlugin
    };
    
    enum UnloadEventPolicy {
        UnloadEventPolicyNone,
        UnloadEventPolicyUnloadOnly,
        UnloadEventPolicyUnloadAndPageHide
    };

    enum ReferrerPolicy {
        SendReferrer,
        NoReferrer
    };
    
    enum SandboxFlag {
        SandboxNone = 0,
        SandboxNavigation = 1,
        SandboxPlugins = 1 << 1,
        SandboxOrigin = 1 << 2,
        SandboxForms = 1 << 3,
        SandboxScripts = 1 << 4,
        SandboxAll = -1 // Mask with all bits set to 1.
    };

    typedef unsigned SandboxFlags;
}

#endif
