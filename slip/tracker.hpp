#ifndef SLIP_TRACKER_HPP
#define SLIP_TRACKER_HPP

#include <windows.h>
#include <string>
#include <map>
#include <stack>
#include <vector>
#include <assert.h>

namespace slip {

	class tracker
	{
	private:
		struct taginfo
		{						 
			sliptag tag;
			unsigned int count;
			LARGE_INTEGER time;
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

		};

		unsigned int m_tagcount;
		taginfo *info;

		std::map<std::string, sliptag> tagnames;
		std::stack<activetag> tagstack;
		calltree tree;
		std::stack<calltree*> treeposition;
		bool enabled;

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
			newtree.info.count = 0;
			newtree.info.time.QuadPart = 0;

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
			activetag newtag;
			newtag.tag = tag;
			
			QueryPerformanceCounter(&(newtag.start));
			tagstack.push(newtag);
			pushtree(tag);
		}

		void end(sliptag tag)
		{
			LARGE_INTEGER end;
			QueryPerformanceCounter(&end);

			activetag top = tagstack.top();

			assert(	top.tag == tag );

			tagstack.pop();

			calltree *tree = treeposition.top();

			tree->info.count++;
			tree->info.time.QuadPart += (end.QuadPart - top.start.QuadPart);

			info[tag].count++;
			info[tag].time.QuadPart += (end.QuadPart - top.start.QuadPart);

			poptree();
		}

		void enable()
		{
			enabled = true;
			if( info == NULL )
			{
				int size = tagnames.size();
				info = new taginfo[	size ];
				memset(info, 0, sizeof(taginfo) * size);
			}
		}

		void checkpoint()
		{
		
		}

		void disable(bool _report = true)
		{
			enabled = false;
			if( _report )
				report();
		}

		void report()
		{
			LARGE_INTEGER perffreq;
			QueryPerformanceFrequency(&perffreq);

			std::map<sliptag, std::string> names;

			for( std::map<std::string, sliptag>::const_iterator it = tagnames.begin(); it != tagnames.end(); ++it)
			{
				const std::string& name = (*it).first;
				sliptag tag = (*it).second;

				names[tag] = name;

				double time = (double)( info[tag].time.QuadPart * 1000000 / perffreq.QuadPart );

				char str[1024];
				sprintf_s(str, 1024, "%s (id: %d): took %fms for %d calls\n", name.c_str(), tag, time / 1000.0, info[tag].count);	 

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

				sprintf_s(tabstr, 1024-depth, "%s (id: %d): took %fms for %d calls\n", names[root.info.tag].c_str(), root.info.tag, time / 1000.0, root.info.count);	 

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