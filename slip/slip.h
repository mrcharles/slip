#ifndef SLIPLIB_H
#define SLIPLIB_H

typedef int sliptag;

#define DECLARESLIPTAG(name) const static sliptag SLIP_##name = slip::maketag(#name)

#ifdef _SLIP_ENABLE_

namespace slip {

sliptag maketag( const char* name );

void begin(sliptag tag);
void end(sliptag tag);

void enable();
void checkpoint();
void disable(bool report = true);

void report();

	class scoper
	{
		sliptag m_tag;
	public:
		scoper( sliptag tag )
		{
			m_tag = tag;
			slip::begin(tag);
		}
		~scoper()
		{
			slip::end(m_tag);
		}
	};
}

#else

class slip
{
public:
	static void begin(sliptag tag){}
	static void end(sliptag tag){}

	static void enable(){}
	static void checkpoint(){}
	static void disable(bool report = true){}

	static void report(){}

	static sliptag maketag(const char*) { return 0; }
	class scoper
	{
	public:
		scoper( sliptag tag )
		{
			(void)tag;
		}
	};


};

#endif



#endif