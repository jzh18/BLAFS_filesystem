project_path=/home/ubuntu/repos/cached_container

echo "---------------------build original_simple_file_reader-----------------"
cd ${project_path}/simple_file_reader/_docker/
sh build.sh


echo "-----------build sfpt servr based on original_simple_file_reader--------"
cd ${project_path}/src/golang/sftp_fileserver/
mkdir build
go build -o build/sftp_server main.go

cd ${project_path}/src/golang/sftp_fileserver/_docker/
cp ../build/sftp_server .
cp ../id_rsa .
cp ../id_rsa.pub .
docker build -t sftp_server .