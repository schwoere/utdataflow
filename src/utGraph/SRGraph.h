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

#ifndef __Ubitrack_Graph_SRGraph_INCLUDED__
#define __Ubitrack_Graph_SRGraph_INCLUDED__ __Ubitrack_Graph_SRGraph_INCLUDED__

#include <map>
#include <list>
#include <string>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <utGraph/Graph.h>
#include <utGraph/UTQLSubgraph.h>
#include <utGraph/PatternAttributes.h>

#include <log4cpp/Category.hh>
#include <utUtil/Exception.h>
#include <utUtil/Logging.h>

/**
 * @ingroup srg_algorithms
 * @file
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */

namespace Ubitrack { namespace Graph {

	class SRGNodeAttributes
	    : public KeyValueAttributes
	    , public PatternAttributes
	{
	public:
		SRGNodeAttributes()
		{}

		SRGNodeAttributes( UTQLSubgraph::GraphNodeAttributes& utqlAttrs, const std::string& subgraphID,
						   UTQLSubgraph::NodePtr referenceNode )
			: KeyValueAttributes( utqlAttrs )
		{
			if ( subgraphID.length() != 0 )
			{
				m_SubgraphIDs.insert( subgraphID );
			}

			m_NodeRefs.insert( referenceNode );
		}


		// the ids of the subgraphs which spawn this node
		// is either an applied pattern or srg registration
		std::set< std::string > m_SubgraphIDs;
		// keep a list of all nodes that are matched to this one in all
		// pattern instances and srg registratons ( for updateing node
		// attributes )
		std::set< UTQLSubgraph::NodePtr > m_NodeRefs;
	};

	class SRGEdgeAttributes
		: public KeyValueAttributes
	    , public PatternAttributes
	{
	public:
		SRGEdgeAttributes()
		{}

		SRGEdgeAttributes( UTQLSubgraph::GraphEdgeAttributes& utqlAttrs, const std::string& subgraphID, const std::string& localName )
			: KeyValueAttributes( utqlAttrs )
			, m_SubgraphID( subgraphID )
			, m_LocalName( localName )
		{}

		// the id of the subgraph which spawns this edge
		// is either an applied pattern or srg registration
		std::string m_SubgraphID;

		// if the edge is spawned by an pattern, store its name
		std::string m_PatternName;

		// we need to store the original name of the edge seperately
		// since the local (original) names may not be unique
		std::string m_LocalName;

		// the set of all subgraphs which use this edge as an input
		// (and need to be deleted should this edge go away)
		std::set< std::string > m_DependantSubgraphIDs;
	};

	class SRGraph
		: public Graph< SRGNodeAttributes, SRGEdgeAttributes >
	{
	public:

		SRGraph()
		{}

		bool hasNode( const std::string& id )
		{
			return ( m_NodeIDMap.find( id ) != m_NodeIDMap.end() );
		}

		NodePtr addNode( UTQLSubgraph::NodePtr utqlNode, const std::string& subgraphID )
		{
			static long idCounter = 1000;

			std::string id = utqlNode->m_QualifiedName;
			if ( id.length() == 0 )
			{
				std::ostringstream oss;
				oss << "tmp" << idCounter;
				id = oss.str();
				idCounter++;
			}

			if ( hasNode( id ) )
				UBITRACK_THROW( "Trying to register duplicate IDs" );

			NodePtr newNode = Graph< SRGNodeAttributes, SRGEdgeAttributes >::addNode( id, SRGNodeAttributes( *utqlNode, subgraphID, utqlNode ) );

			m_NodeIDMap[ id ] = newNode;

			return newNode;
		}

		void removeNode( const std::string& id )
		{
			if ( !hasNode( id ) )
				UBITRACK_THROW( "Trying to erase node with unknown id: " + id );

			Graph< SRGNodeAttributes, SRGEdgeAttributes >::removeNode( id );
			m_NodeIDMap.erase( id );
		}

		void mergeNodeAttributes( NodePtr node, UTQLSubgraph::NodePtr utqlNode, const std::string& subgraphID )
		{
			node->m_SubgraphIDs.insert( subgraphID );

			node->mergeAttributes( *utqlNode );

			for ( std::set< UTQLSubgraph::NodePtr >::iterator it = node->m_NodeRefs.begin();
				  it != node->m_NodeRefs.end(); ++it )
				(*it)->mergeAttributes( *node );

			node->m_NodeRefs.insert( utqlNode );
		}

// 		void mergeNodeAttributes( NodePtr node, UTQLSubgraph::NodePtr utqlNode, std::string subgraphID )
// 		{
// 			node->m_SubgraphIDs.insert( subgraphID );

// 			node->mergeAttributes( *utqlNode );
// 			for ( std::map< std::string, Dataflow::XMLConfigurationElement >::iterator it = utqlNode->m_Values.begin();
// 				  it != utqlNode->m_Values.end(); ++it )
// 			{
// 				node->setAttribute( it->first, it->second );
// 			}

// 			for ( std::map< std::string, Dataflow::XMLConfigurationElement >::iterator it = node->m_Values.begin();
// 				  it != node->m_Values.end(); ++it )
// 			{
// 				for ( std::set< UTQLSubgraph::NodePtr >::iterator j = node->m_NodeRefs.begin();
// 					  j != node->m_NodeRefs.end(); ++j )
// 				{
// 					(*j)->setAttribute( it->first, it->second );
// 				}
// 			}

// 			node->m_NodeRefs.insert( utqlNode );
// 		}


		NodePtr getNode( const std::string& id )
		{
			std::map< std::string, NodePtr >::iterator it = m_NodeIDMap.find( id );
			if ( it == m_NodeIDMap.end() )
				UBITRACK_THROW( "Trying to get node for unregistered id" );

			return it->second;
		}

		std::map< std::string, NodePtr > m_NodeIDMap;
	};

}}

#endif // __Ubitrack_Graph_SRGraph_INCLUDED__
