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
}
