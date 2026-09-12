# Select dependencies based on ESP-IDF version
if(${IDF_VERSION_MAJOR} STREQUAL 5 AND ${IDF_VERSION_MINOR} LESS 3)
    # Use driver component as esp_driver_gpio is not available before 5.3
    set(req driver esp_timer)
else()
    set(req esp_driver_gpio esp_timer)
endif()

idf_component_register(
    SRCS
        "${CMAKE_CURRENT_LIST_DIR}/../../src/core/cs123x.cpp"
    INCLUDE_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/../../src/core"
        "${CMAKE_CURRENT_LIST_DIR}/../../src/hal"
    REQUIRES
        ${req}
)