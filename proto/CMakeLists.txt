find_package(Protobuf REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})

set(MESSAGE_SOURCES
  test.proto
)

protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${MESSAGE_SOURCES})

add_library(proto ${PROTO_HEADER} ${PROTO_SRC})