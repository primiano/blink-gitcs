#!/usr/bin/python
# Copyright (C) 2013 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Compile an .idl file to Blink V8 bindings (.h and .cpp files).

Design doc: http://www.chromium.org/developers/design-documents/idl-compiler
"""

from optparse import OptionParser
import os
import cPickle as pickle
import sys

from code_generator_v8 import CodeGeneratorV8
from idl_reader import IdlReader
# from utilities import write_file  # FIXME: import once in same directory


def parse_options():
    parser = OptionParser()
    parser.add_option('--idl-attributes-file')
    parser.add_option('--output-directory')
    parser.add_option('--interfaces-info-file')
    parser.add_option('--write-file-only-if-changed', type='int')
    # ensure output comes last, so command line easy to parse via regexes
    parser.disable_interspersed_args()

    options, args = parser.parse_args()
    if options.output_directory is None:
        parser.error('Must specify output directory using --output-directory.')
    options.write_file_only_if_changed = bool(options.write_file_only_if_changed)
    if len(args) != 1:
        parser.error('Must specify exactly 1 input file as argument, but %d given.' % len(args))
    idl_filename = os.path.realpath(args[0])
    return options, idl_filename


# FIXME: import from utilities once moved into same directory (lines vs. text)
def write_file(new_text, destination_filename, only_if_changed):
    if only_if_changed and os.path.isfile(destination_filename):
        with open(destination_filename) as destination_file:
            if destination_file.read() == new_text:
                return
    with open(destination_filename, 'w') as destination_file:
        destination_file.write(new_text)


class IdlCompiler(object):
    def __init__(self, output_directory, interfaces_info, idl_attributes_file, only_if_changed=False):
        self.output_directory = output_directory
        self.only_if_changed = only_if_changed
        self.reader = IdlReader(interfaces_info, idl_attributes_file, output_directory)
        self.code_generator = CodeGeneratorV8(interfaces_info, output_directory)

    def compile(self, idl_filename):
        basename = os.path.basename(idl_filename)
        interface_name, _ = os.path.splitext(basename)

        definitions = self.reader.read_idl_definitions(idl_filename)
        header_text, cpp_text = self.code_generator.generate_code(definitions, interface_name)

        header_filename = os.path.join(self.output_directory,
                                       'V8%s.h' % interface_name)
        cpp_filename = os.path.join(self.output_directory,
                                    'V8%s.cpp' % interface_name)
        write_file(header_text, header_filename, self.only_if_changed)
        write_file(cpp_text, cpp_filename, self.only_if_changed)


def main():
    options, idl_filename = parse_options()

    interfaces_info_filename = options.interfaces_info_file
    if interfaces_info_filename:
        with open(interfaces_info_filename) as interfaces_info_file:
            interfaces_info = pickle.load(interfaces_info_file)
    else:
        interfaces_info = None

    idl_compiler = IdlCompiler(options.output_directory,
                               interfaces_info,
                               options.idl_attributes_file,
                               options.write_file_only_if_changed)
    idl_compiler.compile(idl_filename)

if __name__ == '__main__':
    sys.exit(main())
