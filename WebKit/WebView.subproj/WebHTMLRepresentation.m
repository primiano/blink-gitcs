/*	
        WebHTMLRepresentation.m
	Copyright 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/WebHTMLRepresentation.h>

#import <WebKit/DOM.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebBridge.h>
#import <WebKit/WebKitStatisticsPrivate.h>
#import <WebKit/WebFramePrivate.h>
#import <WebKit/WebDocument.h>
#import <Foundation/NSURLResponse.h>

@interface WebHTMLRepresentationPrivate : NSObject
{
@public
    WebDataSource *dataSource;
    WebBridge *bridge;
}
@end

@implementation WebHTMLRepresentationPrivate
@end

@implementation WebHTMLRepresentation

- init
{
    [super init];
    
    _private = [[WebHTMLRepresentationPrivate alloc] init];
    
    ++WebHTMLRepresentationCount;
    
    return self;
}

- (void)dealloc
{
    --WebHTMLRepresentationCount;
    
    [_private release];

    [super dealloc];
}

- (WebBridge *)_bridge
{
    return _private->bridge;
}

- (void)setDataSource:(WebDataSource *)dataSource
{
    _private->dataSource = dataSource;
    _private->bridge = [[dataSource webFrame] _bridge];
}

- (void)receivedData:(NSData *)data withDataSource:(WebDataSource *)dataSource
{
    if ([dataSource webFrame])
        [_private->bridge receivedData:data withDataSource:dataSource];
}

- (void)receivedError:(NSError *)error withDataSource:(WebDataSource *)dataSource
{
}

- (void)finishedLoadingWithDataSource:(WebDataSource *)dataSource
{
    // Telling the bridge we received some data and passing nil as the data is our
    // way to get work done that is normally done when the first bit of data is
    // received, even for the case of a document with no data (like about:blank).
    if ([dataSource webFrame])
        [_private->bridge receivedData:nil withDataSource:dataSource];
}

- (BOOL)canProvideDocumentSource
{
    return YES;
}

- (NSString *)documentSource
{
    return [WebBridge stringWithData:[_private->dataSource data] textEncoding:[_private->bridge textEncoding]];
}

- (NSString *)title
{
    return [_private->dataSource _title];
}

- (DOMDocument *)DOMDocument
{
    return [_private->bridge DOMDocument];
}

- (void)setSelectionFrom:(DOMNode *)start startOffset:(int)startOffset to:(DOMNode *)end endOffset:(int) endOffset
{
}

- (NSString *)reconstructedDocumentSource
{
    // FIXME implement
    return @"";
}

- (NSAttributedString *)attributedText
{
    // FIXME:  Implement
    return nil;
}

- (NSAttributedString *)attributedStringFrom: (DOMNode *)startNode startOffset: (int)startOffset to: (DOMNode *)endNode endOffset: (int)endOffset
{
    return [_private->bridge attributedStringFrom: startNode startOffset: startOffset to: endNode endOffset: endOffset];
}

- (DOMElement *)elementWithName:(NSString *)name inForm:(DOMElement *)form
{
    return [_private->bridge elementWithName:name inForm:form];
}

- (DOMElement *)elementForView:(NSView *)view
{
    return [_private->bridge elementForView:view];
}

- (BOOL)elementDoesAutoComplete:(DOMElement *)element
{
    return [_private->bridge elementDoesAutoComplete:element];
}

- (BOOL)elementIsPassword:(DOMElement *)element
{
    return [_private->bridge elementIsPassword:element];
}

- (DOMElement *)formForElement:(DOMElement *)element
{
    return [_private->bridge formForElement:element];
}

- (DOMElement *)currentForm;
{
    return [_private->bridge currentForm];
}

- (NSArray *)controlsInForm:(DOMElement *)form
{
    return [_private->bridge controlsInForm:form];
}

- (NSString *)searchForLabels:(NSArray *)labels beforeElement:(DOMElement *)element
{
    return [_private->bridge searchForLabels:labels beforeElement:element];
}

- (NSString *)matchLabels:(NSArray *)labels againstElement:(DOMElement *)element
{
    return [_private->bridge matchLabels:labels againstElement:element];
}

- (NSString *)HTMLString
{		
	return [_private->bridge HTMLString:nil];
}

@end
