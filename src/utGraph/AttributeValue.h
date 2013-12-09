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
 * @ingroup dataflow_framework
 * @file
 * Attribute value container.
 * This file contains the datastructure to store UTQL attributes as extracted from the XML.
 * Also supports dynamic conversion with lazy evaluation for strings and doubles.
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */


#ifndef __Ubitrack_Graph_AttributeValue_INCLUDED__
#define __Ubitrack_Graph_AttributeValue_INCLUDED__ __Ubitrack_Graph_AttributeValue_INCLUDED__

#include <boost/shared_ptr.hpp>

#include <string>
#include <sstream>
#include <tinyxml.h>

#include <utDataflow.h>

namespace Ubitrack { namespace Graph {

	/**
	 * XML Element
	 *
	 * XML constructed configuration element.
	 * This represents one part of a configuration.
	 * The configuration is available as either a string, a double or
	 * an XML DOM tree.
	 */
	struct UTDATAFLOW_EXPORT AttributeValue
	{
		/// Type for storing XML Node pointer
		typedef boost::shared_ptr< TiXmlElement > XMLNodePtr;

		/**
		 * Empty constructor
		 *
		 * This constructor creates an empty configuration element
		 * which evaluates either to an empty string or a null pointer
		 */
		AttributeValue()
			: m_contentState( stateEmpty )
		{}

		/** Constructor from XML Tree Pointer
		 *
		 * This constructor creates an configuration element from a pointer to a
		 * cloned XML Element. This constrcutor does not clone the element itself
		 * but stores a shared_ptr to the node.
		 * @param ptr pointer to XML node
		 */
		AttributeValue( XMLNodePtr ptr )
			: m_node( ptr )
			, m_contentState( ptr == 0 ? stateEmpty : stateUnchecked )
		{}

		/** Constructor from String
		 *
		 * This constructor creates an configuration element from a string.
		 * Note that only string representations can be accessed from this
		 * element in that case. XML access will produce NULL.
		 * @param str the string representation of the element
		 */
		AttributeValue( const std::string& str )
			: m_string( str )
			, m_contentState( str.empty() ? stateEmpty : stateUnchecked )
		{}

		/** Constructor from double
		 *
		 * This constructor creates an configuration element from a double.
		 * Note that only string representations can be accessed from this
		 * element in that case. XML access will produce NULL.
		 * @param str the string representation of the element
		 */
		AttributeValue( double v )
			: m_double( v )
			, m_contentState( stateNumber )
		{}

		/** Access XML Element structure
		 *
		 * This function returns a pointer to an xml element representing the
		 * configuration.
		 * Pointer is NULL if no XML configuration is
		 * present.The user must not delete this pointer. This function is const.
		 * @return const pointer to XML element containing the configuration
		 */
		const TiXmlElement* getXML() const
		{
			// just return the pointer
			return m_node.get();
		}

		/** Access configuration as text
		 *
		 * This function returns a string containing the configuration. An
		 * empty string is returned if no configuration is present.
		 * @return string representing the configuration.
		 */
		const std::string& getText() const;
		
		/** Access configuration as double
		 *
		 * This function returns a double containing the configuration. An
		 * exception is thrown if no number configuration is present.
		 * @return string representing the configuration.
		 */
		double getNumber() const;

		/** Check if this element does represent some information
		 *
		 * This function returns whether the configuration element is
		 * empty or not.
		 * @return true if the element is empty. false otherwise
		 */
		 bool isEmpty() const
		 { return m_contentState == stateEmpty; }

		/** Check if this element does represent a number
		 *
		 * This function returns whether the configuration element is
		 * a number or not.
		 * @return true if the element is empty. false otherwise
		 */
		 bool isNumber() const
		 { 
			 if ( m_contentState == stateEmpty )
				 return false;
			 if ( m_contentState == stateUnchecked )
				 checkNumber();
			 return m_contentState == stateNumber;
		 }


		 /** compare against other attribute */
		 bool operator==( const AttributeValue& b ) const;

	protected:
		/// the smart pointer containing the XML tree.
		XMLNodePtr m_node;
		
		/// the string representation
		mutable std::string m_string;
		
		/// the double representation
		mutable double m_double;
		
		/// is the element empty, not checked for number, a number or no number
		mutable enum { stateEmpty, stateUnchecked, stateNumber, stateNoNumber } m_contentState;

		/// check if the attribute is a number and sets the state accordingly
		void checkNumber() const;
	};

} } // namespace Ubitrack::Graph

#endif // __Ubitrack_Graph_ComponentConfiguration_INCLUDED__
