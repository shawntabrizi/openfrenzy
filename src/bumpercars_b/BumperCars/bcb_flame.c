/*
BumperCars/bcb_flame.c  <-  gap function 0x00096240 (file unknown; it reports itself as
"CBCar_Flame::Initialise", so it is likely the tail of CBCar_Flame.cpp, whose other
functions are in bumpercars_a)

CBCar_Flame::Initialise: reads the flame configuration file of the world (FLAMES "...").
*/

#include "bumpercars_b.h"
#include "bumpercars_b_internal.h"

/* 0x00096240  (gap; CBCar_Flame.cpp by its message)
Clears the flame object (this = 0x002f1380), then, when `name` is not empty, reads the
configuration file `name` of `package` with bumpercars_a's loader (0x000952e0). Returns 1. */
int BCarFlame_Initialise(void *self, struct bkPackage *package, const char *name)
{
	struct BCarTextReader reader;
	char *file;
	uint size;

	BCarTextReader_Construct(&reader);
	BCB_I32(self, 0) = 0;
	BCB_I32(self, 4) = 1;
	if (*name != '\0')
	{
		file = (char *)bkLoadFile(package, name, NULL, &size, NULL, 0);
		if (file == NULL)
			bkPrintf("CBCar_Flame::Initialise - Configuration file [%s] not found\n", name);
		else
		{
			BCarTextReader_Init(&reader, file, (int)size, name);
			/* the original pushes `package` too (ret 8), but 0x000952e0 never reads it;
			bumpercars_a defines it as LoadFlameInfo(self, reader) */
			BCB_CALL(FUN_000952e0, void (*)(void *, struct BCarTextReader *))(self, &reader);
		}
	}
	BCarTextReader_Destruct(&reader);
	return 1;
}
