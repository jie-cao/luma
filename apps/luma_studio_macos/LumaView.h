// LUMA Studio - Metal View
#pragma once

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

@interface LumaView : NSView

@property (nonatomic, readonly) CAMetalLayer* metalLayer;

@end
