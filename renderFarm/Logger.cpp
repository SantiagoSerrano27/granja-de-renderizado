#include "Logger.h"
#include <iostream>
#include <fstream>
#include <chrono>

Logger::Logger(std::string nombre) : archivo_nombre(nombre) {}

void Logger::inicializar_archivo() {
    std::lock_guard<std::mutex> lock(mtx_log);
    std::ofstream archivo(archivo_nombre, std::ios::trunc); // Truncado para limpiar ejecuciones previas
    if (archivo.is_open()) {
        archivo << "TIMESTAMP_MS,JOB_ID,PRIORIDAD,EVENTO\n";
        archivo.close();
    }
}

void Logger::registrar_evento(const Job& job, std::string evento) {
    mtx_log.lock(); // Garantiza escritura atomica y evita entrelazado de lineas

    auto ahora = std::chrono::high_resolution_clock::now().time_since_epoch();
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(ahora).count();

    std::string string_prio = (job.prioridad == 1) ? "Premium" : "Free";

    std::string linea_csv = std::to_string(ms) + "," +
                            std::to_string(job.id) + "," +
                            string_prio + "," +
                            evento;

    std::cout << "[" << ms << "] - Job ID: " << job.id
              << " (" << string_prio << ") -> Estado: " << evento << std::endl;

    std::ofstream archivo(archivo_nombre, std::ios::app);
    if (archivo.is_open()) {
        archivo << linea_csv << "\n";
        archivo.close();
    }

    mtx_log.unlock();
}
