#include <xinu.h>
#include <future.h>
#include <stddef.h>
#include <future_prodcons.h>

sid32 print_sem;

uint future_prod(future_t *fut, char *value) {
  int status;

  wait(print_sem);
  printf("Producing %d\n", *value);
  signal(print_sem);
  
  status = (int) future_set(fut, value);
  if (status < 1) {
    wait(print_sem);
    printf("future_set failed\n");
    signal(print_sem);
    return -1;
  }

  return OK;
}