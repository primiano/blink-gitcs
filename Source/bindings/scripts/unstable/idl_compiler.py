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

FIXME: Not currently used in build.
This is a rewrite of the Perl IDL compiler in Python, but is not complete.
Once it is complete, we will switch all IDL files over to Python at once.
Until then, please work on the Perl IDL compiler.
For details, see bug http://crbug.com/239771
"""

import optparse
import os
import cPickle as pickle
import sys

import code_generator_v8
import idl_reader

def parse_options():
    parser = optparse.OptionParser()
    parser.add_option('--idl-attributes-file')
    parser.add_option('--output-directory')
    parser.add_option('--interfaces-info-file')
    parser.add_option('--write-file-only-if-changed', type='int')
    # ensure output comes last, so command line easy to parse via regexes
    parser.disable_interspersed_args()

    options, args = parser.parse_args()
    if options.output_directory is None:
        parser.error('Must specify output directory using --output-directory.')
    if len(args) != 1:
        parser.error('Must specify exactly 1 input file as argument, but %d given.' % len(args))
    idl_filename = os.path.realpath(args[0])
    return options, idl_filename


def main():
    options, idl_filename = parse_options()
    basename = os.path.basename(idl_filename)
    interface_name, _ = os.path.splitext(basename)
    output_directory = options.output_directory

    interfaces_info_filename = options.interfaces_info_file
    if interfaces_info_filename:
        with open(interfaces_info_filename) as interfaces_info_file:
            interfaces_info = pickle.load(interfaces_info_file)
    else:
        interfaces_info = None

    reader = idl_reader.IdlReader(interfaces_info, options.idl_attributes_file, output_directory)
    definitions = reader.read_idl_definitions(idl_filename)
    code_generator_v8.write_header_and_cpp(definitions, interface_name, interfaces_info, output_directory)


if __name__ == '__main__':
    sys.exit(main())
