/* On Unix systems config.in is converted by configure into config.h. PCRE is
written in Standard C, but there are a few non-standard things it can cope
with, allowing it to run on SunOS4 and other "close to standard" systems.

On a non-Unix system you should just copy this file into config.h, and set up
the macros the way you need them. You should normally change the definitions of
HAVE_STRERROR and HAVE_MEMMOVE to 1. Unfortunately, because of the way autoconf
works, these cannot be made the defaults. If your system has bcopy() and not
memmove(), change the definition of HAVE_BCOPY instead of HAVE_MEMMOVE. If your
system has neither bcopy() nor memmove(), leave them both as 0; an emulation
function will be used. */

/* If you are compiling for a system that uses EBCDIC instead of ASCII
character codes, define this macro as 1. On systems that can use "configure",
this can be done via --enable-ebcdic. */

#define EBCDIC 0

/* If you are compiling for a system that needs some magic to be inserted
before the definition of an exported function, define this macro to contain the
relevant magic. It apears at the start of every exported function. */

#define PCRE_EXPORT

/* Define to empty if the "const" keyword does not work. */

#undef const

/* Define to "unsigned" if <stddef.h> doesn't define size_t. */

#undef size_t

/* The following two definitions are mainly for the benefit of SunOS4, which
doesn't have the strerror() or memmove() functions that should be present in
all Standard C libraries. The macros HAVE_STRERROR and HAVE_MEMMOVE should
normally be defined with the value 1 for other systems, but unfortunately we
can't make this the default because "configure" files generated by autoconf
will only change 0 to 1; they won't change 1 to 0 if the functions are not
found. */

#define HAVE_STRERROR 1
#define HAVE_MEMMOVE  1

/* There are some non-Unix systems that don't even have bcopy(). If this macro
is false, an emulation is used. If HAVE_MEMMOVE is set to 1, the value of
HAVE_BCOPY is not relevant. */

#define HAVE_BCOPY    1

/* The value of NEWLINE determines the newline character. The default is to
leave it up to the compiler, but some sites want to force a particular value.
On Unix systems, "configure" can be used to override this default. */

#define NEWLINE '\n'

/* The value of LINK_SIZE determines the number of bytes used to store
links as offsets within the compiled regex. The default is 2, which allows for
compiled patterns up to 64K long. This covers the vast majority of cases.
However, PCRE can also be compiled to use 3 or 4 bytes instead. This allows for
longer patterns in extreme cases. On Unix systems, "configure" can be used to
override this default. */

#define LINK_SIZE   2

/* The value of MATCH_LIMIT determines the default number of times the match()
function can be called during a single execution of pcre_exec(). (There is a
runtime method of setting a different limit.) The limit exists in order to
catch runaway regular expressions that take for ever to determine that they do
not match. The default is set very large so that it does not accidentally catch
legitimate cases. On Unix systems, "configure" can be used to override this
default default. */

#define MATCH_LIMIT 10000000

/* When calling PCRE via the POSIX interface, additional working storage is
required for holding the pointers to capturing substrings because PCRE requires
three integers per substring, whereas the POSIX interface provides only two. If
the number of expected substrings is small, the wrapper function uses space on
the stack, because this is faster than using malloc() for each call. The
threshold above which the stack is no longer use is defined by POSIX_MALLOC_
THRESHOLD. On Unix systems, "configure" can be used to override this default.
*/

#define POSIX_MALLOC_THRESHOLD 10

/* PCRE uses recursive function calls to handle backtracking while matching.
This can sometimes be a problem on systems that have stacks of limited size.
Define NO_RECURSE to get a version that doesn't use recursion in the match()
function; instead it creates its own stack by steam using pcre_recurse_malloc
to get memory. For more detail, see comments and other stuff just above the
match() function. On Unix systems, "configure" can be used to set this in the
Makefile (use --disable-stack-for-recursion). */

#define NO_RECURSE

/* End */

#define SUPPORT_UCP 1
#define SUPPORT_UTF8 1

#define JAVASCRIPT 1
