#ifndef ENTROPY_H
#define ENTROPY_H

#include "fileio.h"

#define ENTROPY_LOW 5.0

double file_entropy(const pg_file *f);
double password_bits(int length, int alphabet_size);

#endif
