#include <zephyr/kernel.h>
#include "mqtt_messenger.h"

int main() {
    int ret;
    
    ret = mqtt_messenger_init();
    printk("Main return: %d\n", ret);

    return 0;
}
