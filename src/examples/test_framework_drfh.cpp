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

#include <chrono>
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
#include <stout/strings.hpp>

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

std::chrono::steady_clock::time_point start;
std::chrono::duration<double> frameworkDuration;
std::function<double()> generateTaskInterarrivalTime;

std::mutex _lock;
uint64_t queuedTasksNumber = 0;

int receivedOffers = 0;

uint64_t totalTasksLaunched = 0;
uint64_t totalTasksNotLaunched = 0;
uint64_t totalOffersDeclined = 0;
uint64_t totalOffersAccepted = 0;
uint64_t totalOffersUnused = 0;
uint64_t totalOffersReceived = 0;


uint64_t allocationRunNumber = 0;
uint64_t tasksLaunched = 0;
uint64_t tasksNotLaunched = 0;
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
Option<string> statsFilepath;

static const double CPUS_PER_EXECUTOR = 0.1;
static const int32_t MEM_PER_EXECUTOR = 32;

std::mt19937 tasksInterarrivalTimeGenerator;
// std::default_random_engine taskDurationGenerator;

// Interarrival time distributions
/*std::lognormal_distribution<double>
  tasksInterarrivalTimeLogNormDistribution60(4.064, 0.25);
*/
std::exponential_distribution<double>
  tasksInterarrivalTimeExpDistribution_A;

// Task duration distributions
/*std::lognormal_distribution<double>
  taskDurationLogNormDistribution60(4.064, 0.25);
std::exponential_distribution<double> taskDurationExpDistribution05(0.5);
*/

// Interarrival time generators

// Get the task interarrival time from an lognormal distribution
// with m=4.064,s=0.25.
// The mean value is about 60.
// Return values in [0, +inf).
/*uint64_t generateLognormalTasksInterarrivalTime60()
{
  uint64_t value =
      static_cast<uint64_t>(tasksInterarrivalTimeLogNormDistribution60(
          tasksInterarrivalTimeGenerator));

  return value * 60;
}
*/

// Task duration generators

// Get the task duration from an lognormal distribution with m=4.064,s=0.25.
// The mean value is about 60.
// Return values in [0, +inf).
/*uint64_t generateLognormalTasksDuration60()
{
  return static_cast<uint64_t>
    (taskDurationLogNormDistribution60(taskDurationGenerator));
}*/

// Get the task duration from an exponential distribution with u=0.5.
// Return values in [shift, +inf).
/*uint64_t generateExpTasksDuration05(uint64_t shift)
{
  double value = taskDurationExpDistribution05(taskDurationGenerator);
    return static_cast<uint64_t>(value + shift);
}*/

// Generate functions

// Generate interarrival time (nanosecs)
Duration getNextTaskInterarrivalTime()
{
  Try<Duration> time = Duration::create(generateTaskInterarrivalTime());
  CHECK_SOME(time);
  return time.get();
}

/*uint64_t getTaskDuration()
{
  if (frameworkType == FrameworkType::COMMON)
    return generateLognormalTasksDuration60();
  else if (frameworkType == FrameworkType::LOW)
    return generateExpTasksDuration05(10);
  else
    exit(EXIT_FAILURE);
}*/

uint64_t getTaskDuration()
{
  /*if (frameworkType == FrameworkType::COMMON)
    return generateLognormalTasksDuration60();
  else*/ if (frameworkType == FrameworkType::LOW)
    return 10; //secs
  else
    exit(EXIT_FAILURE);
}

///////

uint64_t enqueueTask() {
  _lock.lock();
  queuedTasksNumber += 1;
  _lock.unlock();
  return 1;
}

void dequeueTask() {
  _lock.lock();
  queuedTasksNumber -= 1;
  _lock.unlock();
}

void reinsertTask() {
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
            << "Total tasks not launched = "
            << totalTasksNotLaunched   << endl
            << "Total offers received = "  << totalOffersReceived  << endl
            << "Total offers declined = "  << totalOffersDeclined  << endl
            << "Total offers accepted = "  << totalOffersAccepted  << endl
            << "Total offers unused = "    << totalOffersUnused;
}

void resetStats() {
  receivedOffers  = 0;
  tasksLaunched   = 0;
  tasksNotLaunched   = 0;
  offersDeclined  = 0;
  offersAccepted  = 0;
  offersUnused    = 0;
}

void printStats()
{
  LOG(INFO) << endl
            << "Allocation run#"             << allocationRunNumber      << endl
            << "Tasks launched = "           << tasksLaunched            << endl
            << "Tasks not launched = "       << tasksNotLaunched         << endl
            << "Offers received= "           << receivedOffers           << endl
            << "Offers declined = "          << offersDeclined           << endl
            << "Offers accepted = "          << offersAccepted           << endl
            << "Offers unused = "            << offersUnused ;
}

void printOnFile() {
  if (statsFilepath.isNone())
    return;

  std::ofstream myfile;
  myfile.open (statsFilepath.get(),  std::ofstream::app);
  if (!myfile.is_open())
    LOG(ERROR) << "Error opening the file to ouptut stats.";
  else {
    myfile << receivedOffers        << " "
           << offersDeclined        << " "
           << offersAccepted        << " "
           << offersUnused          << " "
           << totalOffersDeclined   << " "
           << totalOffersAccepted   << " "
           << totalOffersUnused     << " "
           << totalOffersReceived   << " "
           << tasksLaunched         << " "
           << tasksNotLaunched      << " "
           << totalTasksLaunched    << " "
           << totalTasksNotLaunched << endl;
    myfile.close();
  }
}

bool checkStoppableFramework() {
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed = end-start;

  LOG(INFO) << "elapses secs=" << elapsed.count();
  LOG(INFO) << "framework duration secs=" << frameworkDuration.count();

  if (elapsed.count() >= frameworkDuration.count())
     return true;
  return false;
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
    if(checkStoppableFramework()) {
      driver->stop();
      LOG(INFO) << "STOPPING FRAMEWORK";
      exit(EXIT_SUCCESS);
    }

    uint64_t lastReadQueuedTaskNumber;
    allocationRunNumber++;
    foreach (const Offer& offer, offers) {
      receivedOffers++;
      totalOffersReceived++;

      LOG(INFO) << "Received offer "
                << offer.id() << " with "
                << offer.resources();

      Resources remaining = offer.resources();

      // Launch tasks.
      vector<TaskInfo> tasksToLaunch;
      tasksToLaunch.reserve(10);
      bool insufficientResources = false;

      while ((lastReadQueuedTaskNumber = getQueuedTasks()) > 0
          && allocatable(remaining)) {
        dequeueTask();
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
          reinsertTask();
          insufficientResources = true;
          break; // this offer is not useful anymore
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
    tasksNotLaunched = lastReadQueuedTaskNumber;
    totalTasksNotLaunched += tasksNotLaunched;
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
    LOG(INFO) << "New task queued="   << enqueueTask();
    LOG(INFO) << "Total tasks queued=" << getQueuedTasks();
    Duration arrivalTimeDelay = getNextTaskInterarrivalTime();
    LOG(INFO) << "The next arrival will be in " << arrivalTimeDelay;
    os::sleep(arrivalTimeDelay);
  }
}

void setupDistributions(const string& configuration)
{
  vector<string> tokens = strings::split(configuration, ",");

  if (tokens.size() == 3) {
    if(tokens[0].compare("N") == 0) {
      LOG(INFO) << "Selected a Gamma distribution.";
      // TODO(danang) check the fields
      return;
    }
  } else if(tokens.size() == 2) {
    if(tokens[0].compare("E") == 0) {
      double lambda = lexical_cast<double>(tokens[1]);
      LOG(INFO) << "Selected an Exponential distribution with lambda="
                << lambda << " => E[X]=" << 1/lambda;
      tasksInterarrivalTimeExpDistribution_A =
          std::exponential_distribution<double>(lambda);

      generateTaskInterarrivalTime =
          std::bind(
              tasksInterarrivalTimeExpDistribution_A,
              tasksInterarrivalTimeGenerator);
      return;
    }
  }

  LOG(ERROR) << "Wrong format for the distribution configuration.";
  exit(EXIT_FAILURE);
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

  double _frameworkDuration;
  flags.add(&_frameworkDuration,
         "duration",
         None(),
         "How much time to execute the framework."
         " Please specify the number of seconds.",
         static_cast<const double*>(nullptr),
         [](const double& value) -> Option<Error> {
           if (value <= 0) {
             return Error(
                 "Please use a --duration greater than " +
                 stringify(1.0));
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

  Option<string> generators_seed;
  flags.add(&generators_seed,
            "generators_seed",
            "seed to use for the internal pseudorandom generators.");

  Option<string> interarrivals_distribution;
  flags.add(&interarrivals_distribution,
            "interarrivals_distribution",
            "distribution to use for the interarrival of tasks.\n"
            "   Examples:\n"
            "   E,lambda\n"
            "   N,u,stddev");

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
  } else if (generators_seed.isNone()) {
    cerr << "Missing --generators_seed" << endl;
    usage(argv[0], flags);
    exit(EXIT_FAILURE);
  } else if (interarrivals_distribution.isNone()) {
    cerr << "Missing --interarrivals_distribution" << endl;
    usage(argv[0], flags);
    exit(EXIT_FAILURE);
  }

  internal::logging::initialize(argv[0], flags, true); // Catch signals.

  // Log any flag warnings (after logging is initialized).
  foreach (const flags::Warning& warning, load->warnings) {
    LOG(WARNING) << warning.message;
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

  frameworkDuration =  std::chrono::duration<double>(_frameworkDuration);
  LOG(INFO) << "Task duration for this framework will be: "
              << frameworkDuration.count() << " seconds.";

  std::seed_seq seed(
      generators_seed.get().begin(),
      generators_seed.get().end());
  tasksInterarrivalTimeGenerator = std::mt19937(seed);

  setupDistributions(interarrivals_distribution.get());


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

  start = std::chrono::steady_clock::now();

  int status = driver->run() == DRIVER_STOPPED ? 0 : 1;

  // Ensure that the driver process terminates.
  driver->stop();

  delete driver;
  return status;
}
