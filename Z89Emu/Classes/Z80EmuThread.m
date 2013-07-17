//
//  Z80EmuThread.m
//  Z89Emu
//
//  Created by Les Bird on 3/20/11.
//  Copyright 2011 Les Bird. All rights reserved.
//

#import "Z80EmuThread.h"
#import "Z80.h"

extern Z80 Z80CPU;

@implementation Z80EmuThread
@synthesize started;

-(void)start
{
	if (started)
	{
		return;
	}
	started = YES;
	[NSThread detachNewThreadSelector:@selector(run) toTarget:self withObject:nil];
}

-(void)run
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	RunZ80(&Z80CPU);
	
	started = NO;
	[pool release];
}

-(void)dealloc
{
	[super dealloc];
}

@end
