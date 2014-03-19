// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NavigatorBattery_h
#define NavigatorBattery_h

#include "core/frame/Navigator.h"
#include "heap/Handle.h"
#include "platform/Supplementable.h"

namespace WebCore {

class BatteryManager;
class Navigator;

class NavigatorBattery FINAL : public NoBaseWillBeGarbageCollectedFinalized<NavigatorBattery>, public WillBeHeapSupplement<Navigator> {
    WILL_BE_USING_GARBAGE_COLLECTED_MIXIN(NavigatorBattery);
public:
    virtual ~NavigatorBattery();

    static NavigatorBattery& from(Navigator&);

    static BatteryManager* battery(Navigator&);
    BatteryManager* batteryManager(Navigator&);

    void trace(Visitor*);

private:
    NavigatorBattery();
    static const char* supplementName();

    RefPtrWillBeMember<BatteryManager> m_batteryManager;
};

} // namespace WebCore

#endif // NavigatorBattery_h
