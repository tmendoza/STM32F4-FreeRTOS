#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "math.h"
#include "stdio.h"
#include "stm32f4xx_usart.h"

// Macro to use CCM (Core Coupled Memory) in STM32F4
#define CCM_RAM __attribute__((section(".ccmram")))

#define FPU_TASK_STACK_SIZE 256

StackType_t fpuTaskStack[FPU_TASK_STACK_SIZE] CCM_RAM;  // Put task stack in CCM
StaticTask_t fpuTaskBuffer CCM_RAM;  // Put TCB in CCM

void vApplicationTickHook(void) {
}

/* vApplicationMallocFailedHook() will only be called if
   configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
   function that will get called if a call to pvPortMalloc() fails.
   pvPortMalloc() is called internally by the kernel whenever a task, queue,
   timer or semaphore is created.  It is also called by various parts of the
   demo application.  If heap_1.c or heap_2.c are used, then the size of the
   heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
   FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
   to query the size of free heap space that remains (although it does not
   provide information on how the remaining heap might be fragmented). */
void vApplicationMallocFailedHook(void) {
  taskDISABLE_INTERRUPTS();
  for(;;);
}

/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
   task.  It is essential that code added to this hook function never attempts
   to block in any way (for example, call xQueueReceive() with a block time
   specified, or call vTaskDelay()).  If the application makes use of the
   vTaskDelete() API function (as this demo application does) then it is also
   important that vApplicationIdleHook() is permitted to return to its calling
   function, because it is the responsibility of the idle task to clean up
   memory allocated by the kernel to any task that has since been deleted. */
void vApplicationIdleHook(void) {
}

void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName) {
  (void) pcTaskName;
  (void) pxTask;
  /* Run time stack overflow checking is performed if
     configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
     function is called if a stack overflow is detected. */
  taskDISABLE_INTERRUPTS();
  for(;;);
}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

TaskHandle_t task1_handle = NULL;
TaskHandle_t task2_handle = NULL;
TaskHandle_t task3_handle = NULL;
TaskHandle_t task4_handle = NULL;
TaskHandle_t task5_handle = NULL;


/*
 * Configure USART3(PB10, PB11) to redirect printf data to host PC.
 */
void init_usart3(void) {
  GPIO_InitTypeDef GPIO_InitStruct;
  USART_InitTypeDef USART_InitStruct;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);

  USART_InitStruct.USART_BaudRate = 115200;
  USART_InitStruct.USART_WordLength = USART_WordLength_8b;
  USART_InitStruct.USART_StopBits = USART_StopBits_1;
  USART_InitStruct.USART_Parity = USART_Parity_No;
  USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(USART3, &USART_InitStruct);
  USART_Cmd(USART3, ENABLE);
}

void init_leds(void) {
   // Enable clock for GPIOD (for orange LED)
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

    // Initialization of GPIOD (for orange LED)
    GPIO_InitTypeDef GPIO_InitDef;
    GPIO_InitDef.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitDef.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitDef.GPIO_OType = GPIO_OType_PP;
    GPIO_InitDef.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitDef.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitDef);

}

void usart_putchar(char c)
{
    // Wait until transmit data register is empty
    while (!USART_GetFlagStatus(USART3, USART_FLAG_TXE));
    // Send a char using USART3
    USART_SendData(USART3, c);
}

void usart_putstr(char *s)
{
    // Send a string
    while (*s)
    {
        usart_putchar(*s++);
    }
}

uint16_t usart_getchar()
{
    // Wait until data is received
    while (!USART_GetFlagStatus(USART3, USART_FLAG_RXNE));
    // Read received char
    return USART_ReceiveData(USART3);
}

void my_task1(void *p) {
  TickType_t last_unblock;
  last_unblock = xTaskGetTickCount();

  int count = 0;

  while(1) {
    printf("Hello World Counter: %d\r\n", count++);

    vTaskDelayUntil( &last_unblock, 1000 / portTICK_RATE_MS);

    if(count == 30) {
      vTaskDelete(task1_handle);
    }
  }
}

void my_task2(void *p) {
  TickType_t last_unblock;
  last_unblock = xTaskGetTickCount();

  while(1) {
    GPIO_SetBits(GPIOD, GPIO_Pin_12);
    vTaskDelayUntil( &last_unblock, pdMS_TO_TICKS( 1000 ));
    GPIO_ResetBits(GPIOD, GPIO_Pin_12);
    vTaskDelayUntil( &last_unblock, pdMS_TO_TICKS( 1000 ));
  }
}

void my_task3(void *p) {
  TickType_t last_unblock;
  last_unblock = xTaskGetTickCount();

  while(1) {
    GPIO_SetBits(GPIOD, GPIO_Pin_13);
    vTaskDelayUntil( &last_unblock, pdMS_TO_TICKS( 600 ));
    GPIO_ResetBits(GPIOD, GPIO_Pin_13);
    vTaskDelayUntil( &last_unblock, pdMS_TO_TICKS( 600 ));
  }
}

void my_task4(void *p) {
  TickType_t last_unblock;
  last_unblock = xTaskGetTickCount();

  while(1) {
    GPIO_SetBits(GPIOD, GPIO_Pin_14);
    vTaskDelayUntil( &last_unblock, pdMS_TO_TICKS( 400 ));
    GPIO_ResetBits(GPIOD, GPIO_Pin_14);
    vTaskDelayUntil( &last_unblock, pdMS_TO_TICKS( 400 ));
  }
}

void my_task5(void *p) {
  TickType_t last_unblock;
  last_unblock = xTaskGetTickCount();

  while(1) {
    GPIO_SetBits(GPIOD, GPIO_Pin_15);
    vTaskDelayUntil( &last_unblock, pdMS_TO_TICKS( 200 ));
    GPIO_ResetBits(GPIOD, GPIO_Pin_15);
    vTaskDelayUntil( &last_unblock, pdMS_TO_TICKS( 200 ));
  }
}

int main(void) {

  init_usart3();
  init_leds();

  printf("Creating Tasks...\r\n");
  printf("Launching Counter...\r\n");
  
  xTaskCreate(my_task1, "task1", 200, (void*) 0, 5, &task1_handle);
  xTaskCreate(my_task2, "task2", 200, (void*) 0, 4, &task2_handle);
  xTaskCreate(my_task3, "task3", 200, (void*) 0, 3, &task3_handle);
  xTaskCreate(my_task4, "task4", 200, (void*) 0, 2, &task4_handle);
  xTaskCreate(my_task5, "task5", 200, (void*) 0, 1, &task5_handle);

  printf("Counter Launched.\r\n");
  printf("System Started!\r\n");

  vTaskStartScheduler();

  while(1) {
    /* Add app code here */
  }
}

