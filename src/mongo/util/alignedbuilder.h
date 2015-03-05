// @file alignedbuilder.h

/**
*    Copyright (C) 2009 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*    As a special exception, the copyright holders give permission to link the
*    code of portions of this program with the OpenSSL library under certain
*    conditions as described in each individual source file and distribute
*    linked combinations including the program with the OpenSSL library. You
*    must comply with the GNU Affero General Public License in all respects
*    for all of the code used other than as permitted herein. If you modify
*    file(s) with this exception, you may extend this exception to your
*    version of the file(s), but you are not obligated to do so. If you do not
*    wish to do so, delete this exception statement from your version. If you
*    delete this exception statement from all source files in the program,
*    then also delete it in the license file.
*/

#pragma once

#include "mongo/base/string_data.h"

//==========================================================================
#if 0
#include "boost/detail/endian.hpp"
#include "boost/static_assert.hpp"
#include <string.h>
#endif
//===========================================================================


namespace mongo {

//===========================================================================
#if 0
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
      MYTYPE& operator=( const T& val ) {                               \
          BASE_TYPE::_val = val;                                        \
          return *this;                                                 \
      }                                                                 \
                                                                        \
      operator const T() const {                                        \
          return BASE_TYPE::_val;                                       \
      }                                                                 \
                                                                        \
      MYTYPE& operator+=( T other ) {                                   \
          (*this) = T(*this) + other;                                   \
          return *this;                                                 \
      }                                                                 \
                                                                        \
      MYTYPE& operator-=( T other ) {                                   \
          (*this) = T(*this) - other;                                   \
          return *this;                                                 \
      }                                                                 \
                                                                        \
      MYTYPE& operator&=( T other ) {                                   \
          (*this) = T(*this) & other;                                   \
          return *this;                                                 \
      }                                                                 \
                                                                        \
      MYTYPE& operator|=( T other ) {                                   \
          (*this) = T(*this) | other;                                   \
          return *this;                                                 \
      }                                                                 \
                                                                        \
      MYTYPE& operator^=( T other ) {                                   \
          (*this) = T(*this) ^ other;                                   \
          return *this;                                                 \
      }                                                                 \
                                                                        \
      MYTYPE& operator++() {                                            \
          return (*this) += 1;                                          \
      }                                                                 \
                                                                        \
      MYTYPE operator++(int) {                                          \
          MYTYPE old = *this;                                           \
          ++(*this);                                                    \
          return old;                                                   \
      }                                                                 \
                                                                        \
      MYTYPE& operator--() {                                            \
          return (*this) -= 1;                                          \
      }                                                                 \
                                                                        \
      MYTYPE operator--(int) {                                          \
          MYTYPE old = *this;                                           \
          --(*this);                                                    \
          return old;                                                   \
      }                                                                 \
                                                                        \
      friend std::ostream& operator<<( std::ostream& ost, MYTYPE val ) { \
          return ost << T(val);                                         \
      }                                                                 \
                                                                        \
      friend Nullstream& operator<<( Nullstream& ost, MYTYPE val ) {    \
          return ost << T(val);                                         \
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

#ifdef __GNUC__
  #define ATTRIB_PACKED __attribute__((packed))
#else
  #define ATTRIB_PACKED
#endif

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
      
      
  } ATTRIB_PACKED ;

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
  } ATTRIB_PACKED ;
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
  } ATTRIB_PACKED ;

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
  } ATTRIB_PACKED;
#endif


#pragma pack()
  

#define MONGO_ENDIAN_REF_FUNCS( TYPE )                          \
      static TYPE& ref( char* src ) {                           \
          return *reinterpret_cast<TYPE*>( src );               \
      }                                                         \
                                                                \
      static const TYPE& ref( const char* src ) {               \
          return *reinterpret_cast<const TYPE*>( src );         \
      }                                                         \
                                                                \
      static TYPE& ref( void* src ) {                           \
          return ref( reinterpret_cast<char*>( src ) );         \
      }                                                         \
                                                                \
      static const TYPE& ref( const void* src ) {               \
          return ref( reinterpret_cast<const char*>( src ) );   \
      }

  template<class T> class little_pod {
  protected:
      packed_little_storage<T> _val;
  public:
      MONGO_ENDIAN_REF_FUNCS( little_pod );
      MONGO_ENDIAN_BODY( little_pod, little_pod<T>, T );
  } ATTRIB_PACKED;

  template<class T> class little : public little_pod<T> {
  public:
      inline little( T x ) {
          *this = x;
      }

      inline little() {}
      MONGO_ENDIAN_REF_FUNCS( little );
      MONGO_ENDIAN_BODY( little, little_pod<T>, T );
  } ATTRIB_PACKED;

  template<class T> class big_pod {
  protected:
      packed_big_storage<T> _val;
  public:
      MONGO_ENDIAN_REF_FUNCS( big_pod );
      MONGO_ENDIAN_BODY( big_pod, big_pod<T>, T );
  } ATTRIB_PACKED;

  template<class T> class big : public big_pod<T> {
  public:
      inline big( T x ) {
          *this = x;
      }

      inline big() {}
      MONGO_ENDIAN_REF_FUNCS( big );
      MONGO_ENDIAN_BODY( big, big_pod<T>, T );
  } ATTRIB_PACKED;

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

#endif
//===========================================================================

    /** a page-aligned BufBuilder. */
    class AlignedBuilder {
    public:
        AlignedBuilder(unsigned init_size);
        ~AlignedBuilder() { kill(); }

        /** reset with a hint as to the upcoming needed size specified */
        void reset(unsigned sz);

        /** reset for a re-use. shrinks if > 128MB */
        void reset();

        /** note this may be deallocated (realloced) if you keep writing or reset(). */
        const char* buf() const { return _p._data; }

        /** leave room for some stuff later
            @return offset in the buffer that was our current position
        */
        size_t skip(unsigned n) {
            unsigned l = len();
            grow(n);
            return l;
        }

        /** if buffer grows pointer no longer valid */
        char* atOfs(unsigned ofs) { return _p._data + ofs; }

        /** if buffer grows pointer no longer valid */
        char* cur() { return _p._data + _len; }

    private:
        /* 
         *  Wrap all primitive types in an endian/alignment insensitive way.
         */
        template<class T> void append( T j ) {
            little<T>::ref( grow( sizeof( T ) ) ) = j;
        }
    public:
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

        void appendBuf(const void *src, size_t len) { memcpy(grow((unsigned) len), src, len); }

        template<class T>
        void appendStruct(const T& s) { appendBuf(&s, sizeof(T)); }

        void appendStr(const StringData &str , bool includeEOO = true ) {
            const unsigned len = str.size() + ( includeEOO ? 1 : 0 );
            verify( len < (unsigned) BSONObjMaxUserSize );
            str.copyTo( grow(len), includeEOO );
        }

        /** @return the in-use length */
        unsigned len() const { return _len; }

    private:
        static const unsigned Alignment = 8192;

        /** returns the pre-grow write position */
        inline char* grow(unsigned by) {
            unsigned oldlen = _len;
            _len += by;
            if (MONGO_unlikely( _len > _p._size )) {
                growReallocate(oldlen);
            }
            return _p._data + oldlen;
        }

        void growReallocate(unsigned oldLenInUse);
        void kill();
        void mallocSelfAligned(unsigned sz);
        void _malloc(unsigned sz);
        void _realloc(unsigned newSize, unsigned oldLenInUse);
        void _free(void*);

        struct AllocationInfo {
            char *_data;
            void *_allocationAddress;
            unsigned _size;
        } _p;
        unsigned _len;  // bytes in use
    };

}
