#pragma once

#include "bookmark_preferences.h"
#include "bookmark_types.h"
#include "bookmark_persistence.h"
#include <vector>

class bookmark_store {
private:
	//..
public:
	bookmark_store();
	~bookmark_store();

	const std::vector<bookmark_t>& GetMasterList() {
		return m_masterList;
	}

	size_t Size() {
		return m_masterList.size();
	};
	
	bool Initialize() { 
		m_persist.readDataFileJSON(m_masterList); /*todo*/return true;
	}

	//todo

	const bookmark_t _getItem(size_t pos) {
		return m_masterList.at(pos);
	}
	const bookmark_t GetItem(size_t pos) {
		return _getItem(pos);
	}

	void _setItem(size_t pos, bookmark_t rec) {
		m_masterList[pos] = rec;
		m_persist.writeDataFile(m_masterList);
	}
	void SetItem(size_t pos, bookmark_t rec) {
		_setItem(pos, rec);
	}

	void _addItem(const bookmark_t rec) {
		m_masterList.emplace_back(rec);
	}
	void AddItem(const bookmark_t rec) {
		_addItem(rec);
	}

	void _reorder(const pfc::array_t<t_size> p_order, t_size p_count) {
		pfc::reorder_t(m_masterList, p_order.get_ptr(), p_count);
	}
	void Reorder(const pfc::array_t<t_size> p_order, t_size p_count) {
		_reorder(p_order, p_count);
	}

	bool _write() {
		m_persist.writeDataFile(m_masterList); /*todo*/ return true;
	}
	void Write() {
		_write();
	}

	void _remove(const bit_array& p_mask) {
		pfc::remove_mask_t(m_masterList, p_mask);
	}
	void Remove(const bit_array_bittable p_mask) {
		_remove(p_mask);
	}

	void _clear() { m_masterList.clear(); }
	void Clear() {
		_clear();
	}

private:
	std::vector<bookmark_t> m_masterList;
	bookmark_persistence m_persist;
};


namespace std {
	inline void swap(bookmark_t& a, bookmark_t& b)
	{
		a.swap(b);
	}
}
