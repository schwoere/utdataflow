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
 * Template class to implement Modules that contain resources shared between multiple components
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#ifndef __Ubitrack_Dataflow_Module_INCLUDED__
#define __Ubitrack_Dataflow_Module_INCLUDED__

#include <map>
#include <stdexcept>
#include <string>
#include <boost/weak_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include "ComponentFactory.h"

namespace Ubitrack { namespace Dataflow {

/**
 * @ingroup dataflow_framework
 * Template class to simplify sharing resources between multiple components.
 *
 * Modules must derive from this class and register themselves at a \c ComponentFactory
 * using the registerModule method.
 *
 * @param ModuleKey class that identifies a module.\n
 *    Must provide a constructor to convert from \c ComponentConfiguration and the usual semantics
 *    for using it as a a key in a \c std::map.
 * @param ComponentKey class that identifies a component within a module.\n
 *    Must provide a constructor to convert from  \c ComponentConfiguration and the usual semantics
 *    for using it as a a key in a \c std::map.
 * @param ModuleClassT class that implements modules.\n
 *    Must be derived from \c Module and provide a constructor with signature
 *    \code ModuleClass( const ModuleKey&, const ComponentConfiguration&, FactoryHelper* ) \endcode
 * @param ComponentClass class that implements a component.\n
 *    Must be derived from \c Module::Component and provide a constructor with signature
 *    \code ComponentClass( const std::string& name, const ComponentConfiguration& config, const ComponentKey& , ModuleClass* ) \endcode
 */
template< class ModuleKeyT, class ComponentKeyT, class ModuleClassT, class ComponentClassT >
class Module
	: private boost::noncopyable
{
public:
	// copy template parameters to class definition for usage by external and derived classes

	/** type of the class derived from this one */
	typedef ModuleClassT ModuleClass;

	/** type of the module key */
	typedef ModuleKeyT ModuleKey;

	/** type of the component key */
	typedef ComponentKeyT ComponentKey;

	/** type of the component class */
	typedef ComponentClassT ComponentClass;

	/**
	 * Base class of components belonging to a particular module.
	 */
	class Component
		: public Dataflow::Component
	{
	public:
		/**
		 * constructor
		 * @param name name of component, passed to \c Dataflow::Component::Component
		 * @param componentKey key of this component
		 * @param pModule pointer to module owning this component, stored in m_pComponent
		 */
		Component( const std::string& name, const ComponentKey& componentKey, ModuleClass* pModule )
			: Dataflow::Component( name )
			, m_componentKey( componentKey )
			, m_pModule( pModule )
		{}

		/** destructor, unregisters component at module */
		~Component()
		{
			m_pModule->unregisterComponent( m_componentKey );
		}

		/** access to parent module */
		ModuleClass& getModule()
		{
			return *m_pModule;
		}

		/** returns the \c ComponentKey of this component */
        const ComponentKey& getKey() const
        {
            return m_componentKey;
        }

		/**
		 * component stop method for modules
		 *
		 * deletes the running flag and signals the module
		 * that this component is stopped
		 */
		virtual void stop()
		{
			m_running = false;
			getModule().componentStopped( getKey() );
		}

		/**
		 * component start method for modules
		 *
		 * sets the running flag and signals the module
		 * that this component is started
		 */
		virtual void start()
		{
			m_running = true;
			getModule().componentStarted( getKey() );
		}

		virtual bool isRunning()
		{
			return m_running;
		}

	protected:
		// component key for this component
		ComponentKey m_componentKey;

		// pointer to module
		ModuleClass* m_pModule;
	};


	/**
	 * Factory for modules.
	 * This is registered at the component factory and creates new modules when necessary.
	 */
	class FactoryHelper
		: public ComponentFactory::FactoryHelper
	{
	public:
		/**
		 * implementation of virtual factory method from ComponentFactory::FactoryHelper
		 */
		boost::shared_ptr< Dataflow::Component > getComponent( const std::string& type, const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph )
		{
			// key for map lookup
			ModuleKey key( subgraph );

			// create new module if necessary
			if ( !m_moduleMap[ key ] )
				m_moduleMap[ key ] = new ModuleClass( key, subgraph, this );

			// create new component
			// FIXME: what happens when an exception is thrown? We could have a module without a component...
			return m_moduleMap[ key ]->newComponent( type, name, subgraph );
		}

		/** remove a module from the map */
		void unregisterModule( const ModuleKey& k )
		{
			m_moduleMap.erase( k );
		}

	protected:
		// map where we store our modules
		typedef std::map< ModuleKey, ModuleClass* > ModuleMap;
		ModuleMap m_moduleMap;
	};

	// finally, the module methods

	/**
	 * Constructor, usually called by the factory.
	 * @param key The \c ModuleKey of this module instance
	 * @param pFactory pointer to a FactoryHelper object
	 */
	Module( const ModuleKey& key, FactoryHelper* pFactory )
		: m_moduleKey( key )
		, m_pFactory( pFactory )
		, m_running( false )
		{}

	/**
	 * virtual destructor.
	 */
	virtual ~Module()
	{}


	/**
	 * Create a new component in this module, usually called by the factory.
	 * UTQL version
	 *
	 * @param type Class name of the component
	 * @param name Name of the new component
	 * @param subgraph UTQL subgraph
	 */
	boost::shared_ptr< Component > newComponent( const std::string& type, const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph )
	{
		// for lookup in map
		ComponentKey key( subgraph );

		// check if component is already there and if yes, return shared_ptr to it
		// Note: This allows a single component to be used in two dataflows!
		{
			boost::mutex::scoped_lock l( m_componentMapMutex );
			if ( !m_componentMap[ key ].expired() )
				return boost::shared_ptr< ComponentClass >( m_componentMap[ key ] );
		}

		// create new component and add to map
		boost::shared_ptr< ComponentClass > pComp( createComponent( type, name, subgraph, key, static_cast< ModuleClass* >( this ) ) );

		boost::mutex::scoped_lock l( m_componentMapMutex );
		m_componentMap[ key ] = pComp;

		return boost::shared_ptr< Component >( pComp );
	}

    /**
     * Returns whether a component specified by a ComponentKey exists in the module.
     * Does not throw.
	 *
	 * @param key the key to the component
	 * @return true if the component exists.
     */
    bool hasComponent( const ComponentKey& key )
    {
        typename ComponentMap::iterator it = m_componentMap.find( key );
        return ( it != m_componentMap.end() );
    }

	/**
	 * Returns a component by \c ComponentKey.
	 * Throws a \c Ubitrack::Util::Exception if component does not exist.
	 *
	 * @param key \c ComponentKey
	 * @param return shared pointer to component
	 */
	boost::shared_ptr< ComponentClass > getComponent( const ComponentKey& key )
	{
		boost::mutex::scoped_lock l( m_componentMapMutex );

		typename ComponentMap::iterator it = m_componentMap.find( key );
		if ( it == m_componentMap.end() )
			UBITRACK_THROW( "component does not exist" );
		else
		{
			// convert weak to shared pointer to prevent unwanted deletion of object
			boost::shared_ptr< ComponentClass > r( it->second );
			if ( !r )
				UBITRACK_THROW( "shared pointer to component cannot be acquired" );
			return r;
		}
	}

	typedef std::vector< boost::shared_ptr< ComponentClass > >  ComponentList;

	/**
	 * Return all components managed by this module.
	 * Locks all components and thus delays the destruction until
	 * after the caller is done with the pointers.
	 */
    ComponentList getAllComponents()
	{
		ComponentList components;

		boost::mutex::scoped_lock l( m_componentMapMutex );
		typename ComponentMap::iterator mapEnd = m_componentMap.end();

		for ( typename ComponentMap::iterator mapIt = m_componentMap.begin();
			mapIt != mapEnd; ++mapIt) {

			boost::shared_ptr< ComponentClass > ptr = mapIt->second.lock();

			// only return those pointer that actually exist..
			if ( ptr )
				components.push_back( ptr );
		}

		return components;
	}

	/** unregisters a component and destroys the module if no components remain */
	void unregisterComponent( const ComponentKey& key )
	{
		bool bDelete;

		{
			// remove component and check if no component left
			boost::mutex::scoped_lock l( m_componentMapMutex );
			m_componentMap.erase( key );
			bDelete = m_componentMap.empty();
		}

		// destroy this module if no components exist
		if ( bDelete )
		{
			m_pFactory->unregisterModule( m_moduleKey );
			delete this;
		}
	}

	/**
	 * mark a component as stopped
	 *
	 * this method is called by a component belonging to a module
	 * when it is stopped.
	 *
	 * the module keeps a set of all started components and
	 * stops itself as soon as no components are running.
	 */
	virtual void componentStopped( const ComponentKey& key )
	{
		if ( m_running )
		{
			bool allStopped = true;
			{
				boost::mutex::scoped_lock l( m_componentMapMutex );

				for ( typename ComponentMap::iterator pComponent = m_componentMap.begin();
					pComponent != m_componentMap.end(); ++pComponent )
				{
					if ( pComponent->second.lock()->isRunning() )
					{
						allStopped = false;
						break;
					}
				}
			}

			if ( allStopped )
			{
				stopModule();
				m_running = false;
			}
		}
	}

	/**
	 * mark a component as started
	 *
	 * this method is called by a component belonging to a module
	 * when it is started.
	 *
	 * the module keeps a set of all started components and
	 * stats itself as soon as the first component is started.
	 */
	virtual void componentStarted( const ComponentKey& key )
	{
		if ( !m_running )
		{
			startModule();
			m_running = true;
		}
	}

	/**
	 * stops the module
	 *
	 * this method is called to stop the module.
	 * Called when the last component is stopped.
	 */
	virtual void stopModule()
	{
	}

	/**
	 * starts the module
	 *
	 * this method is called to start the module.
	 * Called for the first component that needs to be started.
	 */
	virtual void startModule()
	{
	}

	/**
	 * returns the module key
	 */
	const ModuleKey& getModuleKey() const
	{ return m_moduleKey; }

protected:
	// module key of this module
	ModuleKey m_moduleKey;

	// pointer to FactoryHelper that created the module
	FactoryHelper* m_pFactory;

	// map of all components
	typedef std::map< ComponentKey, boost::weak_ptr< ComponentClass > > ComponentMap;
	ComponentMap m_componentMap;

	// flag if module is running
	bool m_running;

	// mutex to protect the component list
	boost::mutex m_componentMapMutex;

	/**
	 * Factory method that actually creates components.
	 * Override this if your module supports more than one component class.
	 *
	 * @param type Class name of the new component
	 * @param name Name of the new component
	 * @param subgraph \c UTQL subgraph configuration of the new component
	 * @param key, \c ComponentKey of the new component
	 * @return shared pointer to new component
	 */
	virtual boost::shared_ptr< ComponentClass > createComponent( const std::string&, const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph> subgraph,
		const ComponentKey& key, ModuleClass* pModule )
	{ return boost::shared_ptr< ComponentClass >( new ComponentClass( name, subgraph, key, pModule ) ); }
};


/**
 * @ingroup dataflow_framework
 * simple key type that can be used as the module key when only one module ("singleton") is required
 */
class SingleModuleKey
{
public:
	SingleModuleKey( boost::shared_ptr< Graph::UTQLSubgraph > )
	{}
};

static inline bool operator<( const SingleModuleKey&, const SingleModuleKey& )
{ return false; }


/**
 * Base class used by NodeAttributeKey and EdgeAttributeKey
 * \internal
 */
template< typename ValueType >
class NodeEdgeAttributeKeyBase
{
public:
	/**
	 * exception-throwing constructor
	 */
	NodeEdgeAttributeKeyBase( const Graph::KeyValueAttributes& attribs, const char* sAttributeName )
	{
		if ( !attribs.hasAttribute( sAttributeName ) )
			UBITRACK_THROW( "No \"" + std::string( sAttributeName ) + "\" attribute found" );

		readAttribute( attribs, sAttributeName );
	}

	/**
	 * default-value-assuming constructor
	 */
	NodeEdgeAttributeKeyBase( const Graph::KeyValueAttributes& attribs, const char* sAttributeName, const ValueType& defaultValue )
	{
		if ( !attribs.hasAttribute( sAttributeName ) )
			m_value = defaultValue;
		else
			readAttribute( attribs, sAttributeName );
	}

	/** construct from value */
	NodeEdgeAttributeKeyBase( const ValueType& value )
		: m_value( value )
	{}
	
	/** conversion to base type */
	operator ValueType() const
	{ return m_value; }
	
	/** returns the content */
	const ValueType& get() const
	{ return m_value; }
	
protected:
	void readAttribute( const Graph::KeyValueAttributes& attribs, const char* sAttributeName );
	ValueType m_value;
};

template<> 
inline void NodeEdgeAttributeKeyBase< std::string >::readAttribute( const Graph::KeyValueAttributes& attribs, const char* sAttributeName )
{ 
	m_value = attribs.getAttributeString( sAttributeName ); 
}

template< typename ValueType >
inline void NodeEdgeAttributeKeyBase< ValueType >::readAttribute( const Graph::KeyValueAttributes& attribs, const char* sAttributeName )
{ 
	attribs.getAttributeData( sAttributeName, m_value ); 
}

/** comparison operator for NodeEdgeAttributeKeyBase */
template< typename ValueType >
bool operator<( const NodeEdgeAttributeKeyBase< ValueType >& a, const NodeEdgeAttributeKeyBase< ValueType >& b )
{ return static_cast< ValueType >( a ) < static_cast< ValueType >( b ); }


/**
 * Predefined key type for reading an attribute of a node.
 * The value of the attribute is automatically converted to a certain type.
 * Needs to be subclassed with a constructor that defines the node and attribute names.
 */
template< typename ValueType >
class NodeAttributeKey
	: public NodeEdgeAttributeKeyBase< ValueType >
{
public:
	/**
	 * Constructor that throws an exception if the attribute is not present
	 * @param pConfig UTQL component configuration, supplied by the factory
	 * @param sNodeName name of the node containing the attributed
	 * @param sAttributeName name of the attribute that will be the key
	 */
	NodeAttributeKey( boost::shared_ptr< Graph::UTQLSubgraph > pConfig, const char* sNodeName, const char* sAttributeName )
		: NodeEdgeAttributeKeyBase< ValueType >( *pConfig->getNode( sNodeName ), sAttributeName )
	{}
	
	/**
	 * Constructor that assumes a default value if the attribute is not present
	 * The node still needs to be present, though!
	 * @param pConfig UTQL component configuration, supplied by the factory
	 * @param sNodeName name of the node containing the attributed
	 * @param sAttributeName name of the attribute that will be the key
	 * @param defaultValue default value when the attribute is not present
	 */
	NodeAttributeKey( boost::shared_ptr< Graph::UTQLSubgraph > pConfig, const char* sNodeName, const char* sAttributeName, 
		const ValueType& defaultValue )
		: NodeEdgeAttributeKeyBase< ValueType >( *pConfig->getNode( sNodeName ), sAttributeName, defaultValue )
	{}
	
	/** construct from a known value */
	NodeAttributeKey( const ValueType & value )
		: NodeEdgeAttributeKeyBase< ValueType >( value )
	{}
};


/**
 * Predefined key type for reading an attribute of an edge.
 * The value of the attribute is automatically converted to a certain type.
 * Needs to be subclassed with a constructor that defines the edge and attribute names.
 */
template< typename ValueType >
class EdgeAttributeKey
	: public NodeEdgeAttributeKeyBase< ValueType >
{
public:
	/**
	 * Constructor that throws an exception if the attribute is not present.
	 * @param pConfig UTQL component configuration, supplied by the factory
	 * @param sEdgeName name of the edge containing the attributed
	 * @param sAttributeName name of the attribute that will be the key
	 */
	EdgeAttributeKey( boost::shared_ptr< Graph::UTQLSubgraph > pConfig, const char* sEdgeName, const char* sAttributeName )
		: NodeEdgeAttributeKeyBase< ValueType >( *pConfig->getEdge( sEdgeName ), sAttributeName )
	{}
	
	/**
	 * Constructor that assumes a default value if the attribute is not present.
	 * The edge still needs to be present, though!
	 * @param pConfig UTQL component configuration, supplied by the factory
	 * @param sEdgeName name of the edge containing the attributed
	 * @param sAttributeName name of the attribute that will be the key
	 * @param defaultValue default value when the attribute is not present
	 */
	EdgeAttributeKey( boost::shared_ptr< Graph::UTQLSubgraph > pConfig, const char* sEdgeName, const char* sAttributeName, 
		const ValueType& defaultValue )
		: NodeEdgeAttributeKeyBase< ValueType >( *pConfig->getEdge( sEdgeName ), sAttributeName, defaultValue )
	{}
	
	/** construct from a known value */
	EdgeAttributeKey< ValueType >( const ValueType & value )
		: NodeEdgeAttributeKeyBase< ValueType >( value )
	{}
};


/**
 * Predefined key type for reading an attribute of the DataflowConfiguration.
 * The value of the attribute is automatically converted to a certain type.
 * Needs to be subclassed with a constructor that defines the edge and attribute names.
 */
template< typename ValueType >
class DataflowConfigurationAttributeKey
	: public NodeEdgeAttributeKeyBase< ValueType >
{
public:
	/**
	 * Constructor that throws an exception if the attribute is not present.
	 * @param pConfig UTQL component configuration, supplied by the factory
	 * @param sAttributeName name of the attribute that will be the key
	 */
	DataflowConfigurationAttributeKey( boost::shared_ptr< Graph::UTQLSubgraph > pConfig, const char* sAttributeName )
		: NodeEdgeAttributeKeyBase< ValueType >( pConfig->m_DataflowAttributes, sAttributeName )
	{}
	
	/**
	 * Constructor that assumes a default value if the attribute is not present.
	 * The edge still needs to be present, though!
	 * @param pConfig UTQL component configuration, supplied by the factory
	 * @param sAttributeName name of the attribute that will be the key
	 * @param defaultValue default value when the attribute is not present
	 */
	DataflowConfigurationAttributeKey( boost::shared_ptr< Graph::UTQLSubgraph > pConfig, const char* sAttributeName, 
		const ValueType& defaultValue )
		: NodeEdgeAttributeKeyBase< ValueType >( pConfig->m_DataflowAttributes, sAttributeName, defaultValue )
	{}
	
	/** construct from a known value */
	DataflowConfigurationAttributeKey< ValueType >( const ValueType & value )
		: NodeEdgeAttributeKeyBase< ValueType >( value )
	{}
};


/**
 * Predefined key that takes the id of a node.
 * Needs to be subclassed in order to define the node name.
 */
class NodeIdKey
	: public std::string
{
public:
	/** constructor */
	NodeIdKey( boost::shared_ptr< Graph::UTQLSubgraph > pConfig, const char* sNodeName )
		: std::string( pConfig->getNode( sNodeName )->m_QualifiedName )
	{
		if ( empty() )
			UBITRACK_THROW( "Node " + std::string( sNodeName ) + " has no ID!" );
	}

	/** construct from a known value */
	NodeIdKey( const std::string& value )
		: std::string( value )
	{}
	
};


/**
 * Predefined key that takes the id of the whole subgraph.
 */
class SubgraphIdKey
	: public std::string
{
public:
	/** constructor */
	SubgraphIdKey( boost::shared_ptr< Graph::UTQLSubgraph > pConfig )
		: std::string( pConfig->m_ID )
	{
		if ( empty() )
			UBITRACK_THROW( "UTQL Pattern has no ID!" );
	}
	
	/** construct from known value */
	SubgraphIdKey( const std::string& sId )
		: std::string( sId )
	{}
};


/**
 * Commodity macro to create a NodeAttributeKey for a particular node name.
 * Not really nice, but is there a better solution?
 * @param ClassName name of the new key class
 * @param ValueType class of value to extract
 * @param nodeName name of the node in the pattern whose attribute needs to be extracted
 * @param attributeName name of the attribute to extract
 */
#define MAKE_NODEATTRIBUTEKEY( ClassName, ValueType, nodeName, attributeName ) \
class ClassName : public Ubitrack::Dataflow::NodeAttributeKey< ValueType > \
{ public: \
	ClassName( boost::shared_ptr< Ubitrack::Graph::UTQLSubgraph > pConfig ) \
		: Ubitrack::Dataflow::NodeAttributeKey< ValueType >( pConfig, nodeName, attributeName ) {} \
	ClassName( const ValueType& defVal ) \
		: Ubitrack::Dataflow::NodeAttributeKey< ValueType >( defVal ) {} \
};
 
/**
 * Commodity macro to create a NodeAttributeKey for a particular node name.
 * Not really nice, but is there a better solution?
 * @param ClassName name of the new key class
 * @param ValueType class of value to extract
 * @param nodeName name of the node in the pattern whose attribute needs to be extracted
 * @param attributeName name of the attribute to extract
 * @param defaultValue default value to use when the attribute is not found
 */
#define MAKE_NODEATTRIBUTEKEY_DEFAULT( ClassName, ValueType, nodeName, attributeName, defaultValue ) \
class ClassName : public Ubitrack::Dataflow::NodeAttributeKey< ValueType > \
{ public: \
	ClassName( boost::shared_ptr< Ubitrack::Graph::UTQLSubgraph > pConfig ) \
		: Ubitrack::Dataflow::NodeAttributeKey< ValueType >( pConfig, nodeName, attributeName, defaultValue ) {} \
	ClassName( const ValueType& defVal ) \
		: Ubitrack::Dataflow::NodeAttributeKey< ValueType >( defVal ) {} \
};

/**
 * Commodity macro to create an EdgeAttributeKey for a particular edge name.
 * Not really nice, but is there a better solution?
 * @param ClassName name of the new key class
 * @param ValueType class of value to extract
 * @param edgeName name of the edge in the pattern whose attribute needs to be extracted
 * @param attributeName name of the attribute to extract
 */
#define MAKE_EDGEATTRIBUTEKEY( ClassName, ValueType, edgeName, attributeName ) \
class ClassName : public Ubitrack::Dataflow::EdgeAttributeKey< ValueType > \
{ public: \
	ClassName( boost::shared_ptr< Ubitrack::Graph::UTQLSubgraph > pConfig ) \
		: Ubitrack::Dataflow::EdgeAttributeKey< ValueType >( pConfig, edgeName, attributeName ) {} \
	ClassName( const ValueType& defVal ) \
		: Ubitrack::Dataflow::EdgeAttributeKey< ValueType >( defVal ) {} \
};
 
/**
 * Commodity macro to create an EdgeAttributeKey for a particular edge name.
 * Not really nice, but is there a better solution?
 * @param ClassName name of the new key class
 * @param ValueType class of value to extract
 * @param edgeName name of the edge in the pattern whose attribute needs to be extracted
 * @param attributeName name of the attribute to extract
 * @param defaultValue default value to use when the attribute is not found
 */
#define MAKE_EDGEATTRIBUTEKEY_DEFAULT( ClassName, ValueType, edgeName, attributeName, defaultValue ) \
class ClassName : public Ubitrack::Dataflow::EdgeAttributeKey< ValueType > \
{ public: \
	ClassName( boost::shared_ptr< Ubitrack::Graph::UTQLSubgraph > pConfig ) \
		: Ubitrack::Dataflow::EdgeAttributeKey< ValueType >( pConfig, edgeName, attributeName, defaultValue ) {} \
	ClassName( const ValueType& defVal ) \
		: Ubitrack::Dataflow::EdgeAttributeKey< ValueType >( defVal ) {} \
};

 
/**
 * Commodity macro to create an DataflowConfigurationAttributeKey for a particular edge name.
 * Not really nice, but is there a better solution?
 * @param ClassName name of the new key class
 * @param ValueType class of value to extract
 * @param attributeName name of the attribute to extract
 */
#define MAKE_DATAFLOWCONFIGURATIONATTRIBUTEKEY( ClassName, ValueType, attributeName ) \
class ClassName : public Ubitrack::Dataflow::DataflowConfigurationAttributeKey< ValueType > \
{ public: \
	ClassName( boost::shared_ptr< Ubitrack::Graph::UTQLSubgraph > pConfig ) \
		: Ubitrack::Dataflow::DataflowConfigurationAttributeKey< ValueType >( pConfig, attributeName ) {} \
	ClassName( const ValueType& defVal ) \
		: Ubitrack::Dataflow::DataflowConfigurationAttributeKey< ValueType >( defVal ) {} \
};

 
/**
 * Commodity macro to create an DataflowConfigurationAttributeKey for a particular edge name.
 * Not really nice, but is there a better solution?
 * @param ClassName name of the new key class
 * @param ValueType class of value to extract
 * @param attributeName name of the attribute to extract
 * @param defaultValue default value to use when the attribute is not found
 */
#define MAKE_DATAFLOWCONFIGURATIONATTRIBUTEKEY_DEFAULT( ClassName, ValueType, attributeName, defaultValue ) \
class ClassName : public Ubitrack::Dataflow::DataflowConfigurationAttributeKey< ValueType > \
{ public: \
	ClassName( boost::shared_ptr< Ubitrack::Graph::UTQLSubgraph > pConfig ) \
		: Ubitrack::Dataflow::DataflowConfigurationAttributeKey< ValueType >( pConfig, attributeName, defaultValue ) {} \
	ClassName( const ValueType& defVal ) \
		: Ubitrack::Dataflow::DataflowConfigurationAttributeKey< ValueType >( defVal ) {} \
};

 
/**
 * Commodity macro to create a NodeIdKey for a particular node name.
 * Not really nice, but is there a better solution?
 * @param ClassName name of the new key class
 * @param nodeName name of the edge in the pattern whose id needs to be extracted
 */
#define MAKE_NODEIDKEY( ClassName, nodeName ) \
class ClassName : public Ubitrack::Dataflow::NodeIdKey \
{ public: \
	ClassName( boost::shared_ptr< Ubitrack::Graph::UTQLSubgraph > pConfig ) \
		: Ubitrack::Dataflow::NodeIdKey( pConfig, nodeName ) {} \
	ClassName( const std::string& defVal ) \
		: Ubitrack::Dataflow::NodeIdKey( defVal ) {} \
};

 

} } // namespace Ubitrack::Dataflow

#endif
