#pragma once
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <list>
#include <type_traits>
namespace multi {
	// Capture this when code is loaded up!
	const std::thread::id mainThreadID = std::this_thread::get_id();
	const unsigned int processor_count = std::thread::hardware_concurrency();;
	bool isMainThread() {
		return mainThreadID == std::this_thread::get_id();
	}
	class runner;
	struct mobj;
	enum class priority { core, very_high, high, above_normal, normal, below_normal, low, very_low, idle };
	enum class scheduler { custom, round_robin };
	enum class status { error, stopped, running, paused };
	typedef std::chrono::high_resolution_clock Clock;
	typedef std::chrono::nanoseconds nanoseconds;
	typedef std::chrono::microseconds microseconds;
	typedef std::chrono::milliseconds milliseconds;
	typedef std::chrono::seconds seconds;
	typedef std::chrono::minutes minutes;
	typedef std::chrono::hours hours;

	const char _nanosecond	= 0x0;
	const char _microsecond = 0x1;
	const char _millisecond = 0x2;
	const char _second		= 0x3;
	const char _minute		= 0x4;
	const char _hour		= 0x5;

	typedef std::chrono::time_point<std::chrono::high_resolution_clock> time;
	typedef bool (*event_start)(mobj);
	
	typedef bool (*alarm_event)(mobj*);
	typedef bool (*step_event)(mobj*, size_t);
	typedef bool (*loop_event)(mobj*);
	typedef bool (*updater_event)(mobj*);
	typedef bool (*basic_event)(mobj*);

	typedef bool (*event_end)(mobj);

	typedef void (*reset)(mobj*);
	template<typename T>
	struct node {
		node* prev = nullptr;
		node* next = nullptr;
		T* data = nullptr;
		node<T>* addNode(T* dat) {
			node* temp = new node();
			temp->head = false;
			temp->data = dat;
			if (next) {
				temp->next = next;
				next->prev = temp;
				temp->prev = this;
				next = temp;
			}
			else {
				temp->prev = this;
				next = temp; // Empty linked list
			}
			return this;
		}
		inline void deleteNode() {
			if (head) {
				// We dont have a prev
				next->head = true; // Set next as the head
				delete next;
				data = nullptr; // We do not want to touch the data we stored
			}
			else {
				// Manage the pointers
				// You always have a prev! The head is a node, just with no data
				if (next) {
					next->prev = prev;
					prev->next = next;
				}
				else
					prev->next = nullptr;
				next = nullptr;
				prev = nullptr;
				if (data) {
					data->tid = nullptr;
					data = nullptr;
				}
			}
		}
		inline bool isHead() {
			return head;
		}
		inline void setReady(bool r) {
			th_ready = r;
		}
	private:
		bool head=true;
		bool th_ready = false;
	};
	//Queue stuff
	template <typename T>
	class Queue
	{
	public:
		T pop()
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			while (queue_.empty())
			{
				cond_.wait(mlock);
			}
			auto val = queue_.front();
			queue_.pop();
			return val;
		}

		T peek() {
			std::unique_lock<std::mutex> mlock(mutex_);
			while (queue_.empty())
			{
				cond_.wait(mlock);
			}
			auto val = queue_.front();
			return val;
		}

		bool empty() {
			return queue_.empty();
		}

		void pop(T& item)
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			while (queue_.empty())
			{
				cond_.wait(mlock);
			}
			item = queue_.front();
			queue_.pop();
		}

		void push(const T& item)
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			queue_.push(item);
			mlock.unlock();
			cond_.notify_one();
		}
		Queue() = default;
		Queue(const Queue&) = delete;            // disable copying
		Queue& operator=(const Queue&) = delete; // disable assignment

	private:
		std::queue<T> queue_;
		std::mutex mutex_;
		std::condition_variable cond_;
	};
	// End queue
	typedef bool (*object_runner)(mobj*,runner*);
	typedef bool (*runner_func)(runner*);
	struct mobj {
		friend class multi::runner;
		std::chrono::time_point<std::chrono::high_resolution_clock> creation = Clock::now();
		bool active = true;
		node<mobj>* tid = nullptr;
		const char* type;
		void* obj = nullptr;
		void* data = nullptr;
		mobj(const char* t) {
			run = nullptr;
			type = t;
		}
		object_runner run = nullptr;
		// Universal methods
		long long getTimeSinceCreation() const {
			return std::chrono::duration_cast<milliseconds>(Clock::now() - creation).count();
		}
	private:
		runner* ref = nullptr;
		bool ready = false;
		void* hiddendata = nullptr; // This is the event function pointer for all mobjs
		void* reserved1 = nullptr; // For all objects dealing with time, this is a timer object pointer
		void* reserved2 = nullptr; // For all objects dealing with time, this is a long long pointer
		void* reserved3 = nullptr; // For all objects dealing with time, this is a char enum type for the type of time being kept
	};
	// Keeps track of time.
	template <typename T>
	struct timer {
		inline void start() {
			t = Clock::now();
		}
		inline long long get() const {
			return std::chrono::duration_cast<T>(Clock::now() - t).count();
		}
		inline void sleep(long long s) {
			std::this_thread::sleep_for(T(s));
		}
	private:
		time t;
	};
	class runner {
		bool stop = false;
		scheduler scheme;
		node<mobj> tasks;
		runner_func current;
		bool initSubsystem() {
			if (subsystem != nullptr)
				return false; // Subsystem already initiated
			subsystem = new std::thread([](multi::runner* r) {
				node<mobj> timedObjects;
				// The thread main loop
				while (r->isActive()) {
					// Take items from the queue and put them into timedObjects linked list
					while (!r->timeQueue.empty()) { // What if we have 1 million alarms? This could hold things up couldn't it? But why would one do this?
						timedObjects.addNode(r->timeQueue.pop());
					}
					//Handle timed objects
					node<mobj>* current = timedObjects.next;
					while (current != nullptr) {
						mobj* temp = current->data;
						char t = *(r->getReserved3<char>(temp));
						long long set = *(r->getReserved2<long long>(temp));
						switch (t)
						{
						case _nanosecond:
						{
							auto tester = (r->getReserved1<timer<nanoseconds>>(temp));
							if (tester->get() >= set) {
								r->setReady(temp, true);
								current->deleteNode(); // Remove this node then keep going
							}
							break;
						}
						case _microsecond:
						{
							auto tester = (r->getReserved1<timer<microseconds>>(temp));
							if (tester->get() >= set) {
								r->setReady(temp, true);
								current->deleteNode(); // Remove this node then keep going
							}
							break;
						}
						case _millisecond:
						{
							auto tester = (r->getReserved1<timer<milliseconds>>(temp));
							if (tester->get() >= set) {
								r->setReady(temp, true);
								current->deleteNode(); // Remove this node then keep going
							}
							break;
						}
						case _second:
						{
							auto tester = (r->getReserved1<timer<seconds>>(temp));
							if (tester->get() >= set) {
								r->setReady(temp, true);
								current->deleteNode(); // Remove this node then keep going
							}
							break;
						}
						case _minute:
						{
							auto tester = (r->getReserved1<timer<minutes>>(temp));
							if (tester->get() >= set) {
								r->setReady(temp, true);
								current->deleteNode(); // Remove this node then keep going
							}
							break;
						}
						case _hour:
						{
							auto tester = (r->getReserved1<timer<hours>>(temp));
							if (tester->get() >= set) {
								r->setReady(temp, true);
								current->deleteNode(); // Remove this node then keep going
							}
							break;
						}
						default:
							// Invalid data passed, we should do something
							break;
						}
						current = current->next;
					}
					// Continue the loop here


				}
				}, this);
			return true; // Initiated the subsystem
		}
		std::thread* subsystem;
		bool isLocal = true;
		std::thread* threadedloop = nullptr;
		static bool round_robin(runner* run) {
			auto& n = run->getHead();
			node<mobj>* current = n.next;
			while (current != nullptr) {
				current->data->run(current->data, run);
				current = current->next;
			}
			return true;
		}
	public:
		Queue<mobj*> timeQueue;
		runner(bool newThread=false) {
			if (newThread) {
				isLocal = false; // Tell the runner that if it's loop() is called it should spawn a thread to run it's code
			}
			setScheme(scheduler::round_robin);
			initSubsystem(); // This pumps some background stuff
		}
		~runner() {
			// ToDo clean up stuff
		}
		inline bool isReady(mobj* m) {
			return m->ready;
		}
		inline void setReady(mobj* m, bool r) {
			m->ready = r;
		}
		template<typename T>
		inline T* getReserved0(mobj* m) {
			if (m->ref != this)
				return nullptr; // You can only touch objects that are within your own runner
			return (T*)m->hiddendata;
		}
		template<typename T>
		inline T* getReserved1(mobj* m) {
			if (m->ref != this)
				return nullptr; // You can only touch objects that are within your own runner
			return (T*)m->reserved1;
		}
		template<typename T>
		inline T* getReserved2(mobj* m) {
			if (m->ref != this)
				return nullptr; // You can only touch objects that are within your own runner
			return (T*)m->reserved2;
		}
		template<typename T>
		inline T* getReserved3(mobj* m) {
			if (m->ref != this)
				return nullptr; // You can only touch objects that are within your own runner
			return (T*)m->reserved3;
		}
		node<mobj>& getHead() {
			return tasks;
		}
		size_t getTaskCount() {
			size_t c=0;
			node<mobj>* current = tasks.next;
			while (current != nullptr) { c++; current = current->next; }
			return c;
		}
		inline void loop() {
			if (isLocal) {
				while (!stop) {
					current(this);
				}
				return;
			}
			runner_func* fptr = &current; // This is really a pointer to a pointer, wrapped up to look less messy
			if (threadedloop == nullptr) {
				threadedloop = new std::thread([](multi::runner* r, runner_func* current) {
					while (r->isActive()) {
						(*current)(r);
					}
				},this,fptr);
			}
		}
		inline bool update() {
			current(this);
			return !stop;
		}
		inline bool isActive() {
			return !stop;
		}
		void setScheme(scheduler s) {
			if (s == scheduler::round_robin) {
				current = round_robin;
				scheme = s;
			}
		}
		void setRunner(runner_func run) {
			scheme = scheduler::custom;
			current = run;
		}
		template<typename T>
		mobj* setAlarm(long long set, alarm_event evnt)
		{
			auto obj = new mobj("alarm");
			obj->ref = this;
			obj->hiddendata = evnt;
			auto temp = new timer<T>;
			temp->start();
			obj->reserved1 = temp;
			obj->reserved2 = new long long(set);
			// We need to store the type of this data
			if (std::is_same<T, nanoseconds>::value) {
				obj->reserved3 = new char(_nanosecond);
			}
			if (std::is_same<T, microseconds>::value) {
				obj->reserved3 = new char(_microsecond);
			}
			if (std::is_same<T, milliseconds>::value) {
				obj->reserved3 = new char(_millisecond);
			}
			if (std::is_same<T, seconds>::value) {
				obj->reserved3 = new char(_second);
			}
			if (std::is_same<T, minutes>::value) {
				obj->reserved3 = new char(_minute);
			}
			if (std::is_same<T, hours>::value) {
				obj->reserved3 = new char(_hour);
			}
			obj->run = [](mobj* self, runner* run) {
				if (self->ready) {
					self->ready = false;
					return ((alarm_event)self->hiddendata)(self);
				}
				return false;
			};
			timeQueue.push(obj);
			tasks.addNode(obj);
			return obj;
		}
		template<typename T>
		mobj* newTLoop(long long set, loop_event evnt)
		{
			auto obj = new mobj("tloop");
			obj->ref = this;
			obj->hiddendata = evnt;
			auto temp = new timer<T>;
			temp->start();
			obj->reserved1 = temp;
			obj->reserved2 = new long long(set);
			// We need to store the type of this data
			if (std::is_same<T, nanoseconds>::value) {
				obj->reserved3 = new char(_nanosecond);
			}
			if (std::is_same<T, microseconds>::value) {
				obj->reserved3 = new char(_microsecond);
			}
			if (std::is_same<T, milliseconds>::value) {
				obj->reserved3 = new char(_millisecond);
			}
			if (std::is_same<T, seconds>::value) {
				obj->reserved3 = new char(_second);
			}
			if (std::is_same<T, minutes>::value) {
				obj->reserved3 = new char(_minute);
			}
			if (std::is_same<T, hours>::value) {
				obj->reserved3 = new char(_hour);
			}
			obj->run = [](mobj* self,runner* run) {
				if (self->ready) {
					self->ready = false;
					// Turns out we can cast this as something else thats similar if we only call the start function
					auto tmp = run->getReserved1<timer<microseconds>>(self);
					tmp->start();
					run->timeQueue.push(self);
					return ((loop_event)self->hiddendata)(self);
				}
				return false;
			};
			timeQueue.push(obj);
			tasks.addNode(obj);
			return obj;
		}
		mobj* newEvent(basic_event evnt);
		mobj* newStep(int start, int end, int count, step_event evnt);
		mobj* newTStep(double set, int start, int end, int count, step_event evnt);
		mobj* newLoop(loop_event evnt) {
			auto obj = new mobj("loop");
			obj->ref = this;
			obj->hiddendata = evnt;
			obj->run = [](mobj* self, runner* run) {
				return ((loop_event)self->hiddendata)(self);
			};
			obj->data = tasks.addNode(obj); // Set pointer to the node
			return obj;
		}
		mobj* newUpdater(int skip, updater_event evnt);
	};
}