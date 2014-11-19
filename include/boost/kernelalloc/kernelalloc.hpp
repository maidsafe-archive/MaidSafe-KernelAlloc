/* kernelalloc.hpp
Provides a kernel memory allocator and manager
(C) 2014 Niall Douglas http://www.nedprod.com/
File Created: Nov 2014


Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "config.hpp"

#ifdef BOOST_KERNELALLOC_NEED_DEFINE

/*! \file kernelalloc.hpp
 * \brief Defines the functionality provided by Boost.KernelAlloc
 */

BOOST_KERNELALLOC_V1_NAMESPACE_BEGIN

namespace detail
{
  template<class T, typename = decltype(*std::declval<T&>(), ++std::declval<T&>(), void())> struct is_rangeable { typedef T type; };
  template<class T, typename = decltype(*std::begin(std::declval<T>()), *std::end(std::declval<T>()), void())> struct is_container { typedef T type; };
}

class provider;
typedef std::shared_ptr<provider> provider_ptr;

/*! \class allocation
 * \brief An allocation of memory in the kernel
 */
class BOOST_KERNELALLOC_DECL allocation : public std::enable_shared_from_this<allocation>
{
  provider *_provider;
  allocation(provider *p) : _provider(p) { }
public:
  virtual ~allocation() {}
  
  //! \brief A pointer to a map
  typedef void *pointer;
  //! \brief A const pointer to an allocation
  typedef const pointer const_pointer;
  //! \brief A size_t
  typedef size_t size_type;
  //! \brief A sequence of offsets and sizes to map or unmap
  struct map_t
  {
    pointer addr;     //!< The address of the mapping in the local process
    size_type offset; //!< The offset to map
    size_type length; //!< The amount to map
    error_code ec;    //!< Any error which occurred during the operation
    map_t() : addr(nullptr), offset(0), length(0) { }
    map_t(size_type _offset, size_type _length) : addr(nullptr), offset(_offset), length(_length) { }
  };

  //! \brief The provider for this allocation
  provider *provider() const BOOST_NOEXCEPT { return _provider; }
  
  //! \brief The size of the allocation
  virtual size_type size() const BOOST_NOEXCEPT=0;
  
  /*! \name allocation_map
   * \brief Maps part of the allocation into the calling process
   */
  //@{
  virtual size_type map(map_t *m, size_type no) BOOST_NOEXCEPT=0;
  //! \brief Optimisation for a single map_t
  bool map(map_t &m) BOOST_NOEXCEPT { return 1==map(&m 1); }
  //! \brief For a range of dereferenceable pointers or iterators
  template<class T, typename=typename detail::is_rangeable<T>::type> size_type map(T &&begin, T &&end)
  {
    size_type ret=0;
    for(; begin!=end; ++begin)
      ret+=map(&(*begin()), 1);
    return ret;
  }
  //! \brief For a container
  template<class T, typename=typename detail::is_container<T>::type> size_type map(T &&cont)
  {
    return map(std::begin(std::forward<T>(cont)), std::end(std::forward<T>(cont)));
  }
  //! \brief Optimisation for a vector
  size_type map(const std::vector<map_t> &c)
  {
    return map(c.data(), c.size());
  }
  //! \brief Maps all of the allocation into the calling process
  map_t map() BOOST_NOEXCEPT
  {
    map_t m(0, size());
    map(&m, 1);
    return m;
  }
  //@}
  
  
  /*! \name allocation_unmap
   * \brief Unmaps part of the allocation from the calling process
   */
  //@{
  virtual size_type unmap(map_t *m, size_type no) BOOST_NOEXCEPT=0;
  //! \brief Optimisation for a single map_t
  bool unmap(map_t &m) BOOST_NOEXCEPT { return 1==unmap(&m 1); }
  //! \brief For a range of dereferenceable pointers or iterators
  template<class T, typename=typename detail::is_rangeable<T>::type> size_type unmap(T &&begin, T &&end)
  {
    size_type ret=0;
    for(; begin!=end; ++begin)
      ret+=unmap(&(*begin()), 1);
    return ret;
  }
  //! \brief For a container
  template<class T, typename=typename detail::is_container<T>::type> size_type unmap(T &&cont)
  {
    return map(std::begin(std::forward<T>(cont)), std::end(std::forward<T>(cont)));
  }
  //! \brief Optimisation for a vector
  size_type unmap(const std::vector<map_t> &c)
  {
    return unmap(c.data(), c.size());
  }
  //@}
  
  /*! \name allocation_prefault
   * \brief Prefaults preexisting maps into the calling process in a single shot instead of page by page.
   * addr must point to a valid existing map or an error will occur.
   */
  //@{
  virtual size_type prefault(map_t *m, size_type no) BOOST_NOEXCEPT=0;
  //! \brief Optimisation for a single map_t
  bool prefault(map_t &m) BOOST_NOEXCEPT { return 1==prefault(&m 1); }
  //! \brief For a range of dereferenceable pointers or iterators
  template<class T, typename=typename detail::is_rangeable<T>::type> size_type prefault(T &&begin, T &&end)
  {
    size_type ret=0;
    for(; begin!=end; ++begin)
      ret+=prefault(&(*begin()), 1);
    return ret;
  }
  //! \brief For a container
  template<class T, typename=typename detail::is_container<T>::type> size_type prefault(T &&cont)
  {
    return prefault(std::begin(std::forward<T>(cont)), std::end(std::forward<T>(cont)));
  }
  //! \brief Optimisation for a vector
  size_type prefault(const std::vector<map_t> &c)
  {
    return prefault(c.data(), c.size());
  }
  //@}
  
  /*! \name allocation_discard
   * \brief Discards without saving any dirty pages in map and decommits any RAM storage used by the map.
   * This essentially returns a map to a state of being freshly just mapped.
   */
  //@{
  virtual size_type discard(map_t *m, size_type no) BOOST_NOEXCEPT=0;
  //! \brief Optimisation for a single map_t
  bool discard(map_t &m) BOOST_NOEXCEPT { return 1==discard(&m 1); }
  //! \brief For a range of dereferenceable pointers or iterators
  template<class T, typename=typename detail::is_rangeable<T>::type> size_type discard(T &&begin, T &&end)
  {
    size_type ret=0;
    for(; begin!=end; ++begin)
      ret+=discard(&(*begin()), 1);
    return ret;
  }
  //! \brief For a container
  template<class T, typename=typename detail::is_container<T>::type> size_type discard(T &&cont)
  {
    return discard(std::begin(std::forward<T>(cont)), std::end(std::forward<T>(cont)));
  }
  //! \brief Optimisation for a vector
  size_type discard(const std::vector<map_t> &c)
  {
    return discard(c.data(), c.size());
  }
  //@}
};

/*! \class provider
 * \brief A provider of kernel memory
 */
class BOOST_KERNELALLOC_DECL provider : public std::enable_shared_from_this<provider>
{
public:
  //! \brief A pointer to an allocation
  typedef std::shared_ptr<allocation> pointer;
  //! \brief A const pointer to an allocation
  typedef const pointer const_pointer;
  //! \brief A size_t
  typedef size_t size_type;
  
  //! \brief The name of this provider, suitable for printing etc.
  virtual const char *name() BOOST_NOEXCEPT=0;
  
  /*! \brief Allocates at least \em bytes from the provider, returning an empty pointer if unsuccessful
   */
  virtual pointer allocate(error_code &ec, size_type bytes) BOOST_NOEXCEPT=0;
  
  /*! \brief Returns the allocation associated with mapped address \em addr
   */
  virtual pointer allocation(void *addr, allocation::map_t *map=nullptr) BOOST_NOEXCEPT=0;
};

/*! \class allocator
 * \brief A STL compatible allocator allocating memory from a provider
 */
template<class T> class allocator
{
  template<class A, class B> friend inline bool operator==(const allocator<A> &a, const allocator<B> &b) BOOST_NOEXCEPT;
  template<class A, class B> friend inline bool operator!=(const allocator<A> &a, const allocator<B> &b) BOOST_NOEXCEPT;
  
  std::shared_ptr<provider> _provider;
public:
  typedef T value_type;
  typedef T *pointer;
  typedef const pointer const_pointer;
  typedef T &reference;
  typedef const reference const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef std::true_type propagate_on_container_move_assignment;
  typedef std::false_type is_always_equal;
  template<class U> struct rebind { typedef allocator<U> other; };
  
  allocator() BOOST_NOEXCEPT { }
  allocator(const allocator &o) BOOST_NOEXCEPT : _provider(o._provider) { }
  allocator(allocator &&o) BOOST_NOEXCEPT : _provider(std::move(o._provider)) { }
  template<class U> allocator(const allocator<U> &o) BOOST_NOEXCEPT : _provider(o._provider) { }
  template<class U> allocator(allocator<U> &&o) BOOST_NOEXCEPT : _provider(std::move(o._provider)) { }
  pointer address(reference x) const BOOST_NOEXCEPT { return std::addressof(x); }
  const_pointer address(const_reference x) const BOOST_NOEXCEPT { return std::addressof(x); }
  pointer allocate(size_type n, std::allocator<void>::const_pointer hint=0)
  {
    if(!_provider) throw std::invalid_argument("Unset provider");
    if(n>max_size()) throw std::bad_alloc();
    error_code ec;
    size_type bytes=n*sizeof(T);
    auto a(_provider->allocate(ec, bytes));
    if(ec || !a) throw std::system_error(ec);
    auto m(a->map());
    if(!m->addr) throw std::bad_alloc();
    return static_cast<pointer>(m->addr);
  }
  void deallocate(pointer p, size_type n)
  {
    if(!_provider) throw std::invalid_argument("Unset provider");
    allocation::map_t m;
    auto a(_provider->allocation(p, &m));
    if(!a) throw std::invalid_argument("Address not found");
    a->unmap(m);
    if(m.ec) throw std::system_error(m.ec, "Failed to unmap allocation");
  }
  size_type max_size() const BOOST_NOEXCEPT { return ((size_type)-1)/sizeof(T); }
  template<class U, class... Args> void construct(U *p, Args &&... args) { ::new(p) U(std::forward<Args>(args)...); }
  template<class U> void destroy(U *p) { p->~U(); }
};
template<class A, class B> inline bool operator==(const allocator<A> &a, const allocator<B> &b) BOOST_NOEXCEPT { return a._provider==b._provider; }
template<class A, class B> inline bool operator!=(const allocator<A> &a, const allocator<B> &b) BOOST_NOEXCEPT { return a._provider!=b._provider; }

BOOST_KERNELALLOC_V1_NAMESPACE_END

#endif
