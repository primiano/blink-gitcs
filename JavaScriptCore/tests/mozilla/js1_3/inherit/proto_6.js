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
    File Name:          proto_6.js
    Section:
    Description:        Logical OR || in constructors

    This tests Object Hierarchy and Inheritance, as described in the document
    Object Hierarchy and Inheritance in JavaScript, last modified on 12/18/97
    15:19:34 on http://devedge.netscape.com/.  Current URL:
    http://devedge.netscape.com/docs/manuals/communicator/jsobj/contents.htm

    This tests the syntax ObjectName.prototype = new PrototypeObject using the
    Employee example in the document referenced above.

    This tests the logical OR opererator || syntax in constructors.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "proto_6";
    var VERSION = "JS1_3";
    var TITLE   = "Logical OR || in constructors";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

function Employee ( name, dept ) {
     this.name = name || "";
     this.dept = dept || "general";
}
function Manager () {
     this.reports = [];
}
Manager.prototype = new Employee();

function WorkerBee ( name, dept, projs ) {
    this.base = Employee;
    this.base( name, dept)
    this.projects = projs || new Array();
}

WorkerBee.prototype = new Employee();

function SalesPerson () {
    this.dept = "sales";
    this.quota = 100;
}
SalesPerson.prototype = new WorkerBee();

function Engineer ( name, projs, machine ) {
    this.base = WorkerBee;
    this.base( name, "engineering", projs )
    this.machine = machine || "";
}
Engineer.prototype = new WorkerBee();

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

    var pat = new Engineer( "Toonces, Pat",
                            ["SpiderMonkey", "Rhino"],
                            "indy" );

    var les = new WorkerBee( "Morris, Les",
                             "Training",
                             ["Hippo"] )

    var terry = new Employee( "Boomberi, Terry",
                              "Marketing" );

    // Pat, the Engineer

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.name",
                                    "Toonces, Pat",
                                    pat.name );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.dept",
                                    "engineering",
                                    pat.dept );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.projects.length",
                                    2,
                                    pat.projects.length );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.projects[0]",
                                    "SpiderMonkey",
                                    pat.projects[0] );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.projects[1]",
                                    "Rhino",
                                    pat.projects[1] );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.machine",
                                    "indy",
                                    pat.machine );


    // Les, the WorkerBee

    testcases[tc++] = new TestCase( SECTION,
                                    "les.name",
                                    "Morris, Les",
                                    les.name );

    testcases[tc++] = new TestCase( SECTION,
                                    "les.dept",
                                    "Training",
                                    les.dept );

    testcases[tc++] = new TestCase( SECTION,
                                    "les.projects.length",
                                    1,
                                    les.projects.length );

    testcases[tc++] = new TestCase( SECTION,
                                    "les.projects[0]",
                                    "Hippo",
                                    les.projects[0] );

    // Terry, the Employee
    testcases[tc++] = new TestCase( SECTION,
                                    "terry.name",
                                    "Boomberi, Terry",
                                    terry.name );

    testcases[tc++] = new TestCase( SECTION,
                                    "terry.dept",
                                    "Marketing",
                                    terry.dept );
    test();

