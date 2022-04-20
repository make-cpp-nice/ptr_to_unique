#ifndef PTR_TO_UNIQUE_H
#define PTR_TO_UNIQUE_H

#include <memory> //std::unique_ptr.
#include <stdexcept>
#include <type_traits>

namespace xonor
{

template <class T>
struct an_allocating_deleter
{
	//required by std::unique_ptr
	void operator()(T* p)
	{
		delete p;
	}
	//required by make_notifying_unique
	template <class... Types>
	static inline T* allocate(Types&&... _Args)
	{
		return new T(std::forward<Types>(_Args)...);
	}
};

//forward declaration of main public types
template <class U, class D> struct notify_ptrs;
template <class U> class ptr_to_unique;

namespace _internal
{
	//friends that will enable extended funcionality
	template<class T> class _inwards_offset;
	template<class T> class _inwards_index_offset;

	//________________ _ptr_to_unique_cbx____________________
	/*
	cbx = Control Block conneXion
	Encapsulates all non object type dependant (non-templated) activity associated with the
	control block. Its only data member is a pointer to the control block which may be null.
	All accesses to the control block are via _ptr_to_unique_cbx so it also privately holds the
	definition and implementation of the control block.
	*/

	struct _ptr_to_unique_cbx
	{
		//Only these classes have access to private constructor
		template <class U, class D> friend struct ::xonor::notify_ptrs;
		template <class U> friend class ::xonor::ptr_to_unique;
	private:
		class control_block
		{
			friend struct _ptr_to_unique_cbx;
		private:
			unsigned int m_reference_count; //no. of ptr_to_uniques referencing it
			int m_valid_count; //valid if non-zero. Also used to store dynamic array size

			enum {
				invalid = -1
			};
			
			inline bool _is_valid() const
			{
				return (m_valid_count> invalid);
			}
			inline void _invalidate()
			{
				m_valid_count = invalid;
			}
			inline bool _dec_weak_can_delete()
			{
				return (--m_reference_count + m_valid_count<0);
			}

		public:
			//construction
			inline constexpr control_block(size_t count = 1)
				: m_reference_count(0), m_valid_count(count) {
			}
			// tests if m_valid_count is set non-zero
			inline bool get_valid() const {
				return _is_valid();

			}
			//self destructs if no other references - otherwise zeroes m_valid_count
			inline void mark_invalid() {
				if (0 == m_reference_count)
					delete this;
				else
					_invalidate();
			}
			//increments observer count 
			inline void add_weak() {
				m_reference_count++;
			}
			//decrements observer count and self destructs if result is zero
			inline void release_weak() {
				if (_dec_weak_can_delete())
					delete this;//nothing left referencing it
			}
		};//end class control_block

		mutable control_block* pCB;

		_ptr_to_unique_cbx() : pCB(nullptr) {
		}
		inline bool check_valid() const {
			return (pCB) ?
				(pCB->_is_valid()) ? true : quick_release()
				: false;
		}
		// only used by check_valid()
		bool quick_release() const {
			pCB->release_weak();
			pCB = nullptr;
			return false;
		}
		void release() const {
			if (pCB)
			{
				pCB->release_weak();
				pCB = nullptr;
			}
		}
	public:
		bool adopt_block_if_valid(_ptr_to_unique_cbx const& src) {
			if (src.pCB && src.pCB->get_valid())
			{
				src.pCB->add_weak();
				pCB = src.pCB;
				return true;
			}
			else
				return false;
		}
		void adopt_block(_ptr_to_unique_cbx const& src)	{
			if (src.pCB)
			{
				src.pCB->add_weak();
				pCB = src.pCB;
			}
		}
		inline void steal_block(_ptr_to_unique_cbx const& src) {
			pCB = src.pCB;
			src.pCB = nullptr;
		}
		void assure_and_adopt_owner_block(_ptr_to_unique_cbx const& src, size_t array_count = 1) {
			if (nullptr == src.pCB)
				src.pCB = new  control_block(array_count);
			src.pCB->add_weak();
			pCB = src.pCB;
		}
		inline void mark_invalid() const {
			if (pCB)
			{
				pCB->mark_invalid();
				pCB = nullptr;
			}
		}
		void set_owner_block(size_t array_count)
		{
			if (pCB)
				pCB->mark_invalid();
			pCB = new  control_block(array_count);
		}
		inline size_t array_count() const {
			return (nullptr== pCB || pCB->m_valid_count<0)?	
				0 : pCB->m_valid_count; 
		}
	};//end struct _ptr_to_unique_cbx

	template<class T, class D>
	struct make_chooser
	{
		template <class D, class... Types>
		static auto make(Types&&... _Args) {
			return (D:: template allocate(std::forward<Types>(_Args)...));
		}
	};
	template<class T>
	struct make_chooser<T, std::default_delete<T>>
	{
		template <class D = std::default_delete<T>, class... Types>
		static auto make(Types&&... _Args) {
			return (new T(std::forward<Types>(_Args)...));
		}
	};

}//end namespace _internal

//________________ notify_ptrs<T, D>____________________
/*
	A transparent notifying custom deleter for unique_ptr.
	Holds a pointer to the control block (as a _ptr_to_unique_cbx). When called 
	on deletion, it zeroes the valid_count (used by ptr_to_unique to determine
	validity) and calls the passed in deleter D to do the deletion.
*/
template <class T, class D = std::default_delete<T>>
struct notify_ptrs // if we derive from D, then operator D& () will never be called
{
private:
	//D will typically be dataless so we still need empty base class optimisation
	struct InnerDeleter: public D
	{
		mutable _internal::_ptr_to_unique_cbx cbx;
	}; 
	InnerDeleter inner_deleter; // is a D

	template<class U>
	using if_base_of = std::enable_if_t<std::is_base_of<T, U>::value>;


	inline _internal::_ptr_to_unique_cbx& get_cbx() const	{
		return inner_deleter.cbx;
	}

	template <class U, class D> friend struct ::xonor::notify_ptrs;
	template <class U> friend class ::xonor::ptr_to_unique;
public:	
	inline notify_ptrs() {}

	//Explicitly handle move from notifying_unique_ptr to notifying_unique_ptr
	inline notify_ptrs(notify_ptrs const& deleter) {
		get_cbx().pCB = deleter.get_cbx().pCB;
	}
	template< class U, class = if_base_of<U>>
	inline notify_ptrs(notify_ptrs<U> const& deleter) {
		get_cbx().pCB = deleter.get_cbx().pCB;
	}
	
	//permit move from unique_ptr to notifying_unique_ptr
	template< class D2, 
		class = std::enable_if_t<std::is_convertible<D2, D>::value>>
	inline notify_ptrs(const D2& deleter) {	
	}
	template< class D2, 
		class = std::enable_if_t<std::is_convertible<D2, D>::value>>
	inline auto& operator = (const D2& deleter)	{	
		get_cbx().release();
		return *this;
	}

	//Permits and intercepts move from notifying_unique_ptr to unique_ptr
	operator D& ()	{	///plain unique_ptr doesnt support ref_ptrs
		//so any that reference this object must be zeroed
		get_cbx().mark_invalid();
		return inner_deleter; //return the passed in deleter
	}

	//The functor call to do the deletion
	void operator()(T* p) {
		//zero ref_ptrs that reference this object
		get_cbx().mark_invalid();
		//leave deletion to passed in deleter
		inner_deleter(p);
	}

	void reset_all_ptrs() {
		get_cbx().mark_invalid();
	}
};

//________________ notifying_unique_ptr<T, D>____________________
/*
	Shorthand for std::unique_ptr <T, notify_ptrs<T, D>>
*/

template<class T, class D = std::default_delete<T>>
using notifying_unique_ptr =
	typename std::template unique_ptr <T, notify_ptrs<T, D>>;

template <class T, class D = std::default_delete<T>, class... Types,
	std::enable_if_t<!std::is_array_v<T>, int> = 0>
inline auto make_notifying_unique(Types&&... _Args) {
	return notifying_unique_ptr<T, D>(
		_internal::make_chooser<T, D>::template make<D>(std::forward<Types>(_Args)...));
}
template <class T, class D = std::default_delete<T>, class... Types,
	std::enable_if_t<!std::is_array_v<T>, int> = 0>
	inline auto custom_make_unique(Types&&... _Args) {
	return std::unique_ptr<T, D>(
		_internal::make_chooser<T, D>::template make<D>(std::forward<Types>(_Args)...));
}
//________________ ptr_to_unique<T>________________
/*
*/
template <class T>
class ptr_to_unique
{
	template<class U>
	using if_base_of = std::enable_if_t<std::is_base_of<T, U>::value>;
	template<class U>
	using if_derived_from = std::enable_if_t<std::is_base_of<U, T>::value>;
public:
	
	//default constructor - initialises as nullptr
	inline ptr_to_unique() {}
	
	//destructor
	inline ~ptr_to_unique() {
		cbx.release(); 
	}
	
	//Construction and assignment to nullptr
	inline ptr_to_unique(std::nullptr_t) {}
	inline ptr_to_unique const& operator=(std::nullptr_t) {
		cbx.release();	return *this;
	}

	//Copy Construction and assignment explicit to prevent compiler defaults
	inline ptr_to_unique(ptr_to_unique const & ptr) {
		point_to(ptr);
	}
	inline ptr_to_unique const& operator=(ptr_to_unique const & ptr) {
		cbx.release();	point_to(ptr); return *this;
	}
		
	//Construction and assignment from ptr_to_unique<U>
	template <class U, class = if_base_of<U>>
	inline ptr_to_unique(ptr_to_unique<U> const & ptr) {
		point_to(ptr);
	}
	template <class U, class = if_base_of<U>>
	inline ptr_to_unique<T> const& operator = (ptr_to_unique<U> const & ptr) {
		cbx.release(); point_to(ptr); return *this;
	}
	
	//Move Construction and assignment
	inline ptr_to_unique(ptr_to_unique const&& ptr) noexcept {
		accept_move(ptr);
	}
	inline ptr_to_unique const& operator = (ptr_to_unique const&& ptr) noexcept {
		cbx.release(); accept_move(ptr); return *this;
	}
	
	//Move Construction and assignment from ptr_to_unique<U>
	template <class U, class = if_base_of<U>>
	inline ptr_to_unique(ptr_to_unique<U> const&& ptr) {
		accept_move(ptr);
	}
	template <class U, class = if_base_of<U>>
	inline ptr_to_unique<T> const& operator = (ptr_to_unique<U> const&& ptr) {
		cbx.release(); accept_move(ptr); return *this;
	}

	//Construction and assignment from notifying_unique_ptr
	template <class Del>
	inline ptr_to_unique(notifying_unique_ptr<T, Del>const& ptr) {
		point_to(ptr);
	}
	template <class Del>
	inline ptr_to_unique& operator=(notifying_unique_ptr<T, Del>const& ptr) {
		cbx.release(); point_to(ptr); return *this;
	}
	
	//Construction and assignment from notifying_unique_ptr<U>
	template <class U, class Del, class = if_base_of<U>>
	inline ptr_to_unique(notifying_unique_ptr<U, Del>const& ptr) {
		point_to(ptr);
	}
	template <class U, class Del, class = if_base_of<U>>
	inline ptr_to_unique& operator=(notifying_unique_ptr<U, Del>const& ptr)	{
		cbx.release(); point_to(ptr); return *this;
	}

	//but PROHIBIT from a notifying_unique_ptr going out of scope
	template <class U, class Del>
	ptr_to_unique(notifying_unique_ptr<U, Del>const&& ptr) = delete;
	template <class U, class Del>
	ptr_to_unique& operator=(notifying_unique_ptr<U, Del>const&& ptr) = delete;
	
	//boolean test
	inline explicit operator bool() const {
		return cbx.check_valid();
	}
		
	// get() extract raw pointer 
	inline T* get() const {
		return checked_pointer();
	}
	
	//dereference 
	inline T* const operator->() const {
		return checked_pointer();
	}
	inline T& operator*() const {
		return (cbx.check_valid()) ? *m_pT 
			: throw std::runtime_error("ptr_to_unique operator*() null object dereference");
	}
	
	//casting
	template<class U, class = if_base_of<U>>
	ptr_to_unique<U> dynamic_ptr_cast()	{
		ptr_to_unique<U> ptr;
		ptr.m_pT = dynamic_cast<U*>(m_pT);
		ptr.cbx.adopt_block(cbx);
		return ptr;
	}

private:

	template <class U> friend class ptr_to_unique;
	template<class _T> friend class _internal::_inwards_offset;
	template<class _T> friend class _internal::_inwards_index_offset;

	//-----------------Data members------------------------
	//The pointer, local copy - ignored when control block says invalid
	mutable T* m_pT;
	//control block connection - hold reliable validity flag
	mutable _internal::_ptr_to_unique_cbx cbx;
	//------------------------------------------------------
	
	//service functions for construction and assignment
	template <class U>
	inline void point_to(ptr_to_unique<U>const& ptr) {
		if (cbx.adopt_block_if_valid(ptr.cbx))
			m_pT = ptr.m_pT;
	}
	template <class U>
	inline void accept_move(ptr_to_unique<U>const& ptr)	{
		cbx.steal_block(ptr.cbx);
		m_pT = ptr.m_pT;
	}
	template <class U, class D>
	inline void point_to(notifying_unique_ptr<U, D>const& ptr)	{
		if ((m_pT = ptr.get()))
			cbx.assure_and_adopt_owner_block(ptr.get_deleter().get_cbx());
	}
	inline T* const checked_pointer() const {
		return (cbx.check_valid()) ? m_pT : nullptr;
	}

	//Private aliasing constructors not called - available for extended functionality

	//from notifying_unique_ptr
	template <class U, class Del>
	inline ptr_to_unique(notifying_unique_ptr<U, Del>const& ptr, T* pTar) {
		if ((m_pT = (ptr.get()) ? pTar : nullptr))
			cbx.assure_and_adopt_owner_block(ptr.get_deleter().cbx);
	}
	template <class U, class Del> //don't cosume a unique_ptr
	inline ptr_to_unique(notifying_unique_ptr<U, Del>const&& ptr, T* pTar) = delete;

	//from ptr_to_unique
	template <class U>
	inline ptr_to_unique(ptr_to_unique<U>const& ptr, T* pTar) {
		cbx.adopt_block(ptr.cbx);
		m_pT = pTar;
	}
	inline ptr_to_unique(_internal::_ptr_to_unique_cbx const& _cbx, T* pTar) {	//Called when source is falling out of scope 
		cbx.steal_block(_cbx); //slightly faster
		m_pT = pTar;
	}
};

//------ external operations defined for notifying_unique_ptr<T, D> -----

// zero_ptrs_to - reset all ptr_to_uniques that reference this object
template<class T, class D>
inline auto& zero_ptrs_to(notifying_unique_ptr<T, D>& ptr) {
	ptr.get_deleter().reset_all_ptrs();
	return ptr;
}
template<class T, class D> //dont't consume a unique_ptr
inline auto& zero_ptrs_to(notifying_unique_ptr<T, D>&& ptr) = delete;


//________________Comparison - only == and != ____________________-

//________________ ptr_to_unique with nullptr ________________________-

// ==
template<class L>
inline bool operator == (ptr_to_unique<L> const& l_ptr, std::nullptr_t) {
	return l_ptr.get() == nullptr;
}
template<class R>
inline bool operator == (std::nullptr_t, ptr_to_unique<R> const& r_ptr) {
	return nullptr == r_ptr.get();
}
// !=
template<class L>
inline bool operator != (ptr_to_unique<L> const& l_ptr, std::nullptr_t) {
	return l_ptr.get() != nullptr;
}
template<class R>
inline bool operator != (std::nullptr_t, ptr_to_unique<R> const& r_ptr) {
	return nullptr != r_ptr.get();
}

// __________________ptr_to_unique with raw_pointer_______________________-

// ==
template<class L, class R>
inline bool operator == (ptr_to_unique<L> const& l_ptr, R const* p) {
	return l_ptr.get() == p;
}
template<class L, class R>
inline bool operator == (L const* p, ptr_to_unique<R> const& r_ptr) {
	return p == r_ptr.get();
}
// !=
template<class L, class R>
inline bool operator != (ptr_to_unique<L> const& l_ptr, R const* p) {
	return l_ptr.get() != p;
}
template<class L, class R>
inline bool operator != (L const* p, ptr_to_unique<R> const& r_ptr) {
	return p != r_ptr.get();
}

// ____________________ptr_to_unique with ptr_to_unique_____________________

// ==
template<class L, class R>
inline bool operator == (ptr_to_unique<L> const& l_ptr, ptr_to_unique<R> const& r_ptr) {
	return l_ptr.get() == r_ptr.get();
}
// !=
template<class L, class R>
inline bool operator != (ptr_to_unique<L> const& l_ptr, ptr_to_unique<R> const& r_ptr) {
	return l_ptr.get() != r_ptr.get();
}

// _________________________ptr_to_unique with notifying_unique_ptr_________________

// ==
template<class L, class R, class D>
inline bool operator == (ptr_to_unique<L> const& l_ptr, notifying_unique_ptr<R, D> const& r_ptr) {
	return l_ptr.get() == r_ptr.get();
}
template<class L, class R, class D>
inline bool operator == (notifying_unique_ptr<L, D> const& l_ptr, ptr_to_unique<R> const& r_ptr) {
	return l_ptr.get() == r_ptr.get();
}
// !=
template<class L, class R, class D>
inline bool operator != (ptr_to_unique<L> const& l_ptr, notifying_unique_ptr<R, D> const& r_ptr) {
	return l_ptr.get() != r_ptr.get();
}
template<class L, class R, class D>
inline bool operator != (notifying_unique_ptr<L, D> const& l_ptr, ptr_to_unique<R> const& r_ptr) {
	return l_ptr.get() != r_ptr.get();
}



}//end namespace xonor

namespace xnr = xonor;

#endif //PTR_TO_UNIQUE_H