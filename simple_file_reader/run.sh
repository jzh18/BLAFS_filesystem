echo "-----------------------build excutable----------------"
mkdir build
cd build
cmake ..
make

echo "-----------------------build original container----------------"
cd ../_docker
sh build.sh

echo "-----------------------run original container-------------------"
docker run -it --rm original_simple_file_reader f1.txt
docker run -it --rm original_simple_file_reader f2.txt

echo "-----------------------build debloated container----------------"
cd ../_debloated_docker
sh build.sh


echo "-----------------------run debloated container-------------------"
# expect f1.txt and f2.txt to be not found 
docker run -it --rm debloated_simple_file_reader f1.txt
docker run -it --rm debloated_simple_file_reader f2.txt

echo "-----------------------create layer manually----------------------"
base_overlay_dir=/var/lib/docker/overlay2/
# create a layer manually
sudo mkdir ${base_overlay_dir}manually_created_layer
manual_diff=${base_overlay_dir}manually_created_layer/diff/
sudo mkdir  $manual_diff ${base_overlay_dir}manually_created_layer/work
manual_link=${base_overlay_dir}manually_created_layer/link
manual_lower=${base_overlay_dir}manually_created_layer/lower
sudo touch $manual_link
sudo touch $manual_lower
sudo ln -s ../manually_created_layer/diff/ ${base_overlay_dir}l/manually_created_layer_link
sudo sh -c "echo -n l/manually_created_layer_link > $manual_link"

# get the original lowers
upperdir=`docker inspect --format='{{.GraphDriver.Data.UpperDir}}' debloated_simple_file_reader`
len=${#upperdir}
new_len=$((len-4))
upperdir=`echo $upperdir | cut -c 1-$new_len`
original_lowers=`sudo cat ${upperdir}lower`
# set the original lowers to the manual layer
sudo sh -c "echo -n $original_lowers > $manual_lower"
# insert the manul layer to the original lowers
sudo sh -c "echo -n l/manually_created_layer_link:$original_lowers > ${upperdir}lower"

# mount a remote dir to the manual layer
sudo sshfs -o allow_other,IdentityFile=/home/ubuntu/.ssh/aws-stockholm.pem  ubuntu@ec2-13-48-215-98.eu-north-1.compute.amazonaws.com:/home/ubuntu/mounted  $manual_diff

echo "-----------------------run debloated container again----------------------"
# expect f1.txt and f2.txt to be found 
docker run -it --rm debloated_simple_file_reader f1.txt
docker run -it --rm debloated_simple_file_reader f2.txt

echo "-----------------------clean environment----------------------"
sudo umount $manual_diff
sudo rm -r /var/lib/docker/overlay2/manually_created_layer/
sudo rm -r /var/lib/docker/overlay2/l/manually_created_layer_link
docker rmi original_simple_file_reader
docker rmi debloated_simple_file_reader
docker system prune -a -f