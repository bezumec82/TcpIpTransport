#ifndef TCP_IP_SERVER_HPP
#define TCP_IP_SERVER_HPP

#include "TcpIp.h"


namespace TcpIp
{

template< typename Data >
Result Server::send( SessionView& session, Data&& data )
{
    if( session.isValid() )
    {
        session.session()->send( ::std::forward<Data>( data ) );
        return Result::SEND_SUCCESS;
    }
    else
    {
        return Result::SEND_ERROR;
    }
}

template< typename SessionViewList, typename Data >
Result Server::multiCast( SessionViewList&& sessions, Data&& data )
{
    for( const auto& it : sessions )
    {
        if( it.isValid() )
            this->send( it, ::std::forward<Data>( data ) );
    }
}

template< typename Data >
Result Server::broadCast( Data&& data )
{
    ::std::lock_guard< ::std::mutex > lock( m_sessions_mtx ); //access to sessions
    for( auto& it : m_sessions )
    {
        if( it.isValid() )
            it.send( ::std::forward<Data>( data ) );
    }
    return Result::SEND_SUCCESS;
}






} //end namespace TcpIp

#endif /* TCP_IP_HPP */