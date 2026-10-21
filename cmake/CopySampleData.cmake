# Copies the sample data file only when no data file exists yet.
if(NOT EXISTS "${DST}")
    file(COPY_FILE "${SRC}" "${DST}")
endif()
