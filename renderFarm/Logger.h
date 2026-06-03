#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <mutex>
#include "Job.h"

class Logger {
private:
    std::mutex mtx_log;
    std::string archivo_nombre;

public:
    Logger(std::string nombre);
    void inicializar_archivo();
    void registrar_evento(const Job& job, std::string evento);
};

#endif
