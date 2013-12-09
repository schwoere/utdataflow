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
 * @file
 * Announcement class
 *
 * @author Manuel Huber <manuel.huber@in.tum.de>
 */


#ifndef __Ubitrack_Graph_Announcement_INCLUDED__
#define __Ubitrack_Graph_Announcement_INCLUDED__ __Ubitrack_Graph_Announcement_INCLUDED__

#include <string>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include <utGraph/Graph.h>
#include <utGraph/UTQLSubgraph.h>

#include <log4cpp/Category.hh>
#include <utUtil/Exception.h>
#include <utUtil/Logging.h>

namespace Ubitrack { namespace Graph {

		/**
		 * @ingroup srg_algorithms
		 * Datastructure to express all kinds of data sent to or from
		 * the clients.
		 */
	class Announcement
	{
	public:
		/**
		 * @ingroup srg_algorithms
		 * The Type of the announcement
		 */
		enum AnnouncementType { Unknown, SRGRegistration, Pattern, Query, Deletion };

		Announcement( boost::shared_ptr< UTQLSubgraph > data, std::string clientID )
			: m_Type ( Unknown )
			, m_Data ( data )
			, m_ClientID ( clientID )
		{
			determineType();

			m_ID = m_ClientID+":"+m_Data->m_ID;
		}

		Announcement()
			: m_Type ( Unknown )
		{}

	private:
		void determineType()
		{
			// determine type of utql annoucement

			bool hasInput = false;
			bool hasOutput = false;

			for (std::map< std::string, UTQLSubgraph::NodePtr >::iterator it = m_Data->m_Nodes.begin();
				 it != m_Data->m_Nodes.end(); ++it )
			{

				UTQLSubgraph::NodePtr node = it->second;

				if ( node->isInput() )
				{
					hasInput = true;
				}
				else if ( node->isOutput() )
				{
					hasOutput = true;
				}

				if ( hasInput && hasOutput )
				{
					break;
				}
			}

			for (std::map< std::string, UTQLSubgraph::EdgePtr >::iterator it = m_Data->m_Edges.begin();
				 it != m_Data->m_Edges.end(); ++it )
			{
				UTQLSubgraph::EdgePtr edge = it->second;

				if ( edge->isInput() )
				{
					hasInput = true;
				}
				else if ( edge->isOutput() )
				{
					hasOutput = true;
				}

				if ( hasInput && hasOutput )
				{
					break;
				}
			}

			// no input section -> srg registration
			if ( !hasInput && hasOutput )
			{
				m_Type = SRGRegistration;
			}
			// input and output -> pattern
			else if ( hasInput && hasOutput )
			{
				m_Type = Pattern;
			}
			// input and no output -> query
			else if ( hasInput && !hasOutput )
			{
				m_Type = Query;
			}
			else if ( !hasInput && !hasOutput )
			{
				m_Type = Deletion;
			}
		}

	public:
		AnnouncementType m_Type;
		boost::shared_ptr< UTQLSubgraph > m_Data;
		std::string m_ClientID;
		std::string m_ID;
	};
}}

#endif // __Ubitrack_Graph_UTQLServer_INCLUDED__
