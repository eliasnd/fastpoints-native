#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <filesystem>

#include "Converter/potree_converter_api.h"
#include "laslib/inc/lasreader_ply.hpp"
#include "laslib/inc/las2las.hpp"
#include "callbackstream.h"

using namespace std;
using namespace potree_converter;

typedef void (*LoggingCallback)(const char *message);

extern "C" {
    const char* HelloWorld();
    const char* TestCallback(LoggingCallback cb);
    void RunConverter(char* source, char* outDir, LoggingCallback cb);
}