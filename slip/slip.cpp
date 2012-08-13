#include "stdafx.h"

#define _SLIP_ENABLE_

#include "slip.h"
#include "tracker.hpp"

namespace slip
{
	tracker *instance = NULL;

	sliptag maketag( const char* name )
	{
		if( instance == NULL )
			instance = new tracker;

		return instance->maketag(std::string(name));
	}

	void begin(sliptag tag)
	{
		instance->begin(tag);
	}
	void end(sliptag tag)
	{
		instance->end(tag);
	}

	void enable()
	{
		instance->enable();
	}

	void checkpoint()
	{
		instance->checkpoint();

	}

	void disable(bool report)
	{
		instance->disable(report);
	}

	void report()
	{
		instance->report();
	}


}