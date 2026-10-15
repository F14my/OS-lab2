#ifndef LINE_IO_H
#define LINE_IO_H

#include <stddef.h>
#include <stdio.h>


int read_line(FILE *input, char **line, size_t *capacity);

#endif
