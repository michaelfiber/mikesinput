#ifndef MIKESINPUT_H
#define MIKESINPUT_H

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/*
    Joystick axis types:

        ABS_X           x axis
        ABS_Y           y axis
        ABS_Z           z axis
        ABS_RX          x rotational axis
        ABS_RY          y rotational axis
        ABS_RZ          z rotational axis
        ABS_THROTTLE    throttle
        ABS_RUDDER      rudder
        ABS_WHEEL       wheel
        ABS_GAS         gas
        ABS_BRAKE       brake
        ABS_HAT0X       hat 0 x axis
        ABS_HAT0Y       hat 0 y axis
        ABS_HAT1X       hat 1 x axis
        ABS_HAT1Y       hat 1 y axis
        ABS_HAT2X       hat 2 x axis
        ABS_HAT2Y       hat 2 y axis
        ABS_HAT3X       hat 3 x axis
        ABS_HAT3Y       hat 3 y axis
        ABS_PRESSURE    pressure
        ABS_DISTANCE    distance
        ABS_TILT_X      tilt x
        ABS_TILT_Y      tilt y
        ABS_MISC        miscellaneous

        BTN_JOYSTICK    joystick button?
*/

void printAxisType(int i)
{
    switch (i)
    {
    case ABS_X:
        printf(" (X Axis) ");
        break;
    case ABS_Y:
        printf(" (Y Axis) ");
        break;
    case ABS_Z:
        printf(" (Z Axis) ");
        break;
    case ABS_RX:
        printf(" (X Rotation Axis) ");
        break;
    case ABS_RY:
        printf(" (Y Rotation Axis) ");
        break;
    case ABS_RZ:
        printf(" (Z Rotation Axis) ");
        break;
    case ABS_THROTTLE:
        printf(" (Throttle) ");
        break;
    case ABS_RUDDER:
        printf(" (Rudder) ");
        break;
    case ABS_WHEEL:
        printf(" (Wheel) ");
        break;
    case ABS_GAS:
        printf(" (Accelerator) ");
        break;
    case ABS_BRAKE:
        printf(" (Brake) ");
        break;
    case ABS_HAT0X:
        printf(" (Hat zero, x axis) ");
        break;
    case ABS_HAT0Y:
        printf(" (Hat zero, y axis) ");
        break;
    case ABS_HAT1X:
        printf(" (Hat one, x axis) ");
        break;
    case ABS_HAT1Y:
        printf(" (Hat one, y axis) ");
        break;
    case ABS_HAT2X:
        printf(" (Hat two, x axis) ");
        break;
    case ABS_HAT2Y:
        printf(" (Hat two, y axis) ");
        break;
    case ABS_HAT3X:
        printf(" (Hat three, x axis) ");
        break;
    case ABS_HAT3Y:
        printf(" (Hat three, y axis) ");
        break;
    case ABS_PRESSURE:
        printf(" (Pressure) ");
        break;
    case ABS_DISTANCE:
        printf(" (Distance) ");
        break;
    case ABS_TILT_X:
        printf(" (Tilt, X axis) ");
        break;
    case ABS_TILT_Y:
        printf(" (Tilt, Y axis) ");
        break;
    case ABS_MISC:
        printf(" (Miscellaneous) ");
        break;
    default:
        printf(" (Unknown absolute feature) ");
    }
}

#define MI_ERR_BAD_DEV_PATH(path) (printf("[MikesInput] Could not open dev path: %s\n", path))

#define MI_BITS_PER_LONG (8 * sizeof(long))
#define MI_NBITS(x) ((((x)-1) / MI_BITS_PER_LONG) + 1)
#define MI_OFF(x) ((x) % MI_BITS_PER_LONG)
#define MI_BIT(x) (1UL << MI_OFF(x))
#define MI_LONG(x) ((x) / MI_BITS_PER_LONG)
#define MI_IS_BIT_SET(array, bit) ((array[MI_LONG(bit)] >> MI_OFF(bit)) & 1)

#define MI_DEV_PATH "/dev/input/by-id"
#define MI_MAX_FILEPATH_LENGTH 1024

#define MI_JOYSTICK_MAX 10
#define MI_MOUSE_MAX 10
#define MI_KEYBOARD_MAX 10

typedef struct
{
    bool is_active;
    int fd;
    // Each joystick can have up to ABS_MAX axes.
    bool axis_is_active[ABS_MAX];
    struct input_absinfo axis[ABS_MAX];
    bool buttons[KEY_MAX];
} mikesinput_joystick;

// storage of up to 10 joysticks.
static mikesinput_joystick mikesinput_joysticks[MI_JOYSTICK_MAX];

void mikesinput_init(void);
void mikesinput_poll(void);

static int mikesinput_init_joystick(char *path);
static int mikesinput_init_mouse(char *path);
static int mikesinput_init_keyboard(char *path);

static int mikesinput_poll_joysticks(void);
static int mikesinput_poll_mice(void);
static int mikesinput_poll_keyboards(void);

/**
 * Use /dev/input/by-id to find devices, use the device name suffix to determine the type of device.
 */
void mikesinput_init(void)
{
    char path[MI_MAX_FILEPATH_LENGTH] = {0};
    DIR *dir = NULL;
    struct dirent *ent = NULL;

    dir = opendir(MI_DEV_PATH);

    if (dir)
    {
        int js_len = strlen("event-joystick");
        int kb_len = strlen("event-kbd");
        int ms_len = strlen("event-mouse");

        while ((ent = readdir(dir)) != NULL)
        {
            sprintf(path, "%s/%s", MI_DEV_PATH, ent->d_name);

            if ((strncmp("event-joystick", ent->d_name + (strlen(ent->d_name) - js_len), js_len) == 0))
            {
                printf("%s is a joystick\n", path);
                mikesinput_init_joystick(path);
            }
            else if ((strncmp("event-kbd", ent->d_name + (strlen(ent->d_name) - kb_len), kb_len) == 0))
            {
                printf("%s is a keyboard\n", path);
            }
            else if ((strncmp("event-mouse", ent->d_name + (strlen(ent->d_name) - ms_len), ms_len) == 0))
            {
                printf("%s is a mouse\n", path);
            }
        }
    }
}

void mikesinput_poll(void)
{
    mikesinput_poll_joysticks();
    mikesinput_poll_keyboards();
    mikesinput_poll_mice();

    for (int i = 0; i < MI_JOYSTICK_MAX; i++)
    {
        if (!mikesinput_joysticks[i].is_active)
            continue;

        for (int j = 0; j < ABS_MAX; j++)
        {
            if (!mikesinput_joysticks[i].axis_is_active[j])
                continue;

            printAxisType(j);
            printf("%d ", mikesinput_joysticks[i].axis[j].value);
        }

        printf("\n");

        for (int j = 0; j < KEY_MAX; j++)
        {
            printf("%d", mikesinput_joysticks[i].buttons[j]);
        }

        printf("\n");
    }
}

static int mikesinput_init_joystick(char *path)
{
    for (int i = 0; i < MI_JOYSTICK_MAX; i++)
    {
        if (mikesinput_joysticks[i].is_active)
            continue;

        struct input_absinfo features;
        unsigned long abs_bits[ABS_MAX / 8 + 1];
        int fd = -1;

        memset(abs_bits, 0, sizeof(abs_bits));

        if ((mikesinput_joysticks[i].fd = open(path, O_RDONLY | O_NONBLOCK)) < 0)
        {
            return -1;
        }

        if (ioctl(mikesinput_joysticks[i].fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits) < 0)
        {
            close(mikesinput_joysticks[i].fd);
            return -1;
        }

        for (int axis = 0; axis < ABS_MAX; axis++)
        {
            if (MI_IS_BIT_SET(abs_bits, axis))
            {
                printf("  Found axis %d", axis);
                printAxisType(axis);
                printf("\n");

                if (ioctl(mikesinput_joysticks[i].fd, EVIOCGABS(axis), &features) == 0)
                {
                    printf("    Min: %d\n", features.minimum);
                    printf("    Val: %d\n", features.value);
                    printf("    Max: %d\n", features.maximum);
                    printf("    Resolution: %d\n", features.resolution);
                    printf("    Deadzone: %.2f%%\n", (float)features.flat * 100.0f / (float)features.maximum);
                }

                mikesinput_joysticks[i].axis_is_active[axis] = true;
            }
        }

        mikesinput_joysticks[i].is_active = true;

        return 0;
    }

    return -1;
}

static int mikesinput_init_mouse(char *path)
{
}

static int mikesinput_init_keyboard(char *path)
{
}

static int mikesinput_poll_joysticks(void)
{
    struct input_event ev;

    int count = 0;
    for (int i = 0; i < MI_JOYSTICK_MAX; i++)
    {
        if (!mikesinput_joysticks[i].is_active)
            continue;

        count++;

        while (read(mikesinput_joysticks[i].fd, &ev, sizeof(struct input_event)) > 0)
        {
            switch (ev.type)
            {
            case EV_SYN:
                return;
            case EV_KEY:
                mikesinput_joysticks[i].buttons[ev.code] = ev.value;
                break;
            case EV_ABS:
                mikesinput_joysticks[i].axis[ev.code].value = ev.value;
                break;
            }
        }
    }
}

static int mikesinput_poll_mice(void)
{
    struct input_event ev;
}

static int mikesinput_poll_keyboards(void)
{
}

#endif // MIKESINPUT_H