/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IGameClientPlayback.h"
#include "GameLoop.h"
#include "games/addons/streams/GameClientStreamHwFramebuffer.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

#include <memory>
#include <stddef.h>
#include <stdint.h>

namespace KODI
{
namespace GAME
{
  class CGameClient;
  class CSavestateReader;
  class CSavestateWriter;
  class IMemoryStream;

  class CGameClientReversiblePlayback : public IGameClientPlayback,
                                        public IGameLoopCallback,
                                        public IHwFramebufferCallback,
                                        public Observer
  {
  public:
    CGameClientReversiblePlayback(CGameClient* gameClient, double fps, size_t serializeSize);

    virtual ~CGameClientReversiblePlayback();

    // implementation of IGameClientPlayback
    virtual bool CanPause() const override { return true; }
    virtual bool CanSeek() const override { return true; }
    virtual unsigned int GetTimeMs() const override { return m_playTimeMs; }
    virtual unsigned int GetTotalTimeMs() const override { return m_totalTimeMs; }
    virtual unsigned int GetCacheTimeMs() const override { return m_cacheTimeMs; }
    virtual void SeekTimeMs(unsigned int timeMs) override;
    virtual double GetSpeed() const override;
    virtual void SetSpeed(double speedFactor) override;
    virtual void PauseAsync() override;
    virtual std::string CreateSavestate() override;
    virtual bool LoadSavestate(const std::string& path) override;

    // implementation of IGameLoopCallback
    virtual void FrameEvent() override;
    virtual void RewindEvent() override;

    // implementation of Observer
    virtual void Notify(const Observable &obs, const ObservableMessage msg) override;

    // Implementation of IHwFramebufferCallback
    void HardwareContextReset() override;
    void CreateHwContext() override;

  private:
    void AddFrame();
    void RewindFrames(uint64_t frames);
    void AdvanceFrames(uint64_t frames);
    void UpdatePlaybackStats();
    void UpdateMemoryStream();

    // Construction parameter
    CGameClient* const m_gameClient;

    // Gameplay functionality
    CGameLoop m_gameLoop;
    std::unique_ptr<IMemoryStream> m_memoryStream;
    CCriticalSection m_mutex;

    // Savestate functionality
    std::unique_ptr<CSavestateWriter> m_savestateWriter;
    std::unique_ptr<CSavestateReader> m_savestateReader;

    // Playback stats
    uint64_t m_totalFrameCount;
    uint64_t m_pastFrameCount;
    uint64_t m_futureFrameCount;
    unsigned int m_playTimeMs;
    unsigned int m_totalTimeMs;
    unsigned int m_cacheTimeMs;
  };
}
}
