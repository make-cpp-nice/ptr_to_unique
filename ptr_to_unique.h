#ifndef PTR_TO_UNIQUE_H
#define PTR_TO_UNIQUE_H

#include <memory> //std::unique_ptr.
#include <stdexcept>
#include <type_traits>

namespace xonor
{
//forward declaration of main public types
template <class U, class D> struct notify_ptrs;
template <class U> class ptr_to_unique;

//________________ notifying_unique_ptr<T, D>____________________

template<class T, class D = std::default_delete<T>>
using notifying_unique_ptr =
	typename std::template unique_ptr <T, notify_ptrs<T, D>>;


class _xnrptrs_internal
{
	_xnrptrs_internal() = delete;

	template <class U, class D> friend struct notify_ptrs;
	template <class U> friend class ptr_to_unique;
	template<class U, class... Types >
	friend auto point_into(ptr_to_unique<U> const& ptr, Types...  args);
	template<class U, class D, class... Types >
	friend auto point_into(notifying_unique_ptr<U, D> const& ptr, Types...  args);
private:	
	
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
			friend struct _xnrptrs_internal::_ptr_to_unique_cbx;
		private:
			unsigned int m_reference_count; //no. of ptr_to_uniques referencing it
			int m_valid_count; //valid if non-zero. Also used to store dynamic array size
			std::thread::id thread_id;

			enum {
				invalid = -1
			};
			
			inline bool _is_valid() const
			{
				if (thread_id != std::this_thread::get_id())
					throw std::exception();
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
			inline control_block(size_t count = 1)
				: m_reference_count(0), m_valid_count(int(count)) {
				
				thread_id = std::this_thread::get_id();
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

	class inwards_offsets
	{
		inwards_offsets() = delete;

		template <class T>
		friend class ptr_to_unique;
		template<class U, class... Types >
		friend auto point_into(ptr_to_unique<U> const& ptr, Types...  args);
		template<class U, class D, class... Types >
		friend auto point_into(notifying_unique_ptr<U, D> const& ptr, Types...  args);
	private:
		

		template<class T, class Type1>
		static auto& read_offsets(T& t, Type1 arg1) {
			return do_offset(t, arg1);
		}
		template<class T, class Type1, class... Types>
		static auto& read_offsets(T& t, Type1 arg1, Types...  args) {
			return read_offsets(do_offset(t,
				arg1), args...);
		}

		//with integer index
		template<class T, size_t N>
		static inline T& do_offset(T(&a)[N], size_t index) {
			return (index < N) ? a[index] : throw std::out_of_range("xnr::point_into bad index");
		}
		template<class T, size_t N>
		static inline T& do_offset(std::array<T, N>& a, size_t index) {
			return (index < N) ? a[index] : throw std::out_of_range("xnr::point_into bad index");
		}


		//with std::integral_constant index
		template<class T, size_t N, class IntType, IntType Index>
		static inline T& do_offset(T(&a)[N], std::integral_constant<IntType, Index> index) {
			using check_in_range = std::enable_if_t < Index<N >;
			return a[Index];
		}
		template<class T, size_t N, class IntType, IntType Index>
		static inline T& do_offset(std::array<T, N>& a, std::integral_constant<IntType, Index> index) {
			using check_in_range = std::enable_if_t < Index<N >;
			return a[Index];
		}
		
		//class member offset
		template<class T, class T2, class Targ,
			class = std::enable_if_t<std::is_base_of<T2, T>::value>>
			static inline Targ& do_offset(T& t, Targ T2::* member) {
			return t.*member;
		}

		template<int I, int S = I>
		struct tuple_reader
		{
			template<class T, class... Types >
			static auto& read(T& t, std::tuple<Types...> const& path) {
				return tuple_reader<I - 1, S>::read(
					do_offset(t, std::get<S - I>(path)),
					path);
			}
		};
		template<int S>
		struct tuple_reader<1, S>
		{
			template<class T, class... Types >
			static auto& read(T& t, std::tuple<Types...> const& path) {
				return do_offset(t, std::get<S - 1>(path));
			}
		};

		template<class T, class... Types >
		static auto& do_offset(T& t, std::tuple<Types...> const& path) {
			return tuple_reader<sizeof...(Types)>::read(t, path);
		}

		template<class U, class T, class... Types >
		static inline auto _point_into(ptr_to_unique<U> const& ptr, T* pT, Types...  args)
		{
			return ptr_to_unique<T>(ptr, args...);
		}
		template<class U, class D, class T, class... Types >
		static inline auto _point_into(notifying_unique_ptr<U, D> const& ptr, T* pT, Types...  args)
		{
			return ptr_to_unique<T>(ptr, args...);
		}
	};
	
};//end namespace _xnrptrs_internal

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
		mutable _xnrptrs_internal::_ptr_to_unique_cbx cbx;
	}; 
	InnerDeleter inner_deleter; // is a D

	template<class U>
	using if_base_of = std::enable_if_t<std::is_base_of<T, U>::value>;


	inline _xnrptrs_internal::_ptr_to_unique_cbx& get_cbx() const	{
		return inner_deleter.cbx;
	}

	template <class U, class D> friend struct ::xonor::notify_ptrs;
	template <class U> friend class ::xonor::ptr_to_unique;
public:	
	inline notify_ptrs() {}

	//Explicitly handle move from notifying_unique_ptr to notifying_unique_ptr
	inline notify_ptrs(notify_ptrs const& deleter) {
		get_cbx().mark_invalid();
		//get_cbx().pCB = deleter.get_cbx().pCB;
	}
	template< class U, class = if_base_of<U>>
	inline notify_ptrs(notify_ptrs<U> const& deleter) {
		get_cbx().mark_invalid();
		//get_cbx().pCB = deleter.get_cbx().pCB;
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


//________________ ptr_to_unique<T>________________
/*
*/

template <class T>
class ptr_to_unique
{	
	template<class U>
	using if_base_of = std::enable_if_t<std::is_base_of<T, U>::value>;
	
public:
	using element_type = T;
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

	//Alias construction to point at anything within the owned object
	template<class U, class... Types >
	inline ptr_to_unique(ptr_to_unique<U> const& ptr, Types...  args)
		: ptr_to_unique(ptr, &_xnrptrs_internal::inwards_offsets::read_offsets(
			*(ptr.get()), args...))
	{}

	template <class U, class Del, class... Types >
	inline ptr_to_unique(notifying_unique_ptr<U, Del>const& ptr, Types...  args)
		: ptr_to_unique(ptr, &_xnrptrs_internal::inwards_offsets::read_offsets(
			*(ptr.get()), args...))
	{}

	//also PROHIBIT from a notifying_unique_ptr going out of scope
	template <class U, class Del, class... Types >
	inline ptr_to_unique(notifying_unique_ptr<U, Del>const&& ptr, Types...  args) = delete;
	
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
		if(T* p= checked_pointer())
			return p;
		else
		{
			return nullptr;
		}
			
	}
	inline T& operator*() const {
		if (cbx.check_valid())
			return *m_pT;
		else
		{
			throw std::runtime_error("ptr_to_unique operator*() null object dereference");
		}
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
	template<class U, class... Types >
	friend auto point_into(ptr_to_unique<U> const& ptr, Types...  args);
	
	//-----------------Data members------------------------
	//The pointer, local copy - ignored when control block says invalid
	mutable T* m_pT;
	//control block connection - hold reliable validity flag
	mutable _xnrptrs_internal::_ptr_to_unique_cbx cbx;
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

	//Private aliasing constructors called by public aliasing constructord

	//from notifying_unique_ptr
	template <class U, class Del>
	inline ptr_to_unique(notifying_unique_ptr<U, Del>const& ptr, T* pTar) {
		if ((m_pT = (ptr.get()) ? pTar : nullptr))
			cbx.assure_and_adopt_owner_block(ptr.get_deleter().get_cbx());
	}
	template <class U, class Del> //don't cosume a unique_ptr
	inline ptr_to_unique(notifying_unique_ptr<U, Del>const&& ptr, T* pTar) = delete;

	//from ptr_to_unique
	template <class U>
	inline ptr_to_unique(ptr_to_unique<U>const& ptr, T* pTar) {
		cbx.adopt_block(ptr.cbx);
		m_pT = pTar;
	}
	inline ptr_to_unique(
		_xnrptrs_internal::_ptr_to_unique_cbx const& _cbx, T* pTar) {	//Called when source is falling out of scope 
		cbx.steal_block(_cbx); //slightly faster
		m_pT = pTar;
	}

	
};
/*
template <class T>
inline auto point_into(ptr_to_unique<T> const& ptr)
{
	return ptr_to_unique<T>(ptr);
}
template <class T, class D>
inline auto point_into(notifying_unique_ptr<T, D> const& ptr)
{
	return ptr_to_unique<T>(ptr);
}
*/


template<class U, class... Types >
inline auto point_into(ptr_to_unique<U> const& ptr, Types...  args)
{	
	return _xnrptrs_internal::inwards_offsets::_point_into(
		ptr, &_xnrptrs_internal::inwards_offsets::read_offsets(
			*(ptr.get()), args...), args...);
}
template<class U, class D, class... Types >
inline auto point_into(notifying_unique_ptr<U, D> const& ptr, Types...  args)
{
	return _xnrptrs_internal::inwards_offsets::_point_into(
		ptr, &_xnrptrs_internal::inwards_offsets::read_offsets(
			*(ptr.get()), args...), args...);
}


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