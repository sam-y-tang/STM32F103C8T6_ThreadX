/* This is a small demo of the high-performance ThreadX kernel.  It includes examples of eight
   threads of different priorities, using a message queue, semaphore, mutex, event flags group,
   byte pool, and block pool.  */

#include "tx_api.h"
#include "miniprintf.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#define DEMO_STACK_SIZE 1024
#define DEMO_BYTE_POOL_SIZE 9120
#define DEMO_BLOCK_POOL_SIZE 100
#define DEMO_QUEUE_SIZE 100
/* Define the ThreadX object control blocks...  */

TX_THREAD thread_0;
TX_THREAD thread_1;
TX_THREAD thread_2;
TX_THREAD thread_3;
TX_THREAD thread_4;
TX_THREAD thread_5;
TX_THREAD thread_6;
TX_THREAD thread_7;
TX_QUEUE queue_0;
TX_SEMAPHORE semaphore_0;
TX_MUTEX mutex_0;
TX_MUTEX mutex_uart;
TX_EVENT_FLAGS_GROUP event_flags_0;
TX_BYTE_POOL byte_pool_0;
TX_BLOCK_POOL block_pool_0;
UCHAR memory_area[DEMO_BYTE_POOL_SIZE];

/* Define the counters used in the demo application...  */

ULONG thread_0_counter;
ULONG thread_1_counter;
ULONG thread_1_messages_sent;
ULONG thread_2_counter;
ULONG thread_2_messages_received;
ULONG thread_3_counter;
ULONG thread_4_counter;
ULONG thread_5_counter;
ULONG thread_6_counter;
ULONG thread_7_counter;

static int uart_printf(const char *format, ...) __attribute((format(printf, 1, 2)));

static void
init_clock(void)
{

    // rcc_clock_setup_in_hse_8mhz_out_24mhz();
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_24MHZ]);
    rcc_periph_clock_enable(RCC_GPIOC);

    // Clock for GPIO port A: GPIO_USART1_TX, USART1
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART1);
}

static void
init_usart(void)
{

    //////////////////////////////////////////////////////////////
    // STM32F103C8T6:
    //	 UART RX:	 A9  <-->TXD
    //	 UART TX:	 A10 <-->RXD
    //	CTS:	A11 (not used)
    //	RTS:	A12 (not used)
    //	Baud:	115200
    //////////////////////////////////////////////////////////////

    // GPIO_USART1_TX/GPIO13 on GPIO port A for tx
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);

    usart_set_baudrate(USART1, 115200);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_enable(USART1);
}

static void
init_gpio(void)
{
    // C.GPIO13:
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
}

static inline void
uart_putc(char ch)
{
    usart_send_blocking(USART1, ch);
}

static int
uart_printf(const char *format, ...)
{
    va_list args;
    int rc;

    va_start(args, format);

    tx_mutex_get(&mutex_uart, TX_WAIT_FOREVER);
    rc = mini_vprintf_cooked(uart_putc, format, args);
    tx_mutex_put(&mutex_uart);

    va_end(args);
    return rc;
}

/* Define thread prototypes.  */

void thread_0_entry(ULONG thread_input);
void thread_1_entry(ULONG thread_input);
void thread_2_entry(ULONG thread_input);
void thread_3_and_4_entry(ULONG thread_input);
void thread_5_entry(ULONG thread_input);
void thread_6_and_7_entry(ULONG thread_input);

/* Define main entry point.  */

int main(void)
{

    init_clock();
    init_gpio();
    init_usart();

    /* Enter the ThreadX kernel.  */
    tx_kernel_enter();
}

/* Define what the initial system looks like.  */

void tx_application_define(void *first_unused_memory)
{

    CHAR *pointer = TX_NULL;
    (void)first_unused_memory;

    /* Create a byte memory pool from which to allocate the thread stacks.  */
    tx_byte_pool_create(&byte_pool_0, "byte pool 0", memory_area, DEMO_BYTE_POOL_SIZE);

    /* Put system definition stuff in here, e.g. thread creates and other assorted
       create information.  */

    /* Allocate the stack for thread 0.  */
    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create the main thread.  */
    tx_thread_create(&thread_0, "thread 0", thread_0_entry, 0,
                     pointer, DEMO_STACK_SIZE,
                     1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for thread 1.  */
    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create threads 1 and 2. These threads pass information through a ThreadX
       message queue.  It is also interesting to note that these threads have a time
       slice.  */
    tx_thread_create(&thread_1, "thread 1", thread_1_entry, 1,
                     pointer, DEMO_STACK_SIZE,
                     16, 16, 4, TX_AUTO_START);

    /* Allocate the stack for thread 2.  */
    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_thread_create(&thread_2, "thread 2", thread_2_entry, 2,
                     pointer, DEMO_STACK_SIZE,
                     16, 16, 4, TX_AUTO_START);

    /* Allocate the stack for thread 3.  */
    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create threads 3 and 4.  These threads compete for a ThreadX counting semaphore.
       An interesting thing here is that both threads share the same instruction area.  */
    tx_thread_create(&thread_3, "thread 3", thread_3_and_4_entry, 3,
                     pointer, DEMO_STACK_SIZE,
                     8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for thread 4.  */
    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_thread_create(&thread_4, "thread 4", thread_3_and_4_entry, 4,
                     pointer, DEMO_STACK_SIZE,
                     8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for thread 5.  */
    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create thread 5.  This thread simply pends on an event flag which will be set
       by thread_0.  */
    tx_thread_create(&thread_5, "thread 5", thread_5_entry, 5,
                     pointer, DEMO_STACK_SIZE,
                     4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for thread 6.  */
    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create threads 6 and 7.  These threads compete for a ThreadX mutex.  */
    tx_thread_create(&thread_6, "thread 6", thread_6_and_7_entry, 6,
                     pointer, DEMO_STACK_SIZE,
                     8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for thread 7.  */
    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_thread_create(&thread_7, "thread 7", thread_6_and_7_entry, 7,
                     pointer, DEMO_STACK_SIZE,
                     8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the message queue.  */
    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_QUEUE_SIZE * sizeof(ULONG), TX_NO_WAIT);

    /* Create the message queue shared by threads 1 and 2.  */
    tx_queue_create(&queue_0, "queue 0", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE * sizeof(ULONG));

    /* Create the semaphore used by threads 3 and 4.  */
    tx_semaphore_create(&semaphore_0, "semaphore 0", 1);

    /* Create the event flags group used by threads 1 and 5.  */
    tx_event_flags_create(&event_flags_0, "event flags 0");

    /* Create the mutex used by thread 6 and 7 without priority inheritance.  */
    tx_mutex_create(&mutex_0, "mutex 0", TX_NO_INHERIT);

    /* Create the mutex used by uart */
    tx_mutex_create(&mutex_uart, "mutex uart", TX_NO_INHERIT);

    /* Allocate the memory for a small block pool.  */
    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_BLOCK_POOL_SIZE, TX_NO_WAIT);

    /* Create a block memory pool to allocate a message buffer from.  */
    tx_block_pool_create(&block_pool_0, "block pool 0", sizeof(ULONG), pointer, DEMO_BLOCK_POOL_SIZE);

    /* Allocate a block and release the block memory.  */
    tx_block_allocate(&block_pool_0, (VOID **)&pointer, TX_NO_WAIT);

    /* Release the block back to the pool.  */
    tx_block_release(pointer);
}

/* Define the test threads.  */

void thread_0_entry(ULONG thread_input)
{
    (void)thread_input;

    UINT status;

    /* This thread simply sits in while-forever-sleep loop.  */
    while (1)
    {

        /* Increment the thread counter.  */
        thread_0_counter++;

        gpio_toggle(GPIOC, GPIO13);
        uart_printf("thread 0 count= %d\n", (int)thread_0_counter);

        /* Sleep for 10 ticks.  */
        tx_thread_sleep(100);

        /* Set event flag 0 to wakeup thread 5.  */
        status = tx_event_flags_set(&event_flags_0, 0x1, TX_OR);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;
    }
}

void thread_1_entry(ULONG thread_input)
{

    UINT status;

    (void)thread_input;

    /* This thread simply sends messages to a queue shared by thread 2.  */
    while (1)
    {

        /* Increment the thread counter.  */
        thread_1_counter++;
        uart_printf("thread 1 count= %d\n", (int)thread_1_counter);

        /* Send message to queue 0.  */
        status = tx_queue_send(&queue_0, &thread_1_messages_sent, TX_WAIT_FOREVER);

        /* Check completion status.  */
        if (status != TX_SUCCESS)
            break;

        /* Increment the message sent.  */
        thread_1_messages_sent++;
        uart_printf("thread 1 message sentt= %d\n", (int)thread_1_messages_sent);
    }
}

void thread_2_entry(ULONG thread_input)
{

    ULONG received_message;
    UINT status;
    (void)thread_input;

    /* This thread retrieves messages placed on the queue by thread 1.  */
    while (1)
    {

        /* Increment the thread counter.  */
        thread_2_counter++;

        /* Retrieve a message from the queue.  */
        status = tx_queue_receive(&queue_0, &received_message, TX_WAIT_FOREVER);

        /* Check completion status and make sure the message is what we
           expected.  */
        if ((status != TX_SUCCESS) || (received_message != thread_2_messages_received))
            break;

        /* Otherwise, all is okay.  Increment the received message count.  */
        thread_2_messages_received++;
        uart_printf("thread 2 message received= %d\n", (int)thread_2_messages_received);
    }
}

void thread_3_and_4_entry(ULONG thread_input)
{

    UINT status;

    /* This function is executed from thread 3 and thread 4.  As the loop
       below shows, these function compete for ownership of semaphore_0.  */
    while (1)
    {

        /* Increment the thread counter.  */
        if (thread_input == 3)
        {
            thread_3_counter++;
            uart_printf("thread 1 count= %d\n", (int)thread_1_counter);
        }
        else
        {
            thread_4_counter++;
            uart_printf("thread 1 count= %d\n", (int)thread_1_counter);
        }

        /* Get the semaphore with suspension.  */
        status = tx_semaphore_get(&semaphore_0, TX_WAIT_FOREVER);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;

        /* Sleep for 2 ticks to hold the semaphore.  */
        tx_thread_sleep(2);

        /* Release the semaphore.  */
        status = tx_semaphore_put(&semaphore_0);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;
    }
}

void thread_5_entry(ULONG thread_input)
{

    UINT status;
    ULONG actual_flags;
    (void)thread_input;

    /* This thread simply waits for an event in a forever loop.  */
    while (1)
    {

        /* Increment the thread counter.  */
        thread_5_counter++;
        uart_printf("thread 5 count= %d\n", (int)thread_5_counter);

        /* Wait for event flag 0.  */
        status = tx_event_flags_get(&event_flags_0, 0x1, TX_OR_CLEAR,
                                    &actual_flags, TX_WAIT_FOREVER);

        /* Check status.  */
        if ((status != TX_SUCCESS) || (actual_flags != 0x1))
            break;
    }
}

void thread_6_and_7_entry(ULONG thread_input)
{

    UINT status;

    (void)thread_input;

    /* This function is executed from thread 6 and thread 7.  As the loop
       below shows, these function compete for ownership of mutex_0.  */
    while (1)
    {

        /* Increment the thread counter.  */
        if (thread_input == 6)
        {
            thread_6_counter++;
            uart_printf("thread 6 count= %d\n", (int)thread_6_counter);
        }
        else
        {
            thread_7_counter++;
            uart_printf("thread 7 count= %d\n", (int)thread_7_counter);
        }

        /* Get the mutex with suspension.  */
        status = tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;

        /* Get the mutex again with suspension.  This shows
           that an owning thread may retrieve the mutex it
           owns multiple times.  */
        status = tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;

        /* Sleep for 2 ticks to hold the mutex.  */
        tx_thread_sleep(2);

        /* Release the mutex.  */
        status = tx_mutex_put(&mutex_0);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;

        /* Release the mutex again.  This will actually
           release ownership since it was obtained twice.  */
        status = tx_mutex_put(&mutex_0);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;
    }
}
