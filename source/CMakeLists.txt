
message(STATUS "ROOT_DIR = ${ROOT_DIR}")


add_library(
   adam.util
   INTERFACE
)
add_library(adam::util ALIAS adam.util)

target_sources( adam.util
   PUBLIC
      FILE_SET HEADERS
      BASE_DIRS ${ROOT_DIR}/include/util/
      FILES
        ${ROOT_DIR}/include/util/array2d.hpp
        ${ROOT_DIR}/include/util/baseconv.hpp
        ${ROOT_DIR}/include/util/randomutil.hpp
        ${ROOT_DIR}/include/util/soa.hpp
        ${ROOT_DIR}/include/util/soamemorylayout.hpp
        ${ROOT_DIR}/include/util/stackhelpers.hpp
)


add_library( adam.geom
   STATIC
      geom/rowcol.cpp
)
add_library(adam::geom ALIAS adam.geom)

target_sources( adam.geom
   PUBLIC
   FILE_SET HEADERS
   BASE_DIRS ${ROOT_DIR}/include/geom/
   FILES
      ${ROOT_DIR}/include/geom/aabb.hpp
      ${ROOT_DIR}/include/geom/collisions.hpp
      ${ROOT_DIR}/include/geom/onedrange.hpp
      ${ROOT_DIR}/include/geom/rect.hpp
      ${ROOT_DIR}/include/geom/rowcol.hpp
      ${ROOT_INCLUDE}/geom/relative_dimension.hpp
      ${ROOT_INCLUDE}/geom/typed_position.hpp
)


add_library( adam.terminal
   STATIC
      terminal/parsers.cpp
      terminal/term_control.cpp
      terminal/terminfo.cpp
      terminal/parsers.cpp
)

add_library( adam::terminal ALIAS adam.terminal)

target_sources( adam.terminal
   PUBLIC
      FILE_SET HEADERS
      BASE_DIRS ${ROOT_DIR}/include/terminal/
      FILES
         ${ROOT_DIR}/include/terminal/color.hpp
         ${ROOT_DIR}/include/terminal/pixel.hpp
         ${ROOT_DIR}/include/terminal/scanline_stack_render.hpp
         ${ROOT_DIR}/include/terminal/compositor.hpp
         ${ROOT_DIR}/include/terminal/image.hpp
         ${ROOT_DIR}/include/terminal/render.hpp
         ${ROOT_DIR}/include/terminal/scanline_stack_row.hpp
         ${ROOT_DIR}/include/terminal/compositor_shared.hpp
         ${ROOT_DIR}/include/terminal/parsers.hpp
         ${ROOT_DIR}/include/terminal/scanlinerender_painter.hpp
         ${ROOT_DIR}/include/terminal/term_control.hpp
         ${ROOT_INCLUDE}/terminal/types.hpp
         ${ROOT_INCLUDE}/terminal/terminfo.hpp
   )

# target_include_directories(
#    terminal
#    PUBLIC
#    "${ROOT_DIR}/include/terminal"
# )

target_link_libraries( adam.terminal
   PUBLIC
   adam.util
   adam.geom
)





add_executable(
   CppEditor

   main.cpp
)

target_link_libraries(CppEditor
   PUBLIC
   adam::terminal
   adam::util

)






