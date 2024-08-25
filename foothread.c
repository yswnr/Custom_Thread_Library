#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include <time.h>   
#include <unistd.h>  
#include <pthread.h> 
#include <sched.h>
#include <sys/wait.h>
#include <foothread.h>

int total_thread_created = 0;
sem_t for_exit, bin_var_op;
int is_created = 0;

void foothread_create(foothread_t *thread, foothread_attr_t *attr, int (*start_routine)(void *), void *arg)
{
    if (!thread)
        return -1;

    foothread_attr_t default_attr = FOOTHREAD_ATTR_INITIALIZER;
    if (!attr)
        attr = &default_attr;

    thread->stack = malloc(attr->stack_size);
    if (!thread->stack)
    {
        perror("Failed to allocate stack");
        return -1;
    }

    //rintf("Process in execution\n");

    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | CLONE_SYSVSEM;
    if (attr->join_type == FOOTHREAD_DETACHED)
    {
        flags |= CLONE_DETACHED;
    }

    sem_init(&bin_var_op, 0, 1);

    //****************locking
    sem_wait(&bin_var_op);
    total_thread_created++;
    sem_post(&bin_var_op);

    //printf("Process going on\n");

    thread->tid = clone(start_routine, (void *)thread->stack + attr->stack_size, flags, (void *)arg);
    thread->joinable = (attr->join_type == FOOTHREAD_JOINABLE);
    if (thread->tid == -1)
    {
        perror("clone failed");
        free(thread->stack);
        return -1;
    }

    return 0;
}
void foothread_attr_setjointype(foothread_attr_t *attr, int joinType)
{
    if (attr != NULL)
    {
        attr->join_type = joinType;
    }
}

void foothread_attr_setstacksize(foothread_attr_t *attr, size_t stackSize)
{
    if (attr != NULL)
    {
        attr->stack_size = stackSize;
    }
}

void foothread_exit()
{
    if (getpid() == gettid())
    {
        while (total_thread_created > 0)
        {
            sem_wait(&bin_var_op);
            total_thread_created--;
            sem_post(&bin_var_op);

            sem_wait(&for_exit);
        }
    }
    else
    {
        sem_post(&for_exit);
    }
}

void foothread_mutex_init(foothread_mutex_t *mutex)
{
    if (mutex != NULL)
    {
        mutex->tid = 0;
        sem_init(&mutex->sem, 0, 1);
        mutex->locked = 0;
    }
}

void foothread_mutex_lock(foothread_mutex_t *mutex)
{
    if (mutex != NULL)
    {
        sem_wait(&mutex->sem);
        mutex->tid = gettid();
        mutex->locked = 1;
    }
}

void foothread_mutex_unlock(foothread_mutex_t *mutex)
{
    if (mutex != NULL && mutex->locked == 1 && mutex->tid == gettid())
    {
        sem_post(&mutex->sem);
        mutex->locked = 0;
    }
    else
        perror("Error: mutex not locked or not owned by calling thread");
}

void foothread_mutex_destroy(foothread_mutex_t *mutex)
{
    if (mutex != NULL)
    {
        mutex->locked = 0;
        mutex->tid = -1;
        sem_destroy(&mutex->sem);
    }
}

void foothread_barrier_init(foothread_barrier_t *barrier, int count)
{
    if (barrier != NULL)
    {
        barrier->count = 0;
        barrier->threshold = count;
        foothread_mutex_init(&barrier->mutex);
        sem_init(&barrier->waiting_sem, 0, 0);
    }
}

void foothread_barrier_wait(foothread_barrier_t *barrier)
{
    if (barrier != NULL)
    {
        foothread_mutex_lock(&barrier->mutex);
        barrier->count++;
        if (barrier->count == barrier->threshold+1)
        {
            barrier->count = 0;
            foothread_mutex_unlock(&barrier->mutex);
            for (int i = 0; i < barrier->threshold; i++)
            {
                sem_post(&barrier->waiting_sem);
            }
        }
        else
        {
            foothread_mutex_unlock(&barrier->mutex);
            sem_wait(&barrier->waiting_sem);
        }
        
    }
}

void foothread_barrier_destroy(foothread_barrier_t *barrier)
{
    if (barrier != NULL)
    {
        foothread_mutex_destroy(&barrier->mutex);
        sem_destroy(&barrier->waiting_sem);
        barrier->threshold = -1;
        barrier->count = 0;
    }
}
