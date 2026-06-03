#include "VRAMPool.h"
#include <thread>
#include <chrono>

VRAMPool::VRAMPool() {
    init(slots_disponibles, 5); // Máximo 5 slots simultáneos
}

void VRAMPool::asignar_slot(Job job) {
    wait(slots_disponibles); // Bloqueo pasivo si los 5 slots de memoria están saturados

    // Control de flujo estricto: exclusión mutua con retardo forzado de 450ms por asignación exitosa
    mtx_asignacion.lock();
    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    mtx_asignacion.unlock();
}

void VRAMPool::liberar_slot() {
    // Control de flujo estricto: exclusión mutua con retardo forzado de 250ms por liberación de slot
    mtx_liberacion.lock();
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    mtx_liberacion.unlock();

    signal(slots_disponibles);
}
