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
 * UTQL Input
 *
 * This reads an UTQL document (see Techreport) from an input stream
 * and creates an UTQL document consisting of UTQL subgraphs
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */

#ifndef __Ubitrack_Graph_UTQLReader_INCLUDED__
#define __Ubitrack_Graph_UTQLReader_INCLUDED__ __Ubitrack_Graph_UTQLReader_INCLUDED__

#include <iostream>
#include <sstream>
#include <stdexcept>

#include <utDataflow.h>
#include "tinyxml.h"

#include <utGraph/UTQLDocument.h>

namespace Ubitrack { namespace Graph {

    /**
	 * @ingroup srg_algorithms
	 * UTQL reader class
	 *
	 * This is an interface to read an UTQL document from an input
	 * stream. Just implements static functions that do all the work.
     * Uses TinyXML for now.
     */
    class UTDATAFLOW_EXPORT UTQLReader
    {
    public:
        /**
		 * Read an UTQL document
		 *
		 * Allocates a new document and returns a shared pointer.
         *
         * @param input the stream from which to read the SRG
         * @return shared pointer to a new and filled UTQL document
		 * @throws Util::Exception if anything goes wrong
         */
        static boost::shared_ptr< UTQLDocument > processInput( std::istream& input );
    protected:
		enum Section { SectionInput, SectionOutput };
		typedef boost::shared_ptr< UTQLSubgraph > SubgraphPtr;
		typedef boost::shared_ptr< UTQLDocument > DocumentPtr;

		static void handleSubgraph( TiXmlHandle& xmlHandleSubgraph, SubgraphPtr subgraph );
		static void handleGraph( TiXmlHandle& xmlHandle, Section section, SubgraphPtr subgraph );
		static void handleAttributes( TiXmlHandle& xmlHandleBase, KeyValueAttributes* attribs );
		static void handleAttributeExpressions( TiXmlHandle& xmlHandleBase, InOutAttribute& attribs );
		static void handlePredicates( TiXmlHandle& xmlHandleBase, PredicateList* attribs );
	private:

		/**
		 * Private constructor. No need to create instances of this class.
		 */
		UTQLReader()
		{
		}

    };

}}




#endif // __Ubitrack_Graph_XMLReader_INCLUDED__
