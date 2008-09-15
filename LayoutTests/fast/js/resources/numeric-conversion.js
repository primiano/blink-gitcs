description(
"This test checks for accuracy in numeric conversions, particularly with large or infinite values."
);

shouldBe("Number(1152921504606847105).toString()", "'1152921504606847200'");
shouldBe("parseInt('1152921504606847105').toString()", "'1152921504606847200'");
shouldBe("(- (- '1152921504606847105')).toString()", "'1152921504606847200'");

shouldBe("Number(0x1000000000000081).toString(16)", "'1000000000000100'");
shouldBe("parseInt('0x1000000000000081', 16).toString(16)", "'1000000000000100'");
shouldBe("(- (- '0x1000000000000081')).toString(16)", "'1000000000000100'");

shouldBe("Number(0100000000000000000201).toString(8)", "'100000000000000000400'");
shouldBe("parseInt('100000000000000000201', 8).toString(8)", "'100000000000000000400'");

shouldBe("(- 'infinity').toString()", "'NaN'");

shouldBe("parseInt('1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000').toString()", "'Infinity'");
shouldBe("parseInt('0x100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000', 16).toString()", "'Infinity'");
shouldBe("parseInt('100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000', 8).toString()", "'Infinity'");

shouldBe("parseInt('9007199254740992e2000').toString()", "'9007199254740992'");
shouldBe("parseInt('9007199254740992.0e2000').toString()", "'9007199254740992'");

shouldBe("parseInt(NaN)", "0");
shouldBe("parseInt(-Infinity)", "0");
shouldBe("parseInt(Infinity)", "0");

var successfullyParsed = true;
