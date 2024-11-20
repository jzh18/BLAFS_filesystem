# Cached Container
Created: 2023 Feb. 3rd.

docker overlay2:
https://www.terriblecode.com/blog/how-docker-images-work-union-file-systems-for-dummies/

https://linuxconfig.org/introduction-to-the-overlayfs


# Hook Linux Sys Calls
1. Develop an LKM
	1. Find the sys call you want to hook
	2. Make the `sys_call_table` writeable
	3. Write your own function to replace the sys call.
2. Load the LKM

Here are some useful docs for developing an LKM and hook sys calls:

1. Develop an LKM: https://www.thegeekstuff.com/2013/07/write-linux-kernel-module/
2. Hook a sys call: https://exploit.ph/linux-kernel-hacking/2014/07/10/system-call-hooking/index.html
3. Different ways to hook sys calls: https://nixhacker.com/hooking-syscalls-in-linux-using-ftrace/
4. LKM in depth: http://www.ouah.org/LKM_HACKING.html#II.1.

Pros:

1. Powerful

Cons:

1. Not sure how to load an LKM into a container, probably not supported or need to be loaded in host
2. Different kernel versions have different way to make `sys_call_table` writeable. It's hard to find a unified way.

# Container OverlayFS
Use container to represent the cache, i.e. a main cotnaienr paired with a couple of dependent containers. The 
dependent containers are responsible for caching.

Insert a new layer to the original layer. The new layer is mounted to a remote dir.

1. Docker storage: https://docs.docker.com/storage/storagedriver/

* Preload files inadvance
	* predict what file gonna be used based on the current file access pattern and package dependency graph. preload.
	* cached fs should served as the lower layer.
	* We don't need to based on SFTP/sshfs. What we need is to download file from remote to local. Then all the modifications/access is handled by overlay system.
	So the remote host needs a file server.
	* Use digital signature to verify the integrity of files. Read the book (Cypher introduciton) to check how to make a system secure.
	* The fileserver might also be able to push files to containers.
	* Serve the debloated files in a container, i.e. given an original container, we produce two new contianers:
	one is debloated container; the other one is remained contianer. Launch the file server in the remained container/original container.
	* Don't pay for the features you're not using.
	* A config file to record what files get debloated. Note that we shouldn't download everything
	the application needs as the file might not exist in the original container.
	* If the application access a directory, we should download the entire directory
	* If mount the cached_fs to the lowest dir, it will affect all the other containers. How to avoid this? Maybe use file_registry?
	* If we want an LRU cache, we need to add another layer to the top of the overlay fs (NO!).
		* NO! Not the top, should add to the above the original lower layer.
		* NO! basically, we don't need to do anything about it. Just iterate all files and check theis last access time, then delete them. That's all.
	* If some program checks the file last accessed time, then the cached_fs will not work.
	* A model learning the trade off: min cache miss + max debloating
	* What about volume? Is it affected?
	* Might implement the fs as a docker plugin? https://docs.docker.com/engine/extend/plugin_api/
	* https://dominikbraun.io/blog/docker/docker-images-and-their-layers-explained/
	* https://medium.com/tenable-techblog/a-peek-into-docker-images-b4d6b2362eb
	* https://www.baeldung.com/linux/docker-image-storage-host
	* Think about JVM garbage collection machenism!分代手机算法， for the debloated layer!
	* layer sharing between different containers. Make sure one container is optimized, multiple containrs are also optimized, in terms of storage.
	* Linux file system: https://developer.ibm.com/tutorials/l-linux-filesystem/
	* The reason why the top layer is more complex to implement is that it's like a funnel. The original layers filter most system calls. So in most cases the bottom layer only need to handle very few system calls. But the top layer needs to handle all the system calls.
	layer 
* How is docker image id generated?
	* `cat /var/lib/docker/image/overlay2/layerdb/sha256/{img_id} | sha256sum` == `{img_id}`
	* The layers of an image are identified by the `RootFs` fields, which can be calculated by: https://www.baeldung.com/linux/docker-image-storage-host
* Good documents for fuse:
	* https://web.archive.org/web/20070503133705/http://www.night.dataphone.se/~jrydberg/fuse.html




# What To Do
What are the advantages of this bloat-aware filesystem?
1. We remove unsed file and save storage overhead
2. Small containers use less time for provisioning
3. The cached layer can be also used by other containe-debloating tools

What are the disadvantages of it?
1. Newly-added layers might affect performance. This could be mitigate in theory.
2. Cache missing makes performance worse
3. Other containers might affect by layer modification, which might increase the overall storage overhead. Add a debloated layer for each layer could mitigate this issue.

total=provisioning time + cacheloading time = a(remained size) + b(removed size)
for each file , we have size and probability to be used.
remained files = {f1, f2, f3}, remained size=s1\*p1+s2\*p2+s3\*p3
removed files = {f4, f5, f6}, removed size=s4\*p4+s5\*p5+s6\*p6

delta provisioning time - cacheloading time >0

how to estimate probability pi?


# 20230306
1. Try with docker-slim
2. Profile the performance
3. How to set the duration
