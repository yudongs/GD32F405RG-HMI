/*
 * FreeRTOS Kernel V11.3.0
 * GD32F405RG Configuration
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* Processor and Clock Settings */
#define configCPU_CLOCK_HZ              ( 168000000UL )
#define configTICK_RATE_HZ              ( 1000UL )          /* 1ms tick */
#define configUSE_PREEMPTION            1
#define configUSE_IDLE_HOOK             1
#define configUSE_TICK_HOOK             1
#define configMAX_PRIORITIES            8
#define configMINIMAL_STACK_SIZE        ( 256UL )           /* words */
/* heap size:
 *   USE_VTFP=0 (production) -> 48 KB
 *   USE_VTFP=1 (dev/debug)   -> 46 KB (让出 2KB 给 .vtfp section)
 * If the build still hard-codes the smaller value, override locally. */
#ifndef configTOTAL_HEAP_SIZE
#ifdef USE_VTFP
#define configTOTAL_HEAP_SIZE           ( 46 * 1024UL )     /* 46KB heap (VTFP reserves 2KB) */
#else
#define configTOTAL_HEAP_SIZE           ( 48 * 1024UL )     /* 48KB heap */
#endif
#endif
#define configMAX_TASK_NAME_LEN         16
#define configUSE_TRACE_FACILITY        1
#define configUSE_16_BIT_TICKS          0
#define configSUPPORT_STATIC_ALLOCATION 0
#define configENABLE_FPU               1
#define configENABLE_MPU               0

/* Synchronization primitives */
#define configUSE_MUTEXES              1
#define configUSE_RECURSIVE_MUTEXES    1
#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_QUEUE_SETS           1
#define configUSE_TIME_SLICING          1
#define configUSE_NEWLIB_REENTRANT      0

/* Hook functions */
#define configCHECK_FOR_STACK_OVERFLOW   2
#define configUSE_TICK_HOOK             1
#define configUSE_IDLE_HOOK             1

/* System Interrupts - Map to GD32 startup file symbols */
#define vPortSVCHandler               SVC_Handler
#define xPortPendSVHandler            PendSV_Handler
#define xPortSysTickHandler           SysTick_Handler

/* Interrupt priority settings */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     5
#define configNVIC_PRIO_BITS          4         /* GD32F4xx has 4 bits for priority */

/* Optional: Use timer for tickless idle */
#define configUSE_TICKLESS_IDLE       0

/* Function inclusion */
#define INCLUDE_vTaskDelay             1
#define INCLUDE_vTaskDelete            1   /* VTFP_Task 错误路径需要 vTaskDelete(NULL) */

/* Run time stats */
#define configGENERATE_RUN_TIME_STATS  0

/* Co-routine settings */
#define configUSE_CO_ROUTINES         0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

/* Timer settings */
#define configUSE_TIMERS              1
#define configTIMER_TASK_PRIORITY     3
#define configTIMER_QUEUE_LENGTH      10
#define configTIMER_TASK_STACK_DEPTH  ( 256UL )

/* ISR nested interrupt handling */
#define configYIELD_IN_PEND_ISR       1

/* Performance measurement */
#define configMEASURE_INTERRUPT_LATENCY  0

#endif /* FREERTOS_CONFIG_H */
