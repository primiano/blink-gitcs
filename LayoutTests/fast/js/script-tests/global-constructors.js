description("This test documents our set of global constructors we expose on the window object (FF and Opera don't expose them on the window, btw).  This also checks to make sure than any constructor attribute we expose has the expected constructor type.")

var constructorNames = [];

for (var name in window) {
    var value = window[name];
    var re = new RegExp("Constructor]$");
    var isConstructor = re.exec(value);
    if (isConstructor)
        constructorNames.push(name);
}

constructorNames.sort();

for (var x in constructorNames) {
    var name = constructorNames[x];
    var expectedConstructorName = "'[object " + name + "Constructor]'";

    // Ignore these properties because they do not exist in all implementations. They will be tested separately
    if (name == "WebGLRenderingContext" ||
        name == "WebGLArrayBuffer" ||
        name =="WebGLByteArray" ||
        name =="WebGLFloatArray" ||
        name =="WebGLIntArray" ||
        name =="WebGLShortArray" ||
        name =="WebGLUnsignedByteArray" ||
        name =="WebGLUnsignedIntArray" ||
        name =="WebGLUnsignedShortArray" ||
        name == "ArrayBuffer" ||
        name == "Int8Array" ||
        name == "Uint8Array" ||
        name == "Int16Array" ||
        name == "Uint16Array" ||
        name == "Int32Array" ||
        name == "Uint32Array" ||
        name == "FloatArray" ||
        name == "FileError" ||
        name == "FileReader")
        continue;

    if (name == "XMLDocument")
        // Gecko exposes an "XMLDocument" constructor, but we just use Document for XML documents instead of a custom sub-type
        expectedConstructorName = "'[object DocumentConstructor]'";

    shouldBe("" + name + ".toString()", expectedConstructorName);
}

var successfullyParsed = true;
