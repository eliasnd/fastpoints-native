#include "fastpoints-native.h"

using namespace std;
using namespace potree_converter;

#define DllExport __attribute__(( visibility("default") ))

extern "C" {
    DllExport const char* HelloWorld() {
        return "Hello, world!";
    }

    DllExport const char* TestCallback(LoggingCallback cb) {
        callbackstream<> redirect(std::cout, cb);
        cb("Testing 1!");
        cout << "Testing 2!" << endl;
        return "Testing 3";
    }

    DllExport void RunConverter(char* source, char* outdir, LoggingCallback cb) {
        // callbackstream<> redirect_cout(std::cout, cb);
        // callbackstream<> redirect_stderr(stderr, cb);

        char* file_name = source;

        bool conversion_needed = 
            strstr(file_name, ".las") == NULL && strstr(file_name, ".LAS") == NULL &&
            strstr(file_name, ".laz") == NULL && strstr(file_name, ".LAZ") == NULL;

        if (conversion_needed) {
            cb("running conversion");
            char arg0[] = "./las2las64";

            char arg1[] = "-i";
            char arg2[strlen(file_name)];
            strncpy(arg2, file_name, strlen(file_name));

            file_name = "tmp.laz";

            char arg3[] = "-o";
            char arg4[strlen(file_name)];
            strncpy(arg4, file_name, strlen(file_name));

            char* argv[] = {arg0, arg1, arg2, arg3, arg4};
            las2las(5, argv);
            cb("conversion complete");
        }
        
        cb("starting octree generation");
        convert(file_name, outdir);
        cb("octree done");

        if (conversion_needed) {
            remove(file_name);
        }
    }

    DllExport void PopulateDecimatedCloud(char* source, Vector3* points, Color* colors, int decimatedSize, LoggingCallback cb) {
        cb("start");
        LASreadOpener lasreadopener;
        cb("1");
        lasreadopener.set_file_name(source);
        cb("2");
        LASreader* reader = lasreadopener.open();
        cb("3");
        int interval = reader->npoints / decimatedSize;
        cb(("count " + std::to_string(reader->p_count) + " or " + std::to_string(reader->npoints)).c_str());
        cb("4");
        int idx = 0;
        cb("5");

        for (int i = 0; i < decimatedSize; i++) {
            reader->seek(idx);
            if (i % 1000 == 0) {
                cb(("on point " + std::to_string(i)).c_str());
                cb(("seeking idx " + std::to_string(idx)).c_str());
                cb(("reader got point " + std::to_string(reader->point.X) + ", " + std::to_string(reader->point.Y) + ", " + std::to_string(reader->point.Z)).c_str());
                cb(("reader got color " + std::to_string(reader->point.rgb[0]) + ", " + std::to_string(reader->point.rgb[1]) + ", " + std::to_string(reader->point.rgb[2]) + ", " + std::to_string(reader->point.rgb[3])).c_str());
            }
            points[i] = (Vector3){reader->point.X, reader->point.Y, reader->point.Z};
            colors[i] = (Color){reader->point.rgb[0], reader->point.rgb[1], reader->point.rgb[2], reader->point.rgb[3]};
            idx += interval;
        }

        cb("6");

        reader->close();

        cb("7");
    }
}
