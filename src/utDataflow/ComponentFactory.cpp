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


#include <log4cpp/Category.hh>
#include <boost/filesystem/operations.hpp>
#include "ComponentFactory.h"


namespace Ubitrack { namespace Dataflow {


	static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Dataflow.ComponentFactory" ) );


	ComponentFactory::ComponentFactory( const std::string& sComponentDir )
	{
		// library extension
#ifdef _WIN32
		const std::string compSuffix( ".dll" );
#elif __APPLE__
		const std::string compSuffix( ".dylib" );
#else
		const std::string compSuffix( ".so" );
#endif

		// check component directory
		LOG4CPP_INFO( logger, "Looking for components in " << sComponentDir );
		using namespace boost::filesystem;
#if BOOST_FILESYSTEM_VERSION == 3
		path compPath( sComponentDir.c_str() );
#else
		path compPath( sComponentDir.c_str(), native );
#endif
		if ( !exists( compPath ) ){
			LOG4CPP_ERROR(logger, "Component directory does not exist" )
			UBITRACK_THROW( "Component directory \"" + sComponentDir + "\" does not exist" );
		}
		
		// initialize the ltdl library
		if ( lt_dlinit() != 0 ){
			LOG4CPP_ERROR(logger, "libltdl::lt_dlinit() failed: " )
			UBITRACK_THROW( "libltdl::lt_dlinit() failed: " + std::string( lt_dlerror() ) );
		}

		// iterate directory
		directory_iterator dirEnd;
		for ( directory_iterator it( compPath ); it != dirEnd; it++ )
		//for ( int i=0;i<4;i++)
		{
#ifdef BOOST_FILESYSTEM_I18N
			path p( it->path() );
#else
			path p( *it );
#endif
			// check if file of suitable extension
#if BOOST_FILESYSTEM_VERSION == 3
			if ( exists( p ) && !is_directory( p ) && p.leaf().string().size() >= compSuffix.size() &&
				 !p.leaf().string().compare( p.leaf().string().size() - compSuffix.size(), compSuffix.size(), compSuffix ) )
#else
			if ( exists( p ) && !is_directory( p ) && p.leaf().size() >= compSuffix.size() &&
				 !p.leaf().compare( p.leaf().size() - compSuffix.size(), compSuffix.size(), compSuffix ) )
#endif
			{
				LOG4CPP_INFO( logger, "Loading driver: " << p.leaf() );

				std::string sCompPath;
#if BOOST_FILESYSTEM_VERSION == 3
				//file_string is Deprecated 
				sCompPath = p.string();
#else
				sCompPath = p.native_file_string();
#endif
				lt_dlhandle tmp = lt_dlopenext( sCompPath.c_str() );
				if ( tmp == 0 )
				{
					LOG4CPP_ERROR( logger, "libltdl::lt_dlopen( '" << sCompPath << "' ) failed: " << lt_dlerror() );
					continue;
				}
				m_handles.push_back( tmp );

				registerComponentFunction* regfunc = (registerComponentFunction*) lt_dlsym( tmp, "registerComponent" );
				if ( regfunc == 0 )
				{
					LOG4CPP_ERROR( logger, "libltdl::lt_dlsym( '" << sCompPath << "' ) failed: " << lt_dlerror() );
					continue;
				}
				
				try
				{
					(*regfunc)( this );
				}
				catch ( const Util::Exception& e )
				{
					LOG4CPP_ERROR( logger, p.leaf() << " failed to load: " << e.what() );
				}
					
			}
		}
	}


	ComponentFactory::ComponentFactory( const std::vector< std::string >& libs )
	{
		lt_dlhandle tmp;
		registerComponentFunction* regfunc;

		// initialize the ltdl library
		if ( lt_dlinit() != 0 )
			UBITRACK_THROW( "libltdl::lt_dlinit() failed: " + std::string( lt_dlerror() ) );

		// load all requested libraries
		for ( std::vector< std::string >::const_iterator lib = libs.begin(); lib != libs.end(); lib++ )
		{
			LOG4CPP_INFO( logger, "loading library " << *lib );

			tmp = lt_dlopenext( lib->c_str() );
			if ( tmp == 0 )
				UBITRACK_THROW( "libltdl::lt_dlopen( '" + *lib + "' ) failed: " + std::string( lt_dlerror() ) );
			m_handles.push_back( tmp );

			regfunc = (registerComponentFunction*) lt_dlsym( tmp, "registerComponent" );
			if ( regfunc == 0 )
				UBITRACK_THROW( "libltdl::lt_dlsym( '" + *lib + "' ) failed: " + std::string( lt_dlerror() ) );

			(*regfunc)( this );
		}
	}


	ComponentFactory::~ComponentFactory( )
	{
		// first, clean up map to destroy factory helpers
		m_components.clear();

		// now, unload all plugins
		for ( std::vector< lt_dlhandle>::iterator handle = m_handles.begin(); handle != m_handles.end(); handle++ )
		{
			if ( lt_dlclose( *handle ) != 0 )
				LOG4CPP_ERROR( logger, "libltdl::lt_dlclose(..) failed: " << lt_dlerror() );
		}

		if ( lt_dlexit() != 0 )
			LOG4CPP_ERROR( logger, "libltdl::lt_dlexit() failed: " << lt_dlerror() );
	}

	boost::shared_ptr< Component >  ComponentFactory::createComponent( const std::string& type, const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph )
	{
		if ( m_components.find( type ) == m_components.end() ) 
		{
			LOG4CPP_TRACE( logger, "Component class " << type << " has not been registered. ");
			LOG4CPP_TRACE( logger, "Registered components are: " );
			for ( std::map< std::string, boost::shared_ptr< FactoryHelper > >::const_iterator lib = m_components.begin(); lib != m_components.end(); lib++ ) 
			{
				LOG4CPP_TRACE( logger, (*lib).first );
			}
			
			
			UBITRACK_THROW( "Component class '" + type + "' has not been registered." );
		}
		

		return m_components[ type ] -> getComponent( type, name, subgraph );
	}

	void ComponentFactory::deregisterComponent( const std::string& type )
	{
		if ( m_components.find( type ) == m_components.end() )
			UBITRACK_THROW( "Component class '" + type + "' has not been registered." );

		m_components.erase( type );
	}


} } // namespace Ubitrack::Dataflow
