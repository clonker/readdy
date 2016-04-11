SET(SOURCES_DIR "${READDY_GLOBAL_DIR}/readdy2/main")

# includes
SET(READDY_INCLUDE_DIRS "${READDY_GLOBAL_INCLUDE_DIR};${IO_INCLUDE_DIRS};${PLUGIN_INCLUDE_DIRS}" CACHE INTERNAL "Readdy include dirs" FORCE)
LIST(REMOVE_DUPLICATES READDY_INCLUDE_DIRS)

# libraries
SET(READDY_DEPENDENT_LIBRARIES "${READDY_COMMON_LIBRARIES};${READDY_IO_LIBRARIES};${READDY_PLUGIN_LIBRARIES}")
LIST(REMOVE_DUPLICATES READDY_DEPENDENT_LIBRARIES)

# sources
LIST(APPEND READDY_MAIN_SOURCES "${SOURCES_DIR}/Simulation.cpp")

# all sources
LIST(APPEND READDY_ALL_SOURCES ${READDY_MAIN_SOURCES})