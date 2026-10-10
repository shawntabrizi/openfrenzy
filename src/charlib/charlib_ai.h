/*
CHARLIB_AI.H

charlib module. Field offsets of the CharLib AI player (0x1130 bytes, vtable 0x00266fa8;
the minigames derive from it). One AI object exists per player slot in chl_ai_players
(0x002f289c). The AI drives its actor (AI_ACTOR) through a fake joypad: AI_STICK_Y,
AI_STICK_X and AI_BUTTON point at objects whose int at +0x14 is the value.

The AI runs a small state machine: AI_STATE indexes AI_STATE_FUNCS (handler(ai, entered));
each frame every condition AI_COND_FUNCS[i](ai) that is true proposes switching with the
priority AI_PRIORITY[state * AI_COND_COUNT + i]; the highest one wins.

Other modules may include this header. Only the charlib agent edits it.
*/

#ifndef OPENFRENZY_CHARLIB_AI_H
#define OPENFRENZY_CHARLIB_AI_H

/* 0x002f289c  the AI players, one per player slot (4) */
#define chl_ai_players CHL_ARR(void *, 0x002f289c)

/* virtual methods (vtable 0x00266fa8; slots 5, 6, 14 and 15 are pure) */
#define AI_VSLOT_UPDATE_PRE 0      /* 0x00 (ai) before AI_Update */
#define AI_VSLOT_UPDATE_POST 1     /* 0x04 (ai) after AI_Update */
#define AI_VSLOT_PAUSED 2          /* 0x08 (ai) while the game is paused (DAT_002baea4) */
#define AI_VSLOT_THINK 3           /* 0x0c AI_Think */
#define AI_VSLOT_ACT 5             /* 0x14 (ai) end of AI_Think */
#define AI_VSLOT_SET_MODE 6        /* 0x18 (ai, mode) */
#define AI_VSLOT_WANTS_MODE 7      /* 0x1c (ai) -> nonzero: AI_Think resets the mode */
#define AI_VSLOT_RESPAWN_AT 8      /* 0x20 AI_RespawnAt */
#define AI_VSLOT_RESPAWN_FAR 9     /* 0x24 AI_RespawnFarthest */
#define AI_VSLOT_SPAWN 10          /* 0x28 AI_Spawn */
#define AI_VSLOT_PRE_ACT 11        /* 0x2c (ai) */
#define AI_VSLOT_CAN_MOVE 12       /* 0x30 AI_CanMove */
#define AI_VSLOT_TIMER 14          /* 0x38 (ai, timer index) a timer of AI_TIMERS ran out */
#define AI_VSLOT_HOLD_END 16       /* 0x40 AI_HoldEnd */
#define AI_VCALL(ai, slot, type) ((type)((*(void ***)(ai))[slot]))

#define AI_MODE 0x04               /* int: the mode of the derived class (6: avoid stacking) */
#define AI_TARGET 0x1c             /* float *: position to go to */
#define AI_THINK_TIME 0x20         /* float: frames since the last plan */
#define AI_MODE_TIME 0x24          /* int */
#define AI_FORCE_THINK 0x28        /* int */
#define AI_STACK_COUNT 0x2c        /* int: grows while standing on / under another player */
#define AI_ACTOR 0x30              /* the CharLib actor of the player */
#define AI_COND_COUNT 0x34         /* unsigned short */
#define AI_STATE 0x36              /* unsigned short */
#define AI_PREV_STATE 0x38         /* unsigned short */
#define AI_STATE_FRAMES 0x3c       /* int */
#define AI_STATE_TIME 0x40         /* float */
#define AI_MOVE_DIR 0x44           /* float[4]: stick direction in the world */
#define AI_STICK_AMOUNT 0x54       /* float: 0..1 */
#define AI_MOVE_HEADING 0x58       /* float */
#define AI_SENSORS 0x5c            /* { entries (0xf0 bytes each, int at +0x14), ?, count } */
#define AI_STICK_Y 0x60            /* pad axis object (+0x14 int, -32760..32760) */
#define AI_STICK_X 0x64            /* pad axis object */
#define AI_BUTTON 0x68             /* pad button object: +0x14 = 1 presses it (jump) */
#define AI_PUSH 0x7c               /* float[3]: extra velocity added each frame */
#define AI_ROUTE 0x8c              /* struct CLRoute (0xff4 bytes) */
#define AI_WALL_COUNT 0x1080       /* short: grows while walking into walls */
#define AI_CLIMB_COUNT 0x1082      /* short: frames trying to climb */
#define AI_ENEMY 0x1084            /* short: target player (-1) */
#define AI_SHORT_1086 0x1086       /* short (-1) */
#define AI_SHORT_1088 0x1088       /* short (-1) */
#define AI_STATE_FUNCS 0x1094      /* void (**)(void *ai, int entered) */
#define AI_COND_FUNCS 0x1098       /* int (**)(void *ai) */
#define AI_PRIORITY 0x109c         /* signed char[state][condition] */
#define AI_TIMERS 0x10a0           /* float[4] */
#define AI_MAX_SPEED_SCALE 0x10b0  /* float */
#define AI_THINK_INTERVAL 0x10b4   /* float: frames between plans */
#define AI_DISABLED 0x10b8         /* short: 1 while (re)spawning */
#define AI_HELD 0x10bc             /* void *[2]: actors the player holds */
#define AI_HOLD_NODE 0x10c4        /* void *[2]: nodes they are held at */
#define AI_HOLD_ANIM_NODE 0x10cc   /* the node of the hold animation */
#define AI_TIMER_10D0 0x10d0       /* float */
#define AI_SLOW_RADIUS 0x10d4      /* float: the stick eases off inside it */
#define AI_SPEED 0x10dc            /* float: wanted top speed (200) */
#define AI_SKILL 0x10e0            /* float: 0..1, scales the acceleration */
#define AI_CAMERA_ANGLE 0x1118     /* float: stick directions are relative to it */
#define AI_COOLDOWN 0x111c         /* float */
#define AI_RESPAWNS 0x1114         /* int */
#define AI_FIRST_SPAWN_DONE 0x1124 /* int */
#define AI_LIGHT 0x112c            /* the light the player carries */

#endif
