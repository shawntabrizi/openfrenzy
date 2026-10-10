/*
STATIC_INIT.C  <-  LIB:crt_init (0x001f29f0-0x001f4e20) and LIB:crt_exit (0x001f4e20-0x001f5330)

openfrenzy core module: the 81 static constructors (the .CRT$XC table at 0x00281750, run in
that order by the generated of_run_static_constructors()) and the exit functions they
register with atexit. Most construct a global object of another module and register its
destructor. Generated from the disassembly (core agent script), then checked by hand.

The objects belong to other modules. They are tentative definitions here (merged with the
owner's by -fcommon), sized to cover what the constructor writes. Objects at 0x002ba4f0 and
0x002bae5c are in the core block (core_data.h). The 19 exit functions that store a
base-class vtable into an initialised object are stubs (see each one).

Note: the original never runs the exit functions (main never returns on the Xbox). In the
port they run when of_boot.c returns from main().
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core_globals.h"
#include "core_extern.h"
#include "core_internal.h"

/* ---------- foreign constructors and destructors (thiscall: this first) */

#define ext_Ctor_00025920 ((void *(*)(void *))FUN_00025920)  /* core_modes_b class 0x00025920 */
#define ext_Ctor_00080200 ((void *(*)(void *))FUN_00080200)  /* classlib_ui class 0x00080200 */
#define ext_Ctor_000872b0 ((void *(*)(void *))FUN_000872b0)  /* bumpercars_a class 0x000872b0 */
#define ext_Ctor_0010f9c0 ((void *(*)(void *))FUN_0010f9c0)  /* drillers class 0x0010f9c0 */
#define ext_Ctor_00112d40 ((void *(*)(void *))FUN_00112d40)  /* drillers class 0x00112d40 */
#define ext_Ctor_00133f80 ((void *(*)(void *))FUN_00133f80)  /* foxhunt class 0x00133f80 (does nothing) */
#define ext_Ctor_0013eae0 ((void *(*)(void *))FUN_0013eae0)  /* hopper class 0x0013eae0 */
#define ext_Ctor_0015ca00 ((void *(*)(void *))FUN_0015ca00)  /* meta_a class 0x0015ca00 */
#define ext_Ctor_00170ea0 ((void *(*)(void *))FUN_00170ea0)  /* mousewhack class 0x00170ea0 (does nothing) */
#define ext_Ctor_001c1880 ((void *(*)(void *))FUN_001c1880)  /* tankedup_a class 0x001c1880 */
#define ext_Ctor_001d6cb0 ((void *(*)(void *))FUN_001d6cb0)  /* tankedup_b class 0x001d6cb0 */
#define ext_Nothing_Thunk_001608d0 ((void (*)(void *))FUN_001608d0)
#define ext_Dtor_00080220 ((void (*)(void *))FUN_00080220)
#define ext_Dtor_00086ba0 ((void (*)(void *))FUN_00086ba0)
#define ext_Dtor_000259a0 ((void (*)(void *))FUN_000259a0)
#define ext_Dtor_0010f9e0 ((void (*)(void *))FUN_0010f9e0)
#define ext_Dtor_001116b0 ((void (*)(void *))FUN_001116b0)
#define ext_Dtor_0013eb00 ((void (*)(void *))FUN_0013eb00)
#define ext_Dtor_0015c8c0 ((void (*)(void *))FUN_0015c8c0)
#define ext_Dtor_00170eb0 ((void (*)(void *))FUN_00170eb0)
#define ext_Dtor_001d6cd0 ((void (*)(void *))FUN_001d6cd0)
#define ext_Dtor_001c1990 ((void (*)(void *))FUN_001c1990)
#define ext_Ctor_001608b0 ((void *(*)(void *))FUN_001608b0)  /* meta_b class, 0x20 bytes */

/* ---------- objects of other modules (tentative definitions) */

unsigned char DAT_002ed324[0x14];  /* 0x002ed324 */
unsigned char DAT_002ed418[0xc];  /* 0x002ed418 */
unsigned char DAT_002ed424[0xc];  /* 0x002ed424 */
unsigned char DAT_002ed430[0xc];  /* 0x002ed430 */
unsigned char DAT_002ed43c[0xc];  /* 0x002ed43c */
unsigned char DAT_002ed458[0x32d0];  /* 0x002ed458 */
unsigned char DAT_002f27c8[0xc];  /* 0x002f27c8 */
unsigned char DAT_002f6ba0[0x48];  /* 0x002f6ba0 */
unsigned char DAT_002f7a5c[0x18];  /* 0x002f7a5c */
unsigned char DAT_002ff9d4[0xc];  /* 0x002ff9d4 */
unsigned char DAT_002ff9f0[0x8];  /* 0x002ff9f0 */
unsigned char DAT_00300c78[0xfb0];  /* 0x00300c78 */
unsigned char DAT_003021ac[0xc];  /* 0x003021ac */
unsigned char DAT_003021b8[0x48];  /* 0x003021b8 */
unsigned char DAT_003022bc[0xc];  /* 0x003022bc */
unsigned char DAT_003022cc[0x4];  /* 0x003022cc */
unsigned char DAT_00302300[0xc];  /* 0x00302300 */
unsigned char DAT_0030230c[0xc];  /* 0x0030230c */
unsigned char DAT_00302318[0xc];  /* 0x00302318 */
unsigned char DAT_00302324[0xc];  /* 0x00302324 */
unsigned char DAT_00302330[0x10];  /* 0x00302330 */
unsigned char DAT_003024c8[0x48];  /* 0x003024c8 */
unsigned char DAT_00302798[0x4];  /* 0x00302798 */
unsigned char DAT_003055e8[0x48];  /* 0x003055e8 */
unsigned char DAT_00305630[0xc];  /* 0x00305630 */
unsigned char DAT_0030563c[0xc];  /* 0x0030563c */
unsigned char DAT_003075b0[0xc];  /* 0x003075b0 */
unsigned char DAT_003082d4[0xc];  /* 0x003082d4 */
unsigned char DAT_00308440[0xc];  /* 0x00308440 */
unsigned char DAT_0030844c[0xc];  /* 0x0030844c */
unsigned char DAT_00308460[0x88];  /* 0x00308460 */
unsigned char DAT_00308e60[0xc];  /* 0x00308e60 */
unsigned char DAT_00308e6c[0xc];  /* 0x00308e6c */
unsigned char DAT_00308f08[0xc];  /* 0x00308f08 */
unsigned char DAT_00308f1c[0xc];  /* 0x00308f1c */
unsigned char DAT_00308f2c[0xc];  /* 0x00308f2c */
unsigned char DAT_00308f3c[0xc];  /* 0x00308f3c */
unsigned char DAT_00308f48[0x10];  /* 0x00308f48 */
unsigned char DAT_003090a8[0xc];  /* 0x003090a8 */
unsigned char DAT_003090d8[0xc];  /* 0x003090d8 */
unsigned char DAT_00309100[0x58];  /* 0x00309100 */
unsigned char DAT_00309268[0x10];  /* 0x00309268 */
unsigned char DAT_0030a09c[0xc];  /* 0x0030a09c */
unsigned char DAT_0030a0e4[0xc];  /* 0x0030a0e4 */
unsigned char DAT_0030a0f0[0x48];  /* 0x0030a0f0 */
unsigned char DAT_0030a1d0[0x90];  /* 0x0030a1d0 */
unsigned char DAT_0030ea20[0x10];  /* 0x0030ea20 */
unsigned char DAT_0030ed00[0xc];  /* 0x0030ed00 */
unsigned char DAT_0030ee30[0x10];  /* 0x0030ee30 */
unsigned char DAT_0030fea4[0xc];  /* 0x0030fea4 */
unsigned char DAT_00312f50[0xa34];  /* 0x00312f50 */
unsigned char DAT_003150b0[0x158];  /* 0x003150b0 */
unsigned char DAT_00308f58[8 * 0x20];  /* 0x00308f58: 8 objects of 0x20 bytes (meta_b 0x001608b0) */
uint32_t DAT_002aa994[8];  /* 0x002aa994 */
uint32_t DAT_002aec24[8];  /* 0x002aec24 */
uint32_t DAT_002cce60[10];  /* 0x002cce60: bkernel resource type tags */

/* ====================================================================== exit functions */

/* 0x001f4e10  LIB:crt_exit (missing from the function table): destroy the object at 0x002ba4f0 */
void FUN_001f4e10(void)
{
	ext_Dtor_000259a0(CORE_ADDR(0x002ba4f0));
}

/* 0x001f4e20  LIB:crt_exit: ~CGameModeStack of the stack at 0x002ba750 (pop everything) */
void FUN_001f4e20(void)
{
	while (g_ModeStack.count != 0)
		CGameModeStack_Pop(&g_ModeStack, 0);
	g_ModeStack.first_drawn = 0;
}

/* 0x001f4e50  LIB:crt_exit */
void FUN_001f4e50(void)
{
	ext_Nothing(CORE_ADDR(0x002bae5c));
}

/* 0x001f4e60  LIB:crt_exit */
void FUN_001f4e60(void)
{
	/* empty */
}

/* 0x001f4e70  LIB:crt_exit */
void FUN_001f4e70(void)
{
	ext_Dtor_00080220(DAT_002ed324);
}

/* 0x001f4e80  LIB:crt_exit */
void FUN_001f4e80(void)
{
	ext_Nothing(DAT_002ed418);
}

/* 0x001f4e90  LIB:crt_exit */
void FUN_001f4e90(void)
{
	ext_Nothing(DAT_002ed430);
}

/* 0x001f4ea0  LIB:crt_exit */
void FUN_001f4ea0(void)
{
	ext_Nothing(DAT_002ed43c);
}

/* 0x001f4eb0  LIB:crt_exit */
void FUN_001f4eb0(void)
{
	ext_Nothing(DAT_002ed424);
}

/* 0x001f4ec0  LIB:crt_exit */
void FUN_001f4ec0(void)
{
	/* empty */
}

/* 0x001f4ed0  LIB:crt_exit */
void FUN_001f4ed0(void)
{
	ext_Dtor_00086ba0(DAT_002ed458);
}

/* 0x001f4ee0  LIB:crt_exit */
void FUN_001f4ee0(void)
{
	ext_Nothing(DAT_002f27c8);
}

/* 0x001f4ef0  LIB:crt_exit */
void FUN_001f4ef0(void)
{
	/* STUB: the original stores vtable 0x00267468 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x00293ad8. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f4f00  LIB:crt_exit */
void FUN_001f4f00(void)
{
	ext_Dtor_000259a0(DAT_002f6ba0);
}

/* 0x001f4f10  LIB:crt_exit */
void FUN_001f4f10(void)
{
	ext_Nothing(DAT_002f7a5c);
}

/* 0x001f4f20  LIB:crt_exit */
void FUN_001f4f20(void)
{
	/* STUB: the original stores vtable 0x00269648 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x00296ee4. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f4f30  LIB:crt_exit */
void FUN_001f4f30(void)
{
	ext_Nothing(DAT_002ff9d4);
}

/* 0x001f4f40  LIB:crt_exit */
void FUN_001f4f40(void)
{
	ext_Dtor_0010f9e0(DAT_002ff9f0);
}

/* 0x001f4f50  LIB:crt_exit */
void FUN_001f4f50(void)
{
	ext_Dtor_001116b0(DAT_00300c78);
}

/* 0x001f4f60  LIB:crt_exit */
void FUN_001f4f60(void)
{
	/* STUB: the original stores vtable 0x0026c294 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x00298da0. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f4f70  LIB:crt_exit */
void FUN_001f4f70(void)
{
	/* STUB: the original stores vtable 0x0026c2ec (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x00298e70. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f4f80  LIB:crt_exit */
void FUN_001f4f80(void)
{
	/* STUB: the original stores vtable 0x0026c310 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002a54d8. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f4f90  LIB:crt_exit */
void FUN_001f4f90(void)
{
	/* STUB: the original stores vtable 0x0026c568 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002aa830. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f4fa0  LIB:crt_exit */
void FUN_001f4fa0(void)
{
	/* STUB: the original stores vtable 0x0026c5a0 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002aa900. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f4fb0  LIB:crt_exit */
void FUN_001f4fb0(void)
{
	/* STUB: the original stores vtable 0x0026cb20 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002aaaf8. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f4fc0  LIB:crt_exit */
void FUN_001f4fc0(void)
{
	/* STUB: the original stores vtable 0x0026cb7c (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002aab08. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f4fd0  LIB:crt_exit */
void FUN_001f4fd0(void)
{
	/* STUB: the original stores vtable 0x0026cbac (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002aadf8. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f4fe0  LIB:crt_exit */
void FUN_001f4fe0(void)
{
	/* STUB: the original stores vtable 0x0026cc44 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002aae60. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f4ff0  LIB:crt_exit */
void FUN_001f4ff0(void)
{
	ext_Nothing(DAT_003021ac);
}

/* 0x001f5000  LIB:crt_exit */
void FUN_001f5000(void)
{
	ext_Dtor_000259a0(DAT_003021b8);
}

/* 0x001f5010  LIB:crt_exit */
void FUN_001f5010(void)
{
	ext_Nothing(DAT_003022bc);
}

/* 0x001f5020  LIB:crt_exit */
void FUN_001f5020(void)
{
	ext_Nothing_Thunk_001608d0(DAT_003022cc);
}

/* 0x001f5030  LIB:crt_exit */
void FUN_001f5030(void)
{
	ext_Nothing(DAT_00302300);
}

/* 0x001f5040  LIB:crt_exit */
void FUN_001f5040(void)
{
	ext_Nothing(DAT_0030230c);
}

/* 0x001f5050  LIB:crt_exit */
void FUN_001f5050(void)
{
	ext_Nothing(DAT_00302318);
}

/* 0x001f5060  LIB:crt_exit */
void FUN_001f5060(void)
{
	ext_Nothing(DAT_00302324);
}

/* 0x001f5070  LIB:crt_exit */
void FUN_001f5070(void)
{
	ext_Nothing(DAT_00302330);
}

/* 0x001f5080  LIB:crt_exit */
void FUN_001f5080(void)
{
	/* STUB: the original stores vtable 0x0026e330 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002ae878. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f5090  LIB:crt_exit */
void FUN_001f5090(void)
{
	ext_Dtor_000259a0(DAT_003024c8);
}

/* 0x001f50a0  LIB:crt_exit */
void FUN_001f50a0(void)
{
	ext_Dtor_0013eb00(DAT_00302798);
}

/* 0x001f50b0  LIB:crt_exit */
void FUN_001f50b0(void)
{
	/* STUB: the original stores vtable 0x0026e510 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002ae948. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f50c0  LIB:crt_exit */
void FUN_001f50c0(void)
{
	/* STUB: the original stores vtable 0x0026edb8 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002aebd8. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f50d0  LIB:crt_exit */
void FUN_001f50d0(void)
{
	/* STUB: the original stores vtable 0x0026edd4 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002aec10. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f50e0  LIB:crt_exit */
void FUN_001f50e0(void)
{
	/* STUB: the original stores vtable 0x0026eeb4 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002afe80. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f50f0  LIB:crt_exit */
void FUN_001f50f0(void)
{
	/* STUB: the original stores vtable 0x0026eeb8 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002affa0. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f5100  LIB:crt_exit */
void FUN_001f5100(void)
{
	/* STUB: the original stores vtable 0x0026f038 (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002b0038. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f5110  LIB:crt_exit */
void FUN_001f5110(void)
{
	/* STUB: the original stores vtable 0x0026f05c (a base-class vtable, inlined base
	   destructor) into the first word of the initialised object at 0x002b0378. Not done:
	   the owner of that class does not export the vtable symbol yet. Runs only at exit. */
}

/* 0x001f5120  LIB:crt_exit */
void FUN_001f5120(void)
{
	ext_Nothing(DAT_00305630);
}

/* 0x001f5130  LIB:crt_exit */
void FUN_001f5130(void)
{
	ext_Nothing(DAT_0030563c);
}

/* 0x001f5140  LIB:crt_exit */
void FUN_001f5140(void)
{
	ext_Dtor_000259a0(DAT_003055e8);
}

/* 0x001f5150  LIB:crt_exit */
void FUN_001f5150(void)
{
	ext_Nothing(DAT_003075b0);
}

/* 0x001f5160  LIB:crt_exit */
void FUN_001f5160(void)
{
	ext_Nothing(DAT_003082d4);
}

/* 0x001f5170  LIB:crt_exit */
void FUN_001f5170(void)
{
	ext_Nothing(DAT_00308440);
}

/* 0x001f5180  LIB:crt_exit */
void FUN_001f5180(void)
{
	ext_Nothing(DAT_0030844c);
}

/* 0x001f5190  LIB:crt_exit */
void FUN_001f5190(void)
{
	ext_Dtor_0015c8c0(DAT_00308460);
}

/* 0x001f51a0  LIB:crt_exit */
void FUN_001f51a0(void)
{
	ext_Nothing(DAT_00308e60);
}

/* 0x001f51b0  LIB:crt_exit */
void FUN_001f51b0(void)
{
	ext_Nothing(DAT_00308e6c);
}

/* 0x001f51c0  LIB:crt_exit */
void FUN_001f51c0(void)
{
	ext_Nothing(DAT_00308f08);
}

/* 0x001f51d0  LIB:crt_exit */
void FUN_001f51d0(void)
{
	ext_Nothing(DAT_00308f1c);
}

/* 0x001f51e0  LIB:crt_exit */
void FUN_001f51e0(void)
{
	ext_Nothing(DAT_00308f2c);
}

/* 0x001f51f0  LIB:crt_exit */
void FUN_001f51f0(void)
{
	ext_Nothing(DAT_00308f48);
}

/* 0x001f5200  LIB:crt_exit */
void FUN_001f5200(void)
{
	ext_Nothing(DAT_00308f3c);
}

/* 0x001f5210  LIB:crt_exit: destroy the 8 objects at 0x00308f58, last first */
void FUN_001f5210(void)
{
	int i;

	for (i = 8 - 1; i >= 0; i--)
		ext_Nothing_Thunk_001608d0(DAT_00308f58 + i * 0x20);
}

/* 0x001f5230  LIB:crt_exit */
void FUN_001f5230(void)
{
	ext_Nothing(DAT_003090a8);
}

/* 0x001f5240  LIB:crt_exit */
void FUN_001f5240(void)
{
	ext_Nothing(DAT_003090d8);
}

/* 0x001f5250  LIB:crt_exit */
void FUN_001f5250(void)
{
	ext_Nothing(DAT_00309100);
}

/* 0x001f5260  LIB:crt_exit */
void FUN_001f5260(void)
{
	ext_Nothing(DAT_00309268);
}

/* 0x001f5270  LIB:crt_exit */
void FUN_001f5270(void)
{
	ext_Nothing(DAT_0030a09c);
}

/* 0x001f5280  LIB:crt_exit */
void FUN_001f5280(void)
{
	ext_Nothing(DAT_0030a0e4);
}

/* 0x001f5290  LIB:crt_exit */
void FUN_001f5290(void)
{
	ext_Dtor_000259a0(DAT_0030a0f0);
}

/* 0x001f52a0  LIB:crt_exit */
void FUN_001f52a0(void)
{
	ext_Nothing(DAT_0030ea20);
}

/* 0x001f52b0  LIB:crt_exit */
void FUN_001f52b0(void)
{
	ext_Dtor_00170eb0(DAT_0030a1d0);
}

/* 0x001f52c0  LIB:crt_exit */
void FUN_001f52c0(void)
{
	/* empty */
}

/* 0x001f52d0  LIB:crt_exit */
void FUN_001f52d0(void)
{
	ext_Nothing(DAT_0030ed00);
}

/* 0x001f52e0  LIB:crt_exit */
void FUN_001f52e0(void)
{
	ext_Nothing(DAT_0030ee30);
}

/* 0x001f52f0  LIB:crt_exit */
void FUN_001f52f0(void)
{
	/* empty */
}

/* 0x001f5300  LIB:crt_exit */
void FUN_001f5300(void)
{
	ext_Nothing(DAT_0030fea4);
}

/* 0x001f5310  LIB:crt_exit */
void FUN_001f5310(void)
{
	ext_Dtor_001d6cd0(DAT_003150b0);
}

/* 0x001f5320  LIB:crt_exit */
void FUN_001f5320(void)
{
	ext_Dtor_001c1990(DAT_00312f50);
}

/* ====================================================================== constructors */

/* 0x001f29f0  LIB:crt_init (.CRT$XC #0) */
void FUN_001f29f0(void)
{
	ext_Ctor_00025920(CORE_ADDR(0x002ba4f0));
	atexit(FUN_001f4e10);
}

/* 0x001f2a10  LIB:crt_init (.CRT$XC #1) */
void FUN_001f2a10(void)
{
	atexit(FUN_001f4e20);
}

/* 0x001f2a20  LIB:crt_init (.CRT$XC #2) */
void FUN_001f2a20(void)
{
	ext_ListInit(CORE_ADDR(0x002bae5c));
	atexit(FUN_001f4e50);
}

/* 0x001f2a40  LIB:crt_init (.CRT$XC #4) */
void FUN_001f2a40(void)
{
	/* the resource type tags, copied from .rdata 0x0025b9e0 down to 0x0025b998 (step 8) */
	DAT_002cce60[0] = 0x52584554;  /* 'TEXR' */
	DAT_002cce60[1] = 0x52544341;  /* 'ACTR' */
	DAT_002cce60[2] = 0x504d4153;  /* 'SAMP' */
	DAT_002cce60[3] = 0x544e4f46;  /* 'FONT' */
	DAT_002cce60[4] = 0x42415453;  /* 'STAB' */
	DAT_002cce60[5] = 0x414c5053;  /* 'SPLA' */
	DAT_002cce60[6] = 0x20544553;  /* 'SET ' */
	DAT_002cce60[7] = 0x53454d43;  /* 'CMES' */
	DAT_002cce60[8] = 0x4c4f4341;  /* 'ACOL' */
	DAT_002cce60[9] = 0x52545341;  /* 'ASTR' */
}

/* 0x001f2ac0  LIB:crt_init (.CRT$XC #5) */
void FUN_001f2ac0(void)
{
	atexit(FUN_001f4e60);
}

/* 0x001f2ad0  LIB:crt_init (.CRT$XC #6) */
void FUN_001f2ad0(void)
{
	ext_Ctor_00080200(DAT_002ed324);
	atexit(FUN_001f4e70);
}

/* 0x001f2af0  LIB:crt_init (.CRT$XC #7) */
void FUN_001f2af0(void)
{
	ext_ListInit(DAT_002ed418);
	atexit(FUN_001f4e80);
}

/* 0x001f2b10  LIB:crt_init (.CRT$XC #8) */
void FUN_001f2b10(void)
{
	ext_ListInit(DAT_002ed430);
	atexit(FUN_001f4e90);
}

/* 0x001f2b30  LIB:crt_init (.CRT$XC #9) */
void FUN_001f2b30(void)
{
	ext_ListInit(DAT_002ed43c);
	atexit(FUN_001f4ea0);
}

/* 0x001f2b50  LIB:crt_init (.CRT$XC #10) */
void FUN_001f2b50(void)
{
	ext_ListInit(DAT_002ed424);
	atexit(FUN_001f4eb0);
}

/* 0x001f2b70  LIB:crt_init (.CRT$XC #11) */
void FUN_001f2b70(void)
{
	atexit(FUN_001f4ec0);
}

/* 0x001f2b80  LIB:crt_init (.CRT$XC #12) */
void FUN_001f2b80(void)
{
	ext_Ctor_000872b0(DAT_002ed458);
	atexit(FUN_001f4ed0);
}

/* 0x001f2ba0  LIB:crt_init (.CRT$XC #13) */
void FUN_001f2ba0(void)
{
	ext_ListInit(DAT_002f27c8);
	atexit(FUN_001f4ee0);
}

/* 0x001f2bc0  LIB:crt_init (.CRT$XC #14) */
void FUN_001f2bc0(void)
{
	atexit(FUN_001f4ef0);
}

/* 0x001f2bd0  LIB:crt_init (.CRT$XC #15) */
void FUN_001f2bd0(void)
{
	ext_Ctor_00025920(DAT_002f6ba0);
	atexit(FUN_001f4f00);
}

/* 0x001f2bf0  LIB:crt_init (.CRT$XC #16) */
void FUN_001f2bf0(void)
{
	ext_ListInit(DAT_002f7a5c);
	atexit(FUN_001f4f10);
}

/* 0x001f2c10  LIB:crt_init (.CRT$XC #17) */
void FUN_001f2c10(void)
{
	atexit(FUN_001f4f20);
}

/* 0x001f2c20  LIB:crt_init (.CRT$XC #18) */
void FUN_001f2c20(void)
{
	ext_ListInit(DAT_002ff9d4);
	atexit(FUN_001f4f30);
}

/* 0x001f2c40  LIB:crt_init (.CRT$XC #19) */
void FUN_001f2c40(void)
{
	ext_Ctor_0010f9c0(DAT_002ff9f0);
	atexit(FUN_001f4f40);
}

/* 0x001f2c60  LIB:crt_init (.CRT$XC #20) */
void FUN_001f2c60(void)
{
	ext_Ctor_00112d40(DAT_00300c78);
	atexit(FUN_001f4f50);
}

/* 0x001f2c80  LIB:crt_init (.CRT$XC #21) */
void FUN_001f2c80(void)
{
	atexit(FUN_001f4f60);
}

/* 0x001f2c90  LIB:crt_init (.CRT$XC #22) */
void FUN_001f2c90(void)
{
	atexit(FUN_001f4f70);
}

/* 0x001f2ca0  LIB:crt_init (.CRT$XC #23) */
void FUN_001f2ca0(void)
{
	atexit(FUN_001f4f80);
}

/* 0x001f2cb0  LIB:crt_init (.CRT$XC #24) */
void FUN_001f2cb0(void)
{
	atexit(FUN_001f4f90);
}

/* 0x001f2cc0  LIB:crt_init (.CRT$XC #25) */
void FUN_001f2cc0(void)
{
	memset(DAT_002aa994, 0, sizeof(DAT_002aa994));
	atexit(FUN_001f4fa0);
}

/* 0x001f2ce0  LIB:crt_init (.CRT$XC #26) */
void FUN_001f2ce0(void)
{
	atexit(FUN_001f4fb0);
}

/* 0x001f2cf0  LIB:crt_init (.CRT$XC #27) */
void FUN_001f2cf0(void)
{
	atexit(FUN_001f4fc0);
}

/* 0x001f2d00  LIB:crt_init (.CRT$XC #28) */
void FUN_001f2d00(void)
{
	atexit(FUN_001f4fd0);
}

/* 0x001f2d10  LIB:crt_init (.CRT$XC #29) */
void FUN_001f2d10(void)
{
	atexit(FUN_001f4fe0);
}

/* 0x001f2d20  LIB:crt_init (.CRT$XC #30) */
void FUN_001f2d20(void)
{
	ext_ListInit(DAT_003021ac);
	atexit(FUN_001f4ff0);
}

/* 0x001f2d40  LIB:crt_init (.CRT$XC #31) */
void FUN_001f2d40(void)
{
	ext_Ctor_00025920(DAT_003021b8);
	atexit(FUN_001f5000);
}

/* 0x001f2d60  LIB:crt_init (.CRT$XC #32) */
void FUN_001f2d60(void)
{
	ext_ListInit(DAT_003022bc);
	atexit(FUN_001f5010);
}

/* 0x001f2d80  LIB:crt_init (.CRT$XC #33) */
void FUN_001f2d80(void)
{
	ext_Ctor_00133f80(DAT_003022cc);
	atexit(FUN_001f5020);
}

/* 0x001f2da0  LIB:crt_init (.CRT$XC #34) */
void FUN_001f2da0(void)
{
	/* tweak variables: values from the code; the x87 products are computed in double */
#define TWEAK_I32(a) CORE_VAR(int32_t, a)
#define TWEAK_U32(a) CORE_VAR(uint32_t, a)
#define TWEAK_F32(a) CORE_VAR(float, a)
	TWEAK_U32(0x002ad120) = *(const uint32_t *)&DAT_00292c84;
	TWEAK_F32(0x002ad19c) = (float)((double)DAT_00292c84 * 0.2f);  /* * [0x00257c7c] */
	TWEAK_I32(0x002ad138) = 90;
	TWEAK_I32(0x002ad13c) = 90;
	TWEAK_I32(0x002ad140) = 90;
	TWEAK_I32(0x002ad148) = 90;
	TWEAK_I32(0x002ad14c) = 90;
	TWEAK_I32(0x002ad150) = 90;
	TWEAK_I32(0x002ad158) = 90;
	TWEAK_I32(0x002ad15c) = 90;
	TWEAK_I32(0x002ad160) = 90;
	TWEAK_I32(0x002ad124) = 0;
	TWEAK_I32(0x002ad128) = 40;
	TWEAK_I32(0x002ad12c) = 80;
	TWEAK_I32(0x002ad130) = 50;
	TWEAK_I32(0x002ad134) = 0;
	TWEAK_I32(0x002ad144) = 0;
	TWEAK_I32(0x002ad154) = 0;
	TWEAK_I32(0x002ad164) = 0;
	TWEAK_F32(0x002ad168) = 0.1f;
	TWEAK_F32(0x002ad16c) = 0.5f;
	TWEAK_F32(0x002ad170) = 0.1f;
	TWEAK_F32(0x002ad174) = 0.3f;
	TWEAK_F32(0x002ad178) = 0.1f;
	TWEAK_F32(0x002ad17c) = 0.4f;
	TWEAK_F32(0x002ad180) = 0.4f;
	TWEAK_F32(0x002ad184) = 0.1f;
	TWEAK_I32(0x002ad188) = 0;
	TWEAK_I32(0x002ad18c) = 0;
	TWEAK_F32(0x002ad190) = 5.0f;
	TWEAK_F32(0x002ad194) = 0.9f;
	TWEAK_F32(0x002ad198) = 0.4f;
	TWEAK_I32(0x002ad1a0) = 40;
	TWEAK_I32(0x002ad1a4) = 80;
	TWEAK_I32(0x002ad1a8) = 50;
	TWEAK_I32(0x002ad1ac) = 20;
	TWEAK_I32(0x002ad1b0) = 127;
	TWEAK_I32(0x002ad1b4) = 100;
	TWEAK_I32(0x002ad1b8) = 80;
	TWEAK_I32(0x002ad1bc) = 30;
	TWEAK_I32(0x002ad1c0) = 50;
	TWEAK_I32(0x002ad1c4) = 40;
	TWEAK_I32(0x002ad1c8) = 10;
	TWEAK_I32(0x002ad1cc) = 0;
	TWEAK_I32(0x002ad1d0) = 50;
	TWEAK_I32(0x002ad1d4) = 40;
	TWEAK_I32(0x002ad1d8) = 10;
	TWEAK_I32(0x002ad1dc) = 0;
	TWEAK_I32(0x002ad1e0) = 0;
	TWEAK_F32(0x002ad1e4) = 0.3f;
	TWEAK_I32(0x002ad1e8) = 0;
	TWEAK_I32(0x002ad1ec) = 0;
	TWEAK_I32(0x002ad1f0) = 0;
	TWEAK_F32(0x002ad1f4) = 0.1f;
	TWEAK_F32(0x002ad1f8) = 2.5f;
	TWEAK_I32(0x002ad1fc) = 0;
	TWEAK_I32(0x002ad200) = 0;
	TWEAK_I32(0x002ad204) = 0;
	TWEAK_I32(0x002ad208) = 0;
	TWEAK_F32(0x002ad20c) = 0.99f;
	TWEAK_F32(0x002ad210) = 2.0f;
	TWEAK_I32(0x002ad214) = 0;
	TWEAK_I32(0x002ad218) = 0;
	TWEAK_I32(0x002ad21c) = 110;
	TWEAK_I32(0x002ad220) = 80;
	TWEAK_I32(0x002ad224) = 0;
	TWEAK_I32(0x002ad228) = 0;
	TWEAK_I32(0x002ad22c) = 127;
	TWEAK_I32(0x002ad230) = 127;
	TWEAK_I32(0x002ad234) = 0;
	TWEAK_I32(0x002ad238) = 0;
	TWEAK_I32(0x002ad23c) = 127;
	TWEAK_I32(0x002ad240) = 127;
	TWEAK_I32(0x002ad244) = 0;
	TWEAK_I32(0x002ad248) = 0;
	TWEAK_I32(0x002ad24c) = 127;
	TWEAK_I32(0x002ad250) = 127;
	TWEAK_I32(0x002ad254) = 0;
	TWEAK_I32(0x002ad258) = 0;
	TWEAK_F32(0x002ad25c) = 0.3f;
	TWEAK_I32(0x002ad260) = 0;
	TWEAK_I32(0x002ad264) = 0;
	TWEAK_F32(0x002ad268) = 0.5f;
	TWEAK_F32(0x002ad26c) = 0.7f;
	TWEAK_F32(0x002ad270) = 0.8f;
	TWEAK_F32(0x002ad274) = 0.8f;
	TWEAK_I32(0x002ad278) = 0;
	TWEAK_I32(0x002ad27c) = 0;
	TWEAK_I32(0x002ad280) = 0;
	TWEAK_F32(0x002ad284) = 0.99f;
	TWEAK_F32(0x002ad288) = 2.0f;
	TWEAK_I32(0x002ad28c) = 0;
	TWEAK_I32(0x002ad290) = 0;
	TWEAK_I32(0x002ad294) = 127;
	TWEAK_I32(0x002ad298) = 60;
	TWEAK_I32(0x002ad29c) = 0;
	TWEAK_I32(0x002ad2a0) = 125;
	TWEAK_I32(0x002ad2a4) = 90;
	TWEAK_I32(0x002ad2a8) = 60;
	TWEAK_I32(0x002ad2ac) = 0;
	TWEAK_I32(0x002ad2b0) = 125;
	TWEAK_I32(0x002ad2b4) = 90;
	TWEAK_I32(0x002ad2b8) = 90;
	TWEAK_I32(0x002ad2bc) = 0;
	TWEAK_I32(0x002ad2c0) = 125;
	TWEAK_I32(0x002ad2c4) = 126;
	TWEAK_I32(0x002ad2c8) = 127;
	TWEAK_I32(0x002ad2cc) = 0;
	TWEAK_I32(0x002ad2d0) = 0;
	TWEAK_F32(0x002ad2d4) = 1.0f;
	TWEAK_I32(0x002ad2d8) = 0;
	TWEAK_I32(0x002ad2dc) = 0;
	TWEAK_I32(0x002ad2e0) = 0;
	TWEAK_F32(0x002ad2e4) = 0.7f;
	TWEAK_F32(0x002ad2e8) = 0.7f;
	TWEAK_I32(0x002ad2ec) = 0;
	TWEAK_I32(0x002ad2f0) = 0;
	TWEAK_I32(0x002ad2f4) = 0;
	TWEAK_I32(0x002ad2f8) = 0;
	TWEAK_I32(0x002ad2fc) = 0;
	TWEAK_I32(0x002ad300) = 0;
	TWEAK_I32(0x002ad304) = 0;
	TWEAK_I32(0x002ad308) = 0;
	TWEAK_I32(0x002ad30c) = 0;
	TWEAK_I32(0x002ad310) = 0;
	TWEAK_I32(0x002ad314) = 0;
	TWEAK_I32(0x002ad318) = 10;
	TWEAK_I32(0x002ad31c) = 10;
	TWEAK_I32(0x002ad320) = 10;
	TWEAK_I32(0x002ad324) = 1;
	TWEAK_I32(0x002ad328) = 10;
	TWEAK_I32(0x002ad32c) = 10;
	TWEAK_I32(0x002ad330) = 10;
	TWEAK_I32(0x002ad334) = 10;
	TWEAK_I32(0x002ad338) = 10;
	TWEAK_I32(0x002ad33c) = 10;
	TWEAK_I32(0x002ad340) = 10;
	TWEAK_I32(0x002ad344) = 10;
	TWEAK_I32(0x002ad348) = 0;
	TWEAK_F32(0x002ad34c) = 2e+03f;
	TWEAK_I32(0x002ad350) = 0;
	TWEAK_F32(0x002ad354) = 0.1f;
	TWEAK_F32(0x002ad358) = 1.0f;
	TWEAK_F32(0x002ad35c) = 1.0f;
	TWEAK_F32(0x002ad360) = 1.0f;
	TWEAK_F32(0x002ad364) = 1.0f;
	TWEAK_I32(0x002ad368) = 0;
	TWEAK_I32(0x002ad36c) = 0;
	TWEAK_F32(0x002ad370) = 1e+01f;
	TWEAK_F32(0x002ad374) = 1.0f;
	TWEAK_I32(0x002ad378) = 0;
	TWEAK_I32(0x002ad37c) = 0;
	TWEAK_I32(0x002ad380) = 70;
	TWEAK_I32(0x002ad384) = 70;
	TWEAK_I32(0x002ad388) = 70;
	TWEAK_I32(0x002ad38c) = 10;
	TWEAK_I32(0x002ad390) = 80;
	TWEAK_I32(0x002ad394) = 60;
	TWEAK_I32(0x002ad398) = 50;
	TWEAK_I32(0x002ad39c) = 20;
	TWEAK_I32(0x002ad3a0) = 80;
	TWEAK_I32(0x002ad3a4) = 60;
	TWEAK_I32(0x002ad3a8) = 50;
	TWEAK_I32(0x002ad3ac) = 20;
	TWEAK_I32(0x002ad3b0) = 80;
	TWEAK_I32(0x002ad3b4) = 60;
	TWEAK_I32(0x002ad3b8) = 50;
	TWEAK_I32(0x002ad3bc) = 20;
	TWEAK_F32(0x002ad3c0) = 3.5f;
	TWEAK_F32(0x002ad3c4) = 6.5f;
	TWEAK_F32(0x002ad3c8) = 5.5f;
	TWEAK_F32(0x002ad3cc) = 0.1f;
	TWEAK_F32(0x002ad3d0) = 0.2f;
	TWEAK_F32(0x002ad3d4) = 0.3f;
	TWEAK_F32(0x002ad3d8) = 0.4f;
	TWEAK_F32(0x002ad3dc) = 0.6f;
	TWEAK_I32(0x002ad3e0) = 0;
	TWEAK_I32(0x002ad3e4) = 0;
	TWEAK_F32(0x002ad3e8) = 1e+01f;
	TWEAK_F32(0x002ad3ec) = 1.0f;
	TWEAK_F32(0x002ad3f0) = 1.0f;
	TWEAK_I32(0x002ad3f4) = 0;
	TWEAK_I32(0x002ad3f8) = 40;
	TWEAK_I32(0x002ad3fc) = 80;
	TWEAK_I32(0x002ad400) = 50;
	TWEAK_I32(0x002ad404) = 20;
	TWEAK_I32(0x002ad408) = 127;
	TWEAK_I32(0x002ad40c) = 100;
	TWEAK_I32(0x002ad410) = 80;
	TWEAK_I32(0x002ad414) = 30;
	TWEAK_I32(0x002ad418) = 50;
	TWEAK_I32(0x002ad41c) = 40;
	TWEAK_I32(0x002ad420) = 10;
	TWEAK_I32(0x002ad424) = 0;
	TWEAK_I32(0x002ad428) = 50;
	TWEAK_I32(0x002ad42c) = 40;
	TWEAK_I32(0x002ad430) = 10;
	TWEAK_I32(0x002ad434) = 0;
	TWEAK_I32(0x002ad438) = 0;
	TWEAK_F32(0x002ad43c) = 0.35f;
	TWEAK_I32(0x002ad440) = 0;
	TWEAK_I32(0x002ad444) = 0;
	TWEAK_I32(0x002ad448) = 0;
	TWEAK_F32(0x002ad44c) = 0.1f;
	TWEAK_F32(0x002ad450) = 4.5f;
	TWEAK_I32(0x002ad454) = 0;
	TWEAK_I32(0x002ad458) = 0;
	TWEAK_I32(0x002ad45c) = 0;
	TWEAK_I32(0x002ad460) = 0;
	TWEAK_F32(0x002ad464) = 0.99f;
	TWEAK_F32(0x002ad468) = 2.0f;
	TWEAK_I32(0x002ad46c) = 0;
	TWEAK_I32(0x002ad470) = 40;
	TWEAK_I32(0x002ad474) = 80;
	TWEAK_I32(0x002ad478) = 50;
	TWEAK_I32(0x002ad47c) = 20;
	TWEAK_I32(0x002ad480) = 127;
	TWEAK_I32(0x002ad484) = 100;
	TWEAK_I32(0x002ad488) = 80;
	TWEAK_I32(0x002ad48c) = 30;
	TWEAK_I32(0x002ad490) = 50;
	TWEAK_I32(0x002ad494) = 40;
	TWEAK_I32(0x002ad498) = 10;
	TWEAK_I32(0x002ad49c) = 0;
	TWEAK_I32(0x002ad4a0) = 50;
	TWEAK_I32(0x002ad4a4) = 40;
	TWEAK_I32(0x002ad4a8) = 10;
	TWEAK_I32(0x002ad4ac) = 0;
	TWEAK_I32(0x002ad4b0) = 0;
	TWEAK_F32(0x002ad4b4) = 0.4f;
	TWEAK_I32(0x002ad4b8) = 0;
	TWEAK_I32(0x002ad4bc) = 0;
	TWEAK_I32(0x002ad4c0) = 0;
	TWEAK_F32(0x002ad4c4) = 0.1f;
	TWEAK_F32(0x002ad4c8) = 7.5f;
	TWEAK_I32(0x002ad4cc) = 0;
	TWEAK_I32(0x002ad4d0) = 0;
	TWEAK_I32(0x002ad4d4) = 0;
	TWEAK_I32(0x002ad4d8) = 0;
	TWEAK_F32(0x002ad4dc) = 0.99f;
	TWEAK_F32(0x002ad4e0) = 2.0f;
	TWEAK_I32(0x002ad4e4) = 0;
	TWEAK_I32(0x002ad4e8) = 40;
	TWEAK_I32(0x002ad4ec) = 80;
	TWEAK_I32(0x002ad4f0) = 50;
	TWEAK_I32(0x002ad4f4) = 20;
	TWEAK_I32(0x002ad4f8) = 127;
	TWEAK_I32(0x002ad4fc) = 100;
	TWEAK_I32(0x002ad500) = 80;
	TWEAK_I32(0x002ad504) = 30;
	TWEAK_I32(0x002ad508) = 50;
	TWEAK_I32(0x002ad50c) = 40;
	TWEAK_I32(0x002ad510) = 10;
	TWEAK_I32(0x002ad514) = 0;
	TWEAK_I32(0x002ad518) = 50;
	TWEAK_I32(0x002ad51c) = 40;
	TWEAK_I32(0x002ad520) = 10;
	TWEAK_I32(0x002ad524) = 0;
	TWEAK_I32(0x002ad528) = 0;
	TWEAK_F32(0x002ad52c) = 0.3f;
	TWEAK_I32(0x002ad530) = 0;
	TWEAK_I32(0x002ad534) = 0;
	TWEAK_I32(0x002ad538) = 0;
	TWEAK_F32(0x002ad53c) = 0.1f;
	TWEAK_F32(0x002ad540) = 1.5f;
	TWEAK_F32(0x002ad544) = 0.3f;
	TWEAK_I32(0x002ad548) = 0;
	TWEAK_I32(0x002ad54c) = 0;
	TWEAK_I32(0x002ad550) = 0;
	TWEAK_F32(0x002ad554) = 0.99f;
	TWEAK_F32(0x002ad558) = 2.0f;
	TWEAK_I32(0x002ad55c) = 0;
	TWEAK_I32(0x002ad560) = 127;
	TWEAK_I32(0x002ad564) = 127;
	TWEAK_I32(0x002ad568) = 90;
	TWEAK_I32(0x002ad56c) = 0;
	TWEAK_I32(0x002ad570) = 127;
	TWEAK_I32(0x002ad574) = 127;
	TWEAK_I32(0x002ad578) = 90;
	TWEAK_I32(0x002ad57c) = 0;
	TWEAK_I32(0x002ad580) = 127;
	TWEAK_I32(0x002ad584) = 127;
	TWEAK_I32(0x002ad588) = 90;
	TWEAK_I32(0x002ad58c) = 0;
	TWEAK_I32(0x002ad590) = 127;
	TWEAK_I32(0x002ad594) = 125;
	TWEAK_I32(0x002ad598) = 90;
	TWEAK_I32(0x002ad59c) = 0;
	TWEAK_F32(0x002ad5a0) = 0.2f;
	TWEAK_F32(0x002ad5a4) = 0.5f;
	TWEAK_F32(0x002ad5a8) = 0.2f;
	TWEAK_F32(0x002ad5ac) = 0.4f;
	TWEAK_F32(0x002ad5b0) = 0.8f;
	TWEAK_F32(0x002ad5b4) = 0.8f;
	TWEAK_F32(0x002ad5b8) = 0.8f;
	TWEAK_F32(0x002ad5bc) = 0.8f;
	TWEAK_I32(0x002ad5c0) = 0;
	TWEAK_I32(0x002ad5c4) = 0;
	TWEAK_F32(0x002ad5c8) = 5e+01f;
	TWEAK_F32(0x002ad5cc) = 0.99f;
	TWEAK_F32(0x002ad5d0) = 0.4f;
	TWEAK_F32(0x002ad5d4) = 1.0f;
	TWEAK_I32(0x002ad5d8) = 40;
	TWEAK_I32(0x002ad5dc) = 80;
	TWEAK_I32(0x002ad5e0) = 50;
	TWEAK_I32(0x002ad5e4) = 20;
	TWEAK_I32(0x002ad5e8) = 127;
	TWEAK_I32(0x002ad5ec) = 100;
	TWEAK_I32(0x002ad5f0) = 80;
	TWEAK_I32(0x002ad5f4) = 30;
	TWEAK_I32(0x002ad5f8) = 50;
	TWEAK_I32(0x002ad5fc) = 40;
	TWEAK_F32(0x002ad6c4) = (float)((double)DAT_00292c84 * 0.05f);  /* * [0x00258b54] */
	TWEAK_I32(0x002ad600) = 10;
	TWEAK_I32(0x002ad604) = 0;
	TWEAK_I32(0x002ad608) = 50;
	TWEAK_I32(0x002ad60c) = 40;
	TWEAK_I32(0x002ad610) = 10;
	TWEAK_I32(0x002ad614) = 0;
	TWEAK_F32(0x002ad618) = 0.5f;
	TWEAK_F32(0x002ad61c) = 1.5f;
	TWEAK_F32(0x002ad620) = 0.5f;
	TWEAK_F32(0x002ad624) = 0.5f;
	TWEAK_F32(0x002ad628) = 0.5f;
	TWEAK_F32(0x002ad62c) = 1.5f;
	TWEAK_F32(0x002ad630) = 1.0f;
	TWEAK_F32(0x002ad634) = 1.2f;
	TWEAK_I32(0x002ad638) = 0;
	TWEAK_I32(0x002ad63c) = 0;
	TWEAK_F32(0x002ad640) = 2e+01f;
	TWEAK_F32(0x002ad644) = 0.99f;
	TWEAK_I32(0x002ad648) = 0;
	TWEAK_I32(0x002ad64c) = 0;
	TWEAK_I32(0x002ad650) = 80;
	TWEAK_I32(0x002ad654) = 100;
	TWEAK_I32(0x002ad658) = 50;
	TWEAK_I32(0x002ad65c) = 10;
	TWEAK_I32(0x002ad660) = 127;
	TWEAK_I32(0x002ad664) = 100;
	TWEAK_I32(0x002ad668) = 80;
	TWEAK_I32(0x002ad66c) = 30;
	TWEAK_I32(0x002ad670) = 50;
	TWEAK_I32(0x002ad674) = 40;
	TWEAK_I32(0x002ad678) = 10;
	TWEAK_I32(0x002ad67c) = 0;
	TWEAK_I32(0x002ad680) = 50;
	TWEAK_I32(0x002ad684) = 40;
	TWEAK_I32(0x002ad688) = 10;
	TWEAK_I32(0x002ad68c) = 0;
	TWEAK_F32(0x002ad690) = 0.1f;
	TWEAK_F32(0x002ad694) = 1.0f;
	TWEAK_F32(0x002ad698) = 0.4f;
	TWEAK_F32(0x002ad69c) = 2.5f;
	TWEAK_F32(0x002ad6a0) = 0.2f;
	TWEAK_F32(0x002ad6a4) = 1.5f;
	TWEAK_F32(0x002ad6a8) = 3.0f;
	TWEAK_F32(0x002ad6ac) = 0.2f;
	TWEAK_F32(0x002ad6b0) = 0.1f;
	TWEAK_F32(0x002ad6b4) = 0.05f;
	TWEAK_F32(0x002ad6b8) = 3e+01f;
	TWEAK_F32(0x002ad6bc) = 0.9f;
	TWEAK_F32(0x002ad6c0) = 0.4f;
	TWEAK_I32(0x002ad6c8) = 40;
	TWEAK_I32(0x002ad6cc) = 80;
	TWEAK_I32(0x002ad6d0) = 50;
	TWEAK_I32(0x002ad6d4) = 20;
	TWEAK_I32(0x002ad6d8) = 127;
	TWEAK_I32(0x002ad6dc) = 100;
	TWEAK_I32(0x002ad6e0) = 80;
	TWEAK_I32(0x002ad6e4) = 30;
	TWEAK_I32(0x002ad6e8) = 50;
	TWEAK_I32(0x002ad6ec) = 40;
	TWEAK_I32(0x002ad6f0) = 10;
	TWEAK_I32(0x002ad6f4) = 0;
	TWEAK_I32(0x002ad6f8) = 50;
	TWEAK_I32(0x002ad6fc) = 40;
	TWEAK_I32(0x002ad700) = 10;
	TWEAK_I32(0x002ad704) = 0;
	TWEAK_F32(0x002ad708) = 0.2f;
	TWEAK_F32(0x002ad70c) = 1.0f;
	TWEAK_F32(0x002ad710) = 0.2f;
	TWEAK_F32(0x002ad714) = 0.1f;
	TWEAK_F32(0x002ad718) = 0.2f;
	TWEAK_F32(0x002ad71c) = 0.6f;
	TWEAK_F32(0x002ad720) = 0.6f;
	TWEAK_F32(0x002ad724) = 0.2f;
	TWEAK_I32(0x002ad728) = 0;
	TWEAK_I32(0x002ad72c) = 0;
	TWEAK_I32(0x002ad730) = 0;
	TWEAK_F32(0x002ad734) = 0.9f;
	TWEAK_F32(0x002ad738) = 0.4f;
	TWEAK_F32(0x002ad82c) = (float)((double)DAT_00292c84 * 0.4f);  /* * [0x00262dbc] */
	TWEAK_I32(0x002ad73c) = 0;
	TWEAK_I32(0x002ad740) = 127;
	TWEAK_I32(0x002ad744) = 127;
	TWEAK_I32(0x002ad748) = 90;
	TWEAK_I32(0x002ad74c) = 0;
	TWEAK_I32(0x002ad750) = 127;
	TWEAK_I32(0x002ad754) = 127;
	TWEAK_I32(0x002ad758) = 90;
	TWEAK_I32(0x002ad75c) = 0;
	TWEAK_I32(0x002ad760) = 127;
	TWEAK_I32(0x002ad764) = 127;
	TWEAK_I32(0x002ad768) = 90;
	TWEAK_I32(0x002ad76c) = 0;
	TWEAK_I32(0x002ad770) = 127;
	TWEAK_I32(0x002ad774) = 125;
	TWEAK_I32(0x002ad778) = 90;
	TWEAK_I32(0x002ad77c) = 0;
	TWEAK_F32(0x002ad780) = 0.2f;
	TWEAK_F32(0x002ad784) = 0.5f;
	TWEAK_F32(0x002ad788) = 0.2f;
	TWEAK_F32(0x002ad78c) = 0.4f;
	TWEAK_F32(0x002ad790) = 1.0f;
	TWEAK_F32(0x002ad794) = 1.0f;
	TWEAK_F32(0x002ad798) = 1.0f;
	TWEAK_F32(0x002ad79c) = 1.0f;
	TWEAK_I32(0x002ad7a0) = 0;
	TWEAK_I32(0x002ad7a4) = 0;
	TWEAK_F32(0x002ad7a8) = 5e+01f;
	TWEAK_F32(0x002ad7ac) = 0.99f;
	TWEAK_F32(0x002ad7b0) = 0.4f;
	TWEAK_F32(0x002ad7b4) = 1.0f;
	TWEAK_I32(0x002ad7b8) = 127;
	TWEAK_I32(0x002ad7bc) = 127;
	TWEAK_I32(0x002ad7c0) = 127;
	TWEAK_I32(0x002ad7c4) = 10;
	TWEAK_I32(0x002ad7c8) = 100;
	TWEAK_I32(0x002ad7cc) = 100;
	TWEAK_I32(0x002ad7d0) = 100;
	TWEAK_I32(0x002ad7d4) = 100;
	TWEAK_I32(0x002ad7d8) = 100;
	TWEAK_I32(0x002ad7dc) = 100;
	TWEAK_I32(0x002ad7e0) = 100;
	TWEAK_I32(0x002ad7e4) = 100;
	TWEAK_I32(0x002ad7e8) = 100;
	TWEAK_I32(0x002ad7ec) = 100;
	TWEAK_I32(0x002ad7f0) = 100;
	TWEAK_I32(0x002ad7f4) = 100;
	TWEAK_F32(0x002ad7f8) = 0.8f;
	TWEAK_F32(0x002ad7fc) = 2.5f;
	TWEAK_F32(0x002ad800) = 0.6f;
	TWEAK_F32(0x002ad804) = 3.5f;
	TWEAK_F32(0x002ad808) = 0.9f;
	TWEAK_F32(0x002ad80c) = 0.9f;
	TWEAK_F32(0x002ad810) = 0.9f;
	TWEAK_F32(0x002ad814) = 0.9f;
	TWEAK_F32(0x002ad818) = 0.1f;
	TWEAK_F32(0x002ad81c) = 0.05f;
	TWEAK_F32(0x002ad820) = 3e+01f;
	TWEAK_F32(0x002ad824) = 1.0f;
	TWEAK_F32(0x002ad828) = 0.1f;
	TWEAK_I32(0x002ad830) = 127;
	TWEAK_I32(0x002ad834) = 100;
	TWEAK_I32(0x002ad838) = 70;
	TWEAK_I32(0x002ad83c) = 10;
	TWEAK_I32(0x002ad840) = 127;
	TWEAK_I32(0x002ad844) = 100;
	TWEAK_I32(0x002ad848) = 60;
	TWEAK_I32(0x002ad84c) = 10;
	TWEAK_I32(0x002ad850) = 127;
	TWEAK_I32(0x002ad854) = 100;
	TWEAK_I32(0x002ad858) = 60;
	TWEAK_I32(0x002ad85c) = 10;
	TWEAK_I32(0x002ad860) = 100;
	TWEAK_I32(0x002ad864) = 100;
	TWEAK_I32(0x002ad868) = 60;
	TWEAK_I32(0x002ad86c) = 10;
	TWEAK_F32(0x002ad870) = 0.5f;
	TWEAK_F32(0x002ad874) = 8.5f;
	TWEAK_F32(0x002ad8a4) = (float)((double)DAT_00292c84 * 0.1f);  /* * [0x002536cc] */
	TWEAK_F32(0x002ad878) = 0.5f;
	TWEAK_F32(0x002ad91c) = (float)((double)DAT_00292c84 * 0.1f);  /* * [0x002536cc] */
	TWEAK_F32(0x002ad87c) = 3.5f;
	TWEAK_F32(0x002ad994) = (float)((double)DAT_00292c84 * 0.1f);  /* * [0x002536cc] */
	TWEAK_F32(0x002ad880) = 0.2f;
	TWEAK_F32(0x002ad884) = 0.9f;
	TWEAK_F32(0x002ad888) = 0.9f;
	TWEAK_F32(0x002ad88c) = 0.2f;
	TWEAK_F32(0x002ad890) = 0.1f;
	TWEAK_F32(0x002ad894) = 0.05f;
	TWEAK_F32(0x002ad898) = 5.0f;
	TWEAK_F32(0x002ad89c) = 1.0f;
	TWEAK_F32(0x002ad8a0) = 0.4f;
	TWEAK_I32(0x002ad8a8) = 127;
	TWEAK_I32(0x002ad8ac) = 100;
	TWEAK_I32(0x002ad8b0) = 70;
	TWEAK_I32(0x002ad8b4) = 10;
	TWEAK_I32(0x002ad8b8) = 127;
	TWEAK_I32(0x002ad8bc) = 100;
	TWEAK_I32(0x002ad8c0) = 60;
	TWEAK_I32(0x002ad8c4) = 10;
	TWEAK_I32(0x002ad8c8) = 127;
	TWEAK_I32(0x002ad8cc) = 100;
	TWEAK_I32(0x002ad8d0) = 60;
	TWEAK_I32(0x002ad8d4) = 10;
	TWEAK_I32(0x002ad8d8) = 127;
	TWEAK_I32(0x002ad8dc) = 100;
	TWEAK_I32(0x002ad8e0) = 60;
	TWEAK_I32(0x002ad8e4) = 10;
	TWEAK_F32(0x002ad8e8) = 0.5f;
	TWEAK_F32(0x002ad8ec) = 1.0f;
	TWEAK_F32(0x002ad8f0) = 1.0f;
	TWEAK_I32(0x002ad8f4) = 0;
	TWEAK_F32(0x002ad8f8) = 0.1f;
	TWEAK_F32(0x002ad8fc) = 0.9f;
	TWEAK_F32(0x002ad900) = 1.5f;
	TWEAK_F32(0x002ad904) = 2.5f;
	TWEAK_F32(0x002ad908) = 0.1f;
	TWEAK_I32(0x002ad90c) = 0;
	TWEAK_F32(0x002ad910) = 1e+01f;
	TWEAK_F32(0x002ad914) = 1.0f;
	TWEAK_F32(0x002ad918) = 0.4f;
	TWEAK_I32(0x002ad920) = 60;
	TWEAK_I32(0x002ad924) = 40;
	TWEAK_I32(0x002ad928) = 30;
	TWEAK_I32(0x002ad92c) = 0;
	TWEAK_I32(0x002ad930) = 60;
	TWEAK_I32(0x002ad934) = 40;
	TWEAK_I32(0x002ad938) = 20;
	TWEAK_I32(0x002ad93c) = 0;
	TWEAK_I32(0x002ad940) = 60;
	TWEAK_I32(0x002ad944) = 40;
	TWEAK_I32(0x002ad948) = 20;
	TWEAK_I32(0x002ad94c) = 0;
	TWEAK_I32(0x002ad950) = 60;
	TWEAK_I32(0x002ad954) = 40;
	TWEAK_I32(0x002ad958) = 20;
	TWEAK_I32(0x002ad95c) = 0;
	TWEAK_F32(0x002ad960) = 0.2f;
	TWEAK_F32(0x002ad964) = 0.2f;
	TWEAK_F32(0x002ad968) = 0.2f;
	TWEAK_I32(0x002ad96c) = 0;
	TWEAK_F32(0x002ad970) = 1.5f;
	TWEAK_F32(0x002ad974) = 1.5f;
	TWEAK_F32(0x002ad978) = 1.5f;
	TWEAK_F32(0x002ad97c) = 1.5f;
	TWEAK_F32(0x002ad980) = 0.1f;
	TWEAK_I32(0x002ad984) = 0;
	TWEAK_F32(0x002ad988) = 1e+01f;
	TWEAK_F32(0x002ad98c) = 0.9f;
	TWEAK_F32(0x002ad990) = 0.4f;
	TWEAK_I32(0x002ad998) = 127;
	TWEAK_I32(0x002ad99c) = 127;
	TWEAK_I32(0x002ad9a0) = 127;
	TWEAK_I32(0x002ad9a4) = 127;
	TWEAK_I32(0x002ad9a8) = 127;
	TWEAK_I32(0x002ad9ac) = 127;
	TWEAK_I32(0x002ad9b0) = 127;
	TWEAK_I32(0x002ad9b4) = 127;
	TWEAK_I32(0x002ad9b8) = 127;
	TWEAK_I32(0x002ad9bc) = 127;
	TWEAK_I32(0x002ad9c0) = 127;
	TWEAK_I32(0x002ad9c4) = 127;
	TWEAK_I32(0x002ad9c8) = 127;
	TWEAK_I32(0x002ad9cc) = 127;
	TWEAK_I32(0x002ad9d0) = 127;
	TWEAK_I32(0x002ad9d4) = 127;
	TWEAK_F32(0x002ad9d8) = 0.2f;
	TWEAK_F32(0x002ad9dc) = 2000.2f;
	TWEAK_F32(0x002ad9e0) = 0.2f;
	TWEAK_I32(0x002ad9e4) = 0;
	TWEAK_F32(0x002ad9e8) = 1.7f;
	TWEAK_F32(0x002ad9ec) = 1.7f;
	TWEAK_F32(0x002ad9f0) = 1.7f;
	TWEAK_F32(0x002ad9f4) = 1.7f;
	TWEAK_F32(0x002ad9f8) = 0.1f;
	TWEAK_I32(0x002ad9fc) = 0;
	TWEAK_I32(0x002ada00) = 0;
	TWEAK_F32(0x002ada04) = 0.9f;
	TWEAK_F32(0x002ada08) = 0.4f;
	TWEAK_I32(0x002ada0c) = 0;
	TWEAK_I32(0x002ada10) = 100;
	TWEAK_I32(0x002ada14) = 100;
	TWEAK_I32(0x002ada18) = 100;
	TWEAK_I32(0x002ada1c) = 100;
	TWEAK_I32(0x002ada20) = 127;
	TWEAK_I32(0x002ada24) = 127;
	TWEAK_I32(0x002ada28) = 127;
	TWEAK_I32(0x002ada2c) = 127;
	TWEAK_I32(0x002ada30) = 127;
	TWEAK_I32(0x002ada34) = 127;
	TWEAK_I32(0x002ada38) = 127;
	TWEAK_I32(0x002ada3c) = 127;
	TWEAK_I32(0x002ada40) = 127;
	TWEAK_I32(0x002ada44) = 127;
	TWEAK_I32(0x002ada48) = 127;
	TWEAK_I32(0x002ada4c) = 127;
	TWEAK_F32(0x002ada50) = 0.2f;
	TWEAK_F32(0x002ada54) = 2000.2f;
	TWEAK_F32(0x002ada58) = 0.2f;
	TWEAK_I32(0x002ada5c) = 0;
	TWEAK_F32(0x002ada60) = 1.0f;
	TWEAK_F32(0x002ada64) = 1.0f;
	TWEAK_F32(0x002ada68) = 1.0f;
	TWEAK_F32(0x002ada6c) = 1.0f;
	TWEAK_F32(0x002ada70) = 0.05f;
	TWEAK_I32(0x002ada74) = 0;
	TWEAK_I32(0x002ada78) = 0;
	TWEAK_F32(0x002ada7c) = 0.9f;
	TWEAK_F32(0x002ada80) = 0.4f;
	TWEAK_I32(0x002ada84) = 0;
	TWEAK_I32(0x002ada88) = 127;
	TWEAK_I32(0x002ada8c) = 100;
	TWEAK_I32(0x002ada90) = 50;
	TWEAK_I32(0x002ada94) = 10;
	TWEAK_I32(0x002ada98) = 90;
	TWEAK_I32(0x002ada9c) = 60;
	TWEAK_I32(0x002adaa0) = 30;
	TWEAK_I32(0x002adaa4) = 0;
	TWEAK_I32(0x002adaa8) = 90;
	TWEAK_I32(0x002adaac) = 60;
	TWEAK_I32(0x002adab0) = 30;
	TWEAK_I32(0x002adab4) = 0;
	TWEAK_I32(0x002adab8) = 90;
	TWEAK_I32(0x002adabc) = 60;
	TWEAK_I32(0x002adac0) = 30;
	TWEAK_I32(0x002adac4) = 0;
	TWEAK_F32(0x002adac8) = 0.5f;
	TWEAK_F32(0x002adacc) = 0.5f;
	TWEAK_F32(0x002adad0) = 0.5f;
	TWEAK_I32(0x002adad4) = 0;
	TWEAK_F32(0x002adad8) = 2.5f;
	TWEAK_F32(0x002adadc) = 2.5f;
	TWEAK_F32(0x002adae0) = 2.5f;
	TWEAK_F32(0x002adae4) = 2.5f;
	TWEAK_F32(0x002adb74) = (float)((double)DAT_00292c84 * 0.2f);  /* * [0x00257c7c] */
	TWEAK_I32(0x002adae8) = 0;
	TWEAK_I32(0x002adaec) = 0;
	TWEAK_F32(0x002adaf0) = 2e+01f;
	TWEAK_F32(0x002adaf4) = 1.0f;
	TWEAK_F32(0x002adaf8) = 0.4f;
	TWEAK_I32(0x002adafc) = 0;
	TWEAK_I32(0x002adb00) = 70;
	TWEAK_I32(0x002adb04) = 60;
	TWEAK_I32(0x002adb08) = 50;
	TWEAK_I32(0x002adb0c) = 40;
	TWEAK_I32(0x002adb10) = 60;
	TWEAK_I32(0x002adb14) = 60;
	TWEAK_I32(0x002adb18) = 60;
	TWEAK_I32(0x002adb1c) = 60;
	TWEAK_I32(0x002adb20) = 60;
	TWEAK_I32(0x002adb24) = 60;
	TWEAK_I32(0x002adb28) = 60;
	TWEAK_I32(0x002adb2c) = 60;
	TWEAK_I32(0x002adb30) = 80;
	TWEAK_I32(0x002adb34) = 80;
	TWEAK_I32(0x002adb38) = 80;
	TWEAK_I32(0x002adb3c) = 80;
	TWEAK_F32(0x002adb40) = 0.4f;
	TWEAK_F32(0x002adb44) = 0.3f;
	TWEAK_F32(0x002adb48) = 0.2f;
	TWEAK_I32(0x002adb4c) = 0;
	TWEAK_F32(0x002adb50) = 0.3f;
	TWEAK_F32(0x002adb54) = 0.8f;
	TWEAK_F32(0x002adb58) = 1.2f;
	TWEAK_F32(0x002adb5c) = 1.2f;
	TWEAK_I32(0x002adb60) = 0;
	TWEAK_I32(0x002adb64) = 0;
	TWEAK_I32(0x002adb68) = 0;
	TWEAK_F32(0x002adb6c) = 0.9f;
	TWEAK_F32(0x002adb70) = 0.4f;
	TWEAK_I32(0x002adb78) = 40;
	TWEAK_I32(0x002adb7c) = 30;
	TWEAK_I32(0x002adb80) = 20;
	TWEAK_I32(0x002adb84) = 0;
	TWEAK_I32(0x002adb88) = 50;
	TWEAK_I32(0x002adb8c) = 50;
	TWEAK_I32(0x002adb90) = 50;
	TWEAK_I32(0x002adb94) = 50;
	TWEAK_I32(0x002adb98) = 50;
	TWEAK_I32(0x002adb9c) = 50;
	TWEAK_I32(0x002adba0) = 50;
	TWEAK_I32(0x002adba4) = 50;
	TWEAK_I32(0x002adba8) = 70;
	TWEAK_I32(0x002adbac) = 70;
	TWEAK_I32(0x002adbb0) = 70;
	TWEAK_I32(0x002adbb4) = 70;
	TWEAK_F32(0x002adbb8) = 0.1f;
	TWEAK_F32(0x002adbbc) = 0.1f;
	TWEAK_F32(0x002adbc0) = 0.1f;
	TWEAK_I32(0x002adbc4) = 0;
	TWEAK_F32(0x002adbc8) = 0.4f;
	TWEAK_F32(0x002adbcc) = 0.4f;
	TWEAK_F32(0x002adbd0) = 0.3f;
	TWEAK_I32(0x002adbd4) = 0;
	TWEAK_I32(0x002adbd8) = 0;
	TWEAK_I32(0x002adbdc) = 0;
	TWEAK_I32(0x002adbe0) = 0;
	TWEAK_F32(0x002adbe4) = 0.9f;
	TWEAK_F32(0x002adbe8) = 0.4f;
	TWEAK_I32(0x002adbec) = 0;
	TWEAK_I32(0x002adbf0) = 100;
	TWEAK_I32(0x002adbf4) = 100;
	TWEAK_I32(0x002adbf8) = 50;
	TWEAK_I32(0x002adbfc) = 0;
	TWEAK_I32(0x002adc00) = 100;
	TWEAK_I32(0x002adc04) = 100;
	TWEAK_I32(0x002adc08) = 50;
	TWEAK_I32(0x002adc0c) = 0;
	TWEAK_I32(0x002adc10) = 100;
	TWEAK_I32(0x002adc14) = 100;
	TWEAK_I32(0x002adc18) = 50;
	TWEAK_I32(0x002adc1c) = 0;
	TWEAK_I32(0x002adc20) = 100;
	TWEAK_I32(0x002adc24) = 100;
	TWEAK_I32(0x002adc28) = 50;
	TWEAK_I32(0x002adc68) = 127;
	TWEAK_F32(0x002adcdc) = (float)((double)DAT_00292c84 * 0.2f);  /* * [0x00257c7c] */
	TWEAK_I32(0x002adc6c) = 127;
	TWEAK_F32(0x002add54) = (float)((double)DAT_00292c84 * 0.2f);  /* * [0x00257c7c] */
	TWEAK_I32(0x002adc70) = 127;
	TWEAK_I32(0x002adc74) = 127;
	TWEAK_I32(0x002adc78) = 127;
	TWEAK_I32(0x002adc7c) = 127;
	TWEAK_I32(0x002adc80) = 127;
	TWEAK_I32(0x002adc84) = 127;
	TWEAK_I32(0x002adc88) = 127;
	TWEAK_I32(0x002adc8c) = 127;
	TWEAK_I32(0x002adc90) = 127;
	TWEAK_I32(0x002adc94) = 127;
	TWEAK_I32(0x002adc98) = 127;
	TWEAK_I32(0x002adc9c) = 127;
	TWEAK_I32(0x002adca0) = 127;
	TWEAK_I32(0x002adca4) = 127;
	TWEAK_I32(0x002adc2c) = 1;
	TWEAK_F32(0x002adc30) = 0.1f;
	TWEAK_F32(0x002adc34) = 2000.1f;
	TWEAK_F32(0x002adc38) = 0.1f;
	TWEAK_I32(0x002adc3c) = 0;
	TWEAK_F32(0x002adc40) = 0.4f;
	TWEAK_F32(0x002adc44) = 0.4f;
	TWEAK_F32(0x002adc48) = 0.3f;
	TWEAK_I32(0x002adc4c) = 0;
	TWEAK_F32(0x002adc50) = 0.2f;
	TWEAK_I32(0x002adc54) = 0;
	TWEAK_F32(0x002adc58) = 2e+01f;
	TWEAK_F32(0x002adc5c) = 0.9f;
	TWEAK_F32(0x002adc60) = 0.4f;
	TWEAK_I32(0x002adc64) = 0;
	TWEAK_F32(0x002adca8) = 0.1f;
	TWEAK_F32(0x002adcac) = 2000.1f;
	TWEAK_F32(0x002adcb0) = 0.1f;
	TWEAK_I32(0x002adcb4) = 0;
	TWEAK_F32(0x002adcb8) = 1.5f;
	TWEAK_F32(0x002adcbc) = 1.5f;
	TWEAK_F32(0x002adcc0) = 1.5f;
	TWEAK_F32(0x002adcc4) = 1.5f;
	TWEAK_F32(0x002adcc8) = 0.1f;
	TWEAK_F32(0x002adccc) = 0.2f;
	TWEAK_F32(0x002adcd0) = 2e+01f;
	TWEAK_F32(0x002adcd4) = 0.9f;
	TWEAK_F32(0x002adcd8) = 0.4f;
	TWEAK_I32(0x002adce0) = 100;
	TWEAK_I32(0x002adce4) = 100;
	TWEAK_I32(0x002adce8) = 50;
	TWEAK_I32(0x002adcec) = 0;
	TWEAK_I32(0x002adcf0) = 100;
	TWEAK_I32(0x002adcf4) = 100;
	TWEAK_I32(0x002adcf8) = 50;
	TWEAK_I32(0x002adcfc) = 0;
	TWEAK_I32(0x002add00) = 100;
	TWEAK_I32(0x002add04) = 100;
	TWEAK_I32(0x002add08) = 50;
	TWEAK_I32(0x002add0c) = 0;
	TWEAK_I32(0x002add10) = 100;
	TWEAK_I32(0x002add14) = 100;
	TWEAK_I32(0x002add18) = 50;
	TWEAK_I32(0x002add1c) = 1;
	TWEAK_F32(0x002add20) = 0.5f;
	TWEAK_F32(0x002add24) = 0.5f;
	TWEAK_F32(0x002add28) = 0.2f;
	TWEAK_I32(0x002add2c) = 0;
	TWEAK_F32(0x002add30) = 0.5f;
	TWEAK_F32(0x002add34) = 0.5f;
	TWEAK_F32(0x002add38) = 0.5f;
	TWEAK_F32(0x002add3c) = 0.25f;
	TWEAK_F32(0x002add40) = 0.02f;
	TWEAK_F32(0x002add44) = 0.005f;
	TWEAK_F32(0x002add48) = 2e+01f;
	TWEAK_F32(0x002add4c) = 0.9f;
	TWEAK_F32(0x002add50) = 0.4f;
	TWEAK_I32(0x002add58) = 50;
	TWEAK_I32(0x002add5c) = 50;
	TWEAK_I32(0x002add60) = 40;
	TWEAK_I32(0x002add64) = 5;
	TWEAK_I32(0x002add68) = 10;
	TWEAK_I32(0x002addd4) = 100;
	TWEAK_I32(0x002addd8) = 100;
	TWEAK_I32(0x002adde0) = 115;
	TWEAK_I32(0x002adde4) = 115;
	TWEAK_F32(0x002ade44) = (float)((double)DAT_00292c84 * 0.35f);  /* * [0x00265590] */
	TWEAK_I32(0x002adde8) = 115;
	TWEAK_I32(0x002addec) = 20;
	TWEAK_I32(0x002addf0) = 20;
	TWEAK_I32(0x002addf4) = 20;
	TWEAK_I32(0x002addf8) = 20;
	TWEAK_I32(0x002ade00) = 20;
	TWEAK_I32(0x002ade04) = 20;
	TWEAK_I32(0x002ade08) = 20;
	TWEAK_I32(0x002add6c) = 10;
	TWEAK_I32(0x002add70) = 0;
	TWEAK_I32(0x002add74) = 0;
	TWEAK_I32(0x002add78) = 60;
	TWEAK_I32(0x002add7c) = 60;
	TWEAK_I32(0x002add80) = 30;
	TWEAK_I32(0x002add84) = 5;
	TWEAK_I32(0x002add88) = 10;
	TWEAK_I32(0x002add8c) = 10;
	TWEAK_I32(0x002add90) = 0;
	TWEAK_I32(0x002add94) = 0;
	TWEAK_F32(0x002add98) = 0.4f;
	TWEAK_F32(0x002add9c) = 0.4f;
	TWEAK_F32(0x002adda0) = 0.6f;
	TWEAK_F32(0x002adda4) = 0.4f;
	TWEAK_F32(0x002adda8) = 0.8f;
	TWEAK_F32(0x002addac) = 2.0f;
	TWEAK_F32(0x002addb0) = 6.0f;
	TWEAK_F32(0x002addb4) = 9.0f;
	TWEAK_I32(0x002addb8) = 0;
	TWEAK_I32(0x002addbc) = 0;
	TWEAK_I32(0x002addc0) = 0;
	TWEAK_F32(0x002addc4) = 1.0f;
	TWEAK_F32(0x002addc8) = 0.4f;
	TWEAK_I32(0x002addcc) = 0;
	TWEAK_I32(0x002addd0) = 110;
	TWEAK_I32(0x002adddc) = 0;
	TWEAK_I32(0x002addfc) = 0;
	TWEAK_I32(0x002ade0c) = 0;
	TWEAK_F32(0x002ade10) = 0.4f;
	TWEAK_F32(0x002ade14) = 0.4f;
	TWEAK_F32(0x002ade18) = 0.9f;
	TWEAK_F32(0x002ade1c) = 0.3f;
	TWEAK_F32(0x002ade20) = 0.6f;
	TWEAK_F32(0x002ade24) = 0.8f;
	TWEAK_F32(0x002ade28) = 0.7f;
	TWEAK_F32(0x002ade2c) = 0.5f;
	TWEAK_I32(0x002ade30) = 0;
	TWEAK_I32(0x002ade34) = 0;
	TWEAK_I32(0x002ade38) = 0;
	TWEAK_F32(0x002ade3c) = 0.9f;
	TWEAK_F32(0x002ade40) = 0.4f;
	TWEAK_I32(0x002ade48) = 60;
	TWEAK_I32(0x002ade4c) = 50;
	TWEAK_I32(0x002ade50) = 50;
	TWEAK_I32(0x002ade54) = 40;
	TWEAK_I32(0x002ade58) = 50;
	TWEAK_I32(0x002ade5c) = 50;
	TWEAK_I32(0x002ade60) = 50;
	TWEAK_I32(0x002ade64) = 5;
	TWEAK_I32(0x002ade68) = 60;
	TWEAK_I32(0x002ade6c) = 60;
	TWEAK_I32(0x002ade70) = 60;
	TWEAK_I32(0x002ade74) = 60;
	TWEAK_I32(0x002ade78) = 80;
	TWEAK_I32(0x002ade7c) = 80;
	TWEAK_I32(0x002ade80) = 80;
	TWEAK_I32(0x002ade84) = 80;
	TWEAK_F32(0x002ade88) = 0.1f;
	TWEAK_F32(0x002ade8c) = 0.1f;
	TWEAK_F32(0x002ade90) = 0.1f;
	TWEAK_F32(0x002ade94) = 0.5f;
	TWEAK_F32(0x002ade98) = 0.5f;
	TWEAK_I32(0x002adee0) = 10;
	TWEAK_F32(0x002adebc) = (float)((double)DAT_00292c84 * 0.2f);  /* * [0x00257c7c] */
	TWEAK_I32(0x002adee4) = 10;
	TWEAK_I32(0x002adef0) = 10;
	TWEAK_I32(0x002adef4) = 10;
	TWEAK_I32(0x002adec0) = 50;
	TWEAK_I32(0x002adec4) = 50;
	TWEAK_I32(0x002aded0) = 60;
	TWEAK_I32(0x002aded4) = 60;
	TWEAK_F32(0x002ade9c) = 0.8f;
	TWEAK_F32(0x002adea0) = 0.8f;
	TWEAK_F32(0x002adea4) = 0.7f;
	TWEAK_I32(0x002adea8) = 0;
	TWEAK_I32(0x002adeac) = 0;
	TWEAK_I32(0x002adeb0) = 0;
	TWEAK_F32(0x002adeb4) = 0.9f;
	TWEAK_F32(0x002adeb8) = 0.4f;
	TWEAK_I32(0x002adec8) = 30;
	TWEAK_I32(0x002adecc) = 0;
	TWEAK_I32(0x002aded8) = 30;
	TWEAK_I32(0x002adedc) = 0;
	TWEAK_I32(0x002adee8) = 0;
	TWEAK_I32(0x002adeec) = 0;
	TWEAK_I32(0x002adef8) = 0;
	TWEAK_I32(0x002adefc) = 0;
	TWEAK_F32(0x002adf00) = 0.6f;
	TWEAK_F32(0x002adf04) = 0.6f;
	TWEAK_F32(0x002adf08) = 0.3f;
	TWEAK_F32(0x002adf0c) = 0.5f;
	TWEAK_F32(0x002adf10) = 0.5f;
	TWEAK_F32(0x002adf14) = 2.5f;
	TWEAK_F32(0x002adf18) = 5.0f;
	TWEAK_F32(0x002adf1c) = 8.0f;
	TWEAK_I32(0x002adf20) = 0;
	TWEAK_I32(0x002adf24) = 0;
	TWEAK_F32(0x002adf28) = 2e+01f;
	TWEAK_F32(0x002adf2c) = 1.0f;
	TWEAK_F32(0x002adf30) = 0.4f;
	TWEAK_I32(0x002adf34) = 0;
#undef TWEAK_I32
#undef TWEAK_U32
#undef TWEAK_F32
}

/* 0x001f48b0  LIB:crt_init (.CRT$XC #35) */
void FUN_001f48b0(void)
{
	ext_ListInit(DAT_00302300);
	atexit(FUN_001f5030);
}

/* 0x001f48d0  LIB:crt_init (.CRT$XC #36) */
void FUN_001f48d0(void)
{
	ext_ListInit(DAT_0030230c);
	atexit(FUN_001f5040);
}

/* 0x001f48f0  LIB:crt_init (.CRT$XC #37) */
void FUN_001f48f0(void)
{
	ext_ListInit(DAT_00302318);
	atexit(FUN_001f5050);
}

/* 0x001f4910  LIB:crt_init (.CRT$XC #38) */
void FUN_001f4910(void)
{
	ext_ListInit(DAT_00302324);
	atexit(FUN_001f5060);
}

/* 0x001f4930  LIB:crt_init (.CRT$XC #39) */
void FUN_001f4930(void)
{
	ext_ListInit(DAT_00302330);
	atexit(FUN_001f5070);
}

/* 0x001f4950  LIB:crt_init (.CRT$XC #40) */
void FUN_001f4950(void)
{
	atexit(FUN_001f5080);
}

/* 0x001f4960  LIB:crt_init (.CRT$XC #41) */
void FUN_001f4960(void)
{
	ext_Ctor_00025920(DAT_003024c8);
	atexit(FUN_001f5090);
}

/* 0x001f4980  LIB:crt_init (.CRT$XC #42) */
void FUN_001f4980(void)
{
	ext_Ctor_0013eae0(DAT_00302798);
	atexit(FUN_001f50a0);
}

/* 0x001f49a0  LIB:crt_init (.CRT$XC #43) */
void FUN_001f49a0(void)
{
	atexit(FUN_001f50b0);
}

/* 0x001f49b0  LIB:crt_init (.CRT$XC #44) */
void FUN_001f49b0(void)
{
	atexit(FUN_001f50c0);
}

/* 0x001f49c0  LIB:crt_init (.CRT$XC #45) */
void FUN_001f49c0(void)
{
	memset(DAT_002aec24, 0, sizeof(DAT_002aec24));
	atexit(FUN_001f50d0);
}

/* 0x001f49e0  LIB:crt_init (.CRT$XC #46) */
void FUN_001f49e0(void)
{
	atexit(FUN_001f50e0);
}

/* 0x001f49f0  LIB:crt_init (.CRT$XC #47) */
void FUN_001f49f0(void)
{
	atexit(FUN_001f50f0);
}

/* 0x001f4a00  LIB:crt_init (.CRT$XC #48) */
void FUN_001f4a00(void)
{
	atexit(FUN_001f5100);
}

/* 0x001f4a10  LIB:crt_init (.CRT$XC #49) */
void FUN_001f4a10(void)
{
	atexit(FUN_001f5110);
}

/* 0x001f4a20  LIB:crt_init (.CRT$XC #50) */
void FUN_001f4a20(void)
{
	ext_Ctor_00025920(DAT_003055e8);
	atexit(FUN_001f5140);
}

/* 0x001f4a40  LIB:crt_init (.CRT$XC #51) */
void FUN_001f4a40(void)
{
	ext_ListInit(DAT_00305630);
	atexit(FUN_001f5120);
}

/* 0x001f4a60  LIB:crt_init (.CRT$XC #52) */
void FUN_001f4a60(void)
{
	ext_ListInit(DAT_0030563c);
	atexit(FUN_001f5130);
}

/* 0x001f4a80  LIB:crt_init (.CRT$XC #53) */
void FUN_001f4a80(void)
{
	ext_ListInit(DAT_003075b0);
	atexit(FUN_001f5150);
}

/* 0x001f4aa0  LIB:crt_init (.CRT$XC #54) */
void FUN_001f4aa0(void)
{
	ext_ListInit(DAT_003082d4);
	atexit(FUN_001f5160);
}

/* 0x001f4ac0  LIB:crt_init (.CRT$XC #55) */
void FUN_001f4ac0(void)
{
	ext_ListInit(DAT_00308440);
	atexit(FUN_001f5170);
}

/* 0x001f4ae0  LIB:crt_init (.CRT$XC #56) */
void FUN_001f4ae0(void)
{
	ext_ListInit(DAT_0030844c);
	atexit(FUN_001f5180);
}

/* 0x001f4b00  LIB:crt_init (.CRT$XC #57) */
void FUN_001f4b00(void)
{
	ext_Ctor_0015ca00(DAT_00308460);
	atexit(FUN_001f5190);
}

/* 0x001f4b20  LIB:crt_init (.CRT$XC #58) */
void FUN_001f4b20(void)
{
	ext_ListInit(DAT_00308e60);
	atexit(FUN_001f51a0);
}

/* 0x001f4b40  LIB:crt_init (.CRT$XC #59) */
void FUN_001f4b40(void)
{
	ext_ListInit(DAT_00308e6c);
	atexit(FUN_001f51b0);
}

/* 0x001f4b60  LIB:crt_init (.CRT$XC #60) */
void FUN_001f4b60(void)
{
	ext_ListInit(DAT_00308f08);
	atexit(FUN_001f51c0);
}

/* 0x001f4b80  LIB:crt_init (.CRT$XC #61) */
void FUN_001f4b80(void)
{
	ext_ListInit(DAT_00308f1c);
	atexit(FUN_001f51d0);
}

/* 0x001f4ba0  LIB:crt_init (.CRT$XC #62) */
void FUN_001f4ba0(void)
{
	ext_ListInit(DAT_00308f2c);
	atexit(FUN_001f51e0);
}

/* 0x001f4bc0  LIB:crt_init (.CRT$XC #63) */
void FUN_001f4bc0(void)
{
	ext_ListInit(DAT_00308f48);
	atexit(FUN_001f51f0);
}

/* 0x001f4be0  LIB:crt_init (.CRT$XC #64) */
void FUN_001f4be0(void)
{
	ext_ListInit(DAT_00308f3c);
	atexit(FUN_001f5200);
}

/* 0x001f4c00  LIB:crt_init (.CRT$XC #65) */
void FUN_001f4c00(void)
{
	int i;

	/* eh vector constructor iterator: 8 objects of 0x20 bytes at 0x00308f58 */
	for (i = 0; i < 8; i++)
		ext_Ctor_001608b0(DAT_00308f58 + i * 0x20);
	atexit(FUN_001f5210);
}

/* 0x001f4c30  LIB:crt_init (.CRT$XC #66) */
void FUN_001f4c30(void)
{
	ext_ListInit(DAT_003090a8);
	atexit(FUN_001f5230);
}

/* 0x001f4c50  LIB:crt_init (.CRT$XC #67) */
void FUN_001f4c50(void)
{
	ext_ListInit(DAT_003090d8);
	atexit(FUN_001f5240);
}

/* 0x001f4c70  LIB:crt_init (.CRT$XC #68) */
void FUN_001f4c70(void)
{
	ext_ListInit(DAT_00309100);
	atexit(FUN_001f5250);
}

/* 0x001f4c90  LIB:crt_init (.CRT$XC #69) */
void FUN_001f4c90(void)
{
	ext_ListInit(DAT_00309268);
	atexit(FUN_001f5260);
}

/* 0x001f4cb0  LIB:crt_init (.CRT$XC #70) */
void FUN_001f4cb0(void)
{
	ext_ListInit(DAT_0030a09c);
	atexit(FUN_001f5270);
}

/* 0x001f4cd0  LIB:crt_init (.CRT$XC #71) */
void FUN_001f4cd0(void)
{
	ext_Ctor_00025920(DAT_0030a0f0);
	atexit(FUN_001f5290);
}

/* 0x001f4cf0  LIB:crt_init (.CRT$XC #72) */
void FUN_001f4cf0(void)
{
	ext_ListInit(DAT_0030a0e4);
	atexit(FUN_001f5280);
}

/* 0x001f4d10  LIB:crt_init (.CRT$XC #73) */
void FUN_001f4d10(void)
{
	ext_Ctor_00170ea0(DAT_0030a1d0);
	atexit(FUN_001f52b0);
}

/* 0x001f4d30  LIB:crt_init (.CRT$XC #74) */
void FUN_001f4d30(void)
{
	ext_ListInit(DAT_0030ea20);
	atexit(FUN_001f52a0);
}

/* 0x001f4d50  LIB:crt_init (.CRT$XC #75) */
void FUN_001f4d50(void)
{
	atexit(FUN_001f52c0);
}

/* 0x001f4d60  LIB:crt_init (.CRT$XC #76) */
void FUN_001f4d60(void)
{
	ext_ListInit(DAT_0030ed00);
	atexit(FUN_001f52d0);
}

/* 0x001f4d80  LIB:crt_init (.CRT$XC #77) */
void FUN_001f4d80(void)
{
	ext_ListInit(DAT_0030ee30);
	atexit(FUN_001f52e0);
}

/* 0x001f4da0  LIB:crt_init (.CRT$XC #78) */
void FUN_001f4da0(void)
{
	atexit(FUN_001f52f0);
}

/* 0x001f4db0  LIB:crt_init (.CRT$XC #79) */
void FUN_001f4db0(void)
{
	ext_ListInit(DAT_0030fea4);
	atexit(FUN_001f5300);
}

/* 0x001f4dd0  LIB:crt_init (.CRT$XC #80) */
void FUN_001f4dd0(void)
{
	ext_Ctor_001c1880(DAT_00312f50);
	atexit(FUN_001f5320);
}

/* 0x001f4df0  LIB:crt_init (.CRT$XC #81) */
void FUN_001f4df0(void)
{
	ext_Ctor_001d6cb0(DAT_003150b0);
	atexit(FUN_001f5310);
}

