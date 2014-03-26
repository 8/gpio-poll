/* test tool to send data to tty devices */

#include <stdio.h>
#include <iostream>
#include <termios.h> // needed for implementing set_inferface_attribs()
#include <fcntl.h>   /* needed for O_RDWR, O_NOCTTY, O_NDELAY */
#include <string.h>
#include <getopt.h>  /* for getopt_long*/
#include <stdlib.h>  /* for atoi() */
#include <errno.h>

#define MAX_GPIO_COUNT 16

int gpio_base = 55;
int gpio_count = MAX_GPIO_COUNT;
int gpio_states[MAX_GPIO_COUNT];

#define MAX_FILENAME_LENGTH 255
char gpio_filenames[MAX_GPIO_COUNT][MAX_FILENAME_LENGTH];

static void print_info()
{
  printf("gpio-poll tool by mdk\n");
  printf("\n");
}

static void print_usage()
{
  printf("example usage: ./gpio-poll --base=55 --count=16\n");
}

static void handle_parameters(int argc, char** argv)
{
  int c = -1;

  struct option long_options[] = {
    { "base",  required_argument, 0, 0 },
    { "count", required_argument, 0, 0 },
    { "help", no_argument, 0, 0},
    { 0,       0,             0, 0 }
  };

  /* enable error messages for arguments */
  opterr = 1;

  /* getopt_long stores the option index here */
  int option_index = 0;

  do
  {
    c = getopt_long(argc, argv, "", long_options, &option_index);

    if (c != -1)
    {
      switch (option_index)
      {
        case 0: gpio_base = atoi(optarg); break;
        case 1: gpio_count = atoi(optarg); break;
        case 2: print_usage(); exit(0); break;
      }
    }

  } while (c != -1);
}

/* polls the specified gpio and sets the 'value'
   returns 1 if successful, otherwise 0
   example usage: poll_gpio("/sys/class/gpio/gpio55/value", &value); */
static int poll_gpio(const char *path, int* value)
{
  FILE *file;
  char buffer[1];
  int ret;

  /* try to open the file */
  file = fopen(path, "r");
  if (file != NULL)
  {
    /* try to read 1 character */
    if (ret = fread(buffer, 1, 1, file))
    {
      /* ascii 48 -> 0, 49 -> 1 */
      if (buffer[0] == 48)
        *value = 0;
      else if (buffer[0] == 49)
        *value = 1;
      else
        ret = 0;
    }

    /* close the file */
    fclose(file);
  }
  else
    return 0;

  return ret;
}

static void init_gpios(int basegpio, int gpiocount, char filenames[][MAX_FILENAME_LENGTH])
{
  int i;
  /* initialize the gpio names */
  memset(filenames, 0, 255*gpiocount);
  for (i = 0; i < gpiocount; i++)
    snprintf(filenames[i], 254, "/sys/class/gpio/gpio%i/value", i+basegpio);

  /* print the gpio names */
  printf("polling the following gpios:\n");
  for (i = 0; i < gpiocount; i++)
    printf("%s\n", filenames[i]);
  printf("\n");
}

static void poll_gpios(int gpiocount, int states[MAX_GPIO_COUNT], char filenames[][MAX_FILENAME_LENGTH])
{
  int value;
  int i;

  if (gpiocount > MAX_GPIO_COUNT)
    gpiocount = MAX_GPIO_COUNT;

  /* now poll them */
  for (i = 0; i < gpiocount; i++)
  {
    if (poll_gpio(filenames[i], &value))
    {
      //printf("gpio: %s, value: %i\n", filenames[i], value);
      states[i] = value;
    }
    else
    {
      states[i] = -1;
      //printf("gpio: %s, failed!\n", filenames[i]);
    }
  }
}

static void print_gpios(int base, int count, int states[MAX_GPIO_COUNT])
{
  int i;

  for (i = 0; i < count; i++)
    printf("  %i", base + i);
  printf("\n");
  for (i = 0; i < count; i++)
    printf("  %2d", states[i]);
  printf("\n");
}

/* main entry point */
int main(int argc, char *argv[])
{
  int value;

  print_info();
  
  /* parse the cmdline arguments */
  handle_parameters(argc, argv);

  /* initialize the gpio file names */
  init_gpios(gpio_base, gpio_count, gpio_filenames);

  /* read the gpios */
  poll_gpios(gpio_count, gpio_states, gpio_filenames);

  /* print the states of the gpios */
  print_gpios(gpio_base, gpio_count, gpio_states);
}

