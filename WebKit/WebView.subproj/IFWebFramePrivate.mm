/*	
        IFWebFramePrivate.mm
	    
	    Copyright 2001, Apple, Inc. All rights reserved.
*/
#import <WebKit/IFWebDataSource.h>
#import <WebKit/IFWebFramePrivate.h>

@implementation IFWebFramePrivate

- (void)dealloc
{
    [name autorelease];
    [view autorelease];
    [dataSource autorelease];
    [super dealloc];
}

- (NSString *)name { return name; }
- (void)setName: (NSString *)n 
{
    [name autorelease];
    name = [n retain];
}


- view { return view; }
- (void)setView: v 
{ 
    [view autorelease];
    view = [v retain];
}

- (IFWebDataSource *)dataSource { return dataSource; }
- (void)setDataSource: (IFWebDataSource *)d
{ 
    [dataSource autorelease];
    dataSource = [d retain];
}


- (IFWebDataSource *)provisionalDataSource { return provisionalDataSource; }
- (void)setProvisionalDataSource: (IFWebDataSource *)d
{ 
    [provisionalDataSource autorelease];
    provisionalDataSource = [d retain];
}


- (void *)renderFramePart { return renderFramePart; }
- (void)setRenderFramePart: (void *)p 
{
    renderFramePart = p;
}


@end
