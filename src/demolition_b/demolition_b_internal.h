/*
demolition_b_internal.h - helpers shared by the demolition_b source files only.
*/

#ifndef DEMOLITION_B_INTERNAL_H
#define DEMOLITION_B_INTERNAL_H

#include "demolition_b.h"

#include <math.h>

/* ---------- the hit message (demolition_d: constructor 0x0010cec0, vtable 0x0026bd20,
   destructor 0x0010ced0). CDemolition_CharLib's message receiver (at +0x1130) takes it
   through FUN_0010d050(receiver, message). */
struct DbMessage {
	void *vtable;       /* +0x00 */
	short type;         /* +0x04 */
	short pad06;        /* +0x06 */
	int kind;           /* +0x08 */
	int mode;           /* +0x0c */
	float pos[4];       /* +0x10 */
	float value;        /* +0x20 */
};
OF_ASSERT_SIZE(struct DbMessage, 0x24);

static inline void db_message_send(void *receiver, struct DbMessage *message)
{
	FUN_0010d050((int)receiver, (int)message);
}

/* ---------- the player list of the game (DAT_002ff85c + 0x6c; players link at +0x1140) */
#define DB_GAME_PLAYERS() ((char *)DB_PTR(DAT_002ff85c, 0x6c))
#define DB_PLAYER_NEXT(p) ((char *)DB_PTR(p, 0x1140))

/* distance between two points (x, y, z); in double, closer to the x87 extended precision
   the original compares in */
static inline double db_dist3(const float *a, const float *b)
{
	double dx = (double)a[0] - b[0];
	double dy = (double)a[1] - b[1];
	double dz = (double)a[2] - b[2];

	return sqrt(dx * dx + dy * dy + dz * dz);
}

/* ---------- calls into other modules whose table prototype hides a float */

/* 0x000ebff0 (demolition_a)  floating text at `pos` in colour (r, g, b, a), printf-style */
#define DB_POPUP_TABLE void (*)(int, float *, undefined4, undefined4, undefined4, undefined4, undefined4)
#define DB_POPUP_REAL void (*)(void *, float *, int, int, int, int, const char *, ...)
static inline void db_popup(float *pos, int r, int g, int b, const char *text)
{
	DB_DUAL(FUN_000ebff0, DB_POPUP_TABLE, DB_POPUP_REAL,
		((int)(DAT_002ff85c + 0x7900), pos, r, g, b, 0x7f, (undefined4)text),
		(DAT_002ff85c + 0x7900, pos, r, g, b, 0x7f, text));
}

/* the same with one int for the format (the table prototype has no room for it; while the
   table prototype stands, the callee cannot read it anyway) */
static inline void db_popup_int(float *pos, int r, int g, int b, const char *format, int value)
{
	DB_DUAL(FUN_000ebff0, DB_POPUP_TABLE, DB_POPUP_REAL,
		((int)(DAT_002ff85c + 0x7900), pos, r, g, b, 0x7f, (undefined4)format),
		(DAT_002ff85c + 0x7900, pos, r, g, b, 0x7f, format, value));
}

/* rollerball_a timer helpers (0x0018ea10 SetTime(float), 0x0018ea30 Start) */
static inline void db_timer_start(void *timer, float seconds)
{
	FUN_0018ea10((int)timer, seconds);
	FUN_0018ea30((int)timer);
}

#endif
