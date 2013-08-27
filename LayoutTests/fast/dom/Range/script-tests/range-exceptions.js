description(
"This test checks some DOM Range exceptions."
);

// Test to be sure the name BAD_BOUNDARYPOINTS_ERR dumps properly.
var node = document.createElement("DIV");
node.innerHTML = "<BAR>AB<MOO>C</MOO>DE</BAR>";
shouldBe("node.innerHTML", "'<bar>AB<moo>C</moo>DE</bar>'");

// Ensure that we throw BAD_BOUNDARYPOINTS_ERR when trying to split a comment
// (non-text but character-offset node). (Test adapted from Acid3.)
var c1 = document.createComment("aaaaa");
node.appendChild(c1);
var c2 = document.createComment("bbbbb");
node.appendChild(c2);
var r = document.createRange();
r.setStart(c1, 2);
r.setEnd(c2, 3);
shouldThrow("r.surroundContents(document.createElement('a'))", '"InvalidStateError: An attempt was made to use an object that is not, or is no longer, usable."');

// But not when we don't try to split the comment.
r.setStart(c1, 0);
r.setEnd(c1, 5);
shouldThrow("r.surroundContents(document.createElement('a'))", '"HierarchyRequestError: A Node was inserted somewhere it doesn\'t belong."');
