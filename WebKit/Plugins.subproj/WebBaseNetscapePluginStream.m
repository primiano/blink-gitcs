/*	
        WebBaseNetscapePluginStream.m
	Copyright (c) 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/WebBaseNetscapePluginStream.h>
#import <WebKit/WebBaseNetscapePluginView.h>
#import <WebKit/WebKitLogging.h>
#import <WebKit/WebNetscapePluginPackage.h>
#import <WebKit/WebNSURLExtras.h>

#import <Foundation/NSURLResponse.h>
#import <Foundation/NSURLResponsePrivate.h>
#import <Foundation/NSFileManager_NSURLExtras.h>
#import <Foundation/NSURL_NSURLExtras.h>

#define WEB_REASON_NONE -1

@implementation WebBaseNetscapePluginStream

- (void)dealloc
{
    ASSERT(stream.ndata == nil);

    // FIXME: It's generally considered bad style to do work, like deleting a file,
    // at dealloc time. We should change things around so that this is done at a
    // more well-defined time rather than when the last release happens.
    if (path) {
        unlink(path);
    }

    [URL release];
    free((void *)stream.url);
    free(path);
    [plugin release];
    [deliveryData release];
    
    [super dealloc];
}

- (uint16)transferMode
{
    return transferMode;
}

- (void)setPluginPointer:(NPP)pluginPointer
{
    instance = pluginPointer;
    
    plugin = [[(WebBaseNetscapePluginView *)instance->ndata plugin] retain];

    NPP_NewStream = 	[plugin NPP_NewStream];
    NPP_WriteReady = 	[plugin NPP_WriteReady];
    NPP_Write = 	[plugin NPP_Write];
    NPP_StreamAsFile = 	[plugin NPP_StreamAsFile];
    NPP_DestroyStream = [plugin NPP_DestroyStream];
    NPP_URLNotify = 	[plugin NPP_URLNotify];
}

- (void)setNotifyData:(void *)theNotifyData
{
    notifyData = theNotifyData;
}

- (void)startStreamWithURL:(NSURL *)theURL 
     expectedContentLength:(long long)expectedContentLength
          lastModifiedDate:(NSDate *)lastModifiedDate
                  MIMEType:(NSString *)MIMEType
{
    if (![plugin isLoaded]) {
        return;
    }
    
    [theURL retain];
    [URL release];
    URL = theURL;

    free((void *)stream.url);
    stream.url = strdup([URL _web_URLCString]);

    stream.ndata = self;
    stream.end = expectedContentLength > 0 ? expectedContentLength : 0;
    stream.lastmodified = [lastModifiedDate timeIntervalSince1970];
    stream.notifyData = notifyData;
    
    transferMode = NP_NORMAL;
    offset = 0;
    reason = WEB_REASON_NONE;

    // FIXME: Need a way to check if stream is seekable

    NPError npErr = NPP_NewStream(instance, (char *)[MIMEType cString], &stream, NO, &transferMode);
    LOG(Plugins, "NPP_NewStream URL=%@ MIME=%@ error=%d", URL, MIMEType, npErr);

    if (npErr != NPERR_NO_ERROR) {
        ERROR("NPP_NewStream failed with error: %d URLString: %s", npErr, [URL _web_URLCString]);
        // Calling cancelWithReason with WEB_REASON_NONE cancels the load, but doesn't call NPP_DestroyStream.
        [self cancelWithReason:WEB_REASON_NONE];
        return;
    }

    switch (transferMode) {
        case NP_NORMAL:
            LOG(Plugins, "Stream type: NP_NORMAL");
            break;
        case NP_ASFILEONLY:
            LOG(Plugins, "Stream type: NP_ASFILEONLY");
            break;
        case NP_ASFILE:
            LOG(Plugins, "Stream type: NP_ASFILE");
            break;
        case NP_SEEK:
            ERROR("Stream type: NP_SEEK not yet supported");
            [self cancelWithReason:NPRES_NETWORK_ERR];
            break;
        default:
            ERROR("unknown stream type");
    }
}

- (void)startStreamWithResponse:(NSURLResponse *)r
{
    [self startStreamWithURL:[r URL]
       expectedContentLength:[r expectedContentLength]
            lastModifiedDate:[r _lastModifiedDate]
                    MIMEType:[r MIMEType]];
}

- (void)destroyStream
{
    if (![plugin isLoaded] || !stream.ndata || [deliveryData length] > 0 || reason == WEB_REASON_NONE) {
        return;
    }
    
    if (reason == NPRES_DONE && (transferMode == NP_ASFILE || transferMode == NP_ASFILEONLY)) {
        ASSERT(path != NULL);
        NSString *carbonPath = [[NSFileManager defaultManager] _web_carbonPathForPath:[NSString stringWithCString:path]];
        NPP_StreamAsFile(instance, &stream, [carbonPath cString]);
        LOG(Plugins, "NPP_StreamAsFile URL=%@ path=%@", URL, carbonPath);
    }
    
    NPError npErr;
    npErr = NPP_DestroyStream(instance, &stream, reason);
    LOG(Plugins, "NPP_DestroyStream URL=%@ error=%d", URL, npErr);
    
    stream.ndata = nil;
        
    if (notifyData) {
        NPP_URLNotify(instance, [URL _web_URLCString], reason, notifyData);
        LOG(Plugins, "NPP_URLNotify URL=%@ reason=%d", URL, reason);
    }
}

- (void)destroyStreamWithReason:(NPReason)theReason
{
    reason = theReason;
    [self destroyStream];
}

- (void)destroyStreamWithFailingReason:(NPReason)theReason
{
    ASSERT(theReason != NPRES_DONE);
    // Stop any pending data from being streamed.
    [deliveryData setLength:0];
    [self destroyStreamWithReason:theReason];
    stream.ndata = nil;
}

- (void)receivedError:(NSError *)error
{
    if ([[error domain] isEqualToString:NSURLErrorDomain] && [error code] == NSURLErrorCancelled) {
        [self destroyStreamWithFailingReason:NPRES_USER_BREAK];
    } else {
        [self destroyStreamWithFailingReason:NPRES_NETWORK_ERR];
    }
}

- (void)cancelWithReason:(NPReason)theReason
{
    [self destroyStreamWithFailingReason:theReason];
}

- (void)finishedLoadingWithData:(NSData *)data
{
    if (![plugin isLoaded] || !stream.ndata) {
        return;
    }
    
    if ((transferMode == NP_ASFILE || transferMode == NP_ASFILEONLY) && !path) {
        path = strdup("/tmp/WebKitPlugInStreamXXXXXX");
        int fd = mkstemp(path);
        if (fd == -1) {
            // This should almost never happen.
            ERROR("can't make temporary file, almost certainly a problem with /tmp");
            // This is not a network error, but the only error codes are "network error" and "user break".
            [self destroyStreamWithFailingReason:NPRES_NETWORK_ERR];
            free(path);
            path = NULL;
            return;
        }
        int dataLength = [data length];
        if (dataLength > 0) {
            int byteCount = write(fd, [data bytes], dataLength);
            if (byteCount != dataLength) {
                // This happens only rarely, when we are out of disk space or have a disk I/O error.
                ERROR("error writing to temporary file, errno %d", errno);
                close(fd);
                // This is not a network error, but the only error codes are "network error" and "user break".
                [self destroyStreamWithFailingReason:NPRES_NETWORK_ERR];
                free(path);
                path = NULL;
                return;
            }
        }
        close(fd);
    }

    [self destroyStreamWithReason:NPRES_DONE];
}

- (void)deliverData
{
    if (![plugin isLoaded] || !stream.ndata || [deliveryData length] == 0) {
        return;
    }
    
    int32 totalBytes = [deliveryData length];
    int32 totalBytesDelivered = 0;
    
    while (totalBytesDelivered < totalBytes) {
        int32 deliveryBytes = NPP_WriteReady(instance, &stream);
        LOG(Plugins, "NPP_WriteReady URL=%@ bytes=%d", URL, deliveryBytes);
        
        if (deliveryBytes <= 0) {
            // Plug-in can't receive anymore data right now. Send it later.
            [self performSelector:@selector(deliverData) withObject:nil afterDelay:0];
            break;
        } else {
            deliveryBytes = MIN(deliveryBytes, totalBytes - totalBytesDelivered);
            NSData *subdata = [deliveryData subdataWithRange:NSMakeRange(totalBytesDelivered, deliveryBytes)];
            deliveryBytes = NPP_Write(instance, &stream, offset, [subdata length], (void *)[subdata bytes]);
            deliveryBytes = MIN((unsigned)deliveryBytes, [subdata length]);
            offset += deliveryBytes;
            totalBytesDelivered += deliveryBytes;
            LOG(Plugins, "NPP_Write URL=%@ bytes=%d total-delivered=%d/%d", URL, deliveryBytes, offset, stream.end);
        }
    }
    
    if (totalBytesDelivered > 0) {
        if (totalBytesDelivered < totalBytes) {
            NSMutableData *newDeliveryData = [[NSMutableData alloc] initWithCapacity:totalBytes - totalBytesDelivered];
            [newDeliveryData appendBytes:(char *)[deliveryData bytes] + totalBytesDelivered length:totalBytes - totalBytesDelivered];
            [deliveryData release];
            deliveryData = newDeliveryData;
        } else {
            [deliveryData setLength:0];
            [self destroyStream];
        }
    }
}

- (void)receivedData:(NSData *)data
{
    ASSERT([data length] > 0);
    
    if (transferMode != NP_ASFILEONLY) {
        if (!deliveryData) {
            deliveryData = [[NSMutableData alloc] initWithCapacity:[data length]];
        }
        [deliveryData appendData:data];
        [self deliverData];
    }
}

@end
