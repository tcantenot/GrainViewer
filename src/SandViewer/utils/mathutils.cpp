#include "utils/mathutils.h"
#include <cmath>

int ilog2(int x) {
	int log = 0;
	while (x >>= 1) ++log;
	return log;
}

// From J. Dupuy's dj_brdf.h
using float_t = float;
using std::logf;

// See: http://www.johndcook.com/blog/cpp_erf/, by John D. Cook
float djerf(float x)
{
	// constants
	float_t a1 = (float_t)0.254829592;
	float_t a2 = -(float_t)0.284496736;
	float_t a3 = (float_t)1.421413741;
	float_t a4 = -(float_t)1.453152027;
	float_t a5 = (float_t)1.061405429;
	float_t p = (float_t)0.3275911;

	// Save the sign of x
	int sign = 1;
	if (x < 0)
		sign = -1;
	x = fabs(x);

	// A&S formula 7.1.26
	float_t t = (float_t)1.0 / ((float_t)1.0 + p * x);
	float_t y = (float_t)1.0 - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * exp(-x * x);

	return sign * y;
}

// See: Approximating the erfinv function, by Mike Giles
float djerfinv(float u)
{
	float_t w, p;

	w = -logf(((float_t)1.0 - u) * ((float_t)1.0 + u));
	if (w < (float_t)5.0) {
		w = w - (float_t)2.500000;
		p = (float_t)2.81022636e-08;
		p = (float_t)3.43273939e-07 + p * w;
		p = (float_t)-3.5233877e-06 + p * w;
		p = (float_t)-4.39150654e-06 + p * w;
		p = (float_t)0.00021858087 + p * w;
		p = (float_t)-0.00125372503 + p * w;
		p = (float_t)-0.00417768164 + p * w;
		p = (float_t)0.246640727 + p * w;
		p = (float_t)1.50140941 + p * w;
	}
	else {
		w = sqrt(w) - (float_t)3.0;
		p = (float_t)-0.000200214257;
		p = (float_t)0.000100950558 + p * w;
		p = (float_t)0.00134934322 + p * w;
		p = (float_t)-0.00367342844 + p * w;
		p = (float_t)0.00573950773 + p * w;
		p = (float_t)-0.0076224613 + p * w;
		p = (float_t)0.00943887047 + p * w;
		p = (float_t)1.00167406 + p * w;
		p = (float_t)2.83297682 + p * w;
	}

	return p * u;
}
