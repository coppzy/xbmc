/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeSong.h"
#include "QueryParams.h"
#include "ServiceBroker.h"
#include "media/MetadataManager.h"
#include "music/MusicDatabase.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeSong::CDirectoryNodeSong(const std::string& strName,
                                       CDirectoryNode* pParent,
                                       const std::string& strOrigin)
    : CDirectoryNode(NODE_TYPE_SONG, strName, pParent, strOrigin)
{
}

bool CDirectoryNodeSong::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  std::string strBaseDir = BuildPath();

  bool bSuccess = CServiceBroker::GetMetadataManager().GetSongs(
      strBaseDir, items, params.GetGenreId(), params.GetArtistId(), params.GetAlbumId(),
      params.GetPlaylistId());

  musicdatabase.Close();

  return bSuccess;
}
