//
//  H19EmuThread.h
//  Z89Emu
//
//  Created by Les Bird on 3/26/11.
//  Copyright 2011 Les Bird. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface H19EmuThread : NSObject {
	BOOL started;
}

@property BOOL started;

-(void)start;
-(void)run;

@end
