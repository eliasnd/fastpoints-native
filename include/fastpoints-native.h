#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <filesystem>

#include "Converter/potree_converter_api.h"
#include "laslib/inc/lasreader_ply.hpp"
#include "laslib/inc/las2las.hpp"
#include "callbackstream.h"

#include "laspoint.hpp"
#include "lasreader.hpp"

using namespace std;
using namespace potree_converter;

typedef void (*LoggingCallback)(const char *message);

struct Vector3
{
    float x;
    float y;
    float z;
};

struct Color {
    U16 r;
    U16 b;
    U16 g;
    U16 a;
};

extern "C" {
    const char* HelloWorld();
    const char* TestCallback(LoggingCallback cb);
    void RunConverter(char* source, char* outDir, LoggingCallback cb);
    void PopulateDecimatedCloud(char* source, Vector3* points, Color* colors, int decimatedSize, LoggingCallback cb);
}