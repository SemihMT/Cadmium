add_library(colortextedit-lib STATIC
    ${colortextedit_SOURCE_DIR}/TextEditor.cpp
)

target_include_directories(colortextedit-lib PUBLIC
    ${colortextedit_SOURCE_DIR}
)

target_link_libraries(colortextedit-lib PUBLIC imgui-lib)
