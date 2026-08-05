#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include <cadmium/render/native_surface.hpp>

extern "C" void* Cadmium_CreateMetalLayerForWindow(void* nsWindowPtr)
{
    NSWindow* window = (__bridge NSWindow*)nsWindowPtr;
    NSView* contentView = [window contentView];

    [contentView setWantsLayer:YES];

    CAMetalLayer* layer = [CAMetalLayer layer];
    [contentView setLayer:layer];

    return (__bridge void*)layer;
}
