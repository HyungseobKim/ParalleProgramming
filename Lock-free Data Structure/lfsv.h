#include <iostream>       // std::cout
#include <atomic>         // std::atomic
#include <thread>         // std::thread
#include <vector>         // std::vector
#include <deque>          // std::deque
#include <mutex>          // std::mutex

struct Pair 
{
    std::vector<int>* pointer;
    long              ref_count;
}; // __attribute__((aligned(16),packed));
// for some compilers alignment needed to stop std::atomic<Pair>::load to segfault

class MemoryBank 
{
public:
	MemoryBank()
		: slots(6000) 
	{
		for ( int i=0; i<6000; ++i )
			slots[i] = reinterpret_cast<std::vector<int>*>( new char[ sizeof(std::vector<int>) ] );
	}
	
	~MemoryBank() 
	{
		for ( auto & el : slots )
			delete [] reinterpret_cast<char*>( el );
	}
	
	std::vector<int>* Get() 
	{
		std::lock_guard<std::mutex> lock( m );
		std::vector<int>* p = slots[0];
		slots.pop_front();
		return p;
	}
	
	void Store( std::vector<int>* p )
	{
		std::lock_guard<std::mutex> lock( m );
		slots.push_back( p );
	}
	
private:
	std::deque<std::vector<int>*> slots;
    std::mutex m;
};

class LFSV 
{
public:
    LFSV() 
		: mb(), pdata(Pair{new (mb.Get()) std::vector<int>, 1}) 
	{
		// std::cout << "Is lockfree " << pdata.is_lock_free() << std::endl;
    }

    ~LFSV() { 
        Pair temp = pdata.load();
        std::vector<int>* p = temp.pointer;
        
		//delete p;
		p->~vector();
		mb.Store(p);
    }

    void Insert( int const & v ) {
        Pair pdata_new, pdata_old;
		
		pdata_old.ref_count = 1;
        pdata_new.pointer  = nullptr;
		pdata_new.ref_count = 1;
		std::vector<int>* last = nullptr;
		
        do {
			pdata_old.pointer = pdata.load().pointer;
			
			if (last != pdata_old.pointer)
			{
				//delete pdata_new.pointer;
				std::vector<int>* p = pdata_new.pointer;
				if (p != nullptr)
				{
					p->~vector();
					mb.Store(p);
				}
				
				pdata_new.pointer = new (mb.Get()) std::vector<int>(*pdata_old.pointer); // pdata_old may be deleted

				// working on a local copy
				std::vector<int>::iterator b = pdata_new.pointer->begin();
				std::vector<int>::iterator e = pdata_new.pointer->end();
				
				if (b == e || v >= pdata_new.pointer->back()) 
					pdata_new.pointer->push_back(v); //first in empty or last element
				else 
				{
					for ( ; b != e; ++b) {
						if ( *b >= v ) {
							pdata_new.pointer->insert( b, v );
							break;
						}
					}
				}
				
				last = pdata_old.pointer;
			}
        } while (!pdata.compare_exchange_weak( pdata_old, pdata_new));
        //delete pdata_old.pointer;
		pdata_old.pointer->~vector();
		mb.Store(pdata_old.pointer);
    }

    int operator[] ( int pos ) { // not a const method anymore
        Pair pdata_new, pdata_old;
		// std::cout << "Read from " << pdata_new.pointer;
		do {
			pdata_old = pdata;
			pdata_new = pdata_old;
			++pdata_new.ref_count;
		} while (!pdata.compare_exchange_weak(pdata_old, pdata_new));
		
        int ret_val = (*pdata_new.pointer)[pos];
		
		do {
			pdata_old = pdata;
			pdata_new = pdata_old;
			--pdata_new.ref_count;
		} while (!pdata.compare_exchange_weak( pdata_old, pdata_new));
		
        return ret_val;
    }
	
private:
	MemoryBank mb;
	std::atomic<Pair> pdata;
};
