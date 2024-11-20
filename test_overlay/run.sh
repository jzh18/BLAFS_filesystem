fail_any=0

# Firstly, let's mount a remote dir
# the remote dir contains one file
sudo sshfs -o allow_other,IdentityFile=/home/ubuntu/.ssh/aws-stockholm.pem  ubuntu@ec2-13-48-215-98.eu-north-1.compute.amazonaws.com:/home/ubuntu/mounted  mnt


# merged
# upper
# lower
# work dir is needed by overlayFS internally.
mkdir upper work merged

# the second last parameter is device name, for overlayFS, it iss fixed overlay
# the remote dir is used as the lower dir
sudo mount \
    -t overlay \
    -o lowerdir=/home/ubuntu/mnt,upperdir=upper,workdir=work \
    overlay \
    merged



file_num=`ls merged | wc -l`
if [ "$file_num" != "1" ]
then
    echo "should be 1 file at the merged dir"
    fail_any=1
fi

sudo umount merged
rm -rf merged
rm -rf upper
rm -rf work

if [ "$fail_any" = 0 ]; then
   echo '***' PASSED ALL TESTS
else
   echo '***' FAILED SOME TESTS
   exit 1
fi

sudo mount \
    -t overlay \
    -o lowerdir=/var/lib/docker/overlay2/l/N4D4KS7VCBAZ3HCKIQAHWSC3ED:/var/lib/docker/overlay2/l/link_debloated_9c75d5528a30ed755c9c9f1967882e43243b658b92adc38e175a99846ee25e6b:/var/lib/docker/overlay2/l/WL4M26N4NSCBQ22FG6ZGTEWSX2:/var/lib/docker/overlay2/l/7VH7KMOYWS5FC5V7JEXJBBGNTY:/var/lib/docker/overlay2/l/link_cached_9c75d5528a30ed755c9c9f1967882e43243b658b92adc38e175a99846ee25e6b,upperdir=/var/lib/docker/overlay2/d6a2b47ff82b8850c74a9e3d5d84d446f8869b1fa545e81f23cb6748f1685a58/diff,workdir=/var/lib/docker/overlay2/d6a2b47ff82b8850c74a9e3d5d84d446f8869b1fa545e81f23cb6748f1685a58/work \
    overlay \
    /var/lib/docker/overlay2/d6a2b47ff82b8850c74a9e3d5d84d446f8869b1fa545e81f23cb6748f1685a58/merged




overlay on /var/lib/docker/overlay2/d6a2b47ff82b8850c74a9e3d5d84d446f8869b1fa545e81f23cb6748f1685a58/merged type overlay (rw,relatime,lowerdir=/var/lib/docker/overlay2/l/N4D4KS7VCBAZ3HCKIQAHWSC3ED:/var/lib/docker/overlay2/l/link_debloated_9c75d5528a30ed755c9c9f1967882e43243b658b92adc38e175a99846ee25e6b:/var/lib/docker/overlay2/l/WL4M26N4NSCBQ22FG6ZGTEWSX2:/var/lib/docker/overlay2/l/7VH7KMOYWS5FC5V7JEXJBBGNTY:/var/lib/docker/overlay2/l/link_cached_9c75d5528a30ed755c9c9f1967882e43243b658b92adc38e175a99846ee25e6b,upperdir=/var/lib/docker/overlay2/d6a2b47ff82b8850c74a9e3d5d84d446f8869b1fa545e81f23cb6748f1685a58/diff,workdir=/var/lib/docker/overlay2/d6a2b47ff82b8850c74a9e3d5d84d446f8869b1fa545e81f23cb6748f1685a58/work)