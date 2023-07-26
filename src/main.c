#include "mikesinput.h"

int main(void) {
    mikesinput_init();

    while(true)
    {
        mikesinput_poll_devices();
    }
}
