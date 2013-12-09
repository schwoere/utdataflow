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
 * Implementation of \c TriggerComponent
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#include <algorithm>
#include <utUtil/Exception.h>
#include "TriggerComponent.h"
#include <log4cpp/Category.hh>

// get a logger
static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Dataflow.TriggerComponent" ) );
static log4cpp::Category& eventsLogger( log4cpp::Category::getInstance( "Ubitrack.Events.Dataflow.TriggerComponent" ) );


namespace Ubitrack { namespace Dataflow {

TriggerComponent::TriggerComponent( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph )
	: Component( name )
	, m_bPushOutput( false )
	, m_bHasNewPush( false )
	, m_bExpansionConfigured( false )
{
	// make sure trigger group 0 exists
	m_triggerGroups[ 0 ].reset( new TriggerGroup( this, 0 ) );

	// read push/pull configuration
	generatePushPullMap( subgraph );
}


/** clones ports for space expansion if specified by the UTQL configuration */
void TriggerComponent::generateSpaceExpansionPorts( boost::shared_ptr< Graph::UTQLSubgraph > subgraph )
{
	// create a list of all trigger input ports this component had at the beginning
	typedef std::vector< std::pair< std::string, TriggerInPortBase* > > PortVector;
	PortVector originalPorts;
	std::map< std::string, TriggerInPortBase* > processedPorts;

	for ( TriggerGroupMap::iterator itGroup = m_triggerGroups.begin(); itGroup != m_triggerGroups.end(); itGroup++ )
		for ( TriggerGroup::PortList::iterator itPort = itGroup->second->m_ports.begin(); itPort != itGroup->second->m_ports.end(); itPort++ )
		{
			originalPorts.push_back( std::make_pair( (*itPort)->getName(), *itPort ) );
			processedPorts.insert( std::make_pair( (*itPort)->getName(), *itPort ) );
		}

	// iterate all input ports in the configuration
	for ( Graph::UTQLSubgraph::EdgeMap::iterator itEdge = subgraph->m_Edges.begin(); itEdge != subgraph->m_Edges.end(); itEdge++ )
		if ( itEdge->second->isInput() && processedPorts.find( itEdge->first ) == processedPorts.end() )
		{
			// look if one of the original port names is a prefix of this port name
			for ( PortVector::iterator itOrig = originalPorts.begin(); itOrig != originalPorts.end(); itOrig++ )
				if ( itEdge->first.compare( 0, itOrig->first.size(), itOrig->first ) == 0 )
				{
					std::string sPortSuffix( itEdge->first.substr( itOrig->first.size() ) );
					int iTriggerGroup = std::max( m_triggerGroups.begin()->first, m_triggerGroups.rbegin()->first ) + 1;
					
					// check the trigger group of the master port for other ports with the same suffix that should be in the same trigger group
					TriggerGroup* pOrigGroup = itOrig->second->getTriggerGroup();
					for ( TriggerGroup::PortList::iterator itMasterPort = pOrigGroup->m_ports.begin(); 
						itMasterPort != pOrigGroup->m_ports.end(); itMasterPort++ )
					{
						// for each port in the master port, generate a hypthetical name 
						std::string sHypName( (*itMasterPort)->getName() + sPortSuffix );
						
						// if the sibling port with the same suffix was already processed, add the new port to the same trigger group
						if ( processedPorts.find( sHypName ) != processedPorts.end() )
						{
							iTriggerGroup = processedPorts[ sHypName ]->getTriggerGroup()->m_iGroup;
							break;
						}
					}
					
					// clone the master port
					boost::shared_ptr< TriggerInPortBase > pNewPort( itOrig->second->newSlave( itEdge->first, iTriggerGroup ) );
					m_spaceExpansionPorts.push_back( pNewPort );
					
					processedPorts.insert( std::make_pair( itEdge->first, pNewPort.get() ) );
				}
		}
}


/** called when a push input is received */
void TriggerComponent::triggerIn( TriggerInPortBase* p )
{
	m_bHasNewPush = true;
	
	// if the outport is push, get values from non-expanded ports and then compute result
	if ( m_bPushOutput && m_triggerGroups[ 0 ]->trigger( p->getTimestamp() ) )
	{
		LOG4CPP_TRACE( eventsLogger, getName() << " starting computation on push" );
		compute( p->getTimestamp() );
		m_bHasNewPush = false;
	}

	// do nothing if output is pull
}


// called when a pull output port wants data
void TriggerComponent::triggerOut( Measurement::Timestamp t )
{
	// Pull the default trigger group. This does not pull time-expanded input ports. See also ExpansionInPort.h
	if ( !m_triggerGroups[ 0 ]->trigger( t ) )
		UBITRACK_THROW( getName() + ": No valid measurement for specified timestamp" );

	// if we got here safely, then all ports have valid values for the timestamp in question and we can compute a result
	LOG4CPP_TRACE( eventsLogger, getName() << " starting computation on pull" );
	compute( t );
	m_bHasNewPush = false;
	
	// the result will be returned by the calling port
}


TriggerGroup* TriggerComponent::addTriggerInput( TriggerInPortBase* p, int iTriggerGroup )
{
	MutexType::scoped_lock l( m_componentMutex );
	
	LOG4CPP_DEBUG( logger, "adding trigger input " << p->fullName() << " to trigger group " << iTriggerGroup );

	// add port to trigger group, creating a new group if necessary
	boost::shared_ptr< TriggerGroup >& pGroup( m_triggerGroups[ iTriggerGroup ] );
	if ( !pGroup )
		pGroup.reset( new TriggerGroup( this, iTriggerGroup ) );
	pGroup->addPort( p );
	
	return pGroup.get();
}


void TriggerComponent::addTriggerOutput( bool bPush )
{
	MutexType::scoped_lock l( m_componentMutex );
	m_bPushOutput |= bPush;
	LOG4CPP_DEBUG( logger, m_name << ": push output: " << m_bPushOutput );
}


void TriggerComponent::generatePushPullMap( boost::shared_ptr< Graph::UTQLSubgraph > subgraph )
{
	for ( Graph::UTQLSubgraph::EdgeMap::iterator itEdge = subgraph->m_Edges.begin(); itEdge != subgraph->m_Edges.end(); itEdge++ )
		if ( itEdge->second->hasAttribute( "mode" ) )
			m_pushPullMap[ itEdge->first ] = itEdge->second->getAttributeString( "mode" ) == "push";

	// time expansion can currently only be defined on a per-component basis
	m_bExpansionConfigured = subgraph->m_DataflowAttributes.hasAttribute( "expansion" );
	if ( m_bExpansionConfigured )
		m_bTimeExpansion = subgraph->m_DataflowAttributes.getAttributeString( "expansion" ) == "time";
}


bool TriggerComponent::isPortPush( const std::string& name ) const
{
	PushPullMap::const_iterator it = m_pushPullMap.find( name );
	if ( it == m_pushPullMap.end() )
		UBITRACK_THROW( "No \"mode\" attribute on port " + getName() + ":" + name );
	return it->second;
}


bool TriggerComponent::isTimeExpansion() const
{
	if ( !m_bExpansionConfigured )
		UBITRACK_THROW( "No \"expansion\" attribute in DataflowConfiguration of component " + getName() );

	return m_bTimeExpansion;
}

} } // namespace Ubitrack::Dataflow

