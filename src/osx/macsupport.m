#import "macsupport.h"
#import <Cocoa/Cocoa.h>

/* Portions of CPS.h */
typedef struct CPSProcessSerNum
{
	UInt32          lo;
	UInt32          hi;
} CPSProcessSerNum;

extern OSErr    CPSGetCurrentProcess( CPSProcessSerNum *psn);
extern OSErr    CPSEnableForegroundOperation( CPSProcessSerNum *psn, UInt32 
											 _arg2, UInt32 _arg3, UInt32 _arg4, UInt32 _arg5);
extern OSErr    CPSSetFrontProcess( CPSProcessSerNum *psn);

void OSXMain()
{
	NSAutoreleasePool  *pool = [[NSAutoreleasePool alloc] init];
	[ NSApplication sharedApplication ];
	[ NSApp setMainMenu:[[NSMenu alloc] init] ];
    
 	{
		CPSProcessSerNum PSN;
		/* Tell the dock about us */
		if (!CPSGetCurrentProcess(&PSN))
			if (!CPSEnableForegroundOperation(&PSN,0x03,0x3C,0x2C,0x1103))
				if (!CPSSetFrontProcess(&PSN))
					[NSApplication sharedApplication];
	}
}

const char* initResourcesLocation() {
    CFURLRef url;
    static UInt8 path[4096];
 
    // Get the URL to the application’s PlugIns directory.
    url = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
 
    CFURLGetFileSystemRepresentation( url, true, path, 4096 );
 
    // Release the CF objects when done with them.
    CFRelease( url );
    return path;
}

const char* initSaveLocation() {
	NSAutoreleasePool  *pool = [[NSAutoreleasePool alloc] init];
	[ NSApplication sharedApplication ];
    NSFileManager *manager = [NSFileManager defaultManager];
    NSError *err;
    
    // Look up the full path to the user's Application Support folder (usually ~/Library/Application Support/).
    NSString *basePath = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) objectAtIndex: 0];
    
    // Use a folder under Application Support named after the application.
    NSString *appName = [[NSBundle mainBundle] objectForInfoDictionaryKey: @"CFBundleName"];
    NSString *supportPath = [basePath stringByAppendingPathComponent: appName];
    
    // Create our folder the first time it is needed.
    if (![manager fileExistsAtPath: supportPath]) {
        [manager createDirectoryAtPath:supportPath withIntermediateDirectories:YES attributes:nil error:&err];
    }
    
    return [supportPath UTF8String];
}
