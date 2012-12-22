FIND_PATH(EV_INCLUDE_DIR ev.h
  PATH_SUFFIXES include
  PATHS
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
  HINTS $ENV{EV_SRC}
)

FIND_LIBRARY(EV_LIBRARY
  NAMES ev
  HINTS $ENV{EV_SRC}/.libs/
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EV DEFAULT_MSG EV_LIBRARY EV_INCLUDE_DIR)
MARK_AS_ADVANCED(EV_LIBRARY EV_INCLUDE_DIR)