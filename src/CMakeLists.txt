cmake_minimum_required (VERSION 3.8)

find_package (Threads)

add_executable (ncc 
    "ack_maker_and_serialiser.cpp" 
    "config.cpp" 
    "epoll_controller.cpp" 
    "epoll_server.cpp" 
    "logging.cpp" 
    "main.cpp" 
    "session.cpp" 
    "socket_utils.cpp" 
    "tcp_client.cpp"
    "ack_maker_and_serialiser.h" 
    "application_messages.h" 
    "config.h" 
    "epoll_controller.h" 
    "epoll_server.h" 
    "length_prefixed_stream_deserialiser.h" 
    "logging.h" 
    "serialiser.h" 
    "session.h" 
    "socket_reader.h" 
    "socket_utils.h" 
    "socket_writer.h" 
    "tcp_client.h"
    "console_input_loop.h" 
    "io_benchmark.h" 
    "io_benchmark.cpp"
)

target_link_libraries (ncc Threads::Threads)
