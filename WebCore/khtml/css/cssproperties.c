/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -a -L ANSI-C -E -C -c -o -t -k '*' -NfindProp -Hhash_prop -Wwordlist_prop -D -s 2 cssproperties.gperf  */
/* This file is automatically generated from cssproperties.in by makeprop, do not edit */
/* Copyright 1999 W. Bastian */
#include "cssproperties.h"
struct props {
    const char *name;
    int id;
};
/* maximum key range = 1493, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash_prop (register const char *str, register unsigned int len)
{
  static const unsigned short asso_values[] =
    {
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513,    0, 1513, 1513, 1513, 1513,
      1513,    5, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513,   20,   40,   45,
         0,    0,  305,   15,    0,    0,    0,   10,    0,   65,
       430,    0,   35,   55,    0,   30,    0,   50,  115,  285,
         0,  210,    0, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513, 1513,
      1513, 1513, 1513, 1513, 1513, 1513
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 32:
        hval += asso_values[(unsigned char)str[31]];
      case 31:
        hval += asso_values[(unsigned char)str[30]];
      case 30:
        hval += asso_values[(unsigned char)str[29]];
      case 29:
        hval += asso_values[(unsigned char)str[28]];
      case 28:
        hval += asso_values[(unsigned char)str[27]];
      case 27:
        hval += asso_values[(unsigned char)str[26]];
      case 26:
        hval += asso_values[(unsigned char)str[25]];
      case 25:
        hval += asso_values[(unsigned char)str[24]];
      case 24:
        hval += asso_values[(unsigned char)str[23]];
      case 23:
        hval += asso_values[(unsigned char)str[22]];
      case 22:
        hval += asso_values[(unsigned char)str[21]];
      case 21:
        hval += asso_values[(unsigned char)str[20]];
      case 20:
        hval += asso_values[(unsigned char)str[19]];
      case 19:
        hval += asso_values[(unsigned char)str[18]];
      case 18:
        hval += asso_values[(unsigned char)str[17]];
      case 17:
        hval += asso_values[(unsigned char)str[16]];
      case 16:
        hval += asso_values[(unsigned char)str[15]];
      case 15:
        hval += asso_values[(unsigned char)str[14]];
      case 14:
        hval += asso_values[(unsigned char)str[13]];
      case 13:
        hval += asso_values[(unsigned char)str[12]];
      case 12:
        hval += asso_values[(unsigned char)str[11]];
      case 11:
        hval += asso_values[(unsigned char)str[10]];
      case 10:
        hval += asso_values[(unsigned char)str[9]];
      case 9:
        hval += asso_values[(unsigned char)str[8]];
      case 8:
        hval += asso_values[(unsigned char)str[7]];
      case 7:
        hval += asso_values[(unsigned char)str[6]];
      case 6:
        hval += asso_values[(unsigned char)str[5]];
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

#ifdef __GNUC__
__inline
#endif
const struct props *
findProp (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 159,
      MIN_WORD_LENGTH = 3,
      MAX_WORD_LENGTH = 32,
      MIN_HASH_VALUE = 20,
      MAX_HASH_VALUE = 1512
    };

  static const struct props wordlist_prop[] =
    {
      {"right", CSS_PROP_RIGHT},
      {"height", CSS_PROP_HEIGHT},
      {"size", CSS_PROP_SIZE},
      {"top", CSS_PROP_TOP},
      {"border", CSS_PROP_BORDER},
      {"color", CSS_PROP_COLOR},
      {"border-right", CSS_PROP_BORDER_RIGHT},
      {"clear", CSS_PROP_CLEAR},
      {"page", CSS_PROP_PAGE},
      {"clip", CSS_PROP_CLIP},
      {"border-top", CSS_PROP_BORDER_TOP},
      {"border-color", CSS_PROP_BORDER_COLOR},
      {"max-height", CSS_PROP_MAX_HEIGHT},
      {"bottom", CSS_PROP_BOTTOM},
      {"border-right-color", CSS_PROP_BORDER_RIGHT_COLOR},
      {"cursor", CSS_PROP_CURSOR},
      {"border-top-color", CSS_PROP_BORDER_TOP_COLOR},
      {"quotes", CSS_PROP_QUOTES},
      {"border-bottom", CSS_PROP_BORDER_BOTTOM},
      {"border-collapse", CSS_PROP_BORDER_COLLAPSE},
      {"-khtml-user-drag", CSS_PROP__KHTML_USER_DRAG},
      {"border-bottom-color", CSS_PROP_BORDER_BOTTOM_COLOR},
      {"scrollbar-3dlight-color", CSS_PROP_SCROLLBAR_3DLIGHT_COLOR},
      {"scrollbar-highlight-color", CSS_PROP_SCROLLBAR_HIGHLIGHT_COLOR},
      {"-khtml-box-pack", CSS_PROP__KHTML_BOX_PACK},
      {"-apple-text-size-adjust", CSS_PROP__APPLE_TEXT_SIZE_ADJUST},
      {"-khtml-user-select", CSS_PROP__KHTML_USER_SELECT},
      {"scrollbar-track-color", CSS_PROP_SCROLLBAR_TRACK_COLOR},
      {"-khtml-marquee", CSS_PROP__KHTML_MARQUEE},
      {"list-style", CSS_PROP_LIST_STYLE},
      {"width", CSS_PROP_WIDTH},
      {"border-style", CSS_PROP_BORDER_STYLE},
      {"display", CSS_PROP_DISPLAY},
      {"left", CSS_PROP_LEFT},
      {"border-right-style", CSS_PROP_BORDER_RIGHT_STYLE},
      {"opacity", CSS_PROP_OPACITY},
      {"float", CSS_PROP_FLOAT},
      {"border-top-style", CSS_PROP_BORDER_TOP_STYLE},
      {"border-width", CSS_PROP_BORDER_WIDTH},
      {"text-shadow", CSS_PROP_TEXT_SHADOW},
      {"-khtml-marquee-speed", CSS_PROP__KHTML_MARQUEE_SPEED},
      {"table-layout", CSS_PROP_TABLE_LAYOUT},
      {"border-left", CSS_PROP_BORDER_LEFT},
      {"border-right-width", CSS_PROP_BORDER_RIGHT_WIDTH},
      {"border-top-width", CSS_PROP_BORDER_TOP_WIDTH},
      {"max-width", CSS_PROP_MAX_WIDTH},
      {"list-style-image", CSS_PROP_LIST_STYLE_IMAGE},
      {"empty-cells", CSS_PROP_EMPTY_CELLS},
      {"border-bottom-style", CSS_PROP_BORDER_BOTTOM_STYLE},
      {"visibility", CSS_PROP_VISIBILITY},
      {"border-left-color", CSS_PROP_BORDER_LEFT_COLOR},
      {"white-space", CSS_PROP_WHITE_SPACE},
      {"-khtml-box-flex", CSS_PROP__KHTML_BOX_FLEX},
      {"z-index", CSS_PROP_Z_INDEX},
      {"border-bottom-width", CSS_PROP_BORDER_BOTTOM_WIDTH},
      {"line-height", CSS_PROP_LINE_HEIGHT},
      {"text-align", CSS_PROP_TEXT_ALIGN},
      {"page-break-after", CSS_PROP_PAGE_BREAK_AFTER},
      {"direction", CSS_PROP_DIRECTION},
      {"outline", CSS_PROP_OUTLINE},
      {"page-break-before", CSS_PROP_PAGE_BREAK_BEFORE},
      {"position", CSS_PROP_POSITION},
      {"scrollbar-arrow-color", CSS_PROP_SCROLLBAR_ARROW_COLOR},
      {"padding", CSS_PROP_PADDING},
      {"text-decoration", CSS_PROP_TEXT_DECORATION},
      {"text-line-through", CSS_PROP_TEXT_LINE_THROUGH},
      {"min-height", CSS_PROP_MIN_HEIGHT},
      {"orphans", CSS_PROP_ORPHANS},
      {"-khtml-marquee-style", CSS_PROP__KHTML_MARQUEE_STYLE},
      {"padding-right", CSS_PROP_PADDING_RIGHT},
      {"list-style-type", CSS_PROP_LIST_STYLE_TYPE},
      {"margin", CSS_PROP_MARGIN},
      {"scrollbar-shadow-color", CSS_PROP_SCROLLBAR_SHADOW_COLOR},
      {"outline-color", CSS_PROP_OUTLINE_COLOR},
      {"-khtml-box-flex-group", CSS_PROP__KHTML_BOX_FLEX_GROUP},
      {"padding-top", CSS_PROP_PADDING_TOP},
      {"margin-right", CSS_PROP_MARGIN_RIGHT},
      {"text-overline", CSS_PROP_TEXT_OVERLINE},
      {"-khtml-box-orient", CSS_PROP__KHTML_BOX_ORIENT},
      {"text-line-through-color", CSS_PROP_TEXT_LINE_THROUGH_COLOR},
      {"counter-reset", CSS_PROP_COUNTER_RESET},
      {"scrollbar-face-color", CSS_PROP_SCROLLBAR_FACE_COLOR},
      {"scrollbar-darkshadow-color", CSS_PROP_SCROLLBAR_DARKSHADOW_COLOR},
      {"caption-side", CSS_PROP_CAPTION_SIDE},
      {"margin-top", CSS_PROP_MARGIN_TOP},
      {"unicode-bidi", CSS_PROP_UNICODE_BIDI},
      {"text-line-through-mode", CSS_PROP_TEXT_LINE_THROUGH_MODE},
      {"letter-spacing", CSS_PROP_LETTER_SPACING},
      {"-khtml-box-lines", CSS_PROP__KHTML_BOX_LINES},
      {"-khtml-line-break", CSS_PROP__KHTML_LINE_BREAK},
      {"-khtml-box-align", CSS_PROP__KHTML_BOX_ALIGN},
      {"border-left-style", CSS_PROP_BORDER_LEFT_STYLE},
      {"widows", CSS_PROP_WIDOWS},
      {"text-overline-color", CSS_PROP_TEXT_OVERLINE_COLOR},
      {"-khtml-box-direction", CSS_PROP__KHTML_BOX_DIRECTION},
      {"page-break-inside", CSS_PROP_PAGE_BREAK_INSIDE},
      {"padding-bottom", CSS_PROP_PADDING_BOTTOM},
      {"background", CSS_PROP_BACKGROUND},
      {"text-overline-mode", CSS_PROP_TEXT_OVERLINE_MODE},
      {"border-spacing", CSS_PROP_BORDER_SPACING},
      {"word-wrap", CSS_PROP_WORD_WRAP},
      {"-khtml-padding-start", CSS_PROP__KHTML_PADDING_START},
      {"border-left-width", CSS_PROP_BORDER_LEFT_WIDTH},
      {"margin-bottom", CSS_PROP_MARGIN_BOTTOM},
      {"vertical-align", CSS_PROP_VERTICAL_ALIGN},
      {"-apple-dashboard-region", CSS_PROP__APPLE_DASHBOARD_REGION},
      {"background-color", CSS_PROP_BACKGROUND_COLOR},
      {"-khtml-margin-start", CSS_PROP__KHTML_MARGIN_START},
      {"background-repeat", CSS_PROP_BACKGROUND_REPEAT},
      {"-khtml-box-ordinal-group", CSS_PROP__KHTML_BOX_ORDINAL_GROUP},
      {"-khtml-nbsp-mode", CSS_PROP__KHTML_NBSP_MODE},
      {"-apple-line-clamp", CSS_PROP__APPLE_LINE_CLAMP},
      {"overflow", CSS_PROP_OVERFLOW},
      {"text-overflow", CSS_PROP_TEXT_OVERFLOW},
      {"background-image", CSS_PROP_BACKGROUND_IMAGE},
      {"outline-style", CSS_PROP_OUTLINE_STYLE},
      {"font", CSS_PROP_FONT},
      {"-khtml-flow-mode", CSS_PROP__KHTML_FLOW_MODE},
      {"-khtml-user-modify", CSS_PROP__KHTML_USER_MODIFY},
      {"-khtml-marquee-repetition", CSS_PROP__KHTML_MARQUEE_REPETITION},
      {"-khtml-margin-collapse", CSS_PROP__KHTML_MARGIN_COLLAPSE},
      {"text-line-through-style", CSS_PROP_TEXT_LINE_THROUGH_STYLE},
      {"-khtml-marquee-direction", CSS_PROP__KHTML_MARQUEE_DIRECTION},
      {"font-size", CSS_PROP_FONT_SIZE},
      {"outline-width", CSS_PROP_OUTLINE_WIDTH},
      {"list-style-position", CSS_PROP_LIST_STYLE_POSITION},
      {"min-width", CSS_PROP_MIN_WIDTH},
      {"-khtml-margin-top-collapse", CSS_PROP__KHTML_MARGIN_TOP_COLLAPSE},
      {"text-line-through-width", CSS_PROP_TEXT_LINE_THROUGH_WIDTH},
      {"text-overline-style", CSS_PROP_TEXT_OVERLINE_STYLE},
      {"padding-left", CSS_PROP_PADDING_LEFT},
      {"font-stretch", CSS_PROP_FONT_STRETCH},
      {"margin-left", CSS_PROP_MARGIN_LEFT},
      {"text-overline-width", CSS_PROP_TEXT_OVERLINE_WIDTH},
      {"text-transform", CSS_PROP_TEXT_TRANSFORM},
      {"-khtml-margin-bottom-collapse", CSS_PROP__KHTML_MARGIN_BOTTOM_COLLAPSE},
      {"text-indent", CSS_PROP_TEXT_INDENT},
      {"word-spacing", CSS_PROP_WORD_SPACING},
      {"font-size-adjust", CSS_PROP_FONT_SIZE_ADJUST},
      {"-khtml-border-vertical-spacing", CSS_PROP__KHTML_BORDER_VERTICAL_SPACING},
      {"content", CSS_PROP_CONTENT},
      {"text-underline", CSS_PROP_TEXT_UNDERLINE},
      {"text-underline-color", CSS_PROP_TEXT_UNDERLINE_COLOR},
      {"font-style", CSS_PROP_FONT_STYLE},
      {"text-underline-mode", CSS_PROP_TEXT_UNDERLINE_MODE},
      {"-khtml-binding", CSS_PROP__KHTML_BINDING},
      {"font-weight", CSS_PROP_FONT_WEIGHT},
      {"background-position", CSS_PROP_BACKGROUND_POSITION},
      {"background-position-x", CSS_PROP_BACKGROUND_POSITION_X},
      {"outline-offset", CSS_PROP_OUTLINE_OFFSET},
      {"text-underline-style", CSS_PROP_TEXT_UNDERLINE_STYLE},
      {"-khtml-border-horizontal-spacing", CSS_PROP__KHTML_BORDER_HORIZONTAL_SPACING},
      {"background-attachment", CSS_PROP_BACKGROUND_ATTACHMENT},
      {"text-underline-width", CSS_PROP_TEXT_UNDERLINE_WIDTH},
      {"-khtml-marquee-increment", CSS_PROP__KHTML_MARQUEE_INCREMENT},
      {"font-variant", CSS_PROP_FONT_VARIANT},
      {"background-position-y", CSS_PROP_BACKGROUND_POSITION_Y},
      {"font-family", CSS_PROP_FONT_FAMILY},
      {"counter-increment", CSS_PROP_COUNTER_INCREMENT}
    };

  static const short lookup[] =
    {
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        0,   1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,   2,  -1,  -1,  -1,   3,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,   4,  -1,  -1,  -1,
        5,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,   6,  -1,  -1,
        7,  -1,  -1,  -1,   8,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,   9,  10,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  11,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       12,  13,  -1,  -1,  -1,  -1,  -1,  -1,  14,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  15,  -1,  -1,  -1,  -1,  16,  -1,  -1,  -1,
       -1,  17,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  18,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  19,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  20,  -1,  -1,  21,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  22,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  23,  -1,  -1,  -1,  -1,
       24,  -1,  -1,  25,  -1,  -1,  -1,  -1,  26,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  27,  -1,  -1,  28,
       29,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       30,  -1,  31,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  32,  -1,  -1,  -1,  -1,  -1,  -1,  33,
       -1,  -1,  -1,  34,  -1,  -1,  -1,  35,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       36,  37,  -1,  -1,  -1,  -1,  -1,  38,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  39,  -1,  -1,  -1,
       40,  -1,  41,  -1,  -1,  -1,  42,  -1,  43,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  44,  -1,  -1,  45,
       -1,  -1,  -1,  -1,  -1,  -1,  46,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  47,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  48,  49,  -1,  50,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  51,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  52,  -1,  53,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  54,
       -1,  -1,  -1,  -1,  -1,  -1,  55,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  56,  -1,  -1,  -1,  -1,
       -1,  57,  -1,  -1,  58,  -1,  -1,  59,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  60,  61,  -1,  -1,  62,  63,  -1,  -1,
       64,  -1,  65,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       66,  -1,  67,  -1,  -1,  68,  -1,  -1,  69,  -1,
       70,  -1,  -1,  -1,  -1,  -1,  71,  72,  73,  -1,
       -1,  74,  -1,  -1,  -1,  -1,  75,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  76,  77,  -1,
       -1,  -1,  78,  79,  -1,  -1,  -1,  -1,  80,  -1,
       81,  82,  83,  -1,  -1,  84,  -1,  85,  -1,  -1,
       -1,  -1,  86,  -1,  -1,  -1,  -1,  -1,  -1,  87,
       -1,  88,  89,  -1,  -1,  -1,  90,  -1,  -1,  -1,
       -1,  -1,  91,  -1,  -1,  -1,  92,  -1,  -1,  93,
       94,  -1,  -1,  -1,  -1,  -1,  -1,  95,  -1,  96,
       97,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  98,  99,
       -1,  -1,  -1,  -1, 100,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1, 101,  -1, 102, 103,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 104,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 105,  -1,
       -1, 106,  -1,  -1, 107,  -1,  -1,  -1,  -1,  -1,
       -1,  -1, 108,  -1,  -1,  -1,  -1,  -1,  -1, 109,
       -1, 110,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1, 111,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1, 112,  -1,  -1,  -1,  -1, 113,  -1,
       -1,  -1,  -1,  -1,  -1,  -1, 114,  -1,  -1,  -1,
       -1,  -1,  -1, 115,  -1,  -1,  -1,  -1,  -1, 116,
       -1,  -1,  -1,  -1,  -1,  -1, 117,  -1,  -1,  -1,
       -1,  -1,  -1, 118,  -1, 119,  -1, 120, 121,  -1,
       -1,  -1,  -1,  -1, 122,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1, 123,  -1,  -1,  -1, 124,  -1,
       -1,  -1,  -1,  -1, 125,  -1,  -1,  -1,  -1, 126,
       -1,  -1,  -1,  -1,  -1,  -1, 127,  -1,  -1,  -1,
       -1,  -1,  -1, 128, 129,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1, 130,  -1,  -1,
       -1,  -1, 131,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1, 132,  -1,  -1, 133,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1, 134,  -1,  -1,  -1,  -1, 135,
       -1, 136, 137,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1, 138,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      139,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1, 140,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1, 141,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1, 142,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1, 143,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1, 144,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1, 145,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1, 146,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1, 147,  -1, 148,  -1,  -1,  -1,
       -1,  -1,  -1,  -1, 149,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      150,  -1, 151,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1, 152,  -1,  -1,  -1, 153,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 154,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1, 155,  -1,  -1,  -1, 156,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1, 157,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1, 158
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash_prop (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist_prop[index].name;

              if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
                return &wordlist_prop[index];
            }
        }
    }
  return 0;
}
static const char * const propertyList[] = {
"",
"background-color", 
"background-image", 
"background-repeat", 
"background-attachment", 
"background-position", 
"background-position-x", 
"background-position-y", 
"-khtml-binding", 
"border-collapse", 
"border-spacing", 
"-khtml-border-horizontal-spacing", 
"-khtml-border-vertical-spacing", 
"border-top-color", 
"border-right-color", 
"border-bottom-color", 
"border-left-color", 
"border-top-style", 
"border-right-style", 
"border-bottom-style", 
"border-left-style", 
"border-top-width", 
"border-right-width", 
"border-bottom-width", 
"border-left-width", 
"bottom", 
"-khtml-box-align", 
"-khtml-box-direction", 
"-khtml-box-flex", 
"-khtml-box-flex-group", 
"-khtml-box-lines", 
"-khtml-box-ordinal-group", 
"-khtml-box-orient", 
"-khtml-box-pack", 
"caption-side", 
"clear", 
"clip", 
"color", 
"content", 
"counter-increment", 
"counter-reset", 
"cursor", 
"direction", 
"display", 
"empty-cells", 
"float", 
"font-family", 
"font-size", 
"font-size-adjust", 
"font-stretch", 
"font-style", 
"font-variant", 
"font-weight", 
"height", 
"left", 
"letter-spacing", 
"-apple-line-clamp", 
"line-height", 
"list-style-image", 
"list-style-position", 
"list-style-type", 
"margin-top", 
"margin-right", 
"margin-bottom", 
"margin-left", 
"-khtml-line-break", 
"-khtml-margin-collapse", 
"-khtml-margin-top-collapse", 
"-khtml-margin-bottom-collapse", 
"-khtml-margin-start", 
"-khtml-marquee", 
"-khtml-marquee-direction", 
"-khtml-marquee-increment", 
"-khtml-marquee-repetition", 
"-khtml-marquee-speed", 
"-khtml-marquee-style", 
"max-height", 
"max-width", 
"min-height", 
"min-width", 
"-khtml-nbsp-mode", 
"opacity", 
"orphans", 
"outline-color", 
"outline-offset", 
"outline-style", 
"outline-width", 
"overflow", 
"padding-top", 
"padding-right", 
"padding-bottom", 
"padding-left", 
"-khtml-padding-start", 
"page", 
"page-break-after", 
"page-break-before", 
"page-break-inside", 
"position", 
"quotes", 
"right", 
"size", 
"table-layout", 
"text-align", 
"text-decoration", 
"text-indent", 
"text-line-through", 
"text-line-through-color", 
"text-line-through-mode", 
"text-line-through-style", 
"text-line-through-width", 
"text-overflow", 
"text-overline", 
"text-overline-color", 
"text-overline-mode", 
"text-overline-style", 
"text-overline-width", 
"text-shadow", 
"text-transform", 
"text-underline", 
"text-underline-color", 
"text-underline-mode", 
"text-underline-style", 
"text-underline-width", 
"-apple-text-size-adjust", 
"-apple-dashboard-region", 
"top", 
"unicode-bidi", 
"-khtml-user-drag", 
"-khtml-user-modify", 
"-khtml-user-select", 
"vertical-align", 
"visibility", 
"white-space", 
"widows", 
"width", 
"word-wrap", 
"word-spacing", 
"z-index", 
"background", 
"border", 
"border-color", 
"border-style", 
"border-top", 
"border-right", 
"border-bottom", 
"border-left", 
"border-width", 
"font", 
"list-style", 
"margin", 
"outline", 
"padding", 
"scrollbar-face-color", 
"scrollbar-shadow-color", 
"scrollbar-highlight-color", 
"scrollbar-3dlight-color", 
"scrollbar-darkshadow-color", 
"scrollbar-track-color", 
"scrollbar-arrow-color", 
"-khtml-flow-mode", 
    0
};
DOMString getPropertyName(unsigned short id)
{
    if(id >= CSS_PROP_TOTAL || id == 0)
      return DOMString();
    else
      return DOMString(propertyList[id]);
};

