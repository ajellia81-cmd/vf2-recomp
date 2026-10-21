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
	std::string result;
	bool retry = true;
	while (retry) {
		__block bool done = false;
		__block bool again = false;
		__block std::string selected;
		[VF2ROMImport presentFrom:presenter completion:^(NSURL *url, NSString *error) {
			if (url) selected = url.path.UTF8String;
			if (error) SDL_SetError("%s", error.UTF8String);
			[presenter dismissViewControllerAnimated:NO completion:^{
				if (!error) { done = true; return; }
				UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Unable to import archive"
					message:error preferredStyle:UIAlertControllerStyleAlert];
				[alert addAction:[UIAlertAction actionWithTitle:@"Choose another file" style:UIAlertActionStyleDefault
					handler:^(UIAlertAction *) { again = true; [presenter dismissViewControllerAnimated:NO completion:^{ done = true; }]; }]];
				[alert addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel
					handler:^(UIAlertAction *) { [presenter dismissViewControllerAnimated:NO completion:^{ done = true; }]; }]];
				[presenter presentViewController:alert animated:YES completion:nil];
			}];
		}];
		// SDL's iOS event pump services UIKit on this main thread, including document providers.
		// Keep the window and block state alive until import/error handling is complete.
		while (!done) {
			SDL_PumpEvents();
			SDL_Delay(10);
		}
		result = selected;
		retry = again;
	}

	SDL_DestroyWindow(window);
	return result;
}
