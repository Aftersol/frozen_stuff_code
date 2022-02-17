cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(frozen_stuff)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
file(GLOB MUSIC_SAMPELES
    ${PROJECT_SOURCE_DIR}/samples/*.c
    ${PROJECT_SOURCE_DIR}/samples/*.h
)

add_executable(frozen_stuff_firmware
    main.c
    adc.h
    adc.c
    audio_pwm.h
    audio_pwm.c
    audio_samples.h
    ${MUSIC_SAMPELES}
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(frozen_stuff_firmware
    pico_stdlib
    hardware_spi
    hardware_pwm
    hardware_dma
)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(frozen_stuff_firmware)