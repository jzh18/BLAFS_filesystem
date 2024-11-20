cmake_minimum_required(VERSION 3.14)
project(cached_fs)

set(CMAKE_CXX_FLAGS "-fcompare-debug-second")
set(CMAKE_CXX_STANDARD 14)

set(FUSE_INCLUDE /home/ubuntu/miniconda3/envs/cached_container/include/fuse3)
set(FUSE_LIB /home/ubuntu/miniconda3/envs/cached_container/lib/)

set(SPDLOG_INCLUDE /home/ubuntu/miniconda3/envs/cached_container/include/)
set(SPDLOG_LIB /home/ubuntu/miniconda3/envs/cached_container/lib)

set(CURLPP_INCLUDE /home/ubuntu/repos/curlpp-0.8.1/include)
set(CURLPP_LIB /home/ubuntu/repos/curlpp-0.8.1/build)

set(SSH_INCLUDE /home/ubuntu/repos/libssh2/include)
set(SSH_LIB /home/ubuntu/repos/libssh2/build/src)


include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


INCLUDE_DIRECTORIES(${FUSE_INCLUDE} ${SPDLOG_INCLUDE} ${CURLPP_INCLUDE} ${SSH_INCLUDE})
LINK_DIRECTORIES(${FUSE_LIB} ${SPDLOG_LIB} ${CURLPP_LIB} ${SSH_LIB})
add_executable(cached_fs cached_fs.cc sftp_client.h sftp_client.cc file_registry.h file_registry.cc file_meta.h file_meta.cc)
target_link_libraries(cached_fs fuse3 spdlog fmt curlpp curl ssh2 crypto boost_filesystem)


add_executable(debloated_fs debloated_fs.cc sftp_client.h sftp_client.cc file_registry.h file_registry.cc file_meta.h file_meta.cc)
target_link_libraries(debloated_fs fuse3 spdlog fmt curlpp curl ssh2 crypto boost_filesystem)


enable_testing()

# add_executable(
#   hello_test
#   hello_test.cc
# )
# target_link_libraries(
#   hello_test
#   GTest::gtest_main
# )


add_executable(
  file_meta_test
  file_meta_test.cc
  file_registry_test.cc
  file_meta.h
  file_meta.cc
  file_registry.h
  file_registry.cc

  
  hello_test.cc
)
target_link_libraries(
file_meta_test
  GTest::gtest_main
  spdlog
  fmt
)

include(GoogleTest)
gtest_discover_tests(file_meta_test)
