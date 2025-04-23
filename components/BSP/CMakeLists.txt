set(src_dirs
            MYIIC
            MYSPI
            SPILCD
            SPI_SD
            ES8388
            MYI2S
            XL9555
            ei
            )

set(include_dirs
            MYIIC
            MYSPI
            SPILCD
            SPI_SD
            ES8388
            MYI2S
            XL9555
            ei
            )

set(requires
            driver
            esp_lcd
            fatfs)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)