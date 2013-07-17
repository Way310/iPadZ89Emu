//
//  KeyInputView.m
//  Z89Emu
//
//  Created by Les Bird on 8/1/11.
//  Copyright 2011 Les Bird. All rights reserved.
//

#import "KeyInputView.h"
#import "HelloWorldScene.h"

extern BOOL bOffline;
extern HelloWorld *HelloWorldPtr;

@implementation KeyInputView

-(id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
	{
		inputView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 1, 1)];
    }
    return self;
}

-(void)dealloc
{
    [super dealloc];
}

-(BOOL)canBecomeFirstResponder
{
    return YES; 
}

-(UIView*)inputView
{
    return inputView;
}

-(BOOL)hasText
{
    return NO;
}

-(void)insertText:(NSString *)text
{
    char ch = [text characterAtIndex:0];

	if (ch == 0x0A)
	{
		ch = 0x0D;
	}
	
	NSLog(@"keyboard char: %d", ch);
	
	if (bOffline)
	{
		[HelloWorldPtr H19SetChar:ch];
	}
	else
	{
		[HelloWorld keyboardPut:ch];
	}
	
    static int cycleResponder = 0;
    if (++cycleResponder > 20)
	{
        // necessary to clear a buffer that accumulates internally
        cycleResponder = 0;
        [self resignFirstResponder];
        [self becomeFirstResponder];
    }
}

-(void)deleteBackward
{
	NSLog(@"keyboard char: 0x08");

	if (bOffline)
	{
		[HelloWorldPtr H19SetChar:0x08];
	}
	else
	{
		[HelloWorld keyboardPut:0x08];
	}
}

@end
