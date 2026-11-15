# Runs Dear ImGui's binary_to_compressed_c on a font and writes the result to a file.
# The output defines <SYMBOL>_compressed_size and <SYMBOL>_compressed_data (static).
execute_process(
    COMMAND "${TOOL}" -u32 "${INPUT}" "${SYMBOL}"
    OUTPUT_FILE "${OUTPUT}"
    RESULT_VARIABLE result
)
if(result)
    file(REMOVE "${OUTPUT}")
    message(FATAL_ERROR "binary_to_compressed_c failed for ${INPUT}: ${result}")
endif()
