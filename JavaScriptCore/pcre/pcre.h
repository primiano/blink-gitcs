/*************************************************
*       Perl-Compatible Regular Expressions      *
*  extended to UTF-16 for use in JavaScriptCore  *
*************************************************/

/* Copyright (c) 1997-2001 University of Cambridge */
/* Copyright (C) 2004 Apple Computer, Inc. */

#ifndef _PCRE_H
#define _PCRE_H

#define pcre_compile kjs_pcre_compile
#define pcre_copy_substring kjs_pcre_copy_substring
#define pcre_exec kjs_pcre_exec
#define pcre_free kjs_pcre_free
#define pcre_free_substring kjs_pcre_free_substring
#define pcre_free_substring_list kjs_pcre_free_substring_list
#define pcre_fullinfo kjs_pcre_fullinfo
#define pcre_get_substring kjs_pcre_get_substring
#define pcre_get_substring_list kjs_pcre_get_substring_list
#define pcre_info kjs_pcre_info
#define pcre_maketables kjs_pcre_maketables
#define pcre_malloc kjs_pcre_malloc
#define pcre_study kjs_pcre_study
#define pcre_version kjs_pcre_version

#define PCRE_MAJOR 3
#define PCRE_MINOR 9
#define PCRE_DATE 02-Jan-2002
#define PCRE_DL_IMPORT
#define PCRE_UTF16 1

/* Have to include stdlib.h in order to ensure that size_t is defined;
it is needed here for malloc. */

#include <stdlib.h>

/* Allow for C++ users */

#ifdef __cplusplus
extern "C" {
#endif

/* Options */

#define PCRE_CASELESS        0x0001
#define PCRE_MULTILINE       0x0002
#define PCRE_DOTALL          0x0004
#define PCRE_EXTENDED        0x0008
#define PCRE_ANCHORED        0x0010
#define PCRE_DOLLAR_ENDONLY  0x0020
#define PCRE_EXTRA           0x0040
#define PCRE_NOTBOL          0x0080
#define PCRE_NOTEOL          0x0100
#define PCRE_UNGREEDY        0x0200
#define PCRE_NOTEMPTY        0x0400
#define PCRE_UTF8            0x0800

/* Exec-time and get-time error codes */

#define PCRE_ERROR_NOMATCH        (-1)
#define PCRE_ERROR_NULL           (-2)
#define PCRE_ERROR_BADOPTION      (-3)
#define PCRE_ERROR_BADMAGIC       (-4)
#define PCRE_ERROR_UNKNOWN_NODE   (-5)
#define PCRE_ERROR_NOMEMORY       (-6)
#define PCRE_ERROR_NOSUBSTRING    (-7)

/* Request types for pcre_fullinfo() */

#define PCRE_INFO_OPTIONS         0
#define PCRE_INFO_SIZE            1
#define PCRE_INFO_CAPTURECOUNT    2
#define PCRE_INFO_BACKREFMAX      3
#define PCRE_INFO_FIRSTCHAR       4
#define PCRE_INFO_FIRSTTABLE      5
#define PCRE_INFO_LASTLITERAL     6

/* Types */

struct real_pcre;        /* declaration; the definition is private  */
struct real_pcre_extra;  /* declaration; the definition is private */

typedef struct real_pcre pcre;
typedef struct real_pcre_extra pcre_extra;

#if PCRE_UTF16
#include <stdint.h>
typedef uint16_t pcre_char;
#else
typedef char pcre_char;
#endif

/* Store get and free functions. These can be set to alternative malloc/free
functions if required. Some magic is required for Win32 DLL; it is null on
other OS. */

PCRE_DL_IMPORT extern void *(*pcre_malloc)(size_t);
PCRE_DL_IMPORT extern void  (*pcre_free)(void *);

#undef PCRE_DL_IMPORT

/* Functions */

extern pcre *pcre_compile(const pcre_char *, int, const char **, int *,
              const unsigned char *);
extern int  pcre_copy_substring(const pcre_char *, int *, int, int, pcre_char *, int);
extern int  pcre_exec(const pcre *, const pcre_extra *, const pcre_char *,
              int, int, int, int *, int);
extern void pcre_free_substring(const pcre_char *);
extern void pcre_free_substring_list(const pcre_char **);
extern int  pcre_get_substring(const pcre_char *, int *, int, int, const pcre_char **);
extern int  pcre_get_substring_list(const pcre_char *, int *, int, const pcre_char ***);
extern int  pcre_info(const pcre *, int *, int *);
extern int  pcre_fullinfo(const pcre *, const pcre_extra *, int, void *);
extern const unsigned char *pcre_maketables(void);
extern pcre_extra *pcre_study(const pcre *, int, const char **);
extern const char *pcre_version(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* End of pcre.h */
