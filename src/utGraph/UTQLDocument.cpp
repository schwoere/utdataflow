#include <utGraph/UTQLDocument.h>

#include <algorithm>

using namespace Ubitrack::Graph;


log4cpp::Category& UTQLDocument::m_Logger( log4cpp::Category::getInstance( "Ubitrack.Graph.UTQLDocument" ) );

UTQLDocument::UTQLDocument( bool bRequest )
	: m_bRequest( bRequest )
{
}


void UTQLDocument::addSubgraph( boost::shared_ptr< UTQLSubgraph > subgraph )
{
	std::string id = subgraph->m_ID;
	std::string name = subgraph->m_Name;

	// store subgraph by id
	if ( id.length() != 0 )
	{
		if ( m_SubgraphById.find( id ) != m_SubgraphById.end() )
		{
			LOG4CPP_ERROR( m_Logger, "UTQL subgraph with identical id already in document: " << id );
			UBITRACK_THROW( "UTQL subgraph with identical id already in document" );
		}

		m_SubgraphById[id] = subgraph;
	}

	// store subgraphs by name
	if ( name.length() != 0)
	{
		m_SubgraphByName.insert( std::pair< std::string, boost::shared_ptr< UTQLSubgraph > >( name, subgraph ) );
	}

	// store all subgraphs in list
	m_Subgraphs.push_back( subgraph );
}

bool UTQLDocument::hasSubgraphById( std::string id ) const
{
	return ( m_SubgraphById.find( id ) != m_SubgraphById.end() );
}

boost::shared_ptr< UTQLSubgraph >& UTQLDocument::getSubgraphById( std::string id )
{
	if ( !hasSubgraphById( id ) )
	{
		std::cout << "Arg: \"" << id << "\"" << std::endl;
		for ( SubgraphList::iterator it = m_Subgraphs.begin();
			  it != m_Subgraphs.end(); ++it )
		{
			std::cout << "\"" << (*it)->m_ID << "\" (" << (*it)->m_Name << ")" << std::endl;
		}

		LOG4CPP_ERROR( m_Logger, "UTQL subgraph not present in document: " << id );
		UBITRACK_THROW( "UTQL subgraph not present in document" );
	}
	return m_SubgraphById[id];
}

void UTQLDocument::removeSubgraphById( std::string id )
{
	boost::shared_ptr< UTQLSubgraph > subgraph = getSubgraphById( id );

	m_SubgraphById.erase( id );

	if ( subgraph->m_Name.length() != 0 )
	{
		std::string name = subgraph->m_Name;
		SubgraphNameMap::iterator it = m_SubgraphByName.find( name );
		while ( it != m_SubgraphByName.end() && it->first == name && it->second != subgraph )
		{
			it++;
		}
		if ( it == m_SubgraphByName.end() || it->first != name )
		{
			// maybe assumption that find returns iterator to first name is wrong?
			LOG4CPP_ERROR( m_Logger, "Cannot find name entry in name map" );
			UBITRACK_THROW( "Cannot find name entry in name map" );
		}

		// it should now point to the right element
		m_SubgraphByName.erase( it );
	}

	SubgraphList::iterator it = std::find( m_Subgraphs.begin(), m_Subgraphs.end(), subgraph );
	if ( it == m_Subgraphs.end() )
	{
		LOG4CPP_ERROR( m_Logger, "Cannot find subgraph in list" );
		UBITRACK_THROW( "Cannot find subgraph in list" );
	}
	m_Subgraphs.erase( it );
}


void UTQLDocument::exportDot( std::ostream& out )
{
	out << "digraph G { rankdir=LR; node [shape=record]; " << std::endl;
	
	// print list of all components
	for ( SubgraphList::iterator itPattern = m_Subgraphs.begin(); itPattern != m_Subgraphs.end(); ++itPattern )
	{
		out << '\"';
		if ( !(**itPattern).m_ID.empty() )
			out << (**itPattern).m_ID;
		else
			out << (**itPattern).m_Name;
		out << '\"';
			
		out << " [shape=record, label=\"{{";
		// print input ports
		int nInputs = 0;
		for ( std::map< std::string, UTQLSubgraph::EdgePtr >::iterator itEdge = (**itPattern).m_Edges.begin();
			itEdge != (**itPattern).m_Edges.end(); itEdge++ )
			if ( itEdge->second->isInput() )
			{
				if ( nInputs++ )
					out << "|";
				out << "<" << itEdge->first << "> " << itEdge->first;
			}
		
		// print component name and ID
		out << "}|" << (**itPattern).m_ID << "\\n" << (**itPattern).m_Name << "|{";
		
		// print output ports
		int nOutputs = 0;
		for ( std::map< std::string, UTQLSubgraph::EdgePtr >::iterator itEdge = (**itPattern).m_Edges.begin();
			itEdge != (**itPattern).m_Edges.end(); itEdge++ )
			if ( itEdge->second->isOutput() )
			{
				if ( nOutputs++ )
					out << "|";
				out << "<" << itEdge->first << "> " << itEdge->first;
			}
		out << "}}\"];" << std::endl;
	}
	
	// print connections
	for ( SubgraphList::iterator itPattern = m_Subgraphs.begin(); itPattern != m_Subgraphs.end(); ++itPattern )
		for ( std::map< std::string, UTQLSubgraph::EdgePtr >::iterator itEdge = (**itPattern).m_Edges.begin();
			itEdge != (**itPattern).m_Edges.end(); itEdge++ )
			if ( itEdge->second->isInput() && !itEdge->second->m_EdgeReference.empty() )
				out << '\"' << itEdge->second->m_EdgeReference.getSubgraphId() << "\":\""
					<< itEdge->second->m_EdgeReference.getEdgeName() << "\" -> \""
					<< (**itPattern).m_ID << "\":\"" << itEdge->first << "\";" << std::endl;

	// finish
	out << "}" << std::endl;
}
