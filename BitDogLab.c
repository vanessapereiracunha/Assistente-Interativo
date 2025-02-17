#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "ssd1306.h"

#ifndef SSD1306_I2C_ADDR
#define SSD1306_I2C_ADDR 0x3C
#endif

#define SSD1306_I2C_CLOCK 400000
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_BUFFER_LENGTH (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

// Pinos do Joystick (ADC)
const uint JOYSTICK_X = 20;
const uint JOYSTICK_Y = 27;
const uint JOYSTICK_BTN = 22;

// Pinos para funcionalidades adicionais
#define BUZZER_PIN 21   // Pino do buzzer
#define LED_RED_PIN 11  // Pino do LED vermelho
#define LED_GREEN_PIN 13 // Pino do LED verde
#define LED_BLUE_PIN 12 // Pino do LED azul

int selected_item = 0;
const char *menu[] = {
    " Musica",
    "       ",  // Tocar música
    " Led", 
      // Ligar LED
    " Semafaro", // LED piscar em cores
};
const int menu_length = 5;

char selected_message[30] = "";  // Variável global para armazenar a mensagem selecionada

void init_joystick() {
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);

    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);
}

void tocar_musica() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    // Frequências das notas (Dó, Ré, Mi, Fá, Sol)
    uint notas[] = {261, 294, 330, 349, 392};  
    int num_notas = sizeof(notas) / sizeof(notas[0]);

    // Toca cada nota por 500ms
    for (int i = 0; i < num_notas; i++) {
        uint frequency = notas[i]; // Frequência da nota
        pwm_set_wrap(slice_num, frequency);  // Define a frequência do buzzer
        pwm_set_clkdiv(slice_num, 64);       // Prescaler para ajustar a velocidade do PWM
        pwm_set_gpio_level(BUZZER_PIN, frequency / 2); // 50% duty cycle
        pwm_set_enabled(slice_num, true);

        sleep_ms(500); // Toca cada nota por 500ms

        pwm_set_enabled(slice_num, false);
        sleep_ms(100); // Pausa entre as notas
    }

    gpio_set_function(BUZZER_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);
}

void ligar_led() {
    gpio_put(LED_RED_PIN, 1);  // Liga o LED vermelho
    sleep_ms(2000);
    gpio_put(LED_RED_PIN, 0);  // Desliga após 2 segundos
}

void piscar_led_colorido() {
    // Pisca o LED com cores simuladas por LEDs RGB (vermelho, verde e azul)
    
    // LED vermelho
    gpio_put(LED_RED_PIN, 1);  
    gpio_put(LED_GREEN_PIN, 0); 
    gpio_put(LED_BLUE_PIN, 0);
    sleep_ms(500);  
    gpio_put(LED_RED_PIN, 0);  
    sleep_ms(100);

    // LED verde
    gpio_put(LED_RED_PIN, 0);  
    gpio_put(LED_GREEN_PIN, 1);
    gpio_put(LED_BLUE_PIN, 0);
    sleep_ms(500);  
    gpio_put(LED_GREEN_PIN, 0);  
    sleep_ms(100);

    // LED azul
    gpio_put(LED_RED_PIN, 0);  
    gpio_put(LED_GREEN_PIN, 0); 
    gpio_put(LED_BLUE_PIN, 1);
    sleep_ms(500);  
    gpio_put(LED_BLUE_PIN, 0);  
    sleep_ms(100);
}

void executar_acao(int opcao) {
    switch (opcao) {
        case 0:
            tocar_musica();
            break;
        case 1:
            break;
        case 2:
             ligar_led();
            break;
        case 3:
            piscar_led_colorido();
            break;
        default:
            printf("Nenhuma ação definida para esta opção.\n");
    }
}

void navigate_menu() {
    adc_select_input(0);  // Seleciona entrada do joystick
    int x_val = adc_read();

    if (x_val > 3000) {
        selected_item = (selected_item - 1 + menu_length) % menu_length; // Para cima
        sleep_ms(200);
        render_display();
    } else if (x_val < 1000) {
        selected_item = (selected_item + 1) % menu_length; // Para baixo
        sleep_ms(200);
        render_display();
    }

    if (!gpio_get(JOYSTICK_BTN)) {
        sleep_ms(50);  // Debounce
        if (!gpio_get(JOYSTICK_BTN)) {
            printf("Item %d selecionado: %s\n", selected_item + 1, menu[selected_item]);
            snprintf(selected_message, sizeof(selected_message), "Selecionado: %s", menu[selected_item]);

            render_display_selecionado();
            executar_acao(selected_item);
            sleep_ms(2000);
            render_display();
        }
    }
}

void render_display() {
    uint8_t buf[SSD1306_BUFFER_LENGTH] = {0};

    struct render_area frame_area = {
        start_col : 0,
        end_col : SSD1306_WIDTH - 1,
        start_page : 0,
        end_page : SSD1306_NUM_PAGES - 1
    };

    calc_render_area_buflen(&frame_area);

    memset(buf, 0, SSD1306_BUFFER_LENGTH);
    render(buf, &frame_area);

    WriteString(buf, 10, 0, "Menu:");

    int y = 18;
    for (int i = 0; i < menu_length; i++) {
        char option_text[20];

        if (i == selected_item) {
            snprintf(option_text, sizeof(option_text), " X %s", menu[i]);
        } else {
            snprintf(option_text, sizeof(option_text), "  %s", menu[i]);
        }

        WriteString(buf, 5, y, option_text);
        y += 10;
    }

    render(buf, &frame_area);
}

void render_display_selecionado() {
    uint8_t buffer[SSD1306_BUFFER_LENGTH] = {0};

    struct render_area frame_area = {
        start_col : 0,
        end_col : SSD1306_WIDTH - 1,
        start_page : 0,
        end_page : SSD1306_NUM_PAGES - 1
    };

    calc_render_area_buflen(&frame_area);

    memset(buffer, 0, SSD1306_BUFFER_LENGTH);
    render(buffer, &frame_area);

    WriteString(buffer, 0, 0, "Menu:");

    int y = 10;
    if (selected_message[0] != '\0') {
        WriteString(buffer, 0, y + 5, "Selecionado");
        WriteString(buffer, 0, y + 15, menu[selected_item]);
    }

    render(buffer, &frame_area);
}

int main() {
    stdio_init_all();
    i2c_init(i2c1, SSD1306_I2C_CLOCK);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    SSD1306_init();
    init_joystick();

    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);

    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, 0);

    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, 0);

    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_put(LED_BLUE_PIN, 0);

    render_display();

    while (true) {
        navigate_menu();
        sleep_ms(100);
    }

    return 0;
}
