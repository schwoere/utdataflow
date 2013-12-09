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
 * UTQL Output
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */

#ifndef __Ubitrack_Graph_UTQLWriter_INCLUDED__
#define __Ubitrack_Graph_UTQLWriter_INCLUDED__ __Ubitrack_Graph_UTQLWriter_INCLUDED__

#include <iostream>
#include <sstream>
#include <stdexcept>

#include <utDataflow.h>
#include "tinyxml.h"

#include <utGraph/UTQLDocument.h>

namespace Ubitrack { namespace Graph {

    class UTDATAFLOW_EXPORT UTQLWriter
    {
    public:
		static std::string processDocument( boost::shared_ptr< UTQLDocument > );
    protected:
// 		enum Section { SectionInput, SectionOutput };

 		static void handleSubgraph( TiXmlElement* xmlElementSubgraph, boost::shared_ptr< UTQLSubgraph > subgraph );
 		static void handleAttributes( TiXmlElement* xmlElement, KeyValueAttributes& attribs );
// 		static void handleGraph( TiXmlHandle& xmlHandle, Section section, SubgraphPtr subgraph );
// 		static void handlePredicates( TiXmlHandle& xmlHandleBase, PredicateList* attribs );
	private:

		/**
		 * Private constructor. No need to create instances of this class.
		 */
		UTQLWriter()
		{
		}

    };

}}




#endif // __Ubitrack_Graph_UTQLWriter_INCLUDED__
