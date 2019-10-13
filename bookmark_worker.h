#pragma once

#include <vector>
#include <list>
#include "bookmark_types.h"

class bookmark_worker
{
public:
	bookmark_worker();
	~bookmark_worker();

	static void store(std::vector<bookmark_t>& masterList);
	static void restore(std::vector<bookmark_t> & masterList, size_t index);
};

