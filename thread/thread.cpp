#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t lock;
pthread_cond_t cond;

void * tfunc(void * arg)
{
    int i = 0;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    printf("thread run\n");

    while (1) {
        pthread_testcancel();
    }
    printf("thread exit\n");

    //pthread_mutex_lock(&lock);
    //pthread_cond_signal(&cond);
    //pthread_mutex_unlock(&lock);

    return NULL;
}

int main()
{
    int i;
    pthread_t pid;
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    pthread_create(&pid, NULL, tfunc, NULL);

    usleep(1000 * 1000);
    i = pthread_cancel(pid);
    printf("pthread cancel = %d\n", i);
     pthread_join(pid, NULL);
     printf("main exit\n");

    //pthread_mutex_lock(&lock);
    //pthread_cond_wait(&cond, &lock);
    //pthread_mutex_unlock(&lock);

    return 0;
}

