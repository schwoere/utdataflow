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
 * ComponentFactory creates Component objects based on name
 * @author Florian Echtler <echtler@in.tum.de>
 */


#ifndef __Ubitrack_Dataflow_ComponentFactory_INCLUDED__
#define __Ubitrack_Dataflow_ComponentFactory_INCLUDED__


#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include <utDataflow.h>
#include <utUtil/Exception.h>

#include <utGraph/UTQLSubgraph.h>

#include "DLLoader.h"
#include "Component.h"

namespace Ubitrack { namespace Dataflow {

/**
 * @ingroup dataflow_framework
 * ComponentFactory creates Component objects based on name
 */

class UTDATAFLOW_EXPORT ComponentFactory
	: private boost::noncopyable
{

public:

	/**
	 * Constructor
	 *
	 * @param sComponentDir directory with components to load. If NULL, the default
	 *    directory (specified at compile-time) is used.
	 * @throws Ubitrack::Util::Exception when
	 */
	ComponentFactory( const std::string& sComponentDir = std::string() );


	/**
	 * Constructor
	 *
	 * @param libs list of full paths to shared libraries with components to load
	 * @throws Ubitrack::Util::Exception when
	 */
	ComponentFactory( const std::vector< std::string >& libs );


	/**
	 * Destructor
	 */
	~ComponentFactory( );


	/**
	 * register a component class
	 * Stores the mapping between a class name and the class type in an internal map.
	 *
	 * @param componentClass C++ class of which objects are created
	 * @param type name of the component class
	 * @throws Ubitrack::Util::Exception if type is already registered
	 */
	template< class componentClass >
	void registerComponent( const std::string& type )
	{
		if ( m_components.find( type ) != m_components.end() )
			UBITRACK_THROW( "Component class '" + type + "' has already been registered." );

		m_components[ type ] = boost::shared_ptr< FactoryHelper >( static_cast< FactoryHelper* >( new SimpleFactoryHelper< componentClass > ) );
	}


	/**
	 * Register a component class that uses modules.
	 * Stores the mapping between a module class name and the module class type in an internal map.
	 *
	 * @param ModuleClass class of the module. Should be derived from Dataflow::Module or must provide a FactoryHelper class
	 * @param type name of the component class
	 * @throws Ubitrack::Util::Exception if type is already registered
	 */
	template< class ModuleClass >
	void registerModule( const std::string& type )
	{
		if ( m_components.find( type ) != m_components.end() )
			UBITRACK_THROW( "Component class '" + type + "' has already been registered." );

		m_components[ type ] = boost::shared_ptr< FactoryHelper >( new typename ModuleClass::FactoryHelper );
	}


	/**
	 * Register a component class that uses modules and can provide multiple component classes.
	 * Stores the mapping between a module class name and the module class type in an internal map.
	 *
	 * @param ModuleClass class of the module. Should be derived from Dataflow::Module or must provide a FactoryHelper class
	 * @param types vector of names of supported component classes
	 */
	template< class ModuleClass >
	void registerModule( const std::vector< std::string >& types )
	{
		boost::shared_ptr< FactoryHelper > pFh( new typename ModuleClass::FactoryHelper );
		for ( std::vector< std::string >::const_iterator it = types.begin(); it != types.end(); it++ )
			m_components[ *it ] = pFh;
	}


	/**
	 * create a new registered component
	 *
	 * @param type name of the component class
	 * @param name name of the created component instance
	 * @param config configuration extracted from xml
	 * @return a shared pointer to a new instance of the component
	 * @throws Ubitrack::Util::Exception if the type is unknown
	 */
	boost::shared_ptr< Component > createComponent( const std::string& type, const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph );

	/**
	 * deregister a component class
	 *
	 * @param type name of the component class
	 * @throws Ubitrack::Util::Exception if type is unknown
	 */
	void deregisterComponent( const std::string& type );


	/**
	 * @ingroup dataflow_framework
	 *
	 * We need a small class to map strings to which is subclassed for each component.
	 * Normally, this is done by registerClass or registerModule. You only need
	 * this class if you want to provide your own moduling mechanism.
	 */
	class FactoryHelper
	{
		public:
			/** constructor */
			FactoryHelper( ) { }

			/** Virtual destructor. One should always have one. */
			virtual ~FactoryHelper() { }

			/**
			 * Factory method to create new components.
			 * This method can return pointers to existing components, if the configuration matches.
			 *
			 * @param type Class name of the new component
			 * @param name Name of the new component
			 * @param config Dataflow configuration of the new component
			 * @return shared pointer to the component
			 */
			virtual boost::shared_ptr< Component > getComponent( const std::string& type, const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > config ) = 0;
	};


	/**
	 * Simple factory helper that creates instances of a single class without modules
	 */
	template< class componentClass >
	class SimpleFactoryHelper
		: public FactoryHelper
	{
		public:
			SimpleFactoryHelper( ) { }
			boost::shared_ptr< Component > getComponent( const std::string&, const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph )
			{
				return boost::shared_ptr< Component > ( new componentClass( name, subgraph ) );
			}
	};

protected:

	/** a map of all known components */
	std::map< std::string, boost::shared_ptr< FactoryHelper > > m_components;

	/** a vector of library handles */
	std::vector< lt_dlhandle > m_handles;

};


/**
 * function prototype for component registration
 * To register itself, a shared component library has to provide this function
 * and call cf->registerComponent<MyComponentClass>("MyComponentClass") in order
 * to create a relationship between names (strings) and types (classes).
 *
 * @param cf the ComponentFactory at which to register
 */
typedef void registerComponentFunction( ComponentFactory* const cf );


}} // namespace Ubitrack::Dataflow


// add this before the definition of all registerComponentFunction functions
#ifdef _WIN32
	#define UBITRACK_REGISTER_COMPONENT extern "C" __declspec( dllexport ) void registerComponent
#else
	#define UBITRACK_REGISTER_COMPONENT extern "C" void registerComponent
#endif

#endif // __Ubitrack_Dataflow_ComponentFactory_INCLUDED__
