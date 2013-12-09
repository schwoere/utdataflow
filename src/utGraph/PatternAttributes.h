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

#ifndef __Ubitrack_Graph_PatternAttributes_INCLUDED__
#define __Ubitrack_Graph_PatternAttributes_INCLUDED__ __Ubitrack_Graph_PatternAttributes_INCLUDED__

#include <set>
#include <algorithm>

namespace Ubitrack { namespace Graph {

	class PatternAttributes
	{
	public:

		std::set< std::string > m_InformationSources;

		bool hasEqualInformation( const PatternAttributes& x )
		{
			return m_InformationSources == x.m_InformationSources;

			//std::set< std::string > result;
			//std::set_symmetric_difference( m_InformationSources.begin(),
			//							   m_InformationSources.end(),
			//							   x.m_InformationSources.begin(),
			//							   x.m_InformationSources.end(),
			//							   std::insert_iterator< std::set< std::string > >( result, result.begin() ) );

// 			std::set< std::string > result_intersection;
// 			std::set_intersection( m_InformationSources.begin(),
// 								   m_InformationSources.end(),
// 								   x.m_InformationSources.begin(),
// 								   x.m_InformationSources.end(),
// 								   std::insert_iterator< std::set< std::string > >( result_intersection, result_intersection.begin() ) );

// 			bool containedA = ( result_intersection == m_InformationSources );
// 			bool containedB = ( result_intersection == x.m_InformationSources );

// 			std::cerr << "A: " << containedA << " B: " << containedB << std::endl;

// 			std::cerr << "This: " << std::endl;
// 			for ( std::set< std::string >::iterator it = m_InformationSources.begin();
// 				  it != m_InformationSources.end(); ++it )
// 			{
// 				std::cerr << *it << std::endl;
// 			}
// 			std::cerr << "Other: " << std::endl;
// 			for ( std::set< std::string >::iterator it = x.m_InformationSources.begin();
// 				  it != x.m_InformationSources.end(); ++it )
// 			{
// 				std::cerr << *it << std::endl;
// 			}

			//return ( ( result.size() == 0 ) ); //|| ( result_intersection.size() != 0 ) );

			// return containedB;
		}
	};
}}

#endif // __Ubitrack_Graph_PatternAttributes_INCLUDED__
