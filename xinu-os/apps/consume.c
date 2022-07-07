#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>

sid32 is_prodcon_done;
sid32 mutex;

int arr_q[5];

int tail;   // read

int read_Q();

void consumer(int count) {
  // TODO: implement the following:
  // - Iterates from 0 to count (count including)
  //   - reading the value of the global variable 'n' each time
  //   - print consumed value (the value of 'n'), e.g. "consumed : 8"

  wait(is_prodcon_done);

  int i;
  for( i = 0; i <= count; i++ ){
    wait(can_read);
    printf("consumed : %d\n", n);
    signal(can_write);
  }

  signal(is_prodcon_done);
}

void consumer_bb(int id, int count) {
  // TODO: implement the following:
  // - Iterate from 0 to count (count excluding)
  //   - read the next available value from the global array `arr_q`
  //   - print consumer id (starting from 0) and read value as:
  //     "name : consumer_X, read : X"

  wait(is_prodcon_done);

  int i;
  for( i = 0; i < count; i++ ){
    wait(can_read);
    
    wait(mutex);
    int r;
    r = read_Q(i);
    printf("name : consumer_%d, read : %d\n",id,r);    
    signal(mutex);
    
    signal(can_write);
  }

  signal(is_prodcon_done);
}

int read_Q() {
  int read_val = arr_q[tail];
  arr_q[tail] = -1;
  tail = (tail + 1) % 5;

  return read_val;
}