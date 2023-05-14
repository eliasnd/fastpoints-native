# FastPoints (Native Plug-in)

This is the native side of the FastPoints Unity point cloud renderer. For information about the program, please visit the main repository [here](https://github.com/eliasnd/FastPoints).

## Installation

1. Download source code
2. Install CMake 3.25 or later
3. Create and jump into folder "build"

    ```
    mkdir build
    cd build
    ```
4. Run CMake
    ```
    cmake ../
    ```
5. Run make
    ```
    make
    ```
6. Copy the generated `fastpoints-native.bundle` file into the Plug-Ins folder of your Unity project
    ```
    cp fastpoints-native.bundle /path/to/project/Assets/Plug-Ins/fastpoints-native.bundle
    ```


## Contributing

Pull requests are welcome. For major changes, please open an issue first
to discuss what you would like to change.

Please make sure to update tests as appropriate.