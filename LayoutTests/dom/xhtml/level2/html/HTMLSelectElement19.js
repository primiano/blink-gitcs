
/*
Copyright Â© 2001-2004 World Wide Web Consortium, 
(Massachusetts Institute of Technology, European Research Consortium 
for Informatics and Mathematics, Keio University). All 
Rights Reserved. This work is distributed under the W3CÂ® Software License [1] in the 
hope that it will be useful, but WITHOUT ANY WARRANTY; without even 
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 

[1] http://www.w3.org/Consortium/Legal/2002/copyright-software-20021231
*/



   /**
    *  Gets URI that identifies the test.
    *  @return uri identifier of test
    */
function getTargetURI() {
      return "http://www.w3.org/2001/DOM-Test-Suite/level2/html/HTMLSelectElement19";
   }

var docsLoaded = -1000000;
var builder = null;

//
//   This function is called by the testing framework before
//      running the test suite.
//
//   If there are no configuration exceptions, asynchronous
//        document loading is started.  Otherwise, the status
//        is set to complete and the exception is immediately
//        raised when entering the body of the test.
//
function setUpPage() {
   setUpPageStatus = 'running';
   try {
     //
     //   creates test document builder, may throw exception
     //
     builder = createConfiguredBuilder();

      docsLoaded = 0;
      
      var docRef = null;
      if (typeof(this.doc) != 'undefined') {
        docRef = this.doc;
      }
      docsLoaded += preload(docRef, "doc", "select");
        
       if (docsLoaded == 1) {
          setUpPage = 'complete';
       }
    } catch(ex) {
    	catchInitializationError(builder, ex);
        setUpPage = 'complete';
    }
}



//
//   This method is called on the completion of 
//      each asychronous load started in setUpTests.
//
//   When every synchronous loaded document has completed,
//      the page status is changed which allows the
//      body of the test to be executed.
function loadComplete() {
    if (++docsLoaded == 1) {
        setUpPageStatus = 'complete';
    }
}


/**
* 
Add a new option before the selected node using HTMLSelectElement.add.

* @author Curt Arnold
* @see http://www.w3.org/TR/1998/REC-DOM-Level-1-19981001/level-one-html#ID-14493106
*/
function HTMLSelectElement19() {
   var success;
    if(checkInitialization(builder, "HTMLSelectElement19") != null) return;
    var nodeList;
      var testNode;
      var doc;
      var optLength;
      var selected;
      var newOpt;
      var newOptText;
      var opt;
      var optText;
      var optValue;
      var retNode;
      var options;
      var selectedNode;
      
      var docRef = null;
      if (typeof(this.doc) != 'undefined') {
        docRef = this.doc;
      }
      doc = load(docRef, "doc", "select");
      nodeList = doc.getElementsByTagName("select");
      assertSize("Asize",3,nodeList);
testNode = nodeList.item(0);
      newOpt = doc.createElement("option");
      newOptText = doc.createTextNode("EMP31415");
      retNode = newOpt.appendChild(newOptText);
      options = testNode.options;

      selectedNode = options.item(0);
      testNode.add(newOpt,selectedNode);
      optLength = testNode.length;

      assertEquals("optLength",6,optLength);
       selected = testNode.selectedIndex;

      assertEquals("selected",1,selected);
       options = testNode.options;

      opt = options.item(0);
      optText = opt.firstChild;

      optValue = optText.nodeValue;

      assertEquals("lastValue","EMP31415",optValue);
       
}




function runTest() {
   HTMLSelectElement19();
}
