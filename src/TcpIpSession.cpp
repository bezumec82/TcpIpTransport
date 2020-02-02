#include "TcpIp.h"

using namespace TcpIp;

void Server::Session::recv( void )
{
    /* If several thread will serve one 'io_service'
     * it will be concurent resource if it will be common.
     * So each receive have its own buffer. */
    BufferShPtr readBuf_shptr = ::std::make_shared< Buffer >(RECV_BUF_SIZE, 0);
    readBuf_shptr->resize( RECV_BUF_SIZE );
    m_socket.async_read_some( ::boost::asio::buffer( readBuf_shptr->data(), readBuf_shptr->size() ),
        [ &, readBuf_shptr ] ( const ErrCode& error, 
            ::std::size_t bytes_transferred ) //mutable
        {
            if( error )
            {
                PRINT_ERR( "Error when reading : %s\n", error.message().c_str() );
                if( m_socket.is_open() )
                {
                    m_socket.shutdown( Socket::shutdown_receive );
                }
                /* TODO : Call for user's error handler. */
                return;
            }
            readBuf_shptr->resize(bytes_transferred);
            m_parent_ptr->getConfig().m_recv_cb( getIp(), * readBuf_shptr );
            this->recv();
        } ); //end async_read_until
}

Server::Session::~Session()
{
    if( m_socket.is_open() )
    {
        m_socket.shutdown( Socket::shutdown_both );
        m_socket.close();
    }
    PRINTF( YEL, "Session destroyed.\n");
}

/* EOF */