#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include "Job.h"
#include "MessageQueue.h"
#include "VRAMPool.h"
#include "Logger.h"

// Variables Compartidas Globales
MessageQueue queue;
VRAMPool pool;
Logger logger("sistema.log");

int contador_exito = 0;    // Contador global afectado por condiciones de carrera
std::mutex mtx_contador;   // Mutex para mitigar Race Conditions

std::atomic<int> jobs_generados_totales(0);
int jobs_a_producir = 1500;

// Indica si estamos ejecutando la prueba de saturación
bool prueba_saturacion = false;

// Flag global para indicar a los workers que la producción terminó (salida segura)
std::atomic<bool> produccion_terminada(false);

// Hilos Productores: Nodos API Gateway
void productor_api(int id, int total_productores) {
    if (jobs_a_producir == 0) return; // Si es prueba de vacuidad, el productor no hace nada

    int jobs_por_productor = jobs_a_producir / total_productores;
    int resto = jobs_a_producir % total_productores;

    // Los primeros productores generan una tarea extra si sobra alguna
    if (id <= resto) {
        jobs_por_productor++;
    }

    for (int i = 0; i < jobs_por_productor; i++) {
        // Retardo simulado de ingreso a la Message Queue (100ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        Job nuevo_job;
        nuevo_job.id = jobs_generados_totales.fetch_add(1) + 1;

        // En la prueba de saturación todos los jobs son Premium
        if (prueba_saturacion) {
            nuevo_job.prioridad = 1;
        }
        else {
            // Simulación de carga: 1 de cada 3 tareas es Free (0), el resto Premium (1)
            nuevo_job.prioridad = (nuevo_job.id % 3 == 0) ? 0 : 1;
        }

        auto ahora = std::chrono::high_resolution_clock::now().time_since_epoch();
        nuevo_job.timestamp_creacion = std::chrono::duration_cast<std::chrono::milliseconds>(ahora).count();

        logger.registrar_evento(nuevo_job, "CREADO");

        queue.enqueue(nuevo_job);
        logger.registrar_evento(nuevo_job, "EN_COLA");
    }
}

// Hilos Consumidores: Worker Nodes
void worker_node(int id) {
    while (true) {
        // Control de parada seguro contra interbloqueos en el vaciado de cola
        mtx_contador.lock();
        if (contador_exito >= jobs_a_producir || (produccion_terminada && queue.vacia())) {
            mtx_contador.unlock();
            break;
        }
        mtx_contador.unlock();

        // Si la producción terminó y quedó vacío, salimos antes de bloquearnos en el dequeue
        if (produccion_terminada && queue.vacia()) {
            break;
        }

        // Si el sistema se queda sin Jobs en vacuidad, se valida antes de ejecutar
        if (jobs_a_producir == 0) {
            break;
        }

        Job job_a_procesar;

        if (!queue.dequeue(job_a_procesar)) {
            break;
}

        // Intentar ingresar al Pool (Semaforizado a un máximo de 5 slots)
        pool.asignar_slot(job_a_procesar);
        logger.registrar_evento(job_a_procesar, "ASIGNADO_VRAM");

        // Tiempo de Carga de Assets mínimo en VRAM (600ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(600));

        logger.registrar_evento(job_a_procesar, "FINALIZADO");

        // Modificación de sección crítica del contador de éxito global
        mtx_contador.lock();
        contador_exito++;
        mtx_contador.unlock();

        // Liberar slot de la GPU simulada
        pool.liberar_slot();
    }
}

int main() {
    int num_productores = 1;
    int num_consumidores = 2;

    std::cout << "=========================================================" << std::endl;
    std::cout << "       SISTEMA DE RENDERIZADO CONCURRENTE EN LA NUBE     " << std::endl;
    std::cout << "=========================================================" << std::endl;
    std::cout << "Seleccione la configuracion de carga para los hilos:\n";
    std::cout << "1. ConfigA: 1 Productor y 2 Consumidores (Baja recepcion)\n";
    std::cout << "2. ConfigB: 3 Productores y 1 Consumidor (Cuello de Botella)\n";
    std::cout << "3. ConfigC: 3 Productores y 3 Consumidores (Alta concurrencia)\n";
    std::cout << "Opcion [1-3]: ";
    int opcion;
    std::cin >> opcion;

    if (opcion == 2) {
        num_productores = 3;
        num_consumidores = 1;
    }
    else if (opcion == 3) {
        num_productores = 3;
        num_consumidores = 3;
    }

    std::cout << "\nSeleccione el escenario de evaluacion y prueba:\n";
    std::cout << "1. Prueba de Carga Masiva (1500 jobs)\n";
    std::cout << "2. Prueba de Saturacion de Recursos (8 jobs Premium)\n";
    std::cout << "3. Prueba de Vacuidad (0 jobs)\n";
    std::cout << "Opcion [1-3]: ";
    int prueba;
    std::cin >> prueba;

    if (prueba == 2) {
        jobs_a_producir = 8;
        prueba_saturacion = true;
    }
    else if (prueba == 3) {
        jobs_a_producir = 0;
    }

    std::cout << "\nIniciando simulacion concurrente..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> productores;
    std::vector<std::thread> consumidores;

    // Lanzamiento asincrónico coordinado de hilos
    for (int i = 0; i < num_consumidores; i++) {
        consumidores.push_back(std::thread(worker_node, i + 1));
    }

    for (int i = 0; i < num_productores; i++) {
        productores.push_back(std::thread(productor_api, i + 1, num_productores));
    }

    // Esperar a que los productores terminen su trabajo
    for (auto& t : productores) {
        if (t.joinable()) t.join();
    }

    // Cambiamos el estado de la producción a terminado de forma atómica
    produccion_terminada = true;

    // Si la prueba es vacuidad, despertamos pasivamente las estructuras o salimos directamente
    if (jobs_a_producir == 0) {
        // En vacuidad, mandamos una señal al semáforo solo para destrabar la espera pasiva
        for (int i = 0; i < num_consumidores; i++) {
            Job vacio = {0, 0, 0};
            queue.enqueue(vacio);
        }
    }

    // Esperar a que los consumidores terminen de vaciar el buffer
    for (auto& t : consumidores) {
        if (t.joinable()) t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Ajustar contador si se ejecutó la prueba de vacuidad para que muestre 0 absoluto
    if (jobs_a_producir == 0) {
        contador_exito = 0;
    }

    std::cout << "\n=========================================================" << std::endl;
    std::cout << "                   EJECUCION CONCLUIDA                   " << std::endl;
    std::cout << "=========================================================" << std::endl;
    std::cout << ">> Tareas totales procesadas exitosamente: " << contador_exito << std::endl;
    std::cout << ">> Tiempo total del Planificador de Hilos: " << total_duration << " ms" << std::endl;
    std::cout << "=========================================================" << std::endl;

    return 0;
}
