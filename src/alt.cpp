#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

#include "Converter/potree_converter_api.h"

using namespace std;
using namespace potree_converter;

#define DllExport __attribute__(( visibility("default") ))

extern "C" {
    DllExport const char* HelloThis() {
        return "Hello, this!";
    }

    DllExport const void RunConverter(char* source, char* outdir) {
        // return "testing this!";
        convert(source, outdir);
    }
}
