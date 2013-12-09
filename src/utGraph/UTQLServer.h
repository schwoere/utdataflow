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

#ifndef __Ubitrack_Graph_UTQLServer_INCLUDED__
#define __Ubitrack_Graph_UTQLServer_INCLUDED__ __Ubitrack_Graph_UTQLServer_INCLUDED__

#include <map>
#include <list>
#include <string>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include <utDataflow.h>
#include <utGraph/Graph.h>
#include <utGraph/UTQLDocument.h>
#include <utGraph/UTQLSubgraph.h>
#include <utGraph/SRGraph.h>
#include <utGraph/UTQLSRGManager.h>
#include <utGraph/UTQLWriter.h>

#include <utGraph/Announcement.h>
#include <utGraph/AnnouncementRepository.h>

#include <log4cpp/Category.hh>
#include <utUtil/Exception.h>
#include <utUtil/Logging.h>

/**
 * @ingroup srg_algorithms
 * @file
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */

namespace Ubitrack { namespace Graph {

	class /*UTDATAFLOW_EXPORT*/ UTQLServer
	{
	public:

		UTQLServer()
			: m_pClientDataflowState( new ClientDataflowState )
			, m_logger( log4cpp::Category::getInstance( "Ubitrack.Graph.UTQLServer" ) )
		{}

		/**
		 * Process an announcement issued by a client.
		 * Client annoucements are potentially all challange-response, because
		 * every announcement may produce new responses and also new responses
		 * are only produced for client announcements.
		 * From the clients perspective, this may still appear asynchronous
		 * because the response may be triggered by an announcement from a
		 * different client.
		 *
		 * This method receives an announcement and
		 *
		 * input: subgraph the utql subgraph representing the announcement
		 * clientID: the id of the client issuing the announcement
		 */
		void processAnnouncement( boost::shared_ptr< UTQLSubgraph > subgraph,
								 std::string clientID )
		{
			// create an announcement from the subgraph
			// this mostly determines the type and assignes an announcement ID
			Announcement announcement( subgraph, clientID );

			// decide what to do depending on announcement type
			switch ( announcement.m_Type )
			{
				// a new base srg part is to be registered
			case Announcement::SRGRegistration:
				// register the srg part with the local world srg
				m_SRG.registerSRG( announcement.m_Data, clientID );

				// store the announcement in the repository
				m_Announcements.addAnnouncement( announcement );
				break;

				// a new srg pattern (client ability) is registered
			case Announcement::Pattern:

				// register the pattern with the local world srg
				m_SRG.registerPattern( announcement.m_Data, clientID );

				// and store the announcement in the repositty
				m_Announcements.addAnnouncement( announcement );
				break;

				// a new query (client demand) is registered
			case Announcement::Query:

				// register the query with the srg manager
				m_SRG.registerQuery( announcement.m_Data, clientID );

				// and store the announcement in the repository
				m_Announcements.addAnnouncement( announcement );
				break;

				// delete a previously registered announcement
			case Announcement::Deletion:

				// debug output
				LOG4CPP_TRACE( m_logger, "Deleting: " << announcement.m_ID );

				// actual deleting
				deleteAnnouncement( announcement.m_ID, clientID, subgraph->m_Name );
				break;

				// an unknown announcement was received
			case Announcement::Unknown:
				UBITRACK_THROW( "Unknown annoucement type" );
				break;
			}

		}

		/**
		 * delete a previous announcement
		 *
		 * this method removes a previously issued announement by the same
		 * client
		 * if the announcement is not found not found or is unknown
		 * an exception is thrown.
		 *
		 * input: announcementID the id of the announcement to delete
		 * clientID: the id of the client issuning the deletion
		 */
		void deleteAnnouncement( std::string announcementID, std::string clientID, std::string subgraphName )
		{
			LOG4CPP_INFO( m_logger, "deleting: " << announcementID << "for subgraph name " << subgraphName );
			
			if ( m_Announcements.hasAnnouncement( announcementID ) )
			{
				// retrieve the corresponding announcement from the repository
				// minimum required information: the type of the original announcement
				Announcement announcement = m_Announcements.getAnnouncement( announcementID );

				// switch according to announcement type
				switch ( announcement.m_Type )
				{
					case Announcement::SRGRegistration:
						LOG4CPP_TRACE( m_logger, "removing srg: " << announcement.m_Data->m_ID );
						// delete base srg
						m_SRG.deleteSRG( announcement.m_Data->m_ID );
						LOG4CPP_TRACE( m_logger, "done" );
						break;
					case Announcement::Pattern:
						LOG4CPP_TRACE( m_logger, "removing pattern: " << announcement.m_Data->m_Name );
						// delete pattern
						m_SRG.deletePattern( announcement.m_Data->m_Name, clientID );
						LOG4CPP_TRACE( m_logger, "done" );			
						break;
					case Announcement::Query:
						LOG4CPP_TRACE( m_logger, "removing query: " << announcement.m_Data->m_Name );
						// delete query
						m_SRG.deleteQuery( announcement.m_Data->m_Name, clientID );
						LOG4CPP_TRACE( m_logger, "done" );
						break;
					case Announcement::Deletion:
					case Announcement::Unknown:
						UBITRACK_THROW( "Cannot deregister announcement of type deletion or unknown" );
						break;
				}
				// remove original announcement from the repository
				m_Announcements.deleteAnnouncement( announcement );
			}
			else
			{
				LOG4CPP_TRACE( m_logger, "removing pattern: " << subgraphName );
				// delete pattern
				m_SRG.deletePattern( subgraphName, clientID );
				LOG4CPP_TRACE( m_logger, "done" );			
			}
		}


		/**
		 * completely register a client
		 *
		 * this method removes a client and all its registered announcements
		 * completely
		 *
		 * input clientid the id of the client to be removed
		 */
		void deregisterClient( std::string clientID )
		{
			// get all registerd announcement for this clientid
			std::set< std::string > allAnnouncements = m_Announcements.getAllIDsByClientID( clientID );

			// delete all announcements for this clientid
			for ( std::set< std::string >::iterator pAnnouncementID = allAnnouncements.begin();
				  pAnnouncementID != allAnnouncements.end(); ++pAnnouncementID )
			{
				// delete announcement
				deleteAnnouncement( *pAnnouncementID, clientID, "" );
			}

		}


		// -------------------------

		// map of clientid to utqldocument (list of subgraphs)
		// (who needs to do what for which query)
		typedef std::map< std::string, boost::shared_ptr< UTQLDocument > > QueryDistribution;

		/**
		 * recompute the dataflow distribution on the clients
		 *
		 * this method determines which dataflow components need to be
		 * started/stopped or reconfigured on which client, comparing
		 * the new desired configuration with the currentliy running one
		 *
		 * input: responses (?)
		 * output: new query distribution
		 */
		boost::shared_ptr< QueryDistribution > incrementalCompareDataflows( std::map< std::string, std::list< UTQLSRGManager::QueryResponse > > responses )
		{
			// distribute the subgraph across the clients
			// XXX: this should depend on the capabilities and happen in the SRGManager (at least the capability part)
			boost::shared_ptr< QueryDistribution > pNewQueryDistribution( new QueryDistribution );

			// the new state as computed
			boost::shared_ptr< ClientDataflowState > newState( new ClientDataflowState );

			// run over all clients in the response
			for ( std::map< std::string, std::list< UTQLSRGManager::QueryResponse > >::iterator pClientQRList = responses.begin();
				  pClientQRList != responses.end(); pClientQRList++ )
			{
				std::string clientID = pClientQRList->first;
				std::list< UTQLSRGManager::QueryResponse >& queryList = pClientQRList->second;

				(*pNewQueryDistribution)[ clientID ].reset( new UTQLDocument() );

				for ( std::list< UTQLSRGManager::QueryResponse >::iterator pQueryResponse = queryList.begin();
					  pQueryResponse != queryList.end(); ++pQueryResponse )
				{
					for ( std::list< boost::shared_ptr< UTQLSubgraph > >::iterator pSubgraph = pQueryResponse->m_Graphs.begin();
						  pSubgraph != pQueryResponse->m_Graphs.end(); ++pSubgraph )
					{
						std::string subgraphID = (*pSubgraph)->m_ID;

						if ( (*newState)[ clientID ].find( subgraphID ) != (*newState)[ clientID ].end() )
							// subgraph was already encountered on this run
							continue;

						// subgraphs already running on the client will not be sent again!
 						if ( (*m_pClientDataflowState)[ clientID ].find( subgraphID ) != (*m_pClientDataflowState)[ clientID ].end() )
 						{
 							// subgraph already running on client and keeps running
 							(*newState)[ clientID ].insert( subgraphID );
 							continue;
 						}

						// add subgraph to client-docuemnt and subgraph id to running state
						(*pNewQueryDistribution)[ clientID ]->addSubgraph( *pSubgraph );
						(*newState)[ clientID ].insert( subgraphID );
					}
				}
			}

			// run over all clients in the new configuration and remove edge references to edges on other clients
			for ( QueryDistribution::iterator itClient = pNewQueryDistribution->begin(); itClient != pNewQueryDistribution->end(); itClient++ )
			{
				std::set< std::string >& rClientState( (*newState)[ itClient->first ] );

				for ( UTQLDocument::SubgraphList::iterator itPattern = itClient->second->m_Subgraphs.begin(); 
					itPattern != itClient->second->m_Subgraphs.end(); itPattern++ )
					for ( UTQLSubgraph::EdgeMap::iterator itEdge = (*itPattern)->m_Edges.begin(); itEdge != (*itPattern)->m_Edges.end(); itEdge++ )
					{
						UTQLSubgraph::Edge& rEdge( *itEdge->second );

						// only iterate input edges
						if ( rEdge.isInput() && !rEdge.m_EdgeReference.empty() )
							// check if referring pattern is in state
							if ( rClientState.find( rEdge.m_EdgeReference.getSubgraphId() ) == rClientState.end() )
							{
								LOG4CPP_TRACE( m_logger, "Removing remote edge reference " 
									<< (*itPattern)->m_ID << ":" << rEdge.m_Name << " -> "
									<< rEdge.m_EdgeReference.getSubgraphId() << ":" << rEdge.m_EdgeReference.getEdgeName() );

								// Remove edge reference and store old reference in attributes (needed by network component)
								rEdge.setAttribute( "remotePatternID", rEdge.m_EdgeReference.getSubgraphId() );
								rEdge.setAttribute( "remoteEdgeName", rEdge.m_EdgeReference.getEdgeName() );
							}
					}
			}

			// run over all clients in current state and compare running subgraphs to running subgraphs in new state
			// all disapeared need to be deleted
			for ( ClientDataflowState::iterator pClientSubgraphs = m_pClientDataflowState->begin();
				  pClientSubgraphs != m_pClientDataflowState->end(); ++pClientSubgraphs )
			{
				// check if client got erases completely
				std::string clientID = pClientSubgraphs->first;

				if ( pNewQueryDistribution->find( clientID ) == pNewQueryDistribution->end() )
				{
					(*pNewQueryDistribution)[ clientID ].reset( new UTQLDocument() );
				}

				// is the client present in the new dataflow at all?
				bool newHasClient = ( newState->find( clientID ) != newState->end() );

				for ( std::set< std::string >::iterator pSubgraphID = pClientSubgraphs->second.begin();
					  pSubgraphID != pClientSubgraphs->second.end(); ++pSubgraphID )
				{
					if ( !newHasClient || ( (*newState)[ clientID ].find( *pSubgraphID ) == (*newState)[ clientID ].end() ) )
					{
						// subgraph no longer needed on client
						boost::shared_ptr< UTQLSubgraph > deletedSubgraph( new UTQLSubgraph() );
						deletedSubgraph->m_Name = *pSubgraphID;
						deletedSubgraph->m_ID = *pSubgraphID;
						(*pNewQueryDistribution)[ clientID ]->addSubgraph( deletedSubgraph );
					}

				}

			}

			// stroe new dataflow state as current
			m_pClientDataflowState = newState;

			return pNewQueryDistribution;
		}

		// -------------------------

		// generate the new document map
		boost::shared_ptr< std::map< std::string, boost::shared_ptr< UTQLDocument > > > generateDocuments()
		{
			std::map< std::string, std::list< UTQLSRGManager::QueryResponse > > responses = recomputeAllQueries();

			LOG4CPP_TRACE( m_logger, "Size: " << responses.size() );

			boost::shared_ptr< QueryDistribution > pDocumentMap = incrementalCompareDataflows( responses );

			return pDocumentMap;
		}

		// convert document map into strings to be sent
		boost::shared_ptr< std::map< std::string, std::string > > generateResponses()
		{
			boost::shared_ptr< QueryDistribution > pDocumentMap = generateDocuments();

			// convert document into string..
			boost::shared_ptr< std::map< std::string, std::string > > processedResponses( new std::map< std::string, std::string > );

			for (std::map< std::string, boost::shared_ptr< UTQLDocument > >::iterator it = pDocumentMap->begin();
				 it != pDocumentMap->end(); ++it )
			{
				std::string xmlDocument = UTQLWriter::processDocument( it->second );
				(*processedResponses)[ it->first ] = xmlDocument;
			}

			return processedResponses;
		}


		// perform pattern matching and determine which queries can
		// be answered
		std::map< std::string, std::list< UTQLSRGManager::QueryResponse > > recomputeAllQueries()
		{
			// apply all patterns n times or until no patterns are instantiated any more
			for ( unsigned i = 0; i < 10; i++ )
				if ( !m_SRG.applyAllPatterns() )
					break;

			// print logging information
			m_SRG.logCurrentSRG();

			return m_SRG.processQueries();
		}

		// store all announcements
		AnnouncementRepository m_Announcements;

		// store all announement IDs from a client
		std::map< std::string, std::set< std::string > > m_ClientAnnouncements;

		// Keep the main work SRG
		UTQLSRGManager m_SRG;

		// map of clientid to sets of subgraph ids
		// what runs on which client currently
		typedef std::map< std::string, std::set< std::string > > ClientDataflowState;

		boost::shared_ptr< ClientDataflowState > m_pClientDataflowState;
		
		log4cpp::Category& m_logger;
	};

}}

#endif // __Ubitrack_Graph_UTQLServer_INCLUDED__
