// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "osx_file_icon.h"
#include <zen/osx_throw_exception.h>
#include <zen/scope_guard.h>
#include <zen/basic_math.h>

namespace
{
osx::ImageData extractBytes(NSImage* nsImg, int requestedSize) //throw SysError; NSException?
{
    /*
	wxBitmap(NSImage*) is not good enough: it calls "[NSBitmapImageRep imageRepWithData:[img TIFFRepresentation]]"
		=> inefficient: TIFFRepresentation converts all contained images of an NSImage
		=> lacking: imageRepWithData extracts the first contained image only!
		=> wxBitmap(NSImage*) is wxCocoa only, deprecated!
		=> wxWidgets generally is not thread-safe so care must be taken to use wxBitmap from main thread only! (e.g. race-condition on non-atomic ref-count!!!)
		
	-> we need only a single bitmap at a specific size => extract raw bytes for use with wxImage in a thread-safe way!
	*/

    //we choose the Core Graphics solution; for the equivalent App-Kit way see: http://www.cocoabuilder.com/archive/cocoa/193131-is-lockfocus-main-thread-specific.html#193191

    ZEN_OSX_ASSERT(requestedSize > 0);
    NSRect rectProposed = NSMakeRect(0, 0, requestedSize, requestedSize); //this is merely a hint!

	CGImageRef imgRef = [nsImg CGImageForProposedRect:&rectProposed context:nil hints:nil];
    ZEN_OSX_ASSERT(imgRef != NULL); //can this fail? not documented; ownership?? not documented!

    const size_t width  = ::CGImageGetWidth (imgRef);
    const size_t height = ::CGImageGetHeight(imgRef);

    ZEN_OSX_ASSERT(width > 0 && height > 0 && requestedSize > 0);

    int trgWidth  = width;
    int trgHeight = height;

    const int maxExtent = std::max(width, height); //don't stretch small images, but shrink large ones instead!
    if (requestedSize < maxExtent)
    {
        trgWidth  = width  * requestedSize / maxExtent;
        trgHeight = height * requestedSize / maxExtent;
    }

    CGColorSpaceRef colorSpace = ::CGColorSpaceCreateDeviceRGB();
    ZEN_OSX_ASSERT(colorSpace != NULL); //may fail
    ZEN_ON_SCOPE_EXIT(::CGColorSpaceRelease(colorSpace));

    std::vector<unsigned char> buf(trgWidth* trgHeight * 4); //32-bit ARGB, little endian byte order -> already initialized with 0 = fully transparent

    //supported color spaces: https://developer.apple.com/library/mac/#documentation/GraphicsImaging/Conceptual/drawingwithquartz2d/dq_context/dq_context.html#//apple_ref/doc/uid/TP30001066-CH203-BCIBHHBB
    CGContextRef ctxRef = ::CGBitmapContextCreate(&buf[0],      //void *data,
                                                trgWidth,     //size_t width,
                                                trgHeight,    //size_t height,
                                                8,            //size_t bitsPerComponent,
                                                4 * trgWidth, //size_t bytesPerRow,
                                                colorSpace,   //CGColorSpaceRef colorspace,
                                                kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little); //CGBitmapInfo bitmapInfo
    ZEN_OSX_ASSERT(ctxRef != NULL);
    ZEN_ON_SCOPE_EXIT(::CGContextRelease(ctxRef));

    ::CGContextDrawImage(ctxRef, CGRectMake(0, 0, trgWidth, trgHeight), imgRef); //can this fail?  not documented

    //::CGContextFlush(ctxRef); //"If you pass [...] a bitmap context, this function does nothing."

    osx::ImageData imgOut(trgWidth, trgHeight);

    auto it = buf.begin();
    auto itOutRgb = imgOut.rgb.begin();
    auto itOutAlpha = imgOut.alpha.begin();
    for (int i = 0; i < trgWidth * trgHeight; ++i)
    {
        const unsigned char b = *it++;
        const unsigned char g = *it++;
        const unsigned char r = *it++;
        const unsigned char a = *it++;

        //unsigned arithmetics caveat!
        auto demultiplex = [&](unsigned char c) { return static_cast<unsigned char>(numeric::clampCpy(a == 0 ? 0 : (c * 255 + a - 1) / a, 0, 255)); }; //=ceil(c * 255 / a)

        *itOutRgb++ = demultiplex(r);
        *itOutRgb++ = demultiplex(g);
        *itOutRgb++ = demultiplex(b);
        *itOutAlpha++ = a;
    }

    return imgOut;
}
}


osx::ImageData osx::getThumbnail(const char* filename, int requestedSize) //throw SysError
{
    @try
    {
        @autoreleasepool
        {
			NSString* nsFile = [NSString stringWithCString:filename encoding:NSUTF8StringEncoding];
            ZEN_OSX_ASSERT(nsFile != nil); //throw SysError; can this fail?  not documented
            //stringWithCString returns string which is already set to autorelease!

			NSImage* nsImg = [[[NSImage alloc] initWithContentsOfFile:nsFile] autorelease];
            ZEN_OSX_ASSERT(nsImg != nil); //may fail

            return extractBytes(nsImg, requestedSize); //throw SysError
        }
    }
    @catch(NSException* e)
    {
        throwSysError(e); //throw SysError
    }
}


osx::ImageData osx::getFileIcon(const char* filename, int requestedSize) //throw SysError
{
    @try
    {
        @autoreleasepool
        {
			NSString* nsFile = [NSString stringWithCString:filename encoding:NSUTF8StringEncoding];
            ZEN_OSX_ASSERT(nsFile != nil); //throw SysError; can this fail?  not documented
            //stringWithCString returns string which is already set to autorelease!

			NSImage* nsImg = [[NSWorkspace sharedWorkspace] iconForFile:nsFile];
            ZEN_OSX_ASSERT(nsImg != nil); //can this fail?  not documented

            return extractBytes(nsImg, requestedSize); //throw SysError
        }
    }
    @catch (NSException* e)
    {
        throwSysError(e); //throw SysError
    }
}


osx::ImageData osx::getDefaultFileIcon(int requestedSize) //throw SysError
{
    @try
    {
        @autoreleasepool
        {
			NSImage* nsImg = [[NSWorkspace sharedWorkspace] iconForFileType:NSFileTypeForHFSTypeCode(kGenericDocumentIcon)];
            //NSImage* nsImg = [[NSWorkspace sharedWorkspace] iconForFileType:@"dat"];
            ZEN_OSX_ASSERT(nsImg != nil); //can this fail?  not documented

            return extractBytes(nsImg, requestedSize); //throw SysError
        }
    }
    @catch (NSException* e)
    {
        throwSysError(e); //throw SysError
    }
}


osx::ImageData osx::getDefaultFolderIcon(int requestedSize) //throw SysError
{
    @try
    {
        @autoreleasepool
        {
			NSImage* nsImg = [NSImage imageNamed:NSImageNameFolder];
            //NSImage* nsImg = [[NSWorkspace sharedWorkspace] iconForFileType:NSFileTypeForHFSTypeCode(kGenericFolderIcon)];
            ZEN_OSX_ASSERT(nsImg != nil); //may fail

            return extractBytes(nsImg, requestedSize); //throw SysError
        }
    }
    @catch (NSException* e)
    {
        throwSysError(e); //throw SysError
    }
}
