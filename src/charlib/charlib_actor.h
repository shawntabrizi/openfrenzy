/*
CHARLIB_ACTOR.H

charlib module. Field offsets of the CharLib actor (0x360 bytes, Actor.cpp) and of its
character data (0x204 bytes, players only). Actors are reached with the accessors of
charlib.h: CHL_F32(actor, ACT_POS + 4) is the actor's y.

Other modules may include this header. Only the charlib agent edits it.
*/

#ifndef OPENFRENZY_CHARLIB_ACTOR_H
#define OPENFRENZY_CHARLIB_ACTOR_H

/* ---------- actor (0x360 bytes) */

/* 0x000 the list node (next, prev): actors are in chl_actor_list */
/* 0x010 the bactor instance (BActorInstance, 0x170 bytes) */
#define ACT_INST(a) ((void *)((char *)(a) + 0x10))
#define ACT_INST_MATRIX 0x10       /* the instance matrix (16 floats) at the start of it */
#define ACT_NODE 0x144             /* BActorNode *: the instance's root node (instance + 0x134) */
#define ACT_POS 0xd0               /* float[4]: position (instance + 0xc0) */
#define ACT_ORIENT 0xf0            /* float[4]: orientation quaternion (instance + 0xe0) */
#define ACT_COLL_INST 0x180        /* BActorInstance *: collision mesh instance ("_coll.obe") */
#define ACT_SHADOW_INST 0x184      /* BActorInstance *: shadow mesh instance ("_shad.obe") */
#define ACT_BLOCK188 0x188         /* 0x40-byte block of player actors ([0] = 10.0) */
#define ACT_CHARDATA 0x18c         /* character data (players), see CD_* */
#define ACT_COLLMESH 0x190         /* collision mesh data (CRCollision.cpp, 0x2c bytes) */
#define ACT_VEL 0x194              /* float[4]: velocity per frame */
#define ACT_POS_PREV 0x1a4         /* float[4]: position at the end of the last update */
#define ACT_POS_PREMOVE 0x1b4      /* float[4]: position before the collision response */
#define ACT_HEADING_WANTED 0x1d8   /* float: heading the actor turns towards */
#define ACT_EULER 0x1e4            /* float[3]: rotation (x, heading, z) */
#define ACT_HEADING 0x1e8          /* float: heading (ACT_EULER + 4) */
#define ACT_TYPE 0x21c             /* int: 0 scenery, 1 player character, 2 static (not freed),
                                      5, 0xb, 0xe, 0xf ... */
#define ACT_FLAGS 0x220            /* unsigned int, see below */
#define ACT_ALPHA 0x224            /* int: 0x100 opaque */
#define ACT_ALPHA_TARGET 0x228     /* int: -1 none */
#define ACT_ALPHA_STEPS 0x22c      /* int: steps of a fade (0: 0x14) */
#define ACT_PLAYER 0x230           /* unsigned short: player slot */
#define ACT_ANIM_EVENT 0x232       /* unsigned short: an animation ended this frame */
#define ACT_ANIM_DONE 0x234        /* unsigned short: the animation ended (or is missing) */
#define ACT_ANIM_QUEUED 0x236      /* unsigned short: queued animation speeds */
#define ACT_JUMP_TIME 0x238        /* float: frames of reduced gravity left (-1: jump rising) */
#define ACT_ANIM_RATE 0x23c        /* float: animation speed */
#define ACT_ANIM_QUEUE 0x240       /* float[5]: speeds of the queued animations */
#define ACT_DEATH_TIME 0x258       /* float: frames until the actor fades out */
#define ACT_FRICTION 0x25c         /* float: ground friction per frame (type 0xb) */
#define ACT_FADE_SPEED 0x260       /* float: alpha lost per frame when dying */
#define ACT_ANIMS 0x264            /* BAnim *[]: the animation table */
#define ACT_NO_PREP 0x320          /* int: the orientation and matrices are set elsewhere */
#define ACT_ANIM_INFO 0x324        /* per animation 0x44 bytes; +0x40 float speed */
#define ACT_ANIMTEX 0x328          /* ClassLib list of animated textures (classlib_fx) */
#define ACT_FADE 0x334             /* float: animated-texture level */
#define ACT_FADE_TARGET 0x338      /* float */
#define ACT_FADE_RATE 0x33c        /* float: per frame (0: none) */
#define ACT_FOG_OWN 0x340          /* int: own fog; then start, end (floats), r, g, b (ints) */

/* ACT_FLAGS bits seen so far: 0x1 gravity, 0x2 orientation set directly, 0x18 player
(character data), 0x20 no collision mesh, 0x100 stopped by a platform, 0x200 dying
(fading out), 0x400 / 0x600 faded drawing, 0x1000 no friction, 0x8000 cleared each update,
0x20000 no fog, 0x80000 and 0x5 special draw state, 0x200000 unlit, 0x400000 has a
shadow mesh */

/* the character data of a player actor */
#define ACT_CD(a) CHL_PTR(a, ACT_CHARDATA)

/* ---------- character data (0x204 bytes) */

#define CD_STUN_TARGET 0x19c       /* float[3]: where a knock-back takes the actor */
#define CD_STUN_TIME 0x1ac         /* float: knock-back frames left */
#define CD_PLATFORM 0x1b0          /* the platform the actor stands on */
#define CD_PLATFORM_PREV 0x1b4     /* the platform of the last frame */
#define CD_PLATFORM_FRAMES 0x1b8   /* int: frames the platform's speed is still added */

#endif
