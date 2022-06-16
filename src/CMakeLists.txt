include_directories(.)
include_directories(process)
#include_directories(thread)

AUX_SOURCE_DIRECTORY(. SOURCE)
AUX_SOURCE_DIRECTORY(process SOURCE)
#AUX_SOURCE_DIRECTORY(thread SOURCE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

#为源文件的编译添加由-D定义的标志
option(DEBUG "DEBUG MODE" OFF)
if(DEBUG)
    message("DEBUG is defined")
    add_definitions(-DDEBUG)
endif(DEBUG)


add_executable(dict_test ${SOURCE})
target_link_libraries(dict_test rt)

add_executable(test_hash process/unitest/test_hash.cpp process/hashtable.cpp process/shmem.cpp)
target_link_libraries(test_hash rt)

add_executable(test_hash_read process/unitest/test_hash_read.cpp process/hashtable.cpp process/shmem.cpp)
target_link_libraries(test_hash_read rt)


