#ifndef VRAM_POOL_H
#define VRAM_POOL_H

#include <mutex>
#include "Job.h"
#include "semaforo.h"

class VRAMPool {
private:
    Semaforo slots_disponibles;
    std::mutex mtx_asignacion;
    std::mutex mtx_liberacion;

public:
    VRAMPool();
    void asignar_slot(Job job);
    void liberar_slot();
};

#endif
