#import <UIKit/UIKit.h>
#import "VF2ROMImport.h"
#include "../../src/host/ios_platform.h"
#include <SDL3/SDL.h>

std::string ios_data_directory()
{
	NSURL *url = [[VF2ROMImport sandboxROMURL] URLByDeletingLastPathComponent];
	return url.URLByDeletingLastPathComponent.path.UTF8String;
}

std::string ios_import_rom()
{
	if (!SDL_Init(SDL_INIT_VIDEO)) return {};
	SDL_Window *window = SDL_CreateWindow("Import your VF2 archive", 960, 540, SDL_WINDOW_FULLSCREEN);
	if (!window) return {};
	UIWindow *native = (__bridge UIWindow *)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
		SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);
	UIViewController *presenter = native.rootViewController;
	if (!presenter) {
		SDL_SetError("SDL's UIKit presenter is unavailable");
		SDL_DestroyWindow(window);
		return {};
	}
	__block bool done = false;
	__block std::string path;
	[VF2ROMImport presentFrom:presenter completion:^(NSURL *url, NSString *error) {
		if (url) path = url.path.UTF8String;
		if (error) SDL_SetError("%s", error.UTF8String);
		[presenter dismissViewControllerAnimated:NO completion:^{ done = true; }];
	}];
	// SDL's iOS event pump services UIKit on this main thread, including document providers.
	// Do not return while the picker completion still references this stack frame.
	while (!done) {
		SDL_PumpEvents();
		SDL_Delay(10);
	}
	SDL_DestroyWindow(window);
	return path;
}
