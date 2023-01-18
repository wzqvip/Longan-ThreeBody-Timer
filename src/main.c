#include <string.h>

#include "fatfs/tf_card.h"
#include "lcd/lcd.h"

unsigned char image[12800];
FATFS fs;

void init_uart0(void) {
    /* enable GPIO clock */
    rcu_periph_clock_enable(RCU_GPIOA);
    /* enable USART clock */
    rcu_periph_clock_enable(RCU_USART0);

    /* connect port to USARTx_Tx */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
    /* connect port to USARTx_Rx */
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

    /* USART configure */
    usart_deinit(USART0);
    usart_baudrate_set(USART0, 115200U);
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);

    usart_interrupt_enable(USART0, USART_INT_RBNE);
}

void init() {
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOC);
    gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_13);
    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1 | GPIO_PIN_2);

    init_uart0();

    Lcd_Init();  // init OLED
}

int main(void) {
    uint8_t mount_is_ok = 1; /* 0: mount successful ; 1: mount failed */
    int offset = 0;
    FIL fil;
    FRESULT fr; /* FatFs return code */
    UINT br;

    init();

    int hour = 1200, min = 0, sec = 0;
    char timer[20];
    bool flow = FALSE;

    LEDR(1);
    LEDG(1);
    LEDB(1);

    fr = f_mount(&fs, "", 1);
    if (fr == 0) {
        mount_is_ok = 0;
        LCD_Clear(WHITE);
        BACK_COLOR = WHITE;
    } else {
        mount_is_ok = 1;
        LCD_Clear(BLACK);
        BACK_COLOR = BLACK;
    }
    if (mount_is_ok == 0) {
        while (1) {
            offset = 0;
            fr = f_open(&fil, "logo.bin", FA_READ);
            if (fr)
                printf("open error: %d!\n\r", (int)fr);
            f_lseek(&fil, offset);
            fr = f_read(&fil, image, sizeof(image), &br);
            LCD_ShowPicture(0, 0, 159, 39);
            offset += 12800;
            LEDB_TOG;
            f_lseek(&fil, offset);
            fr = f_read(&fil, image, sizeof(image), &br);
            LCD_ShowPicture(0, 40, 159, 79);
            LEDB_TOG;
            delay_1ms(1500);
            f_close(&fil);

            fr = f_open(&fil, "bmp.bin", FA_READ);
            if (fr)
                printf("open error: %d!\n\r", (int)fr);
            offset = 0;

            for (int i = 0; i < 2189; i++) {
                fr = f_read(&fil, image, sizeof(image), &br);
                LCD_ShowPicture(0, 0, 159, 39);
                offset += 12800;
                f_lseek(&fil, offset);
                LEDB_TOG;
                fr = f_read(&fil, image, sizeof(image), &br);
                LCD_ShowPicture(0, 40, 159, 79);
                offset += 12800;
                f_lseek(&fil, offset);
                LEDB_TOG;
            }

            /* Close the file */
            f_close(&fil);
        }
    } else {
        LCD_Clear(BLACK);
        while (1) {
            timer[0] = hour / 1000 + '0';
            timer[1] = (hour % 1000) / 100 + '0';
            timer[2] = (hour % 100) / 10 + '0';
            timer[3] = hour % 10 + '0';
            timer[4] = ':';
            timer[5] = min / 10 + '0';
            timer[6] = min % 10 + '0';
            timer[7] = ':';
            timer[8] = sec / 10 + '0';
            timer[9] = sec % 10 + '0';
            LCD_ShowChar(30, 30, timer[0], 0, WHITE);
            LCD_ShowChar(40, 30, timer[1], 0, WHITE);
            LCD_ShowChar(50, 30, timer[2], 0, WHITE);
            LCD_ShowChar(60, 30, timer[3], 0, WHITE);
            LCD_ShowChar(70, 30, timer[4], 0, WHITE);
            LCD_ShowChar(80, 30, timer[5], 0, WHITE);
            LCD_ShowChar(90, 30, timer[6], 0, WHITE);
            LCD_ShowChar(100, 30, timer[7], 0, WHITE);
            LCD_ShowChar(110, 30, timer[8], 0, WHITE);
            LCD_ShowChar(120, 30, timer[9], 0, WHITE);

            for (int i = 0; i < 10; i++) {
                if (gpio_input_bit_get(GPIOA, GPIO_PIN_8) == 1) {
                    flow = !flow;
                    LEDR_TOG;
                    while (gpio_input_bit_get(GPIOA, GPIO_PIN_8) == 1) {
                        delay_1ms(1);
                    }
                    LEDR_TOG;
                }
                delay_1ms(100);
            }

            if (flow) {
                if (sec <= 0) {
                    sec = 59;
                    if (min <= 0) {
                        min = 59;
                        if (hour <= 0) {
                            continue;
                        } else {
                            hour--;
                        }
                    } else {
                        min--;
                    }
                } else {
                    sec--;
                }
            }
        }
    }
}

int _put_char(int ch) {
    usart_data_transmit(USART0, (uint8_t)ch);
    while (usart_flag_get(USART0, USART_FLAG_TBE) == RESET) {
    }

    return ch;
}
