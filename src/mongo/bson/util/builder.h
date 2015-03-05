/* builder.h */

/*    Copyright 2009 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <cfloat>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <string.h>

#include "mongo/bson/inline_decls.h"
#include "mongo/base/string_data.h"
#include "mongo/util/assert_util.h"

//====================================================================
#include "boost/detail/endian.hpp"
#include "boost/static_assert.hpp"
//====================================================================


namespace mongo {

	//=========================================================================================
	class Nullstream;
		
		   // Generic (portable) byte swap function
		   template<class T> T byteSwap( T j ) {
			   
#ifdef HAVE_BSWAP32
			   if ( sizeof( T ) == 4 ) {
				   return __builtin_bswap32( j );
			   }
#endif
#ifdef HAVE_BSWAP64
			   if ( sizeof( T ) == 8 ) {
				   return __builtin_bswap64( j );
			   }
#endif
		
			  T retVal = 0;
			  for ( unsigned i = 0; i < sizeof( T ); ++i ) {
				 
				 // 7 5 3 1 -1 -3 -5 -7
				 int shiftamount = sizeof(T) - 2 * i - 1;
				 // 56 40 24 8 -8 -24 -40 -56
				 shiftamount *= 8;
		
				 // See to it that the masks can be re-used
				 if ( shiftamount > 0 ) {
					T mask = T( 0xff ) << ( 8 * i );
					retVal |= ( (j & mask ) << shiftamount );
				 } else {
					T mask = T( 0xff ) << ( 8 * (sizeof(T) - i - 1) );
					retVal |= ( j >> -shiftamount ) & mask;
				 }
			  }
			  return retVal;
		   }
		
		   template<> inline double byteSwap( double j ) {
			  union {
				 double d;
				 unsigned long long l;
			  } u;
			  u.d = j;
			  u.l = byteSwap<unsigned long long>( u.l );
			  return u.d;
		   }
		
		
		   // Here we assume that double is big endian if ints are big endian
		   // and also that the format is the same as for x86 when swapped.
		   template<class T> inline T littleEndian( T j ) {
#ifdef BOOST_LITTLE_ENDIAN
			  return j;
#else
			  return byteSwap<T>(j);
#endif
		   }
		
		   template<class T> inline T bigEndian( T j ) {
#ifdef BOOST_BIG_ENDIAN
			  return j;
#else
			  return byteSwap<T>(j);
#endif
		   }
		
#if defined(__arm__) 
#  if defined(__MAVERICK__)
#    define MONGO_ARM_SPECIAL_ENDIAN
		   // Floating point is always little endian
		   template<> inline double littleEndian( double j ) {
			   return j;
		   }
#  elif defined(__VFP_FP__) || defined( BOOST_BIG_ENDIAN )
		   // Native endian floating points even if FPA is used
#  else
#    define MONGO_ARM_SPECIAL_ENDIAN
		   // FPA mixed endian floating point format 456701234
		   template<> inline double littleEndian( double j ) {
			  union { double d; unsigned u[2]; } u;
			  u.d = j;
			  std::swap( u.u[0], u.u[1] );
			  return u.d;
		   }
#  endif
#  if defined(MONGO_ARM_SPECIAL_ENDIAN)
		   template<> inline double bigEndian( double j ) {
			  return byteSwap<double>( littleEndian<double>( j ) );
		   }
#  endif
#endif
		
		
		   BOOST_STATIC_ASSERT( sizeof( double ) == sizeof( unsigned long long ) );
		
		   template<class S, class D> inline D convert( S src )
		   {
			  union { S s; D d; } u;
			  u.s = src;
			  return u.d;
		   }
		
		   template<> inline char convert<bool,char>( bool src ) {
			  return src;
		   }
		
		   template<> inline bool convert<char, bool>( char src ) {
			  return src;
		   }
		
		
#define MONGO_ENDIAN_BODY( MYTYPE, BASE_TYPE, T )                       \
			  MYTYPE& operator=( const T& val ) {								\
				  BASE_TYPE::_val = val;										\
				  return *this; 												\
			  } 																\
																				\
			  operator const T() const {										\
				  return BASE_TYPE::_val;										\
			  } 																\
																				\
			  MYTYPE& operator+=( T other ) {									\
				  (*this) = T(*this) + other;									\
				  return *this; 												\
			  } 																\
																				\
			  MYTYPE& operator-=( T other ) {									\
				  (*this) = T(*this) - other;									\
				  return *this; 												\
			  } 																\
																				\
			  MYTYPE& operator&=( T other ) {									\
				  (*this) = T(*this) & other;									\
				  return *this; 												\
			  } 																\
																				\
			  MYTYPE& operator|=( T other ) {									\
				  (*this) = T(*this) | other;									\
				  return *this; 												\
			  } 																\
																				\
			  MYTYPE& operator^=( T other ) {									\
				  (*this) = T(*this) ^ other;									\
				  return *this; 												\
			  } 																\
																				\
			  MYTYPE& operator++() {											\
				  return (*this) += 1;											\
			  } 																\
																				\
			  MYTYPE operator++(int) {											\
				  MYTYPE old = *this;											\
				  ++(*this);													\
				  return old;													\
			  } 																\
																				\
			  MYTYPE& operator--() {											\
				  return (*this) -= 1;											\
			  } 																\
																				\
			  MYTYPE operator--(int) {											\
				  MYTYPE old = *this;											\
				  --(*this);													\
				  return old;													\
	}
			
														
		  template<class T, class D> void storeLE( D* dest, T src ) {
#if defined(BOOST_LITTLE_ENDIAN) || !defined( ALIGNMENT_IMPORTANT )
			  // This also assumes no alignment issues
			  *dest = littleEndian<T>( src );
#else
			  unsigned char* u_dest = reinterpret_cast<unsigned char*>( dest );
			  for ( unsigned i = 0; i < sizeof( T ); ++i ) {
				 u_dest[i] = src >> ( 8 * i );
			  }
#endif
		   }
		   
		  template<class T, class S> T loadLE( const S* data ) {
#ifdef __APPLE__
			  switch ( sizeof( T ) ) {
			  case 8:
				  return OSReadLittleInt64( data, 0 );
			  case 4:
				  return OSReadLittleInt32( data, 0 );
			  case 2:
				  return OSReadLittleInt16( data, 0 );
			  }
#endif
		
#if defined(BOOST_LITTLE_ENDIAN) || !defined( ALIGNMENT_IMPORTANT )
#if defined(__powerpc__)
			  // Without this trick gcc (4.4.5) compiles 64 bit load to 8 byte loads.
			  if ( sizeof( T ) == 8 ) {
				  const unsigned * x = reinterpret_cast<const unsigned*>( data );
				  unsigned long long a = loadLE<unsigned, unsigned>( x );
				  unsigned long long b = loadLE<unsigned, unsigned>( x + 1 ); 
				  return a | ( b << 32 );
			  }
#endif
			  return littleEndian<T>( *data );
#else
			  T retval = 0;
			  const unsigned char* u_data = reinterpret_cast<const unsigned char*>( data );
			  for( unsigned i = 0; i < sizeof( T ); ++i ) {
				  retval |= T( u_data[i] ) << ( 8 * i );
			  }
			  return retval;
#endif
		  }
		
		  template<class T, class D> void store_big( D* dest, T src ) {
#if defined(BOOST_BIG_ENDIAN) || !defined( ALIGNMENT_IMPORTANT )
			  // This also assumes no alignment issues
			  *dest = bigEndian<T>( src );
#else
			  unsigned char* u_dest = reinterpret_cast<unsigned char*>( dest );
			  for ( unsigned i = 0; i < sizeof( T ); ++i ) {
				  u_dest[ sizeof(T) - 1 - i ] = src >> ( 8 * i );
			  }
#endif
		   }
		   
		  template<class T, class S> T load_big( const S* data ) {
		
			  if ( sizeof( T ) == 8 && sizeof( void* ) == 4 ) {
				  const unsigned * x = reinterpret_cast<const unsigned*>( data );
				  unsigned long long a = load_big<unsigned, unsigned>( x );
				  unsigned long long b = load_big<unsigned, unsigned>( x + 1 );
				  return a << 32 | b;
			  }
		
#if defined(BOOST_BIG_ENDIAN) || !defined( ALIGNMENT_IMPORTANT )
			  return bigEndian<T>( *data );
#else
			  T retval = 0;
			  const unsigned char* u_data = reinterpret_cast<const unsigned char*>( data );
			  for( unsigned i = 0; i < sizeof( T ); ++i ) {
				  retval |= T( u_data[ sizeof(T) - 1 - i ] ) << ( 8 * i );
			  }
			  return retval;
#endif
		  }
		
		
		  /** Converts the type to the type to actually store */
		  template<typename T> class storage_type {
		  public:
			  typedef T t;
			  
			  static inline t toStorage( T src ) { return src; }
			  static inline T fromStorage( t src ) { return src; }
		
		  };
		
		  template<> class storage_type<bool> {
		  public:
			  typedef unsigned char t;
		
			  static inline t toStorage( bool src ) { return src; }
			  static inline bool fromStorage( t src ) { return src; }	   
			  
		  };
		
		  template<> class storage_type<double> {
		  public:
			  typedef unsigned long long t;
		
			  static inline t toStorage( double src ) { return convert<double,t>( src ); }
			  static inline double fromStorage( t src ) { 
				  return convert<t,double>( src ); 
			  }
		  };
		
	#pragma pack(1)
		
//#ifdef __GNUC__
//  #define ATTRIB_PACKED __attribute__((packed))
//#else
//  #define ATTRIB_PACKED
//#endif
		
	#pragma pack(1)
		  template<class T> struct packed_little_storage {
		  protected:
			  typedef storage_type<T> STORAGE;
			  typedef typename STORAGE::t S;
			  S _val;
		
			  void store( S val ) {
#ifdef __APPLE__
				  switch ( sizeof( S ) ) {
				  case 8:
					  return OSWriteLittleInt64( &_val, 0, val );
				  case 4:
					  return OSWriteLittleInt32( &_val, 0, val );
				  case 2:
					  return OSWriteLittleInt16( &_val, 0, val );
			  }
#endif
		
#if defined(BOOST_LITTLE_ENDIAN) || !defined( ALIGNMENT_IMPORTANT )
				  _val = littleEndian<S>( val );
#else
				  unsigned char* u_dest = reinterpret_cast<unsigned char*>( &_val );
				  for ( unsigned i = 0; i < sizeof( T ); ++i ) {
					  u_dest[i] = val >> ( 8 * i );
				  }
#endif
			  }
		
			  S load() const {
				  // Here S should always be an integer type
#ifdef __APPLE__
				  switch ( sizeof( S ) ) {
				  case 8:
					  return OSReadLittleInt64( &_val, 0 );
				  case 4:
					  return OSReadLittleInt32( &_val, 0 );
				  case 2:
					  return OSReadLittleInt16( &_val, 0 );
				  }
#endif
				  // Without this trick gcc (4.4.5) compiles 64 bit load to 8 byte loads.
				  // (ppc)
				  if ( sizeof( S ) == 8 && sizeof( void* ) == 4 ) {
					  const packed_little_storage<unsigned>* x = 
						  reinterpret_cast<const packed_little_storage<unsigned>* >(this);
					  
					  unsigned long long a = x[0];
					  unsigned long long b = x[1];
					  return a | ( b << 32 ); 
				  }
					  
		
#if defined(BOOST_LITTLE_ENDIAN) || !defined( ALIGNMENT_IMPORTANT )
				  return littleEndian<S>( _val );
#else
				  S retval = 0;
				  const unsigned char* u_data = 
					  reinterpret_cast<const unsigned char*>( &_val );
				  for( unsigned i = 0; i < sizeof( T ); ++i ) {
					  retval |= S( u_data[i] ) << ( 8 * i );
				  }
				  return retval;
#endif
			  }
		  public:	   
			  inline packed_little_storage& operator=( T val ) {
				  store( STORAGE::toStorage( val ) );
				  return *this;
			  }
				  
			  inline operator T() const {
				  return STORAGE::fromStorage( load() );
			  }
			  
			  
		  }/* ATTRIB_PACKED */;
		
#ifdef MONGO_ARM_SPECIAL_ENDIAN
		  template<> struct packed_little_storage<double> {
		  private:
			  double _val;
		  public:
			  inline packed_little_storage<double>& operator=( double val ) {
				  _val = littleEndian<double>( val );
				  return *this;
			  }
			  
			  inline operator double() const {
				  return littleEndian<double>( _val );
			  }
		  } /*ATTRIB_PACKED*/ ;
#endif
		
		  template<class T> struct packed_big_storage {
		  private:
			  typedef typename storage_type<T>::t S;
			  S _val;
		  public:
			  
			 packed_big_storage& operator=( T val ) {
				store_big<S>( &_val, convert<T,S>( val ) );
				return *this;
			 }
			 
			 operator T() const {
				return convert<S,T>( load_big<S>( &_val ) );
			 }
		  } /*ATTRIB_PACKED*/ ;
		
#ifdef MONGO_ARM_SPECIAL_ENDIAN
		  template<> struct packed_big_storage<double> {
		  private:
			  double _val;
		  public:
			  inline packed_big_storage<double>& operator=( double val ) {
				  _val = bigEndian<double>( val );
				  return *this;
			  }
			  
			  inline operator double() const {
				  return bigEndian<double>( _val );
			  }
		  } /*ATTRIB_PACKED*/;
#endif
		
		
	#pragma pack()
		  
		
#define MONGO_ENDIAN_REF_FUNCS( TYPE )                          \
			  static TYPE& ref( char* src ) {							\
				  return *reinterpret_cast<TYPE*>( src );				\
			  } 														\
																		\
			  static const TYPE& ref( const char* src ) {				\
				  return *reinterpret_cast<const TYPE*>( src ); 		\
			  } 														\
																		\
			  static TYPE& ref( void* src ) {							\
				  return ref( reinterpret_cast<char*>( src ) ); 		\
			  } 														\
																		\
			  static const TYPE& ref( const void* src ) {				\
				  return ref( reinterpret_cast<const char*>( src ) );	\
			  }
		
		  template<class T> class little_pod {
		  protected:
			  packed_little_storage<T> _val;
		  public:
			  MONGO_ENDIAN_REF_FUNCS( little_pod );
			  MONGO_ENDIAN_BODY( little_pod, little_pod<T>, T );
		  } /*ATTRIB_PACKED*/;
		
		  template<class T> class little : public little_pod<T> {
		  public:
			  inline little( T x ) {
				  *this = x;
			  }
		
			  inline little() {}
			  MONGO_ENDIAN_REF_FUNCS( little );
			  MONGO_ENDIAN_BODY( little, little_pod<T>, T );
		  } /*ATTRIB_PACKED*/;
		
		  template<class T> class big_pod {
		  protected:
			  packed_big_storage<T> _val;
		  public:
			  MONGO_ENDIAN_REF_FUNCS( big_pod );
			  MONGO_ENDIAN_BODY( big_pod, big_pod<T>, T );
		  } /*ATTRIB_PACKED*/;
		
		  template<class T> class big : public big_pod<T> {
		  public:
			  inline big( T x ) {
				  *this = x;
			  }
		
			  inline big() {}
			  MONGO_ENDIAN_REF_FUNCS( big );
			  MONGO_ENDIAN_BODY( big, big_pod<T>, T );
		  } /*ATTRIB_PACKED*/;
		
		  // Helper functions
		  template<class T> T readLE( const void* data ) {
			  return little<T>::ref( data );
		  }
		
		  template<class T> T readBE( const void* data ) {
			  return big<T>::ref( data );
		  }
		
		  template<class T> void copyLE( void* dest, T src ) {
			  little<T>::ref( dest ) = src;
		  }
		
		  template<class T> void copyBE( void* dest, T src ) {
			  big<T>::ref( dest ) = src;
		  }
		
		
		  BOOST_STATIC_ASSERT( sizeof( little_pod<double> ) == 8 );
		  BOOST_STATIC_ASSERT( sizeof( little<double> ) == 8 );
		  BOOST_STATIC_ASSERT( sizeof( big<bool> ) == 1 );
		  BOOST_STATIC_ASSERT( sizeof( little<bool> ) == 1 );
		 
		  /** Marker class to inherit from to mark that endianess has been taken care of */
		  struct endian_aware { typedef int endian_aware_t; };
		
		  /** To assert that a class has the endian aware marker */
  #define STATIC_ASSERT_HAS_ENDIAN_AWARE_MARKER( T ) BOOST_STATIC_ASSERT( sizeof( typename T::endian_aware_t ) > 0 )
		
	
	//=========================================================================================


    /* Accessing unaligned doubles on ARM generates an alignment trap and aborts with SIGBUS on Linux.
       Wrapping the double in a packed struct forces gcc to generate code that works with unaligned values too.
       The generated code for other architectures (which already allow unaligned accesses) is the same as if
       there was a direct pointer access.
    */
    struct PackedDouble {
        double d;
    } PACKED_DECL;


    /* Note the limit here is rather arbitrary and is simply a standard. generally the code works
       with any object that fits in ram.

       Also note that the server has some basic checks to enforce this limit but those checks are not exhaustive
       for example need to check for size too big after
         update $push (append) operation
         various db.eval() type operations
    */
    const int BSONObjMaxUserSize = 16 * 1024 * 1024;

    /*
       Sometimes we need objects slightly larger - an object in the replication local.oplog
       is slightly larger than a user object for example.
    */
    const int BSONObjMaxInternalSize = BSONObjMaxUserSize + ( 16 * 1024 );

    const int BufferMaxSize = 64 * 1024 * 1024;

    template <typename Allocator>
    class StringBuilderImpl;

    class TrivialAllocator { 
    public:
        void* Malloc(size_t sz) { return malloc(sz); }
        void* Realloc(void *p, size_t sz) { return realloc(p, sz); }
        void Free(void *p) { free(p); }
    };

    class StackAllocator {
    public:
        enum { SZ = 512 };
        void* Malloc(size_t sz) {
            if( sz <= SZ ) return buf;
            return malloc(sz); 
        }
        void* Realloc(void *p, size_t sz) { 
            if( p == buf ) {
                if( sz <= SZ ) return buf;
                void *d = malloc(sz);
                if ( d == 0 )
                    msgasserted( 15912 , "out of memory StackAllocator::Realloc" );
                memcpy(d, p, SZ);
                return d;
            }
            return realloc(p, sz); 
        }
        void Free(void *p) { 
            if( p != buf )
                free(p); 
        }
    private:
        char buf[SZ];
    };

    template< class Allocator >
    class _BufBuilder {
        // non-copyable, non-assignable
        _BufBuilder( const _BufBuilder& );
        _BufBuilder& operator=( const _BufBuilder& );
        Allocator al;
    public:
        _BufBuilder(int initsize = 512) : size(initsize) {
            if ( size > 0 ) {
                data = (char *) al.Malloc(size);
                if( data == 0 )
                    msgasserted(10000, "out of memory BufBuilder");
            }
            else {
                data = 0;
            }
            l = 0;
        }
        ~_BufBuilder() { kill(); }

        void kill() {
            if ( data ) {
                al.Free(data);
                data = 0;
            }
        }

        void reset() {
            l = 0;
        }
        void reset( int maxSize ) {
            l = 0;
            if ( maxSize && size > maxSize ) {
                al.Free(data);
                data = (char*)al.Malloc(maxSize);
                if ( data == 0 )
                    msgasserted( 15913 , "out of memory BufBuilder::reset" );
                size = maxSize;
            }
        }

        /** leave room for some stuff later
            @return point to region that was skipped.  pointer may change later (on realloc), so for immediate use only
        */
        char* skip(int n) { return grow(n); }

        /* note this may be deallocated (realloced) if you keep writing. */
        char* buf() { return data; }
        const char* buf() const { return data; }

        /* assume ownership of the buffer - you must then free() it */
        void decouple() { data = 0; }

    private:
        /* 
         *  Wrap all primitive types in an endian/alignment insensitive way.
         */
        template<class T> void append( T j ) {
            little<T>::ref( grow( sizeof( T ) ) ) = j;
        }
    public:
        void appendUChar(unsigned char j) {
            append<unsigned char>( j );
        }
        void appendChar(char j) {
            append<char>( j );
        }
        void appendNum(char j) {
            append<char>( j );
        }
        void appendNum(short j) {
            append<short>( j );
        }
        void appendNum(int j) {
            append<int>( j );
        }
        void appendNum(unsigned j) {
            append<unsigned>( j );
        }
        void appendNum(bool j) {
            append<char>( j );
        }
        void appendNum(double j) {
            append<double>( j );
        }
        void appendNum(long long j) {
            append<long long>( j );
        }
        void appendNum(unsigned long long j) {
            append<unsigned long long>( j );
        }

        void appendBuf(const void *src, size_t len) {
            memcpy(grow((int) len), src, len);
        }

        template<class T>
        void appendStruct(const T& s) {
            appendBuf(&s, sizeof(T));
        }

        void appendStr(const StringData &str , bool includeEndingNull = true ) {
            const int len = str.size() + ( includeEndingNull ? 1 : 0 );
            str.copyTo( grow(len), includeEndingNull );
        }

        /** @return length of current string */
        int len() const { return l; }
        void setlen( int newLen ) { l = newLen; }
        /** @return size of the buffer */
        int getSize() const { return size; }

        /* returns the pre-grow write position */
        inline char* grow(int by) {
            int oldlen = l;
            int newLen = l + by;
            if ( newLen > size ) {
                grow_reallocate(newLen);
            }
            l = newLen;
            return data + oldlen;
        }

    private:
        /* "slow" portion of 'grow()'  */
        void NOINLINE_DECL grow_reallocate(int newLen) {
            int a = 64;
            while( a < newLen ) 
                a = a * 2;
            if ( a > BufferMaxSize ) {
                std::stringstream ss;
                ss << "BufBuilder attempted to grow() to " << a << " bytes, past the 64MB limit.";
                msgasserted(13548, ss.str().c_str());
            }
            data = (char *) al.Realloc(data, a);
            if ( data == NULL )
                msgasserted( 16070 , "out of memory BufBuilder::grow_reallocate" );
            size = a;
        }

        char *data;
        int l;
        int size;

        friend class StringBuilderImpl<Allocator>;
    };

    typedef _BufBuilder<TrivialAllocator> BufBuilder;

    /** The StackBufBuilder builds smaller datasets on the stack instead of using malloc.
          this can be significantly faster for small bufs.  However, you can not decouple() the 
          buffer with StackBufBuilder.
        While designed to be a variable on the stack, if you were to dynamically allocate one, 
          nothing bad would happen.  In fact in some circumstances this might make sense, say, 
          embedded in some other object.
    */
    class StackBufBuilder : public _BufBuilder<StackAllocator> { 
    public:
        StackBufBuilder() : _BufBuilder<StackAllocator>(StackAllocator::SZ) { }
        void decouple(); // not allowed. not implemented.
    };

#if defined(_WIN32)
#pragma push_macro("snprintf")
#define snprintf _snprintf
#endif

    /** stringstream deals with locale so this is a lot faster than std::stringstream for UTF8 */
    template <typename Allocator>
    class StringBuilderImpl {
    public:
        static const size_t MONGO_DBL_SIZE = 3 + DBL_MANT_DIG - DBL_MIN_EXP;
        static const size_t MONGO_S32_SIZE = 12;
        static const size_t MONGO_U32_SIZE = 11;
        static const size_t MONGO_S64_SIZE = 23;
        static const size_t MONGO_U64_SIZE = 22;
        static const size_t MONGO_S16_SIZE = 7;

        StringBuilderImpl() { }

        StringBuilderImpl& operator<<( double x ) {
            return SBNUM( x , MONGO_DBL_SIZE , "%g" );
        }
        StringBuilderImpl& operator<<( int x ) {
            return SBNUM( x , MONGO_S32_SIZE , "%d" );
        }
        StringBuilderImpl& operator<<( unsigned x ) {
            return SBNUM( x , MONGO_U32_SIZE , "%u" );
        }
        StringBuilderImpl& operator<<( long x ) {
            return SBNUM( x , MONGO_S64_SIZE , "%ld" );
        }
        StringBuilderImpl& operator<<( unsigned long x ) {
            return SBNUM( x , MONGO_U64_SIZE , "%lu" );
        }
        StringBuilderImpl& operator<<( long long x ) {
            return SBNUM( x , MONGO_S64_SIZE , "%lld" );
        }
        StringBuilderImpl& operator<<( unsigned long long x ) {
            return SBNUM( x , MONGO_U64_SIZE , "%llu" );
        }
        StringBuilderImpl& operator<<( short x ) {
            return SBNUM( x , MONGO_S16_SIZE , "%hd" );
        }
        StringBuilderImpl& operator<<( char c ) {
            _buf.grow( 1 )[0] = c;
            return *this;
        }

        void appendDoubleNice( double x ) {
            const int prev = _buf.l;
            const int maxSize = 32; 
            char * start = _buf.grow( maxSize );
            int z = snprintf( start , maxSize , "%.16g" , x );
            verify( z >= 0 );
            verify( z < maxSize );
            _buf.l = prev + z;
            if( strchr(start, '.') == 0 && strchr(start, 'E') == 0 && strchr(start, 'N') == 0 ) {
                write( ".0" , 2 );
            }
        }

        void write( const char* buf, int len) { memcpy( _buf.grow( len ) , buf , len ); }

        void append( const StringData& str ) { str.copyTo( _buf.grow( str.size() ), false ); }

        StringBuilderImpl& operator<<( const StringData& str ) {
            append( str );
            return *this;
        }

        void reset( int maxSize = 0 ) { _buf.reset( maxSize ); }

        std::string str() const { return std::string(_buf.data, _buf.l); }

        /** size of current string */
        int len() const { return _buf.l; }

    private:
        _BufBuilder<Allocator> _buf;

        // non-copyable, non-assignable
        StringBuilderImpl( const StringBuilderImpl& );
        StringBuilderImpl& operator=( const StringBuilderImpl& );

        template <typename T>
        StringBuilderImpl& SBNUM(T val,int maxSize,const char *macro)  {
            int prev = _buf.l;
            int z = snprintf( _buf.grow(maxSize) , maxSize , macro , (val) );
            verify( z >= 0 );
            verify( z < maxSize );
            _buf.l = prev + z;
            return *this;
        }
    };

    typedef StringBuilderImpl<TrivialAllocator> StringBuilder;
    typedef StringBuilderImpl<StackAllocator> StackStringBuilder;

#if defined(_WIN32)
#undef snprintf
#pragma pop_macro("snprintf")
#endif
} // namespace mongo
