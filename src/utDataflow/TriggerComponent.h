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
 * Header file for \c TriggerComponent
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */ 

#ifndef __UBITRACK_DATAFLOW_TRIGGERCOMPONENT_H_INCLUDED__
#define __UBITRACK_DATAFLOW_TRIGGERCOMPONENT_H_INCLUDED__

#include <log4cpp/Category.hh>
#include <utDataflow.h>
#include <vector>

#include <boost/thread.hpp>

#include <utMeasurement/Measurement.h>
#include <utUtil/Exception.h>
#include <utGraph/UTQLSubgraph.h>
#include "Component.h"
#include "Port.h"

namespace Ubitrack { namespace Dataflow {

// forward decl
class TriggerInPortBase;
class TriggerGroup;


/**
 * @ingroup dataflow_framework
 * A component that provides some common synchronization logic for push/pull and space/time expansion.
 *
 * This is achieved in combination with \c TriggerInPort, \c TriggerOutPort and \c ExpansionInPort.
 * To implement correctly synchronized dataflow components, use these ports and implement the 
 * \c compute() method.
 */
class UTDATAFLOW_EXPORT TriggerComponent
	: public Component
{
public:
	/** constructor */
	TriggerComponent( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph );

	/** must be called to generate space expansion ports, after the base ports are created */
	void generateSpaceExpansionPorts( boost::shared_ptr< Graph::UTQLSubgraph > subgraph );
	
	/** 
	 * Implemented by derived components and performs the actual computation. 
	 * Fetches data from input ports, does the computation and sends results to output ports.
	 * Throws when no computation is possible.
	 */
	virtual void compute( Measurement::Timestamp ) = 0;

	/** called when a push input is received */
	void triggerIn( TriggerInPortBase* p );

	
	/** called when a pull output port wants data */
	void triggerOut( Measurement::Timestamp t );

	/** register a triggered input port */
	TriggerGroup* addTriggerInput( TriggerInPortBase* p, int triggerGroup );
	
	/** register a time expanded triggered input port */
	void addTimeExpandedTriggerInput( TriggerInPortBase* p );

	/** register a triggered output port */
	void addTriggerOutput( bool bPush );

	/** did the component receive new push inputs since the last compute()? */
	bool hasNewPush() const
	{ return m_bHasNewPush; }

	/** retrieve push/pull configuration of a port */
	bool isPortPush( const std::string& name ) const;
	
	/** is this component a time expansion ? */
	bool isTimeExpansion() const;

	
private:
	/** read and store push/pull configuration from UTQL */
	void generatePushPullMap( boost::shared_ptr< Graph::UTQLSubgraph > subgraph );

	// is the output of this component push?
	bool m_bPushOutput;
	
	// did the component receive new push inputs since the last compute()?
	bool m_bHasNewPush;
	
	// map of trigger groups
	typedef std::map< int, boost::shared_ptr< TriggerGroup > > TriggerGroupMap;
	TriggerGroupMap m_triggerGroups;
	
	/** list of all space expansion ports to maintain ownership */
	std::vector< boost::shared_ptr< TriggerInPortBase > > m_spaceExpansionPorts;

	/** 
	 * store UTQL push/pull configuration for all ports (read from the "mode" attribute).
	 * true means "push".
	 */
	typedef std::map< std::string, bool > PushPullMap;
	PushPullMap m_pushPullMap;

	/**
	 * Is this instance a time expansion?
	 * Currently, this can only be defined on a per-component basis using the data flow attribute 
	 * "expansion=time", rather than per trigger-group.
	 */
	bool m_bTimeExpansion;

	/** is the expansion actually configured */
	bool m_bExpansionConfigured;
};


/**
 * Base class of all TriggerPorts.
 *
 * Provides common attributes and some virtual methods.
 */
class TriggerInPortBase
	: public Port
{
public:
	/** 
	 * Constructor.
	 *
	 * @param sName Name of the port. Used for network instantiation.
	 * @param rParent Reference to owning component where this port will be registered.
	 * @param triggerGroup id of the trigger group for this port
	 */
	TriggerInPortBase( const std::string& sName, TriggerComponent& rParent, int triggerGroup )
		: Port( sName, rParent )
		, m_bPush( rParent.isPortPush( sName ) )
		, m_timestamp( 0 )
	{
		m_pTriggerGroup = rParent.addTriggerInput( this, triggerGroup );
	}

	/** clone this port */
	virtual boost::shared_ptr< TriggerInPortBase > newSlave( const std::string& name, int triggerGroup )
	{ UBITRACK_THROW( fullName() + ": only expansion ports can be cloned" ); return boost::shared_ptr< TriggerInPortBase >(); }
	
	/** 
	 * Pull a measurement from connected partner and store it in internal storage. 
	 * Throw if unsuccessful.
	 */
	virtual void pull( Measurement::Timestamp t )
	{}

	/**
	 * If the port is time expanded, add the stored measurement to the list.
	 *
	 * This method is called when all time expanded inputs have a valid single 
	 * measurement and thus, a correspondence can be generated and added to the 
	 * list.
	 */
	virtual void storeMeasurement() 
	{}

	/** if the port is push, return true if there are events waiting for this port */
	virtual bool eventsWaiting()
	{ return false; }
	
	/** returns true if the port is push, false if pull */
	bool isPush() const
	{ return m_bPush; }
	
	/** returns the timestamp of the stored measurement */
	Measurement::Timestamp getTimestamp() const
	{ return m_timestamp; }
	
	/** returns the trigger group this port belongs to */
	TriggerGroup* getTriggerGroup()
	{ return m_pTriggerGroup; }

protected:
	/** True if push, false if pull */
	bool m_bPush;
	
	/** timestamp of the stored measurement */
	Measurement::Timestamp m_timestamp;	
	
	/** pointer to the trigger group this port belongs to */
	TriggerGroup* m_pTriggerGroup;
};


/**
 * Represents a group of push and pull ports that require synchronized data
 */
class TriggerGroup
{
public:
	/** constructor */
	TriggerGroup( TriggerComponent* pComponent, int iGroup )
		: m_pComponent( pComponent )
		, m_iGroup( iGroup )
		, m_eventsLogger( log4cpp::Category::getInstance( "Ubitrack.Events.Dataflow.TriggerComponent" ) )
	{}
	
	/** add a port to the trigger group */
	void addPort( TriggerInPortBase* pPort )
	{
		LOG4CPP_DEBUG( m_eventsLogger, "adding trigger input " << pPort->fullName() << " to trigger group " << m_iGroup << " in component " << m_pComponent );
		
		m_ports.push_back( pPort );
	}
	
	/** 
	 * Trigger all ports belonging to the group, i.e. check the timestamp of push ports and pull pull ports.
	 * @return true if successfull
	 */
	bool trigger( Measurement::Timestamp t )
	{
		LOG4CPP_TRACE( m_eventsLogger, m_ports.size() << " ports to be triggered in group " << m_iGroup << " in component " << m_pComponent);

		for ( unsigned i = 0; i < m_ports.size(); i++ )
			if ( m_ports[ i ]->isPush() )
			{
				LOG4CPP_TRACE( m_eventsLogger, "port " << m_ports[i] << " is push");
				if ( m_ports[ i ]->getTimestamp() != t )
				{
					LOG4CPP_DEBUG( m_eventsLogger, m_ports[ i ]->getComponent().getName() << " not computing: timestamps do not match on push input: "  
						<< m_ports[ i ]->getName() );
					return false;
				}
			}
			else
			{
				LOG4CPP_TRACE( m_eventsLogger, "port " << m_ports[i] << " is pull");
				try
				{ 
					m_ports[ i ]->pull( t ); 
				}
				catch ( const Util::Exception& e )
				{
					LOG4CPP_DEBUG( m_eventsLogger, m_ports[ i ]->getComponent().getName() << " not computing: error on pull input: "  
						<< m_ports[ i ]->getName() << ", reason: " << e );
					return false;
				}
				catch ( ... )
				{
					LOG4CPP_DEBUG( m_eventsLogger, m_ports[ i ]->getComponent().getName() << " not computing: error on pull input: "  
						<< m_ports[ i ]->getName() );
					return false;
				}
			}
			
		return true;
	}

	/** makes the group store measurements for space/time expansion */
	void storeMeasurements()
	{
		LOG4CPP_TRACE( m_eventsLogger, "storing measurements for " << m_ports.size() << " ports" );
		
		for ( unsigned i = 0; i < m_ports.size(); i++ )
			m_ports[ i ]->storeMeasurement();
	}
	
	/** pointer to parent component */
	TriggerComponent* m_pComponent;
	
	/** trigger group id */
	int m_iGroup;
	
	typedef std::vector< TriggerInPortBase* > PortList;
	
	/** list of ports belonging to this group */
	PortList m_ports;

protected:	
	/** logger */
	log4cpp::Category& m_eventsLogger; 
};



} } // namespace Ubitrack::Dataflow

#endif
