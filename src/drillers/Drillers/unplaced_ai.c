/*
Drillers/unplaced_ai.c  <-  C:\BlitzParty\Game\Src\Drillers\ (file not known)

The computer players (the AI object, one static object at 0x002aadf8) and its geometry
helpers. These gap functions lie between CDrillers_Resources.cpp (0x0011e1e0) and
CDrillers_Particle.cpp (0x00123550); no assert names their file.
*/

#include "drillers.h"

/* 0x00121920  Drillers (gap; file not known)
The difference a - b of two headings (degrees), brought into -180..180 (inputs within
two turns). `self` (the AI object) is not used. */
float DrAi_AngleDiff(void *self, float a, float b)
{
	float d = a - b;

	(void)self;
	if (d > 360.0f)
		d = d - 720.0f;
	if (d < -360.0f)
		d = d + 720.0f;
	if (d <= -180.0f || d > 180.0f) {
		if (d <= 180.0f)
			return d + 360.0f;
		return d - 360.0f;
	}
	return d;
}

/* 0x00121990  Drillers (gap; file not known)
Is the point px, pz inside the polygon of `n` points (x, z pairs; the last point repeats
the first)? Crossing count along +x over the n - 1 edges, from the last edge back. `self`
(the AI object) is not used. */
char DrAi_InPolygon(void *self, float px, float pz, int n, const float *poly)
{
	const float *a;
	const float *b;
	unsigned int count = 0;
	int k;

	(void)self;
	for (k = n - 1; 0 < k; k--) {
		a = &poly[(k - 1) * 2];
		b = &poly[k * 2];
		if ((pz <= a[1]) == (pz <= b[1]))
			continue;
		if ((px <= a[0]) == (px <= b[0])) {
			if (px <= a[0])
				count++;
		} else if (px <= b[0] - ((b[1] - pz) * (a[0] - b[0])) / (a[1] - b[1])) {
			count++;
		}
	}
	return (char)(count & 1);
}
