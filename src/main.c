#include <stdio.h>
#include <stdlib.h>
#include <gpiod.h>

#define PIC_PGC "GPIO17"
#define PIC_PGD "GPIO18"
#define PIC_MCLR "GPIO4"

void pulse_high(struct gpiod_line *line)
{
    gpiod_line_set_value(line, 1);
    usleep(1);
    gpiod_line_set_value(line, 0);
    usleep(1);
}

void pulse_low(struct gpiod_line *line)
{
    gpiod_line_set_value(line, 0);
    usleep(1);
    gpiod_line_set_value(line, 1);
    usleep(1);
}

void send_command(struct gpiod_line *pgc, struct gpiod_line *pgd, unsigned char cmd) {
    gpiod_line_set_value(pgd, cmd & 0x01);
    pulse_high(pgc);
    gpiod_line_set_value(pgd, (cmd >> 1) & 0x01);
    pulse_high(pgc);
    gpiod_line_set_value(pgd, (cmd >> 2) & 0x01);
    pulse_high(pgc);
    gpiod_line_set_value(pgd, (cmd >> 3) & 0x01);
    pulse_high(pgc);
    gpiod_line_set_value(pgd, (cmd >> 4) & 0x01);
    pulse_high(pgc);
    gpiod_line_set_value(pgd, (cmd >> 5) & 0x01);
    pulse_high(pgc);
    gpiod_line_set_value(pgd, (cmd >> 6) & 0x01);
    pulse_high(pgc);
    gpiod_line_set_value(pgd, (cmd >> 7) & 0x01);
    pulse_high(pgc);
}

uint16_t read_data(struct gpiod_line *pgd, struct gpiod_line *pgc) {
    uint16_t data = 0;
    int i;

    // Set the PGC line low to initiate a data read
    gpiod_line_set_value(pgc, 0);
    pulse_low(pgc);

    // Read the most significant 8 bits of data
    for (i = 7; i >= 0; i--) {
        gpiod_line_set_value(pgc, 1);
        pulse_high(pgc);

        data |= (gpiod_line_get_value(pgd) << i);

        gpiod_line_set_value(pgc, 0);
        pulse_low(pgc);
    }

    // Read the least significant 8 bits of data
    for (i = 15; i >= 8; i--) {
        gpiod_line_set_value(pgc, 1);
        pulse_high(pgc);

        data |= (gpiod_line_get_value(pgd) << i);

        gpiod_line_set_value(pgc, 0);
        pulse_low(pgc);
    }

    return data;
}

void dump_program_memory(struct gpiod_line *pgd, struct gpiod_line *pgc) {
    uint16_t address = 0;
    uint16_t data;

    // Send the READ instruction to the PIC16LF1847
    send_instruction(0x4, pgd, pgc);

    // Read the program memory data from the PIC16LF1847
    for (address = 0; address < 0x1000; address++) {
        data = read_data(pgd, pgc);
        printf("0x%04x: 0x%04x\n", address, data);

        // Increment the address for the next memory location
        send_instruction(0x06, pgd, pgc);
    }
}

void enter_LVP(struct gpiod_line *MCLR, struct gpiod_line *PGC, struct gpiod_line *PGD)
{
    // Drive MCLR to VIL
    gpiod_line_set_value(MCLR, 0);

    // Wait 250 microseconds
    usleep(250);

    // Enter the LVP key sequence
    uint32_t lvp_key = 0x4D434C52;

    for (int i = 31; i >= 0; i--) {
        gpiod_line_set_value(PGD, (lvp_key >> i) & 1);
        pulse_high(PGC);
        pulse_low(PGC);
    }

    // Wait 250 microseconds
    usleep(250);
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

    struct gpiod_line *mclr = gpiod_chip_get_line(chip, 17);
    struct gpiod_line *pgd = gpiod_chip_get_line(chip, 18);
    struct gpiod_line *pgc = gpiod_chip_get_line(chip, 22);
    gpiod_line_request_output(mclr, "mclr", 0);
    gpiod_line_request_output(pgd, "pgd", 0);
    gpiod_line_request_output(pgc, "pgc", 0);

    // Enter LVP and read program memory
    enter_LVP(mclr, pgd, pgc);
    dump_program_memory(pgd, pgc);
    exit_LVP(mclr);

    // Release the lines
    gpiod_line_release(mclr);
    gpiod_line_release(pgd);
    gpiod_line_release(pgc);

    // Close the chip
    gpiod_chip_close(chip);

    return 0;
}
