//
//  Z80EmuThread.h
//  Z89Emu
//
//  Created by Les Bird on 3/20/11.
//  Copyright 2011 Les Bird. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface Z80EmuThread : NSObject {
	BOOL started;
}

@property BOOL started;

-(void)start;
-(void)run;

@end
