#include "contracts.h"

#include <math.h>
#include <stdlib.h>

int prime_count(int a, int b)
{
    if (a < 1 || b < a) {
        return -1;
    }
    if (b == 1) {
        return 0;
    }

    const size_t limit = (size_t)b;
    unsigned char *composite = calloc(limit + 1, sizeof *composite);
    if (composite == NULL) {
        return -1;
    }

    for (size_t prime = 2; prime <= limit / prime; ++prime) {
        if (composite[prime] == 0) {
            for (size_t multiple = prime * prime; multiple <= limit; multiple += prime) {
                composite[multiple] = 1;
            }
        }
    }

    int count = 0;
    for (size_t candidate = a < 2 ? 2 : (size_t)a; candidate <= limit; ++candidate) {
        count += composite[candidate] == 0;
    }

    free(composite);
    return count;
}

float e(int x)
{
    if (x < 1) {
        return NAN;
    }

    double term = 1.0;
    double sum = term;
    for (int n = 1; n <= x; ++n) {
        term /= n;
        double next_sum = sum + term;
        if (next_sum == sum) {
            break;
        }
        sum = next_sum;
    }
    return (float)sum;
}
