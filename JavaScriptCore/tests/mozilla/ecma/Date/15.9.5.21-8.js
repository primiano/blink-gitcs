/* The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * 
 */
/**
    File Name:          15.9.5.21.js
    ECMA Section:       15.9.5.21
    Description:        Date.prototype.getUTCMilliseconds

    1.  Let t be this time value.
    2.  If t is NaN, return NaN.
    3.  Return msFromTime(t).

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "15.9.5.21";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Date.prototype.getUTCMilliseconds()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var TZ_ADJUST = TZ_DIFF * msPerHour;

    // get the current time
    var now = (new Date()).valueOf();

    // calculate time for year 0
    for ( var time = 0, year = 1969; year >= 0; year-- ) {
        time -= TimeInYear(year);
    }
    // get time for 29 feb 2000

    var UTC_FEB_29_2000 = TIME_2000 + 31*msPerDay + 28*msPerHour;

    // get time for 1 jan 2005

    var UTC_JAN_1_2005 = TIME_2000 + TimeInYear(2000)+TimeInYear(2001)+
    TimeInYear(2002)+TimeInYear(2003)+TimeInYear(2004);

    testcases[tc++] = new TestCase( SECTION,
                                    "(new Date(NaN)).getUTCMilliseconds()",
                                    NaN,
                                    (new Date(NaN)).getUTCMilliseconds() );

    testcases[tc++] = new TestCase( SECTION,
                                    "Date.prototype.getUTCMilliseconds.length",
                                    0,
                                    Date.prototype.getUTCMilliseconds.length );
    test();
function addTestCase( t ) {
            testcases[tc++] = new TestCase( SECTION,
                                    "(new Date("+t+")).getUTCMilliseconds()",
                                    msFromTime(t),
                                    (new Date(t)).getUTCMilliseconds() );
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
