# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'includes': [
        'custom/custom.gypi',
    ],
    'variables': {
        'bindings_modules_v8_files': [
            '<@(bindings_modules_v8_custom_files)',
        ],
    },
}