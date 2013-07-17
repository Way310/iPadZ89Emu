//
//  HelloWorldLayer.h
//  Z89Emu
//
//  Created by Les Bird on 3/5/11.
//  Copyright Les Bird 2011. All rights reserved.
//


// When you import this file, you import all the cocos2d classes
#import "cocos2d.h"
#import "Z80EmuThread.h"
#import "Z80.h"

// HelloWorld Layer
@interface HelloWorld : CCLayer <UITableViewDelegate, UITableViewDataSource>
{
	Z80EmuThread *emuThread;
//	H19EmuThread *H19Thread;
}

@property (nonatomic,retain) Z80EmuThread *emuThread;
//@property (nonatomic,retain) H19EmuThread *H19Thread;

// returns a Scene that contains the HelloWorld as the only child
+(id)scene;
-(void)loadROM;
-(void)loadDisk:(int)drive :(NSString *)disk_file;
-(void)loadLibrary;
-(void)loadQuickKeys;
-(void)insertDisk:(int)drive :(NSString *)fileName;
-(UIButton *)initButton:(int)x :(int)y :(int)width :(int)height :(int)tag :(NSString *)label;
-(void)initTableView;
+(void)keyboardPut:(byte)c;
+(byte)keyboardGet;
-(void)H19Init;
-(void)H19Cls;
-(void)H19SetCharAt:(int)x :(int)y :(byte)c;
-(void)H19SetChar:(byte)c;
-(void)H19PrintAt:(int)x :(int)y :(NSString *)s;
-(void)H19KeyHilite:(int)i;
-(byte)H19KeyPress:(CGPoint)p;
-(BOOL)H17IsHDOSDisk:(int)drive;
-(NSString *)H17GetHDOSLabel:(int)drive;
-(void)H17SetDiskParameters:(int)drive;
-(int)H17GetImageOffset:(int)drive :(int)side :(int)track :(int)sector;
-(void)H17WriteNextByte:(byte)c;
-(void)H17ReadByte:(byte)c;
-(void)H17ReadNextByte;
-(void)H17Service;
-(void)H17Reset;
-(void)startCPU;
-(void)speedToggle:(id)sender;

@end
