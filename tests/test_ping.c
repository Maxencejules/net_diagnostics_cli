#include "ping.h"
#include "json.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int fixture(const char *directory, const char *name, double loss, double min, double max, double avg) {
    char path[1024], line[1024];
    snprintf(path, sizeof(path), "%s/%s.txt", directory, name);
    FILE *file = fopen(path, "r");
    if (!file) { fprintf(stderr, "Cannot open %s\n", path); return 1; }
    PingResult result;
    ping_result_init(&result);
    while (fgets(line, sizeof(line), file)) ping_parse_line(line, &result);
    fclose(file);
    if (fabs(result.loss_percent - loss) > 0.000001 || fabs(result.min_ms - min) > 0.000001 ||
        fabs(result.max_ms - max) > 0.000001 || fabs(result.avg_ms - avg) > 0.000001) {
        fprintf(stderr, "%s: actual loss/min/max/avg %.9g/%.9g/%.9g/%.9g\n", name,
                result.loss_percent, result.min_ms, result.max_ms, result.avg_ms);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) return 1;
    int errors = 0;
    errors += fixture(argv[1], "windows", 25, 1, 3, 2);
    errors += fixture(argv[1], "linux", 0, 0.027, 0.072, 0.042);
    errors += fixture(argv[1], "macos", 12.5, 1.125, 2.25, 1.75);
    errors += fixture(argv[1], "all_lost", 100, -1, -1, -1);
    errors += fixture(argv[1], "localized", -1, -1, -1, -1);
    errors += fixture(argv[1], "malformed", -1, -1, -1, -1);
    FILE *file = tmpfile();
    if (!file) return 1;
    json_string(file, "quote\" slash\\ tab\t line\n");
    fputc(' ', file); json_metric(file, -1);
    fputc(' ', file); json_metric(file, NAN);
    rewind(file);
    char text[256];
    size_t length = fread(text, 1, sizeof(text) - 1, file);
    text[length] = '\0';
    fclose(file);
    if (strcmp(text, "\"quote\\\" slash\\\\ tab\\u0009 line\\u000a\" null null")) {
        fprintf(stderr, "Invalid JSON escaping/unknown metric output: %s\n", text);
        ++errors;
    }
    return errors ? 1 : 0;
}
