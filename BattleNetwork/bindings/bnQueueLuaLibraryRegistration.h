/*! \file bnQueueLibraryRegistration.h */

#pragma once

#include "../bnLuaLibraryPackageManager.h"
#include "bnLuaLibrary.h"

#ifndef __APPLE__
#include <filesystem>
#endif

static inline void QueueLuaLibraryRegistration( LuaLibraryPackageManager& packageManager )
{
    #if defined(BN_MOD_SUPPORT) && !defined(__APPLE__)
    std::string base_path = "resources/mods/libs/";
    std::map< std::string, bool > ignoreList;

    for( const auto& entry : std::filesystem::directory_iterator( base_path ) )
    {
        auto full_path = std::filesystem::absolute( entry ).string();

        if( ignoreList.find( full_path ) != ignoreList.end() )
            continue;

        try
        {
            if( size_t pos = full_path.find( ".zip" ); pos != std::string::npos )
            {
                if( auto res = packageManager.LoadPackageFromZip<LuaLibrary>( full_path ); res.is_error() )
                {
                    Logger::Logf( "%s", res.error_cstr() );
                    continue;
                }

                ignoreList[ full_path ] = true;
                ignoreList[ full_path.substr( 0, pos ) ] = true;
            }
            else
            {
                if( auto res = packageManager.LoadPackageFromDisk<LuaLibrary>( full_path ); res.is_error() )
                {
                    Logger::Logf( "%s", res.error_cstr() );
                    continue;
                }

                ignoreList[ full_path ] = true;
                ignoreList[ full_path + ".zip" ] = true;
            }
        }
        catch(const std::runtime_error& e)
        {
            Logger::Logf( "Lua Library Package Loader: unknown error: %s", e.what() );
        }
        
    }

    #endif
}