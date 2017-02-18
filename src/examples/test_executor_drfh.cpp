// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <glog/logging.h>

#include <iostream>
#include <random>
#include <thread>

#include <mesos/executor.hpp>

#include <stout/duration.hpp>
#include <stout/os.hpp>

using namespace mesos;

using std::cout;
using std::endl;
using std::string;

std::default_random_engine taskDurationGenerator;
std::uniform_int_distribution<int> taskDurationDistribution(10, 120);
// TODO(danang) make this function thread-safe
auto generateTasksDuration =
    std::bind (taskDurationDistribution, taskDurationGenerator);

void run(ExecutorDriver* driver, const TaskInfo& task)
{
  TaskStatus status;
  status.mutable_task_id()->MergeFrom(task.task_id());
  status.set_state(TASK_RUNNING);

  driver->sendStatusUpdate(status);

  int taskDuration = generateTasksDuration();
  LOG(INFO) << "Task with ID: "
            <<  task.task_id().value()
            <<" is going to work for "
            << taskDuration << " seconds";

  // This is where one would perform the requested task.
  os::sleep(Seconds(taskDuration));

  LOG(INFO) << "Task with ID: "
            << task.task_id().value()
            <<  " terminated its work";

  status.mutable_task_id()->MergeFrom(task.task_id());
  status.set_state(TASK_FINISHED);

  driver->sendStatusUpdate(status);

  // TODO(danang) find a way to stop the driver without killing the process.
}


class TestExecutor : public Executor
{
public:
  virtual ~TestExecutor() {}

  virtual void registered(ExecutorDriver* driver,
                          const ExecutorInfo& executorInfo,
                          const FrameworkInfo& frameworkInfo,
                          const SlaveInfo& slaveInfo)
  {
    cout << "Registered executor on " << slaveInfo.hostname() << endl;
  }

  virtual void reregistered(ExecutorDriver* driver,
                            const SlaveInfo& slaveInfo)
  {
    cout << "Re-registered executor on " << slaveInfo.hostname() << endl;
  }

  virtual void disconnected(ExecutorDriver* driver) {}


  virtual void launchTask(ExecutorDriver* driver, const TaskInfo& task)
  {
    // NOTE: The executor driver calls `launchTask` synchronously, which
    // means that calls such as `driver->sendStatusUpdate()` will not execute
    // until `launchTask` returns.
    std::thread thread([=]() {
      run(driver, task);
    });

    thread.detach();
  }

  virtual void killTask(ExecutorDriver* driver, const TaskID& taskId) {}
  virtual void frameworkMessage(ExecutorDriver* driver, const string& data) {}
  virtual void shutdown(ExecutorDriver* driver) {}
  virtual void error(ExecutorDriver* driver, const string& message) {}
};


int main(int argc, char** argv)
{
  TestExecutor executor;
  MesosExecutorDriver driver(&executor);
  return driver.run() == DRIVER_STOPPED ? 0 : 1;
}
