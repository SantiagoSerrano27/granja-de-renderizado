#include "MessageQueue.h"
#include <chrono>

MessageQueue::MessageQueue() {
    init(hay_datos, 0);
}

void MessageQueue::enqueue(Job job) {
    mtx_queue.lock();
    lista_jobs.push_back(job);
    mtx_queue.unlock();

    signal(hay_datos);
}

bool MessageQueue::dequeue(Job& job_retorno) {
    wait(hay_datos); // Bloqueo pasivo si el buffer esta vacio

    mtx_queue.lock();
    if (lista_jobs.empty()) {
        mtx_queue.unlock();
        return false; // Retorna falso si se notifico el cierre del sistema sin datos remanentes
    }

    int indice_elegido = 0;
    auto ahora = std::chrono::high_resolution_clock::now().time_since_epoch();
    long long ms_actual = std::chrono::duration_cast<std::chrono::milliseconds>(ahora).count();

    bool starvation_detectado = false;

    // Evaluacion de Inanicion: Prioriza tareas Free con espera igual o superior a 5000ms
    for (size_t i = 0; i < lista_jobs.size(); i++) {
        if (lista_jobs[i].prioridad == 0 && (ms_actual - lista_jobs[i].timestamp_creacion) >= 5000) {
            indice_elegido = i;
            starvation_detectado = true;
            break;
        }
    }

    // Criterio de Prioridad: Planifica tareas Premium si no se registro starvation
    if (!starvation_detectado) {
        for (size_t i = 0; i < lista_jobs.size(); i++) {
            if (lista_jobs[i].prioridad == 1) {
                indice_elegido = i;
                starvation_detectado = true;
                break;
            }
        }
    }

    job_retorno = lista_jobs[indice_elegido];
    lista_jobs.erase(lista_jobs.begin() + indice_elegido);

    mtx_queue.unlock();
    return true;
}

bool MessageQueue::vacia() {
    mtx_queue.lock();
    bool resultado = lista_jobs.empty();
    mtx_queue.unlock();
    return resultado;
}

void MessageQueue::notificar_final() {
    signal(hay_datos); // Desbloquea hilos consumidores en espera pasiva durante el apagado del sistema
}
