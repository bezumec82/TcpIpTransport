#include "TcpIp.h"

using namespace TcpIp;

Result TcpIp::Client::setConfig( Config&& cfg ) /* Save config */
{
    boost::system::error_code error;
    m_config = ::std::move( cfg ); //by-member move

    if( m_config.m_address == "" )
    {
        PRINT_ERR( "No IP address provided.\n" );
        return Result::CFG_ERROR;
    }
    if( ! m_config.m_recv_cb ) //not contains function
    {
        PRINT_ERR( "No read callback provided.\n" );
        return Result::CFG_ERROR;
    }
    if( ! m_config.m_send_cb )
    {
        PRINT_ERR( "No send callback provided.\n" );
        return Result::CFG_ERROR;
    }
    if( ! m_config.m_error_cb )
    {
        PRINT_ERR( "No send callback provided.\n" );
        return Result::CFG_ERROR;
    }

    m_address = 
        boost::asio::ip::make_address( m_config.m_address, error );
    if( error )
    {
        PRINT_ERR( "%s", error.message().c_str() );
        return Result::WRONG_IP_ADDRESS;
    }

    m_is_configured.store(true);
    PRINTF( GRN, "Configuration is accepted.\n" );
    return Result::ALL_GOOD;
}

Result Client::start( void )
{
    if( ! m_is_configured.load() )
    {
        PRINT_ERR( "Server has no configuration.\n" );
        throw ::std::runtime_error( "Server has no configuration.\n" );
    }
    ::std::cout << "Starting TcpIp client. Server address : " 
                << m_config.m_address <<::std::endl;
    
    m_socket_uptr = ::std::make_unique< Socket >(m_io_service);
    m_endpoint_uptr = ::std::make_unique<EndPoint>( m_address, m_config.m_port_num ); //::tcp::ip::address here
    connect();
    m_worker = ::std::move( 
        ::std::thread( [&](){ m_io_service.run(); } )
    );
    return Result::ALL_GOOD;
}

#define CON_RETRY_DELAY ::std::chrono::milliseconds( 500 )
void Client::connect()
{
    try {
        m_socket_uptr->connect( * m_endpoint_uptr );
        PRINTF( GRN, "Successfully connected to the server '%s'.\n", 
                m_endpoint_uptr->address().to_string().c_str() );
        m_is_connected.store( true );

        if( m_config.m_delimiter == "" )
            this->recv();
        else this->recv( m_config.m_delimiter );

    } catch( const ::std::exception& e ) {
        PRINT_ERR( "%s.\n", e.what() );
    }
    if( ! m_is_connected.load() ) //try again
    {
        /* Delay */
        ::std::this_thread::sleep_for( CON_RETRY_DELAY );
        this->connect();
    }
}

void Client::recv( void )
{
    BufferShPtr read_buf_shptr = ::std::make_shared< Buffer >();
    read_buf_shptr->resize( RECV_BUF_SIZE );
    m_socket_uptr->async_read_some(
        ::boost::asio::buffer( read_buf_shptr->data(), read_buf_shptr->size() ),
        [ &, read_buf_shptr ] ( const ErrCode& error, 
            ::std::size_t bytes_transferred ) //mutable
        {
            if( error )
            {
                PRINT_ERR( "Error when reading : %s\n", error.message().c_str());
                if( m_socket_uptr->is_open() )
                {
                    m_socket_uptr->shutdown( Socket::shutdown_receive );
                }
                return;
            }
            read_buf_shptr->resize( bytes_transferred );
            m_config.m_recv_cb( * read_buf_shptr );
            this->recv();
        } ); //end async_read_until
}

void Client::recv( ::std::string& msg_delimiter )
{
    BufferShPtr read_buf_shptr = ::std::make_shared< Buffer >();
    read_buf_shptr->reserve( READ_BUF_SIZE );

    ::boost::asio::async_read_until( * m_socket_uptr,
        ::boost::asio::dynamic_buffer( * read_buf_shptr ),
        ::std::string{ msg_delimiter },
        [ & , read_buf_shptr ] ( const ErrCode& error, 
            ::std::size_t bytes_transferred ) //mutable
        {
            if( error )
            {
                PRINT_ERR( "Error when reading : %s\n", error.message().c_str() );
                if( m_socket_uptr->is_open() )
                {
                    m_socket_uptr->shutdown( Socket::shutdown_receive );
                }
                return;
            }
            m_config.m_recv_cb( * read_buf_shptr );
            this->recv( msg_delimiter );
        } ); //end async_read_until
}

/* EOF */