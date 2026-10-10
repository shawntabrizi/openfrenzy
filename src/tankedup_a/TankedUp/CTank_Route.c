/*
TankedUp/CTank_Route.c  <-  C:\BlitzParty\Game\Src\TankedUp\CTank_Route.cpp

CTank_Route (CTank_Game +0xcdc, "routeSystem"): a graph of route nodes for the AI tanks.
The nodes are fixed in the code (two sets, by the world's mode); nodes that see each other
(the world's ground sweep) are linked. For each of five targets the route keeps, per node,
the cost to the target and the next node (a flood from the nodes that see the target; a
node's cost grows by 1000 for each entity near it). The ROUTEFILE is loaded but not read.

0x001c74c0, 0x001c75f0, 0x001c7a70 and 0x001c7ac0 are tagged with this file; the
others between 0x001c7280 and 0x001c7e20 are placed here because they work on this layout.
*/

#include "tankedup_a.h"

#define TU_ROUTE_TARGETS 5

/* a route node (0x4c bytes) */
struct TuRouteNode {
	int valid;                   /* +0x00 it stands on the ground */
	float pos[4];                /* +0x04 */
	struct TuRouteLink *links;   /* +0x14 the nodes it sees */
	float penalty;               /* +0x18 1000 per entity near it */
	float radius;                /* +0x1c the radius for the penalty */
	float cost[TU_ROUTE_TARGETS]; /* +0x20 the cost to target i */
	struct TuRouteNode *next[TU_ROUTE_TARGETS]; /* +0x34 the next node towards target i */
	struct TuRouteNode *nextNode; /* +0x48 the list */
};
OF_ASSERT_SIZE(struct TuRouteNode, 0x4c);

/* a link (0xc bytes); the same shape is the open list of the flood (distance unused) */
struct TuRouteLink {
	struct TuRouteNode *node;    /* +0x00 */
	float distance;              /* +0x04 */
	struct TuRouteLink *next;    /* +0x08 */
};

/* the route system (CTank_Game +0xcdc, 0x64 bytes) */
struct TuRoute {
	float targets[TU_ROUTE_TARGETS][4]; /* +0x00 */
	int loaded;                  /* +0x50 */
	struct TuRouteNode *nodes;   /* +0x54 */
	void *world;                 /* +0x58 */
	void *entities;              /* +0x5c */
	int unknown60;               /* +0x60 */
};
OF_ASSERT_SIZE(struct TuRoute, 0x64);

/* the vertex buffer of the route lines (0x00315350, 2 vertices of FVF 0x152) */
#define TU_ROUTE_LINE (&TU_VAR(float, 0x00315350))

static void route_draw_line(const float *from, const float *to)
{
	float *line = TU_ROUTE_LINE;

	line[0] = from[0];
	line[1] = from[1] + 10.0f;
	line[2] = from[2];
	TU_U32(line, 0x18) = DAT_002bc898[0x7f] | DAT_002bd640[0] | DAT_002bc490[0] | DAT_002bce18[0xff];
	line[9] = to[0];
	line[10] = to[1] + 10.0f;
	line[11] = to[2];
	TU_U32(line, 0x3c) = DAT_002bc490[0xff] | DAT_002bce18[0] | DAT_002bc898[0x7f] | DAT_002bd640[0];
	bdSetWorldMatrix(DAT_002855f0);
	bdDrawPrimitiveFVF(1, line, 2, 0x152);
}

/* 0x001c7280  TankedUp/CTank_Route.cpp (inferred)
The constructor. */
void *CTankRoute_Ctor(void *self)
{
	((struct TuRoute *)self)->loaded = 0;
	return self;
}

/* 0x001c7290  TankedUp (gap; shared by the route and spawn systems)
A subsystem update that keeps the state: returns `state`. */
int CTankSystem_KeepState(void *self, float step, int state)
{
	(void)self;
	(void)step;
	return state;
}

/* 0x001c72a0  TankedUp/CTank_Route.cpp (inferred)
Debug draw (with a debug camera): debug camera 0 shows every link, camera n the next
nodes towards target n-1. */
void CTankRoute_Draw(void *self)
{
	struct TuRoute *route = (struct TuRoute *)self;
	struct TuRouteNode *node;
	struct TuRouteLink *link;

	if (route->loaded == 0)
		return;
	bdSetProjectionMode(1);
	bdSetRenderState(0, 1, 0);
	bdSetRenderState(1, 2, 0);
	bdSetRenderState(3, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	baSetBrightness(0x100);
	bdSetTexture(0, NULL);
	for (node = route->nodes; node != NULL; node = node->nextNode) {
		int camera = TU_DBG_CAMERA;

		if (camera == 0) {
			for (link = node->links; link != NULL; link = link->next)
				route_draw_line(node->pos, link->node->pos);
		} else if (node->next[camera - 1] != NULL) {
			route_draw_line(node->pos, node->next[camera - 1]->pos);
		}
	}
}

/* 0x001c74c0  TankedUp/CTank_Route.cpp
Makes a node at (x, z) on the ground under 10000 (the world's ground test); it is not
valid when there is no ground. */
void CTankRoute_NewNode(void *self, struct TuRouteNode **out, float x, float z, float radius)
{
	struct TuRoute *route = (struct TuRoute *)self;
	struct TuRouteNode *node;
	float normal[4];
	float top;

	node = (struct TuRouteNode *)TU_NEW(0x4c, TU_FILE_ROUTE, 0x43);
	*out = node;
	node->valid = 1;
	(*out)->links = NULL;
	(*out)->nextNode = NULL;
	(*out)->radius = radius;
	(*out)->next[0] = NULL;
	(*out)->next[1] = NULL;
	(*out)->next[2] = NULL;
	(*out)->next[3] = NULL;
	(*out)->next[4] = NULL;
	node = *out;
	node->pos[2] = z;
	node->pos[0] = x;
	node->pos[1] = 10000.0f;
	node->pos[3] = 1.0f;
	top = (*out)->pos[1];
	(*out)->pos[1] = top - CTankWorld_GroundDistance(route->world, (*out)->pos, 100.0f, normal);
	if ((*out)->pos[1] < -10000.0f)
		(*out)->valid = 0;
}

/* 0x001c7580  TankedUp/CTank_Route.cpp (inferred)
Adds a node at (x, z) when it stands on the ground. */
void CTankRoute_AddNode(void *self, float x, float z, float radius)
{
	struct TuRoute *route = (struct TuRoute *)self;
	struct TuRouteNode *node;

	CTankRoute_NewNode(self, &node, x, z, radius);
	if (node->valid != 0) {
		TU_DPRINTF("Adding node [%.0f,%.0f]\n", (double)x, (double)z);
		node->nextNode = route->nodes;
		route->nodes = node;
		return;
	}
	TU_DPRINTF("NOT Adding node [%.0f,%.0f]\n", (double)x, (double)z);
	TU_DELETE(node);
}

/* 0x001c75f0  TankedUp/CTank_Route.cpp
Links every pair of nodes that see each other. */
void CTankRoute_LinkNodes(void *self)
{
	struct TuRoute *route = (struct TuRoute *)self;
	struct TuRouteNode *a, *b;

	for (a = route->nodes; a != NULL; a = a->nextNode) {
		for (b = route->nodes; b != NULL; b = b->nextNode) {
			float dx, dy, dz;
			struct TuRouteLink *link;

			if (a == b)
				continue;
			dx = b->pos[0] - a->pos[0];
			dy = b->pos[1] - a->pos[1];
			dz = b->pos[2] - a->pos[2];
			if (CTankWorld_CanSee(route->world, a->pos, b->pos) == 0)
				continue;
			TU_DPRINTF("Linking Node [%.0f,%.0f] to Node [%.0f,%.0f]\n", (double)a->pos[0],
				(double)a->pos[2], (double)b->pos[0], (double)b->pos[2]);
			link = (struct TuRouteLink *)TU_NEW(0xc, TU_FILE_ROUTE, 0x92);
			link->node = b;
			link->distance = sqrtf(dx * dx + dy * dy + dz * dz);
			link->next = a->links;
			a->links = link;
		}
	}
}

/* 0x001c76f0  TankedUp/CTank_Route.cpp (inferred: "CTank_Route::Initialise")
Initialise: clears the targets, loads the route file (its text is not read), and adds the
node set of the world's mode, then links the nodes. Always returns 1. */
int CTankRoute_Initialise(void *self, void *package, char *file, void *world)
{
	struct TuRoute *route = (struct TuRoute *)self;
	struct TuTextReader reader;
	unsigned int size;
	void *buffer;
	int i;

	TUB_Reader_Ctor(&reader);
	route->world = world;
	for (i = 0; i < TU_ROUTE_TARGETS; i++) {
		route->targets[i][0] = 0.0f;
		route->targets[i][1] = 0.0f;
		route->targets[i][2] = 0.0f;
		route->targets[i][3] = 1.0f;
	}
	if (file[0] != 0) {
		buffer = bkLoadFile((struct bkPackage *)package, file, NULL, &size, NULL, 0);
		if (buffer != NULL) {
			BCarTextReader_Init((struct BCarTextReader *)&reader, (char *)buffer, (int)size, file);
			TU_DPRINTF("Loading Routeing info\n");
			route->nodes = NULL;
			if (CTankWorld_GetMode(world) == 0) {
				CTankRoute_AddNode(self, 1500.0f, 1500.0f, 1000.0f);
				CTankRoute_AddNode(self, 1500.0f, -1500.0f, 1000.0f);
				CTankRoute_AddNode(self, -1500.0f, 1500.0f, 1000.0f);
				CTankRoute_AddNode(self, -1500.0f, -1500.0f, 1000.0f);
				CTankRoute_AddNode(self, 0.0f, 1250.0f, 1000.0f);
				CTankRoute_AddNode(self, 0.0f, -1250.0f, 1000.0f);
				CTankRoute_AddNode(self, 1250.0f, 0.0f, 1000.0f);
				CTankRoute_AddNode(self, -1250.0f, 0.0f, 1000.0f);
			} else {
				CTankRoute_AddNode(self, -41.0f, -3356.0f, 0.0f);
				CTankRoute_AddNode(self, 2011.0f, -1851.0f, 0.0f);
				CTankRoute_AddNode(self, 3004.0f, -390.0f, 0.0f);
				CTankRoute_AddNode(self, 1088.0f, 1004.0f, 0.0f);
				CTankRoute_AddNode(self, 1803.0f, 1561.0f, 0.0f);
				CTankRoute_AddNode(self, 2589.0f, 2538.0f, 0.0f);
				CTankRoute_AddNode(self, 166.0f, 2977.0f, 0.0f);
				CTankRoute_AddNode(self, -1164.0f, 249.0f, 0.0f);
				CTankRoute_AddNode(self, -2243.0f, 432.0f, 0.0f);
				CTankRoute_AddNode(self, -2738.0f, -312.0f, 0.0f);
				CTankRoute_AddNode(self, -1996.0f, -1168.0f, 0.0f);
				CTankRoute_AddNode(self, -915.0f, -1969.0f, 0.0f);
				CTankRoute_AddNode(self, -791.0f, 1583.0f, 0.0f);
				CTankRoute_AddNode(self, -1074.0f, 269.0f, 0.0f);
				CTankRoute_AddNode(self, -1816.0f, 798.0f, 0.0f);
			}
			CTankRoute_LinkNodes(self);
			route->loaded = 1;
			TUB_Reader_Dtor(&reader);
			return 1;
		}
		TU_DPRINTF("CTank_Route::Initialise - Configuration file [%s] not found\n", file);
	}
	route->loaded = 0;
	TUB_Reader_Dtor(&reader);
	return 1;
}

/* 0x001c79f0  TankedUp/CTank_Route.cpp (inferred)
Frees the nodes and their links. */
void CTankRoute_Free(void *self)
{
	struct TuRoute *route = (struct TuRoute *)self;

	if (route->loaded == 0)
		return;
	while (route->nodes != NULL) {
		struct TuRouteNode *node;

		while (route->nodes->links != NULL) {
			struct TuRouteLink *link = route->nodes->links;

			route->nodes->links = link->next;
			TU_DELETE(link);
		}
		node = route->nodes;
		route->nodes = node->nextNode;
		TU_DELETE(node);
	}
}

/* 0x001c7a50  TankedUp/CTank_Route.cpp (inferred)
Frees a list of links (`this` is the list head). */
void CTankRoute_FreeList(struct TuRouteLink **self)
{
	while (*self != NULL) {
		struct TuRouteLink *link = *self;

		*self = link->next;
		TU_DELETE(link);
	}
}

/* 0x001c7a70  TankedUp/CTank_Route.cpp
Puts `node` into the open list (`this` is its head), before the first entry whose node lies
at a higher address (the original orders the list by address). */
void CTankRoute_Push(struct TuRouteLink **self, struct TuRouteNode *node)
{
	struct TuRouteLink **link = self;
	struct TuRouteLink *entry;

	if (*link != NULL) {
		do {
			if ((uintptr_t)&node->cost[0] < (uintptr_t)&(*link)->node->cost[0])
				break;
			link = &(*link)->next;
		} while (*link != NULL);
	}
	entry = (struct TuRouteLink *)TU_NEW(0xc, TU_FILE_ROUTE, 0x248);
	entry->node = node;
	entry->next = *link;
	*link = entry;
}

/* 0x001c7ac0  TankedUp/CTank_Route.cpp
Sets target `index` (0-4) to `pos` and floods the costs towards it: the nodes that see the
target cost their distance (plus their penalty), then each node passes its cost on through
its links. */
void CTankRoute_SetTarget(void *self, float *pos, int index)
{
	struct TuRoute *route = (struct TuRoute *)self;
	struct TuRouteLink *open = NULL;
	struct TuRouteNode *node;

	route->targets[index][0] = pos[0];
	route->targets[index][1] = pos[1];
	route->targets[index][2] = pos[2];
	route->targets[index][3] = pos[3];
	if (route->loaded == 0 || index >= 5 || index < 0)
		return;
	for (node = route->nodes; node != NULL; node = node->nextNode) {
		node->penalty = (float)(CTankEntities_CountNear(route->entities, node->pos, node->radius)
			* 1000);
		node->next[index] = NULL;
		node->cost[index] = 9999999.0f;
	}
	for (node = route->nodes; node != NULL; node = node->nextNode) {
		float dx = pos[0] - node->pos[0];
		float dy = pos[1] - node->pos[1];
		float dz = pos[2] - node->pos[2];

		if (CTankWorld_CanSee(route->world, node->pos, pos) != 0) {
			node->cost[index] = sqrtf(dz * dz + dy * dy + dx * dx) + node->penalty;
			CTankRoute_Push(&open, node);
		}
	}
	while (open != NULL) {
		struct TuRouteLink *entry = open;
		struct TuRouteLink *link;

		node = entry->node;
		open = entry->next;
		TU_DELETE(entry);
		for (link = node->links; link != NULL; link = link->next) {
			float cost = link->distance + node->cost[index] + node->penalty;

			if (cost < link->node->cost[index]) {
				link->node->cost[index] = cost;
				link->node->next[index] = node;
				CTankRoute_Push(&open, link->node);
			}
		}
	}
}

/* 0x001c7cd0  TankedUp/CTank_Route.cpp (inferred)
Where to drive from `pos` towards target `index`: *out gets the target when `pos` sees it,
else the node with the lowest distance + cost that `pos` sees. Returns that distance + cost
(the straight distance when there is no route). */
float CTankRoute_GetWaypoint(void *self, float *pos, float *out, int index)
{
	struct TuRoute *route = (struct TuRoute *)self;
	float *target;
	struct TuRouteNode *node;
	float best;

	target = route->targets[index];
	out[0] = target[0];
	out[1] = target[1];
	out[2] = target[2];
	out[3] = target[3];
	if (route->loaded == 0 || index >= 5 || index < 0)
		return bmVectorDistance(target, pos);
	if (CTankWorld_CanSee(route->world, pos, target) != 0) {
		out[0] = target[0];
		out[1] = target[1];
		out[2] = target[2];
		out[3] = target[3];
		return bmVectorDistance(target, pos);
	}
	best = 9999999.0f;
	for (node = route->nodes; node != NULL; node = node->nextNode) {
		float dx = node->pos[0] - pos[0];
		float dy = node->pos[1] - pos[1];
		float dz = node->pos[2] - pos[2];
		float distance = sqrtf(dx * dx + dy * dy + dz * dz);

		if (distance + node->cost[index] < best
			&& CTankWorld_CanSee(route->world, pos, node->pos) != 0) {
			out[0] = node->pos[0];
			out[1] = node->pos[1];
			out[2] = node->pos[2];
			out[3] = node->pos[3];
			best = distance + node->cost[index];
		}
	}
	return best;
}

/* 0x001c7e20  TankedUp/CTank_Route.cpp (inferred)
Keeps the entity system (for the penalties) and sets the five fixed targets. */
void CTankRoute_LinkEntities(void *self, void *entities)
{
	static const float targets[TU_ROUTE_TARGETS][4] = {
		{ 2500.0f, 100.0f, 0.0f, 1.0f },
		{ 0.0f, 100.0f, 2500.0f, 1.0f },
		{ -2500.0f, 100.0f, 0.0f, 1.0f },
		{ 0.0f, 100.0f, -3200.0f, 1.0f },
		{ 701.0f, 75.0f, -1555.0f, 1.0f },
	};
	float pos[TU_ROUTE_TARGETS][4];
	int i;

	((struct TuRoute *)self)->entities = entities;
	memcpy(pos, targets, sizeof pos);
	for (i = 0; i < TU_ROUTE_TARGETS; i++)
		CTankRoute_SetTarget(self, pos[i], i);
}
