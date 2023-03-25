#include "fastpoints-native.h"
#include <chrono>

using namespace std;
using namespace potree_converter;

#define DllExport __attribute__(( visibility("default") ))

extern "C" {
    LoggingCallback logging_callback;

    mutex mtx;
    deque<NativePointCloudHandle*>* decimation_queue = NULL;
    deque<NativePointCloudHandle*>* converter_queue = NULL;
    thread* converter_thread;
    NativePointCloudHandle* current_handle = NULL;
    bool stop_signal = false;   // Stops entire converter, end checking queue
    bool cancel_conversion = false;   // Stops current conversion, continue checking queue

    bool debug = false;

    void PopulateDecimatedCloud(NativePointCloudHandle* handle) { //, Vector3* points, Color* colors, int decimatedSize, ProgressCallback pcb, LoggingCallback lcb, bool debug) {
        callbackstream<> redirect_cout(cout, logging_callback);
        chrono::steady_clock::time_point begin = chrono::steady_clock::now();

        LASreadOpener lasreadopener;
        lasreadopener.set_file_name(handle->source.c_str());
        LASreader* reader = lasreadopener.open();

        int interval = reader->npoints / handle->decimated_size;
        int idx = 0;

        auto threadCount = getCpuData().numProcessors;

        struct Task {
			const char* path;
            int* offset;
            int64_t first_point;
			int64_t num_points;
            int64_t num_skip;

            mutex* mtx;
            int* progress;
		};

        int progress = 0;

		auto processor = [handle, progress](shared_ptr<Task> task){
			const char* path = task->path;
			int64_t start = task->first_point;
			int64_t num_to_read = task->num_points;
            int64_t num_to_skip = task->num_skip;
            int* offset = task->offset;
            int pid = *offset / num_to_read;

            thread_local Vector3* posBuffer = (Vector3*)malloc(num_to_read * 12);
            thread_local Color* colBuffer = (Color*)malloc(num_to_read * 4);

            LASreadOpener lasreadopener;
            lasreadopener.set_file_name(path);
            LASreader* reader = lasreadopener.open();

            if (strstr(path, ".ply") || strstr(path, ".PLY")) {
                ((LASreaderPLY*)reader)->stream_seek(start);

                for (int i = 0; i < num_to_read; i++) {
                    if (i % 1000 == 0) {
                        lock_guard<mutex> lock(*task->mtx);
                        *task->progress = i;
                        if (debug)
                            logging_callback(("Thread " + std::to_string(start / (num_to_read * num_to_skip)) + " scanning point " + std::to_string(i) + "/" + std::to_string(num_to_read)).c_str());
                    }

                    reader->seek(start + num_to_skip * i);
                    posBuffer[i] = (Vector3){(float)reader->point.X, (float)reader->point.Y, (float)reader->point.Z};
                    colBuffer[i] = (Color){
                        (uint8_t)reader->point.rgb[0],
                        (uint8_t)reader->point.rgb[1],
                        (uint8_t)reader->point.rgb[2],
                        1 // (uint8_t)reader->point.rgb[3]
                    };
                }

                {
                    lock_guard<mutex> lock(*task->mtx);
                    *task->progress = num_to_read;
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

                    for (int i = 0; i < num_to_read; i++) {
                        if (i % 1000 == 0) {
                            lock_guard<mutex> lock(*task->mtx);
                            *task->progress = i;
                            if (debug)
                                logging_callback(("Thread " + std::to_string(start / (num_to_read * num_to_skip)) + " scanning point " + std::to_string(i) + "/" + std::to_string(num_to_read)).c_str());
                        }

                        laszip_seek_point(laszip_reader, task->first_point + num_to_skip * i);

                        double coordinates[3];

                        laszip_read_point(laszip_reader);
				        laszip_get_coordinates(laszip_reader, coordinates);

                        laszip_point* point_ptr;
                        laszip_get_point_pointer(laszip_reader, &point_ptr);

                        posBuffer[i] = (Vector3){(float)coordinates[0], (float)coordinates[1], (float)coordinates[2]};
                        colBuffer[i] = (Color){ // 0, 0, 0, 1 };
                            (uint8_t)point_ptr->rgb[0], // (255.0 * point_ptr->rgb[0] / 65535),
                            (uint8_t)point_ptr->rgb[1], // (255.0 * point_ptr->rgb[1] / 65535),
                            (uint8_t)point_ptr->rgb[2], // (255.0 * point_ptr->rgb[2] / 65535),
                            1 // (uint8_t)point_ptr->rgb[3]
                        };
                    }

                    {
                        lock_guard<mutex> lock(*task->mtx);
                        *task->progress = num_to_read;
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
                    laszip_seek_point(laszip_reader, task->first_point);

                    for (int i = 0; i < num_to_read; i++) {
                        if (i % 1000 == 0) {
                            lock_guard<mutex> lock(*task->mtx);
                            *task->progress = i;
                            if (debug)
                                logging_callback(("Thread " + std::to_string(start / (num_to_read * num_to_skip)) + " scanning point " + std::to_string(i) + "/" + std::to_string(num_to_read)).c_str());
                        }

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

                        for (int j = 0; j < num_to_skip; j++)
                            laszip_read_point(laszip_reader);
                    }

                    {
                        lock_guard<mutex> lock(*task->mtx);
                        *task->progress = num_to_read;
                    }
                }
            }
            reader->close();

            {
                lock_guard<mutex> lock(*task->mtx);
                for (int i = 0; i < num_to_read; i++) {
                    handle->points[*offset+i] = posBuffer[i];
                    handle->colors[*offset+i] = colBuffer[i];
                }
            }

            *offset += num_to_read;

            free(posBuffer);
            free(colBuffer);
            
		};

        TaskPool<Task> pool(threadCount, processor);

        int threadBudget = handle->decimated_size / threadCount;
        int* offset = new int(0);

        mutex task_mtx;
        int* progress_values = new int[threadCount];

        for (int i = 0; i < threadCount; i++) {
            progress_values[i] = 0;

            auto task = make_shared<Task>();
            task->path = handle->source.c_str();
            task->offset = offset;
            task->first_point = threadBudget * interval * i;
            task->num_points = threadBudget;
            task->num_skip = interval;
            task->mtx = &task_mtx;
            task->progress = &progress_values[i];

            pool.addTask(task);
        }

        while (!pool.isWorkDone()) {
            int progress = 0;

            {
                lock_guard<mutex> lock(task_mtx);
                for (int i = 0; i < threadCount; i++)
                    progress += progress_values[i];
            }

            handle->progressCallback((float)progress / (float)handle->decimated_size);

            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        handle->progressCallback(1);
        logging_callback("Decimation done");
    }

    DllExport void RunConverter(NativePointCloudHandle* handle) {
        // callbackstream<> redirect_cout(std::cout, cb);
        // callbackstream<> redirect_stderr(stderr, cb);

        const char* file_name = handle->source.c_str();

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
            // lcb("Starting ply conversion");

            las2las(5, argv);

            std::chrono::steady_clock::time_point conversion_end = std::chrono::steady_clock::now();
            // lcb(("Converted ply in " + to_string(std::chrono::duration_cast<std::chrono::milliseconds>(conversion_end - conversion_start).count()) + " ms").c_str());

            std::chrono::steady_clock::time_point octree_start = std::chrono::steady_clock::now();
            // lcb("Starting octree construction");

            convert(out_name, handle->outdir, handle->method, handle->encoding, handle->chunk_method, handle->progressCallback);

            std::chrono::steady_clock::time_point octree_end = std::chrono::steady_clock::now();
            // lcb(("Constructed octree in " + to_string(std::chrono::duration_cast<std::chrono::milliseconds>(octree_end - octree_start).count()) + " ms").c_str());

            remove(out_name);
        } else {
            std::chrono::steady_clock::time_point octree_start = std::chrono::steady_clock::now();
            // lcb("Starting octree construction");

            convert(file_name, handle->outdir, handle->method, handle->encoding, handle->chunk_method, handle->progressCallback);

            std::chrono::steady_clock::time_point octree_end = std::chrono::steady_clock::now();
            // lcb(("Constructed octree in " + to_string(std::chrono::duration_cast<std::chrono::milliseconds>(octree_end - octree_start).count()) + " ms").c_str());
        }
    }

    void ConverterThread() {
        logging_callback("Starting converter thread!");
        while (true) {
            NativePointCloudHandle* handleToDecimate = NULL;
            NativePointCloudHandle* handleToConvert = NULL;

            {
                lock_guard<mutex> lock(mtx);
                if (stop_signal)
                    break;

                if (decimation_queue->size() > 0) {
                    handleToDecimate = decimation_queue->front();
                    decimation_queue->pop_front();
                } else if (converter_queue->size() > 0) {
                    handleToConvert = converter_queue->front();
                    converter_queue->pop_front();
                }
            }

            if (handleToDecimate != NULL) {
                handleToDecimate->statusCallback(2);    // Decimating status
                PopulateDecimatedCloud(handleToDecimate);
                handleToDecimate->statusCallback(3);    // Waiting for conversion status
                handleToDecimate->progressCallback(-1);
                {
                    lock_guard<mutex> lock(mtx);
                    converter_queue->emplace_back(handleToDecimate);
                }
            } else if (handleToConvert != NULL) {
                handleToConvert->statusCallback(4); // Converting status
                handleToConvert->progressCallback(0);
                RunConverter(handleToConvert);
                handleToConvert->statusCallback(5); // Done status
                handleToConvert->progressCallback(-1);
            } else {
                this_thread::sleep_for(chrono::milliseconds(300));
            }
        }
    }

    DllExport void InitializeConverter(LoggingCallback cb) {
        // logging_callback("Initialize converter");
        lock_guard<mutex> lock(mtx);
        
        logging_callback = cb;
        converter_queue = new deque<NativePointCloudHandle*>();
        decimation_queue = new deque<NativePointCloudHandle*>();
        converter_thread = new thread(ConverterThread);
    }

    DllExport void StopConverter() {
        logging_callback("Stop converter");
        {
            lock_guard<mutex> lock(mtx);

            stop_signal = true;
        }

        converter_thread->join();
        delete decimation_queue;
        delete converter_queue;
    }

    DllExport NativePointCloudHandle* AddPointCloud(const char* source, Vector3* points, Color* colors, int decimated_size, const char* outdir, const char* method, const char* encoding, const char* chunk_method, StatusCallback scb, ProgressCallback pcb) {
        lock_guard<mutex> lock(mtx);

        if (decimation_queue == NULL || stop_signal)
            return NULL;

        scb(1);

        NativePointCloudHandle* handle = new NativePointCloudHandle();
        handle->source = source;
        handle->points = points;
        handle->colors = colors;
        handle->decimated_size = decimated_size;
        handle->outdir = outdir;
        handle->method = method;
        handle->encoding = encoding;
        handle->chunk_method = chunk_method;
        handle->progressCallback = pcb;
        handle->statusCallback = scb;

        logging_callback(("Add point cloud " + handle->source).c_str());

        decimation_queue->emplace_back(handle);
        return handle;
    }

    DllExport bool RemovePointCloud(NativePointCloudHandle* handle) {
        string source = handle->source;
        logging_callback(("Remove point cloud " + source).c_str());
        lock_guard<mutex> lock(mtx);

        if (stop_signal)
            return true;

        if (current_handle == handle) {
            cancel_conversion = true;
            return true;
        }
        
        if (converter_queue == NULL)
            return false;

        deque<NativePointCloudHandle*>::iterator it = decimation_queue->begin();
        while (it != decimation_queue->end()) {
            if (handle == *it) {
                decimation_queue->erase(it);
                return true;
            }
            it++;
        }

        it = converter_queue->begin();
        while (it != converter_queue->end()) {
            if (handle == *it) {
                converter_queue->erase(it);
                return true;
            }
            it++;
        }

        return false;
    }
}
