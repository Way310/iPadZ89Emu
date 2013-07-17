//
//  H19EmuThread.m
//  Z89Emu
//
//  Created by Les Bird on 3/26/11.
//  Copyright 2011 Les Bird. All rights reserved.
//

#import "H19EmuThread.h"
#import "Z80.h"

char	H19RAM[65536];
char	H19ROM[4096];
Z80		H19CPU;

#define ROM_FILE	"2732_444-46_H19CODE.BIN"
#define ROM_SIZE	4096
#define FONT_FILE	"2716_444-29_H19FONT.BIN"
#define FONT_SIZE	2048

@implementation H19EmuThread

@synthesize started;

-(void)loadROM
{
	char romFile[512];
	
	const char *path = [[[NSBundle mainBundle] bundlePath] cStringUsingEncoding:NSASCIIStringEncoding];
	sprintf(romFile, "%s/%s", path, ROM_FILE);
	FILE *fp = fopen(romFile, "rb");
	if (fp != nil)
	{
		fread(&H19ROM[0], sizeof(byte), ROM_SIZE, fp);
		fclose(fp);
	}
}

-(void)start
{
	if (started)
	{
		return;
	}
	[self loadROM];
	
	started = YES;
	[NSThread detachNewThreadSelector:@selector(run) toTarget:self withObject:nil];
}

-(void)run
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	RunZ80(&H19CPU);
	
	started = NO;
	[pool release];
}

-(void)dealloc
{
	[super dealloc];
}

@end
