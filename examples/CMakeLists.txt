build_lib_example(
  NAME dash-example
  SOURCE_FILES dash-example.cc
  LIBRARIES_TO_LINK
    ${libdash}
)

build_lib_example(
  NAME dash-wifi
  SOURCE_FILES dash-wifi.cc
  LIBRARIES_TO_LINK
    ${libdash}
    ${libwifi}
)

build_lib_example(
  NAME lena-dash
  SOURCE_FILES lena-dash.cc
  LIBRARIES_TO_LINK
    ${libdash}
)

build_lib_example(
  NAME link-example
  SOURCE_FILES link-example.cc
  LIBRARIES_TO_LINK
    ${libdash}
)
