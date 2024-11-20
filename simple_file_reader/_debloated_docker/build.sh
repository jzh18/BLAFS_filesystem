cd ..
mkdir build
cd build
cmake ..
make

cd ../_debloated_docker
cp ../build/simple_file_reader .

docker build -t debloated_simple_file_reader .