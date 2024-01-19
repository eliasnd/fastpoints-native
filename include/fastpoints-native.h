// fastpoints-native
// Copyright (C) 2023  Elias Neuman-Donihue

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#define NOMINMAX

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

#if defined(__APPLE__)
    #define DllExport __attribute__(( visibility("default") ))
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    #define DllExport __declspec(dllexport)
#endif

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
    DllExport void InitializeConverter(LoggingCallback cb);
    DllExport void StopConverter();

    DllExport NativePointCloudHandle* AddPointCloud(const char* source, Vector3* points, Color* colors, int decimated_size, const char* outdir, const char* method, const char* encoding, const char* chunk_method, StatusCallback scb, ProgressCallback pcb);
    DllExport bool RemovePointCloud(NativePointCloudHandle* handle);
}