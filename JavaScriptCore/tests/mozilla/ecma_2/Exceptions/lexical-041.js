/**
    File Name:          lexical-041.js
    Corresponds To:     7.5-8-n.js
    ECMA Section:       7.5 Identifiers
    Description:        Identifiers are of unlimited length
                        - can contain letters, a decimal digit, _, or $
                        - the first character cannot be a decimal digit
                        - identifiers are case sensitive

    Author:             christine@netscape.com
    Date:               11 september 1997
*/
    var SECTION = "lexical-041";
    var VERSION = "ECMA_1";
    var TITLE   = "Identifiers";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);
    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        eval("var @abc;");
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "var @abc" +
        " (threw " + exception +")",
        expect,
        result );

    test();


