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
    if ((n - 1) == RAND_MAX) {
    return rand();
    } else {
        // Chop off all of the values that would cause skew...
        long end = RAND_MAX / n; // truncate skew
        //assert (end > 0L);
        end *= n;

        // ... and ignore results from rand() that fall above that limit.
        // (Worst case the loop condition should succeed 50% of the time,
        // so we can expect to bail out of this loop pretty quickly.)
        int r;
        while ((r = rand()) >= end);

        return r % n;
    }
}

void reseed_xor() {
      srand(time(NULL));
      d = rand_int(6615241);
      uint32_t t = (x^(x<<2));
      t = (x^(x<<2));
      x = y;
      y = z;
      z = w;
      w = v;
      v = (v^(v<<4))^(t^(t<<1));
      v = (d+=362437)+v;
      t = (x^(x<<2));
      x = y;
      y = z;
      z = w;
      w = v;
      v = (v^(v<<4))^(t^(t<<1));
      w = (d+=362437)+v;
      x = y;
      y = z;
      z = w;
      w = v;
      v = (v^(v<<4))^(t^(t<<1));
      x = (d+=362437)+v;
      t = (x^(x<<2));
      x = y;
      y = z;
      z = w;
      w = v;
      v = (v^(v<<4))^(t^(t<<1));
      y = (d+=362437)+v;
      t = (x^(x<<2));
      x = y;
      y = z;
      z = w;
      w = v;
      v = (v^(v<<4))^(t^(t<<1));
      z = (d+=362437)+v;;
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