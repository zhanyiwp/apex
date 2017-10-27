#include "svr_timer.h"
#include <iostream>
#include <stdlib.h>

using namespace std;

static svr_timer timer;
static svr_timer timer2;
static svr_timer timer3;
static uint32_t count = 0;

static void on_timer(svr_timer* t)
{
	++count;
	cout << "on timer, count=" << count << endl;
	if(count==5)
		t->stop();
}
static void on_timer2(svr_timer* t)
{
	cout << "on timer2" << endl;
	timer.reset();
	count = 0;
}
static void on_timer3(svr_timer* t)
{
	cout << "on timer3" << endl;
	timer2.reset();
	count = 0;
}

bool svr_test_timer(struct ev_loop* g_loop)
{
	timer.set(g_loop, 1.0, on_timer, NULL, true);
	timer.start();
	timer2.set(g_loop, 10.0, on_timer2, NULL, false);
	timer2.start();
	timer3.set(g_loop, 22.0, on_timer3, NULL, false);
	timer3.start();

	return true;
}

int main(int argc, char const *argv[])
{
	srand(time(NULL));

	struct ev_loop* g_loop = ev_default_loop(EVFLAG_AUTO | EVBACKEND_EPOLL);
	if(NULL==g_loop)
	{
		cout << "ev_default_loop" << endl;
	}

	svr_test_timer(g_loop);

	while(true)
	{
		ev_run(g_loop, 0);
	}
	return 0;
}