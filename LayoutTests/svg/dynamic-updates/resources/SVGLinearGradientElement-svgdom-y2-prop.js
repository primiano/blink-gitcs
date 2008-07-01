// [Name] SVGLinearGradientElement-svgdom-y2-prop.js
// [Expected rendering result] green ellipse, no red visible - and a series of PASS mesages

description("Tests dynamic updates of the 'y2' property of the SVGLinearGradientElement object")
createSVGTestCase();

var ellipseElement = createSVGElement("ellipse");
ellipseElement.setAttribute("cx", "150");
ellipseElement.setAttribute("cy", "150");
ellipseElement.setAttribute("rx", "100");
ellipseElement.setAttribute("ry", "150");
ellipseElement.setAttribute("fill", "url(#gradient)");

var defsElement = createSVGElement("defs");
rootSVGElement.appendChild(defsElement);

var linearGradientElement = createSVGElement("linearGradient");
linearGradientElement.setAttribute("id", "gradient");
linearGradientElement.setAttribute("y1", "-200%");
linearGradientElement.setAttribute("y2", "1000%");

var firstStopElement = createSVGElement("stop");
firstStopElement.setAttribute("offset", "0");
firstStopElement.setAttribute("stop-color", "red");
linearGradientElement.appendChild(firstStopElement);

var lastStopElement = createSVGElement("stop");
lastStopElement.setAttribute("offset", "1");
lastStopElement.setAttribute("stop-color", "green");
linearGradientElement.appendChild(lastStopElement);

defsElement.appendChild(linearGradientElement);
rootSVGElement.appendChild(ellipseElement);

shouldBeEqualToString("linearGradientElement.y2.baseVal.valueAsString", "1000%");

function executeTest() {
    linearGradientElement.y2.baseVal.valueAsString = "0%";
    shouldBeEqualToString("linearGradientElement.y2.baseVal.valueAsString", "0%");

    completeTest();
}

startTest(ellipseElement, 150, 150);
