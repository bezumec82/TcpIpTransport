#include "TcpIp.h"

using namespace TcpIp;

Result TcpIp::Server::setConfig( Config&& cfg ) /* Save config */
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

Result TcpIp::Server::start( void )
{
    if( ! m_is_configured.load() )
    {
        PRINT_ERR( "Server has no configuration.\n" );
        return Result::CFG_ERROR;
    }    
    m_endpoint_uptr = ::std::make_unique<EndPoint>( m_address, m_config.m_port_num );
    m_acceptor_uptr = ::std::make_unique<Acceptor>( m_io_service, * m_endpoint_uptr );
    accept();
    /* TO DO : make several threads for one 'io_service' */
    m_worker = ::std::move( ::std::thread(
                [&]()
                { 
                    m_io_service.run(); 
                } /* lambda */ )/* thread */ )/* move */;
    m_is_started.store( true );
    return Result::ALL_GOOD;
}

void TcpIp::Server::accept( void )
{
    SessionHandle session_handle = m_sessions.emplace( m_sessions.end(), m_io_service, this  );
    m_acceptor_uptr->async_accept(session_handle->getSocket(),
        [&, session_handle]( ErrCode error ) //mutable
        {
            if( !error )
            {
                PRINTF( GRN, "Client accepted.\n" );
                session_handle->saveHandle( session_handle );
                session_handle->recv();
                this->accept();
            }
            else
            {
                PRINT_ERR( "Error when accepting : %s\n", error.message().c_str());
            } //end if
        } /* lambda */ )/* async_accept */;
}

Server::~Server()
{
    /* Stop accepting */
    m_acceptor_uptr->cancel();
    m_acceptor_uptr->close();
    /* Destroy all sessions */
    m_sessions.clear();
    /* Stop handling events */
    m_io_service.stop();
    m_worker.join();
    PRINTF( YEL, "Server destroyed.\n" );
}

/* EOF */

