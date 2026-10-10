# src/bumpercars_b/bumpercars_b.decls - prototype overrides of the bumpercars_b module
# (docs/ARCHITECTURE.md, section 11).  <addr> <prototype>;
# thiscall/fastcall `this` is the first parameter.

# BumperCars/bcb_timer.c (gap): the timer class
00096320 void BCarTimer_Construct(struct BCarTimer *self);
00096350 int BCarTimer_Reset(struct BCarTimer *self);
000963a0 void BCarTimer_Update(struct BCarTimer *self);
000964d0 void BCarTimer_Start(struct BCarTimer *self);
00096550 int BCarGameTimer_Init(struct BCarGameTimer *self, int seconds);
000965b0 float BCarTimer_GetFrames(struct BCarTimer *self);
000965d0 void BCarTimer_Start2(struct BCarTimer *self);
000965e0 float BCarTimer_GetFraction(struct BCarTimer *self);
000965f0 float BCarTimer_GetRemaining(struct BCarTimer *self);
00096600 float BCarTimer_GetElapsed(struct BCarTimer *self);
00096610 float BCarTimer_GetFrameTime(struct BCarTimer *self);
00096620 float BCarTimer_GetAverageFrame(struct BCarTimer *self);
00096630 void BCarTimer_Restart(struct BCarTimer *self);
00096670 unsigned long long BCarTimer_GetUpdates(struct BCarTimer *self);
