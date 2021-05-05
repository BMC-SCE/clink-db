add_executable(clink-db
  src/build.c
  src/colour.c
  src/dirname.c
  src/disppath.c
  src/file_queue.c
  src/help.c
  src/helpx.c
  src/is_root.c
  src/join.c
  src/line_ui.c
  src/main.c
  src/mtime.c
  src/ncurses_ui.c
  src/option.c
  src/path.c
  src/set.c
  src/sigint.c
  src/str_queue.c
  src/work_queue.c
  ${CMAKE_CURRENT_BINARY_DIR}/manpage.c
  ${CMAKE_CURRENT_BINARY_DIR}/CX_CursorKind.c)

target_include_directories(clink-db PRIVATE src)

target_link_libraries(clink-db libclink)

find_package(Threads REQUIRED)
target_link_libraries(clink-db ${CMAKE_THREAD_LIBS_INIT})

set(CURSES_NEED_NCURSES True)
find_package(Curses REQUIRED)
target_include_directories(clink-db PUBLIC ${CURSES_INCLUDE_DIR})
target_link_libraries(clink-db ${CURSES_LIBRARIES})

find_program(XXD xxd)

add_custom_command(OUTPUT manpage.c
  COMMAND ${XXD} -include clink-db.1 ${CMAKE_CURRENT_BINARY_DIR}/manpage.c
  MAIN_DEPENDENCY src/clink-db.1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_custom_command(OUTPUT CX_CursorKind.c
  COMMAND ${XXD} -include CX_CursorKind.1 ${CMAKE_CURRENT_BINARY_DIR}/CX_CursorKind.c
  MAIN_DEPENDENCY src/CX_CursorKind.1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_custom_target(man-clink-db
  ALL
  DEPENDS clink-db.1.gz)
add_custom_command(OUTPUT clink-db.1.gz
  COMMAND gzip -9 --no-name --to-stdout src/clink-db.1 >"${CMAKE_CURRENT_BINARY_DIR}/clink-db.1.gz"
  MAIN_DEPENDENCY src/clink-db.1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS clink-db
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/clink-db.1.gz
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
