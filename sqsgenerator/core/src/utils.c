#include "utils.h"

static uint32_t d = 6615241;
static uint32_t v = 5783321;
static uint32_t w = 88675123;
static uint32_t x = 123456789;
static uint32_t y = 362436069;
static uint32_t z = 521288629;

void factorial_mpz(mpz_t mi_result, uint64_t n) {
    mpz_set_ui(mi_result, 1);
    while (n > 0) {
        mpz_mul_ui(mi_result, mi_result, n--);
    }
}

inline uint32_t xor32()
{
	y^=(y<<13);
	y^=(y>>17);
	return (y^=(y<<15));
}

inline uint32_t xor64()
{
	uint32_t t = (x^(x<<10));
	x = y;
	return y = (y^(y>>10))^(t^(t>>13));
}

inline uint32_t xor96()
{
	uint32_t t = (x^(x<<10));
	x = y;
	y = z;
	return z = (z^(z>>26))^(t^(t>>5));
}

inline uint32_t xor128()
{
	uint32_t t = (x^(x<<11));
	x = y;
	y = z;
	z = w;
	return w = (w^(w>>19))^(t^(t>>8));
}

inline uint32_t xor160()
{
	uint32_t t = (x^(x<<2));
	x = y;
	y = z;
	z = w;
	w = v;
	return v = (v^(v>>4))^(t^(t>>1));
}

inline uint32_t xorwow()
{
	uint32_t t = (x^(x>>2));
	x = y;
    y = z;
    z = w;
    w = v;
    v = (v^(v<<4))^(t^(t<<1));
    return (d+=362437)+v;
}

inline uint32_t rand_int(uint32_t n) {
    uint32_t limit = RAND_MAX - RAND_MAX % n;
    uint32_t rnd;
    while ((rnd = rand()) >= limit);
    return rnd % n;
}

void reseed_xor() {
    srand(time(NULL));
    d = rand_int(6615241);
    v = rand_int(5783321);
    w = rand_int(88675123);
    x = rand_int(123456789);
    y = rand_int(362436069);
    z = rand_int(521288629);
}

bool knuth_fisher_yates_shuffle(uint8_t *configuration, size_t atoms) {
    uint8_t temporary;
    size_t j;

    for (size_t i = atoms -1; i > 0; i--) {
        j = xorwow() % (i + 1);
        temporary = configuration[j];
        configuration[j] = configuration[i];
        configuration[i] = temporary;
    }
    return true;
}