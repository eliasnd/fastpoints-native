laszip_POINTER laszip_reader;
laszip_header* header;
laszip_point* point;

laszip_BOOL request_reader = 1;
laszip_BOOL is_compressed = iEndsWith(path, ".laz") ? 1 : 0;

laszip_create(&laszip_reader);
laszip_request_compatibility_mode(laszip_reader, request_reader);
laszip_open_reader(laszip_reader, path.c_str(), &is_compressed);
laszip_get_header_pointer(laszip_reader, &header);
laszip_get_point_pointer(laszip_reader, &point);

laszip_seek_point(laszip_reader, task->firstPoint);

double coordinates[3];
auto aPosition = outputAttributes.get("position");

for (int64_t i = 0; i < batchSize; i++) {
    laszip_read_point(laszip_reader);
    laszip_get_coordinates(laszip_reader, coordinates);

    int64_t offset = i * outputAttributes.bytes;

    { // copy position
        double x = coordinates[0];
        double y = coordinates[1];
        double z = coordinates[2];

        int32_t X = int32_t((x - outputAttributes.posOffset.x) / scale.x);
        int32_t Y = int32_t((y - outputAttributes.posOffset.y) / scale.y);
        int32_t Z = int32_t((z - outputAttributes.posOffset.z) / scale.z);

        memcpy(data + offset + 0, &X, 4);
        memcpy(data + offset + 4, &Y, 4);
        memcpy(data + offset + 8, &Z, 4);
    }
}


laszip_close_reader(laszip_reader);
				laszip_destroy(laszip_reader);
