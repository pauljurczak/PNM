#ifndef  Portable_Anymap_Format_E84FDB3F_0BF3_447F_821D_B9480766CC13
#define Portable_Anymap_Format_E84FDB3F_0BF3_447F_821D_B9480766CC13

#include <cctype>
#include <algorithm>
#include <vector>
#include <string>

namespace PNM
 {
  enum type{ error, P1=1, P2=2, P3=3, P4=4, P5=5, P6=6/*, P7=7*/ };

  class Info
   {
    public:
      typedef std::size_t size_type;

      Info():m_type(PNM::error){ }
      Info( std::size_t const& width, std::size_t const& height, PNM::type const& type, std::size_t const& max = 255 )
       :m_width(width)
       ,m_height(height)
       ,m_type(type)
       ,m_max(max)
       {
       }

      bool             valid()  const { return PNM::error != m_type;  }
      size_type const& width()  const { return m_width;  }
      size_type const& height() const { return m_height; }
      size_type const& max()    const { return m_max;    }
      size_type const& channel()const { return m_channel;}
      PNM::type const& type()   const { return m_type;   }

    public:
      size_type  & width()  { return m_width;  }
      size_type  & height() { return m_height; }
      size_type  & max()    { return m_max;    }
      size_type  & channel(){ return m_channel;}
      PNM::type  & type()   { return m_type;   }

    private:
      size_type   m_width;
      size_type   m_height;
      size_type   m_max;
      size_type   m_channel;
      PNM::type   m_type;
   };

  namespace _internal
   {

    inline bool load_NL( std::istream& is ) // Parse new line.
     {
      auto begin = is.tellg();
      auto ch = is.get();
      if( ( ch != 0x0d ) && ( ch != 0x0a ) )
       {
        is.unget();
        return false;
       }
      if( ch == 0x0d ) //!< Windows or Mac
       {
        ch = is.get();
        if( ch != 0x0A ) //!< Not Windows
         {
          is.unget();
         }
       }
      return true;
     }

    inline bool load_comment( std::istream& is )
     {
      auto begin = is.tellg();
      auto ch = is.get();
      if( '#' != ch ){ is.seekg( begin ); return false; }

      do
       {
        if( true == load_NL( is ) )
         {
          return true;
         }
        ch = is.get();
       }while( ( std::char_traits<char>::eof() != ch  ) && ( false == is.eof() )  );

      return true;
     }

    inline bool load_space( std::istream& is ) // tab or space. Not new line.
     {
      std::size_t counter=0;
      auto ch = is.get();
      while( ( ch == ' ' ) || ( ch == '\t' ) )
       {
        ch = is.get();
        ++counter;
       }
      is.unget();
      return 0 != counter;
     }

    inline bool load_blank( std::istream& is )
     {
      bool again = true;
      size_t counter = 0;
      while( true == again )
       {
        again  = load_space( is );
        again |= load_NL( is );
        counter += ( again ? 1 : 0 );
       }
      return 0 != counter;
     }

    inline bool load_junk( std::istream& is )
     {
      bool again = true;
      size_t counter = 0;
      while( true == again )
       {
        again  = load_space( is );
        again |= load_NL( is );
        again |= load_comment( is );
        counter += ( again ? 1 : 0 );
       }
      return 0 != counter;
     }

    inline bool load_number( std::istream& is, std::size_t & number )
     {
      auto ch = is.get();
      if( 0 == std::isdigit( ch ) )
       {
        is.unget();
        return false;
       }

      number = 0;

      do{
        number = number *10 + ( ch - '0' );
        ch = is.get();
       }while( 0 != std::isdigit( ch ) );
      is.unget();
      return true;
     }

    inline  PNM::type load_magic( std::istream& is )
     {
      auto begin = is.tellg();
      auto ch1 = is.get(); if( ch1 != 'P' ){ is.seekg( begin ); return PNM::error; }
      auto ch2 = is.get();
      if( ( ch2 - '0' ) < 0 ) { is.seekg( begin ); return PNM::error; }
      if( 6 < ( ch2 - '0' ) ) { is.seekg( begin ); return PNM::error; }

      return PNM::type( ch2 - '0' );
     }

    template< typename data_type >
     inline  bool load_ascii_P1( std::istream& is, data_type * data, std::size_t const& width, std::size_t const& height )
      {
       auto begin = is.tellg();
       std::size_t number;
       for( std::size_t y=0; y < height; ++y )
        {
         std::size_t position = 8 * y * ( width / 8 + ( ( width  % 8 )?1:0 ) );
         for( std::size_t x=0; x < width; x += 8 )
          {
           data[ position/8 ] = 0;
           for( std::size_t b=0; b < std::min<std::size_t>(width-x, 8 ); ++b )
            {
             load_blank( is );
             if( false == load_number( is, number ) )
              {
               is.seekg( begin );  return false; 
              }

             if( number )
              {
               data[ position/8 ] |=  ( (data_type(1) << (7-(position%8))) );
              }
             else
              {
               data[ position/8 ] &= ~( data_type(1) << (7-(position%8))) ;
              }

             ++position;

             if( false == load_space( is ) )
              {
               if( false == load_NL( is ) )
                {
                 if( ( (height-1) != y) && (( width-1 )!=( x + b ) )  )
                  {
                   is.seekg( begin ); 
                   return false; 
                  }
                }
              }
            }
          }
        }

       return true;
      }

    template< typename data_type >
     inline  bool load_ascii_P2P3( std::istream& is, data_type * data, std::size_t const& width, std::size_t const& height, std::size_t const& channel  )
      {
       auto begin = is.tellg();
       std::size_t number;
       for( std::size_t y=0; y < height; ++y )
        {
         for( std::size_t x=0; x < width*channel; ++x )
          {
           load_space( is );
           if( false == load_number( is, number ) ){ is.seekg( begin );  return false; }
           *data = data_type( number );
           ++data;
           if( false == load_junk( is ) )
            {
             if( ( ( height-1 )!= y ) && ( (width-1) != x ) )
              {
               is.seekg( begin ); 
               return false; 
              }
            }
          }
       }

       return true;
      }

    inline bool load_raw_P4( std::istream& is, std::uint8_t * data, std::size_t const& width, std::size_t const& height )
      {
       is.read( (char*)data, height * ( width / 8 + ( ( width  % 8 )?1:0 ) ) );
       if( is ) return true;
       return false;
      }

    inline bool load_raw_P5P6( std::istream& is, std::uint8_t * data, std::size_t const& width, std::size_t const& height, std::size_t const& channel )
      {
       is.read( (char*)data, width * height * channel );
       if( is ) return true;
       return false;
      }

    template< typename data_type >
     inline  bool save_ascii_P1( std::ostream& os, data_type * data, std::size_t const& width, std::size_t const& height )
      {
       for( std::size_t y=0; y < height; ++y )
        {
         std::size_t position = 8 * y * ( width / 8 + ( ( width  % 8 )?1:0 ) );
         for( std::size_t x=0; x < width ; ++x )
           {
            os << std::setw( 2 ) << (char)((( data[ position/8 ] >>( 7- (position%8) ) ) & 1) + '0') ;
            ++position;
           }
         os << '\x0A' /*os.widen('\n')*/;
        }
       return true;
      }

    template< typename data_type >
     inline  bool save_ascii_P2P3( std::ostream& os, data_type * data, std::size_t const& width, std::size_t const& height, std::size_t const& channel )
      {
       for( std::size_t y=0; y < height; ++y )
        {
         for( std::size_t x=0; x < width * channel; ++x )
           {
            os << std::setw( 3 ) << (int)(*data) << ' ';
            ++data;
           }
         os << '\x0A' /*os.widen('\n')*/;
        }
       return true;
      }

    template< typename data_type >
     inline  bool save_bin_P4( std::ostream& os, data_type * data, std::size_t const& width, std::size_t const& height )
      {
       os.write( (char*)data, height * ( width / 8 + ( ( width  % 8 )?1:0 ) ) );
       return true;
      }

    template< typename data_type >
     inline  bool save_bin_P5P6( std::ostream& os, data_type * data, std::size_t const& width, std::size_t const& height, std::size_t const& channel )
      {
       os.write( (char*)data, width * height * channel );
       return true;
      }

    class Probe
     {
      public:

        explicit Probe( PNM::Info &info )
         :m_width( info.width() )
         ,m_height( info.height() )
         ,m_type( info.type() )
         ,m_channel( info.channel() )
         ,m_max( info.max() )
         {
          this->m_width   = -1;
          this->m_height  = -1;
          this->m_type    = PNM::error;
          this->m_channel = 0;
          this->m_max     = 1;
         }

        bool process( std::istream& is )
         {
          auto begin = is.tellg();
          this->m_type = PNM::_internal::load_magic( is ); if( PNM::error == this->m_type ) { is.seekg( begin ); return false; }
          if( false == PNM::_internal::load_junk( is ) ){ is.seekg( begin ); return false; }
          if( false == load_number( is, m_width  ) ){ is.seekg( begin ); return false; }
          if( false == PNM::_internal::load_junk( is ) ){ is.seekg( begin ); return false; }
          if( false == load_number( is, m_height ) ){ is.seekg( begin ); return false; }
          if( false == PNM::_internal::load_junk( is ) ){ is.seekg( begin ); return false; }

          if( ( this->m_type == PNM::P2 ) || ( this->m_type == PNM::P3 ) || ( this->m_type == PNM::P5 ) || ( this->m_type == PNM::P6 ) )
           {
            if( false == load_number( is, m_max ) ){ is.seekg( begin ); return false; }
           }

          if( ( this->m_type == PNM::P2 ) || ( this->m_type == PNM::P3 ) || ( this->m_type == PNM::P6 ) )
           {
            if( false == PNM::_internal::load_junk( is ) ){ is.seekg( begin ); return false; }
           }

          if( ( PNM::P1 == this->m_type ) || ( PNM::P4 == this->m_type ) ){ m_channel = 1; }
          if( ( PNM::P2 == this->m_type ) || ( PNM::P5 == this->m_type ) ){ m_channel = 1; }
          if( ( PNM::P3 == this->m_type ) || ( PNM::P6 == this->m_type ) ){ m_channel = 3; }

          return true;
         }

        std::size_t const& width() const { return m_width;  }
        std::size_t const& height()const { return m_height; }
        std::size_t const& max()const    { return m_max;    }
        std::size_t const& channel()const{ return m_channel;}
        PNM::type   const& type()  const { return m_type;   }

        std::size_t & m_width;
        std::size_t & m_height;
        std::size_t & m_max;
        std::size_t & m_channel;
        PNM::type   & m_type;
     };

    class VectorLoad
     {
      public:
        VectorLoad( std::vector<std::uint8_t > & data, PNM::Info &info )
         :m_probe( info )
         ,m_data( data )
         {
         }

       bool process( std::istream& is )
        {
         if( PNM::error == m_probe.type() ){ return false; }

         if( ( PNM::P1 == m_probe.type() ) || ( PNM::P4 == m_probe.type() ) )
          {
           m_data.resize( this->m_probe.height() * ( this->m_probe.width()/8 + ( (this->m_probe.width()%8) ?1:0 ) ) );
          }
         else
          {
           m_data.resize( m_probe.width() *  m_probe.height() * m_probe.channel() );
          }

         switch( m_probe.type() )
          {
           case( PNM::P1 ): return load_ascii_P1( is, m_data.data(), m_probe.width(), m_probe.height() );
           case( PNM::P2 ): return load_ascii_P2P3( is, m_data.data(), m_probe.width(), m_probe.height(), m_probe.channel() );
           case( PNM::P3 ): return load_ascii_P2P3( is, m_data.data(), m_probe.width(), m_probe.height(), m_probe.channel() );
           case( PNM::P4 ): return load_raw_P4(   is, m_data.data(), m_probe.width(), m_probe.height() );
           case( PNM::P5 ): return load_raw_P5P6( is, m_data.data(), m_probe.width(), m_probe.height(), m_probe.channel() );
           case( PNM::P6 ): return load_raw_P5P6( is, m_data.data(), m_probe.width(), m_probe.height(), m_probe.channel() );
          }

         return false;
        }

      public:
        PNM::_internal::Probe & probe(){ return m_probe; }
      private:
        PNM::_internal::Probe m_probe;

      private:
        std::vector<std::uint8_t > & m_data;
     };

    class RawLoad
     {
      typedef std::uint8_t* (*allocator_type)( size_t const& size );
      public:

        RawLoad( std::uint8_t * data, PNM::Info &info )
          :m_probe( info )
          ,m_dataP( nullptr )
          ,m_dataX( data )
          ,m_allocator( nullptr )
        {
        }

        RawLoad( std::uint8_t ** data, allocator_type &allocator, PNM::Info &info )
         :m_probe( info )
         ,m_dataP( data )
         ,m_dataX( nullptr )
         ,m_allocator( allocator )
         {
         }

        RawLoad( RawLoad const& that )
         :m_probe( that.m_probe )
         , m_dataP( that.m_dataP )
         , m_allocator( that.m_allocator )
         {
         }

        RawLoad & operator=( RawLoad const& that )
         {
          return *this;
         }

       bool process( std::istream& is )
        {
         if( PNM::error == m_probe.type() ){ return false; }
         std::uint8_t *data = nullptr;

         if( nullptr != m_allocator ) 
          {
           if( ( PNM::P1 == m_probe.type() ) || ( PNM::P4 == m_probe.type() ) )
            {
             data = *m_dataP = m_allocator( this->m_probe.height() * ( this->m_probe.width()/8 + ( (this->m_probe.width()%8) ?1:0 ) ) );
            }
           else
            {
             data = *m_dataP = m_allocator( this->m_probe.width() *  this->m_probe.height() * m_probe.channel() );
            }
          }
         else
          {
           data = m_dataX;
          }

         switch( m_probe.type() )
          {
           default:
           case( PNM::P1 ): return load_ascii_P1(   is, data, m_probe.width(), m_probe.height() );
           case( PNM::P2 ): return load_ascii_P2P3( is, data, m_probe.width(), m_probe.height(), m_probe.channel() );
           case( PNM::P3 ): return load_ascii_P2P3( is, data, m_probe.width(), m_probe.height(), m_probe.channel() );
           case( PNM::P4 ): return load_raw_P4(     is, data, m_probe.width(), m_probe.height() );
           case( PNM::P5 ): return load_raw_P5P6(   is, data, m_probe.width(), m_probe.height(), m_probe.channel() );
           case( PNM::P6 ): return load_raw_P5P6(   is, data, m_probe.width(), m_probe.height(), m_probe.channel() );
          }

         return false;
        }

     public:
       Probe & probe()
        {
         return m_probe;
        }
     private:
       Probe m_probe;

     private:
       std::uint8_t ** m_dataP;
       std::uint8_t  * m_dataX;
       allocator_type m_allocator;
    };

    class RawSave
     {
      public:
        RawSave( std::uint8_t const * data, std::size_t const& width, std::size_t const&  height, PNM::type const&type, std::size_t const&  max = 255 )
         : m_data( data )
         , m_width( width )
         , m_height( height )
         , m_type( type )
         , m_max( max )
         {
         }

       bool process( std::ostream& os )
        {
         os << "P" << char( int(m_type) + '0' ) << '\x0A' /*os.widen('\n')*/;
         os <<  m_width << " " << m_height << '\x0A' /*os.widen('\n')*/;

         if( ( PNM::P2 == m_type ) || ( PNM::P3 == m_type ) || ( PNM::P5 == m_type ) || ( PNM::P6 == m_type ) )
          {
           os << m_max << '\x0A' /*os.widen('\n')*/;
          }

         m_channel = 1;
         if( ( PNM::P3 == m_type  ) || ( PNM::P6 == m_type ) )
          {
           m_channel = 3;
          }

         if( PNM::P1 == m_type )  { return save_ascii_P1( os, m_data, m_width, m_height ); }
         if( PNM::P4 == m_type )  { return save_bin_P4( os, m_data, m_width, m_height ); }
         if( ( PNM::P2 == m_type ) || ( PNM::P3 == m_type ) ) return save_ascii_P2P3( os, m_data, m_width, m_height, m_channel );
         if( ( PNM::P5 == m_type ) || ( PNM::P6 == m_type ) ) return save_bin_P5P6( os, m_data, m_width, m_height, m_channel );

         return false;
        }


       private:
         std::uint8_t const* m_data;
         std::size_t m_width;
         std::size_t m_height;
         std::size_t m_channel;
         PNM::type  m_type;
         std::size_t m_max;
     };

   }


  inline PNM::_internal::VectorLoad load( std::vector<std::uint8_t > & data, PNM::Info &info )
   {
    return PNM::_internal::VectorLoad( data, info );
   }

  inline PNM::_internal::RawLoad load( std::uint8_t * data, PNM::Info &info )
   {
    return PNM::_internal::RawLoad( data, info );
   }

  inline PNM::_internal::RawLoad load( std::uint8_t ** data, std::uint8_t* (*allocator)( size_t const& size ), PNM::Info &info )
   {
    return PNM::_internal::RawLoad( data, allocator, info );
   }

  inline PNM::_internal::RawSave save( std::uint8_t const* data, PNM::Info const& info )
   {
    return PNM::_internal::RawSave( data, info.height(), info.width(), info.type(), info.max() );
   }

  inline PNM::_internal::RawSave save(  std::vector<std::uint8_t > const& data, PNM::Info const& info )
   {
    return PNM::_internal::RawSave( data.data(), info.width(), info.height(), info.type(), info.max() );
   }

  inline PNM::_internal::RawSave save( std::uint8_t const* data, std::size_t const& width, std::size_t const& height, PNM::type const&type, std::size_t const&max = 255 )
   {
    return PNM::_internal::RawSave( data, width, height, type, max );
   }

  inline PNM::_internal::RawSave save( std::vector<std::uint8_t > const& data, std::size_t const& height, std::size_t const& width, PNM::type const&type, std::size_t const&max = 255 )
   {
    return PNM::_internal::RawSave( data.data(), height, width, type, max );
   }

  inline  PNM::_internal::Probe probe( PNM::Info &info )
   {
    return PNM::_internal::Probe( info );
   }

  namespace _internal
   {
    namespace operators
     {

      inline std::ostream& operator<<( std::ostream& os, PNM::_internal::RawSave && rs )
       {
        rs.process( os );
        return os;
       }

      inline std::istream& operator>>( std::istream& is, PNM::_internal::Probe && probe )
       {
        if( false == probe.process( is ) )
         {
          probe.m_type = PNM::error;
         }
        return is;
       }

      inline std::istream& operator>>( std::istream& is, PNM::_internal::VectorLoad && vl )
       {
        if( false == vl.probe().process( is ) )
         {
          return is;
         }

        if( false == vl.process( is ) )
         {
          vl.probe().m_type = PNM::error;
          return is;
         }

        return is;
       }

      inline std::istream& operator>>( std::istream& is, PNM::_internal::RawLoad && rl )
       {
        if( false == rl.probe().process( is ) )
         {
          return is;
         }

        if( false == rl.process( is ) )
         {
          rl.probe().m_type = PNM::error;
          return is;
         }

        return is;
       }
     }
   }

 }

// Ugly but effective
using namespace PNM::_internal::operators;

#endif
