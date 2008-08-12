// [Name] SVGMaskElement-dom-width-attr.js
// [Expected rendering result] half-opaque blue rect, with green circle in center - and a series of PASS messages

description("Tests dynamic updates of the 'width' attribute of the SVGMaskElement object")
createSVGTestCase();

var maskElement = createSVGElement("mask");
maskElement.setAttribute("id", "mask");
maskElement.setAttribute("width", "10%");
maskElement.setAttribute("height", "100%");

var circleElement = createSVGElement("circle");
circleElement.setAttribute("cx", "32.5%");
circleElement.setAttribute("cy", "32.5%");
circleElement.setAttribute("r", "25%");
circleElement.setAttribute("fill", "#ffffff");
maskElement.appendChild(circleElement);

var defsElement = createSVGElement("defs");
defsElement.appendChild(maskElement);
rootSVGElement.appendChild(defsElement);

var unclippedRectElement = createSVGElement("rect");
unclippedRectElement.setAttribute("opacity", "0.5");
unclippedRectElement.setAttribute("width", "200");
unclippedRectElement.setAttribute("height", "200");
unclippedRectElement.setAttribute("fill", "blue");
rootSVGElement.appendChild(unclippedRectElement);

var rectElement = createSVGElement("rect");
rectElement.setAttribute("mask", "url(#mask)");
rectElement.setAttribute("width", "200");
rectElement.setAttribute("height", "200");
rectElement.setAttribute("fill", "green");
rootSVGElement.appendChild(rectElement);

shouldBeEqualToString("maskElement.getAttribute('width')", "10%");

function executeTest() {
    maskElement.setAttribute("width", "100%");
    shouldBeEqualToString("maskElement.getAttribute('width')", "100%");

    completeTest();
}

startTest(rectElement, 150, 150);
