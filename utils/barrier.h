#ifndef BARRIER_H
#define BARRIER_H


/**
 * Барьер порядка исполнения инструкций чтения-записи.
 */
#define barrier() __asm__ volatile("":::"memory")


#endif /* BARRIER_H */
