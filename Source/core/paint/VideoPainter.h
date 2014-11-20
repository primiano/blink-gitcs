// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VideoPainter_h
#define VideoPainter_h

namespace blink {

struct PaintInfo;
class LayoutPoint;
class RenderVideo;

class VideoPainter {
public:
    VideoPainter(RenderVideo& renderVideo) : m_renderVideo(renderVideo) { }

    void paintReplaced(const PaintInfo&, const LayoutPoint&);

private:

    RenderVideo& m_renderVideo;
};

} // namespace blink

#endif // VideoPainter_h
