/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#ifndef Profile_h
#define Profile_h

#include "ProfileNode.h"
#include <kjs/ustring.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace KJS {

    class ExecState;

    class Profile : public RefCounted<Profile> {
    public:
        static PassRefPtr<Profile> create(const UString& title);
        virtual ~Profile();

        const UString& title() const { return m_title; }
        ProfileNode* head() const { return m_head.get(); }
        void setHead(PassRefPtr<ProfileNode> head) { m_head = head; }
        double totalTime() const { return m_head->totalTime(); }

        void forEach(void (ProfileNode::*)());
        void sortTotalTimeDescending() { forEach(&ProfileNode::sortTotalTimeDescending); }
        void sortTotalTimeAscending() { forEach(&ProfileNode::sortTotalTimeAscending); }
        void sortSelfTimeDescending() { forEach(&ProfileNode::sortSelfTimeDescending); }
        void sortSelfTimeAscending() { forEach(&ProfileNode::sortSelfTimeAscending); }
        void sortCallsDescending() { forEach(&ProfileNode::sortCallsDescending); }
        void sortCallsAscending() { forEach(&ProfileNode::sortCallsAscending); }
        void sortFunctionNameDescending() { forEach(&ProfileNode::sortFunctionNameDescending); }
        void sortFunctionNameAscending() { forEach(&ProfileNode::sortFunctionNameAscending); }

        void focus(const ProfileNode*);
        void exclude(const ProfileNode*);
        void restoreAll();

        virtual Profile* heavyProfile() = 0;
        virtual Profile* treeProfile() = 0; 

#ifndef NDEBUG
        void debugPrintData() const;
        void debugPrintDataSampleStyle() const;
#endif

    protected:
        Profile(const UString& title);

    private:
        void removeProfileStart();
        void removeProfileEnd();
 
        UString m_title;
        RefPtr<ProfileNode> m_head;
    };

} // namespace KJS

#endif // Profile_h
