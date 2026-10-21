#import "VF2ROMImport.h"
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include "../../src/host/romset.h"

@interface VF2ROMImport () <UIDocumentPickerDelegate>
@property(nonatomic, copy) void (^completion)(NSURL *, NSString *);
@end

@implementation VF2ROMImport

+ (NSURL *)sandboxROMURL
{
	NSURL *support = [[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory
		inDomains:NSUserDomainMask].firstObject;
	NSURL *appFolder = [support URLByAppendingPathComponent:@"Virtua Fighter 2 Recomp" isDirectory:YES];
	NSURL *romFolder = [appFolder URLByAppendingPathComponent:@"roms" isDirectory:YES];
	return [romFolder URLByAppendingPathComponent:@"vf2.zip"];
}

+ (void)presentFrom:(UIViewController *)presenter completion:(void (^)(NSURL *, NSString *))completion
{
	VF2ROMImport *delegate = [VF2ROMImport new];
	delegate.completion = completion;
	UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc]
		initForOpeningContentTypes:@[UTTypeZIP] asCopy:NO];
	picker.delegate = delegate;
	picker.modalPresentationStyle = UIModalPresentationFormSheet;
	// The presented controller retains its delegate for the selection lifetime.
	objc_setAssociatedObject(picker, @selector(presentFrom:completion:), delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	[presenter presentViewController:picker animated:YES completion:nil];
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
	NSURL *source = urls.firstObject;
	if (!source) {
		self.completion(nil, @"The selected archive could not be accessed.");
		return;
	}
	const BOOL scoped = [source startAccessingSecurityScopedResource];
	__block NSError *copyError = nil;
	__block NSURL *temporary = nil;
	NSFileCoordinator *coordinator = [NSFileCoordinator new];
	[coordinator coordinateReadingItemAtURL:source options:0 error:&copyError byAccessor:^(NSURL *readURL) {
		NSURL *destination = [VF2ROMImport sandboxROMURL];
		[[NSFileManager defaultManager] createDirectoryAtURL:[destination URLByDeletingLastPathComponent]
			withIntermediateDirectories:YES attributes:nil error:&copyError];
		if (copyError) return;
		temporary = [[destination URLByDeletingLastPathComponent] URLByAppendingPathComponent:[NSUUID.UUID.UUIDString stringByAppendingString:@".zip"]];
		[[NSFileManager defaultManager] removeItemAtURL:temporary error:nil];
		[[NSFileManager defaultManager] copyItemAtURL:readURL toURL:temporary error:&copyError];
	}];
	if (scoped) [source stopAccessingSecurityScopedResource];
	if (copyError || !temporary) {
		if (temporary) [[NSFileManager defaultManager] removeItemAtURL:temporary error:nil];
		self.completion(nil, copyError.localizedDescription ?: @"The archive could not be copied.");
		return;
	}
	romset::result checked = romset::load(temporary.path.UTF8String);
	if (!checked.ok) {
		[[NSFileManager defaultManager] removeItemAtURL:temporary error:nil];
		self.completion(nil, [NSString stringWithUTF8String:checked.error.c_str()]);
		return;
	}
	NSURL *destination = [VF2ROMImport sandboxROMURL];
	NSFileManager *manager = NSFileManager.defaultManager;
	BOOL installed;
	if ([manager fileExistsAtPath:destination.path])
		installed = [manager replaceItemAtURL:destination withItemAtURL:temporary backupItemName:nil
			options:0 resultingItemURL:nil error:&copyError];
	else installed = [manager moveItemAtURL:temporary toURL:destination error:&copyError];
	if (!installed) {
		[manager removeItemAtURL:temporary error:nil];
		self.completion(nil, copyError.localizedDescription);
		return;
	}
	self.completion(destination, nil);
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller
{
	self.completion(nil, nil);
}
@end
