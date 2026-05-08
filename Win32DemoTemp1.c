// Win32DemoTemp1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

/*
 * FreeRTOS Kernel V11.1.0
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

 /*
  * This is a simple main that will start the FreeRTOS-Kernel and run a periodic task
  * that only delays if compiled with the template port, this project will do nothing.
  * For more information on getting started please look here:
  * https://freertos.org/FreeRTOS-quick-start-guide.html
  */

  /* FreeRTOS includes. */
  #include <FreeRTOS.h>
  #include <task.h>
  #include <queue.h>
  #include <timers.h>
  #include <semphr.h>
  #include <windows.h>
static TaskHandle_t xStatsTaskHandle = NULL;
  
static LARGE_INTEGER ulHighFreqTimer;
static LARGE_INTEGER ulLastCounterValue;

void configureTimerForRunTimeStats(void) {
    //Get frequency of the high resolution timer
    QueryPerformanceFrequency(&ulHighFreqTimer);
    QueryPerformanceCounter(&ulLastCounterValue);
}

unsigned long getRunTimeCounterValue(void) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    unsigned long elapsed = (unsigned long)(
        (now.QuadPart - ulLastCounterValue.QuadPart) * 1000000ULL
        / ulHighFreqTimer.QuadPart
        );
    return elapsed;
}

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#define _CRT_SECURE_NO_WARNINGS
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

/*-----------------------------------------------------------*/

static void exampleTask(void* parameters);
static void vPeriodicTask1(void* pvParameters) {
    TickType_t lastWake1 = xTaskGetTickCount();
    const TickType_t period1 = pdMS_TO_TICKS(1000);
    static int deadlineMisses1 = 0;
    for (;;) {
        TickType_t expected = lastWake1 + period1;

        vTaskDelayUntil(&lastWake1, period1);

        //actual wake time
        TickType_t actual = xTaskGetTickCount();

        //measuring jitter value
        TickType_t jitter = actual - expected;

        //release time
        TickType_t release = actual;
        volatile long sum = 0;
        for (volatile int i = 0; i < 2000000000L; i++) sum += i;
        (void)sum;
        TickType_t finish = xTaskGetTickCount();

        TickType_t response = finish - release;
        if (response > period1) {
            printf("[T1] Deadline Miss: %lu\n", (unsigned long)response);
            deadlineMisses1++;
        }

        printf("[T1] Response: %lu | Jitter: %lu | Misses: %d\n",
            (unsigned long)response,
            (unsigned long)jitter,
            deadlineMisses1);
        //vTaskDelay(pdMS_TO_TICKS(200));



        
    }
}
static void vPeriodicTask2(void* pvParameters) {
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(1500);
    static int deadlineMisses = 0;
    for (;;) {
        //expected wake time
        TickType_t expected = lastWake + period;
        vTaskDelayUntil(&lastWake, period);

        //getting actual wake time
        TickType_t actual = xTaskGetTickCount();

        //measuring jitter value
        TickType_t jitter = actual - expected;

        //storing release time for response calculation
        TickType_t release = actual;
        volatile long sum = 0;
        for (long i = 0; i < 2000000000L; i++) sum += i;
        (void)sum;
        TickType_t finish = xTaskGetTickCount();

        TickType_t response = finish - release;
        if (response > period) {
            printf("[T2] Deadline Miss: %lu\n", (unsigned long)response);
            deadlineMisses++;
        }

        printf("[T2] Response: %lu | Jitter: %lu | Misses: %d\n",
            (unsigned long)response,
            (unsigned long)jitter,
            deadlineMisses);
    }
}

static void vAperiodicTask(void* pvParameters) {
    for (;;) {
        //use 2000 instead of 500 for experiment 3
        //use 200 instead of 500 for experiment 3
        vTaskDelay(pdMS_TO_TICKS(100 + rand() % 500)); 
        TickType_t start = xTaskGetTickCount();

        //use uart transmit to print the start of the tick count
        volatile long sum = 0;
        //use 50000000L for experiment 2 and 2000000000L for experiment 3
        for (volatile long i = 0; i < 50000000L; i++);
     
        
        TickType_t end = xTaskGetTickCount();

        //TickType_t response = end - start;

        printf("[Aperiodic] Release: %lu ticks | Execution %lu ticks\n", 
            (unsigned long) start,
            (unsigned long) (end - start));
       
    }
}

static void vInputTask(void* pvParameters) {
    for (;;) {
        if (_kbhit()) {
            int c = _getch();
            fflush(stdout);
            switch (c) {
            case 's':
            case 'S':
                fflush(stdout);
                if (xStatsTaskHandle != NULL) {
                    xTaskNotifyGive(xStatsTaskHandle);
                }
                else {
                    printf("[Input] Error: xStatsTaskHandle is Null\n");
                    fflush(stdout);
                }
                break;
            case 3:
                fflush(stdout);
                vTaskSuspendAll();
                exit(0);
            default:
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

//task to print the runtime stats table
static void vStatsTask(void* pvParameters) {
    static char statsBuffer[1024];
    printf("Press S to display runtime stats \n");
    fflush(stdout);

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        printf("\n--- Runtime Stats ---\n");
        vTaskGetRunTimeStats(statsBuffer);
        printf("%s\n", statsBuffer);
    }
}

/*-----------------------------------------------------------*/

static void exampleTask(void* parameters)
{
    /* Unused parameters. */
    (void)parameters;

    for (; ; )
    {
        /* Example Task Code */
        vTaskDelay(100); /* delay 100 ticks */
    }
}
/*-----------------------------------------------------------*/

void main(void)
{
    /**static StaticTask_t exampleTaskTCB;
    static StackType_t exampleTaskStack[ configMINIMAL_STACK_SIZE ];

    ( void ) printf( "Example FreeRTOS Project\n" );

    ( void ) xTaskCreateStatic( exampleTask,
                                "example",
                                configMINIMAL_STACK_SIZE,
                                NULL,
                                configMAX_PRIORITIES - 1U,
                                &( exampleTaskStack[ 0 ] ),
                                &( exampleTaskTCB ) );

    /* Start the scheduler. */
    //need to create functions/tasks for run_time_Stats and stats_formatting_Functions
    printf("FreeRTOS Win32 Demo\n");
    printf("Measuring Response Time, Jitter, and Deadline misses. \n");

    static StaticTask_t periodicTCB1, periodicTCB2;
    static StackType_t periodicStack1[configMINIMAL_STACK_SIZE];
    static StackType_t periodicStack2[configMINIMAL_STACK_SIZE];

    static StaticTask_t aperiodicTCB;
    static StackType_t aperiodicStack[configMINIMAL_STACK_SIZE];
    
    static StaticTask_t statsTCB;
    static StackType_t statsStack[configMINIMAL_STACK_SIZE * 4];

    static StaticTask_t inputTCB;
    static StackType_t inputStack[configMINIMAL_STACK_SIZE * 4];

    xTaskCreateStatic(vPeriodicTask1, "Periodic Task 1", configMINIMAL_STACK_SIZE, NULL, 2, periodicStack1, &periodicTCB1);
    xTaskCreateStatic(vPeriodicTask2, "Periodic Task 2", configMINIMAL_STACK_SIZE, NULL, 2, periodicStack2, &periodicTCB2);
    xTaskCreateStatic(vAperiodicTask, "Aperiodic Task 1", configMINIMAL_STACK_SIZE, NULL, 1, aperiodicStack, &aperiodicTCB);
    xStatsTaskHandle = xTaskCreateStatic(vStatsTask, "Stats", configMINIMAL_STACK_SIZE * 4, NULL, 1, statsStack, &statsTCB);
    xTaskCreateStatic(vInputTask, "Input", configMINIMAL_STACK_SIZE * 4, NULL, 1, inputStack, &inputTCB);
    vTaskStartScheduler();

    for (; ; )
    {
        /* Should not reach here. */
    }
}
/*-----------------------------------------------------------*/

#if ( configCHECK_FOR_STACK_OVERFLOW > 0 )

void vApplicationStackOverflowHook(TaskHandle_t xTask,
    char* pcTaskName)
{
    /* Check pcTaskName for the name of the offending task,
     * or pxCurrentTCB if pcTaskName has itself been corrupted. */
    (void)xTask;
    (void)pcTaskName;
}

#endif /* #if ( configCHECK_FOR_STACK_OVERFLOW > 0 ) */
/*-----------------------------------------------------------*/


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
