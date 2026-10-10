/*
OF_MATH.C

openfrenzy runtime module: the halo_* maths functions that OpenCE's halo_math.h
routes sin(), atan2(), pow() and the others to (include/halo_math.h). OpenCE takes
them from its own musl copy, so that every port computes the same values. Here
they are the guest musl's own functions, which are the same on every machine
the port runs on (one guest C library).
The guest flags force-include halo_linux_prefix.h, which pulls in halo_math.h, so
its macros are removed here; otherwise each function calls itself forever.
*/

#include <math.h>

#undef sin
#undef cos
#undef tan
#undef asin
#undef acos
#undef atan
#undef atan2
#undef exp
#undef log
#undef log2
#undef log10
#undef pow

double halo_sin(double x)
{
	return sin(x);
}

double halo_cos(double x)
{
	return cos(x);
}

double halo_tan(double x)
{
	return tan(x);
}

double halo_asin(double x)
{
	return asin(x);
}

double halo_acos(double x)
{
	return acos(x);
}

double halo_atan(double x)
{
	return atan(x);
}

double halo_atan2(double y, double x)
{
	return atan2(y, x);
}

double halo_exp(double x)
{
	return exp(x);
}

double halo_log(double x)
{
	return log(x);
}

double halo_log2(double x)
{
	return log2(x);
}

double halo_log10(double x)
{
	return log10(x);
}

double halo_pow(double x, double y)
{
	return pow(x, y);
}
