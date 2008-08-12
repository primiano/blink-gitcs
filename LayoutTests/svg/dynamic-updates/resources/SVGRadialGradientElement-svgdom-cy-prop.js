// [Name] SVGRadialGradientElement-svgdom-cy-prop.js
// [Expected rendering result] green ellipse, no red visible - and a series of PASS messages

description("Tests dynamic updates of the 'cy' property of the SVGRadialGradientElement object")
createSVGTestCase();

var ellipseElement = createSVGElement("ellipse");
ellipseElement.setAttribute("cx", "150");
ellipseElement.setAttribute("cy", "150");
ellipseElement.setAttribute("rx", "100");
ellipseElement.setAttribute("ry", "150");
ellipseElement.setAttribute("fill", "url(#gradient)");

var defsElement = createSVGElement("defs");
rootSVGElement.appendChild(defsElement);

var radialGradientElement = createSVGElement("radialGradient");
radialGradientElement.setAttribute("id", "gradient");
radialGradientElement.setAttribute("cy", "0%");

var firstStopElement = createSVGElement("stop");
firstStopElement.setAttribute("offset", "0");
firstStopElement.setAttribute("stop-color", "red");
radialGradientElement.appendChild(firstStopElement);

var lastStopElement = createSVGElement("stop");
lastStopElement.setAttribute("offset", "1");
lastStopElement.setAttribute("stop-color", "green");
radialGradientElement.appendChild(lastStopElement);

defsElement.appendChild(radialGradientElement);
rootSVGElement.appendChild(ellipseElement);

shouldBeEqualToString("radialGradientElement.cy.baseVal.valueAsString", "0%");

function executeTest() {
    radialGradientElement.cy.baseVal.valueAsString = "150%";
    shouldBeEqualToString("radialGradientElement.cy.baseVal.valueAsString", "150%");

    completeTest();
}

startTest(ellipseElement, 150, 150);
