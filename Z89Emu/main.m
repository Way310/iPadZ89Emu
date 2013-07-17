//
//  main.m
//  Z89Emu
//
//  Created by Les Bird on 3/5/11.
//  Copyright Les Bird 2011. All rights reserved.
//

#import <UIKit/UIKit.h>

int main(int argc, char *argv[]) {
	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	int retVal = UIApplicationMain(argc, argv, nil, @"Z89EmuAppDelegate");
	[pool release];
	return retVal;
}
