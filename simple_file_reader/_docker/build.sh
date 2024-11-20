cd ..
mkdir build
cd build
cmake ..
make

cd ../_docker
cp ../build/simple_file_reader .

cp ../f1.txt .
cp ../f2.txt .
cp ../redundant_file.tmp.

docker build -t original_simple_file_reader .