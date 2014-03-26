/* test tool to send data to tty devices */

#include <stdio.h>
#include <iostream>
#include <termios.h> // needed for implementing set_inferface_attribs()
#include <fcntl.h>   /* needed for O_RDWR, O_NOCTTY, O_NDELAY */
#include <string.h>
#include <getopt.h>  /* for getopt_long*/
#include <stdlib.h>  /* for atoi() */
#include <errno.h>

int gpio_base = 55;
int gpio_count = 16;

static void print_usage()
{
  printf("gpio-poll tool by mdk\n");
  printf("example usage: ./gpio-poll\n");
  printf("\n");
}

static void handle_parameters(int argc, char** argv)
{
  int c = -1;

  struct option long_options[] = {
    { "base",  required_argument, 0, 0 },
    { "count", required_argument, 0, 0 },
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
      }
    }

  } while (c != -1);
}

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


static void poll_gpios(int basegpio, int gpiocount)
{
  int states[16];
  int value;
  char filename[16][255];
  int i;

  //if (poll_gpio("/sys/class/gpio/gpio55/value", &value))
  //  printf("value: %i\n", value);
  //else
  //  printf("failed!\n");

  if (gpiocount > 16)
    gpiocount = 16;

  /* initialize the gpio names */
  memset(filename, 0, 255*gpiocount);
  for (i = 0; i < gpiocount; i++)
    snprintf(filename[i], 254, "/sys/class/gpio/gpio%i/value", i+basegpio);

  /* print the gpio names */
  printf("polling the following gpios:\n");
  for (i = 0; i < gpiocount; i++)
    printf("%s\n", filename[i]);
  printf("\n");

  /* now poll them */
  for (i = 0; i < gpiocount; i++)
  {
    if (poll_gpio(filename[i], &value))
      printf("gpio: %s, value: %i\n", filename[i], value);
    else
      printf("gpio: %s, failed!\n", filename[i]);
  }
}

/* main entry point */
int main(int argc, char *argv[])
{
  int value;

  print_usage();
  
  /* handle the params */
  handle_parameters(argc, argv);

  /* read the gpios */
  poll_gpios(gpio_base, gpio_count);
}

