#!/bin/bash

# Make the build directory if it doesn't already exist
mkdir -p build

# Defaults to making a debug build
build_type="Debug"

# Parse the options passed by the user
while getopts ":r" opt; do
    case "${opt}" in
        r) 
            build_type="Release"
            ;;
        \?) echo "ERROR: Invalid option: -$OPTARG"
            exit 1
            ;;
    esac
done

# Verify that there aren't any invalid arguments remaining
shift $((OPTIND - 1))

if [[ $# -gt 0 ]]; then
    echo "Error: Unexpected argument '$1'"
    exit 1
fi


# Let the user know what type of build is occuring
if [[ $build_type == "Debug" ]]; then
    echo -e "INFO: Making a debug build!\n"
elif [[ $build_type == "Release" ]]; then
    echo -e "INFO: Making a release build!\n"
else
    echo "ERROR: Invalid build type - $build_type"
    exit 1
fi


# Build the program
cmake -DCMAKE_BUILD_TYPE=$build_type -B ./build
cmake --build ./build/ --config $build_type

# Update .ts file
cmake --build ./build/ --target update_translations

chmod +x ./build/shax-desktop-client