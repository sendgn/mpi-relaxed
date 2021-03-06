/*
 * relaxed_queue.h Relaxed distributed queue implementation on MPI
 *
 * (C) 2017 Alexey Paznikov <apaznikov@gmail.com> 
 */

#pragma once

#include <mpi.h>

#include "common.h"

enum {
    CIRCBUF_STARTSIZE     = 1000000,     /* Initial size of circbuf */
    // CIRCBUF_STARTSIZE     = 40,     /* Initial size of circbuf */
    /* Number of queues from which we get the elements during remove operation,
     * compare it and choose the best */
    NQUEUES_REMOVE        = 2,

    CODE_SUCCESS          = 0,
    CODE_ERROR            = 1,
    CODE_CIRCBUF_FULL     = 2,
    CODE_CIRCBUF_EMPTY    = 3,
    CODE_CIRCBUF_BUSY     = 4,

    CODE_TRYLOCK_SUCCESS  = 0,
    CODE_TRYLOCK_BUSY     = 1,

    LOCK_UNLOCKED         = 0,
    LOCK_LOCKED           = 1
};

typedef int val_t;

typedef struct {
    val_t val;                  /* Element's value */
    double ts;                  /* Timestamp */
} elem_t;

/* Current state of the circular buffer */
typedef struct {
    int head;                   /* Write pointer */
    int tail;                   /* Read pointer */
    int size;                   /* Max number of elements */
} circbuf_state_t;

/* Auxiliary buffers for lock. */
typedef struct {
    int state;
    int unlocked; 
    int locked;
    int result;
} lock_t;

/* Circular buffer. */
typedef struct {
    MPI_Aint basedisp_local;    /* Base address of circbuf (local) */
    MPI_Aint *basedisp;         /* Base address of circbuf (all processes) */
    MPI_Aint *lockdisp;         /* Displacements for lock state 
                                   (all processes) */
    elem_t *buf;                /* Physical buffer */
    MPI_Aint datadisp_local;    /* Address of buf (data) (local) */
    MPI_Aint *datadisp;         /* Address of buf (data) (all processes) */
    circbuf_state_t state;      /* Current state of circbuf */
    lock_t lock;                /* Spinlock variable */
    MPI_Win win;                /* RMA access window */
    MPI_Comm comm;              /* Communicator for the circbuf distribution */
    int nproc;                  /* Number of processes in communicator */
    double ts_offset;           /* Timestamp offset from 0 process */
    int nqueues_remove;         /* Number of queues for remove candidiates */
    int max_attempts;           /* Max number of attempts to lock the mutex */
} circbuf_t;

/* Process-oblivious circular buffer info */
typedef struct {
    size_t lock_state_offset;
    size_t state_offset;
    size_t head_offset;
    size_t tail_offset;
    size_t buf_offset;
} circbuf_info_t;

/* circbuf_init: Init circular buffer with specified size. */
int circbuf_init(circbuf_t **circbuf, int size, MPI_Comm comm);

/* circbuf_free: Free memory and so on. */
void circbuf_free(circbuf_t *circbuf);

/* circbuf_insert: Choose randomly the queue and insert element into it. */
int circbuf_insert(val_t val, circbuf_t *circbuf);

/* circbuf_remove: */
int circbuf_remove(val_t *val, circbuf_t *circbuf);

/* circbuf_insert: Insert an element to the tail of the circular buffer 
 * on specified process. */
int circbuf_insert_proc(elem_t elem, circbuf_t *circbuf, int rank);

/* circbuf_remove: Remove an element from the circular buffer
 * on specified process. */
int circbuf_remove_proc(elem_t *elem, circbuf_t *circbuf, int rank);

/* circbuf_print: Print circbuf (useful for debug) */
void circbuf_print(circbuf_t *circbuf, const char *label);

/* error_msg: Print error message. */
void error_msg(const char *msg, int _errno);
