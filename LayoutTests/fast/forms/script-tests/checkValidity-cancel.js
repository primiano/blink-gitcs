description('Tests for checkValidity() with invalid event canceling');

var parent = document.createElement('div');
document.body.appendChild(parent);
parent.innerHTML = '<form><input name=i required></form>';
var form = parent.firstChild;
var input = form.firstChild;

debug('"invalid" event is not canceled.');
var invalidFired  = false;
var nothingListener = {};
nothingListener.handleEvent = function(event) {
    invalidFired = true;
};
shouldBeTrue('input.addEventListener("invalid", nothingListener, false); !input.checkValidity() && invalidFired');
shouldBeTrue('invalidFired = false; !form.checkValidity() && invalidFired');
input.removeEventListener('invalid', nothingListener, false);

debug('');
debug('"invalid" event is canceled.');
invalidFired = false;
var cancelListener = {};
cancelListener.handleEvent = function(event) {
    invalidFired = true;
    event.preventDefault();
};
// Even if 'invalid' is canceled, the input.checkValidity() result is still false.
shouldBeTrue('input.addEventListener("invalid", cancelListener, false); !input.checkValidity() && invalidFired');
// form.checkValidity() should be true.
shouldBeTrue('invalidFired = false; form.checkValidity() && invalidFired');

var successfullyParsed = true;
