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

#ifndef __Ubitrack_Graph_KeyValueAttributes_INCLUDED__
#define __Ubitrack_Graph_KeyValueAttributes_INCLUDED__  __Ubitrack_Graph_Graph_INCLUDED____Ubitrack_Graph_KeyValueAttributes_INCLUDED__

#include <map>
#include <list>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <log4cpp/Category.hh>
#include <utDataflow.h>
#include <utUtil/Exception.h>
#include <utUtil/Logging.h>

#include <utGraph/AttributeValue.h>

/**
 * @ingroup srg_algorithms
 * @file
 * Key-Value attributes for use as graph edge/node properties
 *
 * The Key Value attributes can be used as both edge or node
 * attributes and represent the concept that any edge or node
 * may be associated with aribtrary key/value pairs.
 * The values can either be accessed as strings or where available
 * as XML trees.
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */

namespace Ubitrack { namespace Graph {

	// TODO: replace strings with string/xml "unions"

	/**
	 * Key-Value attributes for use as graph edge/node properties
	 *
	 * The Key Value attributes can be used as both edge or node
	 * attributes and represent the concept that any edge or node
	 * may be associated with aribtrary key/value pairs.
	 * The values can either be accessed as strings or where available
	 * as XML trees.
	 *
	 * @par TODO XML not yet implemented
	 */
	class UTDATAFLOW_EXPORT KeyValueAttributes
	{
	public:
		/**
		 * Get the attribute for a key
		 * This method returns the value associated to a specified key.
		 *
		 * @param key the key of the attribute
		 * @return the value associated to the key
		 * @throws Ubitrack::Util::Exception if no such key is known.
		 */
		AttributeValue getAttribute ( const std::string& key ) const;

		/**
		 * Set the attribute for a key
		 * This method sets the attribute value for a given key.
		 *
		 * @param key the key of the attribute
		 * @param value the value to be associated with the key
		 */
		void setAttribute ( const std::string& key, const AttributeValue& value );

		/**
		 * Tests if the attribute for a key is present
		 * This method returns if for a given key there is
		 * an attribute available.
		 *
		 * @param key the key of the attribute
		 * @returns true if an attribute for that key is available. false otherwise
		 */
		bool hasAttribute( const std::string& key ) const;

		/**
		 * Get/Set attributes
		 * Via the operator[] attributes can be accessed by key.
		 * If no such key existed before a new attribute is created.
		 *
		 * @param key the key of the attribute
		 * @return a reference to the value of the attribute
		 */
		AttributeValue& operator[] ( const std::string& key );

		/**
		 * Access data for a given key, if available
		 * If data for a key is available, it is parsed and
		 * filled into the parameter passed to the method.
		 * If no such key is available, the variable is not touched.
		 *
		 * @param key the key of the attribute
		 * @param x will be filled with data if available
		 */
		template< typename T > 
		void getAttributeData( const std::string& key, T& x ) const
		{
			AttributeMapType::const_iterator it = m_Values.find( key );
			if ( it == m_Values.end() )
				return;

			std::istringstream is( it->second.getText() );
			is >> x;
		}

		/**
		 * Access string representation for a given key, if available
		 * 
		 * @param key the key of the attribute
		 * @return the string representation of the attribute, empty string otherwise
		 */
		std::string getAttributeString( const std::string& key ) const;

		/** checks if the two attribute sets are equal */
		bool isEqual( const KeyValueAttributes& x ) const;

		/** adds all attribute of another set */
		void mergeAttributes( const KeyValueAttributes& x );

		/** efficiently swaps the contents with another KeyValueAttributes object */
		void swap( KeyValueAttributes& x );

		typedef std::map < std::string, AttributeValue > AttributeMapType;

		/** return a read-only version of the attribute map for iterating */
        const AttributeMapType& map() const
		{ return m_Values; }

	protected:
		/** The map of all attributes */
		AttributeMapType m_Values;

		friend UTDATAFLOW_EXPORT std::ostream& operator<<( std::ostream&, const KeyValueAttributes& );
	};

	/** streaming operator for KeyValueAttributes (for debugging) */
	UTDATAFLOW_EXPORT std::ostream& operator<<( std::ostream& os, const KeyValueAttributes& kv );
}}

#endif // __Ubitrack_Graph_KeyValueGraphAttributes_INCLUDED__
