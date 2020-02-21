#include "TcpIp.h"

using namespace TcpIp;

void Server::Session::recv( void )
{
    /* If several thread will serve one 'io_service'
     * it will be concurent resource if it will be common.
     * So each receive have its own buffer. */
    BufferShPtr readBuf_shptr = ::std::make_shared< Buffer >(RECV_BUF_SIZE, 0);
    readBuf_shptr->reserve( RECV_BUF_SIZE );
    m_socket.async_read_some(
        ::boost::asio::buffer( readBuf_shptr->data(), readBuf_shptr->size() ),
        [ &, readBuf_shptr ] ( const ErrCode& error,
            ::std::size_t bytes_transferred ) mutable
        {
            if( error )
            {
                setValid( false );
                PRINT_ERR( "Error when reading : %s\n", error.message().c_str() );
                if( m_socket.is_open() )
                {
                    m_socket.shutdown( Socket::shutdown_both );
                }
                m_parent_ptr->getConfig().m_error_cb( \
                    * m_self, error.message().c_str() );
                m_io_service_ref.post( 
                    [&]() mutable
                    {
                        m_parent_ptr->removeSession( * m_self );
                    } );
                return;
            }
            m_parent_ptr->getConfig().m_recv_cb( * m_self, * readBuf_shptr );
            this->recv();
        } ); //end async_read_until
}

Server::Session::~Session()
{
    setValid( false );
    if( m_socket.is_open() )
    {
        m_socket.shutdown( Socket::shutdown_both );
        m_socket.close();
    }
    PRINTF( YEL, "Session destroyed.\n");
}

/* EOF */