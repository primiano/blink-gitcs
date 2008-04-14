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

#ifndef StorageArea_h
#define StorageArea_h

#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    class Frame;
    class Page;
    class SecurityOrigin;
    class StorageMap;
    class String;
    typedef int ExceptionCode;

    class StorageArea : public RefCounted<StorageArea> {
    public:
        virtual ~StorageArea();
        
        static PassRefPtr<StorageArea> create(SecurityOrigin*, Page*);
        PassRefPtr<StorageArea> copy(SecurityOrigin*, Page*);
        
        unsigned length() const;
        String key(unsigned index, ExceptionCode&) const;
        String getItem(const String&) const;
        void setItem(const String& key, const String& value, ExceptionCode&, Frame* sourceFrame);
        void removeItem(const String&, Frame* sourceFrame);

        bool contains(const String& key) const;

    private:
        StorageArea(SecurityOrigin*, Page*);
        StorageArea(SecurityOrigin*, Page*, PassRefPtr<StorageMap>);
        
        void dispatchStorageEvent(const String& key, const String& oldValue, const String& newValue, Frame* sourceFrame);
        
        Page* m_page;
        RefPtr<SecurityOrigin> m_securityOrigin;
        RefPtr<StorageMap> m_storageMap;
    };

} // namespace WebCore

#endif // StorageArea_h
