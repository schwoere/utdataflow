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

#ifndef __Ubitrack_Graph_Graph_INCLUDED__
#define __Ubitrack_Graph_Graph_INCLUDED__ __Ubitrack_Graph_Graph_INCLUDED__

#include <map>
#include <list>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <log4cpp/Category.hh>
#include <utUtil/Exception.h>
#include <utUtil/Logging.h>

/**
 * @ingroup srg_algorithms
 * @file
 * Generic Graph data structure
 *
 * A generic data structure for directed graphs. Extensible via
 * edge and node attributes.
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */


namespace Ubitrack { namespace Graph {
	template < class NodeAttributes, class EdgeAttributes > class GraphEdge;

	/**
	 * @ingroup srg_algorithms
	 * Node structure
	 * This class represents a node in the Graph datastructure.
	 * Stores references to its outgoing and incoming edges via
	 * lists of weak_ptrs.
	 */
	template < class NodeAttributes, class EdgeAttributes > class GraphNode
		: public NodeAttributes
	{
	public:
		/** Type of the list of edges stored in each node */
		typedef std::list< boost::weak_ptr< GraphEdge< NodeAttributes, EdgeAttributes > > > EdgeList;

		/**
		 * Standard node constructor
		 * Name and node attributes have to be specified
		 *
		 * @param name Unique name of the node
		 * @param data Node attributes associated with this node
		 */
		GraphNode( const std::string& name, const NodeAttributes& data )
			: NodeAttributes( data )
			, m_Name( name )
		{}

		/** The name of the node */
		std::string m_Name;
		/** The list of all incoming edges */
		EdgeList m_InEdges;
		/** The list of all outgoing edges */
		EdgeList m_OutEdges;
	};

	/**
	 * @ingroup srg_algorithms
	 * Edge structure
	 * This class represents an edge in the Graph datastructure.
	 * It stores references to its source and destination node as weak_ptrs.
	 */
	template < class NodeAttributes, class EdgeAttributes > class GraphEdge
		: public EdgeAttributes
	{
	public:
		/** Type of the node references that are stored */
		typedef boost::weak_ptr< GraphNode< NodeAttributes, EdgeAttributes > > WeakNodePtr;

		/**
		 * Standard edge constructor
		 * The name, the references to the source and destination and the
		 * edge attributes have to be specified.
		 *
		 * @param name Unique name of the edge
		 * @param source Source node reference
		 * @param target Destination node reference
		 * @param data Edge attributes associated with this edge
		 */
		GraphEdge ( const std::string& name, WeakNodePtr source, WeakNodePtr target, const EdgeAttributes& data )
			: EdgeAttributes ( data )
			, m_Name (name)
			, m_Source( source )
			, m_Target( target )
		{}

		/** The name of the edge */
		std::string m_Name;
		/** The reference to the source node */
		WeakNodePtr m_Source;
		/** The reference to the destination node */
		WeakNodePtr m_Target;
	};

	/**
	 * @ingroup srg_algorithms
	 * Graph datastructure
	 * This is the generic graph datastructure.
	 * Both node and edge names must be unique.
	 */
	template < class NodeAttributes, class EdgeAttributes > class Graph
	{
	public:

		/** The specialized Node type for easier access. */
		typedef GraphNode< NodeAttributes, EdgeAttributes > Node;
		/** The specialized Edge type for easier access. */
		typedef GraphEdge< NodeAttributes, EdgeAttributes > Edge;

		/** Shared Pointer to a node */
		typedef boost::shared_ptr< Node > NodePtr;
		/** Shared Pointer to an edge */
		typedef boost::shared_ptr< Edge > EdgePtr;

		/** The type of the node map */
		typedef std::map< std::string, NodePtr > NodeMap;
		/** The type of the edge map */
		typedef std::map< std::string, EdgePtr > EdgeMap;

		/** The type of the node attributes */
		typedef NodeAttributes GraphNodeAttributes;
		/** The type of the edge attributes */
		typedef EdgeAttributes GraphEdgeAttributes;

		/**
		 * Default constructor
		 * Only initializes the logger.
		 */
		Graph()
			: m_logger( log4cpp::Category::getInstance( "Ubitrack.Graph.Graph" ) )
		{}

		/**
		 * Node addition method
		 * This method creates a new node with a given name and data and
		 * adds it to the graph as an isolated node. Returns a shared
		 * pointer to the node.
		 *
		 * @param nodeName the name of the new node. must be unique
		 * @param data the data associated with the new node. May be empty
		 * @return a pointer to the new node.
		 * @throws Ubitrack::Util::Exception if a node with the specified name already exists.
		 */
		NodePtr addNode ( const std::string& nodeName, const NodeAttributes& data = NodeAttributes() )
		{
			if ( m_Nodes.find ( nodeName ) != m_Nodes.end() )
			{
				LOG4CPP_ERROR ( m_logger, "Node is already in Graph: " << nodeName );
				UBITRACK_THROW ( "Trying to insert already present node" );
			}
			NodePtr newNode = NodePtr( new Node ( nodeName, data ) );

			m_Nodes[nodeName] = newNode;
			return newNode;
		}

		/**
		 * Check if a node with a certain name exists
		 * This method checks if a node with a specified name is present
		 * in the graph. Never throws.
		 *
		 * @param nodeName the name of the node.
		 * @return true of the node exists. false otherwise.
		 */
		bool hasNode( const std::string& nodeName ) const
		{
			return ( m_Nodes.find( nodeName ) != m_Nodes.end() );
		}

		/**
		 * Get a node with a certain name
		 * This method returns a shared pointer to a node with a specified name.
		 *
		 * @param nodeName the name of the node.
		 * @return a pointer to the node.
		 * @throws Ubitrack::Util::Exception if no such node exists.
		 */
		NodePtr getNode ( const std::string& nodeName ) const
		{
			if ( m_Nodes.find ( nodeName ) == m_Nodes.end() )
			{
				LOG4CPP_ERROR ( m_logger, "No such node in graph: " << nodeName );
				UBITRACK_THROW ( "Trying to get a non present node " + nodeName );
			}
			return m_Nodes.find(nodeName)->second;
		}

		/**
		 * Delete a node from the graph.
		 * This method removes a specified node from the
		 * graph and disconnects all incident edges.
		 * Note that the node and edge storages are not freed unless
		 * no more pointers exist.
		 *
		 * @param node a pointer to the node which should be removed
		 */
		void removeNode( NodePtr node )
		{
			while ( !node->m_OutEdges.empty() )
			{
				removeEdge( node->m_OutEdges.begin()->lock() );
			}

			while ( !node->m_InEdges.empty() )
			{
				removeEdge( node->m_InEdges.begin()->lock() );
			}

			m_Nodes.erase( node->m_Name );
		}

		/**
		 * Delete a node from the graph.
		 * This method removes the node with a specified name from the
		 * graph and disconnects all incident edges.
		 * Note that the node and edge storages are not freed unless
		 * no more pointers exist.
		 *
		 * @param nodeName the name of the node which should be removed
		 * @throws Ubitrack::Util::Exception if no such node exists.
		 */
		void removeNode( const std::string& nodeName )
		{
			removeNode( getNode( nodeName ) );
		}

		/**
		 * Edge addition method
		 * This method creates a new edge with a given name and data and
		 * adds it to the graph. It is inserted between the two nodes
		 * specified. Returns a shared pointer to the edge.
		 *
		 * @param edgeName the name of the new edge. must be unique
		 * @param source the starting node of the edge.
		 * @param target the destination node of the edge.
		 * @param data the data associated with the new edge. May be empty.
		 * @return a pointer to the new edge.
		 * @throws Ubitrack::Util::Exception if an edge with the specified name already exists.
		 */
		EdgePtr addEdge ( const std::string& edgeName, NodePtr source, NodePtr target, const EdgeAttributes& data = EdgeAttributes() )
		{
			if ( m_Edges.find ( edgeName ) != m_Edges.end() )
			{
				LOG4CPP_WARN ( m_logger, "Edge is already in Graph: " << edgeName );
// 				UBITRACK_THROW ( "Trying to insert already present edge" );
			}
			EdgePtr newEdge = EdgePtr( new Edge( edgeName, source, target, data ) );
			m_Edges.insert( std::pair< std::string, EdgePtr >( edgeName, newEdge ) );

			source->m_OutEdges.push_back( boost::weak_ptr< Edge >( newEdge ) );
			target->m_InEdges.push_back( boost::weak_ptr< Edge >( newEdge ) );

			return newEdge;
		}

		/**
		 * Edge addition method
		 * This method creates a new edge with a given name and data and
		 * adds it to the graph. It is inserted between the two nodes
		 * that are specified by the two node names.
		 * Returns a shared pointer to the edge.
		 *
		 * @param edgeName the name of the new edge. must be unique
		 * @param source the name of the starting node of the edge.
		 * @param target the name of the destination node of the edge.
		 * @param data the data associated with the new edge. May be empty.
		 * @return a pointer to the new edge.
		 * @throws Ubitrack::Util::Exception if an edge with the specified name already exists.
		 */
		EdgePtr addEdge ( const std::string& edgeName, const std::string& source, const std::string& target, const EdgeAttributes& data = EdgeAttributes() )
		{
			NodePtr sourceNode = getNode( source );
			NodePtr targetNode = getNode( target );

			return addEdge( edgeName, sourceNode, targetNode, data );
		}

		/**
		 * Check if an edge with a certain name exists
		 * This method checks if an edge with a specified name is present
		 * in the graph. Never throws.
		 *
		 * @param edgeName the name of the edge.
		 * @return true of the edge exists. false otherwise.
		 */
		bool hasEdge( const std::string& edgeName ) const
		{
			return( m_Edges.find( edgeName ) != m_Edges.end() );
		}

		/**
		 * Get an edge with a certain name
		 * This method returns a shared pointer to an edge with a specified name.
		 *
		 * @param edgeName the name of the edge.
		 * @return a pointer to the edge.
		 * @throws Ubitrack::Util::Exception if no such edge exists.
		 */
		EdgePtr getEdge( const std::string& edgeName ) const
		{
			if ( m_Edges.find ( edgeName ) == m_Edges.end() )
			{
				LOG4CPP_ERROR ( m_logger, "No such edge in graph: " << edgeName );
				UBITRACK_THROW ( "Trying to get a non present edge " + edgeName );
			}
			return m_Edges.find( edgeName )->second;
		}

		/**
		 * Delete an edge from the graph.
		 * This method removes a specified edge from the
		 * graph.
		 * Note that the edge storages is not freed unless
		 * no more pointers in the application exist.
		 *
		 * @param edge a pointer to the edge which should be removed
		 */
		void removeEdge( EdgePtr edge )
		{
			NodePtr source = edge->m_Source.lock();
			NodePtr target = edge->m_Target.lock();

			if (source)
			{
				for (typename Node::EdgeList::iterator it = source->m_OutEdges.begin();
					 it != source->m_OutEdges.end(); ++it)
				{
					if (it->lock() == edge)
					{
						source->m_OutEdges.erase( it );
						break;
					}
				}
			}
			if (target)
			{
				for (typename Node::EdgeList::iterator it = target->m_InEdges.begin();
					 it != target->m_InEdges.end(); ++it)
				{
					if (it->lock() == edge)
					{
						target->m_InEdges.erase( it );
						break;
					}
				}
			}

			source.reset();
			target.reset();
			edge->m_Source.reset();
			edge->m_Target.reset();

			typename std::map< std::string, EdgePtr >::iterator removal = m_Edges.find( edge->m_Name );
			if ( removal == m_Edges.end() )
			{
				LOG4CPP_ERROR( m_logger, "Cannot find edge in edge map: " << edge->m_Name );
			}

			m_Edges.erase( removal );
		}

		/**
		 * Delete an edge from the graph.
		 * This method removes the edge with a specified name from the
		 * graph.
		 * Note that the edge storages is not freed unless
		 * no more pointers in the application exist.
		 *
		 * @param edge a pointer to the edge which should be removed
		 * @throws Ubitrack::Util::Exception if no such edge exists.
		 */
		void removeEdge( const std::string& edgeName )
		{
			removeEdge( getEdge( edgeName ) );
		}

		/**
		 * Get the size of the graph.
		 * This method returns the size (the number of edges) of the graph.
		 *
		 * @return the number of edges in the graph.
		 */
		int size() const
		{
			return m_Edges.size();
		}

		/**
		 * Get the order of the graph.
		 * This method returns the order (the number of nodes) of the graph.
		 *
		 * @return the number of nodes in the graph.
		 */
		int order() const
		{
			return m_Nodes.size();
		}

		/**
		 * Check whether the graph is empty.
		 * This method checks if the graph is the empty graph of its order.
		 * Note that a graph is empty if it contains no edges.
		 *
		 * @return true if the graph contains no edges. false otherwise
		 */
		int empty() const
		{
			return m_Edges.empty();
		}

		/**
		 * Check whether the graph is the null graph.
		 * This method checks if the graph is the null graph.
		 *
		 * @return true if the graph contains no nodes. false otherwise.
		 */
		int null() const
		{
			return m_Nodes.empty();
			// could also check for no edges..
		}

		/** The map of all contained nodes. Also represents (shared) ownership. */
		NodeMap m_Nodes;
		/** The map of all contained edges. Also represents (shared) ownership. */
		EdgeMap m_Edges;

	protected:

		/** Logger for graph */
		log4cpp::Category& m_logger;
	};
}}

#endif // __Ubitrack_Graph_Graph_INCLUDED__
