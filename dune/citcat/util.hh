#ifndef UTIL_HH
#define UTIL_HH

#define IM1 2147483563
#define IM2 2147483399
#define AM (1.0/IM1)
#define IMM1 (IM1-1)
#define IA1 40014
#define IA2 40692
#define IQ1 53668
#define IQ2 52774
#define IR1 12211
#define IR2 3791
#define NTAB 32
#define NDIV (1+IMM1/NTAB)
#define EPS 1.e-14
#define RNMX (1.0-EPS)

double ran2(int *idum)
{
    int j;
    int k;
    static int idum2 = 123456789;
    static int iy = 0;
    static int iv[NTAB];
    double temp;

    if (*idum <= 0) {		// *idum < 0 ==> initialize
	if (-(*idum) < 1)
	    *idum = 1;
	else
	    *idum = -(*idum);
	idum2 = (*idum);

	for (j = NTAB+7; j >= 0; j--) {
	    k = (*idum)/IQ1;
	    *idum = IA1*(*idum-k*IQ1) - k*IR1;
	    if (*idum < 0) *idum += IM1;
	    if (j < NTAB) iv[j] = *idum;
	}
	iy = iv[0];
    }
    k = (*idum)/IQ1;
    *idum = IA1*(*idum-k*IQ1) - k*IR1;
    if (*idum < 0) *idum += IM1;

    k = idum2/IQ2;
    idum2 = IA2*(idum2-k*IQ2)-k*IR2;
    if (idum2 < 0) idum2 += IM2;

    j = iy/NDIV;
    iy = iv[j] - idum2;
    iv[j] = *idum;
    if (iy < 1) iy += IMM1;

    if ((temp = AM*iy) > RNMX)
	return RNMX;
    else
	return temp;
}

#undef IM1
#undef IM2
#undef AM
#undef IMM1
#undef IA1
#undef IA2
#undef IQ1
#undef IQ2
#undef IR1
#undef IR2
#undef NTAB
#undef NDIV
#undef EPS
#undef RNMX

static int randx = 0;		/* copy of random seed (internal use only) */

void ran_init(int seed)	/* initialize the random number generator */
{
    if (seed <= 0)
        DUNE_THROW(Dune::Exception,"Random Number Seed must be > 0");
    randx = -std::abs(seed);
}

float ran(float a, float b)
{
    if (randx == 0) ran_init(1);
    return a + (b-a)*((float)ran2(&randx));
}


#endif // UTIL_HH
