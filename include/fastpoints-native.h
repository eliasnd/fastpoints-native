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

typedef void (*LoggingCallback)(const char* message);
typedef void (*ProgressCallback)(float value);
typedef void (*StatusCallback)(int status); // Status callback is called when status of a point cloud changes

class NativePointCloudHandle {
    public:
    string source;

    Vector3* points;
    Color* colors;
    int decimated_size;

    string outdir;
    string method;
    string encoding;
    string chunk_method;
    ProgressCallback progressCallback;
    StatusCallback statusCallback;

    ~NativePointCloudHandle()
    {
        int i = 0;
    }
};

extern "C" {
    void InitializeConverter(LoggingCallback cb);
    void StopConverter();

    NativePointCloudHandle* AddPointCloud(const char* source, Vector3* points, Color* colors, int decimated_size, const char* outdir, const char* method, const char* encoding, const char* chunk_method, StatusCallback scb, ProgressCallback pcb);
    bool RemovePointCloud(NativePointCloudHandle* handle);
}