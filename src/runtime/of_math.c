/*
OF_MATH.C

openfrenzy runtime module: halo_sin / halo_cos, which OpenCE's halo_math.h
routes sin() and cos() to (OpenCE takes them from its own musl copy, so that
every port computes the same values). Here they are musl's sin and cos.
This file must not include halo_math.h (its macros would make these recursive).
*/

#include <math.h>

double halo_sin(double x)
{
	return sin(x);
}

double halo_cos(double x)
{
	return cos(x);
}
