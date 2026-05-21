int main(void) {
    // Basic loop
    while(1) {
        for(volatile int i = 0; i < 50000; i++); // Dumb delay
    }
}