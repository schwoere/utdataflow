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
 * @file
 * Implementations for AttributeValue
 * @author Manuel Huber <huberma@in.tum.de>
 */

#include "AttributeValue.h"
#include <utUtil/Exception.h>
#include <sstream>

namespace Ubitrack { namespace Graph {

	const std::string& AttributeValue::getText() const
	{
		// If a string only element is stored or if a cached
		// string representation is availabe return this.
		if ( m_string.length() != 0 || m_contentState == stateEmpty )
			return m_string;

		// XML to string
		if ( m_node )
		{
			// iterate over all childs and store their textual representation
			// in an stringstream
			// we can use this more elegant loop because we dont care
			// for Handles here but work with Nodes directly..
			std::ostringstream configData;

			TiXmlNode* valueNode = m_node->FirstChild( "Value" );
			if ( valueNode )
				for ( TiXmlNode* child = valueNode->FirstChild(); child; child = valueNode->IterateChildren ( child ) ) 
					configData << *child;

			m_string = configData.str();
		}
		// double to string
		else if ( m_contentState == stateNumber )
		{
			std::ostringstream data;
			data << m_double;
			m_string = data.str();
		}

		return m_string;
	}


	double AttributeValue::getNumber() const
	{
		if ( m_contentState == stateUnchecked )
			checkNumber();

		if ( m_contentState == stateEmpty || m_contentState == stateNoNumber )
			UBITRACK_THROW( "Attribute is no number! (" + m_string + ")" );

		return m_double;
	}

	bool AttributeValue::operator==( const AttributeValue& b ) const
	{
		if ( m_node )
			return m_node == b.m_node;
		
		//if ( isNumber() && b.isNumber() )
		//	std::cout << "Numeric comparison of " << m_double << " and " << b.m_double << std::endl;
		//else
		//	std::cout << "String comparison of \"" << getText() << "\" and \"" << b.getText() << "\"" << std::endl;

		return ( isNumber() && b.isNumber() && m_double == b.m_double ) || getText() == b.getText();
	}

	void AttributeValue::checkNumber() const
	{
		// assuming that state = unchecked

		// not yet converted to string?
		if ( !m_string.length() )
			getText();

		// try to parse number
		const char* nptr = m_string.c_str();
		char* endptr;
		m_double = strtod( nptr, &endptr );
		if ( endptr == nptr || *endptr != 0 )
			m_contentState = stateNoNumber;
		else
			m_contentState = stateNumber;
	}

} } // namespace Ubitrack::Graph
