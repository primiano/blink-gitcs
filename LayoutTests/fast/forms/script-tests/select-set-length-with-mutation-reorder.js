description('Tests that setting the .length on an HTMLSelectElement works in the presence of DOM mutation listeners that reorder option elements');

var wrapper = document.createElement('div');
document.body.appendChild(wrapper);
wrapper.innerHTML = '<select id="theSelect">' +
                    '<option id="a">a</option>' +
                    '<option id="b">b</option>' +
                    '<option id="c">c</option>' +
                    '<option id="d">d</option>' +
                    '</select>';

var sel = document.getElementById('theSelect');

var firstRemove = true;
function onRemove(e) {
    if (firstRemove) {
        // remove listener temporarily to avoid lots of nesting
        sel.removeEventListener('DOMNodeRemoved', onRemove, false);
        var lastOption = document.getElementById('d');
        sel.removeChild(lastOption);
        sel.insertBefore(lastOption, document.getElementById('c'));
        firstRemove = false;
        sel.addEventListener('DOMNodeRemoved', onRemove, false);
    }
}

sel.addEventListener('DOMNodeRemoved', onRemove, false);
sel.addEventListener('DOMNodeInserted', function() {}, false);

shouldBe('sel.length', '4');
sel.length = 2;
shouldBe('sel.length', '2');
shouldBe('sel.options.item(0).id', '"a"');
shouldBe('sel.options.item(1).id', '"b"');

var successfullyParsed = true;
