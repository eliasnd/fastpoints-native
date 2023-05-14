using namespace std;

typedef void (*ProgressCallback)(float value);
typedef bool (*CancelCallback)();

namespace potree_converter {
    // void decimate(string source, int count);
    void convert(string source, string outdir, string method = "poisson", string encoding = "DEFAULT", string chunkMethod = "LASZIP", ProgressCallback progressCallback = NULL, CancelCallback shouldCancel = NULL);
}