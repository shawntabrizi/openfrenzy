/*
bKernel resources: every loaded texture, actor, sample, font, string table, splash, set,
collision mesh, animated collision mesh and audio stream is on one list of all resources
and on the list of its type. A resource is found by the crc of its file name, loaded from
a package by the type's loader and deleted by the type's destroyer (the tables at
0x002852e8 and 0x00285310 point into the modules that own the types).

The original source file is not known (gap functions in C:\Babel\Xbox\Src\bKernel).
0x0004a2d0 (the string table loader) and 0x00052040 (its destroyer) are placed here too.
*/

#include "bkernel_core.h"

#include <string.h>

/* 0x002852c0: type names */
const char *const DAT_002852c0[BK_RESOURCE_TYPES] = {
	"Texture", "Actor", "Sample", "Font", "StringTable", "Splash", "Set", "CollisionMesh",
	"AnimCollisionMesh", "AudioStream",
};
/* 0x002852e8: loader of each type: resource = loader(package, crc) */
bkResource *(*const DAT_002852e8[BK_RESOURCE_TYPES])(struct bkPackage *, uint) = {
	(bkResource *(*)(struct bkPackage *, uint))FUN_00041a10, /* texture: bdisplay */
	(bkResource *(*)(struct bkPackage *, uint))FUN_00034b50, /* actor: bactor */
	(bkResource *(*)(struct bkPackage *, uint))FUN_00055390, /* sample: engine_misc */
	(bkResource *(*)(struct bkPackage *, uint))FUN_00042e60, /* font: bdisplay */
	(bkResource *(*)(struct bkPackage *, uint))bkLoadStringTable,
	(bkResource *(*)(struct bkPackage *, uint))FUN_00046db0, /* splash: bdisplay */
	(bkResource *(*)(struct bkPackage *, uint))FUN_0003ca50, /* set: bactor */
	(bkResource *(*)(struct bkPackage *, uint))FUN_00052700, /* collision mesh: engine_misc */
	(bkResource *(*)(struct bkPackage *, uint))FUN_000521c0, /* anim collision mesh */
	(bkResource *(*)(struct bkPackage *, uint))FUN_000562d0, /* audio stream: engine_misc */
};
/* 0x00285290: language suffixes of string tables (bkLanguage) */
const char *const DAT_00285290[6] = { "uk", "f", "d", "e", "it", "nl" };
/* 0x00285310: destroyer of each type (the original jumps to it after unlinking) */
void (*const DAT_00285310[BK_RESOURCE_TYPES])(bkResource *) = {
	(void (*)(bkResource *))FUN_00041530, /* texture: bdisplay */
	(void (*)(bkResource *))FUN_00034e30, /* actor: bactor */
	(void (*)(bkResource *))FUN_000553e0, /* sample: engine_misc */
	(void (*)(bkResource *))FUN_00042ec0, /* font: bdisplay */
	(void (*)(bkResource *))bkFreeDataResource,
	(void (*)(bkResource *))FUN_00046dc0, /* splash: bdisplay */
	(void (*)(bkResource *))FUN_0003ca90, /* set: bactor */
	(void (*)(bkResource *))bkFreeDataResource, /* collision mesh: plain data */
	(void (*)(bkResource *))bkFreeDataResource, /* anim collision mesh: plain data */
	(void (*)(bkResource *))FUN_00056340, /* audio stream: engine_misc */
};
/* 0x002ccd00: sentinel of the list of all resources */
bkResource DAT_002ccd00;
/* 0x002ccd20: sentinels of the lists of each type */
bkResource DAT_002ccd20[BK_RESOURCE_TYPES];
/* 0x002cce60: the tag of each type in a package's tag table ("TEXR", "ACTR", ...), set by
a static constructor (0x001f2a40, module core). Initialised so that the interior words
can carry the names core's code uses for them. */
uint DAT_002cce60[BK_RESOURCE_TYPES] = { 0 };
BK_ALIAS(DAT_002cce64, DAT_002cce60, 0x04);
BK_ALIAS(DAT_002cce68, DAT_002cce60, 0x08);
BK_ALIAS(DAT_002cce6c, DAT_002cce60, 0x0c);
BK_ALIAS(DAT_002cce70, DAT_002cce60, 0x10);
BK_ALIAS(DAT_002cce74, DAT_002cce60, 0x14);
BK_ALIAS(DAT_002cce78, DAT_002cce60, 0x18);
BK_ALIAS(DAT_002cce7c, DAT_002cce60, 0x1c);
BK_ALIAS(DAT_002cce80, DAT_002cce60, 0x20);
BK_ALIAS(DAT_002cce84, DAT_002cce60, 0x24);

/* 0x0004a2d0
bkLoadStringTable: the loader of type 4: loads the file and turns its offsets into
pointers. */
struct bkStringTable *bkLoadStringTable(struct bkPackage *package, uint crc)
{
	bkStringTable *table = (bkStringTable *)bkLoadPackageFile(package, crc, NULL, NULL, NULL, 0);
	int index;

	if (table == NULL)
		return NULL;
	table->entries = (bkStringTableEntry *)(table + 1);
	for (index = 0; index < table->numEntries; index++)
	{
		table->entries[index].text = (char *)table + (uint)table->entries[index].text;
		if (table->entries[index].extra != NULL)
			table->entries[index].extra = (char *)table + (uint)table->entries[index].extra;
	}
	return table;
}

/* 0x0004b570
bkInitResources: empties the resource lists. */
void bkInitResources(void)
{
	int type;

	memset(&bkResourceList, 0, sizeof(bkResourceList));
	bkResourceList.prev = &bkResourceList;
	bkResourceList.next = &bkResourceList;
	for (type = 0; type < BK_RESOURCE_TYPES; type++)
	{
		memset(&bkResourceTypeLists[type], 0, sizeof(bkResource));
		bkResourceTypeLists[type].typePrev = &bkResourceTypeLists[type];
		bkResourceTypeLists[type].typeNext = &bkResourceTypeLists[type];
	}
}

/* 0x0004b5b0
bkAddResource: links a new resource of package's into the lists (group 0xdefa keeps the
resource's own group). */
void bkAddResource(struct bkResource *resource, struct bkPackage *package, int type, int group)
{
	resource->packageId = package->id;
	resource->type = (byte)type;
	resource->next = &bkResourceList;
	resource->prev = bkResourceList.prev;
	bkResourceList.prev = resource;
	resource->prev->next = resource;
	resource->typeNext = &bkResourceTypeLists[type];
	resource->typePrev = bkResourceTypeLists[type].typePrev;
	bkResourceTypeLists[type].typePrev = resource;
	resource->typePrev->typeNext = resource;
	resource->context = 0;
	if (group != BK_RESOURCE_GROUP_DEFAULT)
		resource->group = (ushort)group;
}

/* 0x0004b620
bkListResources: prints the resources of the types in typeMask. */
void bkListResources(uint typeMask)
{
	bkResource *resource;
	const char *fileName;
	const char *packageName;
	int index = 0;

	for (resource = bkResourceList.next; resource != &bkResourceList; resource = resource->next)
	{
		if (typeMask & (1u << (resource->type & 0x1f)))
		{
			fileName = bkGetFilename(resource->packageId, resource->crc);
			if (fileName == NULL)
				fileName = "<UNKNOWN>";
			packageName = bkGetFilename(resource->packageId, 0);
			if (packageName == NULL)
				packageName = "<UNKNOWN>";
			bkPrintf("Resource #%03d, %13s, %s, PAK \"%s\" (0x%08x), group %d, File \"%s\" (0x%08x), context 0x%08x\n",
				index, DAT_002852c0[resource->type], (int)resource->packageId < 0 ? "Loaded" : "Opened",
				packageName, resource->packageId & 0x7fffffff, (short)resource->group, fileName,
				resource->crc, resource->context);
		}
		index++;
	}
}

/* unlinks a resource from both lists */
static void bkUnlinkResource(bkResource *resource)
{
	resource->next->prev = resource->prev;
	resource->prev->next = resource->next;
	resource->typeNext->typePrev = resource->typePrev;
	resource->typePrev->typeNext = resource->typeNext;
}

/* 0x0004b6e0
bkDeleteResource: unlinks a resource and hands it to its type's destroyer. */
void bkDeleteResource(struct bkResource *resource)
{
	bkUnlinkResource(resource);
	DAT_00285310[resource->type](resource);
}

/* 0x0004b720
bkFindOrLoadResource: without a package, finds a loaded resource of the type by crc;
with one, loads it with the type's loader and adds it. */
struct bkResource *bkFindOrLoadResource(struct bkPackage *package, uint crc, int type, int group)
{
	bkResource *resource;

	if (package == NULL)
	{
		for (resource = bkResourceTypeLists[type].typeNext; resource != &bkResourceTypeLists[type];
			resource = resource->typeNext)
		{
			if (resource->crc == crc)
				return resource;
		}
		return NULL;
	}
	resource = DAT_002852e8[type](package, crc);
	if (resource == NULL)
		return NULL;
	resource->crc = crc;
	bkAddResource(resource, package, type, group);
	return resource;
}

/* 0x0004b7a0
bkFindResource: a loaded resource of a type by crc: first one from the given package, then
one of the given group (0xffff: any), then any. */
struct bkResource *bkFindResource(int type, uint crc, uint packageId, uint group)
{
	bkResource *list = &bkResourceTypeLists[type];
	bkResource *resource;

	for (resource = list->typeNext; resource != list; resource = resource->typeNext)
	{
		if (resource->crc == crc && ((resource->packageId ^ packageId) & 0x7fffffff) == 0)
			return resource;
	}
	for (resource = list->typeNext; resource != list; resource = resource->typeNext)
	{
		if (resource->crc == crc && (group == 0xffff || resource->group == group))
			return resource;
	}
	for (resource = list->typeNext; resource != list; resource = resource->typeNext)
	{
		if (resource->crc == crc)
			return resource;
	}
	return NULL;
}

/* 0x0004b820
bkGetNextResource: iterates the resources of a type (resource NULL: the first). */
struct bkResource *bkGetNextResource(uint type, struct bkResource *resource)
{
	bkResource *next;

	if (resource == NULL)
	{
		next = bkResourceTypeLists[type].typeNext;
		return next == &bkResourceTypeLists[type] ? NULL : next;
	}
	if (resource->type != type)
		return NULL;
	next = resource->typeNext;
	return next == &bkResourceTypeLists[resource->type] ? NULL : next;
}

/* 0x0004b870
bLoadPackageResources: loads every file of a package whose tags include the tag of a type
in typeMask (and tag, when it is not 0). Returns the number loaded. */
int bLoadPackageResources(struct bkPackage *package, uint typeMask, int group, uint tag)
{
	ulonglong start = bkTimerRead();
	int loaded = 0;
	int type;
	int index;
	bkPackageEntry *entry;
	uint *tags;
	uint count;
	int hasType;
	int hasTag;
	longlong milliseconds;

	for (type = 0; type < BK_RESOURCE_TYPES; type++)
	{
		if ((typeMask & (1u << type)) == 0)
			continue;
		for (index = 0; index < package->numFiles; index++)
		{
			entry = bkGetPackageEntry(package, index);
			tags = &package->tags[entry->tagOffset >> 2];
			hasType = 0;
			hasTag = 0;
			for (count = entry->tagCount; count != 0; count--, tags++)
			{
				if (*tags == DAT_002cce60[type])
					hasType = 1;
				if (*tags == tag)
					hasTag = 1;
			}
			if (hasType && (tag == 0 || hasTag))
				loaded += bkFindOrLoadResource(package, entry->crc, type, group) != NULL;
		}
	}
	milliseconds = ((longlong)bkTimerDelta(start, bkTimerRead()) * 1000) / bkTimerFrequency;
	bkPrintf("bLoadPackageResources took %.2f ms\n", (double)milliseconds);
	return loaded;
}

/* 0x0004b9e0
bkDeletePackageResources: deletes the resources of the types in typeMask that came from a
package. */
void bkDeletePackageResources(uint packageId, uint typeMask)
{
	bkResource *resource;

restart:
	for (resource = bkResourceList.next; resource != &bkResourceList; resource = resource->next)
	{
		if ((typeMask & (1u << (resource->type & 0x1f))) != 0 &&
			((resource->packageId ^ packageId) & 0x7fffffff) == 0)
		{
			bkUnlinkResource(resource);
			DAT_00285310[resource->type](resource);
			goto restart;
		}
	}
}

/* 0x0004ba60
bkDeleteGroupResources: deletes the resources of the types in typeMask in a group (0xffff:
every group). */
void bkDeleteGroupResources(uint typeMask, uint group)
{
	bkResource *resource;

restart:
	for (resource = bkResourceList.next; resource != &bkResourceList; resource = resource->next)
	{
		if ((typeMask & (1u << (resource->type & 0x1f))) != 0 &&
			(group == 0xffff || resource->group == group))
		{
			bkUnlinkResource(resource);
			DAT_00285310[resource->type](resource);
			goto restart;
		}
	}
}

/* 0x0004bae0 */
void bkDeleteAllResources(void)
{
	bkResource *resource;

	while (bkResourceList.next != &bkResourceList)
	{
		resource = bkResourceList.next;
		bkUnlinkResource(resource);
		DAT_00285310[resource->type](resource);
	}
}

/* 0x0004bb30
bLoadResource: finds (package NULL) or loads a resource by file name. A string table's
name gets the language suffix ("name.uk"); a splash's crc goes through FUN_00046da0. */
struct bkResource *bLoadResource(struct bkPackage *package, const char *name, int type, int group)
{
	char localName[128];
	uint crc;
	bkResource *resource;

	if (type == BK_RESOURCE_STRINGTABLE)
	{
		strcpy(localName, name);
		sprintf(localName + strlen(localName), ".%s", DAT_00285290[bkLanguage]);
		crc = bkCRCString(localName);
	}
	else
	{
		crc = bkCRCString(name);
		if (type == BK_RESOURCE_SPLASH)
			crc = FUN_00046da0(crc);
	}
	resource = bkFindOrLoadResource(package, crc, type, group);
	if (resource == NULL)
	{
		if (package != NULL)
		{
			bkPrintf("bLoadResource: Could not load '%s' from '%s'\n", name, package->name);
			return NULL;
		}
		bkPrintf("bLoadResource: Could not find '%s' in memory\n", name);
	}
	return resource;
}

/* 0x00052040
bkFreeDataResource: the destroyer of types 4, 7 and 8 (plain data): frees it unless it lies in a
package in memory. */
void bkFreeDataResource(struct bkResource *resource)
{
	if ((int)resource->packageId >= 0)
		bkHeapFree(resource);
}
