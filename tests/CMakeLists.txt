add_executable(cadmium-tests
    main.cpp
    engine/test_timer.cpp
    engine/test_engine.cpp
)

target_link_libraries(cadmium-tests PRIVATE
    cadmium-engine
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(cadmium-tests)
