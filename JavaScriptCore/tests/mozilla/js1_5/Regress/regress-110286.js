/* ***** BEGIN LICENSE BLOCK *****
* Version: NPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Netscape Public License
* Version 1.1 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is JavaScript Engine testing utilities.
*
* The Initial Developer of the Original Code is Netscape Communications Corp.
* Portions created by the Initial Developer are Copyright (C) 2001
* the Initial Developer. All Rights Reserved.
*
* Contributor(s): pschwartau@netscape.com
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the NPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the NPL, the GPL or the LGPL.
*
* ***** END LICENSE BLOCK *****
*
*
* Date: 16 Nov 2001
* SUMMARY: multiline comments containing "/*" should not be syntax errors
* See http://bugzilla.mozilla.org/show_bug.cgi?id=110286
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 110286;
var summary = 'Multiline comments containing "/*" should not be syntax errors';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
actual = eval("/* /* */3");
expect = 3;
addThis();

status = inSection(2);
actual = eval("3/* /* */");
expect = 3;
addThis();

status = inSection(3);
actual = eval("/* 3/* */");
expect = undefined;
addThis();

status = inSection(4);
actual = eval("/* /*3 */");
expect = undefined;
addThis();

status = inSection(5);
var passed = true;
try
{
  eval("/* blah blah /* blah blah */");
}
catch(e)
{
  passed = false;
}
actual = passed;
expect = true;
addThis();


status = inSection(6);
try
{
  /*
     /*A/* /* /*A/*
     /* blah blah /*
     /* blah blah /*
     /* /*A/* /*A/*
  */
  var result = 'PASSED';
}
catch(e)
{
  var result = 'FAILED';
}
actual = result;
expect = 'PASSED';
addThis();


status = inSection(7);
var str = 'ABC';
/*
 *    /*
 *    /* 
 *    /*
 *    /*
 *
 */
str += 'DEF';
actual = str;
expect = 'ABCDEF';
addThis();



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
 
  for (var i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
