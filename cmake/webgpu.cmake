add_library(cadmium-webgpu INTERFACE)

if(EMSCRIPTEN)

    message(STATUS "Cadmium: using Emdawnwebgpu")

    target_compile_options(cadmium-webgpu INTERFACE
        --use-port=emdawnwebgpu
    )

    target_link_options(cadmium-webgpu INTERFACE
        --use-port=emdawnwebgpu
        --closure=1
    )

       target_include_directories(cadmium-webgpu INTERFACE
        $ENV{EMSDK}/upstream/emscripten/cache/ports/emdawnwebgpu/emdawnwebgpu_pkg/webgpu/include
        $ENV{EMSDK}/upstream/emscripten/cache/ports/emdawnwebgpu/emdawnwebgpu_pkg/webgpu_cpp/include
    )

    target_compile_definitions(cadmium-webgpu INTERFACE
        CADMIUM_WEBGPU_EMDAWN
    )

else()

    set(DAWN_VERSION "7187" CACHE STRING
        "Dawn release revision (the number after 'chromium/' in the tag name)")
    set(DAWN_SOURCE_MIRROR "https://dawn.googlesource.com/dawn" CACHE STRING
        "Repository to fetch Dawn source from")
    set(WEBGPU_LINK_TYPE "SHARED" CACHE STRING
        "Dawn's source distribution only supports SHARED for now")

    set(DAWN_BUILD_MONOLITHIC_LIBRARY "SHARED" CACHE STRING
    "Build monolithic library: SHARED, STATIC, or OFF")

    set(DAWN_FETCH_DEPENDENCIES ON CACHE BOOL "" FORCE)

    # Force-disable D3D, Metal, and Null backends (Both DAWN_ENABLE_* and DAWN_ENABLE_BACKEND_*)
    set(DAWN_ENABLE_D3D11 OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_D3D12 OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_BACKEND_D3D11 OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_BACKEND_D3D12 OFF CACHE BOOL "" FORCE)
    set(DAWN_USE_WINDOWS_UI OFF CACHE BOOL "" FORCE)

    set(DAWN_ENABLE_METAL OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_BACKEND_METAL OFF CACHE BOOL "" FORCE)

    set(DAWN_ENABLE_NULL OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_BACKEND_NULL OFF CACHE BOOL "" FORCE)

    set(DAWN_ENABLE_DESKTOP_GL OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_BACKEND_DESKTOP_GL OFF CACHE BOOL "" FORCE)

    set(DAWN_ENABLE_OPENGLES OFF CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_BACKEND_OPENGLES OFF CACHE BOOL "" FORCE)

    # Force-enable Vulkan backend
    set(DAWN_ENABLE_VULKAN ON CACHE BOOL "" FORCE)
    set(DAWN_ENABLE_BACKEND_VULKAN ON CACHE BOOL "" FORCE)

    # Tint options
    set(TINT_BUILD_SPV_READER OFF CACHE BOOL "" FORCE)
    set(TINT_BUILD_CMD_TOOLS OFF CACHE BOOL "" FORCE)
    set(TINT_BUILD_IR_BINARY OFF CACHE BOOL "" FORCE)
    set(TINT_BUILD_TESTS OFF CACHE BOOL "Build Tint tests" FORCE)
    set(TINT_BUILD_UNITTESTS OFF CACHE BOOL "Build Tint unit tests" FORCE)

    # Dawn build options
    set(DAWN_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(DAWN_BUILD_TESTS OFF CACHE BOOL "" FORCE)

    set(_cadmium_build_shared_libs_saved ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

    find_package(Python3 REQUIRED)
    FetchContent_Declare(
	dawn
	#GIT_REPOSITORY ${DAWN_SOURCE_MIRROR}
	#GIT_TAG        chromium/${DAWN_VERSION}
	#GIT_SHALLOW ON

	# Manual download mode, even shallower than GIT_SHALLOW ON
	DOWNLOAD_COMMAND
		cd ${FETCHCONTENT_BASE_DIR}/dawn-src &&
		git init &&
		git fetch --depth=1 ${DAWN_SOURCE_MIRROR} chromium/${DAWN_VERSION} &&
		git reset --hard FETCH_HEAD
    )

    FetchContent_MakeAvailable(dawn)

    set(BUILD_SHARED_LIBS ${_cadmium_build_shared_libs_saved} CACHE BOOL "" FORCE)

    set(AllDawnTargets
	core_tables
	dawn_common
	dawn_glfw
	dawn_headers
	dawn_native
	dawn_platform
	dawn_proc
	dawn_wire
	dawn_native_objects
	dawn_shared_utils
	partition_alloc
	dawncpp
	dawncpp_headers
	enum_string_mapping
	extinst_tables
	webgpu_dawn
	webgpu_headers_gen

	tint-format
	tint-lint
	tint_api
	tint_api_common
	tint_cmd_common
	tint_lang_core
	tint_lang_core_common
	tint_lang_core_constant
	tint_lang_core_intrinsic
	tint_lang_core_ir
	tint_lang_core_ir_analysis
	tint_lang_core_ir_transform
	tint_lang_core_ir_transform_common
	tint_lang_core_ir_type
	tint_lang_core_type
	tint_lang_glsl_validate
	tint_lang_hlsl_writer_common
	tint_lang_hlsl_writer_helpers
	tint_lang_hlsl_writer_printer
	tint_lang_hlsl_writer_raise
	tint_lang_msl
	tint_lang_msl_intrinsic
	tint_lang_msl_ir
	tint_lang_spirv
	tint_lang_spirv_intrinsic
	tint_lang_spirv_ir
	tint_lang_spirv_reader_lower
	tint_lang_spirv_type
	tint_lang_spirv_validate
	tint_lang_spirv_writer
	tint_lang_spirv_writer_common
	tint_lang_spirv_writer_helpers
	tint_lang_spirv_writer_printer
	tint_lang_spirv_writer_raise
	tint_lang_wgsl
	tint_lang_wgsl_ast
	tint_lang_wgsl_ast_transform
	tint_lang_wgsl_common
	tint_lang_wgsl_features
	tint_lang_wgsl_helpers
	tint_lang_wgsl_inspector
	tint_lang_wgsl_intrinsic
	tint_lang_wgsl_ir
	tint_lang_wgsl_program
	tint_lang_wgsl_reader
	tint_lang_wgsl_reader_lower
	tint_lang_wgsl_reader_parser
	tint_lang_wgsl_reader_program_to_ir
	tint_lang_wgsl_resolver
	tint_lang_wgsl_sem
	tint_lang_wgsl_writer
	tint_lang_wgsl_writer_ast_printer
	tint_lang_wgsl_writer_ir_to_program
	tint_lang_wgsl_writer_raise
	tint_lang_wgsl_writer_syntax_tree_printer
	tint_utils
	tint_utils_bytes
	tint_utils_command
	tint_utils_containers
	tint_utils_diagnostic
	tint_utils_file
	tint_utils_ice
	tint_utils_macros
	tint_utils_math
	tint_utils_memory
	tint_utils_rtti
	tint_utils_strconv
	tint_utils_symbol
	tint_utils_system
	tint_utils_text
	tint_utils_text_generator
    )

    # patch "use of undeclared identifier" errors
    if(TARGET tint_utils_file)
        target_compile_definitions(tint_utils_file PRIVATE
            _SH_DENYNO=0x40
            _S_IREAD=0x0100
            _S_IWRITE=0x0080
        )
    endif()

    foreach (Target ${AllDawnTargets})
        if (TARGET ${Target})
            get_property(AliasedTarget TARGET "${Target}" PROPERTY ALIASED_TARGET)
            if ("${AliasedTarget}" STREQUAL "")
                set_property(TARGET ${Target} PROPERTY FOLDER "Dawn")

                get_property(TargetType TARGET ${Target} PROPERTY TYPE)
                if (TargetType STREQUAL "INTERFACE_LIBRARY")
                    target_compile_options(${Target} INTERFACE -Wno-c2y-extensions)
                elseif (TargetType MATCHES "^(STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY|EXECUTABLE)$")
                    target_compile_options(${Target} PRIVATE -Wno-c2y-extensions)
                endif()
                # else: UTILITY targets (tint-format, tint-lint, etc.) don't compile
                # anything — nothing to suppress warnings on, skip them.
            endif()
        else()
            message(STATUS "NB: '${Target}' is no longer a target of the Dawn project.")
        endif()
    endforeach()

    # ----- Link to your cadmium-webgpu target -----
    # dawn::webgpu_dawn already carries its own include dirs (interface config).
    target_link_libraries(cadmium-webgpu INTERFACE webgpu_dawn)
    target_compile_definitions(cadmium-webgpu INTERFACE CADMIUM_WEBGPU_DAWN)

    function(target_copy_webgpu_binaries Target)
        add_custom_command(
            TARGET ${Target} POST_BUILD
            COMMAND
                "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_FILE_DIR:webgpu_dawn>/$<TARGET_FILE_NAME:webgpu_dawn>"
                "$<TARGET_FILE_DIR:${Target}>"
        )
    endfunction()

endif()
