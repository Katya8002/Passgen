#include <math.h>
#include <stdio.h>

#include "entropy.h"

double file_entropy(const pg_file *f) {
    unsigned long counts[256] = { 0 };
    unsigned char buf[4096];
    unsigned long total = 0;
    size_t n, i;
    double h = 0.0;
    double p;

    if (f == NULL || f->fp == NULL)
        return -1.0;
    if (fseek(f->fp, 0, SEEK_SET) != 0)
        return -1.0;

    while ((n = fread(buf, 1, sizeof buf, f->fp)) > 0) {
        for (i = 0; i < n; i++)
            counts[buf[i]]++;
        total += n;
    }
    if (ferror(f->fp) || total == 0)
        return -1.0;

    for (i = 0; i < 256; i++) {
        if (counts[i] == 0)
            continue; 
        p = (double)counts[i] / (double)total;
        h -= p * log2(p);
    }
    return h;
}

double password_bits(int length, int alphabet_size) {
    if (length <= 0 || alphabet_size <= 1)
        return 0.0;
    return (double)length * log2((double)alphabet_size);
}
