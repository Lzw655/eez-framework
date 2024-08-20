set(UI_DRI ui/)
file(GLOB_RECURSE UI_SRCS ${UI_DRI}/*.c)

idf_component_register(
    SRCS "main.c" ${UI_SRCS}
    INCLUDE_DIRS ".")
