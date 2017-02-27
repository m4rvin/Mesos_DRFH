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
#include <string>
#include <random>
#include <functional>


#include <boost/lexical_cast.hpp>

#include <mesos/resources.hpp>
#include <mesos/scheduler.hpp>
#include <mesos/type_utils.hpp>

#include "master/constants.hpp"

#include <stout/check.hpp>
#include <stout/exit.hpp>
#include <stout/flags.hpp>
#include <stout/numify.hpp>
#include <stout/option.hpp>
#include <stout/os.hpp>
#include <stout/path.hpp>
#include <stout/stringify.hpp>

#include "logging/flags.hpp"
#include "logging/logging.hpp"

using namespace mesos;

using boost::lexical_cast;

using std::cerr;
using std::cout;
using std::endl;
using std::flush;
using std::string;
using std::vector;

using mesos::Resources;

long totalTasksLaunched = 0;

double cpusDemand;
Bytes memDemand;
Resources TASK_RESOURCES;

int maxOffersReceivable;

static const double CPUS_PER_EXECUTOR = 0.1;
static const int32_t MEM_PER_EXECUTOR = 32;

std::default_random_engine taskNumberGenerator;
static const int A_PARAM = 1;
static const int B_PARAM = 10;
std::uniform_int_distribution<int> taskNumberDistribution(1, 10);
auto generateTasksNumber =
    std::bind (taskNumberDistribution, taskNumberGenerator);

std::default_random_engine cpuGenerator;
// std::uniform_int_distribution<int32_t> cpuDistribution(1,10);
static const double LAMBDA_PARAM = 1.3;
std::exponential_distribution<double> cpuDistribution(LAMBDA_PARAM);
// NB: the following will generate numbers starting from 0, so add +1.
auto generateTaskCPU = std::bind (cpuDistribution, cpuGenerator);

std::default_random_engine memGenerator;
// std::uniform_int_distribution<int32_t> memDistribution(128,512);
static const double MEAN_PARAM = 1024.0;
static const double STDDEV_PARAM = 512.0;
std::normal_distribution<double> memDistribution(MEAN_PARAM, STDDEV_PARAM);
auto generateTaskMEM = std::bind (memDistribution, memGenerator);

class TestScheduler : public Scheduler
{
public:
  TestScheduler(
      bool _implicitAcknowledgements,
      const ExecutorInfo& _executor,
      const string& _role)
    : implicitAcknowledgements(_implicitAcknowledgements),
      executor(_executor),
      role(_role),
      tasksFinished(0),
      receivedOffers(0),
      waitingTasksNumber(0) {}

  virtual ~TestScheduler() {}

  virtual void registered(SchedulerDriver*,
                          const FrameworkID&,
                          const MasterInfo&)
  {
    cout << "Registered!" << endl;
  }

  virtual void reregistered(SchedulerDriver*, const MasterInfo& masterInfo) {}

  virtual void disconnected(SchedulerDriver* driver) {}

  /*virtual void resourceOffers(SchedulerDriver* driver,
                              const vector<Offer>& offers)
  {
    foreach (const Offer& offer, offers) {
      cout << "Received offer " << offer.id() << " with " << offer.resources()
           << endl;

      static const Resources TASK_RESOURCES = Resources::parse(
          "cpus:" + stringify(CPUS_PER_TASK) +
          ";mem:" + stringify(MEM_PER_TASK)).get();

      Resources remaining = offer.resources();

      // Launch tasks.
      vector<TaskInfo> tasks;
      while (tasksLaunched < totalTasks &&
             remaining.flatten().contains(TASK_RESOURCES)) {
      if (remaining.flatten().contains(TASK_RESOURCES)) {
        int taskId = tasksLaunched++;

        cout << "Launching task " << taskId << " using offer "
             << offer.id() << endl;

        TaskInfo task;
        task.set_name("Task " + lexical_cast<string>(taskId));
        task.mutable_task_id()->set_value(lexical_cast<string>(taskId));
        task.mutable_slave_id()->MergeFrom(offer.slave_id());
        task.mutable_executor()->MergeFrom(executor);

        Try<Resources> flattened = TASK_RESOURCES.flatten(role);
        CHECK_SOME(flattened);
        Option<Resources> resources = remaining.find(flattened.get());

        CHECK_SOME(resources);
        task.mutable_resources()->MergeFrom(resources.get());
        remaining -= resources.get();

        tasks.push_back(task);
      }

      driver->launchTasks(offer.id(), tasks);
    }
  }
*/

/*
  virtual void resourceOffers(SchedulerDriver* driver,
                                const vector<Offer>& offers)
    {
      foreach (const Offer& offer, offers) {
        receivedOffers++;
        if (receivedOffers > MAX_OFFERS_RECEIVABLE) {
          driver->stop();
        }

        LOG(INFO) << "Received offer "
                  << offer.id() << " with "
                  << offer.resources();

        Resources remaining = offer.resources();

        // Launch tasks.
        int tasksToLaunch = generateTasksNumber();
        LOG(INFO) << "I will try to launch " << tasksToLaunch
                  << " tasks using offer " << offer.id();
        int launchedTasks = 0;
        vector<TaskInfo> tasks;
        // FIXME tasks.reserve(tasksToLaunch);

        while (allocatable(remaining) && launchedTasks < tasksToLaunch) {
          int32_t cpu = static_cast<int32_t>(generateTaskCPU()) + 1;
          CHECK(cpu > 0);
          int32_t mem = 0;
          do{
            mem = static_cast<int32_t>(generateTaskMEM());
          }
          while (mem < 128); // FIXME check value 128

          const Resources TASK_RESOURCES = Resources::parse(
                      "cpus:" + stringify(cpu) +
                      ";mem:" + stringify(mem)).get();
          LOG(INFO) << "Randomly generated task resources: " << TASK_RESOURCES;

          // if(!remaining.flatten().contains(TASK_RESOURCES))
          if(!remaining.contains(TASK_RESOURCES)) {
            // It could happen that the cpu or mem field is absent if its value
            // is 0, so we cannot simply print remaining but we have to check
            // the fields' existence.
            double remainingCpu = remaining.cpus().isSome() ?
                remaining.cpus().get() : 0;
            uint64_t remainingMem = remaining.mem().isSome() ?
                remaining.mem().get().megabytes() : 0;

            LOG(WARNING) << "Unable to launch the desired group of tasks "
                      << "because one of them request more resources "
                      << "than available"
                      << " (i.e. cpu:" << cpu << " mem:" << mem << "MB "
                      << "over remaining cpu:" << remainingCpu
                      << " mem:" << remainingMem << "MB)";
            break;
          }

          launchedTasks++;

          int taskId = totalTasksLaunched++;
          TaskInfo task;
          task.set_name("Task " + lexical_cast<string>(taskId));
          task.mutable_task_id()->set_value(lexical_cast<string>(taskId));
          task.mutable_slave_id()->MergeFrom(offer.slave_id());
          task.mutable_executor()->MergeFrom(executor);

          LOG(INFO) << "Matching task ID " << taskId
                    << " asking for cpu:" << cpu << ", mem:" << mem << "MB"
                    << " with offer " << offer.id();

          Option<Resources> resources = remaining.find(TASK_RESOURCES);
          CHECK_SOME(resources);
          LOG(INFO) << "Found resources needed by task: "
                    << taskId << " : " << resources.get();

          task.mutable_resources()->MergeFrom(resources.get());
          remaining -= resources.get();
          tasks.push_back(task);

          LOG(INFO) << "Resources remaining in offer " << offer.id()
                    << " : " << remaining;


          Try<Resources> flattened = TASK_RESOURCES.flatten(role);
          CHECK_SOME(flattened);
          Option<Resources> resources = remaining.find(flattened.get());

        }
        if(launchedTasks == tasksToLaunch)
        {
          Filters filter;
          filter.set_refuse_seconds(0.0);
          driver->launchTasks(offer.id(), tasks, filter);
        }
        else
        {
          LOG(WARNING) << "Offer refused!!! (unable to launch "
                       << tasksToLaunch << " tasks using offer "
                       << offer.id() << ")";
          Filters filter;
          filter.set_refuse_seconds(0.0);
          driver->declineOffer(offer.id(), filter);
        }
      }
    }
*/

  virtual void resourceOffers(SchedulerDriver* driver,
                                const vector<Offer>& offers)
    {
      foreach (const Offer& offer, offers) {
        receivedOffers++;
        if (receivedOffers > maxOffersReceivable) {
          driver->stop();
        }

        LOG(INFO) << "Received offer "
                  << offer.id() << " with "
                  << offer.resources();

        Resources remaining = offer.resources();

        // Launch tasks.
        int tasksToLaunch = waitingTasksNumber != 0
            ? waitingTasksNumber : generateTasksNumber();

        LOG(INFO) << "I will try to launch " << tasksToLaunch
                  << " tasks using offer " << offer.id();

        int launchableTasks = 0;
        vector<TaskInfo> tasks;
        tasks.reserve(tasksToLaunch);

        while (allocatable(remaining) && launchableTasks < tasksToLaunch) {
          if(!remaining.contains(TASK_RESOURCES)) {
            /*
            // It could happen that the cpu or mem field is absent if its value
            // is 0, so we cannot simply print remaining but we have to check
            // the fields' existence.
            double remainingCpu = remaining.cpus().isSome() ?
                remaining.cpus().get() : 0;
            uint64_t remainingMem = remaining.mem().isSome() ?
                remaining.mem().get().megabytes() : 0;
            */
            break;
          }

          launchableTasks++;

          int taskId = totalTasksLaunched + launchableTasks;
          TaskInfo task;
          task.set_name("Task " + lexical_cast<string>(taskId));
          task.mutable_task_id()->set_value(lexical_cast<string>(taskId));
          task.mutable_slave_id()->MergeFrom(offer.slave_id());
          task.mutable_executor()->MergeFrom(executor);

          LOG(INFO) << "Matching task ID " << taskId
                    << " asking for cpu:" << cpusDemand
                    << ", mem:" << memDemand.megabytes() << "MB"
                    << " with offer " << offer.id();

          Option<Resources> resources = remaining.find(TASK_RESOURCES);
          CHECK_SOME(resources);
         /* LOG(INFO) << "Found resources needed by task: "
                    << taskId << " : " << resources.get();*/

          task.mutable_resources()->MergeFrom(resources.get());
          remaining -= resources.get();
          tasks.push_back(task);

          LOG(INFO) << "Resources remaining in offer " << offer.id()
                    << " : " << remaining;
        }
        if(launchableTasks == tasksToLaunch)
        {
          Filters filter;
          filter.set_refuse_seconds(0.0);
          driver->launchTasks(offer.id(), tasks, filter);

          totalTasksLaunched += launchableTasks;
        }
        else
        {
          LOG(WARNING) << "Offer refused!!! (unable to launch "
                       << tasksToLaunch << " tasks using offer "
                       << offer.id() << ")";
          Filters filter;
          filter.set_refuse_seconds(0.0);
          driver->declineOffer(offer.id(), filter);
        }
      }
    }

  virtual void offerRescinded(SchedulerDriver* driver,
                              const OfferID& offerId) {}

  virtual void statusUpdate(SchedulerDriver* driver, const TaskStatus& status)
  {
    int taskId = lexical_cast<int>(status.task_id().value());

    cout << "Task " << taskId << " is in state " << status.state() << endl;

    if (status.state() == TASK_FINISHED) {
      tasksFinished++;
    }

    if (status.state() == TASK_LOST ||
        status.state() == TASK_KILLED ||
        status.state() == TASK_FAILED) {
      cout << "Aborting because task " << taskId
           << " is in unexpected state " << status.state()
           << " with reason " << status.reason()
           << " from source " << status.source()
           << " with message '" << status.message() << "'"
           << endl;
      driver->abort();
    }

    if (!implicitAcknowledgements) {
      driver->acknowledgeStatusUpdate(status);
    }

    /*if (tasksFinished == totalTasks) {
      driver->stop();
    }*/
  }

  virtual void frameworkMessage(SchedulerDriver* driver,
                                const ExecutorID& executorId,
                                const SlaveID& slaveId,
                                const string& data) {}

  virtual void slaveLost(SchedulerDriver* driver, const SlaveID& sid) {}

  virtual void executorLost(SchedulerDriver* driver,
                            const ExecutorID& executorID,
                            const SlaveID& slaveID,
                            int status) {}

  virtual void error(SchedulerDriver* driver, const string& message)
  {
    cout << message << endl;
  }

private:
  const bool implicitAcknowledgements;
  const ExecutorInfo executor;
  string role;
  // int tasksLaunched;
  int tasksFinished;
  // int totalTasks;
  int receivedOffers;
  int waitingTasksNumber;

  bool allocatable(
      const Resources& resources)
  {
    Option<double> cpus = resources.cpus();
    Option<Bytes> mem = resources.mem();

    return (cpus.isSome() && cpus.get() >= mesos::internal::master::MIN_CPUS) ||
           (mem.isSome() && mem.get() >= mesos::internal::master::MIN_MEM);
  }
};


void usage(const char* argv0, const flags::FlagsBase& flags)
{
  cerr << "Usage: " << Path(argv0).basename() << " [...]" << endl
       << endl
       << "Supported options:" << endl
       << flags.usage();
}


int main(int argc, char** argv)
{
  // Find this executable's directory to locate executor.
  string uri;
  Option<string> value = os::getenv("MESOS_HELPER_DIR");
  if (value.isSome()) {
    uri = path::join(value.get(), "test-executor-drfh");
  } else {
    uri = path::join(
        os::realpath(Path(argv[0]).dirname()).get(),
        "test-executor-drfh");
  }

  mesos::internal::logging::Flags flags;

  string role;
  flags.add(&role,
            "role",
            "Role to use when registering",
            "*");

  Option<string> master;
  flags.add(&master,
            "master",
            "ip:port of master to connect");

  flags.add(&memDemand,
          "task_memory_demand",
          None(),
          "Size, in bytes (i.e. B, MB, GB, ...), of each task's memory demand.",
          static_cast<const Bytes*>(nullptr),
          [](const Bytes& value) -> Option<Error> {
            if (value.megabytes() < MEM_PER_EXECUTOR) {
              return Error(
                  "Please use a --task_memory_demand greater than " +
                  stringify(MEM_PER_EXECUTOR) + " MB");
            }
            return None();
          });


  flags.add(&cpusDemand,
         "task_cpus_demand",
         None(),
         "How much cpus each task will require.",
         static_cast<const double*>(nullptr),
         [](const double& value) -> Option<Error> {
           if (value < CPUS_PER_EXECUTOR) {
             return Error(
                 "Please use a --task_cpus_demand greater than " +
                 stringify(CPUS_PER_EXECUTOR));
           }
           return None();
         });

  flags.add(&maxOffersReceivable,
         "offers_limit",
         None(),
         "How many offers to process before asking to stop the driver and "
         "close the framework.",
         static_cast<const int*>(nullptr),
         [](const int& value) -> Option<Error> {
           if (value <= 0) {
             return Error(
                 "Please use a --offers_limit greater than " +
                 stringify(0));
           }
           return None();
         });


  Try<flags::Warnings> load = flags.load(None(), argc, argv);

  if (load.isError()) {
    cerr << load.error() << endl;
    usage(argv[0], flags);
    exit(EXIT_FAILURE);
  } else if (master.isNone()) {
    cerr << "Missing --master" << endl;
    usage(argv[0], flags);
    exit(EXIT_FAILURE);
  }

  TASK_RESOURCES = Resources::parse(
              "cpus:" + stringify(cpusDemand) +
              ";mem:" + stringify(memDemand.megabytes())).get();

  LOG(INFO) << "Task resources for this framework will be: " << TASK_RESOURCES;

  internal::logging::initialize(argv[0], flags, true); // Catch signals.

  // Log any flag warnings (after logging is initialized).
  foreach (const flags::Warning& warning, load->warnings) {
    LOG(WARNING) << warning.message;
  }

  cout << "TEST FRAMEWORK DRFH " << endl << "Configuration:" << endl
       << "uniform distribution for tasks number per offer with a=" << A_PARAM
       << " and b=" << B_PARAM <<endl
       << "exponential distribution for CPU load with lambda=" << LAMBDA_PARAM
       << endl << "gamma distribution for MEM load with alpha=" << MEAN_PARAM
       << " and beta=" << STDDEV_PARAM << endl;

  ExecutorInfo executor;
  executor.mutable_executor_id()->set_value("default");
  executor.mutable_command()->set_value(uri);
  executor.set_name("Test Executor DRFH (C++)");
  // executor.set_source("cpp_test");

  FrameworkInfo framework;
  framework.set_user(""); // Have Mesos fill in the current user.
  framework.set_name("Test Framework DRFH (C++)");
  framework.set_role(role);

  value = os::getenv("MESOS_CHECKPOINT");
  if (value.isSome()) {
    framework.set_checkpoint(
        numify<bool>(value.get()).get());
  }

  bool implicitAcknowledgements = true;
  if (os::getenv("MESOS_EXPLICIT_ACKNOWLEDGEMENTS").isSome()) {
    cout << "Enabling explicit acknowledgements for status updates" << endl;

    implicitAcknowledgements = false;
  }

  MesosSchedulerDriver* driver;
  TestScheduler scheduler(implicitAcknowledgements, executor, role);

  if (os::getenv("MESOS_AUTHENTICATE_FRAMEWORKS").isSome()) {
    cout << "Enabling authentication for the framework" << endl;

    value = os::getenv("DEFAULT_PRINCIPAL");
    if (value.isNone()) {
      EXIT(EXIT_FAILURE)
        << "Expecting authentication principal in the environment";
    }

    Credential credential;
    credential.set_principal(value.get());

    framework.set_principal(value.get());

    value = os::getenv("DEFAULT_SECRET");
    if (value.isNone()) {
      EXIT(EXIT_FAILURE)
        << "Expecting authentication secret in the environment";
    }

    credential.set_secret(value.get());

    driver = new MesosSchedulerDriver(
        &scheduler,
        framework,
        master.get(),
        implicitAcknowledgements,
        credential);
  } else {
    framework.set_principal("test-framework-DRFH-cpp");

    driver = new MesosSchedulerDriver(
        &scheduler,
        framework,
        master.get(),
        implicitAcknowledgements);
  }

  int status = driver->run() == DRIVER_STOPPED ? 0 : 1;

  // Ensure that the driver process terminates.
  driver->stop();

  delete driver;
  return status;
}
