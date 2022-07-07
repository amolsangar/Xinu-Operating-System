#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>

sid32 is_prodcon_done;
sid32 mutex;

int arr_q[5];

int head;   // write

void write_Q(int val);

void producer(int count) {
  // TODO: implement the following:
  // - Iterates from 0 to count (count including)
  //   - setting the value of the global variable 'n' each time
  //   - print produced value (new value of 'n'), e.g.: "produced : 8"
  
  wait(is_prodcon_done);

  int i;
  for( i = 0; i <= count; i++ ){
    wait(can_write);
    n = i;
    printf("produced : %d\n", n);
    signal(can_read);
  }

  signal(is_prodcon_done);
}

void producer_bb(int id, int count) {
  // TODO: implement the following:
  // - Iterate from 0 to count (count excluding)
  //   - add iteration value to the global array `arr_q`
  //   - print producer id (starting from 0) and written value as:
  //     "name : producer_X, write : X"

  wait(is_prodcon_done);

  int i;
  for( i = 0; i < count; i++ ){
    wait(can_write);
    
    wait(mutex);
    write_Q(i);
    printf("name : producer_%d, write : %d\n",id,i);    
    signal(mutex);
    
    signal(can_read);
  }

  signal(is_prodcon_done);
}

void write_Q(int val) {
  arr_q[head] = val;
  head = (head + 1) % 5;
}