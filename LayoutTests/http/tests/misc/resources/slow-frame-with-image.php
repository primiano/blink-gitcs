<?
header("Content-Type: text/html; charset=utf-8");
?>
<!-- If the scheduler has been disabled as it should, the image will load immediately rather than waiting for this frame to finish. -->
<html>
<head>
<script src="/resources/js-test-pre.js"></script>
<script>
window.jsTestIsAsync = true;
if (self.testRunner)
    testRunner.waitUntilDone();

var is_image_done = false;
var is_dom_content_loaded = false;

function imageDone() {
    is_image_done = true;
    shouldBeFalse("is_dom_content_loaded");
    checkDone();
}

function bodyDone() {
    is_dom_content_loaded = true;
    shouldBeTrue("is_image_done");
    checkDone();
}

function checkDone() {
    if (is_image_done && is_dom_content_loaded) {
        finishJSTest();
    }
}

document.addEventListener("DOMContentLoaded", bodyDone);

var successfullyParsed = true;
</script>
</head>
<body>
<div id="console"></div>
<img src="compass.jpg" onload="imageDone()">
<?php
ob_flush();
flush();
sleep(2);
?>
<script src="/resources/js-test-post.js"></script>
</body>
</html>
