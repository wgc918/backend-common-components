#include "../include/TaskDispatcher.h"

void TaskDispatcher::task_add(Task *task)
{
    pool.submit(task);
}