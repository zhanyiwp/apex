#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "ev.h"
#include "base_type.h"
#include "period_timer.h"

static void timer_callback(struct ev_loop *, ev_timer *watcher, int)
{
	timer* t = timer::get_instance_via_data(watcher);
	t->on_timer();
}

timer* timer::get_instance_via_data(ev_timer* ev)
{ 
	return reinterpret_cast<timer*>(reinterpret_cast<uint8_t*>(ev) - member_offsetof(timer, _timer));
}

timer::timer(struct ev_loop* loop)
{
	assert(loop);
	_loop = loop;
	ev_init(&_timer, timer_callback);
	_active = false;

	_obj = NULL;
	_cb.static_func = NULL;
	_arg = NULL;
	_repeat = false;
}

timer::~timer()
{
	_active = false;
	stop();
}

bool timer::is_active()
{
	return _active;
}

void timer::innert_set(blank_class* obj, void(timer::blank_class::*memfunc)(timer*), uint32_t ms, bool repeat, void* arg)
{
	//convert to double by second prec
	double d = ((double)ms) / 1000;
	ev_timer_set(&_timer, d, d);
	_obj = obj;
	_cb.memfunc = memfunc;
	_arg = arg;
	_repeat = repeat;
}

void timer::innert_set(void(*staticfunc)(timer*), uint32_t ms, bool repeat, void* arg)
{
	double d = ((double)ms) / 1000;
	ev_timer_set(&_timer, d, d);
	_obj = NULL;
	_cb.static_func = staticfunc;
	_arg = arg;
	_repeat = repeat;
}

void timer::on_timer()
{	
	if (!_repeat)
	{
		stop();
	}
	if (_obj)
	{
		(_obj->*(_cb.memfunc))(this);
	}
	else
	{
		(*(_cb.static_func))(this);
	}
}

void timer::start()
{
	_active = true;
	ev_timer_again(_loop, &_timer);
}

void timer::stop()
{
	_active = false;
	ev_timer_stop(_loop, &_timer);
}

void timer::reset()
{
	start();
}

