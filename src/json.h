#ifndef NETDIAG_JSON_H
#define NETDIAG_JSON_H
#include <stdio.h>
void json_string(FILE *stream, const char *value);
void json_metric(FILE *stream, double value);
#endif
