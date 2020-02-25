#include "TcpIp.h"

using namespace TcpIp;

void Server::Session::recv( void )
{
    /* If several thread will serve one 'io_service'
     * it will be concurent resource if it will be common.
     * So each receive have its own buffer. */
    BufferShPtr read_buf_shptr = ::std::make_shared< Buffer >(RECV_BUF_SIZE, 0);
    read_buf_shptr->resize( RECV_BUF_SIZE );
    m_socket.async_read_some(
        ::boost::asio::buffer( read_buf_shptr->data(), read_buf_shptr->size() ),
        [ &, read_buf_shptr ] ( const ErrCode& error,
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
            read_buf_shptr->resize( bytes_transferred );
            m_parent_ptr->getConfig().m_recv_cb( * m_self, * read_buf_shptr );
            this->recv();
        } ); //end async_read_until
}


void Server::Session::recv( ::std::string& msg_delimiter )
{
    BufferShPtr read_buf_shptr = ::std::make_shared< Buffer >();
    read_buf_shptr->reserve( READ_BUF_SIZE );

    ::boost::asio::async_read_until( m_socket,
        ::boost::asio::dynamic_buffer( * read_buf_shptr ),
        msg_delimiter,
        [ &, read_buf_shptr ] ( const ErrCode& error, 
            ::std::size_t bytes_transferred ) //mutable
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
            } //end if( error )
            m_parent_ptr->getConfig().m_recv_cb( * m_self, * read_buf_shptr );
            this->recv( msg_delimiter );
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