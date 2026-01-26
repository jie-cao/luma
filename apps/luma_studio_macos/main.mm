// LUMA Studio - macOS Entry Point
#import <Cocoa/Cocoa.h>
#import "AppDelegate.h"

int main(int argc, const char* argv[]) {
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        AppDelegate* delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        
        // Create menu bar
        NSMenu* menuBar = [[NSMenu alloc] init];
        
        // App menu
        NSMenuItem* appMenuItem = [[NSMenuItem alloc] init];
        NSMenu* appMenu = [[NSMenu alloc] init];
        [appMenu addItemWithTitle:@"About LUMA Studio" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
        [appMenu addItem:[NSMenuItem separatorItem]];
        [appMenu addItemWithTitle:@"Quit LUMA Studio" action:@selector(terminate:) keyEquivalent:@"q"];
        [appMenuItem setSubmenu:appMenu];
        [menuBar addItem:appMenuItem];
        
        // File menu
        NSMenuItem* fileMenuItem = [[NSMenuItem alloc] init];
        NSMenu* fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
        [fileMenu addItemWithTitle:@"Open Model..." action:nil keyEquivalent:@"o"];
        [fileMenuItem setSubmenu:fileMenu];
        [menuBar addItem:fileMenuItem];
        
        // View menu
        NSMenuItem* viewMenuItem = [[NSMenuItem alloc] init];
        NSMenu* viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
        [viewMenu addItemWithTitle:@"Toggle Grid" action:nil keyEquivalent:@"g"];
        [viewMenu addItemWithTitle:@"Reset Camera" action:nil keyEquivalent:@"f"];
        [viewMenuItem setSubmenu:viewMenu];
        [menuBar addItem:viewMenuItem];
        
        // Help menu
        NSMenuItem* helpMenuItem = [[NSMenuItem alloc] init];
        NSMenu* helpMenu = [[NSMenu alloc] initWithTitle:@"Help"];
        [helpMenu addItemWithTitle:@"Show Help" action:nil keyEquivalent:@"?"];
        [helpMenuItem setSubmenu:helpMenu];
        [menuBar addItem:helpMenuItem];
        
        [app setMainMenu:menuBar];
        
        [app activateIgnoringOtherApps:YES];
        [app run];
    }
    return 0;
}
