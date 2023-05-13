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

    bool ShouldCancel() {
        lock_guard<mutex> lock(mtx);
        return cancel_conversion;
    }

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
            bool* should_cancel;
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
                        if (*task->should_cancel) {
                            free(posBuffer);
                            free(colBuffer);
                            return;
                        }

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

                    laszip_header* header;
                    laszip_get_header_pointer(laszip_reader, &header);

                    double scale[3];
                    double offset[3];

                    scale[0] = header->x_scale_factor;
                    scale[1] = header->y_scale_factor;
                    scale[2] = header->z_scale_factor;

                    offset[0] = header->x_offset;
                    offset[1] = header->y_offset;
                    offset[2] = header->z_offset;
                    // laszip_seek_point(laszip_reader, task->firstPoint);

                    for (int i = 0; i < num_to_read; i++) {
                        if (i % 1000 == 0) {
                            lock_guard<mutex> lock(*task->mtx);

                            if (*task->should_cancel) {
                                free(posBuffer);
                                free(colBuffer);
                                return;
                            }

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

                        posBuffer[i] = (Vector3){
                            (float)(coordinates[0] / scale[0] + offset[0]), 
                            (float)(coordinates[1] / scale[1] + offset[1]), 
                            (float)(coordinates[2] / scale[2] + offset[2])
                        };

                        colBuffer[i] = (Color){ // 0, 0, 0, 1 };
                            (uint8_t)(point_ptr->rgb[0] > 255 ? point_ptr->rgb[0] / 256.0 : point_ptr->rgb[0]), // (255.0 * point_ptr->rgb[0] / 65535),
                            (uint8_t)(point_ptr->rgb[1] > 255 ? point_ptr->rgb[1] / 256.0 : point_ptr->rgb[1]), // (255.0 * point_ptr->rgb[1] / 65535),
                            (uint8_t)(point_ptr->rgb[2] > 255 ? point_ptr->rgb[2] / 256.0 : point_ptr->rgb[2]), // (255.0 * point_ptr->rgb[2] / 65535),
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

                    laszip_header* header;
                    laszip_get_header_pointer(laszip_reader, &header);

                    double scale[3];
                    double offset[3];

                    scale[0] = header->x_scale_factor;
                    scale[1] = header->y_scale_factor;
                    scale[2] = header->z_scale_factor;

                    offset[0] = header->x_offset;
                    offset[1] = header->y_offset;
                    offset[2] = header->z_offset;

                    for (int i = 0; i < num_to_read; i++) {
                        if (i % 1000 == 0) {
                            lock_guard<mutex> lock(*task->mtx);
                            
                            if (*task->should_cancel) {
                                free(posBuffer);
                                free(colBuffer);
                                return;
                            }

                            *task->progress = i;
                            if (debug)
                                logging_callback(("Thread " + std::to_string(start / (num_to_read * num_to_skip)) + " scanning point " + std::to_string(i) + "/" + std::to_string(num_to_read)).c_str());
                        }

                        double coordinates[3];

                        laszip_read_point(laszip_reader);
				        laszip_get_coordinates(laszip_reader, coordinates);

                        laszip_point* point_ptr;
                        laszip_get_point_pointer(laszip_reader, &point_ptr);
        
                        posBuffer[i] = (Vector3){
                            (float)(coordinates[0] / scale[0] + offset[0]), 
                            (float)(coordinates[1] / scale[1] + offset[1]), 
                            (float)(coordinates[2] / scale[2] + offset[2])
                        };

                        colBuffer[i] = (Color){ // 0, 0, 0, 1 };
                            (uint8_t)(point_ptr->rgb[0] > 255 ? point_ptr->rgb[0] / 256.0 : point_ptr->rgb[0]), // (255.0 * point_ptr->rgb[0] / 65535),
                            (uint8_t)(point_ptr->rgb[1] > 255 ? point_ptr->rgb[1] / 256.0 : point_ptr->rgb[1]), // (255.0 * point_ptr->rgb[1] / 65535),
                            (uint8_t)(point_ptr->rgb[2] > 255 ? point_ptr->rgb[2] / 256.0 : point_ptr->rgb[2]), // (255.0 * point_ptr->rgb[2] / 65535),
                            1 // (uint8_t)point_ptr->rgb[3]
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
        bool should_cancel = false;

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
            task->should_cancel = &should_cancel;

            pool.addTask(task);
        }

        while (!pool.isWorkDone()) {
            int progress = 0;

            {
                lock_guard<mutex> lock(task_mtx);
                for (int i = 0; i < threadCount; i++)
                    progress += progress_values[i];
                should_cancel = ShouldCancel();
            }

            handle->progressCallback((float)progress / (float)handle->decimated_size);

            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        handle->progressCallback(1);
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

            if (ShouldCancel())
            {
                handle->statusCallback(6);
                remove(out_name);
                return;
            }

            std::chrono::steady_clock::time_point conversion_end = std::chrono::steady_clock::now();
            if (debug)
                logging_callback(("Converted ply in " + to_string(std::chrono::duration_cast<std::chrono::milliseconds>(conversion_end - conversion_start).count()) + " ms").c_str());

            std::chrono::steady_clock::time_point octree_start = std::chrono::steady_clock::now();
            if (debug)
                logging_callback("Starting octree construction");

            convert(out_name, handle->outdir, handle->method, handle->encoding, handle->chunk_method, handle->progressCallback, ShouldCancel);

            std::chrono::steady_clock::time_point octree_end = std::chrono::steady_clock::now();
            if (debug)
                logging_callback(("Constructed octree in " + to_string(std::chrono::duration_cast<std::chrono::milliseconds>(octree_end - octree_start).count()) + " ms").c_str());

            remove(out_name);
        } else {
            std::chrono::steady_clock::time_point octree_start = std::chrono::steady_clock::now();
            if (debug)
                logging_callback("Starting octree construction");

            convert(file_name, handle->outdir, handle->method, handle->encoding, handle->chunk_method, handle->progressCallback, ShouldCancel);

            std::chrono::steady_clock::time_point octree_end = std::chrono::steady_clock::now();
            if (debug)
                logging_callback(("Constructed octree in " + to_string(std::chrono::duration_cast<std::chrono::milliseconds>(octree_end - octree_start).count()) + " ms").c_str());
        }
    }

    void ConverterThread() {
        if (debug)
            logging_callback("Starting converter thread!");

        while (true) {
            NativePointCloudHandle* handle_to_decimate = NULL;
            NativePointCloudHandle* handle_to_convert = NULL;

            /* <<< ACQUIRE LOCK >>> */
            {
                lock_guard<mutex> lock(mtx);

                if (stop_signal)
                    break;

                cancel_conversion = false;  // If set true during last loop, reset

                if (decimation_queue->size() > 0) {
                    handle_to_decimate = decimation_queue->front();
                    current_handle = handle_to_decimate;
                    decimation_queue->pop_front();
                } else if (converter_queue->size() > 0) {
                    handle_to_convert = converter_queue->front();
                    current_handle = handle_to_convert;
                    converter_queue->pop_front();
                }
            }
            /* <<< RELEASE LOCK >>> */

            if (handle_to_decimate != NULL) {
                handle_to_decimate->statusCallback(2);    // Decimating status
                PopulateDecimatedCloud(handle_to_decimate);

                if (ShouldCancel()) // Checks if cancel flag set during decimation
                {
                    handle_to_decimate->statusCallback(6);  // Aborted status
                    continue;
                }

                handle_to_decimate->statusCallback(3);    // Waiting for conversion status
                handle_to_decimate->progressCallback(-1);
                {
                    lock_guard<mutex> lock(mtx);
                    converter_queue->emplace_back(handle_to_decimate);
                }
            } else if (handle_to_convert != NULL) {
                handle_to_convert->statusCallback(4); // Converting status
                handle_to_convert->progressCallback(0);
                RunConverter(handle_to_convert);
                handle_to_convert->statusCallback(5); // Done status
                handle_to_convert->progressCallback(-1);
            } else {
                this_thread::sleep_for(chrono::milliseconds(300));
            }
        }
    }

    DllExport void InitializeConverter(LoggingCallback cb) {
        if (debug)
            logging_callback("Initialize converter");

        lock_guard<mutex> lock(mtx);
        
        logging_callback = cb;
        converter_queue = new deque<NativePointCloudHandle*>();
        decimation_queue = new deque<NativePointCloudHandle*>();
        converter_thread = new thread(ConverterThread);
    }

    DllExport void StopConverter() {
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

        decimation_queue->emplace_back(handle);
        return handle;
    }

    DllExport bool RemovePointCloud(NativePointCloudHandle* handle) {
        string source = handle->source;
        if (debug)
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
