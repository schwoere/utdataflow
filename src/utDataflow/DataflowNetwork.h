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
 * Dataflow Network
 *
 * This file contains the definition of the dataflow network class
 * which handles basic management of dataflow networks and the components
 * contained in the network. This class does not handle passing of measurements
 * or the general simulation of networks.
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */


#ifndef __Ubitrack_Dataflow_DataflowNetwork_INCLUDED__
#define __Ubitrack_Dataflow_DataflowNetwork_INCLUDED__ __Ubitrack_Dataflow_DataflowNetwork_INCLUDED__

#include <map>
#include <set>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/utility.hpp>

#include <utDataflow.h>
#include <utUtil/Exception.h>
#include <utDataflow/Component.h>

// forward decls
namespace Ubitrack {
	namespace Dataflow {
		class ComponentFactory;
		class Port;
	}
	namespace Graph {
		class UTQLDocument;
		class UTQLSubgraph;
	}
}

namespace Ubitrack { namespace Dataflow {

	/**
	 * @ingroup dataflow_framework
	 * Small struct to store half a dataflow network connection
	 *
	 * This is just a small container structure to store identification
	 * of either the sender or the receiver side of a dataflow
	 * connection and to make this identification comparable in
	 * STL containers.
	 * A half connection consists of two strings (component and
	 * port name).
	 */
	struct UTDATAFLOW_EXPORT DataflowNetworkSide
	{
		/// The component name
		std::string m_componentName;
		/// The port name
		std::string m_portName;

		/**
		 * Standard constructor to make building structures more easy
		 *
		 * @param component the name of the component
		 * @param port the name of the port
		 */
		DataflowNetworkSide ( std::string component, std::string port)
			: m_componentName ( component ), m_portName ( port )
		{}

		/**
		 * Simple comparison operator
		 *
		 * Comparison operator to make this structure usable in
		 * ordered STL data structures.
		 * This comparison operator extends the order on strings to
		 * DataflowNetworkSide structure. The component name is
		 * considered first, the port name second if the component
		 * names are equal.
		 */
		bool operator< (const DataflowNetworkSide& x) const
		{
			if ( ( m_componentName < x.m_componentName ) ||
				 ( x.m_componentName < m_componentName ) )
			{
				return m_componentName < x.m_componentName;
			}
			else
			{
				return m_portName < x.m_portName;
			}
		}
	};


	/**
	 * @ingroup dataflow_framework
	 * Small struct to store a full dataflow network connection
	 *
	 * This is just a small container structure to store identification
	 * of a full established dataflow network connection and make this
	 * identification comparable in STL containers.
	 * A full connection consists of two half connections (source and
	 * destination).
	 */
	struct UTDATAFLOW_EXPORT DataflowNetworkConnection
	{
		/// The source half connection
		DataflowNetworkSide m_source;
		/// The destination half connection
		DataflowNetworkSide m_destination;


		/**
		 * Standard constructor to make building structures more easy
		 *
		 * @param s the source half connection
		 * @param d the destination half connection
		 */
		DataflowNetworkConnection ( DataflowNetworkSide s, DataflowNetworkSide d )
			: m_source ( s ), m_destination ( d )
		{}

		/**
		 * Simple comparison operator
		 *
		 * Comparison operator to make this structure usable in
		 * ordered STL data structures.
		 * This comparison operator extends the order on half connections to
		 * DataflowNetworkConnection structure. The source side is
		 * considered first, the destination side second if the source sides
		 * are equal.
		 */
		bool operator< (const DataflowNetworkConnection& x) const
		{
			if ( ( m_source < x.m_source ) ||
				 ( x.m_source < m_source ) )
			{
				return m_source < x.m_source;
			}
			else
			{
				return m_destination < x.m_destination;
			}
		}
	};


	/**
	 * @ingroup dataflow_framework
	 * Representation of a dataflow network
	 *
	 * This class represents a directed graph of components as a
	 * dataflow network. Basic management such as connect/disconnect
	 * and creation/destruction as well as traversal are handeled by
	 * this class.
	 */
	class UTDATAFLOW_EXPORT DataflowNetwork
		: private boost::noncopyable
	{
	public:
		/**
		 * Dataflow Network constructor
		 *
		 * This constructor initializates a new empty dataflow network.
		 * A reference to the ComponentFactory which is used to create
		 * new components has to be supplied.
		 * @param factory the component factory to be used
		 */
		DataflowNetwork(ComponentFactory& factory);

		/**
		 * Dataflow Network destructor
		 *
		 * This destructor cleans up an existing dataflow network.
		 * It drops every component registered in the network once.
		 */
		~DataflowNetwork();

		/**
		 * Process an UTQL response and modify the dataflow
		 * network accordingly.
		 *
		 * This method modifies the dataflow network according
		 * to the data contained in an UTQL document.
		 *
		 * @par
		 * For every subgraph with nonempty dataflow configuration
		 * a dataflow component will be created and connected
		 * as specified by its edge references.
		 * Already existing subgraphs are either deleted if the
		 * new subgraph is empty or reconnected and reconfigured
		 * if the new subgraph differs from the already existing
		 * one.
		 *
		 * @par
		 * At the moment reconfiguration of dataflow configuration
		 * of existing subgraphs is so far not supported.
		 *
		 * @param doc the UTQL document
		 */
		void processUTQLResponse( boost::shared_ptr< Graph::UTQLDocument > doc );

		/**
		 * Create a Dataflow Component
		 *
		 * This method creates a new dataflow component in the
		 * dataflow network. It handles basic bookkeeping to guarantee
		 * smooth destruction and disconnection of components and the
		 * network.
		 * @param className the name of the dataflow component class to be instantiated
		 * @param name the name of the new component (has to be unique)
		 * @param subgraph the UTQL subgraph that specifies the dataflow component
		 * @return reference counted pointer to the new component
		 * @throws Ubitrack::Util::Exception if the name is already used in the network
		 */
		boost::shared_ptr< Component > createComponent( boost::shared_ptr< Graph::UTQLSubgraph > subgraph );

		/**
		 * Drop a component from the dataflow network
		 *
		 * This method drops a component from the dataflow network
		 * and removes all pointers to the component from all
		 * tables stored by the dataflow network. If there are
		 * no other references to that component elsewhere, the
		 * component is destroyed (by reference counting).
		 * The component is first disconnected by disconnectComponent
		 * @param name the name of the component which should be dropped
		 * @throws Ubitrack::Util::Exception if there is no component with that name
		 */
		void dropComponent ( const std::string name );


		// Ergonomisch fantastisch

		/**
		 * Connect two components in the dataflow network
		 *
		 * This method connects two dataflow components as specified by
		 * component and port names. Does not throw on its own, but uses
		 * getPortPair for example.
		 * @param srcName the name of the component with the "output" port
		 * @param srcPortName the name of the port at the source component
		 * @param dstName the name of the component with the "inpout" port
		 * @param dstPortName the name of the port at the destination component
		 */
		void connectComponents( std::string srcName, std::string srcPortName, std::string dstName, std::string dstPortName );

		/**
		 * Connect two components in the dataflow network
		 *
		 * This method connects two dataflow components as specified by
		 * component and port names. Does not throw on its own, but uses
		 * getPortPair for example.
		 * @param connection a dataflow connection (4 tuple)
		 */
		void connectComponents( const DataflowNetworkConnection& connection );

		/**
		 * Disconnect two components in the dataflow network.
		 *
		 * This mathod disconnects one connection between two components
		 * in the dataflow network. This connection is specified by
		 * the four names of the source and destination components and ports.
		 * @param srcName the name of the component with the "output" port
		 * @param srcPortName the name of the port at the source component
		 * @param dstName the name of the component with the "inpout" port
		 * @param dstPortName the name of the port at the destination component
		 */
		void disconnectComponents( std::string srcName, std::string srcPortName, std::string dstName, std::string dstPortName );

		/**
		 * Disconnect two components in the dataflow network.
		 *
		 * This mathod disconnects one connection between two components
		 * in the dataflow network. This connection is specified by
		 * the four names of the source and destination components and ports.
		 * @param connection a dataflow connection (4 tuple)
		 */
		void disconnectComponents( const DataflowNetworkConnection& connection );

		/**
		 * Disconnect a component in the dataflow network.
		 *
		 * This method disconnects all connections going from or to a
		 * dataflow component which is identified by its name. This
		 * is usually done in preparation to drop the component from the
		 * network.
		 * @param name the name of the component to isolate
		 */
		void disconnectComponent( const std::string name );

		/**
		 * Starts or stops the dataflow network
		 *
		 * This method starts or stops all components in the
		 * dataflow network depending on the flag.
		 *
		 * @param start signals if the network should be started (true) or stopped
		 */
		void startStopNetwork( bool start );

		/**
		 * Starts the dataflow network
		 *
		 * This method starts all components in the
		 * dataflow network.
		 */
		void startNetwork();

		/**
		 * Stops the dataflow network
		 *
		 * This method stops all components in the
		 * dataflow network.
		 */
		void stopNetwork();

		/**
		 * Get a component from the network by name
		 *
		 * This method returns a shared pointer to a component in the
		 * dataflow network dynamic_pointer_cast'ed to type T
		 * @param name the name of the component which is desired
		 * @return a shared pointer to the component
		 * @throws Ubitrack::Util::Exception if no such component exists
		 */
		template <class T> boost::shared_ptr< T > componentByName( const std::string& name )
		{
			ComponentMap::iterator it = m_componentIDMap.find( name );
			if ( it == m_componentIDMap.end() )
				UBITRACK_THROW( (std::string)"no match for component name: '" + name + "'" );
			return boost::dynamic_pointer_cast<T>(it->second);
		}

		/**
		 * Get a vector of components which fit a certain prefix
		 *
		 * This mathod returns the vector of components contained in the
		 * dataflow network which have a given prefix in their name.
		 * The components are returned as a vector of shared pointers
		 * dynamic_pointer_cast'ed to type T.
		 * @param prefix the common prefix for which to search
		 * @return a vector of matching components
		 * @throws Ubitrack::Util::Exception if no components are found
		 */
		template <class T> std::vector< boost::shared_ptr< T > > componentsByPrefix( const std::string& prefix )
		{
			std::vector< boost::shared_ptr< T > > matches;

			// find all components whose name starts with prefix
			for ( ComponentMap::iterator it = m_componentIDMap.begin(); it != m_componentIDMap.end(); it++ )
				if ( it->first.find( prefix, 0 ) == 0 )
				{
					boost::shared_ptr< T > tmp = boost::dynamic_pointer_cast<T>(it->second);
					if ( tmp.get() == 0 )
						UBITRACK_THROW( (std::string)"component type mismatch: '" + it->second->getName() + "' is no '" + typeid(T).name() + "'" );
					matches.push_back( tmp );
				}

			// return all matching components or throw
			if ( matches.empty() ) UBITRACK_THROW( (std::string)"no match for component prefix: '" + prefix + "'" );
			return matches;
		}

		/**
		 * Get a vector of components which are of a certain dataflow class
		 *
		 * This mathod returns the vector of components contained in the
		 * dataflow network which are of a specified type.
		 * The membership of a component is tested by dynamic_pointer_cast'ing
		 * the pointer to the desired type and checking if a non-zero
		 * result is obtained. The only parameter of this method is contained
		 * in the template type T.
		 * @return a vector of matching components
		 * @throws Ubitrack::Util::Exception if no components are found
		 */
		template <class T> std::vector< boost::shared_ptr< T > > componentsByType( )
		{
			std::vector< boost::shared_ptr< T > > matches;

			// find all components whose type is compatible to T
			for ( ComponentMap::iterator it = m_componentIDMap.begin(); it != m_componentIDMap.end(); it++ ) {
				boost::shared_ptr< T > tmp = boost::dynamic_pointer_cast<T>(it->second);
				if ( tmp.get() != 0 )
					matches.push_back( tmp );
			}

			// return all matching components or throw
			if ( matches.empty() ) UBITRACK_THROW( (std::string)"no match for component type: '" + typeid(T).name() + "'" );
			return matches;
		}

		/**
		 * Assigns events priorities to the components.
		 */
		void assignEventPriorities();

	protected:
		/**
		 * Helper function that provides easy access to a port pair
		 *
		 * This method is a helper function to facilitate access to
		 * a pair of ports specified by the name of two components and
		 * two names of the ports.
		 * @param connection a dataflow connection (4 tuple)
		 * @return a pair of Port pointers for the specified ports
		 * @throws Ubitrack::Util::Exception if one of the ports cannot be found
		 */
		boost::tuple<Port*, Port*> getPortPair( const DataflowNetworkConnection& connection );

		/// Map that stores all currently existent components by name
		/// The component name is the pattern id from the response
		typedef std::map< std::string, boost::shared_ptr<Component> > ComponentMap;

		/// Set of all connections
		typedef std::set< DataflowNetworkConnection > ConnectionSet;

		/// Map that stores for each component (by name) a set of currently established connections.
		typedef std::map< std::string, std::set< DataflowNetworkConnection > > ConnectionMap;


		/// Keep a reference to the component factory
		ComponentFactory& m_componentFactory;

		/// Map storing all components by component name
		ComponentMap m_componentIDMap;

		/// Map for all components of all incoming connections
		ConnectionMap m_inConnectionMap;
		/// Map for all components of all outgoing connections
		ConnectionMap m_outConnectionMap;

		/// Set of all connections
		ConnectionSet m_allConnections;
	};

} } // namespace Ubitrack::Dataflow

#endif // __Ubitrack_Dataflow_DataflowNetwork_INCLUDED__
