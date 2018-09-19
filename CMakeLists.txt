cmake_minimum_required(VERSION 3.0)
project(zadanie_2 CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

# Czy chcemy oglądać komunikatów o procentowym postępie kompilacji?
set_property(GLOBAL PROPERTY RULE_udp_msgS ON)

# Czy chcemy zobaczyć polecenia wywołane przez make?
set(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wextra -O2 -std=c++17")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -std=c++17 -g -DDEBUG")

add_library(debug_lib debug.cc)
target_link_libraries(debug_lib ${CMAKE_THREAD_LIBS_INIT})

add_library(err err.h)
set_target_properties(err PROPERTIES LINKER_LANGUAGE CXX)

add_library(tcp_socket tcp_socket.cc)
target_link_libraries(tcp_socket
        debug_lib)

add_library(telnet telnet.cc)
target_link_libraries(telnet
        debug_lib)

add_library(console_codes console_codes.cc)
target_link_libraries(console_codes
        debug_lib)

add_library(menu menu.cc)
target_link_libraries(menu
        debug_lib
        console_codes
        telnet)

add_library(program_args program_args.h)
target_link_libraries(program_args
        debug_lib
        err)

add_library(host_and_net host_and_net.cc)
target_link_libraries(host_and_net
        debug_lib
        err)

add_library(smart_thread smart_thread.cc)
target_link_libraries(smart_thread
        debug_lib
        ${CMAKE_THREAD_LIBS_INIT})

add_library(time_alarm time_alarm.h)
target_link_libraries(time_alarm
        debug_lib
        smart_thread
        ${CMAKE_THREAD_LIBS_INIT})

add_library(udp_sock_and_msg udp_sock_and_msg.cc)
target_link_libraries(udp_sock_and_msg
        debug_lib
        err
        host_and_net
        ${CMAKE_THREAD_LIBS_INIT})

add_library(udp_msg_receiver udp_msg_receiver.cc)
target_link_libraries(udp_msg_receiver
        udp_sock_and_msg
        debug_lib
        err
        host_and_net
        ${CMAKE_THREAD_LIBS_INIT})

add_library(udp_msg_sender udp_msg_sender.h)
set_target_properties(udp_msg_sender PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(udp_msg_sender
        udp_sock_and_msg
        debug_lib
        err
        host_and_net
        ${CMAKE_THREAD_LIBS_INIT})

add_library(datagrams datagrams.cc)
target_link_libraries(datagrams
        debug_lib
        host_and_net
        udp_sock_and_msg)

add_library(fifo fifo.h)
set_target_properties(fifo PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(fifo
        debug_lib
        ${CMAKE_THREAD_LIBS_INIT})

add_library(radio_channel radio_channel.h)
set_target_properties(radio_channel PROPERTIES LINKER_LANGUAGE CXX)

add_library(sikreceiver_shared_resource sikreceiver_shared_resource.h)
set_target_properties(sikreceiver_shared_resource PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(sikreceiver_shared_resource
        debug_lib
        smart_thread
        tcp_socket
        fifo
        audio_msg
        radio_channel
        menu
        ${CMAKE_THREAD_LIBS_INIT})

add_executable(sikradio-receiver sikradio-receiver.cc
        sikreceiver_main_thread.h sikreceiver_lookup_sender.h
        sikreceiver_replies_receiver.h
        sikreceiver_audio_receiver.h sikreceiver_audio_player.h
        sikreceiver_rexmit_requester.h time_util.h
        sikreceiver_tcp_manager.h sikreceiver_tcp_manager.cc
        sikreceiver_tcp_new_connections_listener.h sikreceiver_tcp_new_connections_listener.cc
        sikreceiver_tcp_client_handler.h sikreceiver_tcp_client_handler.cc
        sikreceiver_tcp_codes_receiver.h)
target_link_libraries(sikradio-receiver
        err
        debug_lib
        program_args
        tcp_socket
        telnet
        menu
        console_codes
        udp_sock_and_msg
        udp_msg_receiver
        udp_msg_sender
        datagrams
        fifo
        host_and_net
        smart_thread
        ${CMAKE_THREAD_LIBS_INIT})

add_executable(sikradio-sender sikradio-sender.cc siksender_main_thread.h
        siksender_shared_resource.h siksender_control_port_service.h
        siksender_audio_mainsender.h sikreceiver_audio_player.h siksender_rexmiter.h
        time_util.h)
target_link_libraries(sikradio-sender
        err
        debug_lib
        program_args
        udp_sock_and_msg
        udp_msg_receiver
        udp_msg_sender
        datagrams
        fifo
        host_and_net
        smart_thread
        ${CMAKE_THREAD_LIBS_INIT})