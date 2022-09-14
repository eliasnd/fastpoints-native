#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <filesystem>

#include "Converter/potree_converter_api.h"
#include "laslib/inc/lasreader_ply.hpp"
#include "laslib/inc/las2las.hpp"
#include "callbackstream.h"
#include "Converter/modules/unsuck/unsuck.hpp"
#include "Converter/modules/unsuck/TaskPool.hpp"

#include "laspoint.hpp"
#include "lasreader.hpp"
#include "structures.h"
#include "laszip/laszip_api.h"

using std::mutex;

using namespace std;
using namespace potree_converter;

typedef void (*LoggingCallback)(const char *message);

extern "C" {
    const char* HelloWorld();
    const char* TestCallback(LoggingCallback cb);
    void RunConverter(char* source, char* outDir, LoggingCallback cb);
    void PopulateDecimatedCloud(char* source, Vector3* points, Color* colors, int decimatedSize, LoggingCallback cb);
}