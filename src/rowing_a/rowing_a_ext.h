/*
ROWING_A_EXT.H

Calls from rowing_a into rowing_b (0x001b12c0-0x001ba160) and other modules, by role.
rowing_b functions are called by their FUN_<addr> names with the prototypes rowing_b gives
in rowing_b.decls (checked against the disassembly at rowing_a's call sites, 2026-10-10);
where `this` (ECX) is passed but not used, rowing_b's prototype has no `this` and neither
does the macro. Functions of other modules whose of_decls.h prototype is a Ghidra guess go
through a cast to the real shape (RA_CALL, docs/ARCHITECTURE.md).

Owner: rowing_a.
*/

#ifndef OPENFRENZY_ROWING_A_EXT_H
#define OPENFRENZY_ROWING_A_EXT_H

/* ---------- rowing_b: CRowing_Lights.cpp (the lights object, game +0xa4) */
#define rwb_Lights_Ctor(self) FUN_001b4f90((struct RowLights *)(self))
#define rwb_Lights_AddPoint(lights, type, pos, r, g, b, falloff, flag) \
	(void *)FUN_001b52e0((struct RowLights *)(lights), (type), (pos), (r), (g), (b), (falloff), \
		(unsigned char)(flag))
#define rwb_Lights_AddDirectional(lights, type, dir, r, g, b, flag) \
	(void *)FUN_001b5370((struct RowLights *)(lights), (type), (dir), (r), (g), (b), (unsigned char)(flag))
/* light sprite and size (`this` in ECX is the lights object and not used) */
#define rwb_Light_SetTexture(light, texture, size) \
	FUN_001b5010((struct RowLight *)(light), (void *)(texture), (size))
#define rwb_Lights_SetFlags(lights, type, set, toggle) \
	FUN_001b56a0((struct RowLights *)(lights), (type), (set), (toggle))
#define rwb_Lights_SetColour(lights, light, r, g, b) \
	FUN_001b5590((struct RowLights *)(lights), (struct RowLight *)(light), (r), (g), (b))
#define rwb_Lights_SetMode(lights, mode) FUN_001b5630((struct RowLights *)(lights), (mode))
#define rwb_Lights_Enable(lights, type, on) FUN_001b5660((struct RowLights *)(lights), (type), (on))
#define rwb_Lights_Draw(lights) FUN_001b5040((struct RowLights *)(lights))
#define rwb_Lights_Dtor(lights) FUN_001b4fb0((struct RowLights *)(lights))
#define rwb_Lights_SetPosition(lights, light, pos) \
	FUN_001b55d0((struct RowLights *)(lights), (struct RowLight *)(light), (pos))

/* ---------- rowing_b: the world (game +0x7c, 0x37c bytes) */
#define rwb_World_Ctor(self) FUN_001b2c10(self)
#define rwb_World_Init(world, package, name) FUN_001b43c0((world), (void *)(package), (name))
#define rwb_World_SetFog() FUN_001b31d0()
#define rwb_World_DrawA(world, detail) FUN_001b4720((world), (detail))
#define rwb_World_DrawB(world, detail) FUN_001b3230((world), (detail))
#define rwb_World_DrawDetail(world) FUN_001b3280(world)
#define rwb_World_AddObject(objects, package, name, pos, rot) \
	FUN_001b6a90((objects), (void *)(package), (name), (pos), (rot))
#define rwb_World_Tick(world, dt) FUN_001b3170((world), (dt))
#define rwb_World_Shutdown(world) FUN_001b4530(world)
/* the sound banks */
#define rwb_Sound_Init(package, genPackage) FUN_001b6da0((void *)(package), (void *)(genPackage))

/* ---------- rowing_b: Rowing_Sound.cpp (the sound queue, 0x0030fc80) */
#define rwb_Sound_Play(id, volume) FUN_001b6ba0((id), (volume))
#define rwb_Sound_PlayAt(id, pos, volume) FUN_001b6be0((id), (pos), (volume))
#define rwb_Sound_PlayRandom(id, volume) FUN_001b6c30((id), (volume))
#define rwb_Sound_SetPitch(handle, channel, pitch) FUN_001b6c80((handle), (channel), (pitch))
#define rwb_Sound_Track(sound, handle) FUN_001b6d60((sound), (handle))
#define rwb_Sound_Update(sound) FUN_001b6cc0(sound)
#define rwb_Sound_Shutdown() FUN_001b6f50()
#define g_RowSound RW_VAR(void *, 0x0030fc80)

/* ---------- rowing_b: other objects */
#define rwb_GameMode_Ctor(self) FUN_001b60c0(self)
/* the tokens of variant 3 (`this` in ECX is not used) */
#define rwb_Tokens_Init(package) FUN_001b5730((int)(uintptr_t)(package))
#define rwb_Tokens_Draw() FUN_001b5790()
#define rwb_Tokens_DrawBlended() FUN_001b57b0()
#define rwb_Tokens_Update() FUN_001b5a00()
/* the gate arrows ("buoys") of variant 2 */
#define rwb_Buoy_Ctor(self, pos, yaw) (void *)FUN_001b5b30((struct RowArrow *)(self), (pos), (yaw))
#define rwb_Buoy_Dtor(self) FUN_001b5b10((struct RowArrow *)(self))
#define rwb_Buoy_Update(self, dt) FUN_001b5c50((struct RowArrow *)(self), (dt))
#define rwb_Buoy_Draw(buoy, alpha) FUN_001b5ea0((struct RowArrow *)(buoy), (alpha))
/* the floating mines/debris of variant 4 */
#define rwb_Mine_Trigger(self, a, b, flag) FUN_001b8760((self), (a), (b), (flag))
/* the sparks of variant 3 (game +0x270) */
#define rwb_Obj270_Ctor(self) (void *)FUN_001b47b0((struct RowSparks *)(self))
#define rwb_Obj270_Init(obj, texture, count) FUN_001b4c60((struct RowSparks *)(obj), (void *)(texture), (count))
#define rwb_Obj270_Draw(obj) FUN_001b4800((struct RowSparks *)(obj))
#define rwb_Obj270_Update(obj, dt) FUN_001b4d40((struct RowSparks *)(obj), (dt))
#define rwb_Obj270_Dtor(obj) FUN_001b47c0(obj)
/* the sprites (`this` in ECX, the sprite manager, is not used) */
#define rwb_Sprites_DrawOpaque() FUN_001b1430()
#define rwb_Sprites_DrawBlended() FUN_001b1490()
#define rwb_Particles_Init(vents, sprites, steam, blobs) \
	FUN_001b1c60((struct RowParticles *)(vents), (int)(uintptr_t)(sprites), (steam), (blobs))
#define rwb_Particles_Draw(vents) FUN_001b2b10((struct RowParticles *)(vents))

/* ---------- other modules */

/* classlib_ui 0x00071f90 returns the package (the table says void) */
#define ra_AddPackage RA_CALL(FUN_00071f90, void *(*)(const char *, int))
/* foxhunt 0x00138790: the CWaterField constructor (`this` in ECX) */
#define ra_WaterField_Ctor(self) RA_CALL(FUN_00138790, void *(*)(void *))(self)
/* a CWaterField virtual function at byte offset OFF */
#define RA_WATER_VFN(w, off, type) RA_VFN(w, off, type)

#endif
