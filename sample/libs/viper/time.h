#pragma once

#ifndef VP_APP_INCLUDED
#error "Please include <viper/app.h> before <viper/time.h>"
#endif

#if (defined(__cplusplus) && !defined(VP_C_INTERFACE))
// ViperTime core struct
vp_begin_struct(vp_time)
	// Initializer
	VP_API static vp_time* create();

	// Time value getters
	virtual uint64_t now() = 0;
	virtual uint64_t diff(uint64_t new_ticks, uint64_t old_ticks) = 0;
	virtual uint64_t since(uint64_t start_ticks) = 0;
	virtual uint64_t delta_time(uint64_t* last_time) = 0;

	// Time converters
	virtual double seconds(uint64_t ticks) = 0;
	virtual double milliseconds(uint64_t ticks) = 0;
	virtual double microseconds(uint64_t ticks) = 0;
	virtual double nanoseconds(uint64_t ticks) = 0;
vp_end(vp_time);
#endif

#ifdef VP_C_INTERFACE
typedef struct vp_time vp_time;

// Initializer
VP_EXTERN_C VP_API vp_time* vp_time_create();

VP_EXTERN_C VP_API uint64_t vp_time_now(vp_time* tm);
VP_EXTERN_C VP_API uint64_t vp_time_diff(vp_time* tm, uint64_t new_ticks, uint64_t old_ticks);
VP_EXTERN_C VP_API uint64_t vp_time_since(vp_time* tm, uint64_t start_ticks);
VP_EXTERN_C VP_API uint64_t vp_time_delta_time(vp_time* tm, uint64_t* last_time);

VP_EXTERN_C VP_API double vp_time_seconds(vp_time* tm, uint64_t ticks);
VP_EXTERN_C VP_API double vp_time_milliseconds(vp_time* tm, uint64_t ticks);
VP_EXTERN_C VP_API double vp_time_microseconds(vp_time* tm, uint64_t ticks);
VP_EXTERN_C VP_API double vp_time_nanoseconds(vp_time* tm, uint64_t ticks);
#endif
