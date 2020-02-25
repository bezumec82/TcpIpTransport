#include "TcpIp.h"

using namespace TcpIp;

Result Server::setConfig( Config&& cfg ) /* Save config */
{
    boost::system::error_code error;
    m_config = ::std::move( cfg ); //by-member move

    if( ! m_config.m_recv_cb ) //not contains function
    {
        PRINT_ERR( "No RECEIVE callback provided.\n" );
        return Result::CFG_ERROR;
    }
    if( ! m_config.m_send_cb )
    {
        PRINT_ERR( "No SEND callback provided.\n" );
        return Result::CFG_ERROR;
    }
    if( ! m_config.m_error_cb )
    {
        PRINT_ERR( "No ERROR callback provided.\n" );
        return Result::CFG_ERROR;
    }

    if( m_config.m_address == "" )
    {
        PRINT_ERR( "No IP address provided.\n" );
        return Result::CFG_ERROR;
    }
    m_address = 
        boost::asio::ip::make_address( m_config.m_address, error );
    if( error )
    {
        PRINT_ERR( "%s", error.message().c_str() );
        return Result::WRONG_IP_ADDRESS;
    }

    m_is_configured.store( true );
    PRINTF( GRN, "Configuration is accepted.\n" );
    return Result::ALL_GOOD;
}

Result Server::start( void )
{
    if( ! m_is_configured.load() )
    {
        PRINT_ERR( "Server has no configuration.\n" );
        return Result::CFG_ERROR;
    }    
    m_endpoint_uptr = ::std::make_unique<EndPoint>( m_address, m_config.m_port_num );
    m_acceptor_uptr = ::std::make_unique<Acceptor>( m_io_service, * m_endpoint_uptr );
    accept();
    auto work = [&](){
        m_io_service.run();
    }; //end []

#ifdef THREAD_IMPLEMENTATION
    m_worker = ::std::move( ::std::thread( work ) );
#else
    m_future = ::std::async( work );
#endif
    return Result::ALL_GOOD;
}

void Server::accept( void )
{
    /* Full session creation before accept */
    SessionHandle session_handle = m_sessions.emplace( \
        m_sessions.end(), m_io_service, this  );    
    m_acceptor_uptr->async_accept( session_handle->socket(),
        [ &, session_handle ]( ErrCode error ) mutable
        {
            if( !error )
            {
                PRINTF( GRN, "Client accepted.\n" );                
                session_handle->setValid( true );
                if( m_config.m_delimiter == "" )
                    session_handle->recv();
                else session_handle->recv( m_config.m_delimiter );

                this->accept();
            }
            else
            {
                PRINT_ERR( "Error when accepting : %s\n", error.message().c_str());
                
                m_config.m_error_cb( SessionView{ session_handle }, 
                    error.message().c_str() );
                removeSession( SessionView{ session_handle } );
            } //end if
        } /* lambda */ )/* async_accept */;
}




Server::~Server()
{
    /* Stop accepting */
    m_acceptor_uptr->cancel();
    m_acceptor_uptr->close();
    {/* Destroy all sessions */
        ::std::lock_guard< ::std::mutex > lock( m_sessions_mtx );
        m_sessions.clear();
    }
    /* Stop handling events */
    m_io_service.stop();
    m_worker.join();
    PRINTF( YEL, "Server destroyed.\n" );
}

/* EOF */

