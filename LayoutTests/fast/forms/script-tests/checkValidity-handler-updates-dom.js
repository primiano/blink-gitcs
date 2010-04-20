description('HTMLFormElement::checkValidity() with cases that event handlers called by checkValidity() updates DOM structure.')

var parent = document.createElement('div');
document.body.appendChild(parent);

// ----------------------------------------------------------------
debug('The target form is removed.');
parent.innerHTML = '<form id=f1><input name=i id=i required></form>';
var handler = function(event) {
    parent.innerHTML = '';
};
document.getElementById('i').addEventListener('invalid', handler, false);
// The control is invalid, but it is not listed in 'unhandled invalid controls'
// because it is not in any documents.
shouldBeTrue('document.getElementById("f1").checkValidity()');

// ----------------------------------------------------------------
debug('');
debug('A control to be checked is removed.');
parent.innerHTML = '<form id=f1><input name=i1 id=i1 required><input name=i2 id=i2 required></form>';
var handler1 = function(event) {
    document.getElementById('f1').removeChild(document.getElementById('i2'));
};
document.getElementById('i1').addEventListener('invalid', handler1, false);
var handler2Called = false;
var handler2 = function(event) {
    handler2Called = true;
};
document.getElementById('i2').addEventListener('invalid', handler2, false);
shouldBeFalse('document.getElementById("f1").checkValidity()');
// If the node was removed from the form, i2.checkValidity() is called, but an
// invalid event is not fired because it is not in any documents.
shouldBeFalse('handler2Called');

// ----------------------------------------------------------------
debug('');
debug('A new control is added.');
parent.innerHTML = '<form id=f1><input name=i1 id=i1 required></form>';
handler2Called = false;
handler2 = function(event) {
    handler2Called = true;
};
handler1 = function(event) {
    var input = document.createElement('input');
    input.name = 'i2';
    input.required = true;
    input.addEventListener('invalid', handler2, false);
    document.getElementById('f1').appendChild(input);
};
document.getElementById('i1').addEventListener('invalid', handler1, false);
shouldBeFalse('document.getElementById("f1").checkValidity()');
// If a new node is added to the form, checkValidity() doesn't handle it.
shouldBeFalse('handler2Called');

// ----------------------------------------------------------------
debug('');
debug('A control is moved to another form.');
parent.innerHTML = '<form id=f1><input name=i1 id=i1 required><input name=i2 id=i2 required></form>'
    + '<form id=f2></form>';
handler1 = function(event) {
    document.getElementById('f2').appendChild(document.getElementById('i2'));
};
document.getElementById('i1').addEventListener('invalid', handler1, false);
handler2Called = false;
handler2 = function(event) {
    handler2Called = true;
};
document.getElementById('i2').addEventListener('invalid', handler2, false);
shouldBeFalse('document.getElementById("f1").checkValidity()');
// The moved control is not checked.
shouldBeFalse('handler2Called');

// ----------------------------------------------------------------
debug('');
debug('A control is moved to another document.');
parent.innerHTML = '<form id=f1><input name=i1 id=i1 required></form>';
var doc2 = document.implementation.createHTMLDocument();
handler1 = function(event) {
    doc2.body.appendChild(doc2.adoptNode(document.getElementById('i1')));
};
document.getElementById('i1').addEventListener('invalid', handler1, false);
// i1 is not listed in 'unhandled invalid controls' because it was moved to
// another document.
shouldBeTrue('document.getElementById("f1").checkValidity()');

parent.innerHTML = '';
var successfullyParsed = true;
