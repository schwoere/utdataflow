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

#ifndef __Ubitrack_Graph_InOutAttribute_INCLUDED__
#define __Ubitrack_Graph_InOutAttribute_INCLUDED__ __Ubitrack_Graph_InOutAttribute_INCLUDED__

#include <utGraph/Graph.h>
#include <utGraph/PredicateList.h>
#include <utGraph/KeyValueAttributes.h>
#include <list>

/**
 * @ingroup srg_algorithms
 * @file
 * Input/Output-Section concept for UTQL sections.
 *
 * The InOutAttribute extends graph attributes such that
 * each edge or node may belong to either an input section or an output
 * section in UTQL.
 * An iterator class is provided to iterate only over edges of
 * one section.
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */

namespace Ubitrack { namespace Graph {

	// forward declaration
	class AttributeExpression;

	/**
	 * In/Out attribute for use as graph edge/node properties
	 *
	 * The InOutAttribute extend graph attribute concepts such
	 * that every edge or node belongs to either an input or an output
	 * section in UTQL.
	 */
	class InOutAttribute
		: public PredicateList
	    , public KeyValueAttributes
	{
	public:
		/** Tag datatype: either Input or Output */
		enum Tag { Input, Output };

		/** The tag of the graph entity */
		Tag m_Tag;

		/**
		 * Standard constructor.
		 *
		 * @param tag the tag of the graph entity.
		 */
		InOutAttribute( Tag tag )
			: m_Tag( tag )
		{}

		/**
		 * Check if the graph entity belongs to input section.
		 *
		 * @return true if the entity has the input tag. false otherwise
		 */
		bool isInput() const
		{
			return m_Tag == Input;
		}

		/**
		 * Check if the graph entity belongs to output secvtion.
		 *
		 * @return true if the entity has the output tag. false otherwise.
		 */
		bool isOutput() const
		{
			return m_Tag == Output;

		}

		/** type of attribute expression list */
		typedef std::list< std::pair< std::string, boost::shared_ptr< AttributeExpression > > > ExpressionList;

		/** list of attribute expressions */
		ExpressionList m_attributeExpressions;


	};

	/**
	 * Selective Input or Output Iterator
	 *
	 * Iterator class that only iterates over edges belonging to a specified tag.
	 * Automatically skips edges that belong to the other section or
	 * becoms end() if no more edge is available.
	 */
	template < class NodeAttributes, class EdgeAttributes > class InOutIterator
	{
	protected:
		/** the type of the edge list */
		typedef typename GraphNode< NodeAttributes, EdgeAttributes >::EdgeList EdgeList;
		/** the iterator type from EdgeList */
		typedef typename EdgeList::iterator itT;

	public:
		/** The selection datatype */
		enum Selection { select_all, select_input, select_output };

		/**
		 * Standard constructor without arguments.
		 *
		 */
		InOutIterator()
			: m_Selection( select_all )
		{}

		/**
		 * Standard constructor
		 *
		 * @param it start iterator. will not be checked if consistent with selection
		 * @param end end iterator
		 * @param sel selection of edge tags
		 */
		InOutIterator( itT it, itT end, Selection sel = select_all  )
			: m_Iterator( it )
			, m_End( end )
			, m_Selection( sel )
		{}

		/**
		 * Standard constructor
		 *
		 * @param it start iterator. will not be checked if consistent with selection
		 * @param list the edge list over which this iterator should iterate
		 * @param sel selection of edge tags
		 */
		InOutIterator( EdgeList& list, Selection sel )
			: m_Iterator( list.begin() )
			, m_End ( list.end() )
			, m_Selection ( sel )
		{
			if ( m_Iterator->expired() )
			{
				// Logging
				UBITRACK_THROW( "Expired edge in EdgeList of Node" );
			}
			if (!((m_Selection == select_all) ||
				  (m_Iterator->lock()->isInput() && m_Selection==select_input)))
			{
				advanceIterator();
			}
		}

		/**
		 * operator->
		 *
		 * implements iterator semantic operator->
		 *
		 * @return pointed to object
		 */
		typename itT::pointer operator->() const
		{
			return m_Iterator.operator->();
		}

		/**
		 * operator*
		 *
		 * implements iterator semantic operator*
		 *
		 * @return dereferenced pointer
		 */
		typename itT::reference operator*() const
		{
			return m_Iterator.operator*();
		}

		/**
		 * operator==
		 *
		 * implements iterator semantic operator==
		 *
		 * @return true if underlying iterators are equal. false otherwise
		 */
		bool operator==( const InOutIterator& other )
		{
			return m_Iterator == other.m_Iterator;
		}

		/**
		 * operator!=
		 *
		 * implements iterator semantic operator!=
		 *
		 * @return true if underlying iterators are inequal. false otherwise
		 */
		bool operator!=( const InOutIterator& other )
		{
			return m_Iterator != other.m_Iterator;
		}

		/**
		 * operator++(int)
		 *
		 * advances iterator by one. obeys selection criterion.
		 *
		 * @return reference to this instance
		 */
		InOutIterator& operator++(int)
		{
			return operator++();
		}

		/**
		 * operator++(int)
		 *
		 * advances iterator by one. obeys selection criterion.
		 *
		 * @return reference to this instance
		 */
		InOutIterator& operator++()
		{
			advanceIterator();
			return *this;
		}

		/**
		 * checks if this iterator already points to end()
		 *
		 * @return true if this iterator points to end. false otherwise.
		 */
		bool isEnd() const
		{
			return m_Iterator == m_End;
		}


	protected:
		/** the current iterator */
		itT m_Iterator;
		/** the end iterator */
		itT m_End;
		/** the selection by which this iterator filters */
		Selection m_Selection;

		/**
		 * advance the iterator to the next valid position
		 *
		 * @throws Ubitrack::Util::Exception if expired edges are encountered in the edge list
		 */
		void advanceIterator()
		{
			m_Iterator++;
			switch ( m_Selection )
			{
			case select_all:
			default:
				break;
			case select_input:
				while ( m_Iterator != m_End )
				{
					if ( m_Iterator->expired() )
					{
						// Logging
						UBITRACK_THROW ( "Expired edge in EdgeList of Node" );
					}
					if ( m_Iterator->lock()->isInput() )
					{
						break;
					}
					m_Iterator++;
				}
			case select_output:
				while ( m_Iterator != m_End )
				{
					if ( m_Iterator->expired() )
					{
						// Logging
						UBITRACK_THROW ( "Expired edge in EdgeList of Node" );
					}
					if ( m_Iterator->lock()->isOutput() )
					{
						break;
					}
					m_Iterator++;
				}
			}
		}
	};
}}

#endif // __Ubitrack_Graph_InOutEdgeAttributes_INCLUDED__
