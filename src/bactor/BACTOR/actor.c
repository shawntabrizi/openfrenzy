/*
BACTOR/actor.c  <-  C:\Babel\Xbox\Src\BACTOR\actor.cpp

Actors: loading and fixing up .obe models, actor instances and their node
trees, keyframe animation (queues of animation slots per node), animation
events, and vertex extraction.

Only 0x000366f0 is tagged with actor.cpp in the XBE; the other functions sit
between the start of the BACTOR library and the lights.cpp tag and are
placed here by address order (file inferred).
*/

#include <stdio.h>

#include "bactor.h"
#include "bactor_extern.h"

/* 0x002855f0 (initialised data of another module): the identity matrix.
BACTOR only reads it, so it keeps its own copy. */
const float bactor_identity[16] = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
};

#define ALIGNED16(a, b) (((((uintptr_t)(a)) | ((uintptr_t)(b))) & 0xf) == 0)

/* 0x00034730  BACTOR/actor.cpp (file inferred) */
int baInitialise(void)
{
	/* FUN_000a6fa0 is a shared "return 1" function (identical-code folding) */
	if (FUN_000a6fa0() != 0)
	{
		if (baInitLights() != 0)
		{
			if (baInitSets() != 0)
			{
				if (baInitCameras() != 0)
				{
					baInitExtras();
					return 1;
				}
				/* the sets' shutdown function is empty (0x001608d0 -> 0x001d5950) */
			}
			/* the lights' shutdown function is empty (0x001608d0 -> 0x001d5950) */
		}
		/* an empty shutdown function (0x001d5950) */
	}
	return 0;
}

/* 0x00034780  BACTOR/actor.cpp (file inferred) */
void baShutdown(void)
{
	baDeleteAllExtras();
	baDeleteAllCameras();
	/* then three empty shutdown functions (0x001608d0, 0x001608d0, 0x001d5950) */
}

/* 0x000347a0  BACTOR/actor.cpp (file inferred)
number of nodes in a node definition tree (siblings and their children) */
int bCountNodeDefs(struct BActorNodeDef *first)
{
	struct BActorNodeDef *def = first;
	int count = 0;

	do
	{
		count++;
		if (def->child != NULL)
			count += bCountNodeDefs(def->child);
		def = def->next;
	} while (def != first);
	return count;
}

/* 0x000347e0  BACTOR/actor.cpp (file inferred)
builds the node instances of a node definition tree in the array at node;
returns the first free node after them */
struct BActorNode *bInitNodes(struct BActorNodeDef *first, struct BActorNode *node, struct BActorNode *parent,
	int numSlots)
{
	struct BActorNodeDef *def = first;
	struct BActorNode *firstNode = node;
	struct BActorNode *next;

	do
	{
		node->posTrack = &def->posTrack;
		node->scaleTrack = &def->scaleTrack;
		node->rotTrack = &def->rotTrack;
		node->parent = parent;
		node->slots = (struct BAnimSlot *)(node + 1);
		node->def = def;
		node->numSlots = (uint8_t)numSlots;
		memcpy(node->local, bactor_identity, sizeof(node->local));
		next = (struct BActorNode *)((char *)(node + 1) + numSlots * (int)sizeof(struct BAnimSlot));
		if (def->child == NULL)
		{
			node->child = NULL;
		}
		else
		{
			node->child = next;
			next = bInitNodes(def->child, next, node, numSlots);
		}
		if (def->next == first)
			node->next = firstNode;
		else
			node->next = next;
		node->next->prev = node;
		bGetKey3(node->pos, node->posTrack, 0);
		bGetKey3(node->scale, node->scaleTrack, 0);
		bGetKey4(node->rot, node->rotTrack, 0);
		memcpy(node->local, bactor_identity, sizeof(node->local));
		node->flags = BNODEF_POS | BNODEF_ROT | BNODEF_SCALE;
		node->events = def->events;
		node->numEvents = def->numEvents;
		def = def->next;
		node = next;
	} while (def != first);
	return node;
}

/* 0x00034930  BACTOR/actor.cpp (file inferred) */
void baDeleteActorInstance(struct BActorInstance *inst)
{
	ba_bkFree(inst->root);
	inst->root = NULL;
	if (inst->flags & 1)
	{
		ba_bkFree(inst);
		return;
	}
	if (inst->bones != NULL)
	{
		ba_bkFree(inst->bones);
		inst->bones = NULL;
	}
}

/* 0x00034990  BACTOR/actor.cpp (file inferred)
turns the file offsets of a loaded node definition tree into pointers */
void bFixupNodeDefs(char *base, struct BActorNodeDef *first, int depth, struct BActor *actor)
{
	struct BActorNodeDef *def = first;
	int i;

#define FIXUP(field) \
	if ((field) != 0) \
	(field) = (void *)((char *)(field) + (intptr_t)base)

	do
	{
		FIXUP(def->next);
		FIXUP(def->prev);
		FIXUP(def->parent);
		FIXUP(def->child);
		FIXUP(def->posTrack.keys);
		FIXUP(def->scaleTrack.keys);
		FIXUP(def->rotTrack.keys);
		FIXUP(def->events);
		for (i = 0; i < def->numEvents; i++)
		{
			def->events[i].target += (int)(intptr_t)actor->strings;
			def->events[i].message += (intptr_t)actor->strings;
		}
		if (def->type == BNODE_MESH)
		{
			bFixupMesh(base, def, actor);
			for (i = 0; i < 4; i++)
				FIXUP(((struct BVertexStreams *)((char *)def + 0xc0))->format[i]);
		}
		else if (def->type == BNODE_SET)
		{
			/* the set node's fixup function is empty (0x001d5950, called with def + 0x70 and base) */
			for (i = 0; i < 4; i++)
				FIXUP(((struct BVertexStreams *)((char *)def + 0x80))->format[i]);
		}
		if (def->child != NULL)
			bFixupNodeDefs(base, def->child, depth + 1, actor);
		def = def->next;
	} while (def != first);
#undef FIXUP
}

/* 0x00034b50  BACTOR/actor.cpp (file inferred)
bKernel's loader of actor resources (in its resource type table,
src/bkernel/bKernel/unplaced_resource.c): finds the actor crc in a package and fixes it up */
struct BActor *baLoadActor(void *package, uint32_t crc)
{
	struct BActor *actor = (struct BActor *)ba_bkFindResource(package, crc, NULL, NULL, NULL, 0);

	if (actor == NULL)
		return NULL;
	/* the package id (the package's first word), keeping the actor's bit 31 */
	actor->handle = (actor->handle & 0x80000000u) | (*(uint32_t *)package & 0x7fffffffu);
	if (actor->root != NULL)
		actor->root = (struct BActorNodeDef *)((char *)actor->root + (intptr_t)actor);
	if (actor->subObjects != NULL)
		actor->subObjects = (struct BActorSubObject *)((char *)actor->subObjects + (intptr_t)actor);
	if (actor->strings != NULL)
		actor->strings += (intptr_t)actor;
	bFixupNodeDefs((char *)actor, actor->root, 0, actor);
	if (actor->flags & 1)
		bFixupSoftSkin(actor);
	return actor;
}

/* 0x00034bf0  BACTOR/actor.cpp (file inferred)
computes the instance matrices; flags: 1 update the nodes, 4 test visibility */
void baPrepActorInstance(struct BActorInstance *inst, int flags)
{
	struct BActor *actor = inst->actor;
	float parentScale[4];
	float translation[16];
	float scaling[16];

	parentScale[0] = 1.0f;
	parentScale[1] = 1.0f;
	parentScale[2] = 1.0f;
	parentScale[3] = 0.0f;
	ba_MatTranslation(translation, inst->pos[0], inst->pos[1], inst->pos[2]);
	ba_MatScaling(scaling, inst->scale[0], inst->scale[1], inst->scale[2]);
	ba_MatFromQuat(inst->rotMatrix, inst->rot);
	if (ALIGNED16(inst, translation))
		ba_MatMulAligned(inst->rotScale, translation, inst->rotMatrix);
	else
		ba_MatMul(inst->rotScale, translation, inst->rotMatrix);
	if (ALIGNED16(inst, scaling))
		ba_MatMulAligned(inst->matrix, inst->rotScale, scaling);
	else
		ba_MatMul(inst->matrix, inst->rotScale, scaling);
	*(uint8_t *)&inst->flags &= ~2; /* the inverse is stale */
	if ((uint8_t)flags & 4)
	{
		inst->visible = (int16_t)(uint8_t)ba_bdBoxVisible(inst->matrix, actor->bboxMin[0], actor->bboxMin[1],
			actor->bboxMin[2], actor->bboxMax[0], actor->bboxMax[1], actor->bboxMax[2]);
	}
	if ((inst->visible != 0 || ((uint8_t)flags & 6) == 0) && ((uint8_t)flags & 1))
		bUpdateNodeMatrices(inst->root, bactor_identity, parentScale, inst);
}

/* 0x00034d50  BACTOR/actor.cpp (file inferred)
draws an actor instance; returns 0 when it is not visible */
int baDrawActorInstance(struct BActorInstance *inst, int passes)
{
	struct BActor *actor = inst->actor;
	int16_t visible;

	visible = (int16_t)(uint8_t)ba_bdBoxVisible(inst->matrix, actor->bboxMin[0], actor->bboxMin[1],
		actor->bboxMin[2], actor->bboxMax[0], actor->bboxMax[1], actor->bboxMax[2]);
	inst->visible = visible;
	if (visible == 0)
		return 0;
	if (actor->flags & 1)
	{
		bDrawSoftSkin(inst, passes);
		return 1;
	}
	bDrawNodes(inst->root, inst, passes);
	return 1;
}

/* 0x00034dd0  BACTOR/actor.cpp (file inferred) */
void bReleaseNodeDefs(struct BActorNodeDef *first)
{
	struct BActorNodeDef *def = first;

	do
	{
		if (def->type == BNODE_MESH)
			bReleaseMesh(def);
		/* BNODE_SET: an empty function (0x001d5950) */
		if (def->child != NULL)
			bReleaseNodeDefs(def->child);
		def = def->next;
	} while (def != first);
}

/* 0x00034e30  BACTOR/actor.cpp (file inferred)
bKernel's release function of actor resources (resource type 1) */
void baFreeActor(struct BActor *actor)
{
	bReleaseNodeDefs(actor->root);
	if (actor->flags & 1)
		bReleaseSoftSkin(actor);
	if ((int32_t)actor->handle >= 0)
		ba_bkFree(actor);
}

/* 0x00034e70  BACTOR/actor.cpp (file inferred)
sets node (or every root node when node is NULL) to a time of its tracks */
void baSetNodeTime(struct BActorInstance *inst, struct BActorNode *node, int time, int recurse)
{
	struct BActorNode *child;

	if (node == NULL)
	{
		node = inst->root;
		if (node != NULL)
		{
			do
			{
				baSetNodeTime(inst, node, time, recurse);
				node = node->next;
			} while (node != inst->root);
		}
		return;
	}
	node->time = time;
	node->blend = 0.0f;
	if (node->flags & BNODEF_POS)
	{
		bGetKey3(node->pos, node->posTrack, time);
		node->flags |= BNODEF_POS_SET;
	}
	if (node->flags & BNODEF_SCALE)
	{
		bGetKey3(node->scale, node->scaleTrack, time);
		node->flags |= BNODEF_SCALE_SET;
	}
	if (node->flags & BNODEF_ROT)
	{
		bGetKey4(node->rot, node->rotTrack, time);
		node->flags |= BNODEF_ROT_SET;
	}
	if (recurse != 0 && (child = node->child) != NULL)
	{
		do
		{
			baSetNodeTime(inst, child, time, 1);
			child = child->next;
		} while (child != node->child);
	}
}

/* 0x00034f60  BACTOR/actor.cpp (file inferred)
sets node to a blend of two times of its tracks */
void baSetNodeTimeBlend(struct BActorInstance *inst, struct BActorNode *node, int time, int time2, float t,
	int recurse)
{
	struct BActorNode *child;
	float a[4];
	float b[4];

	if (node == NULL)
	{
		node = inst->root;
		do
		{
			baSetNodeTimeBlend(inst, node, time, time2, t, recurse);
			node = node->next;
		} while (node != inst->root);
		return;
	}
	node->time = time2;
	node->blend = t;
	if (node->flags & BNODEF_POS)
	{
		bGetKey3(a, node->posTrack, time);
		bGetKey3(b, node->posTrack, time2);
		node->flags |= BNODEF_POS_SET;
		node->pos[0] = (b[0] - a[0]) * t + a[0];
		node->pos[1] = (b[1] - a[1]) * t + a[1];
		node->pos[2] = (b[2] - a[2]) * t + a[2];
	}
	if (node->flags & BNODEF_SCALE)
	{
		bGetKey3(a, node->scaleTrack, time);
		bGetKey3(b, node->scaleTrack, time2);
		node->flags |= BNODEF_SCALE_SET;
		node->scale[0] = (b[0] - a[0]) * t + a[0];
		node->scale[1] = (b[1] - a[1]) * t + a[1];
		node->scale[2] = (b[2] - a[2]) * t + a[2];
	}
	if (node->flags & BNODEF_ROT)
	{
		bGetKey4(a, node->rotTrack, time);
		bGetKey4(b, node->rotTrack, time2);
		ba_QuatSlerp(node->rot, a, b, t);
		node->flags |= BNODEF_ROT_SET;
	}
	if (recurse != 0 && (child = node->child) != NULL)
	{
		do
		{
			baSetNodeTimeBlend(inst, child, time, time2, t, 1);
			child = child->next;
		} while (child != node->child);
	}
}

/* 0x00035110  BACTOR/actor.cpp (file inferred)
sends the animation events of node whose frame lies between from and to
(in the direction dir); inclusive also sends the events at the from frame.
Original register arguments: ecx inclusive, edx to, esi node, eax anim. */
void bSendAnimEvents(int inclusive, int to, struct BActorInstance *inst, struct BActorNodeDef *def, int from,
	int dir, struct BActorNode *node, struct BAnim *anim)
{
	char text[256];
	struct BAnimEventDef *event;
	int fromFrame = (from + 0x200) >> 10;
	int toFrame = (to + 0x200) >> 10;
	int startFrame;
	int endFrame;
	int frame;
	int i;

	(void)def;
	if (toFrame == fromFrame && inclusive == 0)
		return;
	event = node->events;

#define SEND() \
	do \
	{ \
		sprintf(text, "%08x %08x %s", (unsigned int)(uintptr_t)inst, (unsigned int)(uintptr_t)node, event->message); \
		ba_bkSendMessage(event->target, text); \
	} while (0)

	if (dir < 0)
	{
		if (from < to)
		{
			/* wrapped backwards past the start */
			endFrame = anim->endTime >> 10;
			startFrame = anim->startTime >> 10;
			if (inclusive == 0)
			{
				for (i = 0; i < node->numEvents; i++, event++)
				{
					frame = event->frame;
					if (startFrame <= frame && frame <= endFrame && (frame < fromFrame || toFrame <= frame))
						SEND();
				}
			}
			else
			{
				for (i = 0; i < node->numEvents; i++, event++)
				{
					frame = event->frame;
					if (startFrame <= frame && frame <= endFrame && (frame <= fromFrame || toFrame <= frame))
						SEND();
				}
			}
		}
		else if (inclusive == 0)
		{
			for (i = 0; i < node->numEvents; i++, event++)
			{
				if (event->frame < fromFrame && toFrame <= event->frame)
					SEND();
			}
		}
		else
		{
			for (i = 0; i < node->numEvents; i++, event++)
			{
				if (event->frame <= fromFrame && toFrame <= event->frame)
					SEND();
			}
		}
	}
	else if (to < from)
	{
		/* wrapped forwards past the end */
		endFrame = anim->endTime >> 10;
		startFrame = anim->startTime >> 10;
		if (inclusive == 0)
		{
			for (i = 0; i < node->numEvents; i++, event++)
			{
				frame = event->frame;
				if (startFrame <= frame && frame <= endFrame && (fromFrame < frame || frame <= toFrame))
					SEND();
			}
		}
		else
		{
			for (i = 0; i < node->numEvents; i++, event++)
			{
				frame = event->frame;
				if (startFrame <= frame && frame <= endFrame && (fromFrame <= frame || frame <= toFrame))
					SEND();
			}
		}
	}
	else if (inclusive == 0)
	{
		for (i = 0; i < node->numEvents; i++, event++)
		{
			if (fromFrame < event->frame && event->frame <= toFrame)
				SEND();
		}
	}
	else
	{
		for (i = 0; i < node->numEvents; i++, event++)
		{
			if (fromFrame <= event->frame && event->frame <= toFrame)
				SEND();
		}
	}
#undef SEND
}

/* 0x000354e0  BACTOR/actor.cpp (file inferred)
puts an animation into the first free slot (from the current one) of node
and, without BNODEF_NOANIM, of its children */
void bQueueAnim(struct BActorInstance *inst, struct BActorNode *node, int mode, struct BAnim *anim,
	struct BAnim *anim2, int speed, int speed2, int time, int time2, float blendSpeed, uint32_t flags,
	BAnimCallback callback, void *context)
{
	struct BActorNode *child;
	struct BAnimSlot *slot;
	int numSlots;
	int index;
	int tries;
	int value;

	if ((flags & BANIMF_MOTION) == 0 || node != inst->motionNode)
		mode = mode & 0xff;
	else
		mode = BANIM_PLAY;
	if (mode == BANIM_PLAY || mode == BANIM_BLEND || mode == BANIM_BLENDIN)
	{
		numSlots = node->numSlots;
		index = node->curSlot;
		for (tries = 0; tries < numSlots; tries++)
		{
			slot = &node->slots[index];
			if (slot->active == 0)
			{
				slot->active = 1;
				slot->mode = mode;
				slot->anim = anim;
				if (mode == BANIM_BLEND)
					slot->anim2 = anim2;
				slot->time = time == -1 ? anim->startTime : time;
				if (mode == BANIM_BLEND)
					slot->time2 = time2 == -1 ? anim2->startTime : time2;
				value = speed;
				if (value == 0 && (value = anim->speed) == 0)
					value = 0x400;
				slot->speed = value;
				if (mode == BANIM_BLEND)
				{
					value = speed2;
					if (value == 0 && (value = anim2->speed) == 0)
						value = 0x400;
					slot->speed2 = value;
				}
				if (mode != BANIM_PLAY)
				{
					slot->blend = 0.0f;
					slot->blendSpeed = blendSpeed;
				}
				slot->flags = (uint16_t)flags | BANIMF_NEW;
				slot->callback = callback;
				slot->context = context;
				break;
			}
			index = (index + 1) % numSlots;
		}
	}
	child = node->child;
	if (child != NULL)
	{
		do
		{
			if ((child->flags & BNODEF_NOANIM) == 0)
				bQueueAnim(inst, child, mode, anim, anim2, speed, speed2, time, time2, blendSpeed,
					flags | BANIMF_CHILD, callback, context);
			child = child->next;
		} while (child != node->child);
	}
}

/* 0x00035770  BACTOR/actor.cpp (file inferred)
queues an animation on node (NULL: on every root node) */
void baQueueAnimation(struct BActorInstance *inst, struct BActorNode *node, struct BAnim *anim, int speed,
	int time, uint32_t flags, BAnimCallback callback, void *context)
{
	if (node == NULL)
	{
		node = inst->root;
		do
		{
			if ((node->flags & BNODEF_NOANIM) == 0)
				bQueueAnim(inst, node, BANIM_PLAY, anim, NULL, speed, 0, time, -1, 0.0f, flags, callback,
					context);
			node = node->next;
		} while (node != inst->root);
		return;
	}
	bQueueAnim(inst, node, BANIM_PLAY, anim, NULL, speed, 0, time, -1, 0.0f, flags, callback, context);
}

/* 0x00035810  BACTOR/actor.cpp (file inferred)
queues two animations played together and blended */
void baQueueBlendedAnimation(struct BActorInstance *inst, struct BActorNode *node, struct BAnim *anim,
	struct BAnim *anim2, int speed, int speed2, int time, int time2, float blendSpeed, uint32_t flags,
	BAnimCallback callback, void *context)
{
	if (node == NULL)
	{
		node = inst->root;
		do
		{
			if ((node->flags & BNODEF_NOANIM) == 0)
				bQueueAnim(inst, node, BANIM_BLEND, anim, anim2, speed, speed2, time, time2, blendSpeed, flags,
					callback, context);
			node = node->next;
		} while (node != inst->root);
		return;
	}
	bQueueAnim(inst, node, BANIM_BLEND, anim, anim2, speed, speed2, time, time2, blendSpeed, flags, callback,
		context);
}

/* 0x000358d0  BACTOR/actor.cpp (file inferred)
queues an animation that is blended in from the current pose */
void baQueueAnimationBlendIn(struct BActorInstance *inst, struct BActorNode *node, struct BAnim *anim, int speed,
	int time, float blendSpeed, uint32_t flags, BAnimCallback callback, void *context)
{
	if (node == NULL)
	{
		node = inst->root;
		do
		{
			if ((node->flags & BNODEF_NOANIM) == 0)
				bQueueAnim(inst, node, BANIM_BLENDIN, anim, NULL, speed, 0, time, -1, blendSpeed, flags,
					callback, context);
			node = node->next;
		} while (node != inst->root);
		return;
	}
	bQueueAnim(inst, node, BANIM_BLENDIN, anim, NULL, speed, 0, time, -1, blendSpeed, flags, callback, context);
}

/* 0x00035970  BACTOR/actor.cpp (file inferred)
queues a copy of the animation slots of source (blended in) */
void baQueueCopyAnimation(struct BActorInstance *inst, struct BActorNode *node, struct BActorNode *source,
	float blendSpeed, uint32_t flags, BAnimCallback callback, void *context)
{
	struct BActorNode *child;
	struct BAnimSlot *slot;
	int numSlots;
	int index;
	int tries;

	if (node == NULL)
	{
		node = inst->root;
		do
		{
			if ((node->flags & BNODEF_NOANIM) == 0)
				baQueueCopyAnimation(inst, node, source, blendSpeed, flags, callback, context);
			node = node->next;
		} while (node != inst->root);
		return;
	}
	numSlots = node->numSlots;
	index = node->curSlot;
	for (tries = 0; tries < numSlots; tries++)
	{
		slot = &node->slots[index];
		if (slot->active == 0)
		{
			slot->active = 1;
			slot->mode = BANIM_COPY;
			slot->source = source;
			slot->flags = (uint16_t)flags | BANIMF_NEW;
			slot->callback = callback;
			slot->context = context;
			slot->blend = 0.0f;
			slot->blendSpeed = blendSpeed;
			break;
		}
		index = (index + 1) % numSlots;
	}
	child = node->child;
	if (child != NULL)
	{
		do
		{
			if ((child->flags & BNODEF_NOANIM) == 0)
				baQueueCopyAnimation(inst, child, source, blendSpeed, flags | BANIMF_CHILD, callback, context);
			child = child->next;
		} while (child != node->child);
	}
}

/* 0x00035aa0  BACTOR/actor.cpp (file inferred)
points the nodes at the tracks and events of another node definition tree */
void bRebindNodes(struct BActorNodeDef *first, struct BActorNode *node)
{
	struct BActorNodeDef *def = first;

	do
	{
		node->posTrack = &def->posTrack;
		node->scaleTrack = &def->scaleTrack;
		node->rotTrack = &def->rotTrack;
		node->events = def->events;
		node->numEvents = def->numEvents;
		if (def->child != NULL)
			bRebindNodes(def->child, node->child);
		def = def->next;
		node = node->next;
	} while (def != first);
}

/* 0x00035b10  BACTOR/actor.cpp (file inferred)
makes an instance use the animations of another actor with the same nodes */
void baSetInstanceAnimations(struct BActor *actor, struct BActorInstance *inst)
{
	inst->subObjects = actor->subObjects;
	inst->numSubObjects = actor->numSubObjects;
	bRebindNodes(actor->root, inst->root);
}

/* 0x00035b40  BACTOR/actor.cpp (file inferred) */
struct BActorNode *baFindNodeByHash(struct BActorNode *first, uint32_t hash)
{
	struct BActorNode *node = first;
	struct BActorNode *found;

	for (;;)
	{
		if (node->def->nameHash == hash)
			return node;
		if (node->child != NULL && (found = baFindNodeByHash(node->child, hash)) != NULL)
			return found;
		node = node->next;
		if (node == first)
			return NULL;
	}
}

/* 0x00035b90  BACTOR/actor.cpp (file inferred) */
struct BActorNode *baFindNode(struct BActorNode *first, const char *name)
{
	return baFindNodeByHash(first, ba_bkHash(name));
}

/* 0x00035bb0  BACTOR/actor.cpp (file inferred) */
void baHideNode(struct BActorNode *node, int recurse)
{
	struct BActorNode *child = node->child;

	*(uint8_t *)&node->flags |= BNODEF_HIDDEN;
	if (child != NULL && recurse != 0)
	{
		do
		{
			baHideNode(child, 1);
			child = child->next;
		} while (child != node->child);
	}
}

/* 0x00035bf0  BACTOR/actor.cpp (file inferred) */
void baShowNode(struct BActorNode *node, int recurse)
{
	struct BActorNode *child = node->child;

	*(uint8_t *)&node->flags &= ~BNODEF_HIDDEN;
	if (child != NULL && recurse != 0)
	{
		do
		{
			baShowNode(child, 1);
			child = child->next;
		} while (child != node->child);
	}
}

/* 0x00035c30  BACTOR/actor.cpp (file inferred) */
struct BActorSubObject *baFindSubObjectByHash(struct BActorInstance *inst, uint32_t hash)
{
	unsigned int i;

	for (i = 0; i < (unsigned int)inst->numSubObjects; i++)
	{
		if (inst->subObjects[i].nameHash == hash)
			return &inst->subObjects[i];
	}
	return NULL;
}

/* 0x00035c70  BACTOR/actor.cpp (file inferred) */
struct BActorSubObject *baFindSubObject(struct BActorInstance *inst, const char *name)
{
	uint32_t hash = ba_bkHash(name);
	unsigned int i;

	for (i = 0; i < (unsigned int)inst->numSubObjects; i++)
	{
		if (inst->subObjects[i].nameHash == hash)
			return &inst->subObjects[i];
	}
	return NULL;
}

/* 0x00035cc0  BACTOR/actor.cpp (file inferred) */
void baSetRenderPass(struct BActorInstance *inst, int pass, int mode, int param)
{
	inst->passMode[pass] = mode;
	inst->passParam[pass] = param;
}

/* 0x00035ce0  BACTOR/actor.cpp (file inferred)
world position of a node (out: x, y, z, w) */
void baGetNodePosition(struct BActorInstance *inst, struct BActorNode *node, float *out)
{
	float matrix[16];

	out[2] = 0.0f;
	out[1] = 0.0f;
	out[0] = 0.0f;
	out[3] = 1.0f;
	if (ALIGNED16(inst, node))
		ba_MatMulAligned(matrix, inst->matrix, node->world);
	else
		ba_MatMul(matrix, inst->matrix, node->world);
	ba_MatTransformInPlace(matrix, out);
}

/* 0x00035d50  BACTOR/actor.cpp (file inferred) */
void baSetBrightness(int brightness)
{
	DAT_00284b0c = brightness;
	ba_bdSetBrightness(brightness);
}

/* 0x00035d70  BACTOR/actor.cpp (file inferred)
changes the current animation slot of node: mask 1 speed, 2 speed2,
4 blend speed, 8 blend */
void baChangeAnimation(struct BActorNode *node, int recurse, int speed, int speed2, float blendSpeed, float blend,
	uint32_t mask)
{
	struct BAnimSlot *slot = &node->slots[node->curSlot];
	struct BActorNode *child;
	int value;

	if (slot->active != 0)
	{
		if (slot->mode != BANIM_PLAY)
		{
			if (slot->mode == BANIM_BLEND)
			{
				if (mask & 2)
				{
					value = speed2;
					if (value == 0 && (value = slot->anim2->speed) == 0)
						value = 0x400;
					slot->speed2 = value;
				}
			}
			else if (slot->mode != BANIM_BLENDIN)
			{
				goto children;
			}
			if (mask & 4)
				slot->blendSpeed = blendSpeed;
			if (mask & 8)
				slot->blend = blend;
		}
		if (mask & 1)
		{
			value = speed;
			if (value == 0 && (value = slot->anim->speed) == 0)
				value = 0x400;
			slot->speed = value;
		}
	}
children:
	if (recurse != 0 && (child = node->child) != NULL)
	{
		do
		{
			baChangeAnimation(child, recurse, speed, speed2, blendSpeed, blend, mask);
			child = child->next;
		} while (child != node->child);
	}
}

/* 0x00035e60  BACTOR/actor.cpp (file inferred)
adds up the vertices and indices of a vertex stream in a node tree;
returns the stream description of the last node that has one */
struct BVertexStreams *bCountVertices(struct BActorInstance *inst, struct BActorNode *node, int stream,
	int *numVerts, int *numIndices)
{
	struct BActorNodeDef *def = node->def;
	struct BVertexStreams *streams = NULL;
	struct BVertexStreams *found;
	struct BActorNode *child;

	if (def->type == BNODE_MESH)
		streams = (struct BVertexStreams *)((char *)def + 0xc0);
	else if (def->type == BNODE_SET)
		streams = (struct BVertexStreams *)((char *)def + 0x80);
	else
		goto children;
	if (streams != NULL && streams->format[stream] != NULL)
	{
		*numVerts += streams->numVerts;
		*numIndices += streams->numIndices;
	}
children:
	child = node->child;
	if (child != NULL)
	{
		do
		{
			found = bCountVertices(inst, child, stream, numVerts, numIndices);
			if (found != NULL)
				streams = found;
			child = child->next;
		} while (child != node->child);
	}
	return streams;
}

/* 0x00035f00  BACTOR/actor.cpp (file inferred)
copies the vertices of a vertex stream of a node tree into out, transformed by
the node matrices (flag 0x10000: and by the instance matrix); index elements
are offset by *base. Returns the end of the data written. */
char *bExtractVertices(struct BActorInstance *inst, struct BActorNode *node, int stream, char *out, int *base,
	uint32_t flags)
{
	struct BActorNodeDef *def = node->def;
	struct BActorNode *child;
	struct BVertexStreams *streams;
	struct BVertexFormat *format;
	uint8_t *src;
	float m[16];
	float x, y, z;
	float value;
	int i;
	int e;
	/* the element values of the current vertex: position and normal (per
	vertex), or three positions, a face normal and three normals (per index) */
	float pos[3] = { 0 }, nrm[3] = { 0 };
	float p0[3] = { 0 }, p1[3] = { 0 }, p2[3] = { 0 };
	float n0[3] = { 0 }, n1[3] = { 0 }, n2[3] = { 0 };

	if (def->type == BNODE_MESH)
		streams = (struct BVertexStreams *)((char *)def + 0xc0);
	else if (def->type == BNODE_SET)
		streams = (struct BVertexStreams *)((char *)def + 0x80);
	else
		goto children;
	if (streams == NULL || (format = streams->format[stream]) == NULL)
		goto children;

	src = (uint8_t *)format + 0x54;
	if ((flags & 0x10000) == 0)
		memcpy(m, node->world, sizeof(m));
	else if (((uintptr_t)inst | (uintptr_t)node | (uintptr_t)m) & 0xf)
		ba_MatMul(m, inst->matrix, node->world);
	else
		ba_MatMulAligned(m, inst->matrix, node->world);

#define XFORM_POINT(dst, off) \
	do \
	{ \
		x = *(float *)(src + (off)[0]); \
		y = *(float *)(src + (off)[1]); \
		z = *(float *)(src + (off)[2]); \
		(dst)[0] = m[0] * x + m[4] * y + m[8] * z + m[12]; \
		(dst)[1] = m[1] * x + m[5] * y + m[9] * z + m[13]; \
		(dst)[2] = m[2] * x + m[6] * y + m[10] * z + m[14]; \
	} while (0)
#define XFORM_VECTOR(dst, off) \
	do \
	{ \
		x = *(float *)(src + (off)[0]); \
		y = *(float *)(src + (off)[1]); \
		z = *(float *)(src + (off)[2]); \
		(dst)[0] = m[0] * x + m[4] * y + m[8] * z; \
		(dst)[1] = m[1] * x + m[5] * y + m[9] * z; \
		(dst)[2] = m[2] * x + m[6] * y + m[10] * z; \
	} while (0)

	if (format->indexed == 0)
	{
		for (i = 0; i < streams->numVerts; i++)
		{
			if (format->posOffset[0] != 0xff)
				XFORM_POINT(pos, format->posOffset);
			if (format->normalOffset[0] != 0xff)
				XFORM_VECTOR(nrm, format->normalOffset);
			for (e = 0; e < format->numElements; e++)
			{
				switch (format->elementType[e])
				{
				case 1: value = pos[0]; break;
				case 2: value = pos[1]; break;
				case 3: value = pos[2]; break;
				case 4: value = nrm[0]; break;
				case 5: value = nrm[1]; break;
				case 6: value = nrm[2]; break;
				default: continue;
				}
				*(float *)(out + format->elementOffset[e]) = value;
			}
			out += format->stride;
			src += format->stride;
		}
	}
	else
	{
		const uint8_t *o = format->offsets3e; /* 0x3e: face normal, 0x41-0x47: positions, 0x4a-0x50: normals */

		for (i = 0; i < streams->numIndices; i++)
		{
			if (o[0x41 - 0x3e] != 0xff)
				XFORM_POINT(p0, o + (0x41 - 0x3e));
			if (o[0x44 - 0x3e] != 0xff)
				XFORM_POINT(p1, o + (0x44 - 0x3e));
			if (o[0x47 - 0x3e] != 0xff)
				XFORM_POINT(p2, o + (0x47 - 0x3e));
			if (o[0] != 0xff)
				XFORM_VECTOR(nrm, o);
			if (o[0x4a - 0x3e] != 0xff)
				XFORM_VECTOR(n0, o + (0x4a - 0x3e));
			if (o[0x4d - 0x3e] != 0xff)
				XFORM_VECTOR(n1, o + (0x4d - 0x3e));
			if (o[0x50 - 0x3e] != 0xff)
				XFORM_VECTOR(n2, o + (0x50 - 0x3e));
			for (e = 0; e < format->numElements; e++)
			{
				switch (format->elementType[e])
				{
				case 7:
				case 8:
				case 9:
					*(int16_t *)(out + format->elementOffset[e]) =
						(int16_t)(*(int16_t *)(src + format->elementOffset[e]) + (int16_t)*base);
					continue;
				case 10: value = nrm[0]; break;
				case 11: value = nrm[1]; break;
				case 12: value = nrm[2]; break;
				case 13: value = p0[0]; break;
				case 14: value = p0[1]; break;
				case 15: value = p0[2]; break;
				case 16: value = p1[0]; break;
				case 17: value = p1[1]; break;
				case 18: value = p1[2]; break;
				case 19: value = p2[0]; break;
				case 20: value = p2[1]; break;
				case 21: value = p2[2]; break;
				case 22: value = n0[0]; break;
				case 23: value = n0[1]; break;
				case 24: value = n0[2]; break;
				case 25: value = n1[0]; break;
				case 26: value = n1[1]; break;
				case 27: value = n1[2]; break;
				case 28: value = n2[0]; break;
				case 29: value = n2[1]; break;
				case 30: value = n2[2]; break;
				default: continue;
				}
				*(float *)(out + format->elementOffset[e]) = value;
			}
			out += format->stride;
			src += format->stride;
		}
	}
	*base += streams->numVerts;
#undef XFORM_POINT
#undef XFORM_VECTOR

children:
	child = node->child;
	if (child != NULL)
	{
		do
		{
			out = bExtractVertices(inst, child, stream, out, base, flags);
			child = child->next;
		} while (child != node->child);
	}
	return out;
}

/* 0x000366f0  BACTOR/actor.cpp
gets the vertices of a vertex stream of node (NULL: of the whole instance).
flags: 1 only count, 2 clear the buffer first, 0x10000 world space.
Returns the buffer (allocated when buffer is NULL), or NULL. */
void *baGetActorVertices(struct BActorInstance *inst, struct BActorNode *node, int stream, int *numVerts,
	int *numIndices, unsigned int *size, void *buffer, uint32_t flags)
{
	struct BVertexStreams *streams;
	struct BActorNode *root;
	unsigned int bytes = 0;
	int verts = 0;
	int indices = 0;
	int base;
	char *out;

	if (node == NULL)
	{
		root = inst->root;
		do
		{
			streams = bCountVertices(inst, root, stream, &verts, &indices);
			root = root->next;
		} while (root != inst->root);
	}
	else
	{
		streams = bCountVertices(inst, node, stream, &verts, &indices);
	}
	if (streams != NULL)
	{
		struct BVertexFormat *format = streams->format[stream];

		bytes = format->stride;
		if (format->indexed == 0)
			bytes *= verts;
		else
			bytes *= indices;
	}
	if (numVerts != NULL)
		*numVerts = verts;
	if (numIndices != NULL)
		*numIndices = indices;
	if (size != NULL)
		*size = bytes;
	if (streams == NULL || (flags & 1) != 0)
		return NULL;
	if (buffer == NULL && (buffer = ba_bkAlloc(bytes, "C:\\Babel\\Xbox\\Src\\BACTOR\\actor.cpp", 0xd1e, 0x2002)) == NULL)
		return NULL;
	if (flags & 2)
		memset(buffer, 0, bytes);
	if (node != NULL)
	{
		/* the original passes the address of its numVerts argument slot, which still holds the
		numVerts pointer: the index base is that pointer's value */
		base = (int)(intptr_t)numVerts;
		bExtractVertices(inst, node, stream, buffer, &base, flags);
		return buffer;
	}
	base = 0;
	out = buffer;
	root = inst->root;
	do
	{
		out = bExtractVertices(inst, root, stream, out, &base, flags);
		root = root->next;
	} while (root != inst->root);
	return buffer;
}

/* 0x00036870  BACTOR/actor.cpp (file inferred)
whether node (NULL: any root node) or, with recurse, a child plays an
animation; *anim receives the current one */
int baIsAnimating(struct BActorInstance *inst, struct BActorNode *node, int recurse, struct BAnim **anim)
{
	struct BAnimSlot *slot;
	struct BActorNode *child;

	if (node == NULL)
	{
		node = inst->root;
		while (baIsAnimating(inst, node, recurse, anim) == 0)
		{
			node = node->next;
			if (node == inst->root)
				return 0;
		}
		return 1;
	}
	slot = &node->slots[node->curSlot];
	if (slot->active == 0)
	{
		if (recurse != 0 && (child = node->child) != NULL)
		{
			do
			{
				if (baIsAnimating(inst, child, 1, anim) != 0)
					return 1;
				child = child->next;
			} while (child != node->child);
		}
		return 0;
	}
	if (anim != NULL)
	{
		if (slot->mode == BANIM_BLEND)
		{
			*anim = slot->anim2;
			return 1;
		}
		*anim = slot->anim;
	}
	return 1;
}

/* 0x00036940  BACTOR/actor.cpp (file inferred)
copies the animation state of the nodes of one instance to another */
void baCopyAnimationState(struct BActorInstance *dstInst, struct BActorNode *dst, struct BActorInstance *srcInst,
	struct BActorNode *src, int recurse)
{
	struct BActorNode *srcChild;
	struct BActorNode *dstChild;

	if (src == NULL)
	{
		src = srcInst->root;
		dst = dstInst->root;
		do
		{
			baCopyAnimationState(dstInst, dst, srcInst, src, recurse);
			src = src->next;
			dst = dst->next;
		} while (src != srcInst->root);
		return;
	}
	memcpy(dst->slots, src->slots, src->numSlots * sizeof(struct BAnimSlot));
	dst->curSlot = src->curSlot;
	memcpy(dst->pos, src->pos, 0x30); /* pos, scale, rot */
	memcpy(dst->prevPos, src->prevPos, 0x30);
	dst->blend = src->blend;
	dst->time = src->time;
	if (recurse != 0 && (srcChild = src->child) != NULL)
	{
		dstChild = dst->child;
		do
		{
			baCopyAnimationState(dstInst, dstChild, srcInst, srcChild, 1);
			srcChild = srcChild->next;
			dstChild = dstChild->next;
		} while (srcChild != src->child);
	}
}

/* 0x00036a20  BACTOR/actor.cpp (file inferred)
sets the extra local rotation of a node from a quaternion given in the
node's bind space */
void baSetNodeRotation(struct BActorNode *node, const float *quat)
{
	float axisAngle[4];
	float result[4];
	float bind[16];
	float angle;

	memcpy(bind, node->def->data.boneMatrix, sizeof(bind));
	bind[12] = 0.0f;
	bind[13] = 0.0f;
	bind[14] = 0.0f;
	ba_QuatToAxisAngle(axisAngle, quat);
	angle = axisAngle[3];
	axisAngle[3] = 1.0f;
	ba_MatTransformInPlace(bind, axisAngle);
	axisAngle[3] = angle;
	ba_QuatFromAxisAngle(result, axisAngle);
	ba_MatFromQuat(node->local, result);
}

/* 0x00036ac0  BACTOR/actor.cpp (file inferred) */
void bVec3Cross(float *out, const float *a, const float *b)
{
	out[0] = b[2] * a[1] - a[2] * b[1];
	out[1] = a[2] * b[0] - b[2] * a[0];
	out[2] = a[0] * b[1] - b[0] * a[1];
}

/* 0x00036b00  BACTOR/actor.cpp (file inferred) */
float bVec3Dot(const float *a, const float *b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

/* 0x00036b20  BACTOR/actor.cpp (file inferred)
initialises an actor instance (allocated when inst is NULL) with numSlots
animation slots per node; file and line name the caller for the allocator */
struct BActorInstance *baInitActorInstance(struct BActorInstance *inst, struct BActor *actor, int numSlots,
	const char *file, int line)
{
	int numNodes;
	struct BActorNode *nodes;
	int i;

	if (inst == NULL)
	{
		inst = ba_bkAlloc(actor->numBones * 0x40 + 0x170, file, line, 0x2002);
		if (inst == NULL)
			return NULL;
		inst->flags = 1;
		if (actor->numBones == 0)
			inst->bones = NULL;
		else
			inst->bones = (float *)(inst + 1);
	}
	else
	{
		inst->flags = 0;
		if (actor->numBones == 0)
		{
			inst->bones = NULL;
		}
		else
		{
			inst->bones = ba_bkAlloc(actor->numBones << 6, file, line, 0x2002);
			if (inst->bones == NULL)
				return NULL;
		}
	}
	inst->pos[2] = 0.0f;
	inst->pos[1] = 0.0f;
	inst->pos[0] = 0.0f;
	inst->scale[2] = 1.0f;
	inst->scale[1] = 1.0f;
	inst->scale[0] = 1.0f;
	inst->rot[2] = 0.0f;
	inst->rot[1] = 0.0f;
	inst->rot[0] = 0.0f;
	inst->rot[3] = 1.0f;
	inst->actor = actor;
	numNodes = bCountNodeDefs(actor->root);
	nodes = ba_bkAllocFill((numSlots * (int)sizeof(struct BAnimSlot) + (int)sizeof(struct BActorNode)) * numNodes, 0,
		file, line, 0x2002);
	inst->root = nodes;
	bInitNodes(actor->root, nodes, NULL, numSlots);
	inst->motionNode = baFindNodeByHash(inst->root, ba_bkHash("motion"));
	inst->passMode[0] = BPASS_NORMAL;
	inst->passParam[0] = 0;
	for (i = 1; i < 3; i++)
	{
		inst->passMode[i] = BPASS_OFF;
		inst->passParam[i] = 0;
	}
	inst->subObjects = actor->subObjects;
	inst->numSubObjects = actor->numSubObjects;
	return inst;
}

/* 0x00036cb0  BACTOR/actor.cpp (file inferred)
blends node from its current pose (saved as the previous pose) towards a
time of its tracks */
void bBlendNodeToTime(struct BActorInstance *inst, struct BActorNode *node, int time, float t, int recurse)
{
	struct BActorNode *child;
	float key[4];

	if (node == NULL)
	{
		node = inst->root;
		do
		{
			bBlendNodeToTime(inst, node, time, t, recurse);
			node = node->next;
		} while (node != inst->root);
		return;
	}
	node->time = time;
	node->blend = t;
	memcpy(node->prevPos, node->pos, sizeof(node->prevPos));
	memcpy(node->prevScale, node->scale, sizeof(node->prevScale));
	memcpy(node->prevRot, node->rot, sizeof(node->prevRot));
	if (node->flags & BNODEF_POS)
	{
		bGetKey3(key, node->posTrack, node->time);
		node->flags |= BNODEF_POS_SET;
		node->pos[0] = (key[0] - node->prevPos[0]) * t + node->prevPos[0];
		node->pos[1] = (key[1] - node->prevPos[1]) * t + node->prevPos[1];
		node->pos[2] = (key[2] - node->prevPos[2]) * t + node->prevPos[2];
	}
	if (node->flags & BNODEF_SCALE)
	{
		bGetKey3(key, node->scaleTrack, node->time);
		node->flags |= BNODEF_SCALE_SET;
		node->scale[0] = (key[0] - node->prevScale[0]) * t + node->prevScale[0];
		node->scale[1] = (key[1] - node->prevScale[1]) * t + node->prevScale[1];
		node->scale[2] = (key[2] - node->prevScale[2]) * t + node->prevScale[2];
	}
	if (node->flags & BNODEF_ROT)
	{
		bGetKey4(key, node->rotTrack, node->time);
		ba_QuatSlerp(node->rot, node->prevRot, key, t);
		node->flags |= BNODEF_ROT_SET;
	}
	if (recurse != 0 && (child = node->child) != NULL)
	{
		do
		{
			bBlendNodeToTime(inst, child, time, t, 1);
			child = child->next;
		} while (child != node->child);
	}
}

/* 0x00036e70  BACTOR/actor.cpp (file inferred)
advances the animation slots of a node list (and their children) by dt;
the motion node's movement is added to rootMotion (rotated and scaled into
the instance's space). Finished animations with a callback are reported
in events. Returns the number of events. */
int bUpdateNodes(struct BActorInstance *inst, struct BActorNode *first, float dt, float *rootMotion,
	struct BAnimEvent *events, int numEvents, int *numActive)
{
	struct BActorNode *node = first;
	struct BActorNodeDef *def;
	struct BAnimSlot *slot;
	struct BAnimEvent *event;
	float remaining;
	float before[3];
	float a[4];
	float b[4];
	float blendBefore;
	float blend;
	uint16_t flags;
	int oldTime;
	int oldTime2;
	int delta;
	int delta2;
	int start;
	int end;
	int t;

#define IS_MOTION() (node == inst->motionNode && rootMotion != NULL)
	/* reports the end of the slot's animation to its callback */
#define REPORT() \
	do \
	{ \
		if (slot->callback != NULL && (slot->flags & BANIMF_CHILD) == 0 && numEvents < 8) \
		{ \
			event = &events[numEvents]; \
			event->reason = 0; \
			event->instance = inst; \
			event->node = node; \
			event->anim = slot->anim; \
			event->context = slot->context; \
			numEvents++; \
			event->callback = slot->callback; \
		} \
	} while (0)
#define NEXT_SLOT() \
	do \
	{ \
		slot->active = 0; \
		node->curSlot = (uint8_t)((node->curSlot + 1) % node->numSlots); \
	} while (0)
#define CURRENT_ACTIVE() (node->slots[node->curSlot].active != 0)
#define ADD_MOTION(p) \
	do \
	{ \
		rootMotion[0] = ((p)[0] - node->pos[0]) + rootMotion[0]; \
		rootMotion[1] = ((p)[1] - node->pos[1]) + rootMotion[1]; \
		rootMotion[2] = ((p)[2] - node->pos[2]) + rootMotion[2]; \
	} while (0)

	do
	{
		def = node->def;
		remaining = dt;
		if (node == inst->motionNode && rootMotion != NULL)
		{
			rootMotion[0] = 0.0f;
			rootMotion[1] = 0.0f;
			rootMotion[2] = 0.0f;
			rootMotion[3] = 1.0f;
		}
		node->flags &= ~(BNODEF_POS_SET | BNODEF_ROT_SET | BNODEF_SCALE_SET);

	next_slot:
		if (!CURRENT_ACTIVE())
			goto node_done;
		(*numActive)++;
		slot = &node->slots[node->curSlot];
		flags = slot->flags;
		oldTime = slot->time;
		oldTime2 = slot->time2;
		blendBefore = slot->blend;
		slot->flags = flags & ~BANIMF_NEW;

		switch (slot->mode)
		{
		case BANIM_PLAY:
			if (node == inst->motionNode && (flags & BANIMF_NEW) != 0)
				bGetKey3(node->pos, node->posTrack, slot->time);
			delta = (int)((float)slot->speed * remaining);
			t = slot->time + delta;
			slot->time = t;
			end = slot->anim->endTime;
			if ((slot->flags & BANIMF_LOOP) == 0)
			{
				if (end < t)
				{
					if (node->numEvents != 0)
						bSendAnimEvents(flags & BANIMF_NEW, slot->anim->endTime, inst, def, oldTime, 1, node,
							slot->anim);
					remaining = remaining - (float)(slot->anim->endTime - oldTime) / (float)(slot->time - oldTime) * dt;
					REPORT();
					NEXT_SLOT();
					if (CURRENT_ACTIVE())
						goto next_slot;
					slot->time = slot->anim->endTime;
				}
				if (slot->time < slot->anim->startTime)
				{
					if (node->numEvents != 0)
						bSendAnimEvents(flags & BANIMF_NEW, slot->anim->startTime, inst, def, oldTime, -1, node,
							slot->anim);
					remaining = remaining - (float)(oldTime - slot->anim->endTime) / (float)(oldTime - slot->time) * dt;
					REPORT();
					NEXT_SLOT();
					if (CURRENT_ACTIVE())
						goto next_slot;
					slot->time = slot->anim->startTime;
				}
			}
			else
			{
				if (end < t)
				{
					slot->time = (t - end) % ((end - slot->anim->startTime) + 1) + slot->anim->startTime;
					if (IS_MOTION())
					{
						bGetKey3(a, node->posTrack, slot->anim->endTime);
						ADD_MOTION(a);
						bGetKey3(node->pos, node->posTrack, slot->anim->startTime);
					}
				}
				start = slot->anim->startTime;
				if (slot->time < start)
				{
					end = slot->anim->endTime;
					slot->time = end - (start - slot->time) % ((end - start) + 1);
					if (IS_MOTION())
					{
						bGetKey3(a, node->posTrack, slot->anim->startTime);
						ADD_MOTION(a);
						bGetKey3(node->pos, node->posTrack, slot->anim->endTime);
					}
				}
			}
			if (node->numEvents != 0)
				bSendAnimEvents(flags & BANIMF_NEW, slot->time, inst, def, oldTime, delta, node, slot->anim);
			if (!IS_MOTION())
			{
				baSetNodeTime(inst, node, slot->time, 0);
				goto node_done;
			}
			before[0] = node->pos[0];
			before[1] = node->pos[1];
			before[2] = node->pos[2];
			baSetNodeTime(inst, node, slot->time, 0);
			goto add_movement;

		case BANIM_BLEND:
			if (node == inst->motionNode && (flags & BANIMF_NEW) != 0)
				bGetKey3(node->pos, node->posTrack, slot->time2);
			blend = remaining * slot->blendSpeed + slot->blend;
			slot->blend = blend;
			if (blend > 1.0f)
			{
				remaining = remaining - (1.0f - blendBefore) / (blend - blendBefore) * dt;
				if ((slot->flags & BANIMF_CHAIN) == 0)
				{
					REPORT();
					NEXT_SLOT();
				}
				else
				{
					/* continue with the second animation alone */
					slot->mode = BANIM_PLAY;
					slot->anim = slot->anim2;
					slot->time = slot->time2;
					slot->speed = slot->speed2;
				}
				goto next_slot;
			}
			if (blend < 0.0f)
			{
				remaining = remaining - blendBefore / (blendBefore - blend) * dt;
				if ((slot->flags & BANIMF_CHAIN) == 0)
				{
					REPORT();
					NEXT_SLOT();
				}
				else
				{
					slot->mode = BANIM_PLAY;
				}
				goto next_slot;
			}
			delta = (int)((float)slot->speed * remaining);
			slot->time += delta;
			delta2 = (int)((float)slot->speed2 * remaining);
			slot->time2 += delta2;
			if ((slot->flags & BANIMF_LOOP) == 0)
			{
				if (slot->anim->endTime < slot->time)
				{
					remaining = remaining - (float)(slot->anim->endTime - oldTime) / (float)(slot->time - oldTime) * dt;
					REPORT();
					NEXT_SLOT();
					if (CURRENT_ACTIVE())
						goto next_slot;
					slot->time = slot->anim->endTime;
				}
				if (slot->time < slot->anim->startTime)
				{
					remaining = remaining - (float)(oldTime - slot->anim->endTime) / (float)(oldTime - slot->time) * dt;
					REPORT();
					NEXT_SLOT();
					if (CURRENT_ACTIVE())
						goto next_slot;
					slot->time = slot->anim->startTime;
				}
				if (slot->anim2->endTime < slot->time2)
				{
					if (node->numEvents != 0)
						bSendAnimEvents(flags & BANIMF_NEW, slot->anim2->endTime, inst, def, oldTime2, 1, node,
							slot->anim2);
					remaining = remaining - (float)(slot->anim2->endTime - oldTime2) / (float)(slot->time2 - oldTime2) * dt;
					REPORT();
					NEXT_SLOT();
					if (CURRENT_ACTIVE())
						goto next_slot;
					slot->time2 = slot->anim2->endTime;
				}
				if (slot->time2 < slot->anim2->startTime)
				{
					if (node->numEvents != 0)
						bSendAnimEvents(flags & BANIMF_NEW, slot->anim2->startTime, inst, def, oldTime2, -1, node,
							slot->anim2);
					remaining = remaining - (float)(oldTime2 - slot->anim2->endTime) / (float)(oldTime2 - slot->time2) * dt;
					REPORT();
					NEXT_SLOT();
					if (CURRENT_ACTIVE())
						goto next_slot;
					slot->time2 = slot->anim2->startTime;
				}
			}
			else
			{
				end = slot->anim->endTime;
				if (end < slot->time)
				{
					start = slot->anim->startTime;
					slot->time = (slot->time - end) % ((end - start) + 1) + start;
				}
				start = slot->anim->startTime;
				if (slot->time < start)
				{
					end = slot->anim->endTime;
					slot->time = end - (start - slot->time) % ((end - start) + 1);
				}
				end = slot->anim2->endTime;
				if (end < slot->time2)
				{
					start = slot->anim2->startTime;
					slot->time2 = (slot->time2 - end) % ((end - start) + 1) + start;
					if (IS_MOTION())
					{
						bGetKey3(a, node->posTrack, slot->anim->endTime);
						bGetKey3(b, node->posTrack, slot->anim2->endTime);
						a[0] = (b[0] - a[0]) * slot->blend + a[0];
						a[1] = (b[1] - a[1]) * slot->blend + a[1];
						a[2] = (b[2] - a[2]) * slot->blend + a[2];
						ADD_MOTION(a);
						bGetKey3(node->pos, node->posTrack, slot->anim2->startTime);
					}
				}
				start = slot->anim2->startTime;
				if (slot->time2 < start)
				{
					end = slot->anim2->endTime;
					slot->time2 = end - (start - slot->time2) % ((end - start) + 1);
					if (IS_MOTION())
					{
						bGetKey3(a, node->posTrack, slot->anim->startTime);
						bGetKey3(b, node->posTrack, slot->anim2->endTime);
						a[0] = (b[0] - a[0]) * slot->blend + a[0];
						a[1] = (b[1] - a[1]) * slot->blend + a[1];
						a[2] = (b[2] - a[2]) * slot->blend + a[2];
						ADD_MOTION(a);
						bGetKey3(node->pos, node->posTrack, slot->anim2->endTime);
					}
				}
			}
			if (node->numEvents != 0)
				bSendAnimEvents(flags & BANIMF_NEW, slot->time2, inst, def, oldTime2, delta2, node, slot->anim2);
			if (!IS_MOTION())
			{
				baSetNodeTimeBlend(inst, node, slot->time, slot->time2, slot->blend, 0);
				goto node_done;
			}
			before[0] = node->pos[0];
			before[1] = node->pos[1];
			before[2] = node->pos[2];
			baSetNodeTimeBlend(inst, node, slot->time, slot->time2, slot->blend, 0);
			goto add_movement;

		case BANIM_BLENDIN:
			if (node == inst->motionNode && (flags & BANIMF_NEW) != 0)
				bGetKey3(node->pos, node->posTrack, slot->time2);
			blend = remaining * slot->blendSpeed + slot->blend;
			slot->blend = blend;
			if (blend > 1.0f)
			{
				slot->mode = BANIM_PLAY;
				remaining = remaining - (1.0f - blendBefore) / (blend - blendBefore) * dt;
				goto next_slot;
			}
			if (blend < 0.0f)
			{
				slot->mode = BANIM_PLAY;
				remaining = remaining - blendBefore / (blendBefore - blend) * dt;
				goto next_slot;
			}
			delta = (int)((float)slot->speed * remaining);
			t = slot->time + delta;
			slot->time = t;
			end = slot->anim->endTime;
			if ((slot->flags & BANIMF_LOOP) == 0)
			{
				if (end < t)
				{
					if (node->numEvents != 0)
						bSendAnimEvents(flags & BANIMF_NEW, slot->anim->endTime, inst, def, oldTime, 1, node,
							slot->anim);
					remaining = remaining - (float)(slot->anim->endTime - oldTime) / (float)(slot->time - oldTime) * dt;
					REPORT();
					NEXT_SLOT();
					if (CURRENT_ACTIVE())
						goto next_slot;
					slot->time = slot->anim->endTime;
				}
				if (slot->time < slot->anim->startTime)
				{
					if (node->numEvents != 0)
						bSendAnimEvents(flags & BANIMF_NEW, slot->anim->startTime, inst, def, oldTime, -1, node,
							slot->anim);
					remaining = remaining - (float)(oldTime - slot->anim->startTime) / (float)(oldTime - slot->time) * dt;
					REPORT();
					NEXT_SLOT();
					if (CURRENT_ACTIVE())
						goto next_slot;
					slot->time = slot->anim->startTime;
				}
			}
			else
			{
				if (end < t)
				{
					slot->time = (t - end) % ((end - slot->anim->startTime) + 1) + slot->anim->startTime;
					if (IS_MOTION())
					{
						bGetKey3(a, node->posTrack, slot->anim->endTime);
						ADD_MOTION(a);
						bGetKey3(node->pos, node->posTrack, slot->anim->startTime);
					}
				}
				start = slot->anim->startTime;
				if (slot->time < start)
				{
					end = slot->anim->endTime;
					slot->time = end - (start - slot->time) % ((end - start) + 1);
					if (IS_MOTION())
					{
						bGetKey3(a, node->posTrack, slot->anim->startTime);
						ADD_MOTION(a);
						bGetKey3(node->pos, node->posTrack, slot->anim->endTime);
					}
				}
			}
			if (node->numEvents != 0)
				bSendAnimEvents(flags & BANIMF_NEW, slot->time, inst, def, oldTime, delta, node, slot->anim);
			if (IS_MOTION())
			{
				before[0] = node->pos[0];
				before[1] = node->pos[1];
				before[2] = node->pos[2];
				bBlendNodeToTime(inst, node, slot->time, slot->blend, 0);
				rootMotion[0] = (node->pos[0] - before[0]) + rootMotion[0];
				rootMotion[1] = (node->pos[1] - before[1]) + rootMotion[1];
				rootMotion[2] = (node->pos[2] - before[2]) + rootMotion[2];
			}
			else
			{
				bBlendNodeToTime(inst, node, slot->time, slot->blend, 0);
			}
			goto node_done;

		case BANIM_COPY:
		{
			struct BActorNode *source = slot->source;
			struct BAnimSlot *sourceSlot = &source->slots[source->curSlot];

			if (sourceSlot->active == 0)
			{
				REPORT();
				NEXT_SLOT();
				if (CURRENT_ACTIVE())
					goto next_slot;
				goto node_done;
			}
			blend = slot->blendSpeed + slot->blend; /* not scaled by dt */
			slot->blend = blend;
			if (blend <= 1.0f && sourceSlot->mode == BANIM_PLAY)
			{
				slot->anim = sourceSlot->anim;
				slot->time = sourceSlot->time;
				slot->speed = 0;
				bBlendNodeToTime(inst, node, slot->time, blend, 0);
				goto node_done;
			}
			if (slot->callback != NULL && (flags & BANIMF_CHILD) == 0 && numEvents < 8)
			{
				event = &events[numEvents];
				event->reason = 0;
				event->instance = inst;
				event->node = node;
				event->anim = slot->anim;
				event->context = slot->context;
				event->callback = slot->callback;
				numEvents++;
			}
			/* take over the source's queue */
			node->curSlot = source->curSlot;
			memcpy(node->slots, source->slots, node->numSlots * sizeof(struct BAnimSlot));
			goto next_slot;
		}

		default:
			goto node_done;
		}

	add_movement:
		/* the motion node moved by the difference of its positions */
		rootMotion[0] = (node->pos[0] - before[0]) + rootMotion[0];
		rootMotion[1] = (node->pos[1] - before[1]) + rootMotion[1];
		rootMotion[2] = (node->pos[2] - before[2]) + rootMotion[2];

	node_done:
		if (node == inst->motionNode && rootMotion != NULL)
		{
			ba_MatTransformInPlace(inst->rotMatrix, rootMotion);
			rootMotion[0] = inst->scale[0] * rootMotion[0];
			rootMotion[1] = inst->scale[1] * rootMotion[1];
			rootMotion[2] = inst->scale[2] * rootMotion[2];
		}
		if (node->child != NULL)
			numEvents = bUpdateNodes(inst, node->child, dt, rootMotion, events, numEvents, numActive);
		node = node->next;
	} while (node != first);
	return numEvents;
#undef IS_MOTION
#undef REPORT
#undef NEXT_SLOT
#undef CURRENT_ACTIVE
#undef ADD_MOTION
}

/* 0x000380f0  BACTOR/actor.cpp (file inferred)
advances the animations of an instance by dt (and calls the callbacks of
the animations that ended, until one returns non-zero); returns the number
of active animation slots */
int baUpdateAnimations(struct BActorInstance *inst, struct BActorNode *node, float dt, float *rootMotion)
{
	struct BAnimEvent events[8];
	int numActive = 0;
	int numEvents;
	int i;

	ba_MatFromQuat(inst->rotMatrix, inst->rot);
	numEvents = bUpdateNodes(inst, node, dt, rootMotion, events, 0, &numActive);
	for (i = 0; i < numEvents; i++)
	{
		if (events[i].callback(events[i].reason, events[i].instance, events[i].node, events[i].anim,
				events[i].context) != 0)
			break;
	}
	return numActive;
}

/* 0x00038190  BACTOR/actor.cpp (file inferred)
removes the queued animations of node after the first keep slots (all when
keep is 0), calling their callbacks with reason 1 */
void baFlushAnimations(void *inst, struct BActorNode *node, int recurse, int keep)
{
	struct BAnimSlot *slot;
	struct BActorNode *child;
	int numSlots;
	int index;
	int n;

	memcpy(node->prevPos, node->pos, 0x30); /* pos, scale, rot */
	numSlots = node->numSlots;
	if (numSlots != 0)
	{
		index = (node->curSlot + keep) % numSlots;
		for (n = keep; n < (int)node->numSlots; n++)
		{
			slot = &node->slots[index];
			if (slot->active != 0)
			{
				if (slot->callback != NULL)
				{
					if (slot->mode == BANIM_PLAY || slot->mode == BANIM_BLENDIN)
						slot->callback(1, inst, node, slot->anim, slot->context);
					else if (slot->mode == BANIM_BLEND)
						slot->callback(1, inst, node, slot->anim2, slot->context);
				}
				slot->active = 0;
			}
			index = (index + 1) % node->numSlots;
		}
		if (keep == 0)
			node->curSlot = 0;
	}
	if (recurse != 0 && (child = node->child) != NULL)
	{
		do
		{
			if ((child->flags & BNODEF_NOANIM) == 0)
				baFlushAnimations(inst, child, 1, keep);
			child = child->next;
		} while (child != node->child);
	}
}
