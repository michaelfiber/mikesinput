#ifndef RCORE_NATIVE_INPUT
#define RCORE_NATIVE_INPUT

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define ERR_BAD_DEV_PATH(path) (printf("[MikesInput] Could not open dev path: %s\n", path))

#define BITS_PER_LONG (8 * sizeof(long))
#define NBITS(x) ((((x)-1) / BITS_PER_LONG) + 1)
#define OFF(x) ((x) % BITS_PER_LONG)
#define BIT(x) (1UL << OFF(x))
#define LONG(x) ((x) / BITS_PER_LONG)
#define IS_BIT_SET(array, bit) ((array[LONG(bit)] >> OFF(bit)) & 1)

#define MIKESINPUT_DEV_PATH "/dev/input"
#define MAX_FILEPATH_LENGTH 1024

typedef struct
{
    int fd;

    bool has_abs;
    bool has_rel;
    bool has_abs_multi;

    // Used for absolute input events
    float x_min;
    float x_max;
    float y_min;
    float y_max;
} mikesinput_dev;

mikesinput_dev devices[64] = {0};

void mikesinput_init(void);
mikesinput_dev *mikesinput_init_device(char *path);

/**
 * Many devices (power buttons, webcams) can show up as keyboards.
 * Devices that show up as js# or mouse# also show up as event#.
 * So just go through all the event# devices and handle their
 * specific features detected by:
 *
 *     ioctl(fd, EVIOCG[BIT|ABS]([EV_REL|EV_ABS|EV_KEY], ...)...);
 *
 */
void mikesinput_init(void)
{
    char path[MAX_FILEPATH_LENGTH] = {0};
    DIR *dir = NULL;
    struct dirent *ent = NULL;

    dir = opendir(MIKESINPUT_DEV_PATH);

    if (dir)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            if ((strncmp("event", ent->d_name, strlen("event")) == 0))
            {
                sprintf(path, "%s/%s", MIKESINPUT_DEV_PATH, ent->d_name);
                mikesinput_dev *device = mikesinput_init_device(path);
                if (device != NULL) 
                {
                    printf("Found input device: %d\n", device->fd);
                    printf("Has abs: %d\nHas rel: %d\nHas abs multi: %d\n", device->has_abs, device->has_rel, device->has_abs_multi);
                }
            }
        }
    }
}

mikesinput_dev *mikesinput_init_device(char *path)
{
    mikesinput_dev *device = (mikesinput_dev *)calloc(1, sizeof(mikesinput_dev));

    unsigned long fd_bits[NBITS(EV_MAX)] = {0};

    device->fd = open(path, O_RDONLY | O_NONBLOCK);
    if (device->fd < 0)
    {
        ERR_BAD_DEV_PATH(path);
        free(device);
        return NULL;
    }

    ioctl(device->fd, EVIOCGBIT(0, sizeof(fd_bits)), fd_bits);

    // Handle absolute position input features.
    if (IS_BIT_SET(fd_bits, EV_ABS))
    {
        struct input_absinfo abs_info = {0};
        unsigned long abs_bits[NBITS(ABS_MAX)] = {0};

        ioctl(device->fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits);
        device->has_abs = IS_BIT_SET(abs_bits, ABS_X) && IS_BIT_SET(abs_bits, ABS_Y);
        device->has_abs_multi = IS_BIT_SET(abs_bits, ABS_MT_POSITION_X) && IS_BIT_SET(abs_bits, ABS_MT_POSITION_Y);

        if (device->has_abs || device->has_abs_multi)
        {
            // set absolute x input min and max
            ioctl(device->fd, EVIOCGABS(ABS_X), &abs_info);
            device->x_min = abs_info.minimum;
            device->x_max = abs_info.maximum;

            // set absolute y input min and max
            ioctl(device->fd, EVIOCGABS(ABS_Y), &abs_info);
            device->y_min = abs_info.minimum;
            device->y_max = abs_info.maximum;
        }
    }

    // Handle relative position input features.
    if (IS_BIT_SET(fd_bits, EV_REL))
    {
        unsigned long rel_bits[NBITS(ABS_MAX)] = {0};
        ioctl(device->fd, EVIOCGBIT(EV_REL, sizeof(rel_bits)), rel_bits);
        device->has_rel = IS_BIT_SET(rel_bits, REL_X) && IS_BIT_SET(rel_bits, REL_Y);
    }

    // Handle key input features.
    if (IS_BIT_SET(fd_bits, EV_KEY))
    {
        unsigned long key_bits[NBITS(ABS_MAX)] = {0};
        ioctl(device->fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits);
    }

    if (device->has_abs || device->has_rel || device->has_abs_multi) {
        return device;
    }

    free(device);
    return NULL;
}

#endif