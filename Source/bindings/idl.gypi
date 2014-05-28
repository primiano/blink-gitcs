# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# IDL file lists; see: http://www.chromium.org/developers/web-idl-interfaces

{
  'includes': [
    'bindings.gypi',
    '../core/core.gypi',
    '../modules/modules.gypi',
  ],

  'variables': {
    # Main interface IDL files (excluding dependencies and testing)
    # are included as properties on global objects, and in aggregate bindings
    # FIXME: split into core vs. modules http://crbug.com/358074
    'main_interface_idl_files': [
      '<@(core_idl_files)',
      '<@(modules_idl_files)',
    ],
    # Write lists of main IDL files to a file, so that the command lines don't
    # exceed OS length limits.
    'main_interface_idl_files_list': '<|(main_interface_idl_files_list.tmp <@(main_interface_idl_files))',

    # Global constructors
    # FIXME: Split into core vs. modules http://crbug.com/358074
    'generated_global_constructors_idl_files': [
      '<(blink_output_dir)/WindowConstructors.idl',
      '<(blink_output_dir)/SharedWorkerGlobalScopeConstructors.idl',
      '<(blink_output_dir)/DedicatedWorkerGlobalScopeConstructors.idl',
      '<(blink_output_dir)/ServiceWorkerGlobalScopeConstructors.idl',
    ],

    'generated_global_constructors_header_files': [
      '<(blink_output_dir)/WindowConstructors.h',
      '<(blink_output_dir)/SharedWorkerGlobalScopeConstructors.h',
      '<(blink_output_dir)/DedicatedWorkerGlobalScopeConstructors.h',
      '<(blink_output_dir)/ServiceWorkerGlobalScopeConstructors.h',
    ],
  },
}
