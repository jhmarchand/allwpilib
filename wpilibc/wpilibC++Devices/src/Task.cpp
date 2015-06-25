/*----------------------------------------------------------------------------*/
/* Copyright (c) FIRST 2008. All Rights Reserved.
 */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in $(WIND_BASE)/WPILib.  */
/*----------------------------------------------------------------------------*/

#include "Task.h"

//#include "NetworkCommunication/UsageReporting.h"
#include "WPIErrors.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "HAL/HAL.hpp"

#ifndef OK
#define OK 0
#endif /* OK */
#ifndef ERROR
#define ERROR (-1)
#endif /* ERROR */

const uint32_t Task::kDefaultPriority;

/**
 * Create but don't launch a task.
 * @param name The name of the task.  "FRC_" will be prepended to the task name.
 * @param function The address of the function to run as the new task.
 * @param priority The VxWorks priority for the task.
 * @param stackSize The size of the stack for the task
 */
Task::Task(const char* name, FUNCPTR function, int32_t priority,
           uint32_t stackSize) {
  m_function = function;
  m_priority = priority;
  m_stackSize = stackSize;
  m_taskName = new char[strlen(name) + 5];
  strcpy(m_taskName, "FRC_");
  strcpy(m_taskName + 4, name);

  static int32_t instances = 0;
  instances++;
  HALReport(HALUsageReporting::kResourceType_Task, instances, 0, m_taskName);
}

Task::~Task() {
  if (m_taskID != NULL_TASK) Stop();
  delete[] m_taskName;
  m_taskName = nullptr;
}

/**
 * Starts this task.
 * If it is already running or unable to start, it fails and returns false.
 */
bool Task::Start(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3,
                 uint32_t arg4, uint32_t arg5, uint32_t arg6, uint32_t arg7,
                 uint32_t arg8, uint32_t arg9) {
  m_taskID = spawnTask(
      m_taskName, m_priority,
      VXWORKS_FP_TASK,                // options
      m_stackSize,                    // stack size
      m_function,                     // function to start
      arg0, arg1, arg2, arg3, arg4,   // parameter 1 - pointer to this class
      arg5, arg6, arg7, arg8, arg9);  // additional unused parameters
  if (m_taskID == NULL_TASK) {
    HandleError(ERROR);
    return false;
  }
  return true;
}

/**
 * Restarts a running task.
 * If the task isn't started, it starts it.
 * @return false if the task is running and we are unable to kill the previous
 * instance
 */
bool Task::Restart() { return HandleError(restartTask(m_taskID)); }

/**
 * Kills the running task.
 * @returns true on success false if the task doesn't exist or we are unable to
 * kill it.
 */
bool Task::Stop() {
  bool ok = true;
  if (Verify()) {
    ok = HandleError(deleteTask(m_taskID));
  }
  m_taskID = NULL_TASK;
  return ok;
}

/**
 * Returns true if the task is ready to execute (i.e. not suspended, delayed, or
 * blocked).
 * @return true if ready, false if not ready.
 */
bool Task::IsReady() const { return isTaskReady(m_taskID); }

/**
 * Returns true if the task was explicitly suspended by calling Suspend()
 * @return true if suspended, false if not suspended.
 */
bool Task::IsSuspended() const { return isTaskSuspended(m_taskID); }

/**
 * Pauses a running task.
 * Returns true on success, false if unable to pause or the task isn't running.
 */
bool Task::Suspend() { return HandleError(suspendTask(m_taskID)); }

/**
 * Resumes a paused task.
 * Returns true on success, false if unable to resume or if the task isn't
 * running/paused.
 */
bool Task::Resume() { return HandleError(resumeTask(m_taskID)); }

/**
 * Verifies a task still exists.
 * @returns true on success.
 */
bool Task::Verify() const { return verifyTaskID(m_taskID) == OK; }

/**
 * Gets the priority of a task.
 * @returns task priority or 0 if an error occured
 */
int32_t Task::GetPriority() {
  if (HandleError(getTaskPriority(m_taskID, &m_priority)))
    return m_priority;
  else
    return 0;
}

/**
 * This routine changes a task's priority to a specified priority.
 * Priorities range from 0, the highest priority, to 255, the lowest priority.
 * Default task priority is 100.
 * @param priority The priority the task should run at.
 * @returns true on success.
 */
bool Task::SetPriority(int32_t priority) {
  m_priority = priority;
  return HandleError(setTaskPriority(m_taskID, m_priority));
}

/**
 * Returns the name of the task.
 * @returns Pointer to the name of the task or nullptr if not allocated
 */
const char* Task::GetName() const { return m_taskName; }

/**
 * Get the ID of a task
 * @returns Task ID of this task.  Task::kInvalidTaskID (-1) if the task has not
 * been started or has already exited.
 */
TASK Task::GetID() const {
  if (Verify()) return m_taskID;
  return NULL_TASK;
}

/**
 * Handles errors generated by task related code.
 */
bool Task::HandleError(STATUS results) {
  if (results != ERROR) return true;
  int errsv = errno;
  if (errsv == HAL_objLib_OBJ_ID_ERROR) {
    wpi_setWPIErrorWithContext(TaskIDError, m_taskName);
  } else if (errsv == HAL_objLib_OBJ_DELETED) {
    wpi_setWPIErrorWithContext(TaskDeletedError, m_taskName);
  } else if (errsv == HAL_taskLib_ILLEGAL_OPTIONS) {
    wpi_setWPIErrorWithContext(TaskOptionsError, m_taskName);
  } else if (errsv == HAL_memLib_NOT_ENOUGH_MEMORY) {
    wpi_setWPIErrorWithContext(TaskMemoryError, m_taskName);
  } else if (errsv == HAL_taskLib_ILLEGAL_PRIORITY) {
    wpi_setWPIErrorWithContext(TaskPriorityError, m_taskName);
  } else {
    printf("ERROR: errno=%i", errsv);
    wpi_setWPIErrorWithContext(TaskError, m_taskName);
  }
  return false;
}
