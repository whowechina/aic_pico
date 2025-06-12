pico_sdk_init()

include_directories(${CMAKE_CURRENT_LIST_DIR}
                    ${CMAKE_CURRENT_LIST_DIR}/../include)
add_compile_options(-Wall -Werror -Wfatal-errors -O3)
link_libraries(pico_multicore pico_stdlib hardware_i2c hardware_spi
               tinyusb_device tinyusb_board)
                                   
function(make_firmware board board_def)
    if(NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
        return()
    endif()

    add_executable(${board}
                   main.c save.c config.c commands.c light.c keypad.c cardio.c
                   cst816t.c st7789.c gui.c gfx.c rle.c
                   cli.c usb_descriptors.c)

    target_compile_definitions(${board} PUBLIC ${board_def}
                               PICO_XOSC_STARTUP_DELAY_MULTIPLIER=20)

    pico_enable_stdio_usb(${board} 1)

    pico_generate_pio_header(${board} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)
    
    target_link_libraries(${board} PRIVATE
        aic hardware_pio hardware_pwm hardware_dma hardware_flash)

    pico_add_extra_outputs(${board})

    if(CMAKE_HOST_WIN32)
        add_custom_command(TARGET ${board} PRE_BUILD
            COMMAND powershell -Command "(Get-Item '${CMAKE_CURRENT_SOURCE_DIR}/cli.c').LastWriteTime = Get-Date"
        )
        add_custom_command(TARGET ${board} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${board}.uf2 ${CMAKE_CURRENT_LIST_DIR}/..
        )
    else()
        add_custom_command(TARGET ${board} PRE_BUILD
            COMMAND touch ${CMAKE_CURRENT_SOURCE_DIR}/cli.c
        )
        add_custom_command(TARGET ${board} POST_BUILD
            COMMAND cp ${board}.uf2 ${CMAKE_CURRENT_LIST_DIR}/..
        )
    endif()

endfunction()

add_library(aic lib/aime.c lib/bana.c lib/pn532.c lib/pn5180.c lib/nfc.c lib/mode.c lib/spad0.c)
make_firmware(aic_pico BOARD_AIC_PICO)
