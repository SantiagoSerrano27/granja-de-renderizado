#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <vector>
#include <mutex>
#include "Job.h"
#include "semaforo.h"

class MessageQueue {
private:
    std::vector<Job> lista_jobs;
    std::mutex mtx_queue;
    Semaforo hay_datos;

public:
    MessageQueue();
    void enqueue(Job job);
    bool dequeue(Job& job_retorno);
    bool vacia();
    void notificar_final();
};

#endif
