description('Tests for .valueAsDate with &lt;input type=time>.');

var input = document.createElement('input');
input.type = 'time';

function valueAsDateFor(stringValue) {
    input.value = stringValue;
    return input.valueAsDate;
}

function setValueAsDateAndGetValue(hour, minute, second, ms) {
    var date = new Date();
    date.setTime(((hour * 60 + minute) * 60 + second) * 1000 + ms);
    input.valueAsDate = date;
    return input.value;
}

shouldBe('valueAsDateFor("")', 'null');
shouldBe('valueAsDateFor("00:00:00.000").getTime()', 'Date.UTC(1970, 0, 1, 0, 0, 0, 0)');
shouldBe('valueAsDateFor("04:35").getTime()', 'Date.UTC(1970, 0, 1, 4, 35, 0, 0)');
shouldBe('valueAsDateFor("23:59:59.999").getTime()', 'Date.UTC(1970, 0, 1, 23, 59, 59, 999)');

shouldBe('setValueAsDateAndGetValue(0, 0, 0, 0)', '"00:00"');
shouldBe('setValueAsDateAndGetValue(0, 0, 1, 0)', '"00:00:01"');
shouldBe('setValueAsDateAndGetValue(0, 0, 0, 2)', '"00:00:00.002"');
shouldBe('setValueAsDateAndGetValue(11, 59, 59, 999)', '"11:59:59.999"');
shouldBe('setValueAsDateAndGetValue(12, 0, 0, 0)', '"12:00"');
shouldBe('setValueAsDateAndGetValue(23, 59, 59, 999)', '"23:59:59.999"');
shouldBe('setValueAsDateAndGetValue(24, 0, 0, 0)', '"00:00"');
shouldBe('setValueAsDateAndGetValue(48, 0, 13, 0)', '"00:00:13"');
shouldBe('setValueAsDateAndGetValue(-23, -59, -59, 0)', '"00:00:01"');

debug('Invalid Date:');
// Date of JavaScript can't represent 8.65E15.
shouldBe('var date = new Date(); date.setTime(8.65E15); input.valueAsDate = date; input.value', '""');
debug('Invalid objects:');
shouldBe('input.value = "00:00"; input.valueAsDate = document; input.value', '""');
shouldBe('input.value = "00:00"; input.valueAsDate = null; input.value', '""');

debug('Step attribute value and string representation:');
// If the step attribute value is 1 second and the second part is 0, we should show the second part.
shouldBe('input.step = "1"; setValueAsDateAndGetValue(0, 0, 0, 0)', '"00:00:00"');
// If the step attribute value is 0.001 second and the millisecond part is 0, we should show the millisecond part.
shouldBe('input.step = "0.001"; setValueAsDateAndGetValue(0, 0, 0, 0)', '"00:00:00.000"');

var successfullyParsed = true;
