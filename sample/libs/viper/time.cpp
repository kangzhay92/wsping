#ifndef VP_UNITY_BUILD
#include "app.h"
#endif
#include "time.h"

struct vtm_state
{
#ifdef _WIN32
	LARGE_INTEGER freq;
	LARGE_INTEGER start;
#endif
};

struct vp_time_impl : public vp_time
{
	vp_time_impl();
	~vp_time_impl() {}

	uint64_t now() final;
	uint64_t diff(uint64_t new_ticks, uint64_t old_ticks) final;
	uint64_t since(uint64_t start_ticks) final;
	uint64_t delta_time(uint64_t* last_time) final;

	double seconds(uint64_t ticks) final;
	double milliseconds(uint64_t ticks) final;
	double microseconds(uint64_t ticks) final;
	double nanoseconds(uint64_t ticks) final;

	static vp_time_impl instance;
	vtm_state state;
};

vp_time_impl::vp_time_impl()
{
	state = {};
#ifdef _WIN32
	QueryPerformanceFrequency(&state.freq);
	QueryPerformanceCounter(&state.start);
#endif
}

static int64_t int64_muldiv(int64_t value, int64_t numerator, int64_t denominator)
{
	int64_t q = value / denominator;
	int64_t r = value % denominator;
	return q * numerator + r * numerator / denominator;
}

uint64_t vp_time_impl::now()
{
	int64_t ret;

#ifdef _WIN32
	LARGE_INTEGER qpc;
	QueryPerformanceCounter(&qpc);
	ret = int64_muldiv(qpc.QuadPart - state.start.QuadPart, 1000000000, state.freq.QuadPart);
#endif

	return static_cast<uint64_t>(ret);
}

uint64_t vp_time_impl::diff(uint64_t new_ticks, uint64_t old_ticks)
{
	if (new_ticks > old_ticks) {
		return new_ticks - old_ticks;
	}
	return 1;
}

uint64_t vp_time_impl::since(uint64_t start_ticks)
{
	return diff(now(), start_ticks);
}

uint64_t vp_time_impl::delta_time(uint64_t* last_time)
{
	uint64_t dt = 0;
	uint64_t cur = now();
	if (*last_time != 0) {
		dt = diff(cur, *last_time);
	}
	*last_time = cur;
	return dt;
}

double vp_time_impl::seconds(uint64_t ticks)      { return static_cast<double>(ticks) / 1000000000.0; }
double vp_time_impl::milliseconds(uint64_t ticks) { return static_cast<double>(ticks) / 1000000.0; }
double vp_time_impl::microseconds(uint64_t ticks) { return static_cast<double>(ticks) / 1000.0; }
double vp_time_impl::nanoseconds(uint64_t ticks)  { return static_cast<double>(ticks); }

vp_time_impl vp_time_impl::instance;

#pragma region C and C++ Implementations
VP_API vp_time* vp_time::create()
{
	vp_time_impl::instance = vp_time_impl{};
	return &vp_time_impl::instance;
}

VP_EXTERN_C VP_API vp_time* vp_time_create()
{
	return vp_time::create();
}

vp_impl_c_function0(vp_time, now, uint64_t);
vp_impl_c_function2(vp_time, diff, uint64_t, uint64_t, uint64_t);
vp_impl_c_function1(vp_time, since, uint64_t, uint64_t);
vp_impl_c_function1(vp_time, delta_time, uint64_t, uint64_t*);
vp_impl_c_function1(vp_time, seconds, double, uint64_t);
vp_impl_c_function1(vp_time, milliseconds, double, uint64_t);
vp_impl_c_function1(vp_time, microseconds, double, uint64_t);
vp_impl_c_function1(vp_time, nanoseconds, double, uint64_t);

#pragma endregion
