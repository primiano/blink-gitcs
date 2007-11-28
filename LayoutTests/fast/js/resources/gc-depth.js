description("This test makes sure that deep object structures don't lead to GC crashes.")

var a = [];
for (var i = 0; i < 200000; i++) {
    a = [a];
}

var b = "";
for (var i = 0; i < 80000; i++) {
    b += "b";
}

var successfullyParsed = true;
description("This test makes sure that deep object structures don't lead to GC crashes.")

var a = [];
for (var i = 0; i < 200000; i++) {
    a = [a];
}

var b = "";
for (var i = 0; i < 80000; i++) {
    b += "b";
}

var successfullyParsed = true;
