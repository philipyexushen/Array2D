/* Philip 2017.5.17
 * Array2D (custom STL contrainer)
 * SquareMartrix是方阵容器，继承于Array2D，提供类C的双下标运算符操作
 * 带写时复制(copy on write)
 * 适配大部分的STL算法，部分STL算法（比如remove），行为未定义
*/

#ifndef ARRARY
#define ARRARY

#include <algorithm>
#include <iostream>
#include <xutility>
#include <allocators>

#define OVERRIDE

namespace arr
{
	using std::enable_if;
	using std::size_t;
	using std::allocator;
	using std::random_access_iterator_tag;
	using std::iterator_traits;
	using std::uninitialized_copy;
	using std::ptrdiff_t;
	using std::false_type;
	using std::true_type;
	using std::is_pod;
	using std::pair;
	using std::reverse_iterator;

	//Array2D常量迭代器
	template<typename _Elem>
	class Array2D_Const_Iterator
	{
	public:
		typedef Array2D_Const_Iterator<_Elem> _Myiter;
		typedef random_access_iterator_tag iterator_category;
		typedef _Elem value_type;
		typedef ptrdiff_t difference_type;
		typedef const _Elem & reference;
		typedef const _Elem * pointer;

		Array2D_Const_Iterator()
			:_ptr(0)
		{ }

		Array2D_Const_Iterator(pointer ptr)
			:_ptr(ptr)
		{ }

		reference operator*()const
		{ 
			return *_ptr; 
		}

		pointer operator->()const
		{ 
			return _ptr; 
		}

		_Myiter &operator++()
		{
			_ptr++;
			return *this;
		}

		_Myiter operator++(int)
		{
			_Myiter tmp = *this;
			++(*this);
			return tmp;
		}

		_Myiter &operator--()
		{
			--_ptr;
			return *this;
		}

		_Myiter operator--(int)
		{
			_Myiter tmp = *this;
			--(*this);
			return tmp;
		}

		_Myiter &operator+=(difference_type _off)
		{
			_ptr += _off;
			return *this;
		}

		_Myiter &operator-=(difference_type _off)
		{
			_ptr += -_off;
			return *this;
		}

		_Myiter operator+(difference_type _off)const
		{
			_Myiter tmp = *this;
			return tmp += _off;
		}

		_Myiter operator-(difference_type _off)const
		{
			_Myiter tmp = *this;
			return tmp -= _off;
		}

		difference_type operator-(const _Myiter &rhs)const
		{
			return _ptr - rhs._ptr;
		}

		bool operator==(const _Myiter &rhs)
		{
			return _ptr == rhs._ptr;
		}

		bool operator!=(const _Myiter &rhs)
		{
			return !(_ptr == rhs._ptr);
		}

		bool operator<(const _Myiter &rhs)
		{
			return _ptr < rhs._ptr;
		}

		bool operator>(const _Myiter &rhs)
		{
			return rhs < *this;
		}

		bool operator<=(const _Myiter &rhs)
		{
			return !(rhs < *this);
		}

		bool operator>=(const _Myiter &rhs)
		{
			return !(*this < rhs);
		}
	protected:
		pointer _ptr;
	};

	//Array2D迭代器
	template<typename _Elem>
	class Array2D_Iterator : public Array2D_Const_Iterator<_Elem>
	{
	public:
		typedef Array2D_Iterator<_Elem> _Myiter;
		typedef Array2D_Const_Iterator<_Elem> _Mybase;
		typedef random_access_iterator_tag iterator_category;
		typedef _Elem value_type;
		typedef ptrdiff_t difference_type;
		typedef _Elem & reference;
		typedef _Elem * pointer;

		Array2D_Iterator()
			:_ptr(0)
		{ }

		Array2D_Iterator(pointer ptr)
			:_Mybase(ptr)
		{ }

		reference operator*() const
		{ 
			return ((reference)* *(_Mybase *)this); 
		}

		pointer operator->()const
		{ 
			return (pointer)_ptr; 
		}

		_Myiter &operator++()
		{ 
			++*(_Mybase *)this;
			return *this;
		}

		_Myiter operator++(int)
		{
			_Myiter tmp = *this;
			++(*this);
			return tmp;
		}

		_Myiter &operator--()
		{
			--*(_Mybase *)this;
			return *this;
		}

		_Myiter operator--(int)
		{
			_Myiter tmp = *this;
			--(*this);
			return *this;
		}

		_Myiter &operator+=(difference_type _off)
		{
			*(_Mybase *)this += _off;
			return *this;
		}

		_Myiter &operator-=(difference_type _off)
		{
			return (*this += -_off);
		}

		_Myiter operator+(difference_type _off)const
		{
			_Myiter tmp = *this;
			return tmp += _off;
		}

		_Myiter operator-(difference_type _off)const
		{
			_Myiter tmp = *this;
			return tmp -= _off;
		}

		difference_type operator-(const _Myiter &rhs)const
		{
			return *(_Mybase *)this - rhs;
		}
	};

	//带写时复制功能对象基类
	template<typename _Vec>
	class _RCObject
	{
	public:
		typedef _RCObject<_Vec> _Myt;
		_RCObject()
			:_refCount(0), _shareable(true) { }

		_RCObject(const _Myt &)
			:_refCount(0), _shareable(true) { }

		_Myt &operator=(const _Myt &)
		{
			return *this;
		}

		void addRef()
		{
			_refCount++;
		}

		void decRef()
		{
			if (--_refCount == 0)
				delete this;
		}

		void markUnshareable()
		{
			_shareable = false;
		}

		bool isSharedable() const
		{
			return _shareable;
		}

		bool isShared() const
		{
			return _refCount > 1;
		}

		virtual ~_RCObject() = 0 { }

		void swap(_Myt &rhs)throw()
		{
			using std::swap;
			swap(_refCount, rhs._refCount);
			swap(_shareable, rhs._shareable);
		}
	private:
		size_t _refCount;
		bool _shareable;
	};

	//与_RCObject配套使用形成带写时复制的智能指针
	//此处智能指针引用计数没有加锁，带写时复制功能的超级简陋版的shared_ptr
	template<typename _Ty>
	class _RCPtr
	{
	public:
		typedef _RCPtr<_Ty> _Myt;

		template<class _Ux>
		_RCPtr(_Ux *ptr = 0)
			:_rawPtr(ptr)
		{
			init(ptr);
		}

		_RCPtr(const _Myt &rhs)
			:_rawPtr(rhs._rawPtr)
		{
			init(rhs._rawPtr);
		}

		~_RCPtr()
		{
			if (_rawPtr)
				_rawPtr->decRef();
		}

		_Myt &operator=(const _Myt &rhs)
		{
			if (_rawPtr != rhs._rawPtr)
			{
				if (_rawPtr)
					_rawPtr->decRef();
				_rawPtr = rhs._rawPtr;
				init(rhs._rawPtr);
			}
			return *this;
		}

		_Ty *operator->()
		{
			_makeCopy();
			return const_cast<_Ty *>(static_cast<const _Myt &>(*this).operator->());
		}

		const _Ty *operator->()const
		{
			return _rawPtr;
		}

		_Ty &operator*()
		{
			_makeCopy();
			return const_cast<_Ty &>(static_cast<const _Myt &>(*this).operator*());
		}

		const _Ty &operator*() const
		{
			return _rawPtr;
		}

		void swap(_Myt &rhs)throw()
		{
			_rawPtr.swap(rhs._rawPtr);
		}

		_Ty *get()const
		{
			return this->_rawPtr;
		}
	private:
		template<class _Ux>
		void init(_Ux *ptr)
		{
			if (_rawPtr == 0)
				return;
			//缓释评估lazyevaluation，当拷贝的指针是被标记成非共享时，要拷贝
			if (_rawPtr->isSharedable() == false)
			{
				try
				{
					_rawPtr = new _Ty(*ptr);
				}
				catch (...)
				{
					delete _rawPtr;
					throw;
				}
			}
			_rawPtr->addRef();
		}

		void _makeCopy()
		{
			if (_rawPtr->isShared())
			{
				_Ty *old = _rawPtr;
				_rawPtr->decRef();
				_rawPtr = new _Ty(*old);
				_rawPtr->addRef();
			}
		}
		_Ty *_rawPtr;
	};

	//C++二维数组的实现，内嵌辅助迭代器类，只提供下标运算符用
	//Array2D为隐式共享类
	template<typename _Elem,
		typename _Alloc = allocator<_Elem>>
		class Array2D
	{
	public:
		template<typename _Elem>
		class _Array1D
		{
			typedef _Array1D<_Elem> _Myt;
			typedef Array2D<_Elem, _Alloc> _MyVec;
			friend class _MyVec;

			typedef random_access_iterator_tag iterator_category;
			typedef typename _MyVec::value_type value_type;
			typedef typename _MyVec::size_type size_type;
			typedef typename _MyVec::difference_type difference_type;
			typedef _Elem * pointer;
			typedef const _Elem *const_pointer;
			typedef _Elem & reference;
			typedef const reference const_reference;
		public:
			_Elem &operator[](size_type index)
			{
				return const_cast<_Elem &>(static_cast<const _Array1D &>(*this)[index]);
			}

			const _Elem &operator[](size_type index) const
			{
				if (index >= _sz || index < 0)
					_DEBUG_ERROR("row out of range!");
				return this->_ptr[index];
			}
		private:
			size_t _sz;
			pointer _ptr;

			_Array1D(pointer iter, size_t sz)
				:_sz(sz), _ptr(iter)
			{

			}
			_Array1D(pointer first, pointer end)
				: _sz(end - first), _ptr(first)
			{

			}
		};

		template<typename _Elem, typename _Alloc>
		struct _ElementValue :
			public _RCObject<_ElementValue<_Elem, _Alloc> >
		{
			typedef _ElementValue<_Elem, _Alloc> _Myt;
			typedef _RCObject<_ElementValue<_Elem, _Alloc> > _Base;
			typedef _Alloc allocator_type;

			_ElementValue(size_t h, size_t w)
				:_h(h), _w(w)
			{
				_init(h, w);
			}

			template<typename... _Args>
			//本来这里应该写成universe var的，但是C++98不支持右值，就委屈一下吧
			_ElementValue(size_t h, size_t w, const _Args &... rest)
				: _h(h), _w(w)
			{
				_init(h, w);

				try
				{
					_Elem *_pdata = _memCenter.second;
					allocator_type _alloc = _memCenter.first;
					size_t len = h*w;
					for (size_t i = 0; i != len; i++)
						_alloc.construct(_pdata++, rest...);
				}
				catch (...)
				{
					_memCenter.first.deallocate(_memCenter.second, _h * _w);
					throw;
				}
			}

			template<class _Iter>
			_ElementValue(size_t h, size_t w, _Iter first, _Iter last)
				:_h(h), _w(w)
			{
				_init(h, w);
				try
				{
					uninitialized_copy(first, last, _memCenter.second);
				}
				catch (...)
				{
					_memCenter.first.deallocate(_memCenter.second, _h * _w);
					throw;
				}

			}

			_ElementValue(const _Myt &rhs)
				:_Base(rhs), _h(rhs._h), _w(rhs._w)
			{
				_init(_h, _w);
				try
				{
					_Elem *pdata = rhs._memCenter.second;
					uninitialized_copy(pdata, pdata + _h* _w, _memCenter.second);
				}
				catch (...)
				{
					_memCenter.first.deallocate(_memCenter.second, _h * _w);
					throw;
				}
			}

			~_ElementValue()OVERRIDE
			{
				//never throw
				try
				{
					allocator_type _alloc = _memCenter.first;
					_Elem *_pdata = _memCenter.second;

					_clear(is_pod<_Elem>::type());
				}
				catch (...) {}
			}

			void _clear(false_type)
			{
				_Elem *_pdata = _memCenter.second;
				allocator_type _alloc = _memCenter.first;
				_Elem *p = _pdata, *end = _pdata + _h*_w;
				for (; p != end; p++)
					_alloc.destroy(p);
				_alloc.deallocate(_pdata, _h*_w);
			}

			void _clear(true_type)
			{
				allocator_type _alloc = _memCenter.first;
				_Elem *_pdata = _memCenter.second;
				_alloc.deallocate(_pdata, _h*_w);
			}

			template<class _Iter>
			void _input(_Iter first, _Iter last)
			{
				_input1(first, last, is_pod<_Elem>::type());
			}

			_Elem *ptr()const
			{
				return _memCenter.second;
			}

			size_t size()const 
			{
				return _h*_w;
			}

			pair<_Alloc, _Elem *> _memCenter;
			size_t _h, _w;
		private:
			template<class _Iter>
			void _input1(_Iter first, _Iter last, false_type)
			{
				allocator_type _alloc = _memCenter.first;
				_Elem *_pdata = _memCenter.second;

				_Elem *p = _pdata, *end = _pdata + _h*_w;
				for (; p != end; p++)
					_alloc.destroy(p);

				uninitialized_copy(first, last, _pdata);
			}

			//内置类型省去destroy的步骤
			template<class _Iter>
			void _input1(_Iter first, _Iter last, true_type)
			{
				_Elem *_pdata = _memCenter.second;
				uninitialized_copy(first, last, _pdata);
			}

			void _init(size_t h, size_t w)
			{
				if (h == 0 || w == 0)
					_DEBUG_ERROR("dimension can not be zero!");
				try
				{
					allocator_type _alloc;
					_memCenter.first = _alloc;
					_Elem *_pdata = _alloc.allocate(_h * _w);
					_memCenter.second = _pdata;
				}
				catch (...)
				{
					_memCenter.first.deallocate(_memCenter.second, _h * _w);
					throw;
				}
			}
		};

		typedef _Array1D<_Elem> _InnerArray;
		typedef _Elem value_type;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;
		typedef _Alloc allocator_type;
		typedef Array2D<_Elem, _Alloc> _Myt;
		typedef Array2D_Iterator<_Elem> iterator;
		typedef Array2D_Const_Iterator<_Elem> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	public:
		Array2D(size_t h, size_t w)
			:_data(new _ElementValue<_Elem, _Alloc>(h, w))
		{

		}

		Array2D(const _Myt &rhs)
			:_data(rhs._data)
		{

		}

		template<class _Iter>
		Array2D(size_t h, size_t w, _Iter first, _Iter last)
			: _data(new _ElementValue<_Elem, _Alloc>(h, w, first, last))
		{

		}

		template<class... _Args>
		Array2D(size_t h, size_t w, const _Args &... rest)
			: _data(new _ElementValue<_Elem, _Alloc>(h, w, rest...))
		{

		}

		_InnerArray operator[](size_type index)
		{
			if (index >= h() || index < 0)
				_DEBUG_ERROR("row out of range!");
			_data->markUnshareable();//阻止共享，通过lazy equvation会让以后所有的拷贝都复制
			_Elem *_pdata = _data->ptr();//强制调用非常量成员函数，makeCopy
			return const_cast<_InnerArray &>(static_cast<const Array2D &>(*this)[index]);
		}

		const _InnerArray operator[](size_type index)const
		{
			if (index >= h() || index < 0)
				_DEBUG_ERROR("row out of range!");
			_Elem *_pdata = _data->ptr();//此处不会发生makeCopy
			return _InnerArray(_pdata + w()*index, w());
		}

		bool operator==(const _Myt &rhs)const throw()
		{
			if (this == &rhs)
				return true;
			if (_data->_w != rhs._data->_w
				|| _data->_h != _data->_h)
				return false;
			bool _flag = true;
			_Elem *plhs = _data->ptr(), *pend = plhs + _data->size(), *prhs = rhs._data->ptr();
			for (; plhs != pend; plhs++, prhs++)
			{
				if (!(*prhs == *plhs))
				{
					_flag = false;
					break;
				}
			}
			return _flag;
		}

		bool operator!=(const _Myt &rhs)const throw()
		{
			return !(*this == rhs);
		}

		size_t h() const { return _data->_h; }

		size_t w() const { return _data->_w; }

		iterator begin() throw()
		{
			_data->markUnshareable();
			return iterator(_data->ptr());
		}

		const_iterator begin() const throw()
		{
			return iterator(_data->ptr());
		}

		const_iterator cbegin() const throw()
		{
			return begin();
		}

		iterator end() throw()
		{
			_data->markUnshareable();
			_Elem *ptr = _data->ptr();
			return iterator(ptr + _data->size());
		}

		const_iterator end() const throw()
		{
			_Elem *ptr = _data->ptr();
			return const_iterator(ptr + _data->size());
		}

		const_iterator cend() const throw()
		{
			return end();
		}

		reverse_iterator rbegin()
		{
			return reverse_iterator(end());
		}

		const_reverse_iterator crbegin()
		{
			return const_reverse_iterator(rbegin());
		}

		reverse_iterator rend()
		{
			return reverse_iterator(begin());
		}

		const_reverse_iterator crend()
		{
			return const_reverse_iterator(rend());
		}

		void swap(const _Myt &rhs) throw()
		{
			_data.swap(rhs);
		}

		//隐式共享提供at函数
		const _Elem &at(size_t row, size_t col)const
		{
			return (*this)[row][col];
		}

	protected:
		virtual void _printPrivate(std::ostream &os,
			char elementSeparator, char dimSeparator)const = 0;

		virtual ~Array2D() = 0 { }

		_RCPtr<_ElementValue<_Elem, _Alloc> > _data;
	private:
		Array2D() { }
	};

	//偏特化bool类型
	template<typename _Alloc>
		class Array2D<bool, _Alloc >
	{
	public:
		class _Array1D
		{
			typedef _Array1D _Myt;
			typedef Array2D<bool, _Alloc> _MyVec;
			friend class _MyVec;

			typedef random_access_iterator_tag iterator_category;
			typedef bool value_type;
			typedef typename _MyVec::size_type size_type;
			typedef typename _MyVec::difference_type difference_type;
			typedef bool * pointer;
			typedef bool *const_pointer;
			typedef bool reference;
			typedef bool const_reference;
		public:
			reference operator[](size_type index)
			{
				return static_cast<const _Array1D &>(*this)[index];
			}

			const_reference operator[](size_type index) const
			{
				if (index >= _sz || index < 0)
					_DEBUG_ERROR("row out of range!");
				return this->_ptr[index];
			}
		private:
			size_t _sz;
			pointer _ptr;

			_Array1D(pointer iter, size_t sz)
				:_sz(sz), _ptr(iter)
			{

			}
			_Array1D(pointer first, pointer end)
				: _sz(end - first), _ptr(first)
			{

			}
		};

		template<typename _Alloc>
		struct _ElementValue :
			public _RCObject<_ElementValue<_Alloc>>
		{
			typedef _ElementValue<_Alloc> _Myt;
			typedef _RCObject<_ElementValue<_Alloc> > _Base;
			typedef _Alloc allocator_type;
			typedef bool * pointer;

			_ElementValue(size_t h, size_t w)
				:_h(h), _w(w)
			{
				_init(h, w);
			}

			template<class _Iter>
			_ElementValue(size_t h, size_t w, _Iter first, _Iter last)
				:_h(h), _w(w)
			{
				_init(h, w);
				try
				{
					uninitialized_copy(first, last, _memCenter.second);
				}
				catch (...)
				{
					_memCenter.first.deallocate(_memCenter.second, _h * _w);
					throw;
				}

			}

			_ElementValue(const _Myt &rhs)
				:_Base(rhs), _h(rhs._h), _w(rhs._w)
			{
				_init(_h, _w);
				try
				{
					bool *pdata = rhs._memCenter.second;
					uninitialized_copy(pdata, pdata + _h* _w, _memCenter.second);
				}
				catch (...)
				{
					_memCenter.first.deallocate(_memCenter.second, _h * _w);
					throw;
				}
			}

			~_ElementValue()OVERRIDE
			{
				//never throw
				try
				{
					allocator_type _alloc = _memCenter.first;
					bool *_pdata = _memCenter.second;
					_alloc.deallocate(_pdata, _h*_w);
				}
				catch (...) {}
			}

			template<class _Iter>
			void _input(_Iter first, _Iter last)
			{
				pointer _pdata = _memCenter.second;
				uninitialized_copy(first, last, _pdata);
			}

			pointer ptr()const
			{
				return _memCenter.second;
			}

			size_t size()const
			{
				return _h*_w;
			}

			pair<_Alloc, pointer> _memCenter;
			size_t _h, _w;
		private:
			void _init(size_t h, size_t w)
			{
				if (h == 0 || w == 0)
					_DEBUG_ERROR("dimension can not be zero!");
				try
				{
					allocator_type _alloc;
					_memCenter.first = _alloc;
					bool *_pdata = _alloc.allocate(_h * _w);
					_memCenter.second = _pdata;
				}
				catch (...)
				{
					_memCenter.first.deallocate(_memCenter.second, _h * _w);
					throw;
				}
			}
		};

		typedef _Array1D _InnerArray;
		typedef bool value_type;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;
		typedef _Alloc allocator_type;
		typedef Array2D<_Alloc> _Myt;
	public:
		Array2D(size_t h, size_t w)
			:_data(new _ElementValue<_Alloc>(h, w))
		{

		}

		Array2D(const _Myt &rhs)
			:_data(rhs._data)
		{

		}

		template<class _Iter>
		Array2D(size_t h, size_t w, _Iter first, _Iter last)
			: _data(new _ElementValue<_Alloc>(h, w, first, last))
		{

		}

		_InnerArray operator[](size_type index)
		{
			if (index >= h() || index < 0)
				_DEBUG_ERROR("row out of range!");
			_data->markUnshareable();//阻止共享，通过lazy equvation会让以后所有的拷贝都复制
			bool *pdata = _data->ptr();//强制调用非常量成员函数，makeCopy
			return const_cast<_InnerArray &>(static_cast<const Array2D &>(*this)[index]);
		}

		const _InnerArray operator[](size_type index)const
		{
			if (index >= h() || index < 0)
				_DEBUG_ERROR("row out of range!");
			bool *_pdata = _data->ptr();//此处不会发生makeCopy
			return _InnerArray(_pdata + w()*index, w());
		}

		bool operator==(const _Myt &rhs)const throw()
		{
			if (this == &rhs)
				return true;
			if (_data->_w != rhs._data->_w
				|| _data->_h != _data->_h)
				return false;
			bool _flag = true;
			_Elem *plhs = _data->ptr(), *pend = plhs + _data->size(), *prhs = rhs._data->ptr();
			for (; plhs != pend; plhs++, prhs++)
			{
				if (!(*prhs == *plhs))
				{
					_flag = false;
					break;
				}
			}
			return _flag;
		}

		bool operator!=(const _Myt &rhs)const throw()
		{
			return !(*this == rhs);
		}

		size_t h() const { return _data->_h; }

		size_t w() const { return _data->_w; }

		void swap(const _Myt &rhs) throw()
		{
			_data.swap(rhs);
		}

		//隐式共享提供at函数
		bool at(size_t row, size_t col)const
		{
			return (*this)[row][col];
		}

	protected:
		virtual void _printPrivate(std::ostream &os,
			char elementSeparator, char dimSeparator)const = 0;

		virtual ~Array2D() = 0 { }

		_RCPtr<_ElementValue<_Alloc> > _data;
	private:
		Array2D() { }
	};

	//方阵
	template<typename _Elem, typename _Alloc = allocator<_Elem>>
	class SquareMartrix :public Array2D<_Elem, _Alloc>
	{
	public:
		typedef SquareMartrix<_Elem, _Alloc> _Myt;
		typedef Array2D<_Elem, _Alloc> _Base;

		explicit SquareMartrix(size_t length = 3)
			:Array2D(length, length)
		{

		}

		//TODO: 找个办法区分_Iter和参数包的区别
		template<class _Iter>
		SquareMartrix(size_t length, _Iter first, _Iter last)
			: Array2D(length, length, first, last)
		{

		}

		SquareMartrix(const _Myt &right)
			:Array2D(right)
		{

		}

		template<class... _Args>
		explicit SquareMartrix(size_t length, const _Args &... rest)
			: Array2D(length, length, rest...)
		{

		}

		_Myt &operator=(const _Myt &right)
		{
			if (right.w() != right.h())
				_DEBUG_ERROR("the object assigned isn't the array");
			if (w() != right.w())
				_DEBUG_ERROR("the object dimension isn't same as the the object assigned");

			this->_Base::operator=(right);
			return *this;
		}

		bool operator==(const SquareMartrix &rhs)
		{
			return (*this)._Base::operator==(rhs);
		}

		bool operator!=(const SquareMartrix &rhs)
		{
			return !((*this) == rhs);
		}

		~SquareMartrix() OVERRIDE
		{

		}

		void print(std::ostream &os = std::cout,
			char elemSeparator = ' ', char dimSeparator = '\n')const
		{
			_printPrivate(os, elemSeparator, dimSeparator);
		}

		template<class _Iter>
		void input(_Iter first, _Iter last)
		{
			_data->_input(first, last);
		}

		void change()
		{
			using std::swap;
			size_t sz = w();
			for (size_t i = 0; i < sz; i++)
				for (size_t j = 0; j < i; j++)
					swap((*this)[i][j], (*this)[j][i]);
		}
	protected:
		void _printPrivate(std::ostream &os,
			char elemSeparator, char dimSeparator)const OVERRIDE
		{
			for (int i = 0; i != h(); i++)
			{
				for (int j = 0; j != w(); j++)
					os << (*this)[i][j] << elemSeparator;
				os << dimSeparator;
			}
		}
	};

	template<class _Elem, typename _Alloc = allocator<_Elem>>
	inline std::ostream &__CLR_OR_THIS_CALL operator<<(std::ostream &os, const SquareMartrix<_Elem, _Alloc> &out)
	{
		out.print(os);
		return os;
	}

	//template<class _Elem, class _Alloc = allocator<_Elem>>
	//using SquareMartrix = SquareMartrix<_Elem, _Alloc>;
}

#endif // !ARRARY
