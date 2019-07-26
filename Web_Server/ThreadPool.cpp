// This file has not been used
#include "ThreadPool.h"
#include <stdio.h>

pthread_mutex_t ThreadPool::lock = PTHREAD_MUTEX_INITIALIZER;    // 初始化互斥锁
pthread_cond_t ThreadPool::notify = PTHREAD_COND_INITIALIZER;
std::vector<pthread_t> ThreadPool::threads;
std::vector<ThreadPoolTask> ThreadPool::queue;                   // 任务
int ThreadPool::thread_count = 0;
int ThreadPool::queue_size = 0;
int ThreadPool::head = 0;
int ThreadPool::tail = 0;
int ThreadPool::count = 0;
int ThreadPool::shutdown = 0;
int ThreadPool::started = 0;

// 创建线程池
int ThreadPool::threadpool_create(int _thread_count, int _queue_size)
{
    bool err = false;
    do
    {
        if(_thread_count <= 0 || _thread_count > MAX_THREADS || _queue_size <= 0 || _queue_size > MAX_QUEUE) 
        {
            _thread_count = 4;
            _queue_size = 1024;
        }
    
        thread_count = 0;
        queue_size = _queue_size;
        head = tail = count = 0;
        shutdown = started = 0;

        threads.resize(_thread_count);
        queue.resize(_queue_size);
    
        /* Start worker threads */
        for(int i = 0; i < _thread_count; ++i) 
        {
            // 创建新线程
            if(pthread_create(&threads[i], NULL, threadpool_thread, (void*)(0)) != 0) 
            {
                //threadpool_destroy(pool, 0);
                return -1;
            }
            ++thread_count;
            ++started;
        }
    } while(false);
    
    if (err) 
    {
        //threadpool_free(pool);
        return -1;
    }
    return 0;
}


// 向线程池中添加新任务
int ThreadPool::threadpool_add(std::shared_ptr<void> args, std::function<void(std::shared_ptr<void>)> fun)
{
    int next, err = 0;
    if(pthread_mutex_lock(&lock) != 0)
        return THREADPOOL_LOCK_FAILURE;
    do 
    {
        next = (tail + 1) % queue_size;
        // 队列满
        if(count == queue_size) 
        {
            err = THREADPOOL_QUEUE_FULL;
            break;
        }
        // 已关闭
        if(shutdown)
        {
            err = THREADPOOL_SHUTDOWN;
            break;
        }

        // 将新任务添加到队列末尾
        queue[tail].fun = fun;
        queue[tail].args = args;
        tail = next;
        ++count;
        
        /* pthread_cond_broadcast */
        // 唤醒一个等待该条件的线程
        if(pthread_cond_signal(&notify) != 0) 
        {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }
    } while(false);

    if(pthread_mutex_unlock(&lock) != 0)
        err = THREADPOOL_LOCK_FAILURE;
    return err;
}


int ThreadPool::threadpool_destroy(ShutDownOption shutdown_option)
{
    printf("Thread pool destroy !\n");
    int i, err = 0;

    if(pthread_mutex_lock(&lock) != 0) 
    {
        return THREADPOOL_LOCK_FAILURE;
    }
    do 
    {
        if(shutdown) {
            err = THREADPOOL_SHUTDOWN;
            break;
        }
        shutdown = shutdown_option;
       
        // 激活所有等待进程列表中最先入队的线程
        if((pthread_cond_broadcast(&notify) != 0) || (pthread_mutex_unlock(&lock) != 0)) {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }

        for(i = 0; i < thread_count; ++i)
        {
            // 等待线程结束
            if(pthread_join(threads[i], NULL) != 0)
            {
                err = THREADPOOL_THREAD_FAILURE;
            }
        }
    } while(false);

    if(!err) 
    {
        threadpool_free();
    }
    return err;
}

int ThreadPool::threadpool_free()
{
    if(started > 0)
        return -1;
    pthread_mutex_lock(&lock);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&notify);
    return 0;
}


// 线程执行
void *ThreadPool::threadpool_thread(void *args)
{
    while (true)
    {
        ThreadPoolTask task;
        pthread_mutex_lock(&lock);

        while((count == 0) && (!shutdown)) 
        {
            pthread_cond_wait(&notify, &lock);      // 等待条件变量
        }
        if((shutdown == immediate_shutdown) ||
           ((shutdown == graceful_shutdown) && (count == 0)))
        {
            break;
        }
        task.fun = queue[head].fun;
        task.args = queue[head].args;
        queue[head].fun = NULL;
        queue[head].args.reset();
        head = (head + 1) % queue_size;
        --count;                                   // 任务队列中任务减1

        pthread_mutex_unlock(&lock);
        (task.fun)(task.args);
    }

    --started;                                     // 工作线程数减1
    pthread_mutex_unlock(&lock);
    printf("This threadpool thread finishs!\n");
    pthread_exit(NULL);                            // 线程终止 
    return(NULL);
}