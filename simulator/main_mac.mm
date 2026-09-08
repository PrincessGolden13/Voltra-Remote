#import <Cocoa/Cocoa.h>
#include "SimulatorApp.h"
#include <cmath>

@interface SimulatorWindow : NSWindow
@end
@implementation SimulatorWindow
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)canBecomeMainWindow { return YES; }
@end

@interface SimulatorView : NSView {
    voltra::SimulatorApp* app_;
    double scrollRemainder_;
    BOOL touching_;
}
- (instancetype)initWithApp:(voltra::SimulatorApp*)app;
- (void)tick:(NSTimer*)timer;
- (void)releaseTouch;
@end
@implementation SimulatorView
- (instancetype)initWithApp:(voltra::SimulatorApp*)app {
    self = [super initWithFrame:NSMakeRect(0, 0, app->hardware.width(), app->hardware.height())];
    if (self) app_ = app;
    return self;
}
- (BOOL)isFlipped { return YES; }
- (BOOL)isOpaque { return NO; }
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent*)event { return YES; }
- (void)tick:(NSTimer*)timer {
    app_->ui.tick();
    self.needsDisplay = YES;
}
- (void)drawRect:(NSRect)dirtyRect {
    [NSGraphicsContext saveGraphicsState];
    [[NSColor clearColor] setFill];
    NSRectFillUsingOperation(self.bounds, NSCompositingOperationCopy);
    [[NSBezierPath bezierPathWithOvalInRect:self.bounds] addClip];
    const auto& pixels = app_->hardware.pixels();
    CGDataProviderRef data = CGDataProviderCreateWithData(nullptr, pixels.data(), pixels.size() * 4, nullptr);
    CGColorSpaceRef colors = CGColorSpaceCreateDeviceRGB();
    CGImageRef frame = CGImageCreate(app_->hardware.width(), app_->hardware.height(), 8, 32,
        app_->hardware.width() * 4, colors,
        kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst, data, nullptr, false, kCGRenderingIntentDefault);
    NSImage* image = [[NSImage alloc] initWithCGImage:frame size:self.bounds.size];
    [image drawInRect:self.bounds fromRect:NSZeroRect operation:NSCompositingOperationCopy
        fraction:1.0 respectFlipped:YES hints:@{NSImageHintInterpolation: @(NSImageInterpolationNone)}];
    CGImageRelease(frame);
    CGColorSpaceRelease(colors);
    CGDataProviderRelease(data);
    [NSGraphicsContext restoreGraphicsState];
}
- (BOOL)insideCircle:(NSPoint)p {
    const double radius = self.bounds.size.width / 2;
    return std::hypot(p.x - radius, p.y - radius) <= radius;
}
- (void)pointer:(NSEvent*)event pressed:(BOOL)pressed {
    NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    app_->hardware.pointer(int(p.x), int(p.y), pressed);
    app_->ui.readTouch();
}
- (void)mouseDown:(NSEvent*)event {
    NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    if (![self insideCircle:p]) return;
    if (p.y < self.bounds.size.height * 0.1) {
        [self.window performWindowDragWithEvent:event];
        return;
    }
    touching_ = YES;
    [self pointer:event pressed:YES];
}
- (void)mouseDragged:(NSEvent*)event { if (touching_) [self pointer:event pressed:YES]; }
- (void)mouseUp:(NSEvent*)event {
    if (touching_) [self pointer:event pressed:NO];
    touching_ = NO;
}
- (void)releaseTouch {
    touching_ = NO;
    app_->hardware.pointer(0, 0, false);
    app_->ui.readTouch();
}
- (void)scrollWheel:(NSEvent*)event {
    // Trackpads report pixels; ten points equal one logical encoder detent.
    // Ignore momentum so lifting a finger does not keep adjusting resistance.
    if (event.momentumPhase != NSEventPhaseNone) return;
    const double delta = event.scrollingDeltaY * (event.isDirectionInvertedFromDevice ? -1 : 1);
    scrollRemainder_ += delta / (event.hasPreciseScrollingDeltas ? 10.0 : 1.0);
    const int steps = int(scrollRemainder_);
    scrollRemainder_ -= steps;
    app_->hardware.rotate(steps);
}
- (void)keyDown:(NSEvent*)event {
    NSString* characters = event.charactersIgnoringModifiers;
    if (characters.length == 0) return;
    const unichar key = [characters characterAtIndex:0];
    if (key == 27) { [self.window close]; return; }
    if (key == NSLeftArrowFunctionKey) app_->hardware.rotate(-1);
    if (key == NSRightArrowFunctionKey) app_->hardware.rotate(1);
    if ((key == '\r' || key == ' ') && !event.isARepeat) app_->hardware.press();
    if (key >= NSF1FunctionKey && key <= NSF11FunctionKey)
        app_->functionKey(int(key - NSF1FunctionKey + 1), event.isARepeat);
}
@end

@interface SimulatorDelegate : NSObject <NSWindowDelegate>
@property(strong) NSTimer* timer;
@property(strong) SimulatorView* view;
@end
@implementation SimulatorDelegate
- (void)windowDidResignKey:(NSNotification*)notification { [self.view releaseTouch]; }
- (void)windowWillClose:(NSNotification*)notification {
    [self.timer invalidate];
    [self.view releaseTouch];
    [NSApp stop:nil];
    // Wake the run loop so stop takes effect even when no further input arrives.
    [NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSZeroPoint
        modifierFlags:0 timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0] atStart:NO];
}
@end

int main(int argc, char** argv) {
    @autoreleasepool {
        try {
            const auto options = voltra::SimulatorOptions::parse(argc, argv);
            std::filesystem::path memoryPath;
            if (options.renderPath.empty()) {
                NSURL* support = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory
                    inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:nullptr];
                if (!support) throw std::runtime_error("Could not locate Application Support");
                memoryPath = std::filesystem::path(support.fileSystemRepresentation) / "VOLTRA Remote";
                std::filesystem::create_directories(memoryPath);
                memoryPath /= "recent-voltras.txt";
            }
            voltra::SimulatorApp app(options, memoryPath);
            if (app.render(options)) return 0;

            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            NSMenu* menu = [[NSMenu alloc] init];
            NSMenuItem* root = [[NSMenuItem alloc] init];
            NSMenu* applicationMenu = [[NSMenu alloc] initWithTitle:@"VOLTRA Simulator"];
            [applicationMenu addItemWithTitle:@"Quit VOLTRA Simulator" action:@selector(terminate:) keyEquivalent:@"q"];
            root.submenu = applicationMenu;
            [menu addItem:root];
            NSApp.mainMenu = menu;

            SimulatorWindow* window = [[SimulatorWindow alloc]
                initWithContentRect:NSMakeRect(0, 0, options.size, options.size)
                styleMask:NSWindowStyleMaskBorderless backing:NSBackingStoreBuffered defer:NO];
            window.title = @"VOLTRA Simulator";
            window.opaque = NO;
            window.backgroundColor = NSColor.clearColor;
            window.hasShadow = YES;
            window.releasedWhenClosed = NO;
            SimulatorView* view = [[SimulatorView alloc] initWithApp:&app];
            SimulatorDelegate* delegate = [[SimulatorDelegate alloc] init];
            delegate.view = view;
            window.delegate = delegate;
            window.contentView = view;
            [window center];
            [window makeKeyAndOrderFront:nil];
            [window makeFirstResponder:view];
            [NSApp activateIgnoringOtherApps:YES];
            delegate.timer = [NSTimer timerWithTimeInterval:0.016 target:view selector:@selector(tick:)
                userInfo:nil repeats:YES];
            [[NSRunLoop mainRunLoop] addTimer:delegate.timer forMode:NSRunLoopCommonModes];
            [NSApp run];
            [delegate.timer invalidate];
            window.delegate = nil;
            return 0;
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 2;
        }
    }
}
