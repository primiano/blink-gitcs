description("Tests whether changes to the 'currentTranslate' SVGSVGElement property take effect. You should see an unclipped 100x100 rect above.");

var svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
svg.setAttribute("width", "150");
svg.setAttribute("height", "150");
document.documentElement.insertBefore(svg, document.documentElement.firstChild);

var rect = document.createElementNS("http://www.w3.org/2000/svg", "rect");
rect.setAttribute("x", "-100");
rect.setAttribute("y", "-100");
rect.setAttribute("width", "100");
rect.setAttribute("height", "100");
rect.setAttribute("fill", "green");
svg.appendChild(rect);

shouldBe("svg.currentTranslate.x", "0");
shouldBe("svg.currentTranslate.y", "0");

// Modify tear off, and check results by calling svg.currentTranslate.
var currentTranslate = svg.currentTranslate;
currentTranslate.x = 100;
shouldBe("svg.currentTranslate.x", "100");

currentTranslate.y = 50;
shouldBe("svg.currentTranslate.y", "50");

currentTranslate.y = currentTranslate.y + 50;
shouldBe("svg.currentTranslate.y", "100");

var successfullyParsed = true;
