description('This tests that the width of textareas and inputs is correctly calculated based on the metrics of the SVG font.');

var styleElement = document.createElement('style');
// FIXME: Is there a better way to create a font-face from JS?
styleElement.innerText = '@font-face { font-family: "SVGraffiti"; src: url("resources/graffiti.svg#SVGraffiti") format(svg) }';
document.getElementsByTagName('head')[0].appendChild(styleElement);

var textarea = document.createElement('textarea');
textarea.style.fontFamily = 'SVGraffiti';
textarea.cols = 20;
document.body.appendChild(textarea);

var input = document.createElement('input');
input.style.fontFamily = 'SVGraffiti';
input.size = 20;
document.body.appendChild(input);

// Force a layout to ensure SVGGraffiti gets loaded.
// Needs to happen before onLoad.
document.body.offsetWidth;

// Need to wait for the load event to make sure the font is loaded.
window.addEventListener('load', function()
{
    shouldBe(String(textarea.offsetWidth), '116');
    shouldBe(String(input.offsetWidth), '103');
}, true);

var successfullyParsed = true;
