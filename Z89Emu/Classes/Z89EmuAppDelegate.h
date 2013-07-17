//
//  Z89EmuAppDelegate.h
//  Z89Emu
//
//  Created by Les Bird on 3/5/11.
//  Copyright Les Bird 2011. All rights reserved.
//

#import <UIKit/UIKit.h>

@class RootViewController;

@interface Z89EmuAppDelegate : NSObject <UIApplicationDelegate> {
	UIWindow			*window;
	RootViewController	*viewController;
}

@property (nonatomic, retain) UIWindow *window;

@end
