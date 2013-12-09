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

#ifndef __Ubitrack_Graph_AnnouncementRepository_INCLUDED__
#define __Ubitrack_Graph_AnnouncementRepository_INCLUDED__ __Ubitrack_Graph_AnnouncementRepository_INCLUDED__

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <utDataflow.h>

#include <utGraph/Announcement.h>

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

	class AnnouncementRepository
	{
	public:
		AnnouncementRepository()
		{}

		bool hasAnnouncement( std::string announcementID )
		{
			return ( m_Store.find( announcementID ) != m_Store.end() );
		}

		bool hasAnnouncement( Announcement announcement )
		{
			return hasAnnouncement( announcement.m_ID );
		}

		void addAnnouncement( Announcement announcement )
		{
			if ( hasAnnouncement( announcement ) )
			{
				UBITRACK_THROW( "Announcement already registered: " + announcement.m_ID );
			}

			m_Store[ announcement.m_ID ] = announcement;
			m_BackStore[ announcement.m_ClientID ].insert( announcement.m_ID );
		}


		Announcement getAnnouncement( std::string announcementID )
		{
			if ( !hasAnnouncement( announcementID ) )
			{
				UBITRACK_THROW( "Trying to get announcement that is not stored: " + announcementID );
			}

			return m_Store[ announcementID ];
		}

		Announcement getAnnouncement( Announcement announcement )
		{
			return getAnnouncement( announcement.m_ID );
		}

		void deleteAnnouncement( std::string announcementID )
		{
			if ( !hasAnnouncement( announcementID ) )
			{
				UBITRACK_THROW( "Trying to delete announcement that is not stored: " + announcementID );
			}

			std::string clientID = getAnnouncement( announcementID ).m_ClientID;
			m_BackStore[ clientID ].erase( announcementID );
			if ( m_BackStore[ clientID ].empty() )
			{
				m_BackStore.erase( clientID );
			}

			m_Store.erase( announcementID );
		}

		void deleteAnnouncement( Announcement announcement )
		{
			deleteAnnouncement( announcement.m_ID );
		}

		std::set< std::string > getAllIDsByClientID( std::string clientID )
		{
			if ( m_BackStore.find( clientID ) == m_BackStore.end() )
			{
				UBITRACK_THROW( "Trying to get all IDs by unknown client ID: " + clientID );
			}

			return m_BackStore[ clientID ];
		}

	private:
		// keep a mep of all announcements by announcement ID
		std::map< std::string, Announcement > m_Store;

		// keep a backmap of all clientIDs to announcements
		std::map< std::string, std::set< std::string > > m_BackStore;
	};
}}

#endif // __Ubitrack_Graph_AnnouncementRepository_INCLUDED__
