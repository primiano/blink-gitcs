description('Test behavior of Document.replaceChild() when oldChild is null.');

shouldThrow('document.replaceChild(document.firstChild, null)', '"NotFoundError: Failed to execute \'replaceChild\' on \'Node\': The node to be replaced is null."');
