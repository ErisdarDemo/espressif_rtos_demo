/**************************************************************************************************/
/** @file     main.c
 *  @brief    ESP32 FreeRTOS demonstration entry source file
 *  @details  FreeRTOS Real Time Stats Example 'real_time_stats_example_main'
 *
 *  @author   Justin Reina, Firmware Engineer
 *  @created  2/3/25
 *  @last rev 2/3/25
 *
 *   @section 	Opens
 * 		post both updates to new www rtos page
 *   		
 *   @section 	Deferred
 * 		Get 'F11' to launch debug
 * 		Relocate main\CMakeLists.txt & close main\
 *
 *  @section    Legal Disclaimer
 * 		©2025 Justin Reina. All rights reserved. All contents of this source file and/or any other
 *      related source files are the explicit property of Justin Reina. Do not distribute.
 *      Do not copy.
 */
/**************************************************************************************************/

//************************************************************************************************//
//                                            INCLUDES                                            //
//************************************************************************************************//

//Standard Library Includes
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

//SDK Includes
#include "sdkconfig.h"
#include "esp_err.h"

//FreeRTOS Includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


//Project Includes
#include "main.h"


//************************************************************************************************//
//                                        DEFINITIONS & TYPES                                     //
//************************************************************************************************//

//-----------------------------------------  Definitions -----------------------------------------//

//Task Definitions
#define NUM_OF_SPIN_TASKS   (6)
#define SPIN_ITER           (500000)  	/* Actual ct used will depend on compiler optimization	  */
#define SPIN_TASK_PRIO      (2)

//Statistic Definitions
#define STATS_TASK_PRIO     (3)
#define STATS_TICKS         pdMS_TO_TICKS(1000)

//Error Support
#define ARRAY_SIZE_OFFSET   (5)   		/* Soln ++ iff ESP_ERR_INVALID_SIZE					      */

//-------------------------------------------- Macros --------------------------------------------//


//----------------------------------------- Enumerations -----------------------------------------//


//------------------------------------------- Typedefs -------------------------------------------//


//************************************************************************************************//
//                                            VARIABLES                                           //
//************************************************************************************************//

//Tasks
static char task_names[NUM_OF_SPIN_TASKS][configMAX_TASK_NAME_LEN];

//Semaphores
static SemaphoreHandle_t sync_spin_task;
static SemaphoreHandle_t sync_stats_task;


//************************************************************************************************//
//                                       FUNCTION DECLARATIONS                                    //
//************************************************************************************************//


//************************************************************************************************//
//                                          PUBLIC FUNCTIONS                                      //
//************************************************************************************************//

/**************************************************************************************************/
/** @fcn        static esp_err_t print_real_time_stats(TickType_t xTicksToWait)
 *  @brief      Function to print the CPU usage of tasks over a given duration.
 *  @details    x
 *
 *  @section    Purpose
 *      This function will measure and print the CPU usage of tasks over a specified number of 
 *      ticks (i.e. real time stats). This is implemented by simply calling uxTaskGetSystemState() 
 *      twice separated by a delay, then calculating the differences of task run times before and 
 *      after the delay.
 *
 *  @param    [in]  (TickType_t) xTicksToWait - Period of stats measurement
 *
 *  @return   (esp_err_t) ?
 *             - ESP_OK                Success
 *             - ESP_ERR_NO_MEM        Insufficient memory to allocated internal arrays
 *             - ESP_ERR_INVALID_SIZE  Insufficient array size for uxTaskGetSystemState. 
 *             - ESP_ERR_INVALID_STATE Delay duration too short
 *
 *  @note    for 'ESP_ERR_INVALID_SIZE' - Trying increasing ARRAY_SIZE_OFFSET
 *
 *  @section    Notes
 *       If any tasks are added or removed during the delay, the stats of those tasks will not be 
 *       printed
 *
 *       This function should be called from a high priority task to minimize inaccuracies w/delays
 *
 *       When running in dual core mode, each core will correspond to 50% of the run time.
 */
/**************************************************************************************************/
static esp_err_t print_real_time_stats(TickType_t xTicksToWait) {
	
	//Locaks
    TaskStatus_t *start_array = NULL, *end_array = NULL;
    UBaseType_t start_array_size, end_array_size;
    configRUN_TIME_COUNTER_TYPE start_run_time, end_run_time;
    esp_err_t ret;

    //Allocate array to store current task states
    start_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    start_array = malloc(sizeof(TaskStatus_t) * start_array_size);
    
    if (start_array == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    
    
    //Get current task states
    start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);
    
    if (start_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    vTaskDelay(xTicksToWait);

    //Allocate array to store tasks states post delay
    end_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    end_array = malloc(sizeof(TaskStatus_t) * end_array_size);
    
    if (end_array == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    
    
    //Get post delay task states
    end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);
    
    if (end_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }
    

    //Calculate total_elapsed_time in units of run time stats clock period.
    uint32_t total_elapsed_time = (end_run_time - start_run_time);
    if (total_elapsed_time == 0) {
        ret = ESP_ERR_INVALID_STATE;
        goto exit;
    }

    printf("| Task X Run Time | Percentage\n");
    
    //Match each task in start_array to those in the end_array
    for (int i = 0; i < start_array_size; i++) {
		
        int k = -1;
        
        for (int j = 0; j < end_array_size; j++) {
			
            if (start_array[i].xHandle == end_array[j].xHandle) {
				
                k = j;
                
                //Mark that task have been matched by overwriting their handles
                start_array[i].xHandle = NULL;
                end_array[j].xHandle = NULL;
                
                break;
            }
        }
        
        //Check if matching task found
        if (k >= 0) {
			
            uint32_t task_elapsed_time = end_array[k].ulRunTimeCounter - start_array[i].ulRunTimeCounter;
            uint32_t percentage_time = (task_elapsed_time * 100UL) / (total_elapsed_time * CONFIG_FREERTOS_NUMBER_OF_CORES);
            
            printf("| %s | %"PRIu32" | %"PRIu32"%%\n", start_array[i].pcTaskName, task_elapsed_time, percentage_time);
        }
    }

    //Print unmatched tasks
    for (int i = 0; i < start_array_size; i++) {
        if (start_array[i].xHandle != NULL) {
            printf("| %s | Deleted\n", start_array[i].pcTaskName);
        }
    }
    for (int i = 0; i < end_array_size; i++) {
        if (end_array[i].xHandle != NULL) {
            printf("| %s | Created\n", end_array[i].pcTaskName);
        }
    }
    ret = ESP_OK;

exit:    //Common return path
    free(start_array);
    free(end_array);
    return ret;
}


/**************************************************************************************************/
/** @fcn        static void spin_task(void *arg)
 *  @brief      x
 *  @details    x
 *
 *  @param    [in]  (void *) arg - ?
 */
/**************************************************************************************************/
static void spin_task(void *arg) {
	
    xSemaphoreTake(sync_spin_task, portMAX_DELAY);
    
    for(;;) {
        //Consume CPU cycles
        for (int i = 0; i < SPIN_ITER; i++) {
            __asm__ __volatile__("NOP");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


/**************************************************************************************************/
/** @fcn        static void stats_task(void *arg)
 *  @brief      x
 *  @details    x
 *
 *  @param    [in]  (void *) arg - ?
 */
/**************************************************************************************************/
static void stats_task(void *arg) {
	
    xSemaphoreTake(sync_stats_task, portMAX_DELAY);

    //Start all the spin tasks
    for (int i = 0; i < NUM_OF_SPIN_TASKS; i++) {
        xSemaphoreGive(sync_spin_task);
    }

    //Print real time stats periodically
    while (1) {
        
        printf("\n\nSweating real time stats over %"PRIu32" ticks\n", STATS_TICKS);
        
        if (print_real_time_stats(STATS_TICKS) == ESP_OK) {
			
            printf("Real time stats obtained\n");
            
        } else {
			
            printf("Error getting real time stats\n");
            
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    return;
}


/**************************************************************************************************/
/** @fcn        int app_main(void)
 *  @brief      x
 *  @details    x
 */
/**************************************************************************************************/
void app_main(void) {
	
    //Allow other core to finish initialization
    vTaskDelay(pdMS_TO_TICKS(100));

    //Create semaphores to synchronize
    sync_spin_task = xSemaphoreCreateCounting(NUM_OF_SPIN_TASKS, 0);
    sync_stats_task = xSemaphoreCreateBinary();

    //Create spin tasks
    for (int i = 0; i < NUM_OF_SPIN_TASKS; i++) {
        snprintf(task_names[i], configMAX_TASK_NAME_LEN, "spin%d", i);
        xTaskCreatePinnedToCore(spin_task, task_names[i], 1024, NULL, SPIN_TASK_PRIO, NULL, tskNO_AFFINITY);
    }

    //Create and start stats task
    xTaskCreatePinnedToCore(stats_task, "stats", 4096, NULL, STATS_TASK_PRIO, NULL, tskNO_AFFINITY);
    xSemaphoreGive(sync_stats_task);
    
    return;
}

