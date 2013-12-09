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

#include "KeyValueAttributes.h"

#include <log4cpp/Category.hh>
#include <utUtil/Exception.h>
#include <utUtil/Logging.h>

/**
 * @ingroup srg_algorithms
 * @file
 * Implementations for KeyValueGraphAttributes.h
 * @author Manuel Huber <huberma@in.tum.de>
 */

static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Graph.KeyValueAttributes" ) );

namespace Ubitrack { namespace Graph {

	AttributeValue KeyValueAttributes::getAttribute ( const std::string& key ) const
	{
		AttributeMapType::const_iterator it = m_Values.find ( key );
		if ( it == m_Values.end() )
		{
			LOG4CPP_ERROR ( logger, "No such attribute: " << key );
			UBITRACK_THROW ( "Unknown key in get attribute" );
		}

		return it->second;
	}

	bool KeyValueAttributes::hasAttribute( const std::string& key ) const
	{
		return ( m_Values.find ( key ) != m_Values.end() );
	}


	void KeyValueAttributes::setAttribute ( const std::string& key, const AttributeValue& value )
	{
		m_Values[key] = value;
	}

	void KeyValueAttributes::mergeAttributes( const KeyValueAttributes& x )
	{
		for ( std::map< std::string, AttributeValue >::const_iterator it = x.m_Values.begin();
			  it != x.m_Values.end(); ++it )
		{
			setAttribute( it->first, it->second );
		}
	}

	void KeyValueAttributes::swap( KeyValueAttributes& x )
	{
		m_Values.swap( x.m_Values );
	}

	bool KeyValueAttributes::isEqual( const KeyValueAttributes& x ) const
	{
		return m_Values == x.m_Values;
	}


	AttributeValue& KeyValueAttributes::operator[] ( const std::string& key )
	{
		return m_Values[key];
	}

	
	std::string KeyValueAttributes::getAttributeString( const std::string& key ) const
	{

		AttributeMapType::const_iterator it = m_Values.find( key );
		if ( it == m_Values.end() )
		{
			return std::string();			
		}
		return it->second.getText();

	}
	

	std::ostream& operator<<( std::ostream& os, const KeyValueAttributes& kv )
	{
		os << "{ ";
		for ( std::map< std::string, AttributeValue >::const_iterator it = kv.m_Values.begin(); it != kv.m_Values.end(); ++it )
			os << it->first << ": " << it->second.getText() << ", ";
		os << " }";
		return os;
	}
}}
