/* 
  gpio-poll
  test tool to poll for gpio changes
  written by mdk
*/
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <getopt.h>  /* for getopt_long*/
#include <stdlib.h>  /* for atoi() */
#include <errno.h>

#define MAX_GPIO_COUNT 16
#define MAX_FILENAME_LENGTH 255

struct settings_t {
  int gpio_base;
  int gpio_count;
  int gpio_states[MAX_GPIO_COUNT];
  char gpio_filenames[MAX_GPIO_COUNT][MAX_FILENAME_LENGTH];
  int keep_looping;
};

static void print_info()
{
  printf("gpio-poll tool by mdk\n");
  printf("\n");
}

static void print_usage()
{
  printf("example usage: ./gpio-poll --base=55 --count=16 --loop\n");
}

static void handle_parameters(int argc, char** argv, settings_t *settings)
{
  int c = -1;

  struct option long_options[] = {
    { "base",  required_argument, 0, 0 },
    { "count", required_argument, 0, 0 },
    { "help", no_argument, 0, 0},
    { "loop", no_argument, 0, 0},
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
        case 0: settings->gpio_base = atoi(optarg); break;
        case 1: settings->gpio_count = atoi(optarg); break;
        case 2: print_usage(); exit(0); break;
        case 3: settings->keep_looping = 1; break;
      }
    }

  } while (c != -1);
}

/* polls the specified gpio and sets the 'value'
   returns 1 if successful, otherwise 0
   example usage: poll_gpio("/sys/class/gpio/gpio55/value", &value); */
static int read_gpio(const char *path, int* value)
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

/* initializes the filenames based supplied values */
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

/* reads the state from the gpios and updates their states 
   returns the number of state changes */
static int read_gpios(int gpiocount, int states[MAX_GPIO_COUNT], char filenames[][MAX_FILENAME_LENGTH])
{
  int value;
  int i;
  int value_changed = 0;

  if (gpiocount > MAX_GPIO_COUNT)
    gpiocount = MAX_GPIO_COUNT;

  for (i = 0; i < gpiocount; i++)
  {
    if (read_gpio(filenames[i], &value))
    {
      if (states[i] != value)
        value_changed++;
      states[i] = value;
    }
    else
    {
      if (states[i] != -1)
        value_changed++;
      states[i] = -1;
    }
  }
  return value_changed;
}

/* prints the gpios and their states */
static void print_gpios(int base, int count, int states[MAX_GPIO_COUNT])
{
  int i;
  printf("gpios:  ");
  for (i = 0; i < count; i++)
    printf("  %i", base + i);
  printf("\nvalues: ");
  for (i = 0; i < count; i++)
    printf("  %2d", states[i]);
  printf("\n");
}

/* function that keeps reading the gpios in a tight loop and prints detected changes */
static void enter_read_gpios_loop(int base, int count, int states[MAX_GPIO_COUNT], char filenames[][MAX_FILENAME_LENGTH])
{
  int readcount = 0;
  while (1)
  {
    readcount++;

    /* keep polling the states of the gpios and print them if they change */
    if (read_gpios(count, states, filenames))
    {
      printf("%i * %i gpios polled before change occurred\n", readcount, count);
      count = 0;
      print_gpios(base, count, states);
    }
  }
}

/* main entry point */
int main(int argc, char *argv[])
{
  struct settings_t settings;
  settings.gpio_base = 55;
  settings.gpio_count = MAX_GPIO_COUNT;
  settings.keep_looping = 0;

  print_info();
  
  /* parse the cmdline arguments */
  handle_parameters(argc, argv, &settings);

  /* initialize the gpio file names */
  init_gpios(settings.gpio_base, settings.gpio_count, settings.gpio_filenames);

  /* read the gpios */
  read_gpios(settings.gpio_count, settings.gpio_states, settings.gpio_filenames);

  /* print the states of the gpios */
  print_gpios(settings.gpio_base, settings.gpio_count, settings.gpio_states);

  /* keep reading gpios cpu intensive in a tight loop */
  if (settings.keep_looping)
    enter_read_gpios_loop(settings.gpio_base, settings.gpio_count, settings.gpio_states, settings.gpio_filenames);

}

