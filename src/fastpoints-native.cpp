#include "fastpoints-native.h"
#include <chrono>

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

        if (strstr(file_name, ".las") == NULL && strstr(file_name, ".LAS") == NULL &&
            strstr(file_name, ".laz") == NULL && strstr(file_name, ".LAZ") == NULL) {

            char arg0[] = "./las2las64";

            char arg1[] = "-i";
            char arg2[strlen(file_name)];
            strcpy(arg2, file_name);

            char* out_name = "tmp.las";

            char arg3[] = "-o";
            char arg4[strlen(out_name)];
            strncpy(arg4, out_name, strlen(out_name));

            char* argv[] = {arg0, arg1, arg2, arg3, arg4};

            std::chrono::steady_clock::time_point conversion_start = std::chrono::steady_clock::now();
            cb("Starting ply conversion");

            las2las(5, argv);

            std::chrono::steady_clock::time_point conversion_end = std::chrono::steady_clock::now();
            cb(("Converted ply in " + to_string(std::chrono::duration_cast<std::chrono::milliseconds>(conversion_end - conversion_start).count()) + " ms").c_str());

            std::chrono::steady_clock::time_point octree_start = std::chrono::steady_clock::now();
            cb("Starting octree construction");

            convert(out_name, outdir);

            std::chrono::steady_clock::time_point octree_end = std::chrono::steady_clock::now();
            cb(("Constructed octree in " + to_string(std::chrono::duration_cast<std::chrono::milliseconds>(octree_end - octree_start).count()) + " ms").c_str());

            remove(out_name);
        } else {
            std::chrono::steady_clock::time_point octree_start = std::chrono::steady_clock::now();
            cb("Starting octree construction");

            convert(file_name, outdir);

            std::chrono::steady_clock::time_point octree_end = std::chrono::steady_clock::now();
            cb(("Constructed octree in " + to_string(std::chrono::duration_cast<std::chrono::milliseconds>(octree_end - octree_start).count()) + " ms").c_str());
        }
    }

    DllExport void PopulateDecimatedCloud(char* source, Vector3* points, Color* colors, int decimatedSize, LoggingCallback cb) {
        callbackstream<> redirect_cout(std::cout, cb);
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        LASreadOpener lasreadopener;
        lasreadopener.set_file_name(source);
        LASreader* reader = lasreadopener.open();

        int interval = reader->npoints / decimatedSize;
        int idx = 0;

        auto threadCount = getCpuData().numProcessors;

        struct Task {
			char* path;
            int* offset;
            mutex mtx;
			int64_t firstPoint;
			int64_t numPoints;
            int64_t numSkip;
		};

		auto processor = [points, colors, cb](shared_ptr<Task> task){
			char* path = task->path;
			int64_t start = task->firstPoint;
			int64_t numToRead = task->numPoints;
            int64_t numToSkip = task->numSkip;
            int* offset = task->offset;
            int pid = *offset / numToRead;

            thread_local Vector3* posBuffer = (Vector3*)malloc(numToRead * 12);
            thread_local Color* colBuffer = (Color*)malloc(numToRead * 4);

            LASreadOpener lasreadopener;
            lasreadopener.set_file_name(path);
            LASreader* reader = lasreadopener.open();

            if (strstr(path, ".ply") || strstr(path, ".PLY")) {
                ((LASreaderPLY*)reader)->stream_seek(start);

                for (int i = 0; i < numToRead; i++) {
                    if (i % 1000 == 0)
                        cout << "Thread " + std::to_string(start / (numToRead * numToSkip)) + " scanning point " + std::to_string(i) + "/" + std::to_string(numToRead);
                    reader->seek(start + numToSkip * i);
                    posBuffer[i] = (Vector3){(float)reader->point.X, (float)reader->point.Y, (float)reader->point.Z};
                    colBuffer[i] = (Color){
                        (uint8_t)reader->point.rgb[0],
                        (uint8_t)reader->point.rgb[1],
                        (uint8_t)reader->point.rgb[2],
                        1 // (uint8_t)reader->point.rgb[3]
                    };
                }
            } else if (strstr(path, ".las") || strstr(path, ".LAS")) { 
                laszip_POINTER laszip_reader;
                {
                    laszip_BOOL is_compressed = iEndsWith(path, ".laz") ? 1 : 0;
                    laszip_BOOL request_reader = 1;

                    laszip_create(&laszip_reader);
                    laszip_request_compatibility_mode(laszip_reader, request_reader);
                    laszip_open_reader(laszip_reader, path, &is_compressed);
                    // laszip_seek_point(laszip_reader, task->firstPoint);

                    for (int i = 0; i < numToRead; i++) {
                        if (i % 1000 == 0)
                            cout << "Thread " + std::to_string(start / (numToRead * numToSkip)) + " scanning point " + std::to_string(i) + "/" + std::to_string(numToRead);

                        laszip_seek_point(laszip_reader, task->firstPoint + numToSkip * i);

                        double coordinates[3];

                        laszip_read_point(laszip_reader);
				        laszip_get_coordinates(laszip_reader, coordinates);

                        laszip_point* point_ptr;
                        laszip_get_point_pointer(laszip_reader, &point_ptr);

                        posBuffer[i] = (Vector3){(float)coordinates[0], (float)coordinates[1], (float)coordinates[2]};
                        colBuffer[i] = (Color){ // 0, 0, 0, 1 };
                            (uint8_t)(255.0 * point_ptr->rgb[0] / 65535),
                            (uint8_t)(255.0 * point_ptr->rgb[1] / 65535),
                            (uint8_t)(255.0 * point_ptr->rgb[2] / 65535),
                            1 // (uint8_t)point_ptr->rgb[3]
                        };
                    }
                }
            } else {
                laszip_POINTER laszip_reader;
                {
                    laszip_BOOL is_compressed = iEndsWith(path, ".laz") ? 1 : 0;
                    laszip_BOOL request_reader = 1;

                    laszip_create(&laszip_reader);
                    laszip_request_compatibility_mode(laszip_reader, request_reader);
                    laszip_open_reader(laszip_reader, path, &is_compressed);
                    laszip_seek_point(laszip_reader, task->firstPoint);

                    for (int i = 0; i < numToRead; i++) {
                        if (i % 1000 == 0)
                            cout << "Thread " + std::to_string(start / (numToRead * numToSkip)) + " scanning point " + std::to_string(i) + "/" + std::to_string(numToRead);

                        double coordinates[3];

                        laszip_read_point(laszip_reader);
				        laszip_get_coordinates(laszip_reader, coordinates);

                        laszip_point* point_ptr;
                        laszip_get_point_pointer(laszip_reader, &point_ptr);
        
                        posBuffer[i] = (Vector3){(float)coordinates[0], (float)coordinates[1], (float)coordinates[2]};
                        colBuffer[i] = (Color){ // 0, 0, 0, 1 };
                            (uint8_t)point_ptr->rgb[0],
                            (uint8_t)point_ptr->rgb[1],
                            (uint8_t)point_ptr->rgb[2],
                            (uint8_t)point_ptr->rgb[3]
                        };

                        for (int j = 0; j < numToSkip; j++)
                            laszip_read_point(laszip_reader);
                    }
                }
            }

            reader->close();

            task->mtx.lock();

            for (int i = 0; i < numToRead; i++) {
                points[*offset+i] = posBuffer[i];
                colors[*offset+i] = colBuffer[i];
            }

            *offset += numToRead;

            task->mtx.unlock();

            free(posBuffer);
            free(colBuffer);
            
		};

        TaskPool<Task> pool(threadCount, processor);

        int threadBudget = decimatedSize / threadCount;
        int* offset = new int(0);

        for (int i = 0; i < threadCount; i++) {
            auto task = make_shared<Task>();
            task->path = source;
            task->offset = offset;
            task->firstPoint = threadBudget * interval * i;
            task->numPoints = threadBudget;
            task->numSkip = interval;

            pool.addTask(task);
        }

        pool.waitTillEmpty();
    }
}
