add_executable(cadmium-tests
    main.cpp
    engine/test_engine.cpp
    engine/test_layer_stack.cpp
)

target_link_libraries(cadmium-tests PRIVATE
    cadmium-engine
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(cadmium-tests)
