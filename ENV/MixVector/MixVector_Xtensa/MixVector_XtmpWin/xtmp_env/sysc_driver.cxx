/*
 * Copyright (c) 2003-2006 Tensilica Inc.  ALL RIGHTS RESERVED.
 * 
 * These coded instructions, statements, and computer programs are
 * the copyrighted works and confidential proprietary information of
 * Tensilica Inc.  They may be adapted and modified by bona fide
 * purchasers for internal use, but no adapted or modified version
 * may be disclosed or distributed to third parties in any manner,
 * medium, or form, in whole or in part, without the prior  written
 * consent of Tensilica Inc.
 */

// This file provides driver routines for the XTMP System Simulation API.
// You have to modify this file in the following situations:
//
// - To use a different version of SystemC.  Usually this involves recompiling
// this file with your own systemc.h file.
//
// - To remove the use of SystemC and to use an alternative method pre-emptive
// thread package.  This will involve re-implementing the functions below.
//

// Customers may replace the following #ifdef...#endif
// with just a #include "iss/mp.h"
//
#if defined(_BUILD_INTERNAL_)
#include "mp.h"
#else
#include "iss/mp.h"
#endif

#include "systemc.h"

extern "C" bool scheduled_actions_empty(void);

// Used to test for ridiculous delay values.  In other drivers, this
// values is used to size data structures, but not here.
#define MAX_DELTA_TIME 1023

// The system clock.
static sc_time SystemClockPeriod( 1, SC_NS );

//
// Offsets within a clock cycle at which the different phases occur.
// These include a clock phase, a user thread phase, a point by which
// responses must be received (for requests made in the preceding cycle)
// and an end-of-cycle phase (when TIE port callbacks are invoked.)
//
// Each value is a percentage into the cycle that indicates when the
// phase occurs.
//
const u32 phase_r_offset = 20;	// Check for responses
const u32 phase_c_offset = 30;	// Cores execute in this phase
const u32 phase_u_offset = 50;	// User thread phase
const u32 phase_e_offset = 90;	// End of cycle phase: TIE port callbacks

static sc_event* userThreadEvent;
static u32 waitCount;
static bool corePhase = false;

// The number of live threads in the system.  Thread creation 
// increments this counter; driverStop decrements the number. 
static unsigned numberOfThreads = 0;

// The system is running: true when the threads are all running.
static bool running = false;

/*
// Utility function to intern a string
*/
char *
internString(const char *s)
{
  char *interned;
  if( s ) {
    interned = new char[strlen(s)+1];
    strcpy(interned,s);
    return interned;
  }
  return NULL;
}


//
// There is one CPU thread for each XTMP core.
//
class CpuThread: public sc_module
{
public:
  XTMP_time (*stepCPU)(XTMP_threadInfo *);
  void (*syncCPU)(XTMP_threadInfo *, XTMP_timeDelta);
  XTMP_threadInfo *threadInfo;

  SC_CTOR(CpuThread)
  {
    SC_THREAD(cycle);
    set_stack_size(0x20000);
  }

  void cycle(void)
  {
    sc_time clock_period = SystemClockPeriod;
    sc_time phase_c = clock_period * phase_c_offset /100;

    ::wait(phase_c);

    while (!threadInfo->exitFlag) {
      // Cycle the core.
      XTMP_time core_time = stepCPU(threadInfo);
      if (threadInfo->exitFlag || core_time <= XTMP_clockTime())
	continue;

      sc_time this_phase_c_time = sc_time_stamp();
      sc_time next_phase_c_time = this_phase_c_time + clock_period;

      syncCPU(threadInfo, core_time - XTMP_clockTime());

      sc_time now = sc_time_stamp();
      if (now < next_phase_c_time) {
	// Cycle-accurate mode.
	if (now > this_phase_c_time)
	  wait (next_phase_c_time - now);
      }
      else if (now > next_phase_c_time) {
	// Turbo mode.
	sc_time delta = now - (double)(u64)(now/clock_period) * clock_period;
	if (delta < phase_c) {
	  wait(phase_c - delta);
	}
	else {
	  wait(clock_period - delta + phase_c);
	}
      }
    }
  }
};


//
// The following class represents a user thread.
//
struct Thread: public sc_module
{
public:
  void (*f)(XTMP_threadInfo *);
  XTMP_threadInfo *threadInfo;

  SC_CTOR(Thread)
  {
      SC_THREAD(stub);
      set_stack_size(0x20000);
  }

  void stub(void) {
    ::wait( *userThreadEvent );
    f(threadInfo);
  }
};

    
//
// Create a thread.  Threads drive the execution of cpu's and tickers.
//
static void *threadNew( const char *name, void (*f)(XTMP_threadInfo *),
			XTMP_threadInfo *ti )
{
  Thread *t = new Thread( internString(name) );
  numberOfThreads++;
  t->f = f;
  t->threadInfo = ti;
  return t;
}

//
// Create a CPU thread.
//
static void *cpuThreadNew( const char *name, 
			   XTMP_time (*stepCPU)(XTMP_threadInfo *),
			   void (*syncCPU)(XTMP_threadInfo *,XTMP_timeDelta),
			   XTMP_threadInfo *ti )
{
  CpuThread *t = new CpuThread( internString(name) );
  numberOfThreads++;
  t->stepCPU = stepCPU;
  t->syncCPU = syncCPU;
  t->threadInfo = ti;
  return t;
}

/* The XTMP_event structure */
typedef struct event {
  sc_event* sc_evt;
  bool fired;
} *event_t;


class SchedulerThread: public sc_module
{
public:

  SC_CTOR(SchedulerThread)
  {
    SC_THREAD(cycle);
  }

  void cycle(void)
  {
    sc_time clock_period = SystemClockPeriod;

    sc_time phase_r = clock_period * phase_r_offset /100;
    sc_time phase_u = clock_period * phase_u_offset /100;
    sc_time phase_e = clock_period * phase_e_offset /100;

    sc_time phase_r_to_u = phase_u - phase_r;
    sc_time phase_u_to_e = phase_e - phase_u;
    sc_time phase_e_to_r = SystemClockPeriod - phase_e + phase_r;

    event_t evt = reinterpret_cast<event_t>(XTMP_getSchedulerEvent());
    sc_event* scheduler_event = evt->sc_evt;

    wait(phase_r);

    while (true) {
      XTMP_startOfCycleProcessing();

      corePhase = true;
      wait(phase_r_to_u);
      corePhase = false;

      do {
	waitCount = 0; 
	userThreadEvent->notify();
	::wait(SC_ZERO_TIME);
      } while (waitCount);

      wait(phase_u_to_e);
      XTMP_endOfCycleProcessing();

      if (!scheduled_actions_empty()) {
	// Cycle-accurate mode.
	wait(phase_e_to_r);
      }
      else {
	// Turbo mode.
	sc_time next_phase_r_time = sc_time_stamp() + phase_e_to_r;

	wait(*scheduler_event);
	sc_time now = sc_time_stamp();

	if (now < next_phase_r_time)  {
	  wait(next_phase_r_time - now);
	}
	else if (now > next_phase_r_time) {
	  sc_time delta = now - (double)(u64)(now/clock_period) * clock_period;
	  if (delta < phase_r)
	    wait(phase_r - delta);
	  else if (delta < phase_e) {
	    wait(phase_e - delta);
	    XTMP_endOfCycleProcessing();
	    wait(phase_e_to_r);
	  }
	  else {
	    wait(clock_period - delta + phase_r);
	  }
	}
      }
    }
  }
};


static SchedulerThread schedulerThread("_scheduler_thread_");
    

//
// Run the simulation for ncycles; run the simulation until all
// threads exit if ncycles < 0.
//
static void
driverStart(int ncycles)
{
  if( numberOfThreads ) {
    running = true;
    sc_start(ncycles * SystemClockPeriod);
    running = false;
  }
}

//
// Stop the simulation.
//
static void
driverStop(void)
{
  if( numberOfThreads == 0 || !running ) {
    fprintf( stderr, "sysc_driver.cxx: can't call XTMP_stop "
	     " from non thread context (e.g. XTMP_main )\n");
    exit(1);
  }
  
  numberOfThreads--;
  if( numberOfThreads == 0 )
    sc_stop();
  waitCount++;
  ::wait();
}

//
// suspend the current thread for ncycles.
//
static void
driverWait(unsigned ncycles)
{
#if 0
  /*
  // If you know that you won't ever exceed this limit, you can
  // safely "#if 0" this code out.
  */
  if( ncycles > MAX_DELTA_TIME ) {
    fprintf( stderr, "sysc_driver.c: time delay (%d) larger than %d\n"
	     "If you want to do this you have to increase MAX_DELTA_TIME in "
	     "sysc_driver.c\n", ncycles, MAX_DELTA_TIME );
    exit(1);
  }
#endif

  waitCount++;
  switch( ncycles )
    {
    case 0:
      ::wait(SC_ZERO_TIME);
      break;
    default:
      sc_time delay = ncycles * SystemClockPeriod;
      if (!corePhase) {
	sc_time clock_period = SystemClockPeriod;
	sc_time phase_u = clock_period * phase_u_offset /100;	
	sc_time now = sc_time_stamp();
	sc_time delta = now - (double)(u64)(now/clock_period) * clock_period;
	delay += phase_u - delta;
      }
      ::wait( delay );
      break;
    }
}

//
// Provide time as a long long for XTMP
//
static XTMP_time clockTime(void)
{
  return (XTMP_time) (sc_time_stamp()/SystemClockPeriod);
}

static u32
numThreads(void)
{
  return numberOfThreads;
}


static void
driverInit()
{
  assert(phase_r_offset < 100 &&
	 phase_c_offset < 100 &&
	 phase_u_offset < 100 &&
	 phase_e_offset < 100);
  assert(phase_r_offset < phase_c_offset &&
	 phase_c_offset < phase_u_offset &&
	 phase_u_offset < phase_e_offset);

  userThreadEvent = new sc_event();
}


//
// Support for XTMP events.
//

XTMP_event
eventNew()
{
  event_t evt;

  evt = new struct event;
  evt->sc_evt = new sc_event();
  evt->fired = false;
  return (XTMP_event)evt;
}

void
eventFree(XTMP_event event)
{
  event_t evt = (event_t)event;
  delete evt->sc_evt;
  delete evt;
}

void
waitOnEvent(XTMP_event event)
{
  event_t evt = (event_t)event;
  waitCount++;
  ::wait(*evt->sc_evt);
}

void
fireEvent(XTMP_event event)
{
  event_t evt = (event_t)event;
  evt->fired = true;
  evt->sc_evt->notify();
}

bool
hasEventFired(XTMP_event event)
{
  event_t evt = (event_t)event;
  return evt->fired;
}


// On NT, ISS takes the form of a DLL.  To avoid circular dependency with
// the Visual C++ linker, the XTMP (inside the ISS DLL) cannot reference
// the funtions in this file directly.  Instead, XTMP must use a function
// table.
//
static XTMP_driverTable driverTable = {
  XTMP_VERSION,
  NULL,
  NULL,
  driverInit,
  threadNew,
  cpuThreadNew,
  driverStart,
  driverStop,
  driverWait,
  eventNew,
  eventFree,
  waitOnEvent,
  fireEvent,
  hasEventFired,
  clockTime,
  numThreads,
};


static void
Message_Function(const char* s)
{
  cout<<s<<"\n";
}


//
// If you don't want to use XTMP_main, you can simply put the code
// in XTMP_main directly into this main program.
//
int
main( int argc, char **argv )
{
  int status = 0;
  try {
    XTMP_initialize(&driverTable);
    status = XTMP_main(argc, argv);
    XTMP_cleanup();
  }
  catch(const sc_exception& x)
    {
      Message_Function(x.what());
    }
  catch(const char* s)
    {
      Message_Function(s);
    }
  catch(...)
    {
      Message_Function("UNKNOWN EXCEPTION OCCURED");
    }

  return status;

}

