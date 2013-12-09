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
 * @ingroup srg_algorithms
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */

#include "UTQLWriter.h"

#include <log4cpp/Category.hh>
#include <utUtil/Exception.h>
#include <utUtil/Logging.h>
#include <utGraph/PredicateParser.h>

#include "tinyxml.h"

static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Graph.UTQLReader" ) );

namespace Ubitrack { namespace Graph {

	std::string UTQLWriter::processDocument( boost::shared_ptr< UTQLDocument > doc )
	{
		TiXmlDocument xmlDocument;

		TiXmlDeclaration* xmlDecl = new TiXmlDeclaration( "1.0", "", "" );
		xmlDocument.LinkEndChild( xmlDecl );

		TiXmlElement* xmlElementUTQL = new TiXmlElement( doc->isRequest() ? "UTQLRequest" : "UTQLResponse" );
		xmlDocument.LinkEndChild( xmlElementUTQL );
		xmlElementUTQL->SetAttribute( "xmlns", "http://ar.in.tum.de/ubitrack/utql" );
		xmlElementUTQL->SetAttribute( "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" );

		for ( UTQLDocument::SubgraphList::iterator it = doc->m_Subgraphs.begin();
			  it != doc->m_Subgraphs.end(); ++it )
		{
			TiXmlElement* xmlElementSubgraph = new TiXmlElement( "Pattern" );
			xmlElementUTQL->LinkEndChild( xmlElementSubgraph );
			handleSubgraph( xmlElementSubgraph, *it );
		}

		TiXmlPrinter printer;
		printer.SetIndent( "  " );

		xmlDocument.Accept( &printer );
		return printer.CStr();
	}

	void UTQLWriter::handleSubgraph( TiXmlElement* xmlElementSubgraph, boost::shared_ptr< UTQLSubgraph > subgraph )
	{
		if ( subgraph->m_Name.size() != 0 )
		{
			xmlElementSubgraph->SetAttribute( "name", subgraph->m_Name );
		}
		if ( subgraph->m_ID.size() != 0 )
		{
			xmlElementSubgraph->SetAttribute( "id", subgraph->m_ID );
		}

		TiXmlElement* xmlElementInput = new TiXmlElement( "Input" );
		TiXmlElement* xmlElementOutput = new TiXmlElement( "Output" );

		bool hasInput = false;
		bool hasOutput = false;

		for (std::map< std::string, UTQLSubgraph::NodePtr >::iterator it = subgraph->m_Nodes.begin();
			 it != subgraph->m_Nodes.end(); ++it )
		{
			UTQLSubgraph::NodePtr node = it->second;

			TiXmlElement* xmlElementNode = new TiXmlElement( "Node" );
			xmlElementNode->SetAttribute( "name", node->m_Name );

			if ( node->m_QualifiedName.size() != 0 )
			{
				xmlElementNode->SetAttribute( "id", node->m_QualifiedName );
			}

			handleAttributes( xmlElementNode, *node );
			// predicates

			if ( node->isInput() )
			{
				hasInput = true;
				xmlElementInput->LinkEndChild( xmlElementNode );
			}
			else if ( node->isOutput() )
			{
				hasOutput = true;
				xmlElementOutput->LinkEndChild( xmlElementNode );
			}
		}

		for (std::map< std::string, UTQLSubgraph::EdgePtr >::iterator it = subgraph->m_Edges.begin();
			 it != subgraph->m_Edges.end(); ++it )
		{
			UTQLSubgraph::EdgePtr edge = it->second;

			TiXmlElement* xmlElementEdge = new TiXmlElement( "Edge" );

			xmlElementEdge->SetAttribute( "name", edge->m_Name );
			xmlElementEdge->SetAttribute( "source", edge->m_Source.lock()->m_Name );
			xmlElementEdge->SetAttribute( "destination", edge->m_Target.lock()->m_Name );

			if ( !edge->m_EdgeReference.empty() )
			{
				xmlElementEdge->SetAttribute( "pattern-ref", edge->m_EdgeReference.getSubgraphId() );
				xmlElementEdge->SetAttribute( "edge-ref", edge->m_EdgeReference.getEdgeName() );
			}

			handleAttributes( xmlElementEdge, *edge );
			// predicates

			if ( edge->isInput() )
			{
				hasInput = true;
				xmlElementInput->LinkEndChild( xmlElementEdge );
			}
			else if ( edge->isOutput() )
			{
				hasOutput = true;
				xmlElementOutput->LinkEndChild( xmlElementEdge );
			}
		}

		if ( hasInput )
		{
			xmlElementSubgraph->LinkEndChild( xmlElementInput );
		}
		else
		{
			delete xmlElementInput;
		}

		if ( hasOutput )
		{
			xmlElementSubgraph->LinkEndChild( xmlElementOutput );
		}
		else
		{
			delete xmlElementOutput;
		}

		if ( subgraph->m_DataflowConfiguration.getXML() != 0 )
		{
			TiXmlElement* xmlElementDataflow = subgraph->m_DataflowConfiguration.getXML()->Clone()->ToElement();
			xmlElementSubgraph->LinkEndChild( xmlElementDataflow );
		}

	}

	void UTQLWriter::handleAttributes( TiXmlElement* xmlElement, KeyValueAttributes& attribs )
	{
		for ( KeyValueAttributes::AttributeMapType::const_iterator it = attribs.map().begin();
			 it != attribs.map().end(); ++it )
		{
			TiXmlElement* xmlElementAttribute = 0;
			if ( it->second.getXML() != 0 )
			{
				xmlElementAttribute = it->second.getXML()->Clone()->ToElement();
			}
			else
			{
				xmlElementAttribute = new TiXmlElement( "Attribute" );
				xmlElementAttribute->SetAttribute( "name", it->first );
				xmlElementAttribute->SetAttribute( "value", it->second.getText() );
			}
			xmlElement->LinkEndChild( xmlElementAttribute );
		}
	}


// 	void UTQLReader::handleSubgraph( TiXmlHandle& xmlHandleSubgraph, SubgraphPtr subgraph )
// 	{
// 		TiXmlHandle xmlHandleInput = xmlHandleSubgraph.FirstChild( "Input" );
// 		TiXmlHandle xmlHandleOutput = xmlHandleSubgraph.FirstChild( "Output" );
// 		TiXmlHandle xmlHandleConfig = xmlHandleSubgraph.FirstChild( "DataflowConfiguration" );

// 		if ( xmlHandleInput.Element() )
// 		{
// 			handleGraph( xmlHandleInput, SectionInput, subgraph );
// 		}

// 		if ( xmlHandleOutput.Element() )
// 		{
// 			handleGraph( xmlHandleOutput, SectionOutput, subgraph );
// 		}

// 		handleAttributes( xmlHandleConfig, &subgraph->m_DataflowAttributes );

// 		if ( xmlHandleConfig.Element() )
// 		{
// 			boost::shared_ptr< TiXmlElement > config = boost::shared_ptr< TiXmlElement >( xmlHandleConfig.ToElement()->Clone()->ToElement() );
// 			subgraph->m_DataflowConfiguration = AttributeValue( config );
// 		}
// 	}


// 	void UTQLReader::handlePredicates( TiXmlHandle& xmlHandleBase, PredicateList* attribs )
// 	{
// 		// handle predicates
// 		TiXmlHandle xmlHandlePredicate = xmlHandleBase.Child( "Predicate", 0 );
// 		for (int child=0; xmlHandlePredicate.Element(); xmlHandlePredicate = xmlHandleBase.Child( "Predicate", ++child ) )
// 		{
// 			TiXmlElement* xmlElementPredicate = xmlHandlePredicate.ToElement();

// 			try
// 			{
// 				boost::shared_ptr< Predicate > predicate = parsePredicate( xmlElementPredicate->GetText() );
// 				attribs->addPredicate( predicate );
// 			}
// 			catch ( Ubitrack::Util::Exception &e )
// 			{
// 				LOG4CPP_ERROR( logger, "Error parsing predicate: " << xmlElementPredicate->GetText() );
// 				UBITRACK_THROW( "Error parsing predicate" );
// 			}
// 		}
// 	}

// 	void UTQLReader::handleGraph( TiXmlHandle& xmlHandle, Section section, SubgraphPtr subgraph )
// 	{
// 		// first find all nodes
// 		TiXmlHandle xmlHandleNode = xmlHandle.Child( "Node", 0 );
// 		for (int child=0; xmlHandleNode.Element(); xmlHandleNode = xmlHandle.Child( "Node", ++child ) )
// 		{
// 			TiXmlElement* xmlElementNode = xmlHandleNode.ToElement();

// 			std::string name;
// 			if ( !xmlElementNode->Attribute( "name" ) )
// 			{
// 				LOG4CPP_ERROR( logger, "UTQL Node without name" );
// 				UBITRACK_THROW( "UTQL Node without name" );
// 			}
// 			name = xmlElementNode->Attribute( "name" );

// 			std::string id;
// 			if ( xmlElementNode->Attribute( "id" ) )
// 			{
// 				id = xmlElementNode->Attribute( "id" );
// 			}

// 			UTQLSubgraph::GraphNodeAttributes::Tag tag = ( section == SectionInput ) ? UTQLSubgraph::GraphNodeAttributes::Input : UTQLSubgraph::GraphNodeAttributes::Output;
// 			UTQLSubgraph::NodePtr node = subgraph->addNode( name, tag );

// 			// one of these three mechanisms should be enough
// 			node->m_QualifiedName = id;

// // 			if ( id.length() != 0 )
// // 			{
// // 				subgraph->registerNodeID( id, node );
// // 			}

// 			node->setAttribute( "id", AttributeValue( id ) );

// 			// handle attributes
// 			handleAttributes( xmlHandleNode, node.get() );
// 			handlePredicates( xmlHandleNode, node.get() );
// 		}

// 		// find all edges
// 		TiXmlHandle xmlHandleEdge = xmlHandle.Child( "Edge", 0 );
// 		for (int child=0; xmlHandleEdge.Element(); xmlHandleEdge = xmlHandle.Child( "Edge", ++child ) )
// 		{
// 			TiXmlElement* xmlElementEdge = xmlHandleEdge.ToElement();

// 			std::string name;
// 			if ( !xmlElementEdge->Attribute( "name" ) )
// 			{
// 				LOG4CPP_ERROR( logger, "UTQL Edge without name" );
// 				UBITRACK_THROW( "UTQL Edge without name" );
// 			}
// 			name = xmlElementEdge->Attribute( "name" );

// 			std::string source;
// 			if ( !xmlElementEdge->Attribute( "source" ) )
// 			{
// 				LOG4CPP_ERROR( logger, "UTQL Edge without source" );
// 				UBITRACK_THROW( "UTQL Edge without source" );
// 			}
// 			source = xmlElementEdge->Attribute( "source" );

// 			std::string destination;
// 			if ( !xmlElementEdge->Attribute( "destination" ) )
// 			{
// 				LOG4CPP_ERROR( logger, "UTQL Edge without destination" );
// 				UBITRACK_THROW( "UTQL Edge without destination" );
// 			}
// 			destination = xmlElementEdge->Attribute( "destination" );

// 			UTQLSubgraph::GraphEdgeAttributes::Tag tag = ( section == SectionInput ) ? UTQLSubgraph::GraphEdgeAttributes::Input : UTQLSubgraph::GraphEdgeAttributes::Output;

// 			if ( !( subgraph->hasNode( source ) && subgraph->hasNode( destination ) ) )
// 			{
// 				LOG4CPP_ERROR( logger, "UTQL Edge: source or destination not in graph: " << source << " -> " << destination );
// 				UBITRACK_THROW( "UTQL Edge: source or destination not in graph" );
// 			}
// 			UTQLSubgraph::EdgePtr edge = subgraph->addEdge( name, source, destination, tag );

// 			// handle attributes

// 			handleAttributes( xmlHandleEdge, edge.get() );
// 			handlePredicates( xmlHandleEdge, edge.get() );

// // 			TiXmlHandle xmlHandleAttribute = xmlHandleEdge.Child( "Attribute", 0 );
// // 			for (int child=0; xmlHandleAttribute.Element(); xmlHandleAttribute = xmlHandleEdge.Child( "Attribute", ++child ) )
// // 			{
// // 				TiXmlElement* xmlElementAttribute = xmlHandleAttribute.ToElement();

// // 				std::string name;
// // 				if ( !xmlElementAttribute->Attribute( "name" ) )
// // 				{
// // 					LOG4CPP_ERROR( logger, "UTQL Node Attribute without name" );
// // 					UBITRACK_THROW( "UTQL Node Attribute without name" );
// // 				}
// // 				name = xmlElementAttribute->Attribute( "name" );

// // 				if ( xmlElementAttribute->Attribute( "value" ) )
// // 				{
// // 					std::string value = xmlElementAttribute->Attribute( "value" );
// // 					edge->setAttribute( name, AttributeValue( value ) );
// // 				}
// // 				else
// // 				{
// // 					edge->setAttribute( name, AttributeValue( boost::shared_ptr< TiXmlElement >( xmlElementAttribute->Clone()->ToElement() ) ) );
// // 				}
// // 			}

// 			std::string ref;
// 			if ( xmlElementEdge->Attribute( "ref" ) )
// 			{
// 				ref = xmlElementEdge->Attribute( "ref" );
// 			}
// 			edge->m_EdgeReference = ref;

// 		}
// 	}

}}
