file(GLOB_RECURSE srcs src/*.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D _BOOL=1")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
			)
