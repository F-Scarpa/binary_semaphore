#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"                 
#include "driver/gpio.h"   
#include "freertos/semphr.h"            



#define brownWire 21

//binary semaphore = 1 listener always active
                    // 1 receiver active on trigger
                    // 1 actuator per task
                    

adc_oneshot_unit_handle_t adc_handle;
SemaphoreHandle_t binSemaphore; 


void task_ListenForpotVal(void* params)
{
    int val = 0;
    bool lowVal = false;
    while(true)
    {
        adc_oneshot_read(adc_handle, ADC_CHANNEL_6, &val);
        printf("%d\n",val);


        if (val >= 2000 && !lowVal)
        {
            xSemaphoreGive(binSemaphore);
            lowVal = true;
            printf("Call\n");
        }
        else if (val < 2000)
        {
           lowVal = false;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void lightUpLED(void* params)
{   
    bool ledState = false;
    while(true)
    {
        xSemaphoreTake(binSemaphore, portMAX_DELAY);
        ledState = !ledState;
        gpio_set_level(brownWire, ledState);
    }

}



void app_main(void)
{   
    gpio_set_direction(brownWire, GPIO_MODE_OUTPUT );  

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE               
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    // Channel config (ADC1_CHANNEL_6 = GPIO34)
    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_12,      // 0–4095
        .atten = ADC_ATTEN_DB_12,           // 0 to ~3.9V
    };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_6, &channel_config);


      
    printf("start program\n");       

    binSemaphore = xSemaphoreCreateBinary();


    xTaskCreate(&task_ListenForpotVal,"potVal",2048, NULL, 1, NULL);
    xTaskCreate(&lightUpLED,"turnonLED",2048, NULL, 1, NULL);


     }    
        // in this state the CPU works even while there is no reason to, noone might press the button for a long time
        //may need an interrupt to avoid battery consumption
        
        
    
 