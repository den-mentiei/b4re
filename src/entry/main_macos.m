#ifdef BR_PLATFORM_MACOS

#import <Cocoa/Cocoa.h>

#include <stdbool.h>

#include "entry.h"
#include "log.h"

#include <bgfx/platform.h>

static struct {
	float mouse_x;
	float mouse_y;
	bool  mouse_left;
	bool  mouse_right;

	bool should_close;
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

static NSEvent* peek_event()
{
	return [NSApp
			nextEventMatchingMask:NSEventMaskAny
			untilDate:[NSDate distantPast] // do not wait for event
			inMode:NSDefaultRunLoopMode
			dequeue:YES
			];
}

static void get_mouse_position(float* _x, float *_y) {
	NSWindow* window         = [NSApp keyWindow];
	NSRect    original_frame = [window frame];
	NSPoint   location       = [window mouseLocationOutsideOfEventStream];
	NSRect    adjust_frame   = [window contentRectForFrameRect: original_frame];

	int x = location.x;
	int y = (int)adjust_frame.size.height - (int)location.y;

	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x > (int)adjust_frame.size.width) x = (int)adjust_frame.size.width;
	if (y > (int)adjust_frame.size.height) y = (int)adjust_frame.size.height;

	*_x = x;
	*_y = y;
}

static void on_event(NSEvent* e) {
	if (!e) return;

	NSEventType type = [e type];
	switch (type) {
	case NSEventTypeMouseMoved:
	case NSEventTypeLeftMouseDragged:
	case NSEventTypeRightMouseDragged:
	case NSEventTypeOtherMouseDragged:
		get_mouse_position(&s_ctx.mouse_x, &s_ctx.mouse_y);
		break;

	case NSEventTypeLeftMouseDown:
		s_ctx.mouse_left = true;
		break;
	case NSEventTypeLeftMouseUp:
		s_ctx.mouse_left = false;
		break;

	case NSEventTypeRightMouseDown:
		s_ctx.mouse_right = true;
		break;
	case NSEventTypeRightMouseUp:
		s_ctx.mouse_right = false;
		break;

	default:
		break;
	}
}

////////////////////////////////

static entry_window_info_t s_window;

const entry_window_info_t* entry_get_window() {
	return &s_window;
}

bool entry_mouse_pressed(entry_button_t b) {
	if (b == ENTRY_BUTTON_LEFT) {
		return s_ctx.mouse_left;
	}
	if (b == ENTRY_BUTTON_RIGHT) {
		return s_ctx.mouse_right;
	}
	return false;
}

void entry_mouse_position(float* x, float* y) {
	*x = s_ctx.mouse_x;
	*y = s_ctx.mouse_y;
}

int main(int argc, const char* argv[]) {
	const uint16_t w = ENTRY_WINDOW_WIDTH;
	const uint16_t h = ENTRY_WINDOW_HEIGHT;

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
		[window setAcceptsMouseMovedEvents:YES];
		[window setMovableByWindowBackground:YES];
		
		/*NSView * content_view = [window contentView];
		[content_view setWantsBestResolutionOpenGLSurface:YES];*/
		
		WindowDelegate* wd = [[WindowDelegate alloc] init];
		[window setDelegate:wd];

		s_window.display = NULL;
		s_window.window = window;
		s_window.width = w;
		s_window.height = h;
	
		// Forces bgfx to operate in single-threaded mode. TODO: proper threading.
		bgfx_render_frame(-1);
		
		if (!entry_init(argc, argv))
			// TODO: cleanup.
			return 1;

		while (!s_ctx.should_close) {
			entry_tick(1000 / 60.0f);

			NSEvent* e;
			while ((e = peek_event())) {
				on_event(e);
				[NSApp sendEvent:e];
				[NSApp updateWindows];
			}
		}

	    entry_shutdown();
	    return 0;
	}
}

#endif
