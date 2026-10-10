/*
TankedUp/CTank_Entities.c  <-  C:\BlitzParty\Game\Src\TankedUp\CTank_Entities.cpp

The entity system of TankedUp (CTank_Entities): every object of the arena (tanks, mines,
tokens, crates, shells, ...) is an entity (0xf08 bytes) made from a template (0x6e0 bytes),
both read from the level's entity file ("SETTINGS {", "QUICK_ENTITY {", "ENTITY {",
"TEMPLATE {" blocks). The object also keeps a queue of delayed messages.
Offsets: tankedup_b.h (ENTS_*, ENT_*, TPL_*).
*/

#include "tankedup_b.h"
#include "tankedup_b_data.h"
#include "tankedup_b_ext.h"

/* 0x001cee60  TankedUp/CTank_Entities.cpp
CTank_Entities::SpawnEntity: a new list node and entity (not linked in); 0 without memory. */
int TankEnts_SpawnEntity(void *self, void **outNode)
{
	void **node;
	void *ent;

	(void)self;
	node = (void **)bkHeapAlloc(8, TU_FILE_ENTITIES, 0x1aa, 0x1001);
	*outNode = node;
	if (node == NULL) {
		TU_DEBUG("CTank_Entities::SpawnEntity - Could not get memory for new object\n");
		return 0;
	}
	ent = bkHeapAlloc(0xf08, TU_FILE_ENTITIES, 0x1af, 0x1001);
	if (ent != NULL)
		TA_TimerCtor(TU_ADDR(ent, ENT_TIMER));
	*(void **)*outNode = ent;
	if (ent == NULL) {
		TU_DEBUG("CTank_Entities::SpawnEntity - Could not get memory for new object entity\n");
		return 0;
	}
	TA_InitEntity(*(void **)*outNode);
	return 1;
}

/* the "Invalid Command" / "Bad use of Command" report of the entity file parser */
static void tu_report_command(void *reader, const char *line, int result)
{
	char command[512];

	(void)reader;
	if (_stricmp(line, "}") == 0) /* 0x00263ccc */
		return;
	sscanf(line, "%s", command);
	if (result == 0)
		TU_DEBUG("CTank_Entities::LoadEntities(\"%s\", %d): Invalid Command \"%s\"\n",
			TU_STR(reader, RD_NAME), TU_I32(reader, RD_LINE), command);
	else if (result == 2)
		TU_DEBUG("CTank_Entities::LoadEntities(\"%s\", %d): Bad use of Command \"%s\"\n",
			TU_STR(reader, RD_NAME), TU_I32(reader, RD_LINE), command);
}

/* "<KEY> <int>" with a value >= 0 into *out: 1 done, 2 bad use, `result` when not this key */
static int tu_parse_int(const char *line, const char *key, int length, int *out, int result)
{
	int value;

	if (_strnicmp(line, key, length) != 0)
		return result;
	if (sscanf(line, "%*s%d", &value) == 1 && value > -1) {
		*out = value;
		return 1;
	}
	return 2;
}

/* "<KEY> <float>" with a value >= 0 into *out */
static int tu_parse_float_pos(const char *line, const char *key, int length, float *out, int result)
{
	float value;

	if (_strnicmp(line, key, length) != 0)
		return result;
	if (sscanf(line, "%*s%f", &value) == 1 && value >= 0.0f) {
		*out = value;
		return 1;
	}
	return 2;
}

/* "<KEY> "<string>"" into out */
static int tu_parse_name(char *line, const char *key, int length, char *out, int size, int result)
{
	if (_strnicmp(line, key, length) != 0)
		return result;
	return BCarText_GetName(line, out, size) != 0 ? 1 : 2;
}

/* 0x001cef60  TankedUp/CTank_Entities.cpp
CTank_Entities::LoadEntities: reads the entity file (reader: tankedup_b.h RD_*). */
void TankEnts_LoadEntities(void *self, void *reader)
{
	char buffer[512];
	char templateName[64];
	char *line;
	int result;
	int value;
	float fvalue;
	float x, y, z;
	float angles[3];
	void **node;
	char *ent;
	char *tpl;
	int i;

	while (TU_READER_MORE(reader)) {
		line = TankReader_GetLine(reader, buffer);
		if (strncmp(line, "//", 2) == 0)
			continue;

		if (_stricmp(line, "SETTINGS {") == 0) {
			while (TU_READER_MORE(reader)) {
				if (_stricmp(line, "}") == 0)
					break;
				line = TankReader_GetLine(reader, buffer);
				if (strncmp(line, "//", 2) == 0)
					continue;
				result = 0;
				result = tu_parse_int(line, "1Pt_TK ", 7, &TU_I32(self, ENTS_PT1), result);
				result = tu_parse_int(line, "3Pt_TK ", 7, &TU_I32(self, ENTS_PT3), result);
				result = tu_parse_int(line, "5Pt_TK ", 7, &TU_I32(self, ENTS_PT5), result);
				/* the original compares only 7 characters of "SPEED_TK " */
				result = tu_parse_int(line, "SPEED_TK ", 7, &TU_I32(self, ENTS_SPEED_TK), result);
				result = tu_parse_int(line, "PRO_TK ", 7, &TU_I32(self, ENTS_PRO_TK), result);
				result = tu_parse_int(line, "WIN_SWITCH_TK ", 0xe, &TU_I32(self, ENTS_WIN_SWITCH),
					result);
				result = tu_parse_int(line, "FIRE_UP_TK ", 0xb, &TU_I32(self, ENTS_FIRE_UP), result);
				tu_report_command(reader, line, result);
			}
		}

		if (_stricmp(line, "QUICK_ENTITY {") == 0) {
			while (TU_READER_MORE(reader)) {
				if (_stricmp(line, "}") == 0)
					break;
				line = TankReader_GetLine(reader, buffer);
				if (strncmp(line, "//", 2) == 0)
					continue;
				result = 0;
				result = tu_parse_name(line, "TEMPLATE ", 9, templateName, 0x3f, result);
				if (_strnicmp(line, "ENT ", 4) == 0) { /* 0x0027aae8 */
					result = 2;
					if (sscanf(line, "%*s%f%f%f", &x, &y, &z) == 3) {
						node = (void **)bkHeapAlloc(8, TU_FILE_ENTITIES, 0x284, 0x1001);
						if (node == NULL) {
							TU_DEBUG("CTank_Entities::LoadEntities - Could not get memory for new object\n");
							return;
						}
						ent = (char *)bkHeapAlloc(0xf08, TU_FILE_ENTITIES, 0x289, 0x1001);
						if (ent != NULL)
							TA_TimerCtor(ent + ENT_TIMER);
						node[0] = ent;
						if (ent == NULL) {
							TU_DEBUG("CTank_Entities::LoadEntities - Could not get memory for new object entity\n");
							return;
						}
						TA_InitEntity(ent);
						TU_F32(ent, ENT_POS) = x;
						TU_F32(ent, ENT_POS + 4) = y;
						TU_F32(ent, ENT_POS + 8) = z;
						TU_F32(ent, ENT_POS + 12) = 1.0f;
						strcpy(ent + ENT_TEMPLATE_NAME, templateName);
						sprintf(ent + ENT_NAME, "%s - %d", templateName, TU_I32(ent, ENT_ID));
						node[1] = TU_PTR(self, ENTS_LIST);
						TU_PTR(self, ENTS_LIST) = node;
						result = 1;
					}
				}
				tu_report_command(reader, line, result);
			}
		}

		if (_stricmp(line, "ENTITY {") == 0) {
			node = (void **)bkHeapAlloc(8, TU_FILE_ENTITIES, 0x2b2, 0x1001);
			if (node == NULL) {
				TU_DEBUG("CTank_Entities::LoadEntities - Could not get memory for new object\n");
				return;
			}
			ent = (char *)bkHeapAlloc(0xf08, TU_FILE_ENTITIES, 0x2b7, 0x1001);
			if (ent != NULL)
				TA_TimerCtor(ent + ENT_TIMER);
			node[0] = ent;
			if (ent == NULL) {
				TU_DEBUG("CTank_Entities::LoadEntities - Could not get memory for new object entity\n");
				return;
			}
			TA_InitEntity(ent);
			while (TU_READER_MORE(reader)) {
				if (_stricmp(line, "}") == 0)
					break;
				line = TankReader_GetLine(reader, buffer);
				if (strncmp(line, "//", 2) == 0)
					continue;
				result = 0;
				result = tu_parse_name(line, "NAME ", 5, ent + ENT_NAME, 0x3f, result);
				result = tu_parse_name(line, "SPAWNLIST ", 10, ent + ENT_SPAWNLIST, 0x3f, result);
				result = tu_parse_name(line, "TEMPLATE ", 9, ent + ENT_TEMPLATE_NAME, 0x3f, result);
				result = tu_parse_name(line, "ACTOR ", 6, ent + ENT_ACTOR, 0x1ff, result);
				result = tu_parse_name(line, "TURRET ", 7, ent + ENT_TURRET, 0x1ff, result);
				result = tu_parse_name(line, "LOWACTOR ", 9, ent + ENT_LOW_ACTOR, 0x1ff, result);
				result = tu_parse_name(line, "LOWTURRET ", 10, ent + ENT_LOW_TURRET, 0x1ff, result);
				result = tu_parse_name(line, "CANNON ", 7, ent + ENT_CANNON, 0x1ff, result);
				result = tu_parse_name(line, "ANIMATION ", 10, ent + ENT_ANIM, 0x3f, result);
				if (_strnicmp(line, "POSITION ", 9) == 0) {
					result = 2;
					if (sscanf(line, "%*s%f%f%f", &x, &y, &z) == 3) {
						TU_F32(ent, ENT_POS) = x;
						TU_F32(ent, ENT_POS + 4) = y;
						TU_F32(ent, ENT_POS + 8) = z;
						TU_F32(ent, ENT_POS + 12) = 1.0f;
						result = 1;
					}
				}
				if (_strnicmp(line, "ORIENTATION ", 0xc) == 0) {
					result = 2;
					if (sscanf(line, "%*s%f%f%f", &angles[0], &angles[1], &angles[2]) == 3) {
						for (i = 0; i < 3; i++) {
							if (angles[i] < 0.0f || angles[i] > 360.0f) /* 0x00268688 */
								break;
						}
						if (i == 3) {
							TU_F32(ent, ENT_ROT + 12) = 1.0f;
							result = 1;
							/* 0x002640f8: degrees to radians */
							TU_F32(ent, ENT_ROT) = angles[0] * 0.0174532924f;
							TU_F32(ent, ENT_ROT + 4) = angles[1] * 0.0174532924f;
							TU_F32(ent, ENT_ROT + 8) = angles[2] * 0.0174532924f;
							TU_F32(ent, ENT_ROT_START) = TU_F32(ent, ENT_ROT);
							TU_F32(ent, ENT_ROT_START + 4) = TU_F32(ent, ENT_ROT + 4);
							TU_F32(ent, ENT_ROT_START + 8) = TU_F32(ent, ENT_ROT + 8);
							TU_F32(ent, ENT_ROT_START + 12) = TU_F32(ent, ENT_ROT + 12);
						}
					}
				}
				result = tu_parse_float_pos(line, "RADIUS ", 7, TU_FP(ent, ENT_RADIUS), result);
				result = tu_parse_int(line, "AI ", 3, &TU_I32(ent, ENT_AI), result); /* 0x0027aa90 */
				result = tu_parse_int(line, "ROUTENUM ", 9, &TU_I32(ent, ENT_ROUTENUM), result);
				tu_report_command(reader, line, result);
			}
			node[1] = TU_PTR(self, ENTS_LIST);
			TU_PTR(self, ENTS_LIST) = node;
		}

		if (_stricmp(line, "TEMPLATE {") == 0) {
			tpl = (char *)bkHeapAlloc(0x6e0, TU_FILE_ENTITIES, 0x37c, 0x1001);
			if (tpl == NULL) {
				TU_DEBUG("CTank_Entities::LoadEntities - Could not get memory for new template\n");
				return;
			}
			TU_I32(tpl, TPL_MENU) = 0;
			tpl[TPL_NAME] = 0;
			tpl[TPL_ANIM] = 0;
			tpl[TPL_ACTOR] = 0;
			tpl[TPL_TURRET] = 0;
			tpl[TPL_CANNON] = 0;
			TU_I32(tpl, TPL_FLAGS) = 0;
			TU_F32(tpl, TPL_RADIUS) = 0.0f;
			TU_F32(tpl, TPL_MASS) = 10.0f;
			TU_I32(tpl, TPL_AI) = 0;
			TU_I32(tpl, TPL_TYPE) = 0;
			TU_I32(tpl, 0x0c) = 0;
			TU_I32(tpl, TPL_MINES) = 0;
			TU_F32(tpl, TPL_LIFE) = 0.0f;
			TU_I32(tpl, TPL_HEALTH) = 0;
			TU_F32(tpl, TPL_RESTITUTION) = 0.0f;
			TU_F32(tpl, TPL_FORCE) = 10.0f;
			TU_U32(tpl, TPL_ROT) = 0x3e567750;
			TU_F32(tpl, TPL_RELOAD) = 0.0f;
			TU_F32(tpl, TPL_HIT_FRZ) = 0.0f;
			TU_F32(tpl, TPL_HIT_PRO) = 0.0f;
			TU_F32(tpl, TPL_RESTART) = 0.0f;
			TU_F32(tpl, TPL_SCORELOST) = 1.0f;
			TU_I32(tpl, TPL_SCOREGIVE) = 0;
			TU_F32(tpl, TPL_GRAVITY) = TA_WorldGravity(TU_PTR(self, ENTS_WORLD));
			TU_I32(tpl, TPL_CAMLOOK) = 0;
			TU_I32(tpl, 0x04) = 0;
			TU_F32(tpl, TPL_SCALE) = 1.0f;
			while (TU_READER_MORE(reader)) {
				if (_stricmp(line, "}") == 0)
					break;
				line = TankReader_GetLine(reader, buffer);
				if (strncmp(line, "//", 2) == 0)
					continue;
				result = 0;
				result = tu_parse_name(line, "NAME ", 5, tpl + TPL_NAME, 0x3f, result);
				result = tu_parse_name(line, "ACTOR ", 6, tpl + TPL_ACTOR, 0x1ff, result);
				result = tu_parse_name(line, "TURRET ", 7, tpl + TPL_TURRET, 0x1ff, result);
				result = tu_parse_name(line, "CANNON ", 7, tpl + TPL_CANNON, 0x1ff, result);
				result = tu_parse_name(line, "ANIMATION ", 10, tpl + TPL_ANIM, 0x3f, result);
				result = tu_parse_float_pos(line, "RADIUS ", 7, TU_FP(tpl, TPL_RADIUS), result);
				if (_strnicmp(line, "GRAVITY ", 8) == 0) {
					result = 2;
					if (sscanf(line, "%*s%f", &fvalue) == 1) {
						TU_F32(tpl, TPL_GRAVITY) = fvalue;
						result = 1;
					}
				}
				if (_strnicmp(line, "SCALE ", 6) == 0) {
					result = 2;
					if (sscanf(line, "%*s%f", &fvalue) == 1) {
						TU_F32(tpl, TPL_SCALE) = fvalue;
						result = 1;
					}
				}
				if (_strnicmp(line, "SCORELOST ", 10) == 0) {
					result = 2;
					if (sscanf(line, "%*s%f", &fvalue) == 1 && fvalue >= 0.0f) {
						result = 1;
						TU_F32(tpl, TPL_SCORELOST) = 1.0f - fvalue;
					}
				}
				if (_strnicmp(line, "SCOREGIVE ", 10) == 0) {
					result = 2;
					if (sscanf(line, "%*s%d", &value) == 1) {
						TU_I32(tpl, TPL_SCOREGIVE) = value;
						result = 1;
					}
				}
				if (_strnicmp(line, "CAMLOOK", 7) == 0) {
					TU_I32(tpl, TPL_CAMLOOK) = 1;
					result = 1;
				}
				if (_strnicmp(line, "MENU", 4) == 0) { /* 0x0027aa50 */
					TU_I32(tpl, TPL_MENU) = 1;
					result = 1;
				}
				if (_strnicmp(line, "MASS ", 5) == 0) {
					result = 2;
					if (sscanf(line, "%*s%f", &fvalue) == 1 && fvalue > 0.0f) {
						TU_F32(tpl, TPL_MASS) = fvalue;
						result = 1;
					}
				}
				result = tu_parse_int(line, "FLAGS ", 6, &TU_I32(tpl, TPL_FLAGS), result);
				result = tu_parse_int(line, "AI ", 3, &TU_I32(tpl, TPL_AI), result);
				result = tu_parse_int(line, "TYPE ", 5, &TU_I32(tpl, TPL_TYPE), result);
				result = tu_parse_int(line, "HEALTH ", 7, &TU_I32(tpl, TPL_HEALTH), result);
				result = tu_parse_int(line, "NUMOFMINES ", 0xb, &TU_I32(tpl, TPL_MINES), result);
				result = tu_parse_float_pos(line, "LIFE ", 5, TU_FP(tpl, TPL_LIFE), result);
				result = tu_parse_float_pos(line, "RESTITUTION ", 0xc, TU_FP(tpl, TPL_RESTITUTION),
					result);
				result = tu_parse_float_pos(line, "FORCE ", 6, TU_FP(tpl, TPL_FORCE), result);
				result = tu_parse_float_pos(line, "RELOAD ", 7, TU_FP(tpl, TPL_RELOAD), result);
				result = tu_parse_float_pos(line, "HITTIMEOUT_FRZ ", 0xf, TU_FP(tpl, TPL_HIT_FRZ),
					result);
				result = tu_parse_float_pos(line, "HITTIMEOUT_PRO ", 0xf, TU_FP(tpl, TPL_HIT_PRO),
					result);
				result = tu_parse_float_pos(line, "RESTARTTIME ", 0xc, TU_FP(tpl, TPL_RESTART),
					result);
				if (_strnicmp(line, "ROT ", 4) == 0) { /* 0x0027a9fc */
					result = 2;
					if (sscanf(line, "%*s%d", &value) == 1 && value > 0) {
						result = 1;
						TU_F32(tpl, TPL_ROT) = 6.28318548f / (float)value; /* 0x00255930 */
					}
				}
				tu_report_command(reader, line, result);
			}
			TU_PTR(tpl, TPL_NEXT) = TU_PTR(self, ENTS_TEMPLATES);
			TU_PTR(self, ENTS_TEMPLATES) = tpl;
		}
	}
}

/* load one model of an entity: the name (or the template's default), then the resource */
static void *tu_load_model(char *name)
{
	TankChar_SubstituteColours(name);
	return bLoadResource(NULL, name, 1, 0xdefa);
}

/* 0x001d0310  TankedUp/CTank_Entities.cpp
CTank_Entities::EntityLoadTemplate: loads the models of an entity (names from the entity or
its template) and sets it up from the template. */
void TankEnts_EntityLoadTemplate(void *self, void *entity)
{
	char *ent = (char *)entity;
	char *tpl = (char *)TU_PTR(ent, ENT_TEMPLATE);
	int ok = 1;
	void *inst;
	void *decal;

	if (tpl == NULL) {
		TU_I32(ent, ENT_DEAD) = 1;
		TU_DEBUG("CTank_Entities::EntityLoadTemplate - Entity defined with no name, ignoring\n");
		return;
	}
	if (ent[ENT_NAME] == 0) {
		TU_DEBUG("CTank_Entities::EntityLoadTemplate - Entity defined with no name, ignoring\n");
		TU_I32(ent, ENT_DEAD) = 1;
		return;
	}

	if (ent[ENT_ACTOR] == 0)
		_snprintf(ent + ENT_ACTOR, 0x1ff, "%s", tpl + TPL_ACTOR);
	if (ent[ENT_ACTOR] == 0) {
		TU_DEBUG("CTank_Entities::EntityLoadTemplate - No model specified. Entity [%s] now invisible\n",
			ent + ENT_NAME);
	} else {
		TU_PTR(ent, ENT_MODEL) = tu_load_model(ent + ENT_ACTOR);
		if (TU_PTR(ent, ENT_MODEL) == NULL)
			TU_DEBUG("CTank_Entities::EntityLoadTemplate - Could not load actor [%s]. Entity [%s] now invisible\n",
				ent + ENT_ACTOR, ent + ENT_NAME);
	}
	if (ent[ENT_LOW_ACTOR] != 0) {
		TU_PTR(ent, ENT_LOW_MODEL) = tu_load_model(ent + ENT_LOW_ACTOR);
		if (TU_PTR(ent, ENT_LOW_MODEL) == NULL)
			TU_DEBUG("CTank_Entities::EntityLoadTemplate - Could not load low actor [%s]. Entity [%s] will use full model\n",
				ent + ENT_LOW_ACTOR, ent + ENT_NAME);
	}
	if (ent[ENT_TURRET] == 0)
		_snprintf(ent + ENT_TURRET, 0x1ff, "%s", tpl + TPL_TURRET);
	if (ent[ENT_TURRET] != 0) {
		TU_PTR(ent, ENT_TURRET_MODEL) = tu_load_model(ent + ENT_TURRET);
		if (TU_PTR(ent, ENT_TURRET_MODEL) == NULL)
			TU_DEBUG("CTank_Entities::EntityLoadTemplate - Could not load turret actor [%s]. Entity [%s] has no turret\n",
				ent + ENT_TURRET, ent + ENT_NAME);
	}
	if (ent[ENT_LOW_TURRET] != 0) {
		TU_PTR(ent, ENT_LOW_TURRET_MODEL) = tu_load_model(ent + ENT_LOW_TURRET);
		if (TU_PTR(ent, ENT_LOW_TURRET_MODEL) == NULL)
			TU_DEBUG("CTank_Entities::EntityLoadTemplate - Could not load low turret actor [%s]. Entity [%s] will use full model\n",
				ent + ENT_LOW_TURRET, ent + ENT_NAME);
	}
	if (ent[ENT_CANNON] == 0)
		_snprintf(ent + ENT_CANNON, 0x1ff, "%s", tpl + TPL_CANNON);
	if (ent[ENT_CANNON] != 0) {
		TU_PTR(ent, ENT_CANNON_MODEL) = tu_load_model(ent + ENT_CANNON);
		if (TU_PTR(ent, ENT_CANNON_MODEL) == NULL)
			TU_DEBUG("CTank_Entities::EntityLoadTemplate - Could not load cannon actor [%s]. Entity [%s] has no cannon\n",
				ent + ENT_CANNON, ent + ENT_NAME);
	}

	if (TU_F32(ent, ENT_RADIUS) < 0.0f)
		TU_F32(ent, ENT_RADIUS) = TU_F32(tpl, TPL_RADIUS);
	if (TU_F32(ent, ENT_RADIUS) == 0.0f) {
		if (TU_PTR(ent, ENT_MODEL) == NULL) {
			TU_DEBUG("CTank_Entities::EntityLoadTemplate - Entity [%s] defined with no radius or actor, ignoring\n",
				ent + ENT_NAME);
			TU_I32(ent, ENT_DEAD) = 1;
			ok = 0;
		} else {
			/* the model's bounding radius */
			TU_F32(ent, ENT_RADIUS) = TU_F32(TU_PTR(ent, ENT_MODEL), 0x5c);
		}
	}
	TU_F32(ent, ENT_RADIUS) = TU_F32(tpl, TPL_SCALE) * TU_F32(ent, ENT_RADIUS);
	TU_DEBUG("ent: %s, radius: %f\n", ent + ENT_NAME, (double)TU_F32(ent, ENT_RADIUS));
	if (!ok)
		return;

	if (TU_PTR(ent, ENT_MODEL) != NULL) {
		inst = baInitActorInstance(NULL, (struct BActor *)TU_PTR(ent, ENT_MODEL), 2,
			TU_FILE_ENTITIES, 0x5d5);
		TU_PTR(ent, ENT_INST) = inst;
		if (inst == NULL) {
			TU_DEBUG("CTank_Entities::EntityLoadTemplate - Could not load actor instance for model [%s]. Entity [%s] now invisible\n",
				ent + ENT_ACTOR, ent + ENT_NAME);
		} else {
			baPrepActorInstance((struct BActorInstance *)inst, 1);
			if (ent[ENT_ANIM] == 0)
				_snprintf(ent + ENT_ANIM, 0x1ff, "%s", tpl + TPL_ANIM);
			if (ent[ENT_ANIM] != 0) {
				TU_PTR(ent, ENT_ANIM_SEG) = baFindSubObject(
					(struct BActorInstance *)TU_PTR(ent, ENT_INST), ent + ENT_ANIM);
				if (TU_PTR(ent, ENT_ANIM_SEG) == NULL)
					TU_DEBUG("CTank_Entities::EntityLoadTemplate - Could not find animation segment [%s] for Entity [%s]. Ignoring animation\n",
						ent + ENT_ANIM, ent + ENT_NAME);
			}
		}
	}
	if (TU_PTR(ent, ENT_LOW_MODEL) != NULL) {
		inst = baInitActorInstance(NULL, (struct BActor *)TU_PTR(ent, ENT_LOW_MODEL), 2,
			TU_FILE_ENTITIES, 0x5f1);
		TU_PTR(ent, ENT_LOW_INST) = inst;
		if (inst == NULL)
			TU_DEBUG("CTank_Entities::EntityLoadTemplate - Could not load low actor instance for model [%s]. Entity [%s] now usinf full model\n",
				ent + ENT_LOW_ACTOR, ent + ENT_NAME);
		else
			baPrepActorInstance((struct BActorInstance *)inst, 1);
	}
	if (TU_PTR(ent, ENT_TURRET_MODEL) != NULL) {
		inst = baInitActorInstance(NULL, (struct BActor *)TU_PTR(ent, ENT_TURRET_MODEL), 2,
			TU_FILE_ENTITIES, 0x600);
		TU_PTR(ent, ENT_TURRET_INST) = inst;
		if (inst == NULL)
			TU_DEBUG("CTank_Entities::EntityLoadTemplate - Could not load turret actor instance for model [%s]. Entity [%s] now has no turret\n",
				ent + ENT_TURRET, ent + ENT_NAME);
		else
			baPrepActorInstance((struct BActorInstance *)inst, 1);
	}
	if (TU_PTR(ent, ENT_LOW_TURRET_MODEL) != NULL) {
		inst = baInitActorInstance(NULL, (struct BActor *)TU_PTR(ent, ENT_LOW_TURRET_MODEL), 2,
			TU_FILE_ENTITIES, 0x60d);
		TU_PTR(ent, ENT_LOW_TURRET_INST) = inst;
		if (inst == NULL)
			TU_DEBUG("CTank_Entities::EntityLoadTemplate - Could not load low turret actor instance for model [%s]. Entity [%s] will use full model\n",
				ent + ENT_LOW_TURRET, ent + ENT_NAME);
		else
			baPrepActorInstance((struct BActorInstance *)inst, 1);
	}
	if (TU_PTR(ent, ENT_CANNON_MODEL) != NULL) {
		inst = baInitActorInstance(NULL, (struct BActor *)TU_PTR(ent, ENT_CANNON_MODEL), 1,
			TU_FILE_ENTITIES, 0x61a);
		TU_PTR(ent, ENT_CANNON_INST) = inst;
		if (inst == NULL)
			TU_DEBUG("CTank_Entities::EntityLoadTemplate - Could not load cannon actor instance for model [%s]. Entity [%s] now has no cannon\n",
				ent + ENT_CANNON, ent + ENT_NAME);
		else
			baPrepActorInstance((struct BActorInstance *)inst, 1);
	}

	TA_TimerReset(ent + ENT_TIMER);
	TU_I32(ent, ENT_HEALTH) = TU_I32(tpl, TPL_HEALTH);
	TU_I32(ent, ENT_MINES) = TU_I32(tpl, TPL_MINES);
	TU_I32(ent, 0x98c) = TU_I32(tpl, TPL_MASS);
	if (TU_I32(ent, ENT_AI) == -1)
		TU_I32(ent, ENT_AI) = TU_I32(tpl, TPL_AI);
	TU_I32(ent, ENT_DEAD) = 0;
	TU_I32(ent, 0x95c) = 0;
	if (TU_U32(ent, 0x988) == 0xbf800000u) /* -1.0f: not set */
		TU_I32(ent, 0x988) = TU_I32(tpl, TPL_FORCE);
	if (TU_I32(tpl, TPL_TYPE) == 1) {
		/* tanks leave track decals */
		decal = bkHeapAlloc(0x3c, TU_FILE_ENTITIES, 0x634, 0x1001);
		if (decal != NULL)
			decal = TankDecal_Ctor(decal, 0xf0, 90.0f, 4.0f, TU_PTR(self, ENTS_TEX_TRACK_DECAL),
				TU_PTR(self, ENTS_TEX_TRACK_DECAL));
		TU_PTR(ent, ENT_TRACKS) = decal;
	}
}

/* the drawn instance of an entity's body or turret: the low-detail one in variant 4 unless
   the debug switch 0x003151a4 asks for the full one */
#define TU_USE_LOW(self, low) ((low) != NULL && TU_I32(self, ENTS_VARIANT) == 4 && DAT_003151a4 == 0)

/* places an actor instance at `pos` (x, y, z, w) with the entity's rotation (+ `turret` on y),
   and the template's scale when `scale` (the body; the turret and cannon keep theirs) */
static void tu_place_instance(char *ent, void *inst, const float *pos, float turret, int scale)
{
	float rot[16];
	char *tpl = (char *)TU_PTR(ent, ENT_TEMPLATE);

	TU_F32(inst, 0xc4) = pos[1];
	TU_F32(inst, 0xc0) = pos[0];
	TU_F32(inst, 0xcc) = pos[3];
	TU_F32(inst, 0xc8) = pos[2];
	bmMatrixRotationYXZ(rot, TU_F32(ent, ENT_ROT), TU_F32(ent, ENT_ROT + 4) + turret + 3.1415925f,
		TU_F32(ent, ENT_ROT + 8)); /* 0x00256750 */
	bmMatrixToQuaternion(TU_FP(inst, 0xe0), rot);
	if (!scale)
		return;
	TU_F32(inst, 0xd4) = TU_F32(tpl, TPL_SCALE);
	TU_F32(inst, 0xd0) = TU_F32(tpl, TPL_SCALE);
	TU_F32(inst, 0xdc) = 1.0f;
	TU_F32(inst, 0xd8) = TU_F32(tpl, TPL_SCALE);
}

/* 0x001d0920  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::Draw: the glow and shadows of the tokens and effects, the ground shadows of
tanks, mines and crates, then every entity (debug spheres, track decals, body, turret,
cannon). */
void TankEnts_Draw(void *self)
{
	void **node;
	char *ent = NULL;
	char *tpl;
	int type;
	float x0, x1, z0, z1;
	float ground[4];
	float trans[16], rot[16], world[16];
	float height;
	unsigned int colour;
	void *texture;
	float pos[4];
	void *inst;
	int r, g, b;

	CToken_DrawGlowAll();
	bdSetRenderState(5, 0, 0);
	CToken_DrawGlowSprites();
	CToken_DrawShadowAll();
	bdSetRenderState(5, 1, 0);
	bdSetProjectionMode(1);
	bdSetRenderState(0, 1, 0);
	bdSetRenderState(1, 2, 0);
	bdSetRenderState(3, 1, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	baSetBrightness(0x100);
	TA_LightsEnable(TU_PTR(self, ENTS_LIGHTS), 0);
	CParticleEffect_SetOn((struct CParticleEffect *)TU_PTR(self, ENTS_EFFECTS), 0,
		TU_I32(self, ENTS_VARIANT) < 5);
	CParticleEffect_Draw((struct CParticleEffect *)TU_PTR(self, ENTS_EFFECTS), 0,
		TA_WorldLaps(TU_PTR(self, ENTS_WORLD)) == 0);
	TA_LightsEnable(TU_PTR(self, ENTS_LIGHTS), 1);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 3, 0);
	bdSetRenderState(3, 0, 0);

	/* the ground shadows */
	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		ent = (char *)node[0];
		if (TU_I32(ent, ENT_DEAD) != 0)
			continue;
		type = TU_ENT_TYPE(ent);
		if (TU_I32(self, ENTS_VARIANT) >= 5 && (type == 5 || type == 9))
			continue;
		if (type == 5 && TU_I32(ent, 0x97c) < 0)
			TA_DrawGlow(TU_FP(ent, ENT_POS), TU_F32(self, ENTS_SPIN), TU_PTR(self, ENTS_GLOW));
		type = TU_ENT_TYPE(ent);
		if ((type != 5 && type != 1 && type != 2 && type != 9) || TU_I32(ent, ENT_DEAD) != 0)
			continue;
		TankDraw_DecalStates();
		TankDraw_StencilOn();
		bdSetDepthOffset(-3.0f); /* 0xc0400000 */
		type = TU_ENT_TYPE(ent);
		if (type == 9) {
			z1 = 80.0f;
			x1 = 80.0f;
			z0 = -80.0f;
			x0 = -80.0f;
		} else if (type == 1) {
			x1 = 102.3f;
			z1 = 203.5f;
			x0 = -102.3f;
			z0 = -132.0f;
		} else if (type == 2) {
			z1 = 50.0f;
			x1 = 50.0f;
			z0 = -50.0f;
			x0 = -50.0f;
		} else {
			if (TU_I32(ent, 0x97c) == 1) {
				x1 = 114.0f;
				z1 = 20.0f;
			} else if (TU_I32(ent, 0x97c) == 3) {
				x1 = 140.0f;
				z1 = 22.0f;
			} else if (TU_I32(ent, 0x97c) == 5) {
				x1 = 158.0f;
				z1 = 22.0f;
			} else {
				x1 = 100.0f;
				z1 = 100.0f;
			}
			x0 = -x1;
			z0 = -z1;
		}
		height = TA_WorldGroundHeight(TU_PTR(self, ENTS_WORLD), TU_FP(ent, ENT_POS),
			TU_F32(ent, ENT_RADIUS), ground);
		bmMatrixTranslation(trans, TU_F32(ent, ENT_POS),
			(TU_F32(ent, ENT_POS + 4) - height) - TU_F32(ent, ENT_RADIUS), TU_F32(ent, ENT_POS + 8));
		bmMatrixRotationY(rot, TU_F32(ent, ENT_ROT + 4));
		bmMatrixMultiply(world, trans, rot);
		bdSetWorldMatrix(world);
		/* 0x002bc998, 0x002bd7d0, 0x002bc620, 0x002bcfa8: entries 64 and 100 of the colour tables */
		colour = DAT_002bc898[64] | DAT_002bd640[100] | DAT_002bc490[100] | DAT_002bce18[100];
		DAT_003153a0[0].colour = colour;
		DAT_003153a0[0].pos[0] = x0;
		DAT_003153a0[0].pos[2] = z1;
		DAT_003153a0[1].pos[2] = z0;
		DAT_003153a0[1].pos[0] = x0;
		DAT_003153a0[2].pos[0] = x1;
		DAT_003153a0[0].pos[1] = 0.0f;
		DAT_003153a0[1].pos[1] = 0.0f;
		DAT_003153a0[2].pos[1] = 0.0f;
		DAT_003153a0[2].pos[2] = z1;
		DAT_003153a0[3].pos[0] = x1;
		DAT_003153a0[3].pos[1] = 0.0f;
		DAT_003153a0[3].pos[2] = z0;
		DAT_003153a0[0].uv[0] = 0.0f;
		DAT_003153a0[0].uv[1] = 1.0f;
		DAT_003153a0[1].uv[0] = 0.0f;
		DAT_003153a0[1].uv[1] = 0.0f;
		DAT_003153a0[2].uv[0] = 1.0f;
		DAT_003153a0[2].uv[1] = 1.0f;
		DAT_003153a0[3].uv[0] = 1.0f;
		DAT_003153a0[3].uv[1] = 0.0f;
		type = TU_ENT_TYPE(ent);
		if (type == 1)
			texture = TU_PTR(self, ENTS_TEX_SHADOW + 4);
		else if (type == 2)
			texture = TU_PTR(self, ENTS_TEX_SHADOW + 8);
		else if (type == 9)
			texture = TU_PTR(self, ENTS_TEX_SHADOW + 12);
		else
			texture = TU_PTR(self, ENTS_TEX_SHADOW);
		DAT_003153a0[1].colour = colour;
		DAT_003153a0[2].colour = colour;
		DAT_003153a0[3].colour = colour;
		bdSetTexture(0, (struct bdTexture *)texture);
		bdDrawPrimitiveFVF(4, DAT_003153a0, 4, 0x152);
		bdSetDepthOffset(0.0f);
		FUN_0018e640();
	}

	/* the entities */
	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		ent = (char *)node[0];
		tpl = (char *)TU_PTR(ent, ENT_TEMPLATE);
		if (TU_I32(self, ENTS_VARIANT) > 4 && TU_I32(tpl, TPL_TYPE) == 5)
			continue;
		bdSetRenderState(6, 1, 0);
		bdSetRenderState(10, 3, 0);
		bdSetRenderState(3, 0, 0);
		bdSetRenderState(5, 1, 0);
		if (TU_PTR(ent, ENT_TRACKS) != NULL && DAT_0031519c != 0) {
			TankDraw_DecalStates();
			TankDraw_StencilOn();
			TankDecal_Draw(TU_PTR(ent, ENT_TRACKS));
			FUN_0018e640();
		}
		if (TU_I32(ent, ENT_DEAD) != 0)
			continue;
		if (TU_I32(ent, 0x96c) == -1)
			TA_LightsRestore(TU_PTR(self, ENTS_LIGHTS));
		else
			TA_SetAmbient(TU_I32(ent, 0x96c), TU_I32(ent, 0x970), TU_I32(ent, 0x974));
		bdSetRenderState(3, 1, 0);
		bdSetRenderState(10, 0, 0);
		bdSetRenderState(6, 0, 0);
		pos[0] = TU_F32(ent, ENT_POS);
		pos[1] = TU_F32(ent, ENT_POS + 4);
		pos[2] = TU_F32(ent, ENT_POS + 8);
		pos[3] = TU_F32(ent, ENT_POS + 12);
		if (TU_ENT_TYPE(ent) == 1)
			pos[1] = (pos[1] - TU_F32(ent, ENT_RADIUS)) + 60.0f; /* 0x00254e98 */

		/* debug: collision spheres */
		if (DAT_003150c0 != 0) {
			bdSetWorldMatrix(DAT_002855f0);
			r = -1;
			switch (TU_ENT_TYPE(ent)) {
			case 0:
				r = 0, g = 0xff, b = 0xff;
				break;
			case 1:
				TU_F32(ent, ENT_POS + 4) = TU_F32(ent, ENT_POS + 4) - TU_F32(ent, ENT_RADIUS);
				BCarDraw_WireSphere(TU_FP(ent, ENT_POS), TU_F32(ent, ENT_RADIUS), 10, 10, 0xff, 0, 0);
				TU_F32(ent, ENT_POS + 4) = TU_F32(ent, ENT_RADIUS) + TU_F32(ent, ENT_POS + 4);
				break;
			case 2:
			case 4:
				r = 0, g = 0xff, b = 0;
				break;
			case 3:
			case 7:
				r = 0, g = 0, b = 0xff;
				break;
			case 5:
			case 6:
			case 8:
				r = 0xff, g = 0xff, b = 0;
				break;
			case 9:
			case 10:
				r = 0xff, g = 0xff, b = 0xff;
				break;
			default:
				break;
			}
			if (r != -1)
				BCarDraw_WireSphere(TU_FP(ent, ENT_POS), TU_F32(ent, ENT_RADIUS), 10, 10, r, g, b);
		}

		tpl = (char *)TU_PTR(ent, ENT_TEMPLATE);
		if ((TU_U32(tpl, TPL_FLAGS) & 0x400) != 0)
			continue; /* drawn by TankEnts_DrawSpecial */
		if (TU_I32(tpl, TPL_TYPE) == 5)
			TankDraw_SetAlphaFactor(0xaa);
		if (TU_ENT_TYPE(ent) == 1)
			TA_LightsEnableTank(TU_PTR(self, ENTS_LIGHTS), 0);
		bdTextureSetFrame((struct bdTexture *)TU_PTR(self, ENTS_TEX_TRACKS),
			TU_FTOL(TU_F32(ent, 0x9c4)) % 4);
		bdTextureSetFrame((struct bdTexture *)TU_PTR(self, ENTS_TEX_TRACKS + 4),
			TU_FTOL(TU_F32(ent, 0x9c8)) % 4);
		if (TU_ENT_TYPE(ent) == 5)
			pos[1] = pos[1] - TU_F32(ent, ENT_RADIUS);
		if (TU_PTR(ent, ENT_INST) != NULL) {
			if (TU_USE_LOW(self, TU_PTR(ent, ENT_LOW_INST)))
				inst = TU_PTR(ent, ENT_LOW_INST);
			else
				inst = TU_PTR(ent, ENT_INST);
			tu_place_instance(ent, inst, pos, 0.0f, 1);
			baPrepActorInstance((struct BActorInstance *)inst, 1);
			bdSetWorldMatrix(DAT_002855f0);
			baDrawActorInstance((struct BActorInstance *)inst, 7);
			baDrawActorInstance((struct BActorInstance *)inst, 6);
		}
		if (TU_ENT_TYPE(ent) == 1) {
			if (TU_PTR(ent, ENT_TURRET_INST) != NULL) {
				/* no debug switch for the turret */
				if (TU_PTR(ent, ENT_LOW_TURRET_INST) != NULL && TU_I32(self, ENTS_VARIANT) == 4)
					inst = TU_PTR(ent, ENT_LOW_TURRET_INST);
				else
					inst = TU_PTR(ent, ENT_TURRET_INST);
				tu_place_instance(ent, inst, pos, TU_F32(ent, 0x9a0), 0);
				bdSetWorldMatrix(DAT_002855f0);
				baPrepActorInstance((struct BActorInstance *)inst, 1);
				baDrawActorInstance((struct BActorInstance *)inst, 7);
			}
			if (TU_PTR(ent, ENT_CANNON_INST) != NULL) {
				inst = TU_PTR(ent, ENT_CANNON_INST);
				tu_place_instance(ent, inst, pos, TU_F32(ent, 0x9a0), 0);
				baPrepActorInstance((struct BActorInstance *)inst, 1);
				bdSetWorldMatrix(DAT_002855f0);
				baDrawActorInstance((struct BActorInstance *)inst, 7);
			}
		}
		if (TU_ENT_TYPE(ent) == 5)
			SMEffects_ResetDarkenStage();
	}
	/* the original reads the last entity of the list here (garbage when the list is empty) */
	if (ent != NULL && TU_ENT_TYPE(ent) == 1)
		TA_LightsEnableTank(TU_PTR(self, ENTS_LIGHTS), 1);
	TA_LightsRestore(TU_PTR(self, ENTS_LIGHTS));
}

/* 0x001d14e0  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::DrawSpecial: the entities whose template has flag 0x400 (drawn after the
others with z writes on). */
void TankEnts_DrawSpecial(void *self)
{
	void **node;
	char *ent;
	void *inst;
	float rot[16];
	int type;

	bdSetProjectionMode(1);
	bdSetRenderState(0, 1, 0);
	bdSetRenderState(1, 2, 0);
	bdSetRenderState(3, 1, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	baSetBrightness(0x100);
	TA_LightsEnable(TU_PTR(self, ENTS_LIGHTS), 0);
	CParticleEffect_Draw((struct CParticleEffect *)TU_PTR(self, ENTS_EFFECTS), 1,
		TA_WorldLaps(TU_PTR(self, ENTS_WORLD)) == 0);
	TA_LightsEnable(TU_PTR(self, ENTS_LIGHTS), 1);
	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		ent = (char *)node[0];
		type = TU_ENT_TYPE(ent);
		if (TU_I32(self, ENTS_VARIANT) >= 5 && (type == 5 || type == 9))
			continue;
		if (TU_I32(ent, ENT_DEAD) != 0 ||
			(TU_U32(TU_PTR(ent, ENT_TEMPLATE), TPL_FLAGS) & 0x400) == 0)
			continue;
		if (TU_I32(ent, 0x96c) != -1)
			TA_SetAmbient(TU_I32(ent, 0x96c), TU_I32(ent, 0x970), TU_I32(ent, 0x974));
		inst = TU_PTR(ent, ENT_INST);
		if (inst != NULL) {
			TU_F32(inst, 0xc0) = TU_F32(ent, ENT_POS);
			TU_F32(inst, 0xc4) = TU_F32(ent, ENT_POS + 4);
			TU_F32(inst, 0xc8) = TU_F32(ent, ENT_POS + 8);
			TU_F32(inst, 0xcc) = TU_F32(ent, ENT_POS + 12);
			if (TU_ENT_TYPE(ent) == 5) {
				TankDraw_SetAlphaFactor(0xaa);
				TU_F32(TU_PTR(ent, ENT_INST), 0xc4) =
					TU_F32(TU_PTR(ent, ENT_INST), 0xc4) - TU_F32(ent, ENT_RADIUS);
			}
			bmMatrixRotationYXZ(rot, TU_F32(ent, ENT_ROT),
				TU_F32(ent, ENT_ROT + 4) + 3.1415925f, TU_F32(ent, ENT_ROT + 8)); /* 0x00256750 */
			bmMatrixToQuaternion(TU_FP(TU_PTR(ent, ENT_INST), 0xe0), rot);
			inst = TU_PTR(ent, ENT_INST);
			TU_F32(inst, 0xd4) = TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_SCALE);
			TU_F32(inst, 0xd0) = TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_SCALE);
			TU_F32(inst, 0xdc) = 1.0f;
			TU_F32(inst, 0xd8) = TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_SCALE);
			baPrepActorInstance((struct BActorInstance *)TU_PTR(ent, ENT_INST), 1);
			bdSetWorldMatrix(DAT_002855f0);
			if (TU_ENT_TYPE(ent) == 3) {
				baDrawActorInstance((struct BActorInstance *)TU_PTR(ent, ENT_INST), 7);
				bdSetRenderState(3, 0, 0);
				baDrawActorInstance((struct BActorInstance *)TU_PTR(ent, ENT_INST), 6);
				bdSetRenderState(3, 1, 0);
			} else {
				baDrawActorInstance((struct BActorInstance *)TU_PTR(ent, ENT_INST), 7);
			}
		}
		if (TU_ENT_TYPE(ent) == 5)
			SMEffects_ResetDarkenStage();
		if (TU_I32(ent, 0x96c) != -1)
			TA_LightsRestore(TU_PTR(self, ENTS_LIGHTS));
	}
}

/* 0x001d1770  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::GetCameraBounds: the centre and the box of the entities whose template has
CAMLOOK, each with the point DAT_00315178 ahead of it. */
void TankEnts_GetCameraBounds(void *self, float *centre, float *maxs, float *mins)
{
	void **node;
	char *ent;
	float ahead[4];
	float offset[4];
	float f;

	centre[0] = 0.0f;
	centre[1] = 0.0f;
	centre[2] = 0.0f;
	centre[3] = 1.0f;
	maxs[0] = -999999.0f;
	maxs[1] = -999999.0f;
	maxs[2] = -999999.0f;
	maxs[3] = 1.0f;
	mins[0] = 999999.0f;
	mins[1] = 999999.0f;
	mins[2] = 999999.0f;
	mins[3] = 1.0f;
	node = (void **)TU_PTR(self, ENTS_LIST);
	if (node == NULL)
		return;
	while (TU_I32(TU_PTR(node[0], ENT_TEMPLATE), TPL_CAMLOOK) == 0) {
		node = (void **)node[1];
		if (node == NULL)
			return;
	}
	ent = (char *)node[0];
	memcpy(centre, ent + ENT_POS, 16);
	memcpy(maxs, ent + ENT_POS, 16);
	memcpy(mins, ent + ENT_POS, 16);
	for (node = (void **)node[1]; node != NULL; node = (void **)node[1]) {
		ent = (char *)node[0];
		if (TU_I32(TU_PTR(ent, ENT_TEMPLATE), TPL_CAMLOOK) == 0)
			continue;
		offset[0] = 0.0f;
		offset[1] = 0.0f;
		offset[3] = 1.0f;
		offset[2] = DAT_00315178;
		bmVectorRotateZXY(ahead, offset, TU_F32(ent, ENT_ROT + 8), -TU_F32(ent, ENT_ROT),
			TU_F32(ent, ENT_ROT + 4));
#define TU_GROW(i)                                                                      \
	maxs[i] = TU_F32(ent, ENT_POS + 4 * (i)) <= maxs[i] ? maxs[i] : TU_F32(ent, ENT_POS + 4 * (i)); \
	mins[i] = TU_F32(ent, ENT_POS + 4 * (i)) <= mins[i] ? TU_F32(ent, ENT_POS + 4 * (i)) : mins[i]; \
	f = ahead[i] + TU_F32(ent, ENT_POS + 4 * (i));                                      \
	if (f <= maxs[i])                                                                   \
		f = maxs[i];                                                                    \
	maxs[i] = f;                                                                        \
	f = ahead[i] + TU_F32(ent, ENT_POS + 4 * (i));                                      \
	if (mins[i] < f)                                                                    \
		f = mins[i];                                                                    \
	mins[i] = f;
		TU_GROW(0)
		TU_GROW(1)
		TU_GROW(2)
#undef TU_GROW
	}
	centre[0] = (maxs[0] + mins[0]) * 0.5f;
	centre[1] = (mins[1] + maxs[1]) * 0.5f;
	centre[2] = (maxs[2] + mins[2]) * 0.5f;
}

/* 0x001d1a60  TankedUp/CTank_Entities.cpp (bracketed)
The heading from an entity to a point relative to the entity's heading, wrapped (thiscall;
`self` is not used). */
float TankEnts_AngleToPoint(void *self, void *entity, float *point)
{
	float dir[3];

	(void)self;
	dir[0] = point[0] - TU_F32(entity, ENT_POS);
	dir[1] = point[1] - TU_F32(entity, ENT_POS + 4);
	dir[2] = point[2] - TU_F32(entity, ENT_POS + 8);
	return CLMath_WrapAngle(CLMath_HeadingXZ(dir) - TU_F32(entity, ENT_ROT + 4));
}

/* the due time of a message (+0x08, 64 bits) */
#define TU_MSG_DUE(msg) TU_AT(msg, 0x08, long long)

/* 0x001d1ac0  TankedUp/CTank_Entities.cpp
CTank_Entities::StoreMessage: queues a message (0x30 bytes) by due time. */
void TankEnts_StoreMessage(void *self, void *message)
{
	void **node;
	void **link;

	node = (void **)bkHeapAlloc(8, TU_FILE_ENTITIES, 0x9a9, 0x1001);
	if (node == NULL) {
		TU_DEBUG("CTank_Entities::StoreMessage - Not enough memory to create and store message in stack\n");
		return;
	}
	node[0] = message;
	TU_AT(message, 0x10, unsigned long long) = bkTimerRead();
	link = (void **)TU_ADDR(self, ENTS_MESSAGES);
	while (*link != NULL) {
		if (TU_MSG_DUE(message) < TU_MSG_DUE(((void **)*link)[0]))
			break;
		link = &((void **)*link)[1];
	}
	node[1] = *link;
	*link = node;
}

/* 0x001d1b40  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::RemoveMessages: drops the queued messages of type `type` for entity `id`. */
void TankEnts_RemoveMessages(void *self, int id, int type)
{
	void **link = (void **)TU_ADDR(self, ENTS_MESSAGES);
	void **node;

	while (*link != NULL) {
		node = (void **)*link;
		if (TU_I32(node[0], 0) == type && TU_I32(node[0], 0x1c) == id) {
			*link = node[1];
			bkHeapFree(node[0]);
			bkHeapFree(node);
		} else {
			link = &node[1];
		}
	}
}

/* 0x001d1b90  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::ProcessMessages: hands out the messages that are due. */
void TankEnts_ProcessMessages(void *self)
{
	void **node;
	long long now;

	while (TU_PTR(self, ENTS_MESSAGES) != NULL) {
		node = (void **)TU_PTR(self, ENTS_MESSAGES);
		now = (long long)bkTimerRead();
		if (now < TU_MSG_DUE(node[0]))
			return;
		TU_PTR(self, ENTS_MESSAGES) = node[1];
		TA_HandleMessage(self, node[0]);
		bkHeapFree(node[0]);
		bkHeapFree(node);
	}
}

/* 0x001d1bf0  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::FlushMessages: hands out every queued message now. */
void TankEnts_FlushMessages(void *self)
{
	void **node = (void **)TU_PTR(self, ENTS_MESSAGES);
	void **next;

	TU_PTR(self, ENTS_MESSAGES) = NULL;
	while (node != NULL) {
		next = (void **)node[1];
		TA_HandleMessage(self, node[0]);
		bkHeapFree(node[0]);
		bkHeapFree(node);
		node = next;
	}
}

/* the entity with id `id`, or NULL */
static char *tu_find_entity(void *self, int id)
{
	void **node;

	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		if (TU_I32(node[0], ENT_ID) == id)
			return (char *)node[0];
	}
	return NULL;
}

/* 0x001d1c30  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::GetEntityInfo: position (raised to the turret), velocity, rotation, radius
and +0x9a4 of entity `id`; 0 when there is none. */
int TankEnts_GetEntityInfo(void *self, int id, float *pos, float *vel, float *rot, float *radius,
	float *turret)
{
	char *ent = tu_find_entity(self, id);

	if (ent == NULL)
		return 0;
	memcpy(pos, ent + ENT_POS, 16);
	pos[1] = (pos[1] - TU_F32(ent, ENT_RADIUS)) + 60.0f; /* 0x00254e98 */
	memcpy(vel, ent + ENT_VEL, 16);
	memcpy(rot, ent + ENT_ROT, 16);
	*radius = TU_F32(ent, ENT_RADIUS);
	*turret = TU_F32(ent, 0x9a4);
	return 1;
}

/* 0x001d1d10  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::GetEntityPos: the position of entity `id` (unchanged when there is none). */
void TankEnts_GetEntityPos(void *self, int id, float *pos)
{
	char *ent = tu_find_entity(self, id);

	if (ent != NULL)
		memcpy(pos, ent + ENT_POS, 16);
}

/* 0x001d1d60  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::Push: adds a push of `amount` along the heading of entity `id` (with its
force; times its speed-up +0x99c unless `raw`). */
void TankEnts_Push(void *self, int id, float amount, int raw)
{
	char *ent = tu_find_entity(self, id);
	float push[4];
	float force[3];

	if (ent == NULL)
		return;
	if ((TU_I32(ent, 0x93c) == 0 && raw == 0) || TU_I32(ent, ENT_DEAD) != 0)
		return;
	if (TU_I32(ent, 0x944) != 0 || amount == 0.0f)
		return;
	if (raw == 0)
		push[2] = TU_F32(ent, 0x99c) * TU_F32(ent, 0x988);
	else
		push[2] = TU_F32(ent, 0x988);
	push[2] = amount * push[2];
	push[0] = 0.0f;
	push[1] = 0.0f;
	push[3] = 1.0f;
	bmVectorRotateZXY(force, push, TU_F32(ent, ENT_ROT + 8), -TU_F32(ent, ENT_ROT),
		TU_F32(ent, ENT_ROT + 4));
	TU_F32(ent, ENT_FORCE) = force[0] + TU_F32(ent, ENT_FORCE);
	TU_F32(ent, ENT_FORCE + 4) = force[1] + TU_F32(ent, ENT_FORCE + 4);
	TU_F32(ent, ENT_FORCE + 8) = force[2] + TU_F32(ent, ENT_FORCE + 8);
	TU_I32(ent, 0x978) = 0;
}

/* 0x001d1e80  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::DriveTracks: drives tank `id` with left and right track speeds: the mean
pushes (halved backwards, limited by the speed-up), the difference turns. */
void TankEnts_DriveTracks(void *self, int id, float left, float right)
{
	char *ent = tu_find_entity(self, id);
	float speed;
	float turn;
	float limit;

	/* the original reads the entity at address 0 when there is none */
	speed = (left + right) * 0.5f;
	if (speed < 0.0f)
		speed = speed * 0.5f;
	turn = (right - left) * 0.5f;
	if (speed > 0.0f && ent != NULL) {
		limit = DAT_00315158 * TU_F32(self, ENTS_DT) + TU_F32(ent, 0x9c0);
		if (limit < speed)
			speed = limit;
	}
	if (TU_I32(self, ENTS_VARIANT) < 5) {
		TA_PlayersSetSpeed(TU_PTR(self, ENTS_PLAYERS), speed, id);
		if (TU_I32(self, ENTS_VARIANT) == 4 && ent != NULL) {
			TU_F32(ent, 0x9c0) = speed;
			TankEnts_Push(self, id, speed, 0);
			TA_TurnEntity(self, id, turn);
			TA_PlayersSetSpeed(TU_PTR(self, ENTS_PLAYERS), speed, id);
			TU_F32(ent, 0x9c8) = right + TU_F32(ent, 0x9c8);
			TU_F32(ent, 0x9c4) = left + TU_F32(ent, 0x9c4);
		}
	}
}

/* 0x001d1f90  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::DriveAnalog: drives tank `id` towards stick angle `angle` (0-2 pi, relative
to the tank) at `speed`: inside the turn band (DAT_003150b4 degrees, pi in single-human
mode) ahead it curves, else it spins on the spot. */
void TankEnts_DriveAnalog(void *self, int id, float angle, float speed)
{
	float band = DAT_003150b4 * 0.0174532924f; /* 0x002640f8 */
	float f;

	if (TankChar_SingleHumanMode(NULL) != 0)
		band = tu_bitsf(0x40490fdb); /* pi */
	if (!(angle < 0.0f) && angle < band) {
		f = 1.0f - angle / band;
		TankEnts_DriveTracks(self, id, speed, ((f * f + f * f) - 1.0f) * speed);
	}
	if (!(angle < band) && angle < 3.1415925f) /* 0x00256750 */
		TankEnts_DriveTracks(self, id, speed, -speed);
	if (!(angle < 3.1415925f) && angle < 6.28318548f - band) /* 0x00255930 */
		TankEnts_DriveTracks(self, id, -speed, speed);
	if (!(angle < 6.28318548f - band) && angle <= 6.28318548f) {
		f = (angle - (6.28318548f - band)) / band;
		TankEnts_DriveTracks(self, id, ((f + f) - 1.0f) * speed, speed);
	}
}

/* 0x001d20e0  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::DriveStick: drives tank `id` with a stick (x, y) seen from the camera. */
void TankEnts_DriveStick(void *self, int id, float x, float y)
{
	char *ent = tu_find_entity(self, id);
	float size;
	float angle;

	if (ent == NULL)
		return;
	size = fabsf(x) > fabsf(y) ? fabsf(x) : fabsf(y);
	angle = (float)atan2((double)y, (double)x) - TU_F32(ent, ENT_ROT_START + 4);
	angle = TA_WrapAngle((TA_CameraHeading(TU_PTR(self, ENTS_CAMERA)) + angle) -
		1.57079625f); /* 0x00253898 */
	if (angle < 0.0f || angle > 6.28318548f)
		TU_DEBUG("Aabs error:  %f\n", (double)angle);
	TankEnts_DriveAnalog(self, id, angle, size);
}

/* 0x001d2200  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::DriveAnalogLinear: like DriveAnalog with four linear quarter turns. */
void TankEnts_DriveAnalogLinear(void *self, int id, float angle, float speed)
{
	/* 0x0027b258 = 2 / pi, 0x0027b254 = 4 / pi, 0x002630c0 = 3 pi / 2 */
	if (!(angle < 0.0f) && angle < 1.57079625f)
		TankEnts_DriveTracks(self, id, speed, (1.0f - angle * 0.636619806f) * speed);
	if (!(angle < 1.57079625f) && angle < 3.1415925f)
		TankEnts_DriveTracks(self, id, -((angle - 1.57079625f) * 0.636619806f * speed), -speed);
	if (!(angle < 3.1415925f) && angle < 4.71238899f)
		TankEnts_DriveTracks(self, id, -speed,
			-((1.0f - (angle - 3.1415925f) * 1.27323949f) * speed));
	if (!(angle < 4.71238899f) && angle <= 6.28318548f)
		TankEnts_DriveTracks(self, id, (angle - 4.71238899f) * 0.636619806f * speed, speed);
}

/* 0x001d2340  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::DriveStickLinear: DriveStick with DriveAnalogLinear. */
void TankEnts_DriveStickLinear(void *self, int id, float x, float y)
{
	char *ent = tu_find_entity(self, id);
	float size;
	float angle;

	/* the original reads the entity at address 0 when there is none */
	if (ent == NULL)
		return;
	size = fabsf(x) > fabsf(y) ? fabsf(x) : fabsf(y);
	angle = TA_CameraHeading(TU_PTR(self, ENTS_CAMERA));
	angle = TA_WrapAngle((((float)atan2((double)y, (double)x) - TU_F32(ent, ENT_ROT_START + 4)) +
		angle) - 1.57079625f);
	if (angle < 0.0f || angle > 6.28318548f)
		TU_DEBUG("Aabs error:  %f\n", (double)angle);
	TankEnts_DriveAnalogLinear(self, id, angle, size);
}

/* a sphere and a hit record of the engine's collision tests (engine_misc layouts) */
struct TankSphere {
	float centre[4];
	float radius;
};
struct TankHit {
	unsigned int flags;
	float t;
	float rest[11];
};

/* 0x001d2460  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::RayFraction: the fraction (0-1) of the segment from `from` to `to` that is
clear of the tanks (except tank `ignore`) and of the world. */
float TankEnts_RayFraction(void *self, float *from, float *to, int ignore)
{
	void **node;
	char *ent;
	float segment[8];
	struct TankSphere sphere;
	struct TankHit hit;
	float best;
	float world;

	segment[4] = to[0] - from[0];
	segment[0] = from[0];
	segment[2] = from[2];
	segment[5] = to[1] - from[1];
	segment[1] = from[1];
	segment[3] = 1.0f;
	segment[6] = to[2] - from[2];
	segment[7] = 1.0f;
	best = 1.0f;
	hit.flags = 1;
	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		ent = (char *)node[0];
		if (TU_I32(ent, ENT_ID) == ignore || TU_ENT_TYPE(ent) != 1)
			continue;
		sphere.centre[0] = TU_F32(ent, ENT_POS);
		sphere.centre[1] = TU_F32(ent, ENT_POS + 4);
		sphere.centre[2] = TU_F32(ent, ENT_POS + 8);
		sphere.centre[3] = 1.0f;
		sphere.radius = TU_F32(node[0], ENT_RADIUS);
		if (TU_SegmentSphere(segment, (float *)&sphere, &hit) != 0 && hit.t < best)
			best = hit.t;
	}
	world = TA_WorldRayLength(TU_PTR(self, ENTS_WORLD), from, to);
	if (best <= world)
		world = best;
	return world;
}

/* 0x001d2590  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::UpdateTracks: ages the track decals and, once a second (60 frames of
time), adds a point under every living tank. */
void TankEnts_UpdateTracks(void *self)
{
	void **node;
	char *ent;
	float pos[4];
	float normal[4];
	float dir[4];
	float t = TU_F32(self, ENTS_TRACK_TIME);

	TU_F32(self, ENTS_TRACK_TIME) = TU_F32(self, ENTS_DT) + t;
	if (TU_F32(self, ENTS_DT) + t < 60.0f) /* 0x00254e98 */
		return;
	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		ent = (char *)node[0];
		if (TU_PTR(ent, ENT_TRACKS) == NULL)
			continue;
		BCarTrack_Update((struct BCarTrack *)TU_PTR(ent, ENT_TRACKS), TU_F32(self, ENTS_DT));
		if (TU_I32(ent, ENT_DEAD) == 0) {
			pos[0] = TU_F32(ent, ENT_POS);
			pos[2] = TU_F32(ent, ENT_POS + 8);
			pos[3] = TU_F32(ent, ENT_POS + 12);
			normal[0] = 0.0f;
			normal[1] = 1.0f;
			pos[1] = (TU_F32(ent, ENT_POS + 4) - TU_F32(ent, ENT_RADIUS)) + 2.0f; /* 0x00257318 */
			normal[2] = 0.0f;
			normal[3] = 1.0f;
			dir[0] = 0.0f;
			dir[1] = 0.0f;
			dir[2] = 1.0f;
			dir[3] = 1.0f;
			bmVectorRotateZXY(dir, dir, TU_F32(ent, ENT_ROT + 8), -TU_F32(ent, ENT_ROT),
				TU_F32(ent, ENT_ROT + 4));
			TankDecal_AddPoint(TU_PTR(ent, ENT_TRACKS), pos, normal, dir, 0.5f);
		} else {
			BCarTrack_End((struct BCarTrack *)TU_PTR(ent, ENT_TRACKS));
		}
	}
}

/* 0x001d26e0  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::KillBuriedTanks: a tank whose top (10000 up) cannot see its centre is dead. */
void TankEnts_KillBuriedTanks(void *self)
{
	void **node;
	char *ent;
	float top[4];

	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		ent = (char *)node[0];
		if (TU_ENT_TYPE(ent) != 1)
			continue;
		top[0] = TU_F32(ent, ENT_POS);
		top[2] = TU_F32(ent, ENT_POS + 8);
		top[1] = TU_F32(ent, ENT_POS + 4) + 10000.0f; /* 0x002657a0 */
		top[3] = 1.0f;
		if (TA_WorldHitSphere(TU_PTR(self, ENTS_WORLD), top, TU_FP(ent, ENT_POS),
			TU_F32(ent, ENT_RADIUS), 1) != 0)
			TU_I32(ent, ENT_DEAD) = 1;
	}
}

/* 0x001d2770  TankedUp/CTank_Entities.cpp
CTank_Entities::LinkEntityTemplates: drops templates without a name or mass, links every
entity to its template by name and loads it. */
void TankEnts_LinkEntityTemplates(void *self)
{
	void **link = (void **)TU_ADDR(self, ENTS_TEMPLATES);
	char *tpl;
	void **node;

	while (*link != NULL) {
		tpl = (char *)*link;
		if (tpl[TPL_NAME] == 0 || TU_F32(tpl, TPL_MASS) <= 0.0f) {
			if (TU_F32(tpl, TPL_MASS) > 0.0f)
				TU_DEBUG("CTank_Entities::LinkEntityTemplates - Template defined with no name, ignoring\n");
			else
				TU_DEBUG("CTank_Entities::LinkEntityTemplates - Template [%s] defined with no mass, ignoring\n",
					tpl + TPL_NAME);
			*link = TU_PTR(tpl, TPL_NEXT);
			bkHeapFree(tpl);
		} else {
			link = (void **)TU_ADDR(tpl, TPL_NEXT);
		}
	}
	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		TU_PTR(node[0], ENT_TEMPLATE) = NULL;
		if (TU_STR(node[0], ENT_TEMPLATE_NAME)[0] != 0) {
			for (tpl = (char *)TU_PTR(self, ENTS_TEMPLATES); tpl != NULL;
				tpl = (char *)TU_PTR(tpl, TPL_NEXT)) {
				if (_stricmp(tpl + TPL_NAME, TU_STR(node[0], ENT_TEMPLATE_NAME)) == 0) {
					TU_PTR(node[0], ENT_TEMPLATE) = tpl;
					break;
				}
			}
		}
		TankEnts_EntityLoadTemplate(self, node[0]);
	}
}

/* 0x001d2880  TankedUp/CTank_Entities.cpp
CTank_Entities::SendDelayedMessage: a message (0x30 bytes) of type `type` with six words of
arguments, due `delay` seconds from now. The words are passed as raw 32-bit values (a float
argument as its bit pattern, tu_fbits). */
void TankEnts_SendDelayedMessage(void *self, int type, unsigned int a0, unsigned int a1,
	unsigned int a2, unsigned int a3, unsigned int a4, unsigned int a5, float delay)
{
	char *msg;

	msg = (char *)bkHeapAlloc(0x30, TU_FILE_ENTITIES, 0x8cf, 0x1001);
	if (msg == NULL) {
		TU_DEBUG("CTank_Entities::SendDelayedMessage - Not enough memory to create message\n");
		return;
	}
	TU_I32(msg, 0) = type;
	TU_U32(msg, 0x18) = a0;
	TU_U32(msg, 0x1c) = a1;
	TU_U32(msg, 0x20) = a2;
	TU_U32(msg, 0x24) = a3;
	TU_U32(msg, 0x28) = a4;
	TU_U32(msg, 0x2c) = a5;
	TU_MSG_DUE(msg) = (long long)bkTimerRead() +
		TU_FTOLL((double)TU_AT(self, ENTS_TICKS, long long) * (double)delay);
	TankEnts_StoreMessage(self, msg);
}

/* 0x001d2900  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::DestroyEntity: frees an entity, its instances, token, decal and move queue,
and its list node (not unlinked here). */
void TankEnts_DestroyEntity(void *self, void **node)
{
	char *ent = (char *)node[0];
	void *p;

	TA_LightsRemove(TU_PTR(self, ENTS_LIGHTS), TU_I32(ent, 0x960));
	/* the life timer's destructor is the empty function 0x001d5950 (ECX = ent + 0xa88) */
	FUN_001d5950();
	if (TU_PTR(node[0], ENT_INST) != NULL)
		baDeleteActorInstance((struct BActorInstance *)TU_PTR(node[0], ENT_INST));
	if (TU_PTR(node[0], ENT_TURRET_INST) != NULL)
		baDeleteActorInstance((struct BActorInstance *)TU_PTR(node[0], ENT_TURRET_INST));
	if (TU_PTR(node[0], ENT_CANNON_INST) != NULL)
		baDeleteActorInstance((struct BActorInstance *)TU_PTR(node[0], ENT_CANNON_INST));
	if (TU_PTR(node[0], ENT_LOW_INST) != NULL)
		baDeleteActorInstance((struct BActorInstance *)TU_PTR(node[0], ENT_LOW_INST));
	if (TU_PTR(node[0], ENT_LOW_TURRET_INST) != NULL)
		baDeleteActorInstance((struct BActorInstance *)TU_PTR(node[0], ENT_LOW_TURRET_INST));
	if (TU_PTR(node[0], ENT_TOKEN) != NULL)
		CToken_Collect((struct CToken *)TU_PTR(node[0], ENT_TOKEN));
	p = TU_PTR(node[0], ENT_TRACKS);
	if (p != NULL) {
		BCarTrack_Destruct((struct BCarTrack *)p);
		bkHeapFree(p);
	}
	p = TU_PTR(node[0], ENT_MOVEQUE);
	if (p != NULL) {
		TankMoveQue_Free(p);
		bkHeapFree(p);
	}
	p = node[0];
	if (p != NULL) {
		TA_EntityCleanup(p);
		bkHeapFree(p);
	}
	bkHeapFree(node);
}

/* 0x001d2a00  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::CreateEntity: a new entity from template `templateName`, owned by
`owner`, at `pos` turned `rot`, pushed by `push`; its id, or 0. */
int TankEnts_CreateEntity(void *self, int owner, const char *templateName, float *pos, float *rot,
	float push)
{
	char *tpl;
	void **node;
	char *ent;
	char msg[0x30];

	for (tpl = (char *)TU_PTR(self, ENTS_TEMPLATES); tpl != NULL; tpl = (char *)TU_PTR(tpl, TPL_NEXT)) {
		if (_stricmp(tpl + TPL_NAME, templateName) == 0)
			break;
	}
	if (tpl == NULL)
		return 0;
	if (TankEnts_SpawnEntity(self, (void **)&node) == 0)
		return 0;
	ent = (char *)node[0];
	TU_I32(ent, 0x04) = owner;
	_snprintf(ent + ENT_NAME, 0x3f, "%s-%d", tpl + TPL_NAME, TU_I32(ent, ENT_ID));
	TU_PTR(ent, ENT_TEMPLATE) = tpl;
	memcpy(ent + ENT_ROT, rot, 16);
	memcpy(ent + ENT_ROT_START, rot, 16);
	memcpy(ent + ENT_POS, pos, 16);
	TankEnts_EntityLoadTemplate(self, ent);
	if (TU_I32(ent, ENT_DEAD) != 0) {
		if (TU_PTR(ent, ENT_INST) != NULL)
			baDeleteActorInstance((struct BActorInstance *)TU_PTR(ent, ENT_INST));
		if (TU_PTR(ent, ENT_TURRET_INST) != NULL)
			baDeleteActorInstance((struct BActorInstance *)TU_PTR(ent, ENT_TURRET_INST));
		if (TU_PTR(ent, ENT_CANNON_INST) != NULL)
			baDeleteActorInstance((struct BActorInstance *)TU_PTR(ent, ENT_CANNON_INST));
		TA_EntityCleanup(ent);
		bkHeapFree(ent);
		bkHeapFree(node);
		return 0;
	}
	node[1] = TU_PTR(self, ENTS_LIST);
	TU_PTR(self, ENTS_LIST) = node;
	/* the creation messages 1 and 4, handled at once (the other words are not set) */
	memset(msg, 0, sizeof msg);
	TU_I32(msg, 0x28) = 0;
	TU_I32(msg, 0x20) = 1;
	TU_I32(msg, 0x1c) = TU_I32(ent, ENT_ID);
	TU_I32(msg, 0x18) = TU_I32(ent, ENT_ID);
	TU_I32(msg, 0) = 1;
	TA_SendMessageNow(self, ent, 1, msg);
	TU_I32(msg, 0) = 4;
	TA_SendMessageNow(self, ent, 1, msg);
	if (TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_LIFE) != 0.0f) {
		TA_TimerSetLength(ent + ENT_TIMER,
			TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_LIFE));
		TA_TimerStart(ent + ENT_TIMER);
	}
	TU_I32(ent, 0x964) = 0;
	if (TU_PTR(ent, ENT_ANIM_SEG) != NULL) {
		baQueueAnimation((struct BActorInstance *)TU_PTR(ent, ENT_INST),
			(struct BActorNode *)TU_PTR(TU_PTR(ent, ENT_INST), 0x134),
			(struct BAnim *)TU_PTR(ent, ENT_ANIM_SEG), 0x400, -1, 0,
			(int (*)())FUN_001d7d60, ent);
		if (TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_LIFE) != 0.0f) {
			/* the animation's frames (1/1024 units) in seconds over the life */
			TU_F32(ent, 0x998) = ((float)(TU_I32(TU_PTR(ent, ENT_ANIM_SEG), 8) -
				TU_I32(TU_PTR(ent, ENT_ANIM_SEG), 4)) * 0.0009765625f * 0.0166666675f) /
				TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_LIFE); /* 0x002593b0, 0x00262b7c */
			goto pushed;
		}
	}
	TU_F32(ent, 0x998) = 1.0f;
pushed:
	TankEnts_Push(self, TU_I32(ent, ENT_ID), push, 1);
	return TU_I32(ent, ENT_ID);
}

/* 0x001d2c80  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::Reverse: in variant 4, pushes tank `id` back and turns its tracks back. */
void TankEnts_Reverse(void *self, int id)
{
	char *ent = tu_find_entity(self, id);

	if (ent == NULL)
		return;
	if (TU_I32(ent, ENT_DEAD) == 0 && TU_I32(ent, 0x944) == 0 && TU_I32(self, ENTS_VARIANT) == 4) {
		TankEnts_Push(self, id, -0.5f, 0);
		TU_F32(ent, 0x9c8) = TU_F32(ent, 0x9c8) - 0.5f;
		TU_F32(ent, 0x9c4) = TU_F32(ent, 0x9c4) - 0.5f;
	}
}

/* 0x001d2d00  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::EntityHit: entity `ent` touched `other`: template flag 2 sends message 6
(hit), flag 1 kills it and sends message 9 (killed by). */
void TankEnts_EntityHit(void *self, void *ent, void *other)
{
	if ((TU_U8(TU_PTR(ent, ENT_TEMPLATE), TPL_FLAGS) & 2) != 0)
		TankEnts_SendDelayedMessage(self, 6, TU_U32(ent, ENT_ID), TU_U32(other, ENT_ID), 1, 0, 0, 0,
			0.0f);
	if ((TU_U8(TU_PTR(ent, ENT_TEMPLATE), TPL_FLAGS) & 1) != 0) {
		TU_DEBUG("Kill it Kill it, It must die.\n");
		TU_I32(ent, ENT_DEAD) = 1;
		TankEnts_SendDelayedMessage(self, 9, TU_U32(other, ENT_ID), TU_U32(ent, ENT_ID), 1, 0, 0, 0,
			0.0f);
	}
}

/* the sphere of an entity for the collision tests: its position (tanks: the centre is
   lowered by the radius) and radius */
static void tu_entity_sphere(char *ent, struct TankSphere *sphere)
{
	memcpy(sphere->centre, ent + ENT_POS, 16);
	sphere->radius = TU_F32(ent, ENT_RADIUS);
	if (TU_ENT_TYPE(ent) == 1)
		sphere->centre[1] = sphere->centre[1] - TU_F32(ent, ENT_RADIUS);
}

/* n . v in the original's order */
#define TU_NDOT(n, v) ((n)[0] * (v)[0] + (n)[2] * (v)[2] + (n)[1] * (v)[1])

/* 0x001d2dc0  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::CollideEntity: sweeps the moving entity of `node` against the others (not
dead, not flag 0x80, not friendly unless +0x928) and bounces both off the nearest one;
1 when it hit. */
int TankEnts_CollideEntity(void *self, void **node)
{
	char *me;
	char *other;
	void **it;
	void **hitNode;
	void *camera;
	struct TankSphere a;
	struct TankSphere b;
	struct TankHit hit;
	float best;
	float lim;
	float *vel;
	float e, k, vm, vo, after, d, s;
	float dir[3];
	int r;

	if (node == NULL || node[0] == NULL)
		return 0;
	me = (char *)node[0];
	if (sqrtf(TU_F32(me, 0xa40) * TU_F32(me, 0xa40) + TU_F32(me, 0xa3c) * TU_F32(me, 0xa3c) +
		TU_F32(me, 0xa38) * TU_F32(me, 0xa38)) == 0.0f)
		return 0;
	if ((signed char)TU_U8(TU_PTR(me, ENT_TEMPLATE), TPL_FLAGS) < 0)
		return 0;
	tu_entity_sphere(me, &a);
	it = (void **)TU_PTR(self, ENTS_LIST);
	hitNode = NULL;
	best = 2.0f;
	camera = TU_PTR(self, ENTS_PLAYERS);
	if (it == NULL)
		return 0;
	for (; it != NULL; it = (void **)it[1]) {
		if (it == node)
			continue;
		other = (char *)it[0];
		if (TU_I32(other, ENT_DEAD) != 0 || (signed char)TU_U8(TU_PTR(other, ENT_TEMPLATE), TPL_FLAGS) < 0)
			continue;
		me = (char *)node[0];
		if (TA_IsFriendly(camera, TU_I32(me, ENT_ID), TU_I32(other, 4)) != 0 &&
			TU_I32(it[0], 0x928) == 0)
			continue;
		if (TA_IsFriendly(camera, TU_I32(it[0], ENT_ID), TU_I32(node[0], 4)) != 0 &&
			TU_I32(node[0], 0x928) == 0)
			continue;
		if (TA_IsFriendly(camera, TU_I32(it[0], 4), TU_I32(node[0], 4)) != 0 &&
			TU_I32(node[0], 4) != 0 && TU_I32(node[0], 0x928) == 0)
			continue;
		if (TA_IsFriendly(camera, TU_I32(it[0], 4), TU_I32(node[0], 4)) != 0 &&
			TU_I32(node[0], 4) != 0 && TU_I32(it[0], 0x928) == 0)
			continue;
		/* a box test along the velocity first */
		me = (char *)node[0];
		other = (char *)it[0];
		vel = TU_FP(me, ENT_VEL);
		lim = 0.0f < vel[0] ? vel[0] : 0.0f;
		if (!(TU_F32(other, ENT_POS) <= TU_F32(me, ENT_POS) + TU_F32(me, ENT_RADIUS) +
			TU_F32(other, ENT_RADIUS) + lim))
			continue;
		lim = vel[0] <= 0.0f ? vel[0] : 0.0f;
		if (!(((TU_F32(me, ENT_POS) - TU_F32(me, ENT_RADIUS)) - TU_F32(other, ENT_RADIUS)) + lim <=
			TU_F32(other, ENT_POS)))
			continue;
		lim = 0.0f < vel[2] ? vel[2] : 0.0f;
		if (!(TU_F32(other, ENT_POS + 8) <= TU_F32(me, ENT_RADIUS) + TU_F32(other, ENT_RADIUS) + lim +
			TU_F32(me, ENT_POS + 8)))
			continue;
		lim = vel[2] <= 0.0f ? vel[2] : 0.0f;
		if (!(((TU_F32(me, ENT_POS + 8) - TU_F32(me, ENT_RADIUS)) - TU_F32(other, ENT_RADIUS)) + lim <=
			TU_F32(other, ENT_POS + 8)))
			continue;
		tu_entity_sphere(other, &b);
		hit.flags = 1;
		if (TU_SweepSphereSphere((float *)&a, (float *)&b, vel, &hit) != 0 && hit.t < best) {
			best = hit.t;
			hitNode = it;
		}
	}
	if (hitNode == NULL)
		return 0;

	me = (char *)node[0];
	other = (char *)hitNode[0];
	TU_I32(other, 0x978) = 0;
	tu_entity_sphere(other, &b);
	hit.flags = 5;
	TU_SweepSphereSphere((float *)&a, (float *)&b, TU_FP(me, ENT_VEL), &hit);
	if (TU_I32(me, 0x9d4) != TU_I32(other, ENT_ID)) {
		/* a new contact */
		TU_I32(me, 0x9d4) = TU_I32(other, ENT_ID);
		TU_I32(other, 0x9d4) = TU_I32(me, ENT_ID);
		TankEnts_EntityHit(self, me, other);
		TankEnts_EntityHit(self, other, me);
		if ((TU_ENT_TYPE(me) == 1 || TU_ENT_TYPE(other) == 1) &&
			(TU_ENT_TYPE(me) == 9 || TU_ENT_TYPE(other) == 9 || TU_ENT_TYPE(me) == 10 ||
			TU_ENT_TYPE(other) == 10)) {
			TU_DEBUG("tank hits a rock\n");
			r = tu_rand() % 3;
			if (r == 0)
				TankSound_Play(TU_PTR(self, ENTS_SOUND), 0x16, TU_FP(me, ENT_POS), 200, 1, 0);
			else if (r == 1)
				TankSound_Play(TU_PTR(self, ENTS_SOUND), 0x17, TU_FP(me, ENT_POS), 200, 1, 0);
			else if (r == 2)
				TankSound_Play(TU_PTR(self, ENTS_SOUND), 0x18, TU_FP(me, ENT_POS), 200, 1, 0);
		}
		if (TU_ENT_TYPE(me) == 1 && TU_ENT_TYPE(other) == 1 &&
			(TU_I32(me, 0x920) != 0 || TU_I32(other, 0x920) != 0)) {
			TankSound_Play(TU_PTR(self, ENTS_SOUND), 0x1b, TU_FP(me, ENT_POS), 200, 1, 0);
			TU_I32(me, 0x920) = 0;
			TU_I32(other, 0x920) = 0;
			TankEnts_SendDelayedMessage(self, 0x18, TU_U32(me, ENT_ID), TU_U32(me, ENT_ID), 1, 0, 0,
				0, 1.0f);
			TankEnts_SendDelayedMessage(self, 0x18, TU_U32(other, ENT_ID), TU_U32(other, ENT_ID), 1, 0,
				0, 0, 1.0f);
		}
	}

	/* the bounce: the normal parts of the velocities exchanged by mass and restitution */
	me = (char *)node[0];
	other = (char *)hitNode[0];
	e = TU_F32(TU_PTR(me, ENT_TEMPLATE), TPL_RESTITUTION) *
		TU_F32(TU_PTR(other, ENT_TEMPLATE), TPL_RESTITUTION) * 0.5f + 0.5f;
	vm = TU_NDOT(&hit.rest[2], TU_FP(me, ENT_VEL)) * TU_F32(me, ENT_MASS);
	k = 1.0f - e;
	vo = TU_NDOT(&hit.rest[2], TU_FP(other, ENT_VEL)) * TU_F32(other, ENT_MASS);
	after = (k * vm + vo * e) / TU_F32(me, ENT_MASS);
	d = TU_NDOT(&hit.rest[2], TU_FP(me, ENT_VEL));
	d = d + d;
	dir[0] = hit.rest[2] * d;
	dir[1] = hit.rest[3] * d;
	dir[2] = hit.rest[4] * d;
	bmVectorNormalise(dir, dir);
	s = -((e * vm + vo * k) / TU_F32(other, ENT_MASS));
	TU_F32(other, 0xa38) = dir[0] * s;
	TU_F32(other, 0xa3c) = dir[1] * s;
	TU_F32(other, 0xa40) = dir[2] * s;
	d = TU_NDOT(&hit.rest[2], TU_FP(me, ENT_VEL));
	d = d + d;
	dir[0] = TU_F32(me, 0xa38) - hit.rest[2] * d;
	dir[1] = TU_F32(me, 0xa3c) - hit.rest[3] * d;
	dir[2] = TU_F32(me, 0xa40) - hit.rest[4] * d;
	bmVectorNormalise(dir, dir);
	TU_F32(me, 0xa38) = -dir[0] * after;
	TU_F32(me, 0xa3c) = -dir[1] * after;
	TU_F32(me, 0xa40) = -dir[2] * after;
	return 1;
}

/* the player index (0-3) of an entity named "player1".."player4", else `fallback` */
static int tu_player_index(const char *name, int fallback)
{
	int index = fallback;

	if (strcmp(name, "player1") == 0)
		index = 0;
	if (strcmp(name, "player2") == 0)
		index = 1;
	if (strcmp(name, "player3") == 0)
		index = 2;
	if (strcmp(name, "player4") == 0)
		index = 3;
	return index;
}

/* 0x001d35e0  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::CheckWaypoints: the race laps of a tank: passing the line in the right way
counts a lap; after the last one the tank turns into "player - Race Done" (or explodes in
its colour when there is no such template). */
void TankEnts_CheckWaypoints(void *self, void *entity)
{
	char *ent = (char *)entity;
	unsigned int flags;
	int player;
	int character;
	unsigned int red, green, blue;
	char *tpl;

	if (TA_WorldCheckLine(TU_PTR(self, ENTS_WORLD), TU_FP(ent, ENT_POS), TU_FP(ent, ENT_VEL),
		&flags) == -1)
		return;
	if (TU_I32(ent, ENT_LAP) == TA_WorldLaps(TU_PTR(self, ENTS_WORLD))) {
		TU_DEBUG("Laped Out\n");
		return;
	}
	TU_DEBUG("Waypoint: %s has gone thow waypoint num: %d on lap %d\n", ent + ENT_NAME, 0,
		TU_I32(ent, ENT_LAP));
	TU_DEBUG((flags & 4) != 0 ? " right way.   \n" : " WRONG WAY!!   \n");
	if ((flags & 2) == 0) {
		TU_DEBUG("lap %d\n", TU_I32(ent, ENT_LAP));
		return;
	}
	if ((flags & 4) == 0)
		TU_I32(ent, ENT_LAP) = TU_I32(ent, ENT_LAP) - 1;
	else
		TU_I32(ent, ENT_LAP) = TU_I32(ent, ENT_LAP) + 1;
	if (TU_I32(ent, ENT_LAP) == TA_WorldLaps(TU_PTR(self, ENTS_WORLD))) {
		/* the original starts from the flags word when the name is no player's */
		player = tu_player_index(ent + ENT_NAME, (int)flags);
		character = DAT_002baed4[player * 10] * 0x60;
		red = DAT_00290718[character];
		green = DAT_00290719[character];
		blue = DAT_0029071a[character];
		for (tpl = (char *)TU_PTR(self, ENTS_TEMPLATES); tpl != NULL;
			tpl = (char *)TU_PTR(tpl, TPL_NEXT)) {
			if (_stricmp(tpl + TPL_NAME, "player - Race Done") == 0)
				break;
		}
		if (tpl != NULL) {
			TU_I32(ent, ENT_HEALTH) = TU_I32(tpl, TPL_HEALTH);
			TU_I32(ent, ENT_MINES) = TU_I32(tpl, TPL_MINES);
			TU_I32(ent, ENT_MASS) = TU_I32(tpl, TPL_MASS);
			TU_I32(ent, ENT_AI) = TU_I32(tpl, TPL_AI);
			TU_I32(ent, ENT_DEAD) = 0;
			TU_I32(ent, 0x95c) = 0;
			TU_PTR(ent, ENT_TEMPLATE) = tpl;
			TankEnts_SendDelayedMessage(self, 1, TU_U32(ent, ENT_ID), TU_U32(ent, ENT_ID), 1, 0, 0, 0,
				0.0f);
		} else {
			TU_I32(ent, 0x95c) = 1;
			TU_I32(ent, ENT_DEAD) = 1;
			TA_DebrisAdd(TU_PTR(self, ENTS_DEBRIS), 10, TU_FP(ent, ENT_POS), DAT_002ea478, 0,
				((((red & 0xff) << 8) + (green & 0xff)) << 8) + (blue & 0xff));
		}
	}
	TU_DEBUG("lap %d\n", TU_I32(ent, ENT_LAP));
}

/* 0x001d3850  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::UpdatePhysics: for every live entity (not flag 0x200, not resting): eases
the rotation to its target, applies forces, gravity and ground friction, then moves it
through the world (up to 10 collision steps) and against the other entities. Tanks check
their laps; `variant` >= 5 freezes the positions. */
void TankEnts_UpdatePhysics(void *self, int variant)
{
	void **node;
	char *ent;
	float *rot;
	float d, lim;
	float fx, fy, fz;
	float dt;
	float inv;
	int i;
	int steps;
	int moved;
	float pos[4];
	float vel[4];
	float normal[4];
	float turned[4];
	unsigned int hitFlags;
	int onGround;
	int noKill;
	float friction;
	float v;

	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		ent = (char *)node[0];
		if ((TU_U32(TU_PTR(ent, ENT_TEMPLATE), TPL_FLAGS) & 0x200) != 0 || TU_I32(ent, ENT_DEAD) != 0 ||
			TU_I32(ent, 0x978) != 0)
			continue;
		memcpy(ent + ENT_OLD_VEL, ent + ENT_VEL, 16);
		rot = TU_FP(ent, ENT_ROT);
		for (i = 0; i < 3; i++) {
			if (rot[i] != rot[i + 4]) {
				d = rot[i + 4] - rot[i];
				if (d > 3.1415925f) /* 0x00256750 */
					d = d - 6.28318548f;
				if (d < -3.1415925f) /* 0x00253f3c */
					d = d + 6.28318548f;
				lim = TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_ROT) * TU_F32(self, ENTS_DT);
				if (d > lim)
					d = lim;
				if (d < -lim)
					d = -lim;
				rot[i] = CLMath_WrapAngle(d + rot[i]);
			}
		}
		fx = TU_F32(ent, ENT_FORCE);
		fz = TU_F32(ent, ENT_FORCE + 8);
		fy = TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_GRAVITY) * TU_F32(ent, ENT_MASS) +
			TU_F32(ent, ENT_FORCE + 4);
		TU_F32(ent, ENT_FORCE) = 0.0f;
		TU_F32(ent, ENT_FORCE + 4) = 0.0f;
		TU_F32(ent, ENT_FORCE + 8) = 0.0f;
		TU_F32(ent, ENT_FORCE + 12) = 1.0f;
		dt = TU_F32(self, ENTS_DT);
		fx = dt * fx;
		fy = dt * fy;
		fz = dt * fz;
		if (TU_F32(ent, ENT_FRICTION) != 0.0f && TU_I32(ent, 0x9d4) == 0) {
			fx = fx - TU_F32(ent, ENT_MASS) * TU_F32(ent, 0xa38) * TU_F32(ent, ENT_FRICTION);
			fy = fy - TU_F32(ent, 0xa3c) * TU_F32(ent, ENT_MASS) * TU_F32(ent, ENT_FRICTION);
			fz = fz - TU_F32(ent, 0xa40) * TU_F32(ent, ENT_MASS) * TU_F32(ent, ENT_FRICTION);
		}
		TU_I32(ent, 0x9d4) = 0;
		inv = 1.0f / TU_F32(ent, ENT_MASS);
		TU_I32(ent, ENT_ON_GROUND) = 0;
		TU_U32(ent, ENT_FRICTION) = 0xc47a0000; /* -1000 */
		TU_I32(ent, 0x940) = 0;
		TU_F32(ent, 0xa38) = inv * fx + TU_F32(ent, 0xa38);
		TU_F32(ent, 0xa3c) = inv * fy + TU_F32(ent, 0xa3c);
		TU_F32(ent, 0xa40) = inv * fz + TU_F32(ent, 0xa40);

		moved = 0;
		steps = 0;
		for (;;) {
			memcpy(pos, ent + ENT_POS, 16);
			steps++;
			moved = 0;
			memcpy(vel, ent + ENT_VEL, 16);
			hitFlags = TU_U32(TU_PTR(ent, ENT_TEMPLATE), TPL_FLAGS);
			if (TA_WorldMove(TU_PTR(self, ENTS_WORLD), pos, vel, TU_FP(ent, ENT_ROT),
				TU_F32(ent, ENT_RADIUS), &hitFlags, TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_RESTITUTION),
				&onGround, &friction, TU_FP(ent, 0x9e8), &noKill) == 0) {
				if (TankEnts_CollideEntity(self, node) == 0)
					goto done;
				moved = 1;
				if (TU_PTR(ent, ENT_TRACKS) != NULL)
					BCarTrack_End((struct BCarTrack *)TU_PTR(ent, ENT_TRACKS));
				if (TU_ENT_TYPE(node[0]) == 10 && TankEnts_CollideEntity(self, node) != 0 &&
					TankEnts_CollideEntity(self, node) != 0)
					TU_I32(node[0], ENT_DEAD) = 1;
				TA_PlayersShake(TU_PTR(self, ENTS_PLAYERS),
					TU_I32(ent, ENT_ID), 2);
				break;
			}
			moved = 1;
			if (onGround != 0)
				TU_I32(ent, ENT_ON_GROUND) = 1;
			if (friction > TU_F32(ent, ENT_FRICTION))
				TU_F32(ent, ENT_FRICTION) = friction;
			TU_U32(ent, 0x940) = TU_U32(ent, 0x940) | hitFlags;
			memcpy(ent + ENT_VEL, vel, 16);
			if (TU_I32(ent, ENT_ON_GROUND) != 0) {
				/* lean the entity to the ground */
				TA_WorldGroundHeight(TU_PTR(self, ENTS_WORLD), TU_FP(ent, ENT_POS),
					TU_F32(ent, ENT_RADIUS), normal);
				normal[3] = 1.0f;
				bmVectorNormalise(normal, normal);
				TU_F32(ent, 0x99c) = normal[1];
				bmVectorRotateY(turned, normal, -TU_F32(ent, ENT_ROT + 4));
				TU_F32(ent, ENT_ROT_START) = CLMath_WrapAngle(-BCarMath_Atan2(turned[2], turned[1]));
				TU_F32(ent, ENT_ROT_START + 8) = CLMath_WrapAngle(BCarMath_Atan2(turned[0], turned[1]));
			}
			if ((TU_U32(TU_PTR(ent, ENT_TEMPLATE), TPL_FLAGS) & 1) != 0 &&
				((TU_U32(TU_PTR(ent, ENT_TEMPLATE), TPL_FLAGS) & 0x100) == 0 || noKill == 0)) {
				TU_I32(ent, ENT_DEAD) = 1;
				TankEnts_SendDelayedMessage(self, 9, 0, TU_U32(ent, ENT_ID), 1, 0, 0, 0, 0.0f);
			}
			if (steps >= 10)
				break;
		}
	done:
		if ((TU_U8(TU_PTR(ent, ENT_TEMPLATE), TPL_FLAGS) & 4) != 0) {
			/* face along the velocity */
			TankEnts_DirToAngles(TU_FP(ent, ENT_VEL), TU_FP(ent, ENT_ROT_START),
				TU_FP(ent, ENT_ROT_START + 4));
			TU_F32(ent, ENT_ROT_START) = TU_F32(ent, ENT_ROT_START) + 3.1415925f;
			TU_F32(ent, ENT_ROT) = TU_F32(ent, ENT_ROT_START);
		}
		if (TU_I32(ent, ENT_ON_GROUND) == 0)
			TU_F32(ent, 0x99c) = 0.0f;
		if (TU_F32(ent, ENT_FRICTION) < -999.0f) /* 0x0027b454 */
			TU_F32(ent, ENT_FRICTION) = 0.0f;
		if (TU_ENT_TYPE(ent) == 1) {
			if (moved || variant >= 5)
				goto rest;
			if (TA_WorldLaps(TU_PTR(self, ENTS_WORLD)) != 0)
				TankEnts_CheckWaypoints(self, ent);
		} else if (moved) {
			goto rest;
		}
		if (variant < 5) {
			TU_F32(ent, ENT_POS) = TU_F32(ent, ENT_POS) + TU_F32(ent, 0xa38);
			TU_F32(ent, ENT_POS + 4) = TU_F32(ent, ENT_POS + 4) + TU_F32(ent, 0xa3c);
			TU_F32(ent, ENT_POS + 8) = TU_F32(ent, ENT_POS + 8) + TU_F32(ent, 0xa40);
		}
	rest:
		v = sqrtf(TU_F32(ent, 0xa40) * TU_F32(ent, 0xa40) + TU_F32(ent, 0xa3c) * TU_F32(ent, 0xa3c) +
			TU_F32(ent, 0xa38) * TU_F32(ent, 0xa38));
		if (v < 0.2f && TU_I32(ent, ENT_ON_GROUND) != 0 && /* 0x00257c7c */
			(TU_U8(TU_PTR(ent, ENT_TEMPLATE), TPL_TYPE) & 1) == 0) {
			TU_I32(ent, 0x978) = 1;
			TU_DEBUG("object '%s' is going into a stable state\n", ent + ENT_NAME);
		}
	}
}

/* 0x001d3e90  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::SetVariant: the game state (`variant`; `unused` is ignored). 4 (playing)
arms every entity (messages 10 after its reload time and 0x17 after 300 s); above 4 the
messages are flushed and +0x948 cleared; 5 (the end) puts the players at their "Exit Cam"
points. */
void TankEnts_SetVariant(void *self, int unused, int variant)
{
	void **node;
	float *exit;
	char *ent;
	int player;

	(void)unused;
	TU_I32(self, ENTS_VARIANT) = variant;
	if (variant == 4) {
		for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
			TU_I32(node[0], 0x938) = 0;
			TankEnts_SendDelayedMessage(self, 10, TU_U32(node[0], ENT_ID), TU_U32(node[0], ENT_ID), 1, 0,
				0, 0, TU_F32(TU_PTR(node[0], ENT_TEMPLATE), TPL_RELOAD));
			TankEnts_SendDelayedMessage(self, 0x17, TU_U32(node[0], ENT_ID), TU_U32(node[0], ENT_ID), 1,
				0, 0, 0, 300.0f);
		}
		return;
	}
	if (variant > 4) {
		TankEnts_FlushMessages(self);
		for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1])
			TU_I32(node[0], 0x948) = 0;
	}
	if (variant != 5)
		return;
	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		player = tu_player_index(TU_STR(node[0], ENT_NAME), -1);
		if (player == -1)
			continue;
		exit = TA_SpawnFind(TU_PTR(self, ENTS_SPAWN), "Exit Cam", player);
		if (exit == NULL)
			continue;
		ent = (char *)node[0];
		memcpy(ent + ENT_POS, exit, 16);
		memcpy(ent + ENT_ROT, exit + 4, 16);
		memcpy(ent + ENT_ROT_START, exit + 4, 16);
	}
}

/* 0x001d40a0  TankedUp/CTank_Entities.cpp (bracketed)
CTank_Entities::ScatterRocks: puts `count` rocks at random "B_List" points that are more
than 300 from every entity and 2000 from every tank. */
void TankEnts_ScatterRocks(void *self, int count)
{
	float rot[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float point[4];
	void **node;
	char *ent;
	float dx, dz, dist;
	float nearest;
	float nearestTank;

	while (count >= 1) {
		TA_SpawnRandom(TU_PTR(self, ENTS_SPAWN), "B_List", point);
		node = (void **)TU_PTR(self, ENTS_LIST);
		nearestTank = 1e8f;
		nearest = 1e8f; /* 0x00268784 */
		if (node != NULL) {
			for (; node != NULL; node = (void **)node[1]) {
				ent = (char *)node[0];
				dx = TU_F32(ent, ENT_POS) - point[0];
				dz = TU_F32(ent, ENT_POS + 8) - point[2];
				dist = dx * dx + dz * dz;
				if (dist <= nearest)
					nearest = dist;
				if (TU_ENT_TYPE(ent) == 1 && dist < nearestTank)
					nearestTank = dist;
			}
			if (!(nearest > 90000.0f && nearestTank > 4000000.0f)) /* 0x0026b028, 0x0027b46c */
				continue;
		}
		count = count - 1;
		TankEnts_CreateEntity(self, 0, "rock", point, rot, 0.0f); /* 0x0027b464 */
	}
}

/* 0x001d41d0  TankedUp/CTank_Entities.cpp
CTank_Entities::Initialise: loads the shared textures and effects, reads the entity file
`name` from `package`, links the templates, scatters rocks in race mode and adds the tweak
menus. 0 when the file is missing. */
int TankEnts_Initialise(void *self, void *package, const char *name, void *world, void *route,
	void *spawn, void *debris, void *lights, void *sound, void *boom)
{
	char reader[0x214];
	unsigned int size;
	void *text;
	void *p;
	char *tpl;
	int i;

	TankReader_Ctor(reader);
	TU_I32(self, 0xe4) = 0;
	CToken_LoadAssets(1, NULL);
	for (i = 0xa8; i < 0xb8; i += 4)
		TU_F32((void *)(intptr_t)DAT_002ecfac, i) = 20.0f;
	RefBot_Create();
	TU_I32(self, 0x64) = 0;
	p = bkHeapAlloc(0x28, TU_FILE_ENTITIES, 0xae, 0x1001);
	TU_PTR(self, ENTS_EFFECTS) = p != NULL ? BCarGenericGoal_Construct((struct CParticleEffect *)p, 1)
		: NULL;
	TU_PTR(self, ENTS_TEX_TRACKS) = bLoadResource(NULL, "entities\\textures_tank\\trackanim.gif", 0, 0xdefa);
	TU_PTR(self, ENTS_TEX_TRACKS + 4) = bLoadResource(NULL, "entities\\textures_tank\\trackanim2.gif", 0,
		0xdefa);
	TU_PTR(self, ENTS_TEX_SHADOW + 8) = bLoadResource(NULL, "decal\\shellshad.bmp", 0, 0xdefa);
	TU_PTR(self, ENTS_TEX_SHADOW) = bLoadResource(NULL, "decal\\shadowsq.bmp", 0, 0xdefa);
	TU_PTR(self, ENTS_TEX_SHADOW + 4) = bLoadResource(NULL, "decal\\tankshad.bmp", 0, 0xdefa);
	TU_PTR(self, ENTS_TEX_SHADOW + 12) = bLoadResource(NULL, "decal\\bombshad.bmp", 0, 0xdefa);
	TU_PTR(self, ENTS_GLOW) = bLoadResource(NULL, "tokenglow.bmp", 0, 0xdefa);
	TU_PTR(self, ENTS_TEX_TRACK_DECAL) = bLoadResource(NULL, "decal\\decal-tracks.bmp", 0, 0xdefa);
	if (TU_PTR(self, ENTS_TEX_TRACK_DECAL) == NULL)
		TU_DEBUG("could not load decal texture\n");
	TU_PTR(self, ENTS_BOOM) = boom;
	TU_F32(self, ENTS_TRACK_TIME) = 0.0f;
	memset(TU_ADDR(self, 0x98), 0, 0xb * 4);
	TU_PTR(self, ENTS_WORLD) = world;
	TU_PTR(self, ENTS_ROUTE) = route;
	TU_PTR(self, ENTS_SPAWN) = spawn;
	TU_PTR(self, ENTS_DEBRIS) = debris;
	TU_PTR(self, ENTS_LIGHTS) = lights;
	TU_PTR(self, ENTS_SOUND) = sound;
	TU_PTR(self, ENTS_MESSAGES) = NULL;
	TU_PTR(self, ENTS_LIST) = NULL;
	TU_PTR(self, ENTS_TEMPLATES) = NULL;
	TU_F32(self, ENTS_DT) = 1.0f;
	TU_AT(self, ENTS_TICKS, unsigned long long) = DAT_002cd7a0;
	if (name[0] != 0) {
		text = bkLoadFile((struct bkPackage *)package, name, NULL, &size, NULL, 0);
		if (text == NULL) {
			TU_DEBUG("CTank_Entities::Initialise - Configuration file [%s] not found\n", name);
			TankReader_Dtor(reader);
			return 0;
		}
		BCarTextReader_Init((struct BCarTextReader *)reader, (char *)text, (int)size, name);
		TankEnts_LoadEntities(self, reader);
		TankEnts_LinkEntityTemplates(self);
		if (TA_WorldLaps(TU_PTR(self, ENTS_WORLD)) != 0)
			TankEnts_ScatterRocks(self, 4);
		TA_EntsSetup(self);
		p = bkHeapAlloc(0xe8, TU_FILE_ENTITIES, 0xfa, 0x1001);
		p = p != NULL ? CTweakMenu_CtorSub((struct CTweakMenu *)p, (struct CTweakMenu *)DAT_003151b4,
			"tokens") : NULL;
		TU_PTR(self, ENTS_TWEAKS) = p;
		CTweakMenu_AddInt((struct CTweakMenu *)p, "1 Point", &TU_I32(self, ENTS_PT1), 1, 0, 0x14, 0);
		CTweakMenu_AddInt((struct CTweakMenu *)TU_PTR(self, ENTS_TWEAKS), "3 Point",
			&TU_I32(self, ENTS_PT3), 1, 0, 0x14, 0);
		CTweakMenu_AddInt((struct CTweakMenu *)TU_PTR(self, ENTS_TWEAKS), "5 Point",
			&TU_I32(self, ENTS_PT5), 1, 0, 0x14, 0);
		CTweakMenu_AddInt((struct CTweakMenu *)TU_PTR(self, ENTS_TWEAKS), "Speed up",
			&TU_I32(self, ENTS_SPEED_TK), 1, 0, 0x14, 0);
		CTweakMenu_AddInt((struct CTweakMenu *)TU_PTR(self, ENTS_TWEAKS), "PROTECT",
			&TU_I32(self, ENTS_PRO_TK), 1, 0, 0x14, 0);
		CTweakMenu_AddInt((struct CTweakMenu *)TU_PTR(self, ENTS_TWEAKS), "Win Swap",
			&TU_I32(self, ENTS_WIN_SWITCH), 1, 0, 0x14, 0);
		CTweakMenu_AddInt((struct CTweakMenu *)TU_PTR(self, ENTS_TWEAKS), "Fire up",
			&TU_I32(self, ENTS_FIRE_UP), 1, 0, 0x14, 0);
		for (tpl = (char *)TU_PTR(self, ENTS_TEMPLATES); tpl != NULL; tpl = (char *)TU_PTR(tpl, TPL_NEXT)) {
			if (TU_I32(tpl, TPL_MENU) == 0)
				continue;
			p = bkHeapAlloc(0xe8, TU_FILE_ENTITIES, 0x107, 0x1001);
			p = p != NULL ? CTweakMenu_CtorSub((struct CTweakMenu *)p,
				(struct CTweakMenu *)DAT_003151b4, tpl + TPL_NAME) : NULL;
			TU_PTR(tpl, 0x04) = p;
			CTweakMenu_AddFloat((struct CTweakMenu *)p, "Reload Time", TU_FP(tpl, TPL_RELOAD), 1.0f, 0.0f,
				180.0f);
			CTweakMenu_AddFloat((struct CTweakMenu *)TU_PTR(tpl, 4), "Rate of turn", TU_FP(tpl, TPL_ROT),
				1.0f, 0.0f, 1000.0f);
			CTweakMenu_AddInt((struct CTweakMenu *)TU_PTR(tpl, 4), "Score Give", &TU_I32(tpl, TPL_SCOREGIVE),
				1, 0, 1000, 0);
			CTweakMenu_AddFloat((struct CTweakMenu *)TU_PTR(tpl, 4), "%score after death",
				TU_FP(tpl, TPL_SCORELOST), 0.01f, 0.0f, 1.0f);
			CTweakMenu_AddFloat((struct CTweakMenu *)TU_PTR(tpl, 4), "gravity", TU_FP(tpl, TPL_GRAVITY),
				0.1f, 0.0f, 100.0f);
			CTweakMenu_AddFloat((struct CTweakMenu *)TU_PTR(tpl, 4), "restartTime", TU_FP(tpl, TPL_RESTART),
				0.25f, 0.0f, 10.0f);
			CTweakMenu_AddFloat((struct CTweakMenu *)TU_PTR(tpl, 4), "mass", TU_FP(tpl, TPL_MASS), 1.0f,
				0.0f, 10000.0f);
			CTweakMenu_AddFloat((struct CTweakMenu *)TU_PTR(tpl, 4), "force/speed", TU_FP(tpl, TPL_FORCE),
				1.0f, 0.0f, 10000.0f);
			CTweakMenu_AddFloat((struct CTweakMenu *)TU_PTR(tpl, 4), "restitution",
				TU_FP(tpl, TPL_RESTITUTION), 0.01f, 0.0f, 1.0f);
		}
	}
	TankReader_Dtor(reader);
	return 1;
}

/* ======================================================================== gap functions
   0x001d4730-0x001d5790: methods of CTank_Entities (file inferred: they work on the same
   object between the last CTank_Entities.cpp tag and CTank_Sound) */

/* 0x001d4730  TankedUp (gap; CTank_Entities.cpp inferred)
CTank_Entities::Free: every entity, message, template and tweak menu. */
void TankEnts_Free(void *self)
{
	void **node;
	void **next;
	char *tpl;
	char *nextTpl;
	void *p;

	RefBot_Delete();
	TA_EntsFree(self);
	node = (void **)TU_PTR(self, ENTS_LIST);
	while (node != NULL) {
		next = (void **)node[1];
		TankEnts_DestroyEntity(self, node);
		node = next;
	}
	TU_PTR(self, ENTS_LIST) = NULL;
	node = (void **)TU_PTR(self, ENTS_MESSAGES);
	while (node != NULL) {
		bkHeapFree(node[0]);
		next = (void **)node[1];
		bkHeapFree(node);
		node = next;
	}
	p = TU_PTR(self, ENTS_EFFECTS);
	TU_PTR(self, ENTS_MESSAGES) = NULL;
	if (p != NULL) {
		BCarGenericGoal_Destruct((struct CParticleEffect *)p);
		bkHeapFree(p);
	}
	TU_PTR(self, ENTS_EFFECTS) = NULL;
	tpl = (char *)TU_PTR(self, ENTS_TEMPLATES);
	while (tpl != NULL) {
		if (TU_I32(tpl, TPL_MENU) != 0 && TU_PTR(tpl, 4) != NULL) {
			p = TU_PTR(tpl, 4);
			CTweakMenu_Dtor((struct CTweakMenu *)p);
			bkHeapFree(p);
		}
		nextTpl = (char *)TU_PTR(tpl, TPL_NEXT);
		bkHeapFree(tpl);
		tpl = nextTpl;
	}
	TU_PTR(self, ENTS_TEMPLATES) = NULL;
	p = DAT_003151b4;
	if (DAT_003151b4 != NULL) {
		CTweakMenu_Dtor((struct CTweakMenu *)DAT_003151b4);
		bkHeapFree(p);
	}
	p = TU_PTR(self, ENTS_TWEAKS);
	if (p != NULL) {
		CTweakMenu_Dtor((struct CTweakMenu *)p);
		bkHeapFree(p);
	}
	DAT_003151b4 = NULL;
	TU_PTR(self, ENTS_TWEAKS) = NULL;
	TU_PTR(self, ENTS_WORLD) = NULL;
}

/* MSVC rand() scaled to -0.5..0.5 (0x00253740 = 1 / 32767) */
#define TU_RAND_HALF() ((float)tu_rand() * 3.05185094e-05f - 0.5f)

/* 0x001d4830  TankedUp (gap; CTank_Entities.cpp inferred)
CTank_Entities::Fire: tank `ent` fires a "tank_shell" from its cannon, turned `offset`
from the turret: recoil animation, sound 2, 30 sparks, and the shell hits anything it
starts inside. */
void TankEnts_Fire(void *self, void *entity, float offset)
{
	char *ent = (char *)entity;
	char *shell;
	char *other;
	void **node;
	void *anim;
	float m[16];
	float off[4];
	float pos[4];
	float rot[4];
	float spark[4];
	float s;
	float sum;
	float dx, dy, dz;
	int id;
	int i;

	if (TU_PTR(ent, ENT_TURRET_INST) != NULL) {
		anim = baFindSubObject((struct BActorInstance *)TU_PTR(ent, ENT_TURRET_INST), "recoil");
		baQueueAnimation((struct BActorInstance *)TU_PTR(ent, ENT_TURRET_INST),
			(struct BActorNode *)TU_PTR(TU_PTR(ent, ENT_TURRET_INST), 0x134), (struct BAnim *)anim,
			0x400, -1, 0, NULL, NULL);
	}
	TankSound_Play(TU_PTR(self, ENTS_SOUND), 2, TU_FP(ent, ENT_POS), 200, 1, 0);
	bmMatrixRotationYXZ(m, CLMath_WrapAngle(TU_F32(ent, ENT_ROT)),
		CLMath_WrapAngle(TU_F32(ent, ENT_ROT + 4) + TU_F32(ent, 0x9a0) + offset), TU_F32(ent, ENT_ROT + 8));
	off[1] = TU_F32(ent, ENT_RADIUS) * -0.5f; /* 0x00257b8c */
	off[2] = DAT_00315170 * TU_F32(ent, ENT_RADIUS);
	off[0] = 0.0f;
	off[3] = 1.0f;
	bmTransformVector4InPlace(m, off);
	pos[0] = off[0] + TU_F32(ent, ENT_POS);
	pos[1] = off[1] + TU_F32(ent, ENT_POS + 4);
	pos[3] = 1.0f;
	rot[0] = 0.0f;
	rot[1] = 0.0f;
	rot[2] = 0.0f;
	pos[2] = off[2] + TU_F32(ent, ENT_POS + 8);
	rot[3] = 1.0f;
	TankEnts_DirToAngles(off, &rot[0], &rot[1]);
	rot[0] = 0.0f;
	id = TankEnts_CreateEntity(self, TU_I32(ent, ENT_ID), "tank_shell", pos, rot, 1.0f);
	shell = tu_find_entity(self, id);
	/* the original writes through a null entity when the shell was not made */
	if (shell == NULL)
		return;
	TU_I32(shell, 0x928) = 0;
	TU_DEBUG("CTank_Entities - entity [%s] has fired [%s], at offset [%.2f, %.2f, %.2f]\n",
		ent + ENT_NAME, shell + ENT_NAME, (double)off[0], (double)off[1], (double)off[2]);
	for (i = 0; i < 0x1e; i++) {
		spark[0] = TU_RAND_HALF();
		spark[2] = TU_RAND_HALF();
		spark[1] = TU_RAND_HALF();
		spark[3] = 1.0f;
		bmVectorNormalise(spark, spark);
		s = (float)tu_rand() * 0.000305185094f; /* 0x00265808 */
		spark[0] = spark[0] * s;
		spark[1] = spark[1] * s;
		spark[2] = spark[2] * s;
		/* 0x002687e4 = 3 / 32767 */
		spark[0] = TU_F32(shell, 0xa38) / ((float)tu_rand() * 9.15555283e-05f + 2.0f) + spark[0];
		spark[1] = TU_F32(shell, 0xa3c) / ((float)tu_rand() * 9.15555283e-05f + 2.0f) + spark[1];
		spark[2] = TU_F32(shell, 0xa40) / ((float)tu_rand() * 9.15555283e-05f + 2.0f) + spark[2];
		TA_DebrisAdd(TU_PTR(self, ENTS_DEBRIS), (i >= 10) + 0xd, TU_FP(shell, ENT_POS), spark, 0,
			0xffffffffu);
	}
	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		other = (char *)node[0];
		if (TU_I32(other, ENT_ID) == TU_I32(shell, ENT_ID) || TU_I32(other, ENT_ID) == TU_I32(ent, ENT_ID))
			continue;
		if ((signed char)TU_U8(TU_PTR(other, ENT_TEMPLATE), TPL_FLAGS) < 0 ||
			TU_I32(other, 4) == TU_I32(shell, 4))
			continue;
		sum = TU_F32(other, ENT_RADIUS) + TU_F32(shell, ENT_RADIUS);
		dy = TU_F32(other, ENT_POS + 4) - TU_F32(shell, ENT_POS + 4);
		dx = TU_F32(other, ENT_POS) - TU_F32(shell, ENT_POS);
		dz = TU_F32(other, ENT_POS + 8) - TU_F32(shell, ENT_POS + 8);
		if (dy * dy + dx * dx + dz * dz <= sum * sum) {
			TU_DEBUG("CTank_Entities::Fire - overlap found with shell [%s] and object [%s]\n",
				shell + ENT_NAME, other + ENT_NAME);
			TankEnts_EntityHit(self, other, shell);
			TankEnts_EntityHit(self, shell, other);
		}
	}
}

/* 0x001d4cc0  TankedUp (gap; CTank_Entities.cpp inferred)
CTank_Entities::FireCommand: tank `id` fires when it is loaded, alive and not frozen in
the playing state (three shells with the triple-shot +0x94c, DAT_0031516c degrees apart);
message 10 reloads it after a time by the player's rank (halved with triple shot). 1 when
it fired. */
int TankEnts_FireCommand(void *self, int id)
{
	char *ent = tu_find_entity(self, id);
	float delay;

	if (ent == NULL)
		return 0;
	if (TU_I32(ent, 0x938) == 0 || TU_I32(ent, ENT_DEAD) != 0 || TU_I32(ent, 0x944) != 0 ||
		TU_I32(self, ENTS_VARIANT) != 4)
		return 0;
	TankEnts_Fire(self, ent, 0.0f);
	if (TU_I32(ent, 0x94c) != 0) {
		TankEnts_Fire(self, ent, DAT_0031516c * 0.0174532924f); /* 0x002640f8 */
		TankEnts_Fire(self, ent, -DAT_0031516c * 0.0174532924f);
	}
	TU_I32(ent, 0x938) = 0;
	/* the original keeps the bits of `id` (its argument slot) as the delay for other ranks */
	delay = tu_bitsf((unsigned int)id);
	switch (TA_PlayersRank(TU_PTR(self, ENTS_PLAYERS), id)) {
	case 1:
		delay = 2.5f;
		break;
	case 2:
	case 3:
		delay = 2.0f;
		break;
	case 4:
		delay = 1.5f;
		break;
	}
	if (TU_I32(ent, 0x94c) != 0)
		delay = delay * 0.5f;
	TankEnts_SendDelayedMessage(self, 10, (unsigned int)id, (unsigned int)id, 1, 0, 0, 0, delay);
	return 1;
}

/* a random direction for debris: x and z -0.5..0.5, y 0..0.5 (0x0025389c = 0.5 / 32767),
   normalised */
static void tu_debris_dir(float *dir)
{
	dir[0] = TU_RAND_HALF();
	dir[2] = TU_RAND_HALF();
	dir[3] = 1.0f;
	dir[1] = (float)tu_rand() * 1.52592547e-05f;
	bmVectorNormalise(dir, dir);
}

/* the blast of a tank or an exploding crate: a flash, a scorch, 11 fragments and 200 sparks */
static void tu_blast(void *self, char *ent)
{
	float *pos = TU_FP(ent, ENT_POS);
	float normal[4];
	float dir[4];
	float f;
	int i;

	TA_LightsFireFlash(TU_PTR(self, ENTS_LIGHTS), pos);
	TA_BoomAdd(TU_PTR(self, ENTS_BOOM), pos,
		TA_WorldGroundHeight(TU_PTR(self, ENTS_WORLD), pos, 1.0f, normal));
	for (i = 0; i < 0xb; i++) {
		tu_debris_dir(dir);
		f = i < 10 ? 1.0f : 0.1f; /* 0x00253850, 0x002536cc */
		f = f * (float)tu_rand() * DAT_003150b8 * 3.05185094e-05f;
		dir[0] = dir[0] * f;
		dir[1] = dir[1] * f;
		dir[2] = dir[2] * f;
		TA_DebrisAdd(TU_PTR(self, ENTS_DEBRIS), i < 10 ? 6 : 9, pos, dir, 0, 0xffffffffu);
	}
	for (i = 0; i < 200; i++) {
		tu_debris_dir(dir);
		dir[0] = dir[0] * 30.0f; /* 0x002553a8 */
		dir[1] = dir[1] * 30.0f;
		tu_rand(); /* drawn and not used */
		dir[2] = dir[2] * 30.0f;
		TA_DebrisAdd(TU_PTR(self, ENTS_DEBRIS), (i > 0x31) + 0xd, pos, dir, 0, 0xffffffffu);
	}
}

/* 0x001d4e10  TankedUp (gap; CTank_Entities.cpp inferred)
CTank_Entities::Explode: the end of an entity by type: tanks (1) blast; shells and mines
(2, 4) throw 6 fragments along their last velocity; type 9 sends message 0x1f, puts a new
rock somewhere and blasts. */
void TankEnts_Explode(void *self, void *entity)
{
	char *ent = (char *)entity;
	float dir[4];
	float base[3];
	float b;
	float f;
	int i;

	switch (TU_ENT_TYPE(ent)) {
	case 1:
		tu_blast(self, ent);
		return;
	case 2:
	case 4:
		for (i = 0; i < 6; i++) {
			dir[0] = TU_RAND_HALF();
			dir[2] = TU_RAND_HALF();
			b = 1.0f / (TU_F32(self, ENTS_DT) * 4.0f); /* 0x00257c40 */
			dir[3] = 1.0f;
			base[0] = b * TU_F32(ent, ENT_OLD_VEL);
			base[1] = b * TU_F32(ent, ENT_OLD_VEL + 4);
			base[2] = b * TU_F32(ent, ENT_OLD_VEL + 8);
			dir[1] = (float)tu_rand() * 1.52592547e-05f;
			bmVectorNormalise(dir, dir);
			f = i < 5 ? 1.0f : 0.1f;
			f = f * (float)tu_rand() * DAT_003150b8 * 3.05185094e-05f;
			dir[0] = base[0] + dir[0] * f;
			dir[1] = base[1] + f * dir[1];
			dir[2] = base[2] + dir[2] * f;
			TA_DebrisAdd(TU_PTR(self, ENTS_DEBRIS), i < 5 ? 6 : 9, TU_FP(ent, ENT_POS), dir, 0,
				0xffffffffu);
		}
		break;
	case 9:
		TankEnts_SendDelayedMessage(self, 0x1f, TU_U32(ent, ENT_ID), TU_U32(self, 0xe4), 1, 0, 0, 0,
			0.0f);
		TankEnts_ScatterRocks(self, 1);
		tu_blast(self, ent);
		return;
	}
}

/* 0x001d55a0  TankedUp (gap; CTank_Entities.cpp inferred)
CTank_Entities::ProcessDead: entities that fell below -1000 die. A dead entity whose
template has flag 8 explodes and (once) gets a respawn point and message 0xd after a time
by its player's rank; any other dead entity explodes, gets message 2 at once and is
destroyed. */
void TankEnts_ProcessDead(void *self)
{
	void **link = (void **)TU_ADDR(self, ENTS_LIST);
	void **node;
	char *ent;
	char msg[0x30];
	float delay;

	while (*link != NULL) {
		ent = (char *)((void **)*link)[0];
		if (TU_F32(ent, ENT_POS + 4) < -1000.0f) /* 0x002679f8 */
			TU_I32(ent, ENT_DEAD) = 1;
		if (TU_I32(ent, ENT_DEAD) == 0 || TU_I32(ent, 0x95c) != 0) {
			link = &((void **)*link)[1];
			continue;
		}
		if (TU_PTR(ent, ENT_TEMPLATE) != NULL && (TU_U8(TU_PTR(ent, ENT_TEMPLATE), TPL_FLAGS) & 8) != 0) {
			TankEnts_Explode(self, ent);
			if (TU_I32(ent, 0x95c) == 0) {
				TankEnts_GetRespawnPos(self, ent);
				delay = -1.0f;
				switch (TA_PlayersRank(TU_PTR(self, ENTS_PLAYERS), TU_I32(ent, ENT_ID))) {
				case 1:
					delay = 3.0f;
					break;
				case 2:
				case 3:
					delay = 2.0f;
					break;
				case 4:
					delay = 1.0f;
					break;
				}
				if (delay >= 0.0f)
					TankEnts_SendDelayedMessage(self, 0xd, TU_U32(ent, ENT_ID), TU_U32(ent, ENT_ID), 1, 0,
						0, 0, delay);
			}
			TU_I32(ent, 0x95c) = 1;
			link = &((void **)*link)[1];
			continue;
		}
		node = (void **)*link;
		*link = node[1];
		TankEnts_Explode(self, ent);
		memset(msg, 0, sizeof msg);
		TU_I32(msg, 0x28) = 0;
		TU_I32(msg, 0x1c) = TU_I32(ent, ENT_ID);
		TU_I32(msg, 0x18) = TU_I32(ent, ENT_ID);
		TU_I32(msg, 0) = 2;
		TA_SendMessageNow(self, ent, 1, msg);
		TankEnts_DestroyEntity(self, node);
	}
}

/* 0x001d5790  TankedUp (gap; CTank_Entities.cpp inferred)
CTank_Entities::Update: one frame of the entity system (`dt` frames, game `state`); returns
the state to go to (5 when +0x60 asks to end; 4 while the RefBot flag holds a state <= 3). */
int TankEnts_Update(void *self, float dt, int state)
{
	void **node;

	CToken_UpdateAll();
	TU_F32(self, ENTS_SPIN) = CLMath_WrapAngle(TU_F32(self, ENTS_SPIN) + 0.0500000007f); /* 0x00258b54 */
	TU_F32(self, ENTS_DT) = dt;
	TU_I32(self, ENTS_VARIANT) = state;
	RefBot_Update();
	CParticleEffect_Update((struct CParticleEffect *)TU_PTR(self, ENTS_EFFECTS), dt);
	if (TU_I32(TU_PTR(self, ENTS_WORLD), 0) != 0)
		TankEnts_KillBuriedTanks(self);
	TA_EntsThink(self);
	TankEnts_ProcessMessages(self);
	TA_EntsStep2(self);
	TankEnts_ProcessDead(self);
	TankEnts_UpdatePhysics(self, state);
	TA_EntsStep3(self);
	TankEnts_UpdateTracks(self);
	if (state < 5) {
		if (TU_I32(self, 0x60) == 0)
			goto done;
		state = 5;
	}
	TankEnts_FlushMessages(self);
	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1])
		TU_I32(node[0], 0x948) = 0;
done:
	if (DAT_002baeb8 == 0 || state > 3)
		return state;
	return 4;
}
