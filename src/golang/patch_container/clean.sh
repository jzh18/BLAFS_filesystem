sudo ./build/patch_container -umount -name=original_ubuntu_simple_file_reade
docker rmi original_ubuntu_simple_file_reader
docker system prune -a -f
sudo rm  /var/lib/docker/overlay2/ -r
sudo mkdir /var/lib/docker/overlay2/
sudo mkdir /var/lib/docker/overlay2/l
