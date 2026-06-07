/*
 * FreeRTOS Board Configuration for GD32F405RG
 */
#ifndef FREERTOS_BOARD_H
#define FREERTOS_BOARD_H

/* GD32F405RG Chip Settings */
#define GD32_CHIP_TYPE              "GD32F405RG"
#define GD32_FPU_ENABLED            1
#define GD32_FLASH_SIZE             ( 512UL * 1024UL )   /* 512KB */
#define GD32_SRAM_SIZE              ( 128UL * 1024UL )    /* 128KB */

/* System Clock */
#define configSYSTICK_CLOCK_HZ     ( 168000000UL )

/* Optional: TCMRAM region for critical data */
#define configUSE_TCMRAM            1

/* Interrupt priority mask - GD32 uses 4 bits (0-15) */
#define configMAX_PRIORITY_MASK     0x0F

/* Hardware specific definitions */
#define configGD32_EXTI_ENABLED     1
#define configGD32_DMA_ENABLED      1

#endif /* FREERTOS_BOARD_H */
