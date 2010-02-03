/*
 * Copyright (C) 2010 Company 100, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "OwnPtrBrew.h"

#include <AEEBitmap.h>
#include <AEEFile.h>
#include <AEEStdLib.h>

namespace WTF {

template <> void freeOwnedPtrBrew<IFileMgr>(IFileMgr* ptr)
{
    if (ptr)
        IFILEMGR_Release(ptr);
}

template <> void freeOwnedPtrBrew<IFile>(IFile* ptr)
{
    if (ptr)
        IFILE_Release(ptr);
}

template <> void freeOwnedPtrBrew<IBitmap>(IBitmap* ptr)
{
    if (ptr)
        IBitmap_Release(ptr);
}

template <typename T> void freeOwnedPtrBrew(T* ptr)
{
    FREEIF(ptr);
}

} // namespace WTF
