/*
 * Ubitrack - Library for Ubiquitous Tracking
 * Copyright 2006, Technische Universitaet Muenchen, and individual
 * contributors as indicated by the @authors tag. See the
 * copyright.txt in the distribution for a full listing of individual
 * contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 */

/**
 * @ingroup clientserver
 * @file
 * Implementation of a TCP-based interface for bi-directional connections between clients and servers
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */
#include "TcpConnection.h"
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <log4cpp/Category.hh>
#include <utUtil/Exception.h>

#include <sstream>

static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.ClientServer.TcpConnection" ) );

using asio::ip::tcp;


namespace Ubitrack { namespace ClientServer {

// for security reasons, limit maximum packet size
static const size_t g_nMaxPacketSize = 1024 * 1024;


TcpConnection::TcpConnection( boost::shared_ptr< tcp::socket > pSocket )
	: m_bBadSocket( false )
	, m_pSocket( pSocket )
{
	// find out the name of the other side
	std::ostringstream oss;
	oss << pSocket->remote_endpoint().address() << "/" << pSocket->remote_endpoint().port();

	m_sName = oss.str();

	// start a size read operation
	readSize();
}


void TcpConnection::send( const TcpConnection::BufferType& data )
{
	if ( !checkSize( data.size() ) )
		return;

	// create a buffer for the packet size
	std::string sSize( makeSize( data.size() ) );

	// create buffer list and send
	boost::array< asio::const_buffer, 2 > sendBuffers =
		{ { asio::buffer( sSize ), asio::buffer( data ) } };

	try
	{
		asio::write( *m_pSocket, sendBuffers );
	}
	catch ( const boost::system::system_error& )
	{
		m_bBadSocket = true;
	}
}


void TcpConnection::send( const std::string& data )
{
	if ( !checkSize( data.size() ) )
		return;

	// create a buffer for the packet size
	std::string sSize( makeSize( data.size() ) );

	// create buffer list and send
	boost::array< asio::const_buffer, 2 > sendBuffers =
		{ { asio::buffer( sSize ), asio::buffer( data ) } };

	try
	{
		asio::write( *m_pSocket, sendBuffers );
	}
	catch ( const boost::system::system_error& )
	{
		m_bBadSocket = true;
	}
}


void TcpConnection::setReceiver( TcpConnection::ReceiveHandler receiver )
{
	m_readHandler = receiver;
}


bool TcpConnection::badConnection()
{
	return m_bBadSocket;
}


const std::string& TcpConnection::getName()
{
	return m_sName;
}


void TcpConnection::readSize()
{
	// start reading next data size
	asio::async_read( *m_pSocket, asio::buffer( m_receivePacketSize, 8 ),
		boost::bind( &TcpConnection::handleSizeRead, this, _1, _2 ) );
}


void TcpConnection::readData( std::size_t nSize )
{
	// create a buffer and start reading data
	m_pReadBuffer.reset( new BufferType( nSize ) );
	asio::async_read( *m_pSocket, asio::buffer( *m_pReadBuffer ),
		boost::bind( &TcpConnection::handleDataRead, this, _1, _2 ) );
}


std::string TcpConnection::makeSize( std::size_t nSize )
{
	char tmp[ 20 ];
	sprintf( tmp, "%08X", nSize );
	return std::string( tmp, tmp + 8 );
}


bool TcpConnection::checkSize( std::size_t nSize )
{
	// security check
	if ( nSize > g_nMaxPacketSize )
	{
		LOG4CPP_ERROR( logger, "Packet size too big" );
		m_bBadSocket = true;
		return false;
	}

	return true;
}


void TcpConnection::handleSizeRead( const boost::system::error_code error, std::size_t bytesTransferred )
{
	if ( error )
	{
		if ( error == asio::error::connection_reset )
			LOG4CPP_INFO( logger, "Connection closed" )
		else
			LOG4CPP_ERROR( logger, "Error in handleSizeRead: " << error.message() )

		m_bBadSocket = true;
		return;
	}

	// terminate the string & read size information
	m_receivePacketSize[ 8 ] = 0;
	char* dummy;
	size_t nSize = strtoul( m_receivePacketSize, &dummy, 16 );

	// security check
	if ( !checkSize( nSize ) )
		return;

	if ( nSize == 0 )
		// keep-alive packet, start reading next size
		readSize();
	else
		readData( nSize );
}


void TcpConnection::handleDataRead( const boost::system::error_code error, std::size_t bytesTransferred )
{
	if ( error )
	{
		if ( error == asio::error::connection_reset )
			LOG4CPP_INFO( logger, "Connection closed" )
		else
			LOG4CPP_ERROR( logger, "Error in handleDataRead: " << error.message() )

		m_bBadSocket = true;
		return;
	}

	// call read handler
	try
	{
		m_readHandler( m_pReadBuffer );
	}
	catch ( const Util::Exception& e )
	{ LOG4CPP_ERROR( logger, "TCP read handler throwed exception: " << e ); }
	catch ( const std::runtime_error& e )
	{ LOG4CPP_ERROR( logger, "TCP read handler throwed std exception: " << e.what() ); }
	catch ( const boost::system::system_error& e )
	{ LOG4CPP_ERROR( logger, "TCP read handler throwed asio exception: " << e.what() << " (" << e.code() << ")" ); }
	catch ( ... )
	{ LOG4CPP_ERROR( logger, "TCP read handler throwed unknown exception" ); }

	// start reading next data size
	readSize();
}


} } // namespace Ubitrack::ClientServer
