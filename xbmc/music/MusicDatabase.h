/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
 \file MusicDatabase.h
\brief
*/

#include <utility>
#include <vector>

#include "addons/Scraper.h"
#include "Album.h"
#include "dbwrappers/Database.h"
#include "MusicDbUrl.h"
#include "MediaSource.h"
#include "settings/LibExportSettings.h"
#include "utils/SortUtils.h"
#include "dbwrappers/CommonDatabase.h"
#include <odb/odb_gen/ODBSong.h>
#include <odb/odb_gen/ODBSong_odb.h>

class CArtist;
class CFileItem;

class CODBFile;
class CODBAlbum;
class CODBPerson;
class CODBSong;
class CODBRole;
class CODBPath;
class CODBPersonLink;
class ODBView_Album;
class ODBView_Music_Genres;
class ODBPlaylist;

namespace dbiplus
{
  class field_value;
  typedef std::vector<field_value> sql_record;
}

#include <set>
#include <string>

// return codes of Cleaning up the Database
// numbers are strings from strings.xml
#define ERROR_OK     317
#define ERROR_CANCEL    0
#define ERROR_DATABASE    315
#define ERROR_REORG_SONGS   319
#define ERROR_REORG_ARTIST   321
#define ERROR_REORG_OTHER   323
#define ERROR_REORG_PATH   325
#define ERROR_REORG_ALBUM   327
#define ERROR_WRITING_CHANGES  329
#define ERROR_COMPRESSING   332

#define NUM_SONGS_BEFORE_COMMIT 500

/*!
 \ingroup music
 \brief A set of std::string objects, used for CMusicDatabase
 \sa ISETPATHS, CMusicDatabase
 */
typedef std::set<std::string> SETPATHS;

/*!
 \ingroup music
 \brief The SETPATHS iterator
 \sa SETPATHS, CMusicDatabase
 */
typedef std::set<std::string>::iterator ISETPATHS;

/*!
\ingroup music
\brief A structure used for fetching music art data
\sa CMusicDatabase::GetArtForItem()
*/

typedef struct {
  std::string mediaType;
  std::string artType;
  std::string prefix;
  std::string url;
} ArtForThumbLoader;

class CGUIDialogProgress;
class CFileItemList;

/*!
 \ingroup music
 \brief Class to store and read tag information

 CMusicDatabase can be used to read and store
 tag information for faster access. It is based on
 sqlite (http://www.sqlite.org).

 Here is the database layout:
  \image html musicdatabase.png

 \sa CAlbum, CSong, VECSONGS, CMapSong, VECARTISTS, VECALBUMS, VECGENRES
 */
class CMusicDatabase : public CDatabase
{
  friend class DatabaseUtils;
  friend class TestDatabaseUtilsHelper;

public:
  CMusicDatabase(void);
  ~CMusicDatabase(void) override;

  bool Open() override;
  bool CommitTransaction() override;
  void EmptyCache();
  void Clean();
  int  Cleanup(CGUIDialogProgress* progressDialog = nullptr);
  bool LookupCDDBInfo(bool bRequery=false);
  void DeleteCDDBInfo();

  /////////////////////////////////////////////////
  // Song CRUD
  /////////////////////////////////////////////////
  /*! \brief Add a song to the database
   \param objAlbum [in] the odb object of the album for the song
   \param strTitle [in] the title of the song (required to be non-empty)
   \param strMusicBrainzTrackID [in] the MusicBrainz track ID of the song
   \param strPathAndFileName [in] the path and filename to the song
   \param strComment [in] the ids of the added songs
   \param strMood [in] the mood of the added song
   \param strThumb [in] the ids of the added songs
   \param artistDisp [in] the assembled artist name(s) display string
   \param artistSort [in] the artist name(s) sort string
   \param genres [in] a vector of genres to which this song belongs
   \param iTrack [in] the track number and disc number of the song
   \param iDuration [in] the duration of the song
   \param iYear [in] the year of the song
   \param iTimesPlayed [in] the number of times the song has been played
   \param iStartOffset [in] the start offset of the song (when using a single audio file with a .cue)
   \param iEndOffset [in] the end offset of the song (when using a single audio file with .cue)
   \param dtLastPlayed [in] the time the song was last played
   \param rating [in] a rating for the song
   \param userrating [in] a userrating (my rating) for the song
   \param votes [in] a vote counter for the song rating
   \param replayGain [in] album and track replaygain and peak values
   \return the odb object of the song
   */
  std::shared_ptr<CODBSong> AddSong(const std::shared_ptr<CODBAlbum> objAlbum,
    const std::string& strTitle, 
    const std::string& strMusicBrainzTrackID,
    const std::string& strPathAndFileName, 
    const std::string& strComment,
    const std::string& strMood, 
    const std::string& strThumb,
    const std::string &artistDisp, 
    const std::string &artistSort,
    const std::vector<std::string>& genres,
    int iTrack, 
    int iDuration, 
    int iYear,
    const int iTimesPlayed, 
    int iStartOffset, 
    int iEndOffset,
    const CDateTime& dtLastPlayed, 
    float rating, 
    int userrating, 
    int votes,
    const ReplayGain& replayGain);

  bool GetSong(int idSong, CSong& song);

  /*! \brief Update a song and all its nested entities (genres, artists, contributors)
    \param song [in/out] the song to update, artist ids are returned in artist credits
    \param bArtists to update artist credits and contributors, default is true
    \return true if sucessfull
   */
  bool UpdateSong(CSong& song, bool bArtists = true);
  
  /*! \brief Update a song in the database.

   NOTE: This function assumes that song.artist contains the artist string to be concatenated.
         Most internal functions should instead use the long-form function as the artist string
         should be constructed from the artist credits.
         This function will eventually be demised.

   \param objSong  the odb object of the song to update
   \param song the song
   \return the odb object of the song.
   */
  std::shared_ptr<CODBSong> UpdateSong(std::shared_ptr<CODBSong> objSong, const CSong &song);

  /*! \brief Update a song in the database
   \param objSong [in] the odbObject of the song to update
   \param strTitle [in] the title of the song (required to be non-empty)
   \param strMusicBrainzTrackID [in] the MusicBrainz track ID of the song
   \param strPathAndFileName [in] the path and filename to the song
   \param strComment [in] the ids of the added songs
   \param strMood [in] the mood of the added song
   \param strThumb [in] the ids of the added songs
   \param artistDisp [in] the artist name(s) display string
   \param artistSort [in] the artist name(s) sort string
   \param genres [in] a vector of genres to which this song belongs
   \param iTrack [in] the track number and disc number of the song
   \param iDuration [in] the duration of the song
   \param iYear [in] the year of the song
   \param iTimesPlayed [in] the number of times the song has been played
   \param iStartOffset [in] the start offset of the song (when using a single audio file with a .cue)
   \param iEndOffset [in] the end offset of the song (when using a single audio file with .cue)
   \param dtLastPlayed [in] the time the song was last played
   \param rating [in] a rating for the song
   \param userrating [in] a userrating (my rating) for the song
   \param votes [in] a vote counter for the song rating
   \param replayGain [in] album and track replaygain and peak values
   \return the odbObject of the song
   */
  std::shared_ptr<CODBSong> UpdateSong(std::shared_ptr<CODBSong> objSong,
    const std::string& strTitle, 
    const std::string& strMusicBrainzTrackID,
    const std::string& strPathAndFileName, 
    const std::string& strComment,
    const std::string& strMood, 
    const std::string& strThumb,
    const std::string &artistDisp, 
    const std::string &artistSort,
    const std::vector<std::string>& genres,
    int iTrack, 
    int iDuration, 
    int iYear,
    int iTimesPlayed, 
    int iStartOffset, 
    int iEndOffset,
    const CDateTime& dtLastPlayed, 
    float rating, 
    int userrating, 
    int votes,
    const ReplayGain& replayGain);

  //// Misc Song
  bool GetSongByFileName(const std::string& strFileName, CSong& song, int64_t startOffset = 0);
  bool GetSongsByPath(const std::string& strPath, MAPSONGS& songs, bool bAppendToMap = false);
  bool Search(const std::string& search, CFileItemList &items);
  bool RemoveSongsFromPath(const std::string &path, MAPSONGS& songs, bool exact=true);
  bool SetSongUserrating(const std::string &filePath, int userrating);
  bool SetSongUserrating(int idSong, int userrating);
  bool SetSongVotes(const std::string &filePath, int votes);
  int  GetSongByArtistAndAlbumAndTitle(const std::string& strArtist, const std::string& strAlbum, const std::string& strTitle);

  /////////////////////////////////////////////////
  // Album
  /////////////////////////////////////////////////
  /*! \brief Add an album and all its songs to the database
  \param album the album to add
  \param idSource the music source id
  \return the id of the album
  */
  bool AddAlbum(CAlbum& album, int idSource);

  /*! \brief Update an album and all its nested entities (artists, songs etc)
   \param album the album to update
   \return true or false
   */
  bool UpdateAlbum(CAlbum& album);

  /*! \brief Add an album to the database
   \param strAlbum the album title
   \param strMusicBrainzAlbumID the Musicbrainz Id
   \param strArtist the album artist name(s) display string
   \param strArtistSort the album artist name(s) sort string
   \param strGenre the album genre(s)
   \param year the year
   \param strRecordLabel the recording label
   \param strType album type (Musicbrainz release type e.g. "Broadcast, Soundtrack, live"),
   \param bCompilation if the album is a compilation
   \param releaseType "album" or "single"
   \return the id of the album
   */
  std::shared_ptr<CODBAlbum> AddAlbum(const std::string& strAlbum, 
    const std::string& strMusicBrainzAlbumID,
    const std::string& strReleaseGroupMBID,
    const std::string& strArtist, 
    const std::string& strArtistSort, 
    const std::string& strGenre, 
    int year,
    const std::string& strRecordLabel, 
    const std::string& strType,
    bool bCompilation, 
    CAlbum::ReleaseType releaseType);
    
  /*! \brief retrieve an album, optionally with all songs.
   \param idAlbum the database id of the album.
   \param album [out] the album to fill.
   \param getSongs whether or not to retrieve songs, defaults to true.
   \return true if the album is retrieved, false otherwise.
   */
  bool GetAlbum(int idAlbum, CAlbum& album, bool getSongs = true);
  std::shared_ptr<CODBAlbum> GetODBAlbum(int idAlbum);
  std::shared_ptr<CODBAlbum> UpdateAlbum(int idAlbum,
    const std::string& strAlbum, 
    const std::string& strMusicBrainzAlbumID,
    const std::string& strReleaseGroupMBID,
    const std::string& strArtist, 
    const std::string& strArtistSort,
    const std::string& strGenre,
    const std::string& strMoods, 
    const std::string& strStyles,
    const std::string& strThemes, 
    const std::string& strReview,
    const std::string& strImage, 
    const std::string& strLabel,
    const std::string& strType,
    float fRating, 
    int iUserrating, 
    int iVotes, 
    int iYear, 
    bool bCompilation,
    CAlbum::ReleaseType releaseType,
    bool bScrapedMBID);
  bool ClearAlbumLastScrapedTime(int idAlbum);
  bool HasAlbumBeenScraped(int idAlbum);

  /////////////////////////////////////////////////
  // Audiobook
  /////////////////////////////////////////////////
  bool AddAudioBook(const CFileItem& item);
  bool SetResumeBookmarkForAudioBook(const CFileItem& item, int bookmark);
  bool GetResumeBookmarkForAudioBook(const std::string& path, int& bookmark);

  /*! \brief Checks if the given path is inside a folder that has already been scanned into the library
   \param path the path we want to check
   */
  bool InsideScannedPath(const std::string& path);

  //// Misc Album
  int  GetAlbumIdByPath(const std::string& path);
  bool GetAlbumFromSong(int idSong, CAlbum &album);
  int  GetAlbumByName(const std::string& strAlbum, const std::string& strArtist="");
  int  GetAlbumByName(const std::string& strAlbum, const std::vector<std::string>& artist);
  int  GetAlbumByMatch(const CAlbum &album);
  std::string GetAlbumById(int id);
  bool SetAlbumUserrating(const int idAlbum, int userrating);

  /////////////////////////////////////////////////
  // Artist CRUD
  /////////////////////////////////////////////////
  bool UpdateArtist(const CArtist& artist);

  std::shared_ptr<CODBPerson> AddArtist(const std::string& strArtist, const std::string& strMusicBrainzArtistID, const std::string& strSortName, bool bScrapedMBID = false);
  std::shared_ptr<CODBPerson> AddArtist(const std::string& strArtist, const std::string& strMusicBrainzArtistID, bool bScrapedMBID = false);
  bool GetArtist(int idArtist, CArtist& artist, bool fetchAll = true);
  bool GetArtistExists(int idArtist);
  int GetLastArtist();
  std::shared_ptr<CODBPerson> UpdateArtist(std::shared_ptr<CODBPerson> objArtist,
                                  const std::string& strArtist, 
                                  const std::string& strSortName,
                                  const std::string& strMusicBrainzArtistID, 
                                  const bool bScrapedMBID,
                                  const std::string& strType,
                                  const std::string& strGender,
                                  const std::string& strDisambiguation,
                                  const std::string& strBorn, 
                                  const std::string& strFormed,
                                  const std::vector<std::string>& genres, 
                                  const std::string& strMoods,
                                  const std::string& strStyles, 
                                  const std::string& strInstruments,
                                  const std::string& strBiography, 
                                  const std::string& strDied,
                                  const std::string& strDisbanded, 
                                  const std::string& strYearsActive,
                                  const std::string& strImage, 
                                  const std::string& strFanart);
  bool UpdateArtistScrapedMBID(int idArtist, const std::string& strMusicBrainzArtistID);
  bool GetTranslateBlankArtist() { return m_translateBlankArtist; }
  void SetTranslateBlankArtist(bool translate) { m_translateBlankArtist = translate; }
  bool HasArtistBeenScraped(int idArtist);
  bool ClearArtistLastScrapedTime(int idArtist);
  bool  AddArtistDiscography(std::shared_ptr<CODBPerson> objPerson, const std::string& strAlbum, const std::string& strYear);
  bool DeleteArtistDiscography(int idArtist);
  bool GetArtistDiscography(int idArtist, CFileItemList& items);

  std::string GetArtistById(int id);
  int GetArtistByName(const std::string& strArtist);
  int GetArtistByMatch(const CArtist& artist);
  bool GetArtistFromSong(int idSong, CArtist &artist);
  bool IsSongArtist(int idSong, int idArtist);
  bool IsSongAlbumArtist(int idSong, int idArtist);
  std::string GetRoleById(int id);

  /*! \brief Propagate artist sort name into the concatenated artist sort name strings
  held for songs and albums
  \param int idArtist to propagate sort name for, -1 means all artists
  */
  bool UpdateArtistSortNames(int idArtist = -1);

  /////////////////////////////////////////////////
  // Paths
  /////////////////////////////////////////////////
  std::shared_ptr<CODBPath> AddPath(const std::string& strPath);
  
  /*! \brief Add file an path
   \param strFileName the filename
   \param strPath thepath
   \return the id of the file
   */
  std::shared_ptr<CODBFile> AddFileAndPath(const std::string& strFileName, const std::string& strPath);

  bool GetPaths(std::set<std::string> &paths);
  bool SetPathHash(const std::string &path, const std::string &hash);
  bool GetPathHash(const std::string &path, std::string &hash);
  bool GetAlbumPaths(int idAlbum, std::vector<std::pair<std::string, int>>& paths);
  bool GetAlbumPath(int idAlbum, std::string &basePath);
  int GetDiscnumberForPathID(int idPath);
  bool GetOldArtistPath(int idArtist, std::string &path);
  bool GetArtistPath(const CArtist& artist, std::string &path);
  bool GetAlbumFolder(const CAlbum& album, const std::string &strAlbumPath, std::string &strFolder);
  bool GetArtistFolderName(const CArtist& artist, std::string &strFolder);
  bool GetArtistFolderName(const std::string &strArtist, const std::string &strMusicBrainzArtistID, std::string &strFolder);

  /////////////////////////////////////////////////
  // Sources
  /////////////////////////////////////////////////
  bool UpdateSources();
  int AddSource(const std::string& strName, const std::string& strMultipath, const std::vector<std::string>& vecPaths, int id = -1);
  int UpdateSource(const std::string& strOldName, const std::string& strName, const std::string& strMultipath, const std::vector<std::string>& vecPaths);
  bool RemoveSource(const std::string& strName);
  int GetSourceFromPath(const std::string& strPath);
  bool AddAlbumSource(int idAlbum, int idSource);
  bool AddAlbumSources(int idAlbum,  const std::string& strPath);
  bool DeleteAlbumSources(int idAlbum);
  bool GetSources(CFileItemList& items);

  bool GetSourcesByArtist(int idArtist, CFileItem* item);
  bool GetSourcesByAlbum(int idAlbum, CFileItem* item);
  bool GetSourcesBySong(int idSong, const std::string& strPath, CFileItem* item);
  int GetSourceByName(const std::string& strSource);
  std::string GetSourceById(int id);

  /////////////////////////////////////////////////
  // Genres
  /////////////////////////////////////////////////
  template <typename T>
  void SetODBDetailsGenres(T odbObject, std::vector<std::string> genres);
  std::string GetGenreById(int id);
  int GetGenreByName(const std::string& strGenre);
  bool GetGenresJSON(CFileItemList& items, bool bSources = false);

  /////////////////////////////////////////////////
  // Link tables
  /////////////////////////////////////////////////
  bool AddAlbumArtist(std::shared_ptr<CODBPerson> artist, std::shared_ptr<CODBAlbum> album, int iOrder);
  bool GetAlbumsByArtist(int idArtist, std::vector<int>& albums);
  bool GetArtistsByAlbum(int idAlbum, CFileItem* item);

  bool AddSongArtist(std::shared_ptr<CODBPerson> artist, std::shared_ptr<CODBSong> song, int iOrder, const std::string& strRole = "artist");
  std::shared_ptr<CODBPerson>  AddSongContributor(std::shared_ptr<CODBSong> objSong, const std::string& strRole, const std::string& strArtist, const std::string &strSort);
  void AddSongContributors(std::shared_ptr<CODBSong> objSong, const VECMUSICROLES& contributors, const std::string &strSort);
  int GetRoleByName(const std::string& strRole);
  std::shared_ptr<CODBRole> AddRole(const std::string& strRole);
  bool GetRolesByArtist(int idArtist, CFileItem* item);
  bool GetArtistsBySong(int idSong, std::vector<int>& artists);
  bool DeleteSongArtistsBySong(std::shared_ptr<CODBSong> objSong);
  bool GetGenresBySong(int idSong, std::vector<int>& genres);
  bool GetGenresByAlbum(int idAlbum, CFileItem* item);

  bool GetGenresByArtist(int idArtist, CFileItem* item);
  bool GetIsAlbumArtist(int idArtist, CFileItem* item);

  /////////////////////////////////////////////////
  // Top 100
  /////////////////////////////////////////////////
  bool GetTop100(const std::string& strBaseDir, CFileItemList& items);
  bool GetTop100Albums(VECALBUMS& albums);
  bool GetTop100AlbumSongs(const std::string& strBaseDir, CFileItemList& item);

  /////////////////////////////////////////////////
  // Recently added
  /////////////////////////////////////////////////
  bool GetRecentlyAddedAlbums(VECALBUMS& albums, unsigned int limit=0);
  bool GetRecentlyAddedAlbumSongs(const std::string& strBaseDir, CFileItemList& item, unsigned int limit=0);
  bool GetRecentlyPlayedAlbums(VECALBUMS& albums);
  bool GetRecentlyPlayedAlbumSongs(const std::string& strBaseDir, CFileItemList& item);

  /////////////////////////////////////////////////
  // Compilations
  /////////////////////////////////////////////////
  bool GetCompilationAlbums(const std::string& strBaseDir, CFileItemList& items);
  bool GetCompilationSongs(const std::string& strBaseDir, CFileItemList& items);
  int  GetCompilationAlbumsCount();

  int GetSinglesCount();

  int GetArtistCountForRole(int role);
  int GetArtistCountForRole(const std::string& strRole);

  /*! \brief Increment the playcount of an item
   Increments the playcount and updates the last played date
   \param item CFileItem to increment the playcount for
   */
  void IncrementPlayCount(const CFileItem &item);
  bool CleanupOrphanedItems();

  /////////////////////////////////////////////////
  // VIEWS
  /////////////////////////////////////////////////
  bool GetGenresNav(const std::string& strBaseDir, CFileItemList& items, const Filter &filter = Filter(), bool countOnly = false);
  bool GetSourcesNav(const std::string& strBaseDir, CFileItemList& items, const Filter &filter = Filter(), bool countOnly = false);
  bool GetYearsNav(const std::string& strBaseDir, CFileItemList& items, const Filter &filter = Filter());
  bool GetRolesNav(const std::string& strBaseDir, CFileItemList& items, const Filter &filter = Filter());
  bool GetArtistsNav(const std::string& strBaseDir, CFileItemList& items, bool albumArtistsOnly = false, int idGenre = -1, int idAlbum = -1, int idSong = -1, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription(), bool countOnly = false);
  bool GetCommonNav(const std::string &strBaseDir, const std::string &table, const std::string &labelField, CFileItemList &items, const Filter &filter /* = Filter() */, bool countOnly /* = false */);
  bool GetAlbumTypesNav(const std::string &strBaseDir, CFileItemList &items, const Filter &filter = Filter(), bool countOnly = false);
  bool GetMusicLabelsNav(const std::string &strBaseDir, CFileItemList &items, const Filter &filter = Filter(), bool countOnly = false);
  bool GetAlbumsNav(const std::string& strBaseDir, CFileItemList& items, int idGenre = -1, int idArtist = -1, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription(), bool countOnly = false);
  bool GetAlbumsByYear(const std::string &strBaseDir, CFileItemList& items, int year);
  bool GetSongsNav(const std::string& strBaseDir, CFileItemList& items, int idGenre, int idArtist,int idAlbum, const SortDescription &sortDescription = SortDescription());
  bool GetSongsByYear(const std::string& baseDir, CFileItemList& items, int year);
  bool GetSongsByWhere(const std::string &baseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription = SortDescription());
  bool GetSongsFullByWhere(const std::string &baseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription = SortDescription(), bool artistData = false);
  bool GetAlbumsByWhere(const std::string &baseDir, const Filter &filter, CFileItemList &items, const SortDescription &sortDescription = SortDescription(), bool countOnly = false);
  bool GetAlbumsByWhere(const std::string &baseDir, const Filter &filter, VECALBUMS& albums, int& total, const SortDescription &sortDescription = SortDescription(), bool countOnly = false);
  bool GetArtistsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription = SortDescription(), bool countOnly = false);
  bool GetRandomSong(CFileItem* item, int& idSong, odb::query<ODBView_Song> objQuery);
  int GetSongsCount(odb::query<ODBView_Song_Count> query = odb::query<ODBView_Song_Count>());
  unsigned int GetSongIDs(odb::query<ODBView_Song>& query, std::vector<std::pair<int,int> > &songIDs);
  bool GetPlaylistsNav(const std::string& strBaseDir,
                       CFileItemList& items,
                       const Filter &filter = Filter(),
                       const SortDescription &sortDescription = SortDescription(),
                       bool countOnly = false);
  bool GetPlaylistsByWhere(const std::string &baseDir,
                           const Filter &filter,
                           CFileItemList &items,
                           const SortDescription &sortDescription,
                           bool countOnly);

  template <typename T> T GetODBFilterGenres(CDbUrl &musicUrl, Filter &filter, SortDescription &sorting);
  template <typename T> T GetODBFilterArtists(CDbUrl &musicUrl, Filter &filter, SortDescription &sorting);
  template <typename T> T GetODBFilterAlbums(CDbUrl &musicUrl, Filter &filter, SortDescription &sorting);
  template <typename T> T GetODBFilterSongs(CDbUrl &musicUrl, Filter &filter, SortDescription &sorting);

  /////////////////////////////////////////////////
  // Scraper
  /////////////////////////////////////////////////
  bool SetScraper(int id, const CONTENT_TYPE &content, const ADDON::ScraperPtr scraper);
  bool SetScraperAll(const std::string& strBaseDir, const ADDON::ScraperPtr scraper);
  bool GetScraper(int id, const CONTENT_TYPE &content, ADDON::ScraperPtr& scraper);

  /*! \brief Check whether a given scraper is in use.
   \param scraperID the scraper to check for.
   \return true if the scraper is in use, false otherwise.
   */
  bool ScraperInUse(const std::string &scraperID);

  /////////////////////////////////////////////////
  // Filters
  /////////////////////////////////////////////////
  bool GetItems(const std::string &strBaseDir, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
  bool GetItems(const std::string &strBaseDir, const std::string &itemType, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
  std::string GetItemById(const std::string &itemType, int id);

  /////////////////////////////////////////////////
  // XML
  /////////////////////////////////////////////////
  void ExportToXML(const CLibExportSettings& settings, CGUIDialogProgress* progressDialog = NULL);
  void ImportFromXML(const std::string &xmlFile);

  /////////////////////////////////////////////////
  // Properties
  /////////////////////////////////////////////////
  void SetPropertiesForFileItem(CFileItem& item);
  static void SetPropertiesFromArtist(CFileItem& item, const CArtist& artist);
  static void SetPropertiesFromAlbum(CFileItem& item, const CAlbum& album);

  /////////////////////////////////////////////////
  // Art
  /////////////////////////////////////////////////
  /*! \brief Sets art for a database item.
   Sets a single piece of art for a database item.
   \param mediaId the id in the media (song/artist/album) table.
   \param mediaType the type of media, which corresponds to the table the item resides in (song/artist/album).
   \param artType the type of art to set, e.g. "thumb", "fanart"
   \param url the url to the art (this is the original url, not a cached url).
   \sa GetArtForItem
   */
  void SetArtForItem(int mediaId, const std::string &mediaType, const std::string &artType, const std::string &url);

  /*! \brief Sets art for a database item.
   Sets multiple pieces of art for a database item.
   \param mediaId the id in the media (song/artist/album) table.
   \param mediaType the type of media, which corresponds to the table the item resides in (song/artist/album).
   \param art a map of <type, url> where type is "thumb", "fanart", etc. and url is the original url of the art.
   \sa GetArtForItem
   */
  void SetArtForItem(int mediaId, const std::string &mediaType, const std::map<std::string, std::string> &art);


  /*! \brief Fetch all related art for a database item.
  Fetches multiple pieces of art for a database item including that for related media types
  Given song id art for the related album, artist(s) and albumartist(s) will also be fetched, looking up the
  album and artist when ids are not provided.
  Given album id (and not song id) art for the related artist(s) will also be fetched, looking up the
  artist(s) when id are not provided.
  \param songId the id in the song table, -1 when song art not being fetched
  \param albumId the id in the album table, -1 when album art not being fetched
  \param artistId the id in the artist table, -1 when artist not known
  \param bPrimaryArtist true if art from only the first song artist or album artist is to be fetched
  \param art [out] a vector, each element having media type e.g. "artist", "album" or "song",
  artType e.g. "thumb", "fanart", etc., prefix of "", "artist" or "albumartist" etc. giving the kind of artist
  relationship, and the original url of the art.

  \return true if art is retrieved, false if no art is found.
  \sa SetArtForItem
  */
  bool GetArtForItem(int songId, int albumId, int artistId, bool bPrimaryArtist, std::vector<ArtForThumbLoader> &art);

  /*! \brief Fetch art for a database item.
   Fetches multiple pieces of art for a database item.
   \param mediaId the id in the media (song/artist/album) table.
   \param mediaType the type of media, which corresponds to the table the item resides in (song/artist/album).
   \param art [out] a map of <type, url> where type is "thumb", "fanart", etc. and url is the original url of the art.
   \return true if art is retrieved, false if no art is found.
   \sa SetArtForItem
   */
  bool GetArtForItem(int mediaId, const std::string &mediaType, std::map<std::string, std::string> &art);

  /*! \brief Fetch art for a database item.
   Fetches a single piece of art for a database item.
   \param mediaId the id in the media (song/artist/album) table.
   \param mediaType the type of media, which corresponds to the table the item resides in (song/artist/album).
   \param artType the type of art to retrieve, eg "thumb", "fanart".
   \return the original URL to the piece of art, if available.
   \sa SetArtForItem
   */
  std::string GetArtForItem(int mediaId, const std::string &mediaType, const std::string &artType);

  /*! \brief Remove art for a database item.
  Removes  a single piece of art for a database item.
  \param mediaId the id in the media (song/artist/album) table.
  \param mediaType the type of media, which corresponds to the table the item resides in (song/artist/album).
  \param artType the type of art to remove, eg "thumb", "fanart".
  \return true if art is removed, false if no art is found.
  \sa RemoveArtForItem
  */
  bool RemoveArtForItem(int mediaId, const MediaType &mediaType, const std::string &artType);

  /*! \brief Remove art for a database item.
  Removes multiple pieces of art for a database item.
  \param mediaId the id in the media (song/artist/album) table.
  \param mediaType the type of media, which corresponds to the table the item resides in (song/artist/album).
  \param arttypes a set of types, e.g. "thumb", "fanart", etc. to be removed.
  \return true if art is removed, false if no art is found.
  \sa RemoveArtForItem
  */
  bool RemoveArtForItem(int mediaId, const MediaType &mediaType, const std::set<std::string> &artTypes);

  /*! \brief Fetch the distinct types of art held in the database for a type of media.
  \param mediaType the type of media, which corresponds to the table the item resides in (song/artist/album).
  \param artTypes [out] the types of art e.g. "thumb", "fanart", etc.
  \return true if art is found, false if no art is found.
  */
  bool GetArtTypes(const MediaType &mediaType, std::vector<std::string> &artTypes);

  /////////////////////////////////////////////////
  // Tag Scan Version
  /////////////////////////////////////////////////
  /*! \brief Check if music files need all tags rescanning regardless of file being unchanged
  because the tag processing has changed (which may happen without db version changes) since they
  where last scanned.
  \return -1 if an error occured, 0 if no scan is needed, or the version number of tags if not the same as current.
  */
  virtual int GetMusicNeedsTagScan();

  /*! \brief Set minimum version number of db needed when tag data scanned from music files
  \param version the version number of db
  */
  void SetMusicNeedsTagScan(int version);

  /*! \brief Set the version number of tag data
  \param version the version number of db when tags last scanned, 0 (default) means current db version
  */
  void SetMusicTagScanVersion(int version = 0);

std::string GetLibraryLastUpdated();
void SetLibraryLastUpdated();

protected:
  std::map<std::string, int> m_genreCache;
  std::map<std::string, std::shared_ptr<CODBPath>> m_pathCache;

  CCommonDatabase m_cdb;

  void CreateTables() override;
  void CreateAnalytics() override;
  int GetMinSchemaVersion() const override { return 32; }
  int GetSchemaVersion() const override;

  const char *GetBaseDBName() const override { return "MyMusic"; };

private:
  /*! \brief (Re)Create the generic database views for songs and albums
   */
  virtual void CreateViews();

  void SplitPath(const std::string& strFileNameAndPath, std::string& strPath, std::string& strFileName);
  CSong GetSongFromODBObject(std::shared_ptr<CODBSong> objSong);
  CArtist GetArtistFromODBObject(std::shared_ptr<CODBPerson> objArtist, bool needThumb = true);
  CAlbum GetAlbumFromODBObject(std::shared_ptr<CODBAlbum> objAlbum, bool imageURL = false);
  CArtistCredit GetArtistCreditFromODBObject(std::shared_ptr<CODBPersonLink> objPersonLinks);
  CMusicRole GetArtistRoleFromODBObject(std::shared_ptr<CODBPersonLink> objArtist);

  /*! \brief Updates the dateAdded field in the song table for the file
  with the given songId and the given path based on the files modification date
  \param objFile odb object of the file
  \param strFileNameAndPath path to the file
  */
  void UpdateFileDateAdded(const std::shared_ptr<CODBFile> objFile, const std::string& strFileNameAndPath);
  void GetFileItemFromODBObject(std::shared_ptr<CODBSong> objSong, CFileItem* item, const CMusicDbUrl &baseUrl);
  void GetFileItemFromArtistCredits(VECARTISTCREDITS& artistCredits, CFileItem* item);
  bool CleanupSongs(CGUIDialogProgress* progressDialog = nullptr);
  bool CleanupPaths();
  bool CleanupAlbums();
  bool CleanupArtists();
  bool CleanupGenres();
  bool CleanupInfoSettings();
  bool CleanupRoles();
  void UpdateTables(int version) override;
  bool SearchArtists(const std::string& search, CFileItemList &artists);
  bool SearchAlbums(const std::string& search, CFileItemList &albums);
  bool SearchSongs(const std::string& strSearch, CFileItemList &songs);
  std::shared_ptr<CODBSong> GetSongObjFromPath(const std::string &filePath);

  /*! \brief Checks that source table matches sources.xml
  returns true when they do 
  */
  bool CheckSources(VECSOURCES& sources);

  /*! \brief Initially fills source table from sources.xml for use only at
  migration of db from an earlier version than 72
  returns true when successfuly done
  */
  bool MigrateSources();

  bool m_translateBlankArtist;

  // Fields should be ordered as they
  // appear in the songview
  static enum _SongFields
  {
    song_idSong=0,
    song_strArtists,
    song_strArtistSort,
    song_strGenres,
    song_strTitle,
    song_iTrack,
    song_iDuration,
    song_iYear,
    song_strFileName,
    song_strMusicBrainzTrackID,
    song_iTimesPlayed,
    song_iStartOffset,
    song_iEndOffset,
    song_lastplayed,
    song_rating,
    song_userrating,
    song_votes,
    song_comment,
    song_idAlbum,
    song_strAlbum,
    song_strPath,
    song_bCompilation,
    song_strAlbumArtists,
    song_strAlbumArtistSort,
    song_strAlbumReleaseType,
    song_mood,
    song_dateAdded,
    song_strReplayGain,
    song_enumCount // end of the enum, do not add past here
  } SongFields;

  // Fields should be ordered as they
  // appear in the albumview
  static enum _AlbumFields
  {
    album_idAlbum=0,
    album_strAlbum,
    album_strMusicBrainzAlbumID,
    album_strReleaseGroupMBID,
    album_strArtists,
    album_strArtistSort,
    album_strGenres,
    album_iYear,
    album_strMoods,
    album_strStyles,
    album_strThemes,
    album_strReview,
    album_strLabel,
    album_strType,
    album_strThumbURL,
    album_fRating,
    album_iUserrating,
    album_iVotes,
    album_bCompilation,
    album_bScrapedMBID,
    album_lastScraped,
    album_iTimesPlayed,
    album_strReleaseType,
    album_dtDateAdded,
    album_dtLastPlayed,
    album_enumCount // end of the enum, do not add past here
  } AlbumFields;

  // Fields should be ordered as they
  // appear in the songartistview/albumartistview
  static enum _ArtistCreditFields
  {
    // used for GetAlbum to get the cascaded album/song artist credits
    artistCredit_idEntity = 0,  // can be idSong or idAlbum depending on context
    artistCredit_idArtist,
    artistCredit_idRole,
    artistCredit_strRole,
    artistCredit_strArtist,
    artistCredit_strSortName,
    artistCredit_strMusicBrainzArtistID,
    artistCredit_iOrder,
    artistCredit_enumCount
  } ArtistCreditFields;

  // Fields should be ordered as they
  // appear in the artistview
  static enum _ArtistFields
  {
    artist_idArtist=0,
    artist_strArtist,
    artist_strSortName,
    artist_strMusicBrainzArtistID,
    artist_strType,
    artist_strGender,
    artist_strDisambiguation,
    artist_strBorn,
    artist_strFormed,
    artist_strGenres,
    artist_strMoods,
    artist_strStyles,
    artist_strInstruments,
    artist_strBiography,
    artist_strDied,
    artist_strDisbanded,
    artist_strYearsActive,
    artist_strImage,
    artist_strFanart,
    artist_bScrapedMBID,
    artist_lastScraped,
    artist_dtDateAdded,
    artist_enumCount // end of the enum, do not add past here
  } ArtistFields;

};
