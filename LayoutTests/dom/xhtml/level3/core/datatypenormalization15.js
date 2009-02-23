
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
      return "http://www.w3.org/2001/DOM-Test-Suite/level3/core/datatypenormalization15";
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
      docsLoaded += preload(docRef, "doc", "datatype_normalization2");
        
       if (docsLoaded == 1) {
          setUpPageStatus = 'complete';
       }
    } catch(ex) {
    	catchInitializationError(builder, ex);
        setUpPageStatus = 'complete';
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

//DOMErrorMonitor's require a document level variable named errorMonitor
var errorMonitor;
	 
/**
* 
Normalize document with datatype-normalization set to true.  
Check if string values were normalized per an explicit whitespace=collapse.

* @author Curt Arnold
* @see http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/core#Document3-normalizeDocument
* @see http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/core#parameter-datatype-normalization
*/
function datatypenormalization15() {
   var success;
    if(checkInitialization(builder, "datatypenormalization15") != null) return;
    var doc;
      var elemList;
      var element;
      var domConfig;
      var str;
      var canSetNormalization;
      var canSetValidate;
      var canSetXMLSchema;
      var xsdNS = "http://www.w3.org/2001/XMLSchema";
      errorMonitor = new DOMErrorMonitor();
      
      var childNode;
      var childValue;
      
      var docRef = null;
      if (typeof(this.doc) != 'undefined') {
        docRef = this.doc;
      }
      doc = load(docRef, "doc", "datatype_normalization2");
      domConfig = doc.domConfig;

      canSetNormalization = domConfig.canSetParameter("datatype-normalization",true);
      canSetValidate = domConfig.canSetParameter("validate",true);
      canSetXMLSchema = domConfig.canSetParameter("schema-type",xsdNS);
      
	if(
	
	(canSetNormalization && canSetValidate && canSetXMLSchema)

	) {
	domConfig.setParameter("datatype-normalization", true);
	 domConfig.setParameter("validate", true);
	 domConfig.setParameter("schema-type", xsdNS);
	 domConfig.setParameter("error-handler", errorMonitor.handleError);
	 doc.normalizeDocument();
      errorMonitor.assertLowerSeverity("normalizeError", 2);
     elemList = doc.getElementsByTagNameNS("http://www.w3.org/1999/xhtml","code");
      element = elemList.item(0);
      childNode = element.firstChild;

      childValue = childNode.nodeValue;

      assertEquals("content1","EMP 0001",childValue);
       element = elemList.item(1);
      childNode = element.firstChild;

      childValue = childNode.nodeValue;

      assertEquals("content2","EMP 0001",childValue);
       element = elemList.item(2);
      childNode = element.firstChild;

      childValue = childNode.nodeValue;

      assertEquals("content3","EMP 0001",childValue);
       
	}
	
}




function runTest() {
   datatypenormalization15();
}
