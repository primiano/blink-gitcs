importScripts('./js-test-pre.js');

description('Test Promise.');

var global = this;

global.jsTestIsAsync = true;

var resolver;

var firstPromise = new Promise(function(newResolver) {
  global.thisInInit = this;
  resolver = newResolver;
});

var secondPromise = firstPromise.then(function(result) {
  global.thisInFulfillCallback = this;
  shouldBeFalse('thisInFulfillCallback === firstPromise');
  shouldBeTrue('thisInFulfillCallback === secondPromise');
  global.result = result;
  shouldBeEqualToString('result', 'hello');
  finishJSTest();
});

shouldBeTrue('thisInInit === firstPromise');
shouldBeTrue('firstPromise instanceof Promise');
shouldBeTrue('secondPromise instanceof Promise');

shouldThrow('firstPromise.then(null)', '"TypeError: fulfillCallback must be a function or undefined"');
shouldThrow('firstPromise.then(undefined, null)', '"TypeError: rejectCallback must be a function or undefined"');
shouldThrow('firstPromise.then(37)', '"TypeError: fulfillCallback must be a function or undefined"');

resolver.fulfill('hello');


