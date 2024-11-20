cd ..
mkdir build
cd build
cmake -DSTATIC=OFF ..
cmake build .

cd ../_ubuntu_docker
cp ../build/simple_file_reader .

cp ../f1.txt .
cp ../f2.txt .
cp ../redundant_file.tmp .

docker build -t original_ubuntu_simple_file_reader .