/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/

/* PCRE is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language.

                       Written by Philip Hazel
           Copyright (c) 1997-2006 University of Cambridge

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the University of Cambridge nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/


/*************************************************
*           Create PCRE character tables         *
*************************************************/

/* This function builds a set of character tables for use by PCRE and returns
a pointer to them. They are build using the ctype functions, and consequently
their contents will depend upon the current locale setting. When compiled as
part of the library, the store is obtained via pcre_malloc(), but when compiled
inside dftables, use malloc().

Arguments:   none
Returns:     pointer to the contiguous block of data
*/

static const unsigned char *
pcre_maketables(void)
{
unsigned char *yield, *p;
int i;

#ifndef DFTABLES
yield = (unsigned char*)(pcre_malloc)(tables_length);
#else
yield = (unsigned char*)malloc(tables_length);
#endif

if (yield == NULL) return NULL;
p = yield;

/* First comes the lower casing table */

for (i = 0; i < 256; i++) *p++ = tolower(i);

/* Next the case-flipping table */

for (i = 0; i < 256; i++) *p++ = islower(i)? toupper(i) : tolower(i);

/* Then the character class tables. Don't try to be clever and save effort on
exclusive ones - in some locales things may be different. Note that the table
for "space" includes everything "isspace" gives, including VT in the default
locale. This makes it work for the POSIX class [:space:]. Note also that it is
possible for a character to be alnum or alpha without being lower or upper,
such as "male and female ordinals" (\xAA and \xBA) in the fr_FR locale (at
least under Debian Linux's locales as of 12/2005). So we must test for alnum
specially. */

memset(p, 0, cbit_length);
for (i = 0; i < 128; i++)
  {
  if (isdigit(i)) p[cbit_digit  + i/8] |= 1 << (i&7);
  if (isalnum(i)) p[cbit_word   + i/8] |= 1 << (i&7);
  if (i == '_')   p[cbit_word   + i/8] |= 1 << (i&7);
  if (isspace(i)) p[cbit_space  + i/8] |= 1 << (i&7);
  }
p += cbit_length;

/* Finally, the character type table. In this, we exclude VT from the white
space chars, because Perl doesn't recognize it as such for \s and for comments
within regexes. */

for (i = 0; i < 128; i++)
  {
  int x = 0;
  if (isspace(i)) x += ctype_space;
  if (isdigit(i)) x += ctype_digit;
  if (isxdigit(i)) x += ctype_xdigit;
  if (isalnum(i) || i == '_') x += ctype_word;
  *p++ = x;
  }

return yield;
}

/* End of pcre_maketables.c */
