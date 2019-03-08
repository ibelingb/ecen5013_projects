/***********************************************************************************
 * @author Brian Ibeling and Joshua Malburg (joma0364)
 * brian.ibeling@colorado.edu
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 8, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file main.c
 * @brief main for project 1 - Multithreaded sensor sampling application
 *
 ************************************************************************************
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "tempThread.h"
#include "lightThread.h"
#include "remoteThread.h"
#include "loggingThread.h"
#include "packet.h"

/* Define static and global variables */


int main(int argc, char *argv[]){
  TaskStatusPacket tempThreadStatus;
  TaskStatusPacket lightThreadStatus;
  TaskStatusPacket remoteThreadStatus;
  TaskStatusPacket loggingThreadStatus;

  /* Check inputs */
  // TODO

  /* Initialize created structs to 0 */
  memset(&tempThreadStatus, 0, sizeof(struct TaskStatusPacket));
  memset(&lightThreadStatus, 0, sizeof(struct TaskStatusPacket));
  memset(&remoteThreadStatus, 0, sizeof(struct TaskStatusPacket));
  memset(&loggingThreadStatus, 0, sizeof(struct TaskStatusPacket));

  /* Create all pipes and message queues used by threads for IPC */
  /* Create pipe to receive thread status */

  /* Populate ThreadInfo objects to pass names for created IPC pieces to threads */

  /* Create threads */

  /* Join threads */

  /* Verify all threads have started successfully */

  /* Periodically get threads status, send to logging thread */
  // TODO Timer?

}

