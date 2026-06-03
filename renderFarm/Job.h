#ifndef JOB_H
#define JOB_H

struct Job {
    int id;
    int prioridad; // 1 = Premium, 0 = Free
    long long timestamp_creacion; // Almacena el tiempo de ingreso para el control de inanicion
};

#endif
