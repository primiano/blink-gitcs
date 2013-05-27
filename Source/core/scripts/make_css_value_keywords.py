#!/usr/bin/env python

import os.path
import re
import subprocess
import sys

from in_file import InFile
import in_generator
import license


HEADER_TEMPLATE = """
%(license)s

#ifndef %(class_name)s_h
#define %(class_name)s_h

#include <string.h>

namespace WebCore {

enum CSSValueID {
    CSSValueInvalid = 0,
%(value_keyword_enums)s
};

const int numCSSValueKeywords = %(value_keywords_count)d;
const size_t maxCSSValueKeywordLength = %(max_value_keyword_length)d;

const char* getValueName(unsigned short id);

} // namespace WebCore

#endif // %(class_name)s_h
"""

GPERF_TEMPLATE = """
%%{
%(license)s

#include "config.h"
#include "%(class_name)s.h"
#include "core/platform/HashTools.h"
#include <string.h>

namespace WebCore {
const char* const valueList[] = {
"",
%(value_keyword_strings)s
0
};

%%}
%%struct-type
struct Value;
%%omit-struct-type
%%language=C++
%%readonly-tables
%%compare-strncmp
%%define class-name %(class_name)sHash
%%define lookup-function-name findValueImpl
%%define hash-function-name value_hash_function
%%define word-array-name value_word_list
%%enum
%%%%
%(value_keyword_to_enum_map)s
%%%%
const Value* findValue(register const char* str, register unsigned int len)
{
    return CSSValueKeywordsHash::findValueImpl(str, len);
}

const char* getValueName(unsigned short id)
{
    if (id >= numCSSValueKeywords || id <= 0)
        return 0;
    return valueList[id];
}

} // namespace WebCore
"""


class CSSValueKeywordsWriter(in_generator.Writer):
    class_name = "CSSValueKeywords"
    defaults = {
        'condition': None,
    }

    def __init__(self, file_paths, enabled_conditions):
        in_generator.Writer.__init__(self, file_paths, enabled_conditions)

        all_properties = self.in_file.name_dictionaries
        self._value_keywords = filter(lambda property: not property['condition'] or property['condition'] in self._enabled_conditions, all_properties)
        first_property_id = 1
        for offset, property in enumerate(self._value_keywords):
            property['name'] = property['name'].lower()
            property['enum_name'] = self._enum_name_from_value_keyword(property['name'])
            property['enum_value'] = first_property_id + offset

    def _enum_name_from_value_keyword(self, value_keyword):
        return "CSSValue" + "".join(w.capitalize() for w in value_keyword.split("-"))

    def _enum_declaration(self, property):
        return "    %(enum_name)s = %(enum_value)s," % property

    def generate_header(self):
        return HEADER_TEMPLATE % {
            'license': license.license_for_generated_cpp(),
            'class_name': self.class_name,
            'value_keyword_enums': "\n".join(map(self._enum_declaration, self._value_keywords)),
            'value_keywords_count': len(self._value_keywords),
            'max_value_keyword_length': reduce(max, map(len, map(lambda property: property['name'], self._value_keywords))),
        }

    def generate_implementation(self):
        gperf_input = GPERF_TEMPLATE % {
            'license': license.license_for_generated_cpp(),
            'class_name': self.class_name,
            'value_keyword_strings': '\n'.join(map(lambda property: '    "%(name)s",' % property, self._value_keywords)),
            'value_keyword_to_enum_map': '\n'.join(map(lambda property: '%(name)s, %(enum_name)s' % property, self._value_keywords)),
        }
        # FIXME: If we could depend on Python 2.7, we would use subprocess.check_output
        gperf_args = ['gperf', '--key-positions=*', '-D', '-n', '-s', '2']
        gperf = subprocess.Popen(gperf_args, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        return gperf.communicate(gperf_input)[0]


if __name__ == "__main__":
    in_generator.Maker(CSSValueKeywordsWriter).main(sys.argv)
