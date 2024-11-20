mount cached fs layer: 
```
./cached_fs -s -d  --realdir=/tmp/real3 --fileregistry=file_registry.txt /tmp/mnt
```

mount debloated fs layer:
```
./debloated_fs -s -d  --realdir=/tmp/real5 --lowerdir=/tmp/lower5  /tmp/mnt5
```


## Test debloated_fs layer
1. find the cache-id and change it to the debloated_fs: https://www.baeldung.com/linux/docker-image-storage-host
2. restart docker 
```
sudo systemctl daemon-reload
sudo systemctl restart docker
```
1. mount the image top layer to an overlay fs:
```
sudo mount \
    -t overlay \
    -o lowerdir=/var/lib/docker/overlay2/l/7VH7KMOYWS5FC5V7JEXJBBGNTY:/var/lib/docker/overlay2/l/link_cached_9c75d5528a30ed755c9c9f1967882e43243b658b92adc38e175a99846ee25e6b,upperdir=/var/lib/docker/overlay2/pa0gypfkk5nhg8q6q5f3uoube/diff,workdir=/var/lib/docker/overlay2/pa0gypfkk5nhg8q6q5f3uoube/work \
    overlay \
    /var/lib/docker/overlay2/pa0gypfkk5nhg8q6q5f3uoube/merged
```
2. mount the debloated_fs layer to a top image layer:
```
/usr/bin/sudo /home/ubuntu/repos/cached_container/build/debloated_fs -s -d --realdir=/var/lib/docker/overlay2/debloated_9c75d5528a30ed755c9c9f1967882e43243b658b92adc38e175a99846ee25e6b/real --lowerdir=/var/lib/docker/overlay2/pa0gypfkk5nhg8q6q5f3uoube/merged /var/lib/docker/overlay2/debloated_9c75d5528a30ed755c9c9f1967882e43243b658b92adc38e175a99846ee25e6b/diff
```
3. start a container from the image:
```
docker run  -it  --rm  debloated_simple_file_reader
```

4. Expected:
	* the container should work
	* debloated_fs should receive callback

It seem like docker-run relies on ioctl_callback. If we remove the callback, the container wouldn't work. 
https://github.com/libfuse/libfuse/blob/master/example/ioctl.c

https://docs.kernel.org/driver-api/ioctl.html

TODO:
1. Check what is ioctl
2. Update patch_container to make the whole workflow works.