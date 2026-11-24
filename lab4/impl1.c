#include "contracts.h"

#include <math.h>

int prime_count(int a, int b)
{
    if (a < 1 || b < a) {
        return -1;
    }

    int count = 0;
    for (long long candidate = a < 2 ? 2 : a; candidate <= b; ++candidate) {
        int divisor = 2;
        while (divisor < candidate && candidate % divisor != 0) {
            ++divisor;
        }
        if (divisor == candidate) {
            ++count;
        }
    }
    return count;
}

float e(int x)
{
    if (x < 1) {
        return NAN;
    }

    return (float)pow(1.0 + 1.0 / x, x);
}
