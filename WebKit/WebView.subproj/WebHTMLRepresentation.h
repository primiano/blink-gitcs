/*	
   	WebHTMLRepresentation.h
	Copyright 2002, Apple, Inc. All rights reserved.
*/

#import <Foundation/Foundation.h>

@class WebHTMLRepresentationPrivate;
@class NSView;

@class DOMDocument;
@class DOMNode;
@class DOMElement;

@protocol WebDocumentRepresentation;
@protocol WebDocumentSourceRepresentation;

/*!
    @class WebHTMLRepresentation
*/
@interface WebHTMLRepresentation : NSObject <WebDocumentRepresentation>
{
    WebHTMLRepresentationPrivate *_private;
}

/*!
    @method DOMDocument
    @abstract return the DOM document for this data source.
*/
- (DOMDocument *)DOMDocument;

/*!
    @method setSelectionFrom:startOffset:to:endOffset
    @abstract Set the text selection in the document.
    @param start The node that include the starting selection point.
    @param startOffset The character offset into the text of the starting node.
    @param end The node that includes the ending selection point.
    @param endOffset The character offset into the text of the ending node.
*/
- (void)setSelectionFrom:(DOMNode *)start startOffset:(int)startOffset to:(DOMNode *)end endOffset:(int)endOffset;

//- (NSAttributedString *)selectedAttributedString;

/*!
    @method documentSource
    @abstract Get the current HTML reconstructed from the current state of the DOM.
*/
- (NSString *)reconstructedDocumentSource;


- (NSAttributedString *)attributedStringFrom:(DOMNode *)startNode startOffset:(int)startOffset to:(DOMNode *)endNode endOffset:(int)endOffset;

- (DOMElement *)elementWithName:(NSString *)name inForm:(DOMElement *)form;
- (DOMElement *)elementForView:(NSView *)view;
- (BOOL)elementDoesAutoComplete:(DOMElement *)element;
- (BOOL)elementIsPassword:(DOMElement *)element;
- (DOMElement *)formForElement:(DOMElement *)element;
- (DOMElement *)currentForm;
- (NSArray *)controlsInForm:(DOMElement *)form;
- (NSString *)searchForLabels:(NSArray *)labels beforeElement:(DOMElement *)element;
- (NSString *)matchLabels:(NSArray *)labels againstElement:(DOMElement *)element;

- (NSString *)HTMLString;

@end
