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
#include <poll.h>   /* for poll() */
#include <fcntl.h>  /* used for O_RDONLY */

#define MAX_GPIO_COUNT 16
#define MAX_FILENAME_LENGTH 255

struct settings_t {
  int gpio_base;
  int gpio_count;
  int gpio_states[MAX_GPIO_COUNT];
  char gpio_filenames[MAX_GPIO_COUNT][MAX_FILENAME_LENGTH];
  char gpio_edge_filenames[MAX_GPIO_COUNT][MAX_FILENAME_LENGTH];
  int gpio_edge_values[MAX_GPIO_COUNT];
  int loop;
  int poll;
  int poll_timeout;
};

static void print_info()
{
  printf("gpio-poll tool by mdk\n");
  printf("use 'gpio-poll --help' for additional information\n");
  printf("\n");
}

static void print_usage()
{
  printf("example usage: ./gpio-poll --base=55 --count=16 --poll --timeout=1000\n");
  printf("\n");
  printf("if you are using --poll remember to configure the interrupt edges for the gpio\n");
  printf("e.g.: echo both > /sys/class/gpio/gpio8/edge\n");
}

static void handle_parameters(int argc, char** argv, settings_t *settings)
{
  int c = -1;

  struct option long_options[] = {
    { "base",  required_argument, 0, 0 },
    { "count", required_argument, 0, 0 },
    { "help", no_argument, 0, 0 },
    { "loop", no_argument, 0, 0 },
    { "poll", no_argument, 0, 0 },
    { "timeout", required_argument, 0, 0 },
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
        case 3: settings->loop = 1; break;
        case 4: settings->poll = 1; break;
        case 5: settings->poll_timeout = atoi(optarg); break;
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
    ret = 0;

  return ret;
}

/* initializes the filenames based supplied values */
static void init_gpio_filenames(int basegpio, int gpiocount, char filenames[][MAX_FILENAME_LENGTH])
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
    printf("  %2d", base + i);
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
      readcount = 0;
      print_gpios(base, count, states);
    }
  }
}

static void init_gpio_edges(int basegpio, int gpiocount, char edges[][MAX_FILENAME_LENGTH])
{
  int i;
  memset(edges, 0, 255*gpiocount);
  for (i = 0; i < gpiocount; i++)
    snprintf(edges[i], 254, "/sys/class/gpio/gpio%i/edge", i+basegpio);
}

typedef enum {
  gpio_edge_none = 0, 
  gpio_edge_falling, 
  gpio_edge_rising, 
  gpio_edge_both
} gpio_edge_t;

/* returns 1 if the edge was read successfully, otherwise 0 */
static int read_edge(const char *path, int *value)
{
  FILE *file;
  char buffer[32];
  int ret = 0;

  file = fopen(path, "r");
  if (file != NULL)
  {
    if (ret = fread(buffer, 1, sizeof(buffer), file))
    {
      ret = 1;

      if (strncmp("none", buffer, 4) == 0)
        *value = gpio_edge_none;
      else if (strncmp("falling", buffer, 7) == 0)
        *value = gpio_edge_falling;
      else if(strncmp("rising", buffer, 6) == 0)
        *value = gpio_edge_rising;
      else if (strncmp("both", buffer, 4) == 0)
        *value = gpio_edge_both;
      else
      {
        *value = gpio_edge_none;
        ret = 0;
      }
    }
    fclose(file);
  }
  return ret;
}

static int read_edges(int count, int edges[MAX_GPIO_COUNT], char filenames[][MAX_FILENAME_LENGTH])
{
  int i, value, ret;

  memset(edges, 0, sizeof(edges));

  ret = 1;

  for (i = 0; i < count; i++)
    if (read_edge(filenames[i], &value))
    {
      edges[i] = value;
    }
    else
    {
      printf("reading edge '%s' from gpio failed\n", filenames[i] );
      ret = 0;
      break;
    }
    
  return ret;
}

/* waits for changes in the supplied gpios by using the poll() function
   works only for gpios that support interrupts but does not put a load on the cpu
   timeout: in ms, 0 returns immediately, -1 blocks forever
   returns: the index of the changed gpio or -1 if timeout occurrs or -2 if an error occurrs */
static int poll_gpios(int count, char filenames[][MAX_FILENAME_LENGTH], int timeout)
{
  int ret, i, value;
  char buffer[1];

  /* the file descriptors we're waiting on */
  struct pollfd fds[MAX_GPIO_COUNT];

  for (i = 0; i < count; i++)
  {
    /* open the file descriptors */
    fds[i].fd = open(filenames[i], O_RDONLY);
    fds[i].events = POLLPRI | POLLERR; /* poll high priority data without blocking as specified in gpio.txt */

    /* clear the current data so that poll returns only on new data, not on the currently available one */
    if (fds[i].fd != -1)
      read(fds[i].fd, buffer, sizeof(buffer));
  }

  value = poll(fds, count, timeout);

  /* check if a timeout occurred */
  if (value == 0)
    ret = -1; 
  /* check if an error occurred */
  else if(value == -1)
    ret = -2;
  else /* value specifies the number of events that occurred */
  {
    ret = -3;
    /* evaluate which gpio has changed */
    for (i = 0; i < count; i++)
    {
      /* can high priority data be read without blocking? */
      if (fds[i].revents & POLLPRI)
      {
        ret = i;
        break;
      }
    }
  }

  /* close all file descriptors */
  for (i = 0; i < count; i++)
    if (fds[i].fd != -1)
      close(fds[i].fd);

  return ret;
}

static void enter_poll_gpios(int base, int count, int timeout, int states[MAX_GPIO_COUNT], char filenames[][MAX_FILENAME_LENGTH])
{
  int ret;

  printf("waiting for interrupt on the gpios...\n");
  ret = poll_gpios(count, filenames, timeout);
  if (ret == -1)
    printf("timeout occurred\n");
  else if (ret == -2)
    printf("error occurred\n");
  else if (ret == -3)
    printf("unknown reason");
  else
  {
    printf("gpio %i changed\n", base + ret);

    /* update and show the new gpio states */
    read_gpios(count, states, filenames);
    print_gpios(base, count, states);
  }

}

static int validate_edges(int base, int count, int edges[MAX_GPIO_COUNT])
{
  int i, ret;

  ret = 1;

  for (i = 0; i < count && ret == 1; i++)
  {
    printf("edge of gpio %i is '", base + i);
    switch (edges[i])
    {
      case gpio_edge_none: printf("none"); ret = 0; break;
      case gpio_edge_falling: printf("falling"); break;
      case gpio_edge_rising: printf("rising"); break;
      case gpio_edge_both: printf("both"); break;
      default: printf("unknown"); ret = 0; break;
    }
    printf (" (%i)'\n", edges[i]);
  }

  if (ret == 0)
    printf("polling won't work! consider changing the edge to rising/falling/both\n");

  return ret;
}

/* main entry point */
int main(int argc, char *argv[])
{
  int ret;

  /* init the default settings */
  struct settings_t settings;
  settings.gpio_base  = 55;
  settings.gpio_count = MAX_GPIO_COUNT;
  settings.loop       = 0;
  settings.poll       = 0;
  settings.poll_timeout = -1;

  print_info();
  
  /* parse the cmdline arguments */
  handle_parameters(argc, argv, &settings);

  /* initialize the gpio file names */
  init_gpio_filenames(settings.gpio_base, settings.gpio_count, settings.gpio_filenames);

  /* read the gpios */
  read_gpios(settings.gpio_count, settings.gpio_states, settings.gpio_filenames);

  /* print the states of the gpios */
  printf("initial values:\n");
  print_gpios(settings.gpio_base, settings.gpio_count, settings.gpio_states);
  printf("\n");

  /* keep reading gpios cpu intensive in a tight loop */
  if (settings.loop)
    enter_read_gpios_loop(settings.gpio_base, settings.gpio_count, settings.gpio_states, settings.gpio_filenames);

  /* poll the gpios once */
  if (settings.poll)
  {
    init_gpio_edges(settings.gpio_base, settings.gpio_count, settings.gpio_edge_filenames);
    if (read_edges(settings.gpio_count, settings.gpio_edge_values, settings.gpio_edge_filenames))
      if (validate_edges(settings.gpio_base, settings.gpio_count, settings.gpio_edge_values))
        enter_poll_gpios(settings.gpio_base, settings.gpio_count, settings.poll_timeout, settings.gpio_states, settings.gpio_filenames);
  }
}

