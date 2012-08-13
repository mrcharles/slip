#ifndef SLIP_TRACKER_HPP
#define SLIP_TRACKER_HPP

#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <string>
#include <map>
#include <stack>
#include <assert.h>

namespace slip {

	LARGE_INTEGER perffreq;
	inline double microseconds(const LARGE_INTEGER &i)
	{
		return (double)( i.QuadPart * 1000000 / perffreq.QuadPart );
	}


	class tracker
	{
	private:
		struct taginfo
		{						 
			sliptag tag;
			unsigned int count;
			LARGE_INTEGER time;

			double mintime;
			double maxtime;

			unsigned int totalcount;
			double totalmintime;
			double totalmaxtime;

			double totaltime;

			taginfo()
			{
				tag = 0;
				count = 0;

				time.QuadPart = 0;

				mintime = 1000000000.0;
				maxtime = 0;

				totalcount = 0;
				totalmintime = 1000000000.0;
				totalmaxtime = 0;
				totaltime = 0;

			}

			void addtime( LARGE_INTEGER idelta, double delta )
			{
				time.QuadPart += idelta.QuadPart;

				if( delta > maxtime )
					maxtime = delta;
				if( delta < mintime )
					mintime = delta;

				count++;
			}

			void checkpoint()
			{
				double ms = microseconds(time);

				totaltime += ms;

				if(mintime < totalmintime)
					totalmintime = mintime;
				if(maxtime > totalmaxtime)
					totalmaxtime = maxtime;
				totalcount += count;

				time.QuadPart = 0;
				count = 0;
				mintime = 1000000000.0;
				maxtime = 0;
			}

		};

		struct activetag
		{
			sliptag tag;
			LARGE_INTEGER start;
			LARGE_INTEGER end;
		};

		struct calltree
		{
			taginfo info;
			std::map<sliptag, calltree> children;

			void checkpoint()
			{
				info.checkpoint();

				for( std::map<sliptag, calltree>::iterator it = children.begin(); it != children.end(); ++it )
				{
					((*it).second).checkpoint();
				}
			}

		};

		unsigned int m_tagcount;
		taginfo *info;

		std::map<std::string, sliptag> tagnames;
		std::stack<activetag> tagstack;
		calltree tree;
		std::stack<calltree*> treeposition;
		bool enabled;
		bool firstupdate;

		void pushtree(sliptag tag)
		{
			calltree* root = treeposition.top();

			for( std::map<sliptag, calltree>::iterator it = root->children.begin(); it != root->children.end(); ++it )
			{
				if( (*it).first == tag ) //we already have a branch, push it
				{
					treeposition.push( &((*it).second) );
					return;
				}
			}


			//if we get here we need to add a map entry
			calltree newtree;
			newtree.info.tag = tag;

			root->children[tag] = newtree;
			treeposition.push(&(root->children[tag]));
		}
		void poptree()
		{
			treeposition.pop();
		}

	public:
		tracker()
			:tagstack( std::deque<activetag>(100) )
		{
			tree.info.tag = -1;
			enabled = false;
			info = NULL;
			m_tagcount = 0;
			treeposition.push(&tree);
			QueryPerformanceFrequency(&perffreq);
		}


		sliptag maketag( const std::string& name )
		{
			assert(enabled == false);
			if( tagnames.find( name ) != tagnames.end() )
				return tagnames[name];
			
			sliptag tag = m_tagcount++;
			tagnames[name] = tag;

			return tag;
		}

		void begin(sliptag tag)
		{
			if(!enabled) return;

			activetag newtag;
			newtag.tag = tag;
			
			QueryPerformanceCounter(&(newtag.start));
			tagstack.push(newtag);
			pushtree(tag);
		}

		void end(sliptag tag)
		{
			if(!enabled) return;
			LARGE_INTEGER end;
			QueryPerformanceCounter(&end);

			activetag top = tagstack.top();

			assert(	top.tag == tag );

			tagstack.pop();

			calltree *tree = treeposition.top();

			LARGE_INTEGER idelta;
			idelta.QuadPart = end.QuadPart - top.start.QuadPart;

			double time = microseconds(idelta);

			if( !firstupdate )
			{
				tree->info.addtime( idelta, time );

				info[tag].addtime(idelta, time);
			}

			poptree();
		}

		void enable()
		{
			enabled = true;
			if( info == NULL )
			{
				size_t size = tagnames.size();
				info = new taginfo[	size ];
				//memset(info, 0, sizeof(taginfo) * size);
			}
			firstupdate = true;
		}

		void checkpoint()
		{
			if(!enabled) return;
			if(firstupdate)
			{
				firstupdate = false;
				return;
			}
			for(int i = 0; i < m_tagcount; i++ )
				info[i].checkpoint();

			tree.checkpoint();
		
		}

		void disable(bool _report = true)
		{
			enabled = false;
			if( _report )
				report();
		}

		void report()
		{

			std::map<sliptag, std::string> names;

			for( std::map<std::string, sliptag>::const_iterator it = tagnames.begin(); it != tagnames.end(); ++it)
			{
				const std::string& name = (*it).first;
				sliptag tag = (*it).second;

				names[tag] = name;

				double time = microseconds(info[tag].time);

				char str[1024];
				sprintf_s(str, 1024, "%s (id: %d): took %fms for %d calls (%fms avg, %fms min, %fms max)\n", name.c_str(), tag, info[tag].totaltime / 1000.0, info[tag].totalcount, info[tag].totaltime / 1000.0 / info[tag].totalcount, info[tag].totalmintime / 1000.0, info[tag].totalmaxtime / 1000.0);	 

				OutputDebugStringA(str);

			}

			OutputDebugStringA("\ntree calls\n\n");
		
			treereport( tree, names );
		}
private:
		void treereport( const calltree& root, std::map<sliptag, std::string>& names, int depth = 0 )
		{
			LARGE_INTEGER perffreq;
			QueryPerformanceFrequency(&perffreq);

			if( root.info.tag != -1 )
			{
				char str[1024];
				double time = (double)( root.info.time.QuadPart * 1000000 / perffreq.QuadPart );
				char *tabstr = str;
				for(int t = 0; t < depth; t++)
				{
					sprintf_s(tabstr, 1024 - t, "\t");
					tabstr++;
				}

				sprintf_s(tabstr, 1024-depth, "%s (id: %d): took %fms for %d calls (%fms avg, %fms min, %fms max)\n", names[root.info.tag].c_str(), root.info.tag, root.info.totaltime / 1000.0, root.info.totalcount, root.info.totaltime / 1000.0 / root.info.totalcount, root.info.totalmintime / 1000.0, root.info.totalmaxtime / 1000.0);	 

				OutputDebugStringA(str);
			}
			for( std::map<sliptag, calltree>::const_iterator it = root.children.begin(); it != root.children.end(); ++it )
			{
				sliptag tag = (*it).first;
				const calltree& child = (*it).second;

				treereport( child, names, depth + 1 );
			}
			
		}
	};

}
#endif