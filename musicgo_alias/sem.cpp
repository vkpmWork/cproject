/*
 * tsem.cpp
 *
 *  Created on: 03.07.2018
 *      Author: irina
 */

#include "sem.h"

tsem::tsem(int max_res, int id, const char* identify)
    {
    sid = -1;
    res_count = 0;
    /*
    получаем ключ для семафора
    */
    if ((key = ftok(identify,id)) == -1) return;

    /*
    0666 - rw для всех, чтобы потом можно было обратиться к семафору
    от любого пользователя системы, в общем то личное дело каждого
    сначала пытаемся открыть имеющийся семафор с таким ключом - и
    удалить его, старый нам ни к чему
    */
    if ((sid = semget(key, 0, 0666)) != -1)
        {
        if (semctl(sid, 0, IPC_RMID, 0) == -1)
            {
            sid = 0;
            }
        }

    if (sid != 0)    // проверяем что семафор был найден и удален или не существовал
        {
        /*
        создаем с флагом IPC_EXCL - означает что в случае если семафор уже
        имеется - вызов будет провален c значением EEXIST,
        без возврата значения отрытого уже существующего семафора
        */
        if ((sid = semget( key, 1, IPC_CREAT | IPC_EXCL | 0666 )) != -1)    //
            {
            union semun semopts;
                semopts.val = max_res;
              semctl(sid, 0, SETVAL, semopts);
              res_count = max_res;
            }
          } else sid = -1;
    }

tsem::~tsem()
    {
    if (sid != -1)
        {
        semctl(sid, 0, IPC_RMID, 0);
        }
    }

bool tsem::lock(int res)
    {
    /*
    отсекаем неверные запросы сразу, не используя обращения к структурам семафора
    */
    if ((res > res_count) || (sid == -1))
        {
        return false;
        }
    /*
    параметры в структуре
    0 - номер семафора
    количество ресурсов - если захватываем, должно быть отрицательным
    0 - ждать если на данный момент нет достаточного
        количества ресурсов или IPC_NOWAIT - возвращать ошибку
    */
    struct sembuf sem_lock={0,(-1)*res,0};
    /*
    параметры запроса
    - идентификатор семафора
    - структура которую заполняли выше - ее адрес
    - сколько раз выполнить операцию
    */
    if ((semop(sid, &sem_lock, 1)) == -1)
          {
          return false;
          }
      return true;
    }

bool tsem::unlock(int res)
{
    /*
    аналогично функции lock
    */
    if ((res > res_count) || (sid == -1))
        {
        return false;
        }
    struct sembuf sem_unlock= { 0, res, IPC_NOWAIT};
      if ((semop(sid, &sem_unlock, 1)) == -1)
          {
          return false;
         }
    return true;
}
