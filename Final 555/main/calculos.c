float calc_astable_freq(float R1, float R2, float C) {
    return 1.44 / ((R1 + 2 * R2) * C);
}

float calc_duty_cycle(float R1, float R2) {
    return ((R1 + R2) / (R1 + 2 * R2)) * 100;
}

float calc_monostable_pulse(float R, float C) {
    return 1.1 * R * C;
}