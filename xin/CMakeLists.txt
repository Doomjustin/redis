add_subdirectory(base)
add_subdirectory(redis)


if(${PROJECT_NAME}_BUILD_TESTS)
    add_executable(xin.test unit_test.cpp)

    target_link_libraries(xin.test
        PRIVATE
            $<LINK_LIBRARY:WHOLE_ARCHIVE,xin::base>
            doctest::doctest
    )

    target_compile_features(xin.test PRIVATE cxx_std_23)
    add_test(NAME xin.test COMMAND xin.test)
endif()
