mkdir build
cd build
cmake ..
make

mkdir /tmp/example
rm /tmp/example/*
./fuse_example -s /tmp/example 
file_num=`ls /tmp/example | wc -l`
fail_any=0

if [ "$file_num" != "1" ]
then
	echo "There should be one file."
	fail_any=1

fi

umount /tmp/example

if [ "$fail_any" = 0 ]; then
   echo '***' PASSED ALL TESTS
else
   echo '***' FAILED SOME TESTS
   exit 1
fi