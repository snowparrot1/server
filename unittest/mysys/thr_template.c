/* Copyright (c) 2006-2008 MySQL AB, 2009 Sun Microsystems, Inc.
   Use is subject to license terms.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include <my_global.h>
#include <my_sys.h>
#include <my_atomic.h>
#include <tap.h>

volatile uint32 bad;
pthread_attr_t thr_attr;
pthread_mutex_t mutex;
pthread_cond_t cond;
pthread_cond_t cond2;
uint running_threads;

void do_tests();

void test_concurrently(const char *test, pthread_handler handler, int n, int m)
{
  pthread_t t;
  ulonglong now= my_interval_timer();

  bad= 0;

  diag("Testing %s with %d threads, %d iterations... ", test, n, m);
  for (running_threads= n ; n ; n--)
  {
    if (pthread_create(&t, &thr_attr, handler, &m) != 0)
    {
      diag("Could not create thread");
      abort();
    }
  }
  pthread_mutex_lock(&mutex);
  while (running_threads)
    pthread_cond_wait(&cond, &mutex);
  pthread_mutex_unlock(&mutex);

  now= my_interval_timer() - now;
  ok(!bad, "tested %s in %g secs (%d)", test, ((double)now)/1e9, bad);
}

void test_concurrently2(const char *test, pthread_handler handler1,
                        pthread_handler handler2, int n1, int n2, ulonglong param_h)
{
  pthread_t t;
  ulonglong now= my_interval_timer();

  bad= 0;

  diag("Testing %s with %d+%d threads... ", test, n1, n2);
  running_threads= n1 + n2;
  for (; n1 ; n1--)
  {
    if (pthread_create(&t, &thr_attr, handler1, &param_h) != 0)
    {
      diag("Could not create thread");
      abort();
    }
  }
  for (; n2 ; n2--)
  {
    if (pthread_create(&t, &thr_attr, handler2, &param_h) != 0)
    {
      diag("Could not create thread");
      abort();
    }
  }
  pthread_mutex_lock(&mutex);
  while (running_threads)
    pthread_cond_wait(&cond, &mutex);
  pthread_mutex_unlock(&mutex);

  now= my_interval_timer() - now;
  ok(!bad, "tested %s in %g secs (%d)", test, ((double)now)/1e9, bad);
}

#ifdef MY_IO_CACHE_CONC
int main(int argc, char **argv)
#else
int main(int argc __attribute__((unused)), char **argv)
#endif
{
  MY_INIT(argv[0]);

#ifdef MY_IO_CACHE_CONC
  if (argc > 1)
       n_readers= atoi(argv[1]);
  if (argc > 2)
       n_writers= atoi(argv[2]);
  if (argc > 3)
       n_messages= atoi(argv[3]);
  if (argc > 4)
       cache_read_with_care= atoi(argv[4]);
#else
  if (argv[1] && *argv[1])
       DBUG_SET_INITIAL(argv[1]);
#endif

  pthread_mutex_init(&mutex, 0);
  pthread_cond_init(&cond,  0);
  pthread_cond_init(&cond2, 0);
  pthread_attr_init(&thr_attr);
  pthread_attr_setdetachstate(&thr_attr,PTHREAD_CREATE_DETACHED);

#define CYCLES 3000
#define THREADS 30

  diag("N CPUs: %d, atomic ops: %s", my_getncpus(), MY_ATOMIC_MODE);

  do_tests();

  /*
    workaround until we know why it crashes randomly on some machine
    (BUG#22320).
  */
#ifdef NOT_USED
  sleep(2);
#endif
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);
  pthread_cond_destroy(&cond2);
  pthread_attr_destroy(&thr_attr);
  my_end(0);
  return exit_status();
}

