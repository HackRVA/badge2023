#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "badge.h"

#define MAX_COMMAND_LEN 200


#include "cli.h"
#include "cli_flash.h"
#include "cli_led.h"
#include "cli_button.h"
#include "cli_ir.h"

#include "rtc.h"
#include "button.h"
#include "hal/usb.h"
#include "flash_storage.h"
#include "delay.h"
#include "init.h"

int exit_process(char *args) {
    return -1;
}

CLI_COMMAND exit_command = {
    .name = "exit",
    .help = "Exits command loop.",
    .process = exit_process,
};

int help_process(char *args) {
    puts("Available commands are: ");
    puts("\thelp - Print this message");
    puts("\tflash - Write data to nonvolatile storage");
    puts("\tled - Toggle lights on the device");
    puts("\tbutton - Probe button state");
    puts("\tir - Send IR packets and check received data");
    puts("\texit - Exit command loop, triggering a reboot");
    return 0;
}

CLI_COMMAND help_command = {
    .name = "help",
    .help = "Prints out the list of commands.",
    .process = help_process,
};

int app_main() {

    hal_init();
    UserInit();

    if (button_poll(BADGE_BUTTON_LEFT)) {

        // Run debug CLI

        // Need to ensure USB is connected before reading stdin, or else that will hang
        while (!usb_is_connected()) {
            sleep_ms(5);
        }

        CLI_COMMAND root_commands[] = {
                [0] = help_command, // Add your command name to the printout in help_process!
                [1] = exit_command,
                [2] = flash_command,
                [3] = led_command,
                [4] = button_command,
                [5] = ir_command,
                [6] = {}
        };

        cli_run(root_commands);
        puts("Exited CLI, running main app");
    }

    // run main app
    uint64_t frame_time = rtc_get_us_since_boot();
    while (1) {
        uint64_t frame_period_us = ProcessIO();
        uint64_t current_time = rtc_get_us_since_boot();
        if (frame_time + frame_period_us <= current_time) {
            printf("Frame time was long: %llu\n", current_time - frame_time);
            frame_time = current_time;
            continue;
        }

        frame_time = frame_period_us + frame_time;
        sleep_us(frame_time - current_time);
    }
    hal_deinit();
    hal_reboot();

    return 0;
}

#if TARGET_PICO
int main(void) {
    return app_main();
}
#elif TARGET_SIMULATOR
#include <pthread.h>

void *main_in_thread(void* params) {
    app_main();
    return NULL;
}

int main(int argc, char** argv) {
    pthread_t app_thread;
    pthread_create(&app_thread, NULL, main_in_thread, NULL);

    hal_start_gtk(&argc, &argv);

    pthread_kill(app_thread, SIGKILL);
    exit(0);
}

#endif