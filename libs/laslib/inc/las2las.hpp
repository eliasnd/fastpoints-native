#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "lasreader.hpp"
#include "laswriter.hpp"
#include "lastransform.hpp"
#include "geoprojectionconverter.hpp"
#include "bytestreamout_file.hpp"
#include "bytestreamin_file.hpp"

int las2las(int argc, char *argv[]);