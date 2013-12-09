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

#ifndef __Ubitrack_Graph_UTQLSubgraph_INCLUDED__
#define __Ubitrack_Graph_UTQLSubgraph_INCLUDED__ __Ubitrack_Graph_UTQLSubgraph_INCLUDED__

#include <map>
#include <list>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <utGraph/InOutAttribute.h>
#include <utGraph/PatternAttributes.h>
#include <utGraph/Graph.h>

#include <log4cpp/Category.hh>
#include <utUtil/Exception.h>
#include <utUtil/Logging.h>

#include "tinyxml.h"

/**
 * @ingroup srg_algorithms
 * @file
 * UTQL Subgraph data structure
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */

namespace Ubitrack { namespace Graph {
	// forward declarations
	class AttributeExpression;

	class EdgeReference
	{
	public:
		EdgeReference()
		{}

		EdgeReference( const std::string& graphRef, const std::string& edgeRef )
			: m_id( graphRef )
			, m_name( edgeRef )
		{
			if ( graphRef.empty() || edgeRef.empty() )
				UBITRACK_THROW( "illegal edge reference: " + graphRef + ":" + edgeRef );
		}

		const std::string& getSubgraphId() const
		{
			return m_id;
		}

		const std::string& getEdgeName() const
		{
			return m_name;
		}

		bool empty() const
		{ return m_name.empty() || m_id.empty(); }
		
	protected:
		std::string m_id;
		std::string m_name;
	};

	class UTQLNode
		: public InOutAttribute
	    , public PatternAttributes
	{
	public:
		UTQLNode( InOutAttribute::Tag tag )
			: InOutAttribute( tag )
		{}

		// Nodes in the output section may have qualified names in the global sense (Ids)
		std::string m_QualifiedName;
	};

	class UTQLEdge
		: public InOutAttribute
	    , public PatternAttributes
	{
	public:
		UTQLEdge( InOutAttribute::Tag tag )
			: InOutAttribute( tag )
		{}

		// Edges in the output section are references to edges
		// in other subgraphs
		EdgeReference m_EdgeReference;
	};


	class UTQLSubgraph
		: public Graph< UTQLNode, UTQLEdge >
	{
	public:

		UTQLSubgraph()
			: m_onlyBestEdgeMatch( false )
		{}


		UTQLSubgraph( std::string id, std::string name )
			: m_ID( id )
			, m_Name( name )
			, m_onlyBestEdgeMatch( false )
		{}

		// general pattern attributes
		std::string m_ID;
		std::string m_Name;

		// constraints
		/** accept only the best matching for a given node set? */
		bool m_onlyBestEdgeMatch;

		/** attribute expression that is to be minimized */
		boost::shared_ptr< AttributeExpression > m_bestMatchExpression;

		// dataflow configuration
		AttributeValue m_DataflowConfiguration;
		KeyValueAttributes m_DataflowAttributes;
		std::string m_DataflowClass;
	};

}}

#endif // __Ubitrack_Graph_UTQLSubgraph_INCLUDED__
