/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



#include "oox/helper/zipstorage.hxx"

#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/embed/XTransactedObject.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/storagehelper.hxx>
#include "oox/helper/helper.hxx"

namespace oox {

// ============================================================================

using namespace ::com::sun::star::container;
using namespace ::com::sun::star::embed;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;

using ::rtl::OUString;

// ============================================================================

ZipStorage::ZipStorage( const Reference< XComponentContext >& rxContext, const Reference< XInputStream >& rxInStream ) :
    StorageBase( rxInStream, false )
{
    OSL_ENSURE( rxContext.is(), "ZipStorage::ZipStorage - missing component context" );
    // create base storage object
    if( rxContext.is() ) try
    {
        /*  #i105325# ::comphelper::OStorageHelper::GetStorageFromInputStream()
            cannot be used here as it will open a storage with format type
            'PackageFormat' that will not work with OOXML packages.

            #161971# The MS-document storages should always be opened in repair
            mode to ignore the format errors and get so much info as possible.
            I hate this solution, but it seems to be the only consistent way to
            handle the MS documents.

            TODO: #i105410# switch to 'OFOPXMLFormat' and use its
            implementation of relations handling.
         */
        Reference< XMultiServiceFactory > xFactory( rxContext->getServiceManager(), UNO_QUERY_THROW );
        mxStorage = ::comphelper::OStorageHelper::GetStorageOfFormatFromInputStream(
            ZIP_STORAGE_FORMAT_STRING, rxInStream, xFactory,
            sal_False );    // DEV300_m80: Was sal_True, but DOCX and others did not load
    }
    catch( Exception& )
    {
    }
}

ZipStorage::ZipStorage( const Reference< XComponentContext >& rxContext, const Reference< XStream >& rxStream ) :
    StorageBase( rxStream, false )
{
    OSL_ENSURE( rxContext.is(), "ZipStorage::ZipStorage - missing component context" );
    // create base storage object
    if( rxContext.is() ) try
    {
        Reference< XMultiServiceFactory > xFactory( rxContext->getServiceManager(), UNO_QUERY_THROW );
        const sal_Int32 nOpenMode = ElementModes::READWRITE | ElementModes::TRUNCATE;
        mxStorage = ::comphelper::OStorageHelper::GetStorageOfFormatFromStream(
            OFOPXML_STORAGE_FORMAT_STRING, rxStream, nOpenMode, xFactory, sal_True );
    }
    catch( Exception& )
    {
        OSL_ENSURE( false, "ZipStorage::ZipStorage - cannot open output storage" );
    }
}

ZipStorage::ZipStorage( const ZipStorage& rParentStorage, const Reference< XStorage >& rxStorage, const OUString& rElementName ) :
    StorageBase( rParentStorage, rElementName, rParentStorage.isReadOnly() ),
    mxStorage( rxStorage )
{
    OSL_ENSURE( mxStorage.is(), "ZipStorage::ZipStorage - missing storage" );
}

ZipStorage::~ZipStorage()
{
}

bool ZipStorage::implIsStorage() const
{
    return mxStorage.is();
}

Reference< XStorage > ZipStorage::implGetXStorage() const
{
    return mxStorage;
}

void ZipStorage::implGetElementNames( ::std::vector< OUString >& orElementNames ) const
{
    Sequence< OUString > aNames;
    if( mxStorage.is() ) try
    {
        aNames = mxStorage->getElementNames();
        if( aNames.getLength() > 0 )
            orElementNames.insert( orElementNames.end(), aNames.getConstArray(), aNames.getConstArray() + aNames.getLength() );
    }
    catch( Exception& )
    {
    }
}

StorageRef ZipStorage::implOpenSubStorage( const OUString& rElementName, bool bCreateMissing )
{
    Reference< XStorage > xSubXStorage;
    bool bMissing = false;
    if( mxStorage.is() ) try
    {
        // XStorage::isStorageElement may throw various exceptions...
        if( mxStorage->isStorageElement( rElementName ) )
            xSubXStorage = mxStorage->openStorageElement(
                rElementName, ::com::sun::star::embed::ElementModes::READ );
    }
    catch( NoSuchElementException& )
    {
        bMissing = true;
    }
    catch( Exception& )
    {
    }

    if( bMissing && bCreateMissing ) try
    {
        xSubXStorage = mxStorage->openStorageElement(
            rElementName, ::com::sun::star::embed::ElementModes::READWRITE );
    }
    catch( Exception& )
    {
    }

    StorageRef xSubStorage;
    if( xSubXStorage.is() )
        xSubStorage.reset( new ZipStorage( *this, xSubXStorage, rElementName ) );
    return xSubStorage;
}

Reference< XInputStream > ZipStorage::implOpenInputStream( const OUString& rElementName )
{
    Reference< XInputStream > xInStream;
    if( mxStorage.is() ) try
    {
        xInStream.set( mxStorage->openStreamElement( rElementName, ::com::sun::star::embed::ElementModes::READ ), UNO_QUERY );
    }
    catch( Exception& )
    {
        // Bug 126720 - sometimes the relationship says the file is "sharedStrings.xml" but the file is actually "SharedStrings.xml".
        // Do a case-insensitive search:
        ::com::sun::star::uno::Sequence< ::rtl::OUString > aNames = mxStorage->getElementNames( );
        for ( sal_Int32 i = 0; i < aNames.getLength(); i++ )
        {
            if ( aNames[i].equalsIgnoreAsciiCase( rElementName ) )
            {
                try
                {
                    xInStream.set( mxStorage->openStreamElement( aNames[i], ::com::sun::star::embed::ElementModes::READ ), UNO_QUERY );
                }
                catch( Exception& )
                {
                }
                break;
            }
        }
    }
    return xInStream;
}

Reference< XOutputStream > ZipStorage::implOpenOutputStream( const OUString& rElementName )
{
    Reference< XOutputStream > xOutStream;
    if( mxStorage.is() ) try
    {
        xOutStream.set( mxStorage->openStreamElement( rElementName, ::com::sun::star::embed::ElementModes::READWRITE ), UNO_QUERY );
    }
    catch( Exception& )
    {
    }
    return xOutStream;
}

void ZipStorage::implCommit() const
{
    try
    {
        Reference< XTransactedObject >( mxStorage, UNO_QUERY_THROW )->commit();
    }
    catch( Exception& )
    {
    }
}

// ============================================================================

} // namespace oox
