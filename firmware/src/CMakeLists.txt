set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)
set(LWIP_ROOT ${PICO_SDK_PATH}/lib/lwip)

function(make_firmware board board_def)
    pico_sdk_init()
    add_executable(${board}
        main.c save.c config.c commands.c light.c keypad.c
        aime.c cli.c nfc.c pn532.c pn5180.c
        usb_descriptors.c)
    target_compile_definitions(${board} PUBLIC ${board_def})
    pico_enable_stdio_usb(${board} 1)
    pico_enable_stdio_uart(${board} 0)

    pico_generate_pio_header(${board} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)
    
    target_compile_options(${board} PRIVATE -Wall -Werror -Wfatal-errors -O3)
    target_include_directories(${board} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_include_directories(${board} PRIVATE 
                               ${BTSTACK_ROOT}/src
                               ${LWIP_ROOT}/src/include)
    
    target_link_libraries(${board} PRIVATE
        pico_multicore pico_stdlib hardware_pio hardware_pwm hardware_flash
        hardware_adc hardware_i2c hardware_spi hardware_watchdog
        tinyusb_device tinyusb_board)

    pico_add_extra_outputs(${board})

    add_custom_command(TARGET ${board} POST_BUILD
                       COMMAND cp ${board}.uf2 ${CMAKE_CURRENT_LIST_DIR}/..)
endfunction()

make_firmware(aic_pico BOARD_AIC_PICO)

