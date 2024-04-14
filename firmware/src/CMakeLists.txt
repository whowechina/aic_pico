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
                   main.c save.c config.c commands.c light.c keypad.c
                   cli.c usb_descriptors.c)
    target_compile_definitions(${board} PUBLIC ${board_def})
    pico_enable_stdio_usb(${board} 1)
    pico_enable_stdio_uart(${board} 0)

    pico_generate_pio_header(${board} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)
    
    target_link_libraries(${board} PRIVATE
        aic hardware_pio hardware_pwm hardware_flash)

    pico_add_extra_outputs(${board})

    add_custom_command(TARGET ${board} POST_BUILD
                       COMMAND cp ${board}.uf2 ${CMAKE_CURRENT_LIST_DIR}/..)
endfunction()

add_library(aic lib/aime.c lib/bana.c lib/pn532.c lib/pn5180.c lib/nfc.c)
make_firmware(aic_pico BOARD_AIC_PICO)
