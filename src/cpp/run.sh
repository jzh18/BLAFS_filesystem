project_path=/home/ubuntu/repos/cached_container

cd $project_path
mkdir build
cd build
cmake $project_path/src/cpp
cmake --build .
ctest
# check memory leaking
valgrind --leak-check=full ./file_meta_test