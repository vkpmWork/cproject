/*
 * sem.h
 *
 *  Created on: 03.07.2018
 *      Author: irina
 */

#ifndef SEM_H_
#define SEM_H_

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

// semafore
#include <ctype.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>

union semun {
    int      val;            /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
         (Linux specific) */
    };


class tsem
    {
    private:
        int sid;       // идентификатор семафора
        key_t key;     // ключ по которому получаем идентификатор
        int res_count; // количество ресурсов у семафора
    public:
        /*
        кол-во ресурсов
        некое случайное число
        путь в системе - обязательно должен существовать !
        */
        tsem(int max_res, int id, const char* identify);
        /*
        деструктор - тут удаляем семафор, иначе он останется до
        следующей перезагрузки системы или пока его кто-то явно не удалит
        */
        ~tsem();
        /*
        обертки для занятия/освобождения ресурсов
        */
        bool lock(int res);
        bool unlock(int res);
    };

typedef tsem* psem;

#endif /* SEM_H_ */
