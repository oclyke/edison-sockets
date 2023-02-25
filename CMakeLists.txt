# https://cliutils.gitlab.io/modern-cmake
cmake_minimum_required(VERSION 3.21...3.24)
project(
  edison-sockets
  VERSION 0.0
  LANGUAGES C
)

# set C standard
set(CMAKE_C_STANDARD 17)

# the main executbales
add_executable(client ${CMAKE_CURRENT_LIST_DIR}/src/client.c)
add_executable(server ${CMAKE_CURRENT_LIST_DIR}/src/server.c)
