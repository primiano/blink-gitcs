/**
 *  File Name:          try-004.js
 *  ECMA Section:
 *  Description:        The try statement
 *
 *  This test has a try with one catch block but no finally.
 *
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 */
    var SECTION = "try-004";
    var VERSION = "ECMA_2";
    var TITLE   = "The try statement";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    TryToCatch( "Math.PI", Math.PI );
    TryToCatch( "Thrower(5)",   "Caught 5" );
    TryToCatch( "Thrower(\"some random exception\")", "Caught some random exception" );

    test();

    function Thrower( v ) {
        throw "Caught " + v;
    }

    /**
     *  Evaluate a string.  Catch any exceptions thrown.  If no exception is
     *  expected, verify the result of the evaluation.  If an exception is
     *  expected, verify that we got the right exception.
     */

    function TryToCatch( value, expect ) {
        try {
            result = eval( value );
        } catch ( e ) {
            result = e;
        }

        testcases[tc++] = new TestCase(
            SECTION,
            "eval( " + value +" )",
            expect,
            result );
    }


