set(source_files
    model/dash-client.cc
    model/http-parser.cc
    model/mpeg-player.cc
    model/dash-server.cc
    model/http-header.cc
    model/mpeg-header.cc
    model/algorithms/osmp-client.cc
    model/algorithms/svaa-client.cc
    model/algorithms/aaash-client.cc
    model/algorithms/fdash-client.cc
    model/algorithms/raahs-client.cc
    model/algorithms/sftm-client.cc
    helper/dash-client-helper.cc
    helper/dash-server-helper.cc
)

set(header_files
    model/dash-client.h
    model/http-parser.h
    model/mpeg-player.h
    model/dash-server.h
    model/http-header.h
    model/mpeg-header.h
    model/algorithms/osmp-client.h
    model/algorithms/svaa-client.h
    model/algorithms/aaash-client.h
    model/algorithms/fdash-client.h
    model/algorithms/raahs-client.h
    model/algorithms/sftm-client.h
    helper/dash-client-helper.h
    helper/dash-server-helper.h
)

set(test_sources
    test/dash-test-suite.cc
)

build_lib(
  LIBNAME dash
  SOURCE_FILES ${source_files}
  HEADER_FILES ${header_files}
  LIBRARIES_TO_LINK
    ${liblte}
    ${libinternet-apps}
  TEST_SOURCES ${test_sources}
)
