--
-- Copyright (C) 2015-2017 Uwe Bonnes bon@elektron.ikp.physik.tu-darmstadt.de.
--
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions
-- are met:
--
-- 1. Redistributions of source code must retain the above copyright
--    notice, this list of conditions and the following disclaimer.
-- 2. Redistributions in binary form must reproduce the above copyright
--    notice, this list of conditions and the following disclaimer in the
--    documentation and/or other materials provided with the distribution.
-- 3. Neither the name of the copyright holders nor the names of
--    contributors may be used to endorse or promote products derived
--    from this software without specific prior written permission.
--
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
-- ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
-- LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
-- FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
-- COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
-- INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
-- BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
-- OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
-- AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
-- OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
-- THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
-- SUCH DAMAGE.
--
-- For additional information see http://www.ethernut.de/

-- SPI1 TX
function GetSpi1TxDmaChoices()
    if c_is_provided("HW_DMA_STM32F2") then
        return {"(DMA_CONTROL2 | DMA_STREAM2 | DMA_CHANNEL2)",
                "(DMA_CONTROL2 | DMA_STREAM3 | DMA_CHANNEL3)",
                "(DMA_CONTROL2 | DMA_STREAM5 | DMA_CHANNEL3)",
                "DMA_NONE"}
        end
    if c_is_provided("HW_SPI1_DMA_REMAP_STM32F1") then
        return {"DMA1_CH3", "DMA2_CH5", "DMA1_CH7", "DMA_NONE"}
        end
    return {"DMA1_CH3", "DMA_NONE"}
end
-- SPI1 RX_DMA
function GetSpi1RxDmaChoices()
    if c_is_provided("HW_DMA_STM32F2") then
        return {"(DMA_CONTROL2 | DMA_STREAM0 | DMA_CHANNEL3)",
                "(DMA_CONTROL2 | DMA_STREAM2 | DMA_CHANNEL3)",
                "DMA_NONE"}
        end
    if c_is_provided("HW_SPI1_DMA_REMAP_STM32F1") then
        return {"DMA1_CH2", "DMA2_CH4", "DMA1_CH6", "DMA_NONE"}
        end
    return {"DMA1_CH2", "DMA_NONE"}
end

-- SPI2 TX_DMA
--
-- FIXME: STM32F07  may remap SPI2 DMA
--
function GetSpi2TxDmaChoices()
    if c_is_provided("HW_DMA_STM32F2") then
        return {"(DMA_CONTROL1 | DMA_STREAM4 | DMA_CHANNEL0)",
                "DMA_NONE"}
        end
    return {"DMA1_CH5", "DMA_NONE"}
end
--SPI2 RX_DMA
function GetSpi2RxDmaChoices()
    if c_is_provided("HW_DMA_STM32F2") then
        return {"(DMA_CONTROL1 | DMA_STREAM3 | DMA_CHANNEL0)",
                "DMA_NONE"}
        end
    return {"DMA1_CH4", "DMA_NONE"}
end

-- SPI3 TX_DMA
function GetSpi3TxDmaChoices()
    if c_is_provided("HW_DMA_STM32F2") then
        return {"(DMA_CONTROL1 | DMA_STREAM5 | DMA_CHANNEL0)",
                "(DMA_CONTROL1 | DMA_STREAM7 | DMA_CHANNEL0)",
                "DMA_NONE"}
        end
    return {"DMA2_CH2", "DMA_NONE"}
end
-- SPI3 RX_DMA
function GetSpi3RxDmaChoices()
    if c_is_provided("HW_DMA_STM32F2") then
        return {"(DMA_CONTROL1 | DMA_STREAM0 | DMA_CHANNEL0)",
                "(DMA_CONTROL1 | DMA_STREAM2 | DMA_CHANNEL0)",
                "DMA_NONE"}
        end
    return {"DMA2_CH1", "DMA_NONE"}
end

-- SPI4 TX_DMA
function GetSpi4TxDmaChoices()
    if c_is_provided("HW_DMA_STM32F2") then
        return {"(DMA_CONTROL2 | DMA_STREAM1 | DMA_CHANNEL4)",
                "(DMA_CONTROL2 | DMA_STREAM3 | DMA_CHANNEL5)",
                "DMA_NONE"}
        end
    return {"DMA2_CH5", "DMA_NONE"}
end
-- SPI4 RX_DMA
function GetSpi4RxDmaChoices()
    if c_is_provided("HW_DMA_STM32F2") then
        return {"(DMA_CONTROL1 | DMA_STREAM0 | DMA_CHANNEL4)",
                "(DMA_CONTROL1 | DMA_STREAM3 | DMA_CHANNEL5)",
                "DMA_NONE"}
        end
    return {"DMA2_CH4", "DMA_NONE"}
end

-- SPI5 TX_DMA
function GetSpi5TxDmaChoices()
--    if c_is_provided("HW_DMA_STM32F2") then
        return {"(DMA_CONTROL2 | DMA_STREAM4 | DMA_CHANNEL2)",
                "(DMA_CONTROL2 | DMA_STREAM6 | DMA_CHANNEL7)",
                "DMA_NONE"}
--        end
--    return {"DMA2_CH5", "DMA_NONE"}
end
-- SPI5 RX_DMA
function GetSpi5RxDmaChoices()
--    if c_is_provided("HW_DMA_STM32F2") then
        return {"(DMA_CONTROL1 | DMA_STREAM3 | DMA_CHANNEL2)",
                "(DMA_CONTROL1 | DMA_STREAM5 | DMA_CHANNEL7)",
                "DMA_NONE"}
--        end
--    return {"DMA2_CH4", "DMA_NONE"}
end

-- SPI6 TX_DMA
function GetSpi6TxDmaChoices()
    return
        {"(DMA_CONTROL2 | DMA_STREAM5 | DMA_CHANNEL1)", "DMA_NONE"}
end
--SPI6 RX_DMA
function GetSpi6RxDmaChoices()
    return
        {"(DMA_CONTROL2 | DMA_STREAM6 | DMA_CHANNEL1)", "DMA_NONE"}
end

function SpiDevice(SPIX, DEVICE_FLAG, SRCX, REMAP_FLAG, REMAP_CHOICE,
         NSS_FLAG, NSS_REQ, NSS_CHOICE,
         SCK_CHOICE,
         MISO_CHOICE,
         MOSI_CHOICE,
         TX_DMA_CHOICES,
         RX_DMA_CHOICES
         )
    return
    {
        name = function() return 'nutarch_cm3_stm32_' .. SPIX ;end,
        brief = function() return 'STM32 ' .. SPIX .. ' Bus Controller' ;end,
        description = function() return SPIX ..
            ' Controller with up to four Chip select line.' ;end,
        requires = function() return DEVICE_FLAG ;end,
        provides = { "SPIBUS_CONTROLLER", "HW_SPI_STM32" },
        sources =  function() return SRCX ;end,
        options =
        {
            {
                macro = function() return SPIX .. '_REMAP';end,
                brief = "Use Alternate Pins",
                description = function() return REMAP_CHOICE ;end,
                requires = function() return REMAP_FLAG ;end,
                type = "enumerated",
                choices = {"0", "1"},
                default = "0",
                file = "include/cfg/spi.h"
            },
            {
                macro = function() return SPIX .. '_SPEED' ; end,
                brief = function() return SPIX .. 'Pin speed' ; end,
                description = function() return PinSpeedDesc() end,
                choices = function() return GetPinSpeedChoices() end,
                type = "enumerated",
                default = "GPIO_MED",
                file = "include/cfg/spi.h"
            },
            {
                macro = function() return SPIX .. '_CS0' ; end,
                brief = function() return SPIX .. ' CS0 Gpio' ; end,
                description = function() return
                    "Choice of " .. SPIX .. "CS0 Gpio.\n"..
                    "Use PIN_NONE if no CS is required.\n"; end,
                default = "PIN_NONE",
                file = "include/cfg/spi.h"
            },
            {
                macro = function() return SPIX .. '_CS1' ; end,
                brief = function() return SPIX .. ' CS1 Gpio' ; end,
                description = function() return
                'Choose ' .. SPIX .. ' CS1. Use PIN_NONE is unused.' ; end,
                default = "PIN_NONE",
                file = "include/cfg/spi.h"
            },
            {
                macro = function() return SPIX .. '_CS2' ; end,
                brief = function() return SPIX .. ' CS2 Gpio' ; end,
                description = function() return
                'Choose ' .. SPIX .. ' CS2. Use PIN_NONE is unused.' ; end,
                default = "PIN_NONE",
                file = "include/cfg/spi.h"
            },
            {
                macro = function() return SPIX .. '_CS3' ; end,
                brief = function() return SPIX .. ' CS3 Gpio' ; end,
                description = function() return
                   'Choose ' .. SPIX .. ' CS3. Use PIN_NONE is unused.' ; end,
                default = "PIN_NONE",
                file = "include/cfg/spi.h"
            },
            {
                macro = function() return SPIX .. '_USE_NSS' ; end,
                brief = "Use NSS",
                description = "Use Hardware NSS for SPI. Default off.",
                provides = function() return NSS_FLAG; end,
                flavor = "booldata",
                file = "include/cfg/uart.h"
            },
            {
                macro = function() return SPIX .. '_NSS' ; end,
                brief = function() return SPIX .. ' NSS Pins' ; end,
                description = function() return
                    'Choice of ' .. SPIX .. ' NSS Pins' ; end,
                requires = function() return NSS_REQ; end,
                type = "enumerated",
                choices = function() return NSS_CHOICE end,
                default = "PIN_NONE",
                file = "include/cfg/spi.h"
            },
            {
                macro = function() return SPIX .. '_SCK' ; end,
                brief = function() return SPIX .. ' SCK Pin' ; end,
                description = function() return
                   'Choice of ' .. SPIX .. ' SCK Pins.' ; end,
                requires = { "HW_GPIO_STM32V2" },
                type = "enumerated",
                choices = function() return SCK_CHOICE() ; end,
                default = function() return SCK_CHOICE()[1] ; end,
                file = "include/cfg/spi.h"
            },
            {
                macro = function() return SPIX .. '_MISO' ; end,
                brief = function() return SPIX .. ' MISO Pin' ; end,
                description = function() return
                   'Choice of ' .. SPIX .. ' MISO Pins.' ; end,
                requires = { "HW_GPIO_STM32V2" },
                type = "enumerated",
                choices = function() return MISO_CHOICE() ; end,
                default = function() return MISO_CHOICE()[1] ; end,
                file = "include/cfg/spi.h"
            },
            {
                macro = function() return SPIX .. '_MOSI' ; end,
                brief = function() return SPIX .. ' MOSI Pin' ; end,
                description = function() return
                   'Choice of ' .. SPIX .. ' MOSI Pins.' ; end,
                requires = { "HW_GPIO_STM32V2" },
                type = "enumerated",
                choices = function() return MOSI_CHOICE() ; end,
                default = function() return MOSI_CHOICE()[1] ; end,
                file = "include/cfg/spi.h"
            },
            {
                macro = function() return SPIX .. '_MODE' ; end,
                brief = function() return SPIX .. ' MODE Pin' ; end,
                description = function() return
                   'Choice of ' .. SPIX .. ' IRQ_MODE(default),'..
                   ' DMA_MODE or POLLING_MODE' ; end,
                type = "enumerated",
                choices = {"IRQ_MODE", "DMA_MODE", "POLLING_MODE"},
                default = "IRQ_MODE",
                file = "include/cfg/spi.h"
            },
            {
                macro = function() return SPIX .. '_DMA_TX' ; end,
                brief = function() return SPIX .. '_DMA_TX' ; end,
                description = function() return
                   'Select ' .. SPIX .. ' TX DMA.\n'..
                   'Take care for collisions!\n'..
                   'Use DMA_NONE to disable even with xDMA_MODE.' ; end,
                type = "enumerated",
                choices = function() return TX_DMA_CHOICES() ; end,
                default = function() return TX_DMA_CHOICES()[1] ; end,
                file = "include/cfg/spi.h"
            },
            {
                macro = function() return SPIX .. '_DMA_RX' ; end,
                brief = function() return SPIX .. '_DMA_RX' ; end,
                description = function() return
                   'Select ' .. SPIX .. ' RX DMA.\n'..
                   'Take care for collisions!\n'..
                   'Use DMA_NONE to disable.' ; end,
                type = "enumerated",
                choices = function() return RX_DMA_CHOICES() ; end,
                default = function() return RX_DMA_CHOICES()[1] ; end,
                file = "include/cfg/spi.h"
            },
        },
    }
end

nutarch_cm3_stm32_spi_devices =
{
    -- ******************************************
    --
    -- STM32 SPI Device Configuration and drivers
    --
    -- ******************************************
    --
    SpiDevice(
        "SPI1",

        { "HW_SPI1_STM32" },

        { "cm3/dev/stm/stm32_spi1.c" },

        { "HW_GPIO_STM32V1"},

        "Choose SPI1 Pin remap. Default = 0 means no remap.\n\n"..
        "Default 0 Pin Mapping is:\n"..
        "NSS PA04\nSCK PA05\nMISO PA06\nMOSI PA07\n"..
        "Remap   1 Pin Mapping is:\n" ..
        "NSS PA15\nSCK PB03\nMISO PB04\nMOSI PB05\n",

        { "SPI1_USE_NSS" },

        { "HW_GPIO_STM32V2", "SPI1_USE_NSS" },

        GetSpi1Nss,

        GetSpi1Sck,

        GetSpi1Miso,

        GetSpi1Mosi,

        GetSpi1TxDmaChoices,

        GetSpi1RxDmaChoices
    ),
    --
    -- STM32F SPI2 Interface
    --
    SpiDevice(
        "SPI2",

        { "HW_SPI2_STM32" },

        { "cm3/dev/stm/stm32_spi2.c" },

        { "NOT_AVAILVABLE" },

        "",

        { "SPI2_USE_NSS" },

        { "HW_GPIO_STM32V2", "SPI2_USE_NSS" },

        GetSpi2Nss,

        GetSpi2Sck,

        GetSpi2Miso,

        GetSpi2Mosi,

        GetSpi2TxDmaChoices,

        GetSpi2RxDmaChoices
    ),
    --
    -- STM32F SPI3 Interface
    --
    SpiDevice(
        "SPI3",

        { "HW_SPI3_STM32" },

        { "cm3/dev/stm/stm32_spi3.c" },

        { "HW_GPIO_STM32V1" },

        "Choose SPI3 Pin remap. Default = 0 means no remap.\n\n"..
        "Default 0 Pin Mapping is:\n NSS PA15\n SCK PB03\n MISO PB04\n MOSI PB05\n"..
        "Remap   1 Pin Mapping is:\n NSS PA04\n SCK PC10\n MISO PC11\n MOSI PC12\n",

        { "SPI3_USE_NSS" },

        { "HW_GPIO_STM32V2", "SPI3_USE_NSS" },

        GetSpi3Nss,

        GetSpi3Sck,

        GetSpi3Miso,

        GetSpi3Mosi,

        GetSpi3TxDmaChoices,

        GetSpi3RxDmaChoices
    ),
    --
    -- STM32F SPI4 Interface
    --
    SpiDevice(
        "SPI4",

        { "HW_SPI4_STM32" },

        { "cm3/dev/stm/stm32_spi4.c" },

        { "NOT_AVAILVABLE" },

        "",

        { "SPI4_USE_NSS" },

        { "HW_GPIO_STM32V2", "SPI4_USE_NSS" },

        GetSpi4Nss,

        GetSpi4Sck,

        GetSpi4Miso,

        GetSpi4Mosi,

        GetSpi4TxDmaChoices,

        GetSpi4RxDmaChoices
    ),
    --
    -- STM32 SPI5 Interface
    --
    SpiDevice(
        "SPI5",

        { "HW_SPI5_STM32" },

        { "cm3/dev/stm/stm32_spi5.c" },

        { "NOT_AVAILVABLE" },

        "",

        { "SPI5_USE_NSS" },

        { "HW_GPIO_STM32V2", "SPI5_USE_NSS" },

        GetSpi5Nss,

        GetSpi5Sck,

        GetSpi5Miso,

        GetSpi5Mosi,

        GetSpi5TxDmaChoices,

        GetSpi5RxDmaChoices
    ),
    --
    -- STM32F SPI6 Interface
    --
   SpiDevice(
        "SPI6",

        { "HW_SPI6_STM32" },

        { "cm3/dev/stm/stm32_spi6.c" },

        { "NOT_AVAILVABLE" },

        "",

        { "SPI6_USE_NSS" },

        { "HW_GPIO_STM32V2", "SPI6_USE_NSS" },

        GetSpi6Nss,

        GetSpi6Sck,

        GetSpi6Miso,

        GetSpi6Mosi,

        GetSpi6TxDmaChoices,

        GetSpi6RxDmaChoices
    ),
}
