#pragma once

#import <Foundation/Foundation.h>

@class UIViewController;

// Presents the iOS document picker for a user-owned vf2.zip and copies a validated
// archive into the app sandbox. The returned URL is never an external provider URL.
@interface VF2ROMImport : NSObject
+ (NSURL *)sandboxROMURL;
+ (void)presentFrom:(UIViewController *)presenter
          completion:(void (^)(NSURL * _Nullable sandboxURL, NSString * _Nullable error))completion;
@end
