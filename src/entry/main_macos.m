#ifdef BR_PLATFORM_MACOS

#import <Cocoa/Cocoa.h>

#include <stdbool.h>

#include <tinycthread.h>

#include "entry.h"
#include "log.h"

#include <bgfx/platform.h>

static struct {
	volatile bool should_close;
} s_ctx;

////////////////////////////////

@interface AppDelegate : NSObject<NSApplicationDelegate>
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
@end

@implementation AppDelegate
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
	(void)sender;
	s_ctx.should_close = true;
	return NSTerminateCancel;
}
@end

@interface WindowDelegate : NSObject<NSWindowDelegate>
- (void)windowWillClose:(NSNotification*)notification;
@end

@implementation WindowDelegate
- (void)windowWillClose:(NSNotification*)notification {
	(void)notification;
	s_ctx.should_close = true;
}
@end

NSEvent* peek_event()
{
	return [NSApp
			nextEventMatchingMask:NSEventMaskAny
			untilDate:[NSDate distantPast] // do not wait for event
			inMode:NSDefaultRunLoopMode
			dequeue:YES
			];
}

////////////////////////////////

static entry_window_info_t s_window;

const entry_window_info_t* entry_get_window() {
	return &s_window;
}

static int worker(void* arg) {
	// TODO:
	while (!s_ctx.should_close) {
		entry_tick(1000/60.0f);
	}
	
	return 0;
}

int main(int argc, const char* argv[]) {
	const uint16_t w = 600;
	const uint16_t h = 400;

	@autoreleasepool {
	    [NSApplication sharedApplication];
	    AppDelegate* ad = [[AppDelegate alloc] init];
		[NSApp setDelegate:ad];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		[NSApp activateIgnoringOtherApps:YES];
		[NSApp finishLaunching];
		
		[[NSNotificationCenter defaultCenter]
		 postNotificationName:NSApplicationWillFinishLaunchingNotification
		 object:NSApp];
		
		[[NSNotificationCenter defaultCenter]
		 postNotificationName:NSApplicationDidFinishLaunchingNotification
		 object:NSApp];
		
		id quit_menu_item = [NSMenuItem new];
		[quit_menu_item
		 initWithTitle:@"Quit"
		 action:@selector(terminate:)
		 keyEquivalent:@"q"];
		
		id app_menu = [NSMenu new];
		[app_menu addItem:quit_menu_item];
		
		id app_menu_item = [NSMenuItem new];
		[app_menu_item setSubmenu:app_menu];
		
		id menu_bar = [[NSMenu new] autorelease];
		[menu_bar addItem:app_menu_item];
		[NSApp setMainMenu:menu_bar];

		NSRect screenRect = [[NSScreen mainScreen] frame];
		const float cx = (screenRect.size.width  - (float)w)*0.5f;
		const float cy = (screenRect.size.height - (float)h)*0.5f;
		
		NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
		NSRect rect = NSMakeRect(cx, cy, w, h);

		NSWindow* window = [[NSWindow alloc]
							initWithContentRect:rect
							styleMask:style
							backing:NSBackingStoreBuffered
							defer:NO];
		
		[window setTitle:@"b4re"];
		[window makeKeyAndOrderFront:window];
		//[window setAcceptsMouseMovedEvents:YES];
		[window setBackgroundColor:[NSColor blackColor]];
		
		/*NSView * content_view = [window contentView];
		[content_view setWantsBestResolutionOpenGLSurface:YES];*/
		
		WindowDelegate* wd = [[WindowDelegate alloc] init];
		[window setDelegate:wd];

		s_window.display = NULL;
		s_window.window = window;
	    s_window.width = w;
	    s_window.height = h;

		bgfx_render_frame(-1);
		
		if (!entry_init(argc, argv))
			// TODO: cleanup.
			return 1;

		thrd_t t;
		if (thrd_create(&t, worker, NULL) != thrd_success) log_fatal("[entry] failed to create game thread.\n");
		
		while (!s_ctx.should_close) {
			bgfx_render_frame(-1);
			
			// TODO: call bgfx.
			while (peek_event()) {}
		}
		
		thrd_join(t, NULL);
		/*
	    if (!entry_init(argc, argv))
		    // TODO: cleanup.
		    return 1;

		while (!entry_tick(1000/60.0f) && !s_ctx.should_close) {
			// TODO: process events.
		}

	    entry_shutdown();
	 */
	    return 0;
	}
}

#endif
