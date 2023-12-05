/*
 *  cpp_main.h
 *
 *  Created on: Oct 27, 2023
 *      Author: sen
 */

#ifndef SRC_USER_TASK_H
#define SRC_USER_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

void UserTask_setup(void);

void UserTask_loop(void);

void UserTask_unsafeLoop(void);

#ifdef __cplusplus
}
#endif


#endif /* SRC_USER_TASK_H */
