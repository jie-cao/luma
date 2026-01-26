// LUMA Studio - macOS App Delegate
#import "AppDelegate.h"
#import "LumaView.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    // Create window
    NSRect frame = NSMakeRect(100, 100, 1280, 720);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | 
                              NSWindowStyleMaskClosable | 
                              NSWindowStyleMaskMiniaturizable | 
                              NSWindowStyleMaskResizable;
    
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:style
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    
    [self.window setTitle:@"LUMA Studio"];
    [self.window setMinSize:NSMakeSize(800, 600)];
    [self.window center];
    
    // Create and set LumaView as content view
    LumaView* lumaView = [[LumaView alloc] initWithFrame:frame];
    [self.window setContentView:lumaView];
    
    // Show window
    [self.window makeKeyAndOrderFront:nil];
    [self.window makeFirstResponder:lumaView];
    
    NSLog(@"[luma] LUMA Studio started");
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification*)notification {
    NSLog(@"[luma] LUMA Studio shutting down");
}

@end
