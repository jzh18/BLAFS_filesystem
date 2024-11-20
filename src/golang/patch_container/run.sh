project_path=/home/ubuntu/repos/cached_container
echo "--------------build an original container--------------"
cd ${project_path}/simple_file_reader/_ubuntu_docker
sh build.sh

echo "--------------build cached_fs executable---------------"
cd ${project_path}
mkdir build
cd build
cmake ../src/
cmake --build .


echo "--------------build patch_container executable----------"
cd ${project_path}/src/golang/patch_container
go build -o build/patch_container patch_container.go

echo "--------mount debloated_fs to debloated container----------"
echo "********************************************"
echo "open another terminal and run container with docker run -it --rm original_ubuntu_simple_file_reader in 10 seconds"
echo "After deloating, terminate the following process."
echo "run sudo systemctl restart docke && docker images to check if the image size is reduced"
echo "Then run sudo ./build/patch_container -cached -debloated -name=original_ubuntu_simple_file_reader"
echo "********************************************"
sudo ./build/patch_container -debloated -name=original_ubuntu_simple_file_reader


