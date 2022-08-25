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
        callbackstream<> redirect(std::cout, cb);
        convert(source, outdir);
    }
}
