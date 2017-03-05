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
#include <list>
#include <thread>
#include <mutex>
#include <iostream>
#include <fstream>
#include <stdio.h>

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
using std::list;

using mesos::Resources;


std::mutex _lock;
uint64_t queuedTasksNumber = 0;

int receivedOffers = 0;
int totalOffersReceived = 0;

uint64_t totalTasksLaunched = 0;
uint64_t totalOffersDeclined = 0;
uint64_t totalOffersAccepted = 0;
uint64_t totalOffersUnused = 0;

uint64_t allocationRunNumber = 0;
uint64_t tasksLaunched = 0;
uint64_t offersDeclined = 0;
uint64_t offersAccepted = 0;
uint64_t offersUnused = 0;

enum class FrameworkType { COMMON, LOW};

std::ostream& operator<<(std::ostream& os, FrameworkType f)
{
    switch(f)
    {
        case FrameworkType::COMMON   : os << "common";    break;
        case FrameworkType::LOW      : os << "low";       break;
        default                     : os.setstate(std::ios_base::failbit);
    }
    return os;
}


FrameworkType frameworkType;

double cpusTaskDemand;
Bytes memTaskDemand;
Resources TASK_RESOURCES;
int maxOffersReceivable;
Option<string> statsFilepath;

static const double CPUS_PER_EXECUTOR = 0.1;
static const int32_t MEM_PER_EXECUTOR = 32;

std::default_random_engine taskNumberGenerator;
std::default_random_engine tasksInterarrivalTimeGenerator;
std::default_random_engine taskDurationGenerator;


// Tasks number distributions
std::exponential_distribution<double> taskNumberExpDistribution05(0.5);
std::uniform_int_distribution<uint64_t>
  taskNumberUniformDistribution1_10(1, 100);

// Interarrival time distributions
std::lognormal_distribution<double>
  tasksInterarrivalTimeLogNormDistribution60(4.064, 0.25);
std::exponential_distribution<double>
  tasksInterarrivalTimerExpDistribution05(0.5);

// Task duration distributions
std::lognormal_distribution<double>
  taskDurationLogNormDistribution60(4.064, 0.25);
std::exponential_distribution<double> taskDurationExpDistribution05(0.5);


// Tasks number generators

// Get a tasks number from an exponential distribution with u=0.5.
// Return values in [shift, +inf).
uint64_t generateExponentialTasksNumber05(uint64_t shift)
{
  double value = taskNumberExpDistribution05(taskNumberGenerator);
  return static_cast<uint64_t>(value + shift);
}

// Get a tasks number from a uniform distribution [1, 10].
uint64_t generateUniformTasksNumber1_10()
{
  return taskNumberUniformDistribution1_10(taskNumberGenerator);
}

// Interarrival time generators

// Get the task interarrival time from an lognormal distribution
// with m=4.064,s=0.25.
// The mean value is about 60.
// Return values in [0, +inf).
uint64_t generateLognormalTasksInterarrivalTime60()
{
  uint64_t value =
      static_cast<uint64_t>(tasksInterarrivalTimeLogNormDistribution60(
          tasksInterarrivalTimeGenerator));

  return value * 60;
}

// Get a tasks number from an exponential distribution with u=0.5.
// Return values in [shift, +inf).
uint64_t generateExponentialTasksInterarrivalTime05(uint64_t shift)
{
  double value = tasksInterarrivalTimerExpDistribution05(taskNumberGenerator);
  return static_cast<uint64_t>(value + shift);
}

// Task duration generators

// Get the task duration from an lognormal distribution with m=4.064,s=0.25.
// The mean value is about 60.
// Return values in [0, +inf).
uint64_t generateLognormalTasksDuration60()
{
  return static_cast<uint64_t>
    (taskDurationLogNormDistribution60(taskDurationGenerator));
}

// Get the task duration from an exponential distribution with u=0.5.
// Return values in [shift, +inf).
uint64_t generateExpTasksDuration05(uint64_t shift)
{
  double value = taskDurationExpDistribution05(taskDurationGenerator);
    return static_cast<uint64_t>(value + shift);
}

// Generate functions

uint64_t generateTasksNumber()
{
  if (frameworkType == FrameworkType::COMMON)
    return generateExponentialTasksNumber05(5);
  else if (frameworkType == FrameworkType::LOW)
    return generateUniformTasksNumber1_10();
  else
    exit(EXIT_FAILURE);
}

uint64_t generateTasksInterarrivalTime()
{
  if (frameworkType == FrameworkType::COMMON)
    return generateLognormalTasksInterarrivalTime60();
  else if (frameworkType == FrameworkType::LOW)
    return generateExponentialTasksInterarrivalTime05(3);
  else
    exit(EXIT_FAILURE);
}

uint64_t getTaskDuration()
{
  if (frameworkType == FrameworkType::COMMON)
    return generateLognormalTasksDuration60();
  else if (frameworkType == FrameworkType::LOW)
    return generateExpTasksDuration05(10);
  else
    exit(EXIT_FAILURE);
}

///////

uint64_t fillTasksList() {
  uint64_t newTasks = generateTasksNumber();
  _lock.lock();
  queuedTasksNumber += newTasks;
  _lock.unlock();
  return newTasks;
}

void dequeueTaskFromList() {
  _lock.lock();
  queuedTasksNumber -= 1;
  _lock.unlock();
}

void requeueTaskIntoList() {
  _lock.lock();
  queuedTasksNumber += 1;
  _lock.unlock();
}

uint64_t getQueuedTasks() {
  _lock.lock();
  uint64_t  value = queuedTasksNumber;
  _lock.unlock();

  return value;
}

void printTotalStats()
{
  LOG(INFO) << endl
            << "Total tasks launched = "   << totalTasksLaunched   << endl
            << "Total offers received = "  << totalOffersReceived  << endl
            << "Total offers declined = "  << totalOffersDeclined  << endl
            << "Total offers accepted = "  << totalOffersAccepted  << endl
            << "Total offers unused = "    << totalOffersUnused;
}

void resetStats() {
  receivedOffers  = 0;
  tasksLaunched   = 0;
  offersDeclined  = 0;
  offersAccepted  = 0;
  offersUnused    = 0;
}

void printStats()
{
  LOG(INFO) << endl
            << "Allocation run#"     << allocationRunNumber << endl
            << "Tasks launched = "   << tasksLaunched   << endl
            << "Offers received= "   << receivedOffers  << endl
            << "Offers declined = "  << offersDeclined  << endl
            << "Offers accepted = "  << offersAccepted  << endl
            << "Offers unused = "    << offersUnused;
}

void printOnFile() {
  if (statsFilepath.isNone())
    return;

  std::ofstream myfile;
  myfile.open (statsFilepath.get(),  std::ofstream::app);
  if (!myfile.is_open())
    LOG(ERROR) << "Error opening the file to ouptut stats.";
  else {
    myfile << receivedOffers << " "
           << offersDeclined << " "
           << offersAccepted << " "
           << offersUnused << endl;
    myfile.close();
  }
}


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

  virtual void resourceOffers(SchedulerDriver* driver,
                                const vector<Offer>& offers)
  {
    allocationRunNumber++;
    foreach (const Offer& offer, offers) {
      receivedOffers++;
      totalOffersReceived++;

      if (totalOffersReceived > maxOffersReceivable) {
        driver->stop();
      }

      LOG(INFO) << "Received offer "
                << offer.id() << " with "
                << offer.resources();

      Resources remaining = offer.resources();

      // Launch tasks.
      vector<TaskInfo> tasksToLaunch;
      tasksToLaunch.reserve(10);
      bool insufficientResources = false;

      while (getQueuedTasks() > 0 && allocatable(remaining)) {
        dequeueTaskFromList();
        if (remaining.contains(TASK_RESOURCES)) {
          tasksLaunched++;
          uint64_t taskId = totalTasksLaunched++;
          TaskInfo task;
          task.set_name("Task " + lexical_cast<string>(taskId));
          task.mutable_task_id()->set_value(lexical_cast<string>(taskId));
          task.mutable_slave_id()->MergeFrom(offer.slave_id());
          task.mutable_executor()->MergeFrom(executor);
          uint64_t taskDuration = getTaskDuration();
          task.set_data(lexical_cast<string>(taskDuration));

          LOG(INFO) << "Matching task ID " << taskId
                    << " with duration " << taskDuration << "secs "
                    << " asking for cpu:" << cpusTaskDemand << ", mem:"
                    << memTaskDemand.megabytes() << "MB"
                    << " with offer " << offer.id();

          Option<Resources> resources = remaining.find(TASK_RESOURCES);
          CHECK_SOME(resources);

          task.mutable_resources()->MergeFrom(resources.get());
          remaining -= resources.get();
          tasksToLaunch.push_back(task);

          LOG(INFO) << "Resources remaining in offer " << offer.id()
                   << " : " << remaining;
        }
        else {
          requeueTaskIntoList();
          insufficientResources = true;
          break;
        }
      }
      if (!tasksToLaunch.empty()) {
        offersAccepted++;
        totalOffersAccepted++;
        Filters filter;
        filter.set_refuse_seconds(0.0);
        driver->launchTasks(offer.id(), tasksToLaunch, filter);
      }
      else {
        if (insufficientResources) {
          offersDeclined++;
          totalOffersDeclined++;
          LOG(WARNING) << "Offer refused!!! (unable to launch tasks"
                       << " using offer " << offer.id() << " )";
        }
        else {
          offersUnused++;
          totalOffersUnused++;
          LOG(WARNING) << "Offer refused!!! (no task scheduled to start)";
        }
        Filters filter;
        filter.set_refuse_seconds(0.0);
        driver->declineOffer(offer.id(), filter);
      }
    }
    printStats();
    printOnFile();
    resetStats();
    printTotalStats();
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


void run()
{
  while (true) {
    LOG(INFO) << "New tasks queued="   << fillTasksList();
    LOG(INFO) << "Total tasks queued=" << getQueuedTasks();
    os::sleep(Seconds(generateTasksInterarrivalTime()));
  }
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

  flags.add(&memTaskDemand,
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

  flags.add(&cpusTaskDemand,
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

  flags.add(&statsFilepath,
         "offers_stats_file",
         "The absolute filepath (i.e. /path/filename) to the file where to "
         "write stats. NB: if not specified no stats will be printed on file."
         );

  Option<string> frameworkTypeString;
  flags.add(&frameworkTypeString,
           "framework_type",
           "The framework type to launch. This imply the use of predefined "
           "distributions to generate tasks/duration/interarrival time. "
           "Each type has its own distributions.\n"
           "Options are: common, low."
           );

  Try<flags::Warnings> load = flags.load(None(), argc, argv);

  if (load.isError()) {
    cerr << load.error() << endl;
    usage(argv[0], flags);
    exit(EXIT_FAILURE);
  } else if (master.isNone()) {
    cerr << "Missing --master" << endl;
    usage(argv[0], flags);
    exit(EXIT_FAILURE);
  } else if (frameworkTypeString.isNone()) {
    cerr << "Missing --framework_type" << endl;
    usage(argv[0], flags);
    exit(EXIT_FAILURE);
  }

  if (frameworkTypeString.get().compare("common") == 0)
    frameworkType = FrameworkType::COMMON;
  else if (frameworkTypeString.get().compare("low") == 0)
    frameworkType = FrameworkType::LOW;
  else {
    cerr << "value for --framework_type unrecognized" << endl;
    usage(argv[0], flags);
    exit(EXIT_FAILURE);
  }

  LOG(INFO) << "Framework type selected: " << frameworkType;


  if (statsFilepath.isSome()) {
    if (remove(statsFilepath.get().c_str()) == 0 )
      LOG(INFO) << "File " << statsFilepath.get() << " successfully deleted";
  }

  TASK_RESOURCES = Resources::parse(
              "cpus:" + stringify(cpusTaskDemand) +
              ";mem:" + stringify(memTaskDemand.megabytes())).get();

  LOG(INFO) << "Task resources for this framework will be: " << TASK_RESOURCES;

  internal::logging::initialize(argv[0], flags, true); // Catch signals.

  // Log any flag warnings (after logging is initialized).
  foreach (const flags::Warning& warning, load->warnings) {
    LOG(WARNING) << warning.message;
  }

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

  std::thread thread([=]() {
       run();
     });
  thread.detach();

  int status = driver->run() == DRIVER_STOPPED ? 0 : 1;

  // Ensure that the driver process terminates.
  driver->stop();

  delete driver;
  return status;
}
