// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2006 George Staikos <staikos@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "UnicodeQt4.h"

using namespace WTF::Unicode;

Direction WTF::Unicode::direction(int c)
{
  // FIXME: implement support for non-BMP code points
  if ((c & 0xffff0000) != 0)
    return LeftToRight;

  switch (QChar((unsigned short)c).direction()) {
    case QChar::DirL:
      return LeftToRight;
    case QChar::DirR:
      return RightToLeft;
    case QChar::DirNSM:
      return NonSpacingMark;
    case QChar::DirBN:
      return BoundaryNeutral;
    case QChar::DirEN:
      return EuropeanNumber;
    case QChar::DirES:
      return EuropeanNumberSeparator;
    case QChar::DirET:
      return EuropeanNumberTerminator;
    case QChar::DirAN:
      return ArabicNumber;
    case QChar::DirCS:
      return CommonNumberSeparator;
    case QChar::DirB:
      return BlockSeparator;
    case QChar::DirS:
      return SegmentSeparator;
    case QChar::DirWS:
      return WhiteSpaceNeutral;
    case QChar::DirON:
      return OtherNeutral;
    case QChar::DirLRE:
      return LeftToRightEmbedding;
    case QChar::DirLRO:
      return LeftToRightOverride;
    case QChar::DirAL:
      return RightToLeftArabic;
    case QChar::DirRLE:
      return RightToLeftEmbedding;
    case QChar::DirRLO:
      return RightToLeftOverride;
    case QChar::DirPDF:
      return PopDirectionalFormat;
    default:
      return LeftToRight;
  }
}

CharCategory WTF::Unicode::category(int c)
{
  // FIXME: implement support for non-BMP code points
  if ((c & 0xffff0000) != 0)
    return NoCategory;

  switch (QChar((unsigned short)c).category()) {
    case QChar::Mark_NonSpacing:
      return Mark_NonSpacing;
    case QChar::Mark_SpacingCombining:
      return Mark_SpacingCombining;
    case QChar::Mark_Enclosing:
      return Mark_Enclosing;
    case QChar::Number_DecimalDigit:
      return Number_DecimalDigit;
    case QChar::Number_Letter:
      return Number_Letter;
    case QChar::Number_Other:
      return Number_Other;
    case QChar::Separator_Space:
      return Separator_Space;
    case QChar::Separator_Line:
      return Separator_Line;
    case QChar::Separator_Paragraph:
      return Separator_Paragraph;
    case QChar::Other_Control:
      return Other_Control;
    case QChar::Other_Format:
      return Other_Format;
    case QChar::Other_Surrogate:
      return Other_Surrogate;
    case QChar::Other_PrivateUse:
      return Other_PrivateUse;
    case QChar::Other_NotAssigned:
      return Other_NotAssigned;
    case QChar::Letter_Uppercase:
      return Letter_Uppercase;
    case QChar::Letter_Lowercase:
      return Letter_Lowercase;
    case QChar::Letter_Titlecase:
      return Letter_Titlecase;
    case QChar::Letter_Modifier:
      return Letter_Modifier;
    case QChar::Letter_Other:
      return Letter_Other;
    case QChar::Punctuation_Connector:
      return Punctuation_Connector;
    case QChar::Punctuation_Dash:
      return Punctuation_Dash;
    case QChar::Punctuation_Open:
      return Punctuation_Open;
    case QChar::Punctuation_Close:
      return Punctuation_Close;
    case QChar::Punctuation_InitialQuote:
      return Punctuation_InitialQuote;
    case QChar::Punctuation_FinalQuote:
      return Punctuation_FinalQuote;
    case QChar::Punctuation_Other:
      return Punctuation_Other;
    case QChar::Symbol_Math:
      return Symbol_Math;
    case QChar::Symbol_Currency:
      return Symbol_Currency;
    case QChar::Symbol_Modifier:
      return Symbol_Modifier;
    case QChar::Symbol_Other:
      return Symbol_Other;
    default:
      return NoCategory;
  }
}


Decomposition WTF::Unicode::decomposition(int c)
{
  // FIXME: implement support for non-BMP code points
  if ((c & 0xffff0000) != 0)
    return None;

  switch (QChar((unsigned short)c).decompositionTag()) {
    default:
    case QChar::NoDecomposition:
      return None;
    case QChar::Canonical:
      return Canonical;
    case QChar::Font:
      return Font;
    case QChar::NoBreak:
      return NoBreak;
    case QChar::Initial:
      return Initial;
    case QChar::Medial:
      return Medial;
    case QChar::Final:
      return Final;
    case QChar::Isolated:
      return Isolated;
    case QChar::Circle:
      return Circle;
    case QChar::Super:
      return Super;
    case QChar::Sub:
      return Sub;
    case QChar::Vertical:
      return Vertical;
    case QChar::Wide:
      return Wide;
    case QChar::Narrow:
      return Narrow;
    case QChar::Small:
      return Small;
    case QChar::Square:
      return Square;
    case QChar::Compat:
      return Compat;
    case QChar::Fraction:
      return Fraction;
  }
}

// vim: ts=2 sw=2 et
