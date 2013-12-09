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
 * Dataflow Network implementations
 * @author Manuel Huber <huberma@in.tum.de>
 */


#include "DataflowNetwork.h"
#include "Component.h"
#include "ComponentFactory.h"
#include "Port.h"
#include <utGraph/UTQLDocument.h>

#include <log4cpp/Category.hh>

namespace Ubitrack { namespace Dataflow {

	static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Dataflow.DataflowNetwork" ) );

	// Constructor just stores the factory
	DataflowNetwork::DataflowNetwork (ComponentFactory &factory)
		:m_componentFactory (factory)
	{}


	DataflowNetwork::~DataflowNetwork ()
	{
		// destroy all components in the network
		while (!m_componentIDMap.empty ())
			dropComponent (m_componentIDMap.begin()->first);

		LOG4CPP_DEBUG( logger, "Destroyed DataflowNetwork" );
	}

	void DataflowNetwork::processUTQLResponse( boost::shared_ptr< Graph::UTQLDocument > doc )
	{
		std::map< boost::shared_ptr< Graph::UTQLSubgraph >, boost::shared_ptr< Component > > subgraphComponentMap;

		for ( Graph::UTQLDocument::SubgraphList::iterator it = doc->m_Subgraphs.begin();
			  it != doc->m_Subgraphs.end(); ++it )
		{
			boost::shared_ptr< Graph::UTQLSubgraph > subgraph = *it;
			LOG4CPP_DEBUG( logger, "Considering component.. " << subgraph->m_ID );

			// is subgraph already present in Dataflow?
			if ( ( subgraph->m_ID.length() != 0 ) &&
				 ( m_componentIDMap.find( subgraph->m_ID ) != m_componentIDMap.end() ) )
			{

				LOG4CPP_TRACE( logger, "Already known. Disconecting.." );
				// disconnect component
				// should be reasonable since connections are stateless.. for now

				// boost::shared_ptr< Component > comp = m_componentIDMap[ subgraph->m_ID ];
				disconnectComponent( subgraph->m_ID );

				// is the new subgraph empty? -> delete
				if ( subgraph->null() )
				{
					LOG4CPP_INFO( logger, subgraph->m_ID << " replaced with empty subgraph. Deleting.." );
					dropComponent( subgraph->m_ID );
				}
				else
				{
					LOG4CPP_INFO( logger, subgraph->m_ID << " replaced with non-empty subgraph. Reconfiguring.." );
					// not empty subgraph..
					// connections will be reestablished later..

					// TODO: check if graph changed or if dataflow configuration is different..
					if ( !subgraph->m_DataflowConfiguration.isEmpty() )
					{
						LOG4CPP_WARN( logger, "Cannot handle reconfigurations at the moment. We hope nothing changed and carry on.." );
					}

					// nothing else to do. connections will be made later
				}
			}
			else
			{
				// subgraph is unknown: create it

				LOG4CPP_TRACE( logger, "Unknown. Creating.." );
				if ( !subgraph->m_DataflowConfiguration.isEmpty() )
				{
					createComponent( subgraph );

					LOG4CPP_DEBUG( logger, (std::string)"Created component: " + (*it)->m_ID
									+ " [" + (*it)->m_Name + "]" );
				}
			}
		}


		LOG4CPP_INFO( logger, "Making connections" );

		for ( Graph::UTQLDocument::SubgraphList::iterator it = doc->m_Subgraphs.begin();
			  it != doc->m_Subgraphs.end(); ++it )
		{
			boost::shared_ptr< Graph::UTQLSubgraph > subgraph = *it;

			// TODO: does this break anything?
			// skip subgraphs without dataflow
// 			if ( subgraph->m_DataflowConfiguration.isEmpty() )
// 			{
// 				continue;
// 			}


			for (std::map< std::string, Graph::UTQLSubgraph::EdgePtr >::iterator it = subgraph->m_Edges.begin();
				 it != subgraph->m_Edges.end(); ++it )
			{

				// XXX: TODO: sucks. this should be handeled by the InOutAttributeIterator
				Graph::UTQLSubgraph::EdgePtr edge = it->second;
				if ( !edge->isInput() )
					continue;
				
				// ignore edges on other clients
				if ( edge->hasAttribute( "remotePatternID" ) )
					continue;

				// warn on empty edge references
				if ( edge->m_EdgeReference.empty() )
				{
					LOG4CPP_NOTICE ( logger, "Warning: " << subgraph->m_Name << " has dangling edge (missing graph-ref and/or edge-ref)" << edge->m_Name );
					continue;
				}

				// do connecting components have a data flow configuration (might be config edge)
				std::string otherSubgraphId = edge->m_EdgeReference.getSubgraphId();
				bool bOtherIsDF = m_componentIDMap.find( otherSubgraphId ) != m_componentIDMap.end() || 
					( doc->hasSubgraphById( otherSubgraphId ) && !doc->getSubgraphById( otherSubgraphId )->m_DataflowConfiguration.isEmpty() );

				if ( !subgraph->m_DataflowConfiguration.isEmpty() && bOtherIsDF )
				{
					LOG4CPP_DEBUG( logger, "Connecting: " << subgraph->m_ID << " / " << edge->m_Name << " to " << otherSubgraphId
									 << " [" << edge->m_EdgeReference.getEdgeName() << "]" );

					// srcName, srcPort, dstName, dstPort
					connectComponents( otherSubgraphId, edge->m_EdgeReference.getEdgeName(),
									   subgraph->m_ID, edge->m_Name );
				}
			}
		}

		// compute event priorities
		assignEventPriorities();
	}

	boost::shared_ptr< Component > DataflowNetwork::createComponent( boost::shared_ptr< Graph::UTQLSubgraph > subgraph )
	{
		// check if config is valid for ubitrack lib
		Graph::AttributeValue& config = subgraph->m_DataflowConfiguration;
		const TiXmlElement* xmlElementConfig = config.getXML();

		if ( !xmlElementConfig )
		{
			UBITRACK_THROW( (std::string)"Non-XML Dataflow Configuration: " + config.getText() );
		}

		const TiXmlElement* xmlElementUbitrackLib = xmlElementConfig->FirstChildElement( "UbitrackLib" );

		if ( !xmlElementUbitrackLib )
		{
			UBITRACK_THROW( (std::string)"Dataflow Configuration does not specify ubitrackLib config" + config.getText() );
		}

		// extract component class
		std::string ubitrackLibClass;
		if ( !xmlElementUbitrackLib->Attribute( "class" ) )
		{
			UBITRACK_THROW( (std::string)"Dataflow Configuration does not specify ubitrackLib class" + config.getText() );
		}
		ubitrackLibClass = xmlElementUbitrackLib->Attribute( "class" );
		subgraph->m_DataflowClass = ubitrackLibClass;

		// get name
		std::string componentName = subgraph->m_ID;

		LOG4CPP_INFO( logger, "Creating: " << componentName << " [" << ubitrackLibClass << "]" );

		// check if component name is already used and throw if so
		if (m_componentIDMap.find (componentName) != m_componentIDMap.end ())
		{
			LOG4CPP_WARN( logger, "duplicate component name: " << componentName );
			UBITRACK_THROW( (std::string)"duplicate component name: " + componentName );
		}

		// get a new component from the factory and stroe it in a smart pointer
		LOG4CPP_DEBUG( logger, "Creating component: " << componentName );
		boost::shared_ptr< Component > comp (m_componentFactory.createComponent (ubitrackLibClass, componentName, subgraph));
		LOG4CPP_DEBUG( logger, "Created component: " << componentName );

		// check if an already instantiated component was returned by the module mechanism.
		// this happens e.g. if two marker tracker components on the same camera with identical IDs are instantiated.
		if ( comp->getName() != componentName && m_componentIDMap.find( comp->getName() ) != m_componentIDMap.end() )
			LOG4CPP_WARN( logger, "WARNING: Creating component " << componentName << " returned existing component " << comp->getName()
				<< ". This may cause problems. Check your configuration for duplicate marker IDs and such." );
		
		// register component under name
		m_componentIDMap[componentName] = comp;

		// return pointer
		return comp;
	}

	void DataflowNetwork::dropComponent (const std::string name)
	{
		ComponentMap::iterator it = m_componentIDMap.find (name);
		if (it == m_componentIDMap.end ())
		{
			UBITRACK_THROW( "component " + name + " not found" );
		}
		LOG4CPP_DEBUG( logger, "Dropping component: " << name );

		disconnectComponent (name);
		m_componentIDMap.erase (name);

	}

	void DataflowNetwork::connectComponents ( const DataflowNetworkConnection& connection )
	{
		if ( m_allConnections.find ( connection ) != m_allConnections.end() )
		{
			UBITRACK_THROW( (std::string)"ports already connected: "
							+ connection.m_source.m_componentName + ":"
							+ connection.m_source.m_portName + " -> "
							+ connection.m_destination.m_componentName + ":"
							+ connection.m_destination.m_portName + "" );
		}

		Port *srcPort;
		Port *dstPort;
		boost::tie (srcPort, dstPort) = getPortPair ( connection );

		// we need to connect in both directions since we do not
		// know which protocol (push/pull) is involved
		try
		{
			srcPort->connect (*dstPort);			
			try
			{
				dstPort->connect (*srcPort);
			}
			catch ( ... )
			{
				srcPort->disconnect( *dstPort );
				throw;
			}
		}
		catch ( std::runtime_error& e )
		{
			LOG4CPP_ERROR( logger, "Error connecting " + srcPort->fullName() + " to " + dstPort->fullName() + ": " + e.what() );
			UBITRACK_THROW( "Error connecting " + srcPort->fullName() + " to " + dstPort->fullName() + ": " + e.what() );
		}

		LOG4CPP_DEBUG( logger, "Connected: " << srcPort->fullName() << " -> " << dstPort->fullName() );

		m_allConnections.insert ( connection );
		m_inConnectionMap[connection.m_destination.m_componentName].insert ( connection );
		m_outConnectionMap[connection.m_source.m_componentName].insert ( connection );
	}


	void DataflowNetwork::connectComponents (std::string srcName,
											 std::string srcPortName,
											 std::string dstName,
											 std::string dstPortName)
	{
		DataflowNetworkSide source ( srcName, srcPortName );
		DataflowNetworkSide destination ( dstName, dstPortName );
		DataflowNetworkConnection connection ( source, destination );

		connectComponents ( connection );
	}

	void DataflowNetwork::disconnectComponents ( const DataflowNetworkConnection& connection )
	{
		if ( m_allConnections.find ( connection ) == m_allConnections.end() )
		{
			UBITRACK_THROW( (std::string)"ports not connected: "
							+ connection.m_source.m_componentName + " ("
							+ connection.m_source.m_portName + ") -> "
							+ connection.m_destination.m_componentName + " ("
							+ connection.m_destination.m_portName + ")" );
		}

		Port *srcPort;
		Port *dstPort;
		boost::tie (srcPort, dstPort) = getPortPair ( connection );

		// disconnect
		dstPort->disconnect (*srcPort);
		srcPort->disconnect (*dstPort);

		LOG4CPP_DEBUG( logger,
					   "Disconnected: "
					   << connection.m_source.m_componentName << " ("
					   << connection.m_source.m_portName << ") -> "
					   << connection.m_destination.m_componentName << " ("
					   << connection.m_destination.m_portName << ")" );

		m_outConnectionMap[connection.m_source.m_componentName].erase ( connection );
		m_inConnectionMap[connection.m_destination.m_componentName].erase ( connection );
		m_allConnections.erase ( connection );
	}


	void DataflowNetwork::disconnectComponents (std::string srcName,
												std::string srcPortName,
												std::string dstName,
												std::string dstPortName)
	{
		DataflowNetworkSide source ( srcName, srcPortName );
		DataflowNetworkSide destination ( dstName, dstPortName );
		DataflowNetworkConnection connection ( source, destination );

		disconnectComponents ( connection );
	}

	void DataflowNetwork::disconnectComponent (const std::string name)
	{
		ConnectionMap::iterator mapEntry;
		LOG4CPP_DEBUG( logger, "Isolating: " << name );


		// disconnect in ports
		if ( m_inConnectionMap.find ( name ) != m_inConnectionMap.end() )
		{
			std::set< DataflowNetworkConnection >& connections = m_inConnectionMap[ name ];

			while ( !connections.empty() )
			{
				DataflowNetworkConnection connection = *(connections.begin());
				LOG4CPP_TRACE ( logger, "Disconecting In-Port: "
								<< connection.m_source.m_componentName << " ("
								<< connection.m_source.m_portName << ") -> "
								<< connection.m_destination.m_componentName << " ("
								<< connection.m_destination.m_portName << ")" );
				disconnectComponents ( connection );
			}
			m_inConnectionMap.erase ( name );
		}

		// disconnect out ports
		if ( m_outConnectionMap.find ( name ) != m_outConnectionMap.end() )
		{
			std::set< DataflowNetworkConnection >& connections = m_outConnectionMap[ name ];

			while ( !connections.empty() )
			{
				DataflowNetworkConnection connection = *(connections.begin());
				LOG4CPP_TRACE ( logger, "Disconecting Out-Port: "
								<< connection.m_source.m_componentName << " ("
								<< connection.m_source.m_portName << ") -> "
								<< connection.m_destination.m_componentName << " ("
								<< connection.m_destination.m_portName << ")" );
				disconnectComponents ( connection );
			}
			m_outConnectionMap.erase ( name );
		}
		LOG4CPP_TRACE ( logger, "Done isolating" );
	}

	boost::tuple < Port *, Port * >DataflowNetwork::getPortPair ( const DataflowNetworkConnection& connection )
	{
		// get components involved by name
		// throws if not available
		boost::shared_ptr< Component > src = componentByName < Component > (connection.m_source.m_componentName);
		boost::shared_ptr< Component > dst = componentByName < Component > (connection.m_destination.m_componentName);

		// get ports involved by name
		// throws if not available
		Port *srcPort = &src->getPortByName (connection.m_source.m_portName);
		Port *dstPort = &dst->getPortByName (connection.m_destination.m_portName);

		return boost::make_tuple (srcPort, dstPort);
	}

	void DataflowNetwork::startStopNetwork( bool start )
	{
		LOG4CPP_INFO( logger, "Signaling components to " << ( start ? "start" : "stop" ) );
		for ( ComponentMap::iterator it = m_componentIDMap.begin(); it != m_componentIDMap.end(); it++ )
		{
			if ( start )
			{
				LOG4CPP_DEBUG( logger, "Signaling " << it->second->getName() << " to start" );
				it->second->start();
			}
			else
			{
				LOG4CPP_DEBUG( logger, "Signaling: " << it->second->getName() << " to stop" );
				it->second->stop();
			}
		}
		if ( start )
		{
			LOG4CPP_INFO( logger, "Dataflow started" );
		}
		else
		{
			LOG4CPP_INFO( logger, "Dataflow terminated" );
		}
	}

	void DataflowNetwork::startNetwork()
	{
		startStopNetwork( true );
	}

	void DataflowNetwork::stopNetwork()
	{
		startStopNetwork( false );
	}

	/** \internal used internally by assignEventPriorities. gcc does not like this declaration inside the function. */
	struct AEPSearchInfo
	{
		int priority;
		boost::shared_ptr< Component > pComponent;

		AEPSearchInfo( int p, boost::shared_ptr< Component > c )
			: priority( p ), pComponent( c )
		{}
	};

	// FIXME: This is a hack, as priorities are now counted downwards, starting at the sinks.
	// This is necessary so that all events to sinks which belong to the same module are delivered
	// simultaneously, as the DFN does not know about module ownership.
	// The value should stay below 1000 so that the sorting order in the event queue is not disturbed.
	// It simply looks at (timestamp + priority) and relies on timestamps being always more coarse
	// than their storage data type, so that the least-significant digits are 0.
	// The best solution, however, is to start counting at 0 again and have the event queue sorting consider
	// timestamp and priority separately.
	#define DFN_MAX_PATHLENGTH 255

	void DataflowNetwork::assignEventPriorities()
	{
		LOG4CPP_INFO( logger, "assigning event priorities" );

		// performs a depth-first search from each sink to find for each component the longest path to
		// a source. This longest path length is used to determine the priority.

		// clear all event priorities
		for ( ComponentMap::iterator it = m_componentIDMap.begin(); it != m_componentIDMap.end(); it++ )
			it->second->setEventPriority( DFN_MAX_PATHLENGTH );

		// find all sinks in the data flow network
		for ( ComponentMap::iterator it = m_componentIDMap.begin(); it != m_componentIDMap.end(); it++ )
			if ( m_outConnectionMap[ it->first ].empty() )
			{
				LOG4CPP_TRACE( logger, it->first << " is a sink" );

				// perform the depth-first search, avoiding circles
				std::vector< AEPSearchInfo > search;
				std::set< Component* > visiting; // for loop detection

				search.push_back( AEPSearchInfo( DFN_MAX_PATHLENGTH, it->second ) );
				while ( !search.empty() )
				{
					// a negative priority is used as a marker to indicate that the component should be
					// removed from the visiting set
					if ( search.back().priority < 0 )
					{
						LOG4CPP_TRACE( logger, "  dropping " << search.back().pComponent->getName() );
						visiting.erase( search.back().pComponent.get() );
						search.pop_back();
					}
					else
					{
						// set priority
						int prio = search.back().priority;
						LOG4CPP_TRACE( logger, "  visiting " << search.back().pComponent->getName() << ", current priority " << prio );
						if ( search.back().pComponent->getEventPriority() > prio )
							search.back().pComponent->setEventPriority( prio );
						prio--;

						// add to visiting list and change priority to negative value
						search.back().priority = -1;
						visiting.insert( search.back().pComponent.get() );

						// add all ancestors to the list
						const ConnectionSet& inConns( m_inConnectionMap[ search.back().pComponent->getName() ] );
						for ( ConnectionSet::const_iterator itIn = inConns.begin(); itIn != inConns.end(); itIn++ )
						{
							boost::shared_ptr< Component > pComponent =
								m_componentIDMap[ itIn->m_source.m_componentName ];

							LOG4CPP_TRACE( logger, "  queueing " << pComponent->getName() );

							if ( visiting.find( pComponent.get() ) == visiting.end() )
								search.push_back( AEPSearchInfo( prio, pComponent ) );
						}
					}
				}
			}

		// debug output
		if ( logger.isDebugEnabled() )
			for ( ComponentMap::iterator it = m_componentIDMap.begin(); it != m_componentIDMap.end(); it++ )
				LOG4CPP_DEBUG( logger, it->first << " has priority " << it->second->getEventPriority() );
	}

} } // namespace Ubitrack::Dataflow
