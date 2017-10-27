#ifndef PERIOD_TIMER_H_
#define PERIOD_TIMER_H_

#include "ev.h"

class timer
{
public:
	timer(struct ev_loop* loop);
	virtual ~timer();

private:
	/**
	* timer handler who hold the timeout handle functions.
	* all classes who wanna handle timeout via their non-static member functions should inherit from this class.
	* this class has no method, so you don't need to worry about careless override, you can even "private" inherit from this class.
	*/
	class blank_class
	{
	};

	void innert_set(blank_class* obj, void(blank_class::*memfunc)(timer*), uint32_t ms, bool repeat, void* arg);
	void innert_set(void(*staticfunc)(timer*), uint32_t ms, bool repeat, void* arg);

	struct ev_timer _timer;
	blank_class* _obj;
	union
	{
		void(*static_func)(timer*);
		void(blank_class::*memfunc)(timer*);
	}_cb;
	void* _arg;
	bool _repeat;
	bool _active;
	struct ev_loop* _loop;

public:
	static timer* get_instance_via_data(ev_timer* ev);
    void on_timer();

	/**
	* initialize the timer.
	* @param loop		this timer is working on which loop
	* @param tv			the timer repeat time second
	* @param handler	the callback method will be called on which class object
	* @param function	the callback method
	* @param arg		an argument carried by the timer
	* @param repeat	whether this timer will repeat infinitely
	*/
	template<typename class_type, typename memfunc_type>
	void set(class_type* obj, memfunc_type func, uint32_t ms, bool repeat = true, void* arg = NULL)
	{
		innert_set(reinterpret_cast<blank_class*>(obj), reinterpret_cast<void(blank_class::*)(timer*)>(func), ms, repeat, arg);
	}
	template<typename memfunc_type>
	void set(memfunc_type func, uint32_t ms, bool repeat = true, void* arg = NULL)
	{
		innert_set(reinterpret_cast<void(*)(timer*)>(func), ms, repeat, arg);
	}	
	inline void* get_arg(){ return _arg; }

	void start();
	void stop();
	void reset();
	bool is_active();
};

#endif // PERIOD_TIMER_H_
