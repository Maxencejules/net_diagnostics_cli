#include "json.h"
#include <math.h>

void json_string(FILE *stream, const char *value) {
    fputc('"', stream);
    for (const unsigned char *p = (const unsigned char *)value; *p; ++p) {
        if (*p == '"' || *p == '\\') { fputc('\\', stream); fputc(*p, stream); }
        else if (*p < 0x20) fprintf(stream, "\\u%04x", (unsigned int)*p);
        else fputc(*p, stream);
    }
    fputc('"', stream);
}

void json_metric(FILE *stream, double value) {
    if (value < 0 || !isfinite(value)) fputs("null", stream);
    else fprintf(stream, "%.9g", value);
}
