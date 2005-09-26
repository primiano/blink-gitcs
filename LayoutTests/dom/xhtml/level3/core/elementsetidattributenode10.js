
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
      return "http://www.w3.org/2001/DOM-Test-Suite/level3/core/elementsetidattributenode10";
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
       setImplementationAttribute("namespaceAware", true);

      docsLoaded = 0;
      
      var docRef = null;
      if (typeof(this.doc) != 'undefined') {
        docRef = this.doc;
      }
      docsLoaded += preload(docRef, "doc", "hc_staff");
        
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
     This method declares the attribute specified by node to be of type ID. If the value of the specified attribute 
     is unique then this element node can later be retrieved using getElementById on Document. Note, however, 
     that this simply affects this node and does not change any grammar that may be in use. 
     
     Invoke setIdAttributeNode on the 4th acronym element using the class attribute (containing entity reference)
     as a parameter .  Verify by calling isId on the attribute node and getElementById on document node.  
     Reset by invoking setIdAttributeNode with isId=false.  
    
* @author IBM
* @author Jenny Hsu
* @see http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/core#ID-ElSetIdAttrNode
*/
function elementsetidattributenode10() {
   var success;
    if(checkInitialization(builder, "elementsetidattributenode10") != null) return;
    var doc;
      var elemList;
      var acronymElem;
      var attributesMap;
      var attr;
      var id = false;
      var elem;
      var elemName;
      
      var docRef = null;
      if (typeof(this.doc) != 'undefined') {
        docRef = this.doc;
      }
      doc = load(docRef, "doc", "hc_staff");
      elemList = doc.getElementsByTagNameNS("*","acronym");
      acronymElem = elemList.item(3);
      attributesMap = acronymElem.attributes;

      attr = attributesMap.getNamedItem("class");
      acronymElem.setIdAttributeNode(attr,true);
      id = attr.isId;

      assertTrue("elementsetidattributenodeIsIdTrue10",id);
elem = doc.getElementById("Yα");
      elemName = elem.tagName;

      assertEquals("elementsetidattributenodeGetElementById10","acronym",elemName);
       acronymElem.setIdAttributeNode(attr,false);
      id = attr.isId;

      assertFalse("elementsetidattributenodeIsIdFalse10",id);

}




function runTest() {
   elementsetidattributenode10();
}
