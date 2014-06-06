# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Generate IDL interfaces info for modules, used to generate bindings
#
# Design doc: http://www.chromium.org/developers/design-documents/idl-build

{
  'includes': [
    # ../.. == Source
    '../../bindings/bindings.gypi',
    '../../bindings/core/core.gypi',
    '../../bindings/scripts/scripts.gypi',
    '../../build/scripts/scripts.gypi',  # FIXME: Needed for event files, should be in modules, not bindings_modules http://crbug.com/358074
    '../../modules/modules.gypi',
    'generated.gypi',
    'idl.gypi',
    'modules.gypi',
  ],

  'targets': [
################################################################################
  {
    # FIXME: Should be in modules, not bindings_modules http://crbug.com/358074
    'target_name': 'modules_event_generated',
    'type': 'none',
    'actions': [
      {
        'action_name': 'event_interfaces',
        'variables': {
          'event_idl_files': [
            '<@(modules_event_idl_files)',
          ],
          'event_idl_files_list':
              '<|(event_idl_files_list.tmp <@(event_idl_files))',
        },
        'inputs': [
          '<(bindings_scripts_dir)/generate_event_interfaces.py',
          '<(bindings_scripts_dir)/utilities.py',
          '<(event_idl_files_list)',
          '<@(event_idl_files)',
        ],
        'outputs': [
          '<(blink_modules_output_dir)/EventModulesInterfaces.in',
        ],
        'action': [
          'python',
          '<(bindings_scripts_dir)/generate_event_interfaces.py',
          '--event-idl-files-list',
          '<(event_idl_files_list)',
          '--event-interfaces-file',
          '<(blink_modules_output_dir)/EventModulesInterfaces.in',
          '--write-file-only-if-changed',
          '<(write_file_only_if_changed)',
          '--suffix',
          'Modules',
        ],
      },
      {
        'action_name': 'EventModulesFactory',
        'inputs': [
          '<@(make_event_factory_files)',
          '<(blink_modules_output_dir)/EventModulesInterfaces.in',
        ],
        'outputs': [
          '<(blink_modules_output_dir)/EventModules.cpp',
          '<(blink_modules_output_dir)/EventModulesHeaders.h',
          '<(blink_modules_output_dir)/EventModulesInterfaces.h',
        ],
        'action': [
          'python',
          '../../build/scripts/make_event_factory.py',
          '<(blink_modules_output_dir)/EventModulesInterfaces.in',
          '--output_dir',
          '<(blink_modules_output_dir)',
        ],
      },
      {
        'action_name': 'EventModulesNames',
        'inputs': [
          '<@(make_names_files)',
          '<(blink_modules_output_dir)/EventModulesInterfaces.in',
        ],
        'outputs': [
          '<(blink_modules_output_dir)/EventModulesNames.cpp',
          '<(blink_modules_output_dir)/EventModulesNames.h',
        ],
        'action': [
          'python',
          '../../build/scripts/make_names.py',
          '<(blink_modules_output_dir)/EventModulesInterfaces.in',
          '--output_dir',
          '<(blink_modules_output_dir)',
        ],
      },
      {
        'action_name': 'EventTargetModulesFactory',
        'inputs': [
          '<@(make_event_factory_files)',
          '../../modules/EventTargetModulesFactory.in',
        ],
        'outputs': [
          '<(blink_modules_output_dir)/EventTargetModulesHeaders.h',
          '<(blink_modules_output_dir)/EventTargetModulesInterfaces.h',
        ],
        'action': [
          'python',
          '../../build/scripts/make_event_factory.py',
          '../../modules/EventTargetModulesFactory.in',
          '--output_dir',
          '<(blink_modules_output_dir)',
        ],
      },
      {
        'action_name': 'EventTargetModulesNames',
        'inputs': [
          '<@(make_names_files)',
          '../../modules/EventTargetModulesFactory.in',
        ],
        'outputs': [
          '<(blink_modules_output_dir)/EventTargetModulesNames.cpp',
          '<(blink_modules_output_dir)/EventTargetModulesNames.h',
        ],
        'action': [
          'python',
          '../../build/scripts/make_names.py',
          '../../modules/EventTargetModulesFactory.in',
          '--output_dir',
          '<(blink_modules_output_dir)',
        ],
      },
    ],
  },
################################################################################
  {
    'target_name': 'modules_global_objects',
    'type': 'none',
    'dependencies': [
        '../core/generated.gyp:core_global_objects',
    ],
    'actions': [{
      'action_name': 'compute_modules_global_objects',
      'inputs': [
        '<(bindings_scripts_dir)/compute_global_objects.py',
        '<(bindings_scripts_dir)/utilities.py',
        # Only look in main IDL files (exclude dependencies and testing,
        # which should not define global objects).
        '<(modules_idl_files_list)',
        '<@(modules_idl_files)',
      ],
      'outputs': [
        '<(bindings_modules_output_dir)/GlobalObjectsModules.pickle',
      ],
      'action': [
        'python',
        '<(bindings_scripts_dir)/compute_global_objects.py',
        '--idl-files-list',
        '<(modules_idl_files_list)',
        '--write-file-only-if-changed',
        '<(write_file_only_if_changed)',
        '--',
        '<(bindings_core_output_dir)/GlobalObjectsCore.pickle',
        '<(bindings_modules_output_dir)/GlobalObjectsModules.pickle',
       ],
       'message': 'Computing global objects in modules',
      }]
  },
################################################################################
  {
    # Global constructors for global objects in modules (ServiceWorker)
    # but interfaces in core.
    'target_name': 'modules_core_global_constructors_idls',
    'type': 'none',
    'dependencies': [
        'modules_global_objects',
    ],
    'actions': [{
      'action_name': 'generate_modules_core_global_constructors_idls',
      'inputs': [
        '<(bindings_scripts_dir)/generate_global_constructors.py',
        '<(bindings_scripts_dir)/utilities.py',
        # Only includes main IDL files (exclude dependencies and testing,
        # which should not appear on global objects).
        '<(core_idl_files_list)',
        '<@(core_idl_files)',
        '<(bindings_modules_output_dir)/GlobalObjectsModules.pickle',
      ],
      'outputs': [
        '<@(modules_core_global_constructors_generated_idl_files)',
        '<@(modules_core_global_constructors_generated_header_files)',
      ],
      'action': [
        'python',
        '<(bindings_scripts_dir)/generate_global_constructors.py',
        '--idl-files-list',
        '<(core_idl_files_list)',
        '--global-objects-file',
        '<(bindings_modules_output_dir)/GlobalObjectsModules.pickle',
        '--write-file-only-if-changed',
        '<(write_file_only_if_changed)',
        '--',
        'ServiceWorkerGlobalScope',
        '<(blink_modules_output_dir)/ServiceWorkerGlobalScopeCoreConstructors.idl',
       ],
       'message':
         'Generating IDL files for constructors for interfaces in core, on global objects from modules',
      }]
  },
################################################################################
  {
    'target_name': 'modules_global_constructors_idls',
    'type': 'none',
    'dependencies': [
      'modules_global_objects',
    ],
    'actions': [{
      'action_name': 'generate_modules_global_constructors_idls',
      'inputs': [
        '<(bindings_scripts_dir)/generate_global_constructors.py',
        '<(bindings_scripts_dir)/utilities.py',
        # Only includes main IDL files (exclude dependencies and testing,
        # which should not appear on global objects).
        '<(modules_idl_files_list)',
        '<@(modules_idl_files)',
        '<(bindings_modules_output_dir)/GlobalObjectsModules.pickle',
      ],
      'outputs': [
        '<@(modules_global_constructors_generated_idl_files)',
        '<@(modules_global_constructors_generated_header_files)',
      ],
      'action': [
        'python',
        '<(bindings_scripts_dir)/generate_global_constructors.py',
        '--idl-files-list',
        '<(modules_idl_files_list)',
        '--global-objects-file',
        '<(bindings_modules_output_dir)/GlobalObjectsModules.pickle',
        '--write-file-only-if-changed',
        '<(write_file_only_if_changed)',
        '--',
        'Window',
        '<(blink_modules_output_dir)/WindowModulesConstructors.idl',
        'SharedWorkerGlobalScope',
        '<(blink_modules_output_dir)/SharedWorkerGlobalScopeModulesConstructors.idl',
        'DedicatedWorkerGlobalScope',
        '<(blink_modules_output_dir)/DedicatedWorkerGlobalScopeModulesConstructors.idl',
        'ServiceWorkerGlobalScope',
        '<(blink_modules_output_dir)/ServiceWorkerGlobalScopeModulesConstructors.idl',
       ],
       'message':
         'Generating IDL files for constructors on global objects from modules',
      }]
  },
################################################################################
  {
    'target_name': 'interfaces_info_individual_modules',
    'type': 'none',
    'dependencies': [
      'modules_core_global_constructors_idls',
      'modules_global_constructors_idls',
    ],
    'actions': [{
      'action_name': 'compute_interfaces_info_individual_modules',
      'inputs': [
        '<(bindings_scripts_dir)/compute_interfaces_info_individual.py',
        '<(bindings_scripts_dir)/utilities.py',
        '<(modules_static_idl_files_list)',
        '<@(modules_static_idl_files)',
        '<@(modules_generated_idl_files)',
      ],
      'outputs': [
        '<(bindings_modules_output_dir)/InterfacesInfoModulesIndividual.pickle',
      ],
      'action': [
        'python',
        '<(bindings_scripts_dir)/compute_interfaces_info_individual.py',
        '--component-dir',
        'modules',
        '--idl-files-list',
        '<(modules_static_idl_files_list)',
        '--interfaces-info-file',
        '<(bindings_modules_output_dir)/InterfacesInfoModulesIndividual.pickle',
        '--write-file-only-if-changed',
        '<(write_file_only_if_changed)',
        '--',
        # Generated files must be passed at command line
        '<@(modules_generated_idl_files)',
      ],
      'message': 'Computing global information about individual IDL files',
      }]
  },
################################################################################
  {
    'target_name': 'interfaces_info',
    'type': 'none',
    'dependencies': [
        '../core/generated.gyp:interfaces_info_individual_core',
        'interfaces_info_individual_modules',
    ],
    'actions': [{
      'action_name': 'compute_interfaces_info_overall',
      'inputs': [
        '<(bindings_scripts_dir)/compute_interfaces_info_overall.py',
        '<(bindings_core_output_dir)/InterfacesInfoCoreIndividual.pickle',
        '<(bindings_modules_output_dir)/InterfacesInfoModulesIndividual.pickle',
      ],
      'outputs': [
        '<(bindings_modules_output_dir)/InterfacesInfoModules.pickle',
      ],
      'action': [
        'python',
        '<(bindings_scripts_dir)/compute_interfaces_info_overall.py',
        '--write-file-only-if-changed',
        '<(write_file_only_if_changed)',
        '--',
        '<(bindings_core_output_dir)/InterfacesInfoCoreIndividual.pickle',
        '<(bindings_modules_output_dir)/InterfacesInfoModulesIndividual.pickle',
        '<(bindings_modules_output_dir)/InterfacesInfoModules.pickle',
      ],
      'message': 'Computing overall global information about IDL files',
      }]
  },
################################################################################
  ],  # targets
}
