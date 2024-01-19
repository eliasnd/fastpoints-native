typedef void (*ProgressCallback)(float value);
typedef bool (*CancelCallback)();

namespace potree_converter {
    // void decimate(string source, int count);
    void convert(std::string source, std::string outdir, std::string method = "poisson", std::string encoding = "DEFAULT", std::string chunkMethod = "LASZIP", ProgressCallback progressCallback = NULL, CancelCallback shouldCancel = NULL);
}