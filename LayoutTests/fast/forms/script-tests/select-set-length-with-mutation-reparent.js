description('Tests that setting .length on an HTMLSelectElement works in the presence of mutation event listeners that reparent options');

var sel = document.createElement('select');
document.body.appendChild(sel);
var otherSel = document.createElement('select');
document.body.appendChild(otherSel);

function onRemove(e) {
    if (e.target.nextSibling != null) {
        // remove listener temporarily to avoid lots of nesting
        sel.removeEventListener('DOMNodeRemoved', onRemove, false);
        var n = e.target.nextSibling;
        n.parentNode.removeChild(n);
        otherSel.appendChild(n);
        sel.addEventListener('DOMNodeRemoved', onRemove, false);
    }
}

sel.addEventListener('DOMNodeRemoved', onRemove, false);
sel.addEventListener('DOMNodeInserted', function() {}, false);

sel.length = 200;
shouldBe('sel.length', '200');
shouldBe('otherSel.length', '0');

sel.length = 100;
shouldBe('sel.length', '100');
shouldBe('otherSel.length', '0');

sel.length = 180;
shouldBe('sel.length', '180');
shouldBe('otherSel.length', '0');

var successfullyParsed = true;
