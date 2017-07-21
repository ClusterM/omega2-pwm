/*
 * pwm.c: Simple program control hardware PWM on Omega2
 *
 *  Copyright (C) 2017, Alexey 'Cluster' Avdyukhin (clusterrr@clusterrr.com)
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <inttypes.h>

#define printerr(fmt,...) do { fprintf(stderr, fmt, ## __VA_ARGS__); fflush(stderr); } while(0)

#define PWM_ENABLE 0x10005000        // PWM Enable register
#define PWM0_CON 0x10005010          // PWM0 Control register
#define PWM0_HDURATION 0x10005014    // PWM0 High Duration register
#define PWM0_LDURATION 0x10005018    // PWM0 Low Duration register
#define PWM0_GDURATION 0x1000501C    // PWM0 Guard Duration register
#define PWM0_SEND_DATA0 0x10005030   // PWM0 Send Data0 register
#define PWM0_SEND_DATA1 0x10005034   // PWM0 Send Data1 register
#define PWM0_WAVE_NUM 0x10005038     // PWM0 Wave Number register
#define PWM0_DATA_WIDTH 0x1000503C   // PWM0 Data Width register
#define PWM0_THRESH 0x10005040       // PWM0 Thresh register
#define PWM0_SEND_WAVENUM 0x10005044 // PWM0 Send Wave Number register

uint32_t devmem(uint32_t target, uint8_t size, uint8_t write, uint32_t value)
{
    int fd;
    void *map_base, *virt_addr;
    uint32_t pagesize = (unsigned)getpagesize(); /* or sysconf(_SC_PAGESIZE)  */
    uint32_t map_size = pagesize;
    uint32_t offset;
    uint32_t result;

    offset = (unsigned int)(target & (pagesize-1));
    if (offset + size > pagesize ) {
        // Access straddles page boundary:  add another page:
        map_size += pagesize;
    }

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        printerr("Error opening /dev/mem (%d) : %s\n", errno, strerror(errno));
        exit(1);
    }

    map_base = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                    fd, 
                    target & ~((typeof(target))pagesize-1));
    if (map_base == (void *) -1) {
        printerr("Error mapping (%d) : %s\n", errno, strerror(errno));
        exit(1);
    }

    virt_addr = map_base + offset;

    if (write)
    {
        switch (size) {
            case 1:
                *((volatile uint8_t *) virt_addr) = value;
                break;
            case 2:
                *((volatile uint16_t *) virt_addr) = value;
                break;
            case 4:
                *((volatile uint32_t *) virt_addr) = value;
                break;
        }
    }    

    switch (size) {
        case 1:
            result = *((volatile uint8_t *) virt_addr);
            break;
        case 2:
            result = *((volatile uint16_t *) virt_addr);
            break;
        case 4:
            result = *((volatile uint32_t *) virt_addr);
            break;
    }

    if (munmap(map_base, map_size) != 0) {
        printerr("ERROR munmap (%d) %s\n", errno, strerror(errno));
        exit(1);
    }
    close(fd);

    return result;
}

static void usage(const char *cmd)
{
    fprintf(stderr, 
        "\nUsage:\t%s pwm <channel> <frequency> [duty]\n",
        cmd);
}

static void pwm(uint8_t channel, uint32_t freq, uint8_t duty)
{
    uint32_t enable = devmem(PWM_ENABLE, 4, 0, 0);

    enable &= ~((uint32_t)(1<<channel));
    devmem(PWM_ENABLE, 4, 1, enable);

    // Need to disable PWM?
    if (!freq)
        return;

    uint32_t reg_offset = 0x40 * channel;

    uint32_t duration = 100000 / freq;
    uint32_t duration0 = duration * duty / 100;
    uint32_t duration1 = duration * (100-duty) / 100;

    devmem(PWM0_CON + reg_offset, 4, 1, 0x7000);
    devmem(PWM0_HDURATION + reg_offset, 4, 1, duration1 - 1);
    devmem(PWM0_LDURATION + reg_offset, 4, 1, duration0 - 1);
    devmem(PWM0_GDURATION + reg_offset, 4, 1, duration / 2);
    devmem(PWM0_SEND_DATA0 + reg_offset, 4, 1, 0x55555555);
    devmem(PWM0_SEND_DATA1 + reg_offset, 4, 1, 0x55555555);
    devmem(PWM0_WAVE_NUM + reg_offset, 4, 1, 0);

    enable |= 1<<channel;
    devmem(PWM_ENABLE, 4, 1, enable);
}

int main(int argc, char **argv)
{
    const char *progname = argv[0];
    if (argc < 3)
    {
        usage(progname);
        exit(2);
    }

    uint8_t channel;
    uint32_t freq;
    uint8_t duty = 50;

    if ((sscanf(argv[1], "%d", &channel) != 1) || (channel > 3))
    {
        fprintf(stderr, "Invalid channel number\n");
        exit(1);
    }

    if (sscanf(argv[2], "%d", &freq) != 1)
    {
        fprintf(stderr, "Invalid frequency number\n");
        exit(1);
    }

    if (argc >= 4)
    {
        if ((sscanf(argv[3], "%d", &duty) != 1) || (duty > 100))
        {
            fprintf(stderr, "Invalid duty number\n");
            exit(1);
        }
    }

    pwm(channel, freq, duty);

    return 0;
}
