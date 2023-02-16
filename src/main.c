#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>

#define WAIT 1

void pulse_high(struct gpiod_line *line)
{
    gpiod_line_set_value(line, 1);
    usleep(WAIT);
    gpiod_line_set_value(line, 0);
    usleep(WAIT);
}

void send_command(struct gpiod_line *PGC, struct gpiod_line *PGD, uint16_t cmd) {
    int i;
    
    // Send 6 clock pulses
    for (i = 0; i < 6; i++) {
        gpiod_line_set_value(PGD, (cmd >> i) & 1);
        pulse_high(PGC);
    }
}

uint16_t read_data(struct gpiod_line *PGC, struct gpiod_line *PGD) {
    uint16_t cmd = 0x04; // Command to read data
    uint16_t data = 0;
    int i;

    send_command(PGC, PGD, cmd);

    gpiod_line_set_direction_input(PGD);

    // Read data, LSB first
    for (i = 0; i < 16; i++) {
        pulse_high(PGC);
        data |= (gpiod_line_get_value(PGD)) << i;
    }

    gpiod_line_set_direction_output(PGD, 0);

    return data;
}

void dump_program_memory(struct gpiod_line *PGC, struct gpiod_line *PGD) {
    int i;
    uint16_t address = 0x0;
    uint16_t data;
    uint16_t cmd;
    
    // Send reset address command
    cmd = 0x16;
    send_command(PGC, PGD, cmd);
    
    // Loop through each address
    for (i = 0; i < 0x1000; i++) {
        // Read data
        data = read_data(PGC, PGD);
        printf("0x%04X: 0x%04X\n", address, data);

        // Send increment address command
        cmd = 0x06;
        send_command(PGC, PGD, cmd);
        address++;
    }
}

void enter_LVP(struct gpiod_line *MCLR, struct gpiod_line *PGC, struct gpiod_line *PGD)
{
    int i;

    // Drive MCLR to VIL
    gpiod_line_set_value(MCLR, 0);

    // Wait 250 microseconds
    usleep(250);

    // Enter the LVP key sequence
    uint32_t lvp_key = 0x4D434850;

    for (i = 0; i < 32; i++) {
        gpiod_line_set_value(PGD, (lvp_key >> i) & 1);
        pulse_high(PGC);
    }

    // Drive MCLR to VDD
    gpiod_line_set_value(MCLR, 1);
}

void exit_LVP(struct gpiod_line *MCLR)
{
    gpiod_line_set_value(MCLR, 0);
}

int main(void) {
    // Initialize the gpiod library
    struct gpiod_chip *chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) {
        perror("Error opening chip");
        return -1;
    }

    struct gpiod_line *mclr = gpiod_chip_get_line(chip, 4);
    struct gpiod_line *pgc = gpiod_chip_get_line(chip, 17);
    struct gpiod_line *pgd = gpiod_chip_get_line(chip, 22);
    gpiod_line_request_output(mclr, "mclr", 0);
    gpiod_line_request_output(pgc, "pgc", 0);
    gpiod_line_request_output(pgd, "pgd", 0);

    // Enter LVP and read program memory
    enter_LVP(mclr, pgc, pgd);
    dump_program_memory(pgc, pgd);
    exit_LVP(mclr);

    // Release the lines
    gpiod_line_release(mclr);
    gpiod_line_release(pgd);
    gpiod_line_release(pgc);

    // Close the chip
    gpiod_chip_close(chip);

    return 0;
}
