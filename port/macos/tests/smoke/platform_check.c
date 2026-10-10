/*
PLATFORM_CHECK.C

The image built from this file links every object of the platform layer
(libofplatform.a, whole archive). It only has to link: running it prints
one line and exits 0.
*/

#include <stdio.h>

int main(void)
{
	printf("platform layer links\n");
	return 0;
}
