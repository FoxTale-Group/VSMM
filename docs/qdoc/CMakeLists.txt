set(QDOC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/html)

# module header for qdoc, without one it warns
file(GLOB_RECURSE QDOC_HEADERS CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.hpp)
list(FILTER QDOC_HEADERS EXCLUDE REGEX "/src/(tests|qml)/")
list(TRANSFORM QDOC_HEADERS REPLACE "(.+)" "#include \"\\1\"")
list(JOIN QDOC_HEADERS "\n" QDOC_MODULE_HEADER)
file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/VSMMDocs.hpp CONTENT "${QDOC_MODULE_HEADER}\n")

add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${QDOC_OUTPUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E env
            QT_INSTALL_DOCS=${QT6_INSTALL_PREFIX}/${QT6_INSTALL_DOCS}
            QT_VER=${Qt6_VERSION_MAJOR}.${Qt6_VERSION_MINOR}
            QT_VERSION=${Qt6_VERSION}
            VSMM_VERSION=${VSMM_RELEASE_VERSION}
            $<TARGET_FILE:Qt6::qdoc>
            --outputdir ${QDOC_OUTPUT_DIR}
            -I${CMAKE_CURRENT_BINARY_DIR}/include
            "-I$<JOIN:$<TARGET_PROPERTY:VSMM,INCLUDE_DIRECTORIES>,;-I>"
            ${CMAKE_CURRENT_SOURCE_DIR}/vsmm.qdocconf
        COMMAND_EXPAND_LISTS
        VERBATIM
        COMMENT "Generating qdoc html in ${QDOC_OUTPUT_DIR}")
