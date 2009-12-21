description('Test HTMLInputElement::valueAsDate binding.');

var input = document.createElement('input');
var invalidStateError = 'Error: INVALID_STATE_ERR: DOM Exception 11';

debug('Unsuppported type:');
input.type = 'text';
shouldBe('input.valueAsDate', 'null');
shouldThrow('input.valueAsDate = date', 'invalidStateError');

debug('');
debug('Supported type:');
input.type = 'month';
input.value = '2009-12';
var valueAsDate = input.valueAsDate;
shouldBeTrue('valueAsDate != null');
shouldBe('typeof valueAsDate', '"object"');
shouldBe('valueAsDate.constructor.name', '"Date"');

debug('Sets an Epoch Date:');
var date = new Date();
date.setTime(0);
input.valueAsDate = date;
shouldBe('input.value', '"1970-01"');
shouldBe('input.valueAsDate.getTime()', '0');
debug('Sets a number 0:');
input.valueAsDate = 0;
shouldBe('input.value', '"1970-01"');
shouldBe('input.valueAsDate.getTime()', '0');
debug('Sets other types:');
shouldThrow('input.valueAsDate = null', 'invalidStateError');
shouldThrow('input.valueAsDate = undefined', 'invalidStateError');
shouldThrow('input.valueAsDate = document', 'invalidStateError');

var successfullyParsed = true;
